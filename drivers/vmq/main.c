
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
#include <linux/seq_file.h>
#endif
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/file.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/mempool.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include "vmq-dev.h"
#include "balloc.h"
#include "debug.h"

/*****************************************************************************/

#define	VMQ_MINOR		MISC_DYNAMIC_MINOR
#define	VMQ_MODULE_NAME		"vmq"
#define	VMQ_MAX_QUEUE_COUNT	32
#define	VMQ_PERM_MASK		0666
#define	VMQ_FLAG_MASK		1

/*****************************************************************************/

struct VMQBuffer {
	int size;
	int offset;
	atomic_t refcnt;
	struct VMQBufferManager *mgr;
};

struct VMQItem {
	struct VMQBuffer *buffer;
	struct list_head entry;
};

struct VMQBufferManager {
	int size;						/* 공유메모리 크기 */
	int page_size;					/* 할당의 최소 단위 */
	struct VMSharedMemory *mem;		/* 이 큐가 사용하는 공유 메모리 */
	struct BuddyAllocator *ba;		/* 공유메모리 Allocator */
	struct mutex ba_mutex;
	struct list_head entry;			/* 버퍼 리스트용 */
	wait_queue_head_t b_wq;			/* 버퍼 waitq */
	wait_queue_head_t q_wq;			/* 큐 waitq */
	atomic_t refcnt;				/* 참조 카운터 */
	mempool_t *pool;
	struct kmem_cache *cache;
	struct mutex mutex;
	struct list_head consumers;		/* 이 버퍼와 연관된 소비자 리스트 */
	struct list_head producers;;	/* 이 버퍼와 연관된 생산자 리스트 */
	struct proc_dir_entry *proc_entry;
	/* for proc interface debugging */
	int bufItemCount;
	int bufItemAllocCount;
	int bufItemRemoveCount;
	int bufFullCount;

	int maxItemCount;
};

struct VMQProducer {
	struct VMQBufferManager *mgr;	/* 사용할 버퍼 */
	struct VMQBuffer *buffer;		/* 현재 할당중인 버퍼 item */
	struct list_head entry;			/* producer간의 리스트 연결용 */
};

struct VMQConsumer {
	struct VMQBufferManager *mgr;	/* 사용할 버퍼 */
	struct list_head items;			/* VMQItem 리스트 */
	struct list_head entry;			/* 컨슈머들간의 리스트 연결용 */
	struct VMQItem *item;			/* 현재 item */
	struct mutex mutex;
};

struct VMQContext {
	int type;						/* VMQ_TYPE_* */
	union {
		struct VMQProducer producer;
		struct VMQConsumer consumer;
	} u;
};

/*****************************************************************************/

static LIST_HEAD(vmq_buffer_list);
static DEFINE_MUTEX(vmq_buffer_mutex);
static DEFINE_MUTEX(cachep_mutex);
static struct kmem_cache *vmq_buffer_cachep;
static struct kmem_cache *vmq_item_cachep;

/*****************************************************************************/

static struct proc_dir_entry *vmq_proc_root;

/*****************************************************************************/

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
static int vmq_proc_read(char* page, char **start, 
		off_t off, int count, int* eof, void* data)
{
	int n = 0;
	struct VMQBufferManager *mgr = (struct VMQBufferManager *)data;

	n += sprintf(&page[n], "current   : %d\n", mgr->bufItemCount);
	n += sprintf(&page[n], "alloc     : %d\n", mgr->bufItemAllocCount);
	n += sprintf(&page[n], "free      : %d\n", mgr->bufItemRemoveCount);
	n += sprintf(&page[n], "full      : %d\n", mgr->bufFullCount);
	n += sprintf(&page[n], "max count : %d\n", mgr->maxItemCount);
	*eof = 1;

	return n;
}
#else
static int vmq_proc_show (struct seq_file *m, void *v)
{
	struct VMQBufferManager *mgr = (struct VMQBufferManager *)m->private;
	DBG("vmq_proc_show mgr = %p\n", mgr);
	if (mgr) {
		seq_printf(m, "current   : %d\n", mgr->bufItemCount);
		seq_printf(m, "alloc     : %d\n", mgr->bufItemAllocCount);
		seq_printf(m, "free      : %d\n", mgr->bufItemRemoveCount);
		seq_printf(m, "full      : %d\n", mgr->bufFullCount);
		seq_printf(m, "max count : %d\n", mgr->maxItemCount);
	}
	return 0;
}
static int vmq_proc_open(struct inode *inode, struct file *file)
{
	struct VMQBufferManager *mgr = PDE_DATA(file_inode(file));
	DBG("vmq_proc_open mgr = %p\n", mgr);
	return single_open(file, vmq_proc_show, (void *)mgr);
}

static const struct file_operations proc_fops = {
	.open       = vmq_proc_open,
	.read       = seq_read,
	.llseek     = seq_lseek,
	.release    = single_release,
};
#endif

/*****************************************************************************/

static int alloc_shared_memory(struct VMQBufferManager *mgr, int size)
{
	int index;
	mutex_lock(&mgr->ba_mutex);
	index = ba_alloc(mgr->ba, size);
//	ba_dump_bitmap(mgr->ba);
	mutex_unlock(&mgr->ba_mutex);
//	DBG("Alloc page index = %d, size = %d\n", index, size);
	return index;
}

static void free_shared_memory(struct VMQBufferManager *mgr, int index)
{
	mutex_lock(&mgr->ba_mutex);
	ba_free(mgr->ba, index);
//	ba_dump_bitmap(mgr->ba);
	mutex_unlock(&mgr->ba_mutex);
//	DBG("Free page index = %d\n", index);
}

static void dump_shared_memory(struct VMQBufferManager *mgr)
{
	mutex_lock(&mgr->ba_mutex);
	ba_dump(mgr->ba);
	ba_dump_bitmap(mgr->ba);
	mutex_unlock(&mgr->ba_mutex);
}

static inline struct VMQBuffer *alloc_buffer_item(struct VMQBufferManager *mgr)
{
	struct VMQBuffer *buffer;
//	buffer = kmem_cache_zalloc(vmq_buffer_cachep, GFP_KERNEL);
	mutex_lock (&cachep_mutex);
	buffer = kmem_cache_zalloc(vmq_buffer_cachep, GFP_ATOMIC);
	mutex_unlock (&cachep_mutex);
#if 0
	buffer->mgr = mgr;
	mgr->bufItemCount++;
	mgr->bufItemAllocCount++;
#endif
	return buffer;
}

static inline void free_buffer_item(struct VMQBuffer *buffer)
{
	struct VMQBufferManager *mgr = buffer->mgr;

	mgr->bufItemCount--;
	mgr->bufItemRemoveCount++;

	mutex_lock (&cachep_mutex);	
	kmem_cache_free(vmq_buffer_cachep, buffer);
	mutex_unlock (&cachep_mutex);
}

static inline void remove_buffer_item(struct VMQBuffer *buffer)
{
	int index;
	index = buffer->offset / buffer->mgr->page_size;
	free_shared_memory(buffer->mgr, index);
	free_buffer_item(buffer);
}

static inline int unref_buffer(struct VMQBuffer *buffer)
{
	if(atomic_dec_and_test(&buffer->refcnt)) {
		remove_buffer_item(buffer);
		return 1;
	}
	return 0;
}

static struct VMQBufferManager *lookup_buffer_manager(const char *name)
{
	struct VMQBufferManager *mgr;
	char shmname[VMQ_MAX_NAME_LENGTH];

	mutex_lock(&vmq_buffer_mutex);
	list_for_each_entry(mgr, &vmq_buffer_list, entry) {
		vmshm_get_name(mgr->mem, shmname, VMQ_MAX_NAME_LENGTH);
		if(!strncmp(shmname, name, VMQ_MAX_NAME_LENGTH)) {
			mutex_unlock(&vmq_buffer_mutex);
			return mgr;
		}
	}
	mutex_unlock(&vmq_buffer_mutex);
	return (struct VMQBufferManager *)0;
}

static struct VMQItem *alloc_queue_item(void)
{
	struct VMQItem *item;
	mutex_lock (&cachep_mutex);
	item = kmem_cache_zalloc(vmq_item_cachep, GFP_ATOMIC);
	mutex_unlock (&cachep_mutex);
	return item;
}

static void free_queue_item(struct VMQItem *item)
{
	mutex_lock (&cachep_mutex);
	kmem_cache_free(vmq_item_cachep, item);
	mutex_unlock (&cachep_mutex);
}

/*****************************************************************************/

static struct VMQBuffer *get_buffer_item(
		struct VMQBufferManager *mgr, int size, int *err)
{
	int ret;
	int page;
	struct VMQBuffer *buffer = (struct VMQBuffer *)0;

	if(mgr->maxItemCount > 0 && mgr->bufItemCount >= mgr->maxItemCount) {
		ret = wait_event_interruptible(mgr->b_wq, (mgr->bufItemCount < mgr->maxItemCount));
		if(ret < 0) {
			*err = ret;
			goto done;
		}
	}

	page = alloc_shared_memory(mgr, size);
	if(page < 0) {
		mutex_lock(&mgr->mutex);
		mgr->bufFullCount++;
		mutex_unlock(&mgr->mutex);
		//dump_shared_memory(mgr);
		ret = wait_event_interruptible(mgr->b_wq, ((page = alloc_shared_memory(mgr, size)) >= 0));
		if(ret < 0) {
			*err = ret;
			goto done;
		}
	}

	mutex_lock(&mgr->mutex);
	buffer = alloc_buffer_item(mgr);
	if(unlikely(!buffer)) {
		*err = -ENOMEM;
		mutex_unlock(&mgr->mutex);
		goto done;
	}
	mgr->bufItemCount++;
	mgr->bufItemAllocCount++;
	buffer->mgr = mgr;
	mutex_unlock(&mgr->mutex);

	buffer->offset = page * mgr->page_size;
	buffer->size = size;
	atomic_set(&buffer->refcnt, 0);

done:
	return buffer;
}

static void put_buffer_item (struct VMQBufferManager *mgr, struct VMQBuffer *buffer)
{
	int count = 0;
	struct VMQItem *item = (struct VMQItem *)0;
	struct VMQConsumer *con;

	/* Consumer들의 큐에 넣어준다. */
	mutex_lock(&mgr->mutex);
	list_for_each_entry(con, &mgr->consumers, entry) {
		item = alloc_queue_item();
		if(unlikely(!item)) {
			printk("No memory...\n");
			continue;
		}
		INIT_LIST_HEAD(&item->entry);
		atomic_inc(&buffer->refcnt);
		item->buffer = buffer;
		mutex_lock(&con->mutex);
		list_add_tail(&item->entry, &con->items);
		mutex_unlock(&con->mutex);
		count++;
	}

	if(count == 0)
		remove_buffer_item(buffer);
	else
		wake_up_interruptible_all(&mgr->q_wq);
	mutex_unlock(&mgr->mutex);
}

/*****************************************************************************/

static inline struct VMQItem *get_first_queue_item(struct VMQConsumer *con)
{
	struct VMQItem *item = (struct VMQItem *)0;
	mutex_lock(&con->mutex);
	if(!list_empty(&con->items)) {
		item = list_first_entry(&con->items, struct VMQItem, entry);
		list_del(&item->entry);
	}
	mutex_unlock(&con->mutex);
	return item;
}

static struct VMQItem *get_queue_item(struct VMQConsumer *con, int *err)
{
	int ret;
	struct VMQItem *item = (struct VMQItem *)0;

	item = get_first_queue_item(con);
	if(!item) {
		DBG("Queue is empty, wait for event.\n");
		ret = wait_event_interruptible(con->mgr->q_wq, 
				(item = get_first_queue_item(con)) != (struct VMQItem *)0);
		if(ret < 0) {
			*err = ret;
			goto done;
		}
	}

done:
	return item;
}

static inline void put_queue_item(struct VMQItem *item)
{
	struct VMQBuffer *buffer = item->buffer;
	struct VMQBufferManager *mgr = buffer->mgr;

	mutex_lock (&mgr->mutex);
	free_queue_item(item);

	if(unref_buffer(buffer))
		wake_up_interruptible_all(&mgr->b_wq);
	mutex_unlock (&mgr->mutex);
}

/*****************************************************************************/

static struct VMQBufferManager *create_buffer_manager(
		const char *name, struct VMSharedMemory *mem, 
		int size, int page_size, int maxItemCount, int *err)
{
	struct VMQBufferManager *mgr;

	mgr = kzalloc(sizeof(*mgr), GFP_KERNEL);
	if(unlikely(!mgr)) {
		*err = -ENOMEM;
		goto done;
	}

	mgr->size = size;
	mgr->page_size = page_size;
	mgr->mem = mem;
	mgr->maxItemCount = maxItemCount;
	mgr->ba = ba_create(size, page_size);
	if(unlikely(!mgr->ba)) {
		*err = -ENOMEM;
		goto err1;
	}
	ba_dump(mgr->ba);

	INIT_LIST_HEAD(&mgr->entry);
	atomic_set(&mgr->refcnt, 0);
	mutex_init(&mgr->ba_mutex);
	mutex_init(&mgr->mutex);
	INIT_LIST_HEAD(&mgr->consumers);
	INIT_LIST_HEAD(&mgr->producers);
	init_waitqueue_head(&mgr->b_wq);
	init_waitqueue_head(&mgr->q_wq);

	*err = 0;
	return mgr;

err1:
	kfree(mgr);
	mgr = (struct VMQBufferManager *)0;

done:
	return mgr;
}

static void remove_buffer_manager(struct VMQBufferManager *mgr)
{
	vmshm_unselect(mgr->mem);
	vmshm_remove(mgr->mem);
	ba_remove(mgr->ba);
	kfree(mgr);
}

/*****************************************************************************/

static int vmq_create(struct file *filp, unsigned long arg)
{
	int err;
	int ret = -EINVAL;
	struct VMQCreateParam crarg;
	struct VMQBufferManager *mgr;
	struct VMSharedMemory *mem;
	char name[VMQ_MAX_NAME_LENGTH];

	if(unlikely(!try_module_get(THIS_MODULE))) {
		ret = -EINVAL;
		goto done;
	}

	ret = copy_from_user(&crarg, (const void __user *)arg, sizeof(crarg));
	if(unlikely(ret)) {
		ret = -EFAULT;
		goto err1;
	}

	ret = strncpy_from_user(name, crarg.name, VMQ_MAX_NAME_LENGTH);
	if(unlikely(ret < 0)) {
		goto err1;
	}
	name[ret] = 0;
	
	if(!name[0]) {
		ret = -EINVAL;
		goto err1;
	}
	DBG("name = %s\n", name);

	mem = vmshm_create(name, crarg.size, crarg.perm, crarg.flags, &err);
	if(unlikely(!mem)) {
		ret = err;
		goto err1;
	}

	if(vmshm_select(name) != mem) {
		ERR("Do not found vmshm(%s)\n", name);
		ret = -EFAULT;
		goto err2;
	}

	mgr = create_buffer_manager(
			name, mem, crarg.size, crarg.page_size, crarg.maxItemCount, &err);
	if(unlikely(!mgr)) {
		ret = err;
		goto err3;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
	/* create proc interface */
	mgr->proc_entry = create_proc_entry(name, 0444, vmq_proc_root);
	if(!mgr->proc_entry) {
		DBG("creation proc-entry-%s failed.\n", name);
		ret = -EFAULT;
		goto err4;
	}
	mgr->proc_entry->data = (void *)mgr;
	mgr->proc_entry->read_proc = vmq_proc_read;
#else
	/* create proc interface */
	DBG("proc_create_data (%s) mgr = %p\n", name, mgr);
	mgr->proc_entry = proc_create_data (name, 0444, vmq_proc_root, &proc_fops, (void *)mgr);
	if (!mgr->proc_entry) {
		DBG("Creation proc-entry-%s failed.\n", name);
		ret = -EFAULT;
		goto err4;
	}
#endif

	mutex_lock(&vmq_buffer_mutex);
	list_add_tail(&mgr->entry, &vmq_buffer_list);
	mutex_unlock(&vmq_buffer_mutex);

	return 0;

err4:
	remove_buffer_manager(mgr);
err3:
	vmshm_unselect(mem);
err2:
	vmshm_remove(mem);
err1:
	module_put(THIS_MODULE);

done:
	return ret;
}

static int vmq_remove(struct file *filp, unsigned long arg)
{
	int ret = -EINVAL;
	struct VMQBufferManager *mgr;
	char name[VMQ_MAX_NAME_LENGTH];

	ret = strncpy_from_user(name, (const void __user *)arg, VMQ_MAX_NAME_LENGTH);
	if(unlikely(ret < 0)) {
		goto err1;
	}
	name[ret] = 0;
	
	if(!name[0]) {
		ret = -EINVAL;
		goto err2;
	}

	mgr = lookup_buffer_manager(name);
	if(unlikely(!mgr)) {
		DBG("Unknown manager : %s\n", name);
		ret = -ENOENT;
		goto err3;
	}
	if(atomic_read(&mgr->refcnt)) {
		DBG("Busy.\n");
		ret = -EBUSY;
		goto err4;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
	/* remove proc interface */
//	remove_proc_entry ((const char *)name, mgr->proc_entry->parent);
	remove_proc_entry ((const char *)name, vmq_proc_root);
#else
	/* remove proc interface */
	remove_proc_entry ((const char *)name, vmq_proc_root);
#endif

	mutex_lock(&vmq_buffer_mutex);
	list_del(&mgr->entry);
	mutex_unlock(&vmq_buffer_mutex);

	remove_buffer_manager(mgr);
	module_put(THIS_MODULE);
	ret = 0;

err4:
err3:
err2:
err1:
	return ret;
}

static int vmq_setup(struct file *filp, unsigned long arg)
{
	int ret = -EINVAL;
	struct VMQSetupParam setup;
	struct VMQBufferManager *mgr;
	struct VMQContext *ctxt;
	char name[VMQ_MAX_NAME_LENGTH];

	if(unlikely(filp->private_data)) {
		ret = -EINVAL;
		goto done;
	}

	ret = copy_from_user(&setup,
			(const void __user *)arg, sizeof(setup));
	if(unlikely(ret)) {
		ret = -EFAULT;
		goto err1;
	}

	ret = strncpy_from_user(name, setup.name, VMQ_MAX_NAME_LENGTH);
	if(unlikely(ret < 0)) {
		goto err2;
	}
	name[ret] = 0;
	
	if(!name[0]) {
		ret = -EINVAL;
		goto err3;
	}

	DBG("Setup with %s\n", name);
	mgr = lookup_buffer_manager(name);
	if(unlikely(!mgr)) {
		ret = -EINVAL;
		goto err4;
	}

	ctxt = (struct VMQContext *)kzalloc(sizeof(*ctxt), GFP_KERNEL);
	if(unlikely(!ctxt)) {
		ret = -ENOMEM;
		goto err5;
	}

	if(setup.type == VMQ_TYPE_PRODUCER) {
		DBG("[%p-%s] Producer setup.\n", mgr, name);
		ctxt->type = VMQ_TYPE_PRODUCER;
		atomic_inc(&mgr->refcnt);
		ctxt->u.producer.mgr = mgr;
		ctxt->u.producer.buffer = (struct VMQBuffer *)0;
		INIT_LIST_HEAD(&ctxt->u.producer.entry);
		mutex_lock(&mgr->mutex);
		list_add_tail(&ctxt->u.producer.entry, &mgr->producers);
		mutex_unlock(&mgr->mutex);
	}
	else if(setup.type == VMQ_TYPE_CONSUMER) {
		DBG("[%p-%s] Consumer setup.\n", mgr, name);
		ctxt->type = VMQ_TYPE_CONSUMER;
		atomic_inc(&mgr->refcnt);
		ctxt->u.consumer.mgr = mgr;
		INIT_LIST_HEAD(&ctxt->u.consumer.items);
		INIT_LIST_HEAD(&ctxt->u.consumer.entry);
		mutex_init(&ctxt->u.consumer.mutex);
		mutex_lock(&mgr->mutex);
		list_add_tail(&ctxt->u.consumer.entry, &mgr->consumers);
		mutex_unlock(&mgr->mutex);
	}
	else {
		ret = -EINVAL;
		goto err6;
	}
	filp->private_data = ctxt;
	return 0;

err6:
	kfree(ctxt);
err5:
err4:
err3:
err2:
err1:
done:
	return ret;
}

static int vmq_getbuf(struct file *filp, unsigned long arg)
{
	int err;
	int ret = -EINVAL;
	struct VMQProducer *pro;
	struct VMQBuffer *buffer;
	struct VMQBufferInfo info;
	struct VMQContext *ctxt = (struct VMQContext *)filp->private_data;

	if(unlikely(ctxt->type != VMQ_TYPE_PRODUCER)) {
		ret = -EINVAL;
		goto done;
	}
	pro = &ctxt->u.producer;
	if(unlikely(pro->buffer)) {
		ret = -EBUSY;
		goto done;
	}

	ret = copy_from_user(&info, 
			(const void __user *)arg, sizeof(info));
	if(unlikely(ret)) {
		ret = -EFAULT;
		goto done;
	}

	if(info.size <= 0) {
		DBG("size is less then 0\n");
		ret = -EFAULT;
		goto done;
	}

	if(info.size > pro->mgr->size) {
		ret = -EFAULT;
		goto done;
	}

	buffer = get_buffer_item(pro->mgr, info.size, &err);
	if(unlikely(!buffer)) {
		ret = err;
		goto done;
	}
	info.offset = buffer->offset;

	ret = copy_to_user((void __user *)arg, &info, sizeof(info));
	if(unlikely(ret)) {
		ret = -EFAULT;
		goto err1;
	}
	pro->buffer = buffer;
	return 0;

err1:
	mutex_lock (&pro->mgr->mutex);
	remove_buffer_item(buffer);
	pro->buffer = (struct VMQBuffer *)0;
	mutex_unlock (&pro->mgr->mutex);

done:
	return ret;
}

static int vmq_putbuf(struct file *filp, unsigned long arg)
{
	int ret = -EINVAL;
	struct VMQProducer *pro;
	struct VMQBufferInfo info;
	struct VMQContext *ctxt = (struct VMQContext *)filp->private_data;

	if(unlikely(ctxt->type != VMQ_TYPE_PRODUCER)) {
		ret = -EINVAL;
		goto done;
	}
	pro = &ctxt->u.producer;

	ret = copy_from_user(&info, (const void __user *)arg, sizeof(info));
	if(unlikely(ret)) {
		ret = -EFAULT;
		goto done;
	}

	if(unlikely(pro->buffer->offset != info.offset)) {
		ret = -EINVAL;
		goto done;
	}

	put_buffer_item(pro->mgr, pro->buffer);
	pro->buffer = (struct VMQBuffer *)0;
	ret = 0;

done:
	return ret;
}

static int vmq_getitem(struct file *filp, unsigned long arg)
{
	int err = 0;
	int ret = -EINVAL;
	struct VMQItem *item;
	struct VMQConsumer *con;
	struct VMQItemInfo info;
	struct VMQContext *ctxt = (struct VMQContext *)filp->private_data;

	if(unlikely(ctxt->type != VMQ_TYPE_CONSUMER)) {
		ret = -EINVAL;
		goto done;
	}

	con = &ctxt->u.consumer;
	if(unlikely(con->item)) {
		ret = -EBUSY;
		goto err1;
	}

	item = get_queue_item(con, &err);
	if(unlikely(!item)) {
		ret = err;
		goto err1;
	}

	info.offset = item->buffer->offset;
	info.size = item->buffer->size;

	ret = copy_to_user((void __user *)arg, &info, sizeof(info));
	if(unlikely(ret)) {
		ret = -EFAULT;
		goto err2;
	}
	con->item = item;
	return 0;

err2:
	put_queue_item(item);
err1:
done:
	return ret;
}

static int vmq_putitem(struct file *filp, unsigned long arg)
{
	int ret = -EINVAL;
	struct VMQConsumer *con;
	struct VMQContext *ctxt = (struct VMQContext *)filp->private_data;

	if(unlikely(ctxt->type != VMQ_TYPE_CONSUMER)) {
		ret = -EINVAL;
		goto done;
	}
	con = &ctxt->u.consumer;

	if(unlikely(!con->item)) {
		ret = -ENOENT;
		goto done;
	}


	put_queue_item(con->item);
	con->item = (struct VMQItem *)0;

	return 0;

done:
	return ret;
}

static unsigned int unique_key = 0x08012015 ;
static unsigned int vmq_getkey (struct file *filp, unsigned long arg)
{
	unsigned int key;
	int ret = -EINVAL;

	mutex_lock(&vmq_buffer_mutex);
	key = unique_key ++;
	if (unique_key == 0)	/* Not allow to zero */
		unique_key ++;
	mutex_unlock (&vmq_buffer_mutex);

	ret = copy_to_user((void __user *)arg, &key, sizeof(key));
	if (unlikely(ret)) {
		return -EFAULT;
	}
	return 0;
}

/*****************************************************************************/

static int vmq_open(struct inode *inode, struct file *filp)
{
	int ret;

	DBG("open.\n");
	ret = nonseekable_open(inode, filp);
	if(unlikely(ret))
		return ret;

	filp->private_data = (void *)0;
	return 0;
}

static int vmq_release(struct inode *inode, struct file *filp)
{
	int count = 0;
	struct VMQItem *item, *tmp;
	struct VMQContext *ctxt = (struct VMQContext *)filp->private_data;

	if(!ctxt)
		goto done;

	if(ctxt->type == VMQ_TYPE_CONSUMER) {
		char name[VMQ_MAX_NAME_LENGTH];
		struct VMQConsumer *con = &ctxt->u.consumer;

		vmshm_get_name(con->mgr->mem, name, VMQ_MAX_NAME_LENGTH);
		DBG("Release consumer queue %s\n", name);
		dump_shared_memory(con->mgr);

		mutex_lock(&con->mgr->mutex);
		list_del(&con->entry);

		mutex_lock(&con->mutex);
		if(con->item) {
			if(unref_buffer(con->item->buffer))
				count++;
			free_queue_item(con->item);
		}
		list_for_each_entry_safe(item, tmp, &con->items, entry) {
			list_del(&item->entry);
			if(unref_buffer(item->buffer))
				count++;
			free_queue_item(item);
		}
		mutex_unlock(&con->mutex);
		mutex_unlock(&con->mgr->mutex);

		DBG("unref_buffer count = %d\n", count);
		if(count > 0)
			wake_up_interruptible_all(&con->mgr->b_wq);
		atomic_dec(&con->mgr->refcnt);
		DBG("Consumer released.\n");
	}
	else {
		struct VMQProducer *pro = &ctxt->u.producer;

		mutex_lock(&pro->mgr->mutex);
		if(pro->buffer) {
			remove_buffer_item(pro->buffer);
			pro->buffer = (struct VMQBuffer *)0;
		}
		dump_shared_memory(pro->mgr);

		list_del(&pro->entry);
		mutex_unlock(&pro->mgr->mutex);

		atomic_dec(&pro->mgr->refcnt);
		DBG("Producer released.\n");
	}

	kfree (ctxt);

done:
	return 0;
}

static long vmq_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = -EINVAL;

	switch(cmd)
	{
	case	VMQ_CREATE:
		ret = vmq_create(filp, arg);
		break;

	case	VMQ_REMOVE:
		ret = vmq_remove(filp, arg);
		break;

	case	VMQ_SETUP:
		ret = vmq_setup(filp, arg);
		break;

	/* for sender of notify */
	case	VMQ_GETBUF:
		ret = vmq_getbuf(filp, arg);
		break;
	case	VMQ_PUTBUF:
		ret = vmq_putbuf(filp, arg);
		break;

	/* for event receiver */
	case	VMQ_GETITEM:
		ret = vmq_getitem(filp, arg);
		break;
	case	VMQ_PUTITEM:
		ret = vmq_putitem(filp, arg);
		break;


	case	VMQ_GETKEY:
		ret = vmq_getkey (filp, arg);
		break;

	default:
		ERR ("Not support cmd(%d)\n", cmd);
		break;
	}

	return ret;
}

static int vmq_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int ret = -EINVAL;
	struct VMSharedMemory *mem;
	struct VMQContext *ctxt = (struct VMQContext *)filp->private_data;

	if(!ctxt)
		goto done;

	if(ctxt->type == VMQ_TYPE_CONSUMER) {
		mem = ctxt->u.consumer.mgr->mem;
	}
	else {
		mem = ctxt->u.producer.mgr->mem;
	}

	if(!mem) {
		DBG("Select first.\n");
		goto done;
	}

	ret = vmshm_remap(mem, vma);

done:
        return ret;
}

static unsigned int vmq_poll(struct file *filp, struct poll_table_struct *wait)
{
	unsigned int mask = 0;
	struct VMQContext *ctxt = (struct VMQContext *)filp->private_data;

	if(ctxt->type == VMQ_TYPE_CONSUMER) {
		poll_wait(filp, &ctxt->u.consumer.mgr->q_wq, wait);
		mutex_lock(&ctxt->u.consumer.mutex);
		if(!list_empty(&ctxt->u.consumer.items))
			mask |= POLLIN;
		mask |= POLLOUT;
		mutex_unlock(&ctxt->u.consumer.mutex);
	}
	else {
		poll_wait(filp, &ctxt->u.producer.mgr->b_wq, wait);
		mask |= (POLLIN|POLLOUT);
	}

	return mask;
}

/*****************************************************************************/

static void vmq_proc_init(void)
{
	vmq_proc_root = proc_mkdir(VMQ_MODULE_NAME, (struct proc_dir_entry *)0);
	if(!vmq_proc_root) {
		ERR ("creation proc entry failed.\n");
		return;
	}
}

static void vmq_proc_exit(void)
{
	if(!vmq_proc_root)
		return;

	remove_proc_entry(VMQ_MODULE_NAME, (struct proc_dir_entry *)0);
}

/*****************************************************************************/

static struct file_operations vmq_ops = {
	.owner = THIS_MODULE,
	.open = vmq_open,
	.release = vmq_release,
	.unlocked_ioctl = vmq_ioctl,
	.mmap = vmq_mmap,
	.poll = vmq_poll,
};

static struct miscdevice vmq_dev = {
	.minor = VMQ_MINOR,
	.name = VMQ_MODULE_NAME,
	.fops = &vmq_ops,
};

static int __init vmq_driver_init(void)
{
	printk("Build Date Time: %s %s\n",__DATE__,__TIME__);
	DBG("Driver init.\n");

	vmq_buffer_cachep = kmem_cache_create("vmq_buffer_cache", sizeof(struct VMQBuffer), 0, 0, NULL);
	if(unlikely(!vmq_buffer_cachep)) {
		printk(KERN_ERR "vmq: failed to create slab cache.\n");
		return -ENOMEM;
	}
	DBG("vmq_buffer_cache created.\n");

	vmq_item_cachep = kmem_cache_create("vmq_item_cache", sizeof(struct VMQItem), 0, 0, NULL);
	if(unlikely(!vmq_item_cachep)) {
		printk(KERN_ERR "vmq: failed to create slab cache.\n");
		return -ENOMEM;
	}
	DBG("vmq_item_cache created.\n");

	misc_register(&vmq_dev);
	vmq_proc_init();
	return 0;
}

static void __exit vmq_driver_exit(void)
{
	DBG("Driver exit.\n");

	kmem_cache_destroy(vmq_item_cachep);
	kmem_cache_destroy(vmq_buffer_cachep);

	vmq_proc_exit();
	misc_deregister(&vmq_dev);
}

module_init(vmq_driver_init);
module_exit(vmq_driver_exit);

MODULE_AUTHOR("Youngdo Lee");
MODULE_LICENSE("GPL");
// Modified by HSLee
