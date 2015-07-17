
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
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/miscdevice.h>
#include "vmshm-dev.h"
#include "queue.h"
#include "clist.h"
#include "debug.h"

#define	VMSHM_MINOR		MISC_DYNAMIC_MINOR
#define	VMSHM_MODULE_NAME	"vmshm"
#define	VMSHM_MAX_QUEUE_COUNT	32
#define	VMSHM_PERM_MASK		0666
#define	VMSHM_FLAG_MASK		1

/*****************************************************************************/

struct VMSharedMemory {
	void *base;
	int size;
	int perm;
	int flags;
	char name[VMSHM_MAX_NAME_LENGTH];
	atomic_t refcnt;
	struct list_head entry;
	wait_queue_head_t wq;
};

/*****************************************************************************/

static DEFINE_MUTEX(vmshm_mutex);
static LIST_HEAD(vmshm_list);

/*****************************************************************************/

static struct VMSharedMemory *lookup(const char *name)
{
	struct VMSharedMemory *mem;

	list_for_each_entry(mem, &vmshm_list, entry) {
		if(!strcmp(mem->name, name))
			return mem;
	}
	return (struct VMSharedMemory *)0;
}

static void remove(struct VMSharedMemory *mem)
{
	list_del(&mem->entry);
	if(mem->flags & VMSHM_FLAG_PHYSICAL)
		kfree(mem->base);
	else
		vfree(mem->base);
	kfree(mem);
}

/*
 * helper function, mmap's the kmalloc'd area which is physically contiguous
 */
static int mmap_kmem(struct VMSharedMemory *mem, struct vm_area_struct *vma)
{
	int ret;
	long length = vma->vm_end - vma->vm_start;

	DBG("mmap_kmem()\n");
	if(length > mem->size) {
		DBG("request %d but length %d.\n", (int)length, mem->size);
		return -EIO;
	}

	/* map the whole physically contiguous area in one piece */
	if ((ret = remap_pfn_range(
			vma, 
			vma->vm_start,
			virt_to_phys(mem->base) >> PAGE_SHIFT,
			length, vma->vm_page_prot)) < 0) {
		return ret;
	}

	return 0;
}
/*
 * helper function, mmap's the vmalloc'd area which is not physically contiguous
 */
static int mmap_vmem(struct VMSharedMemory *mem, struct vm_area_struct *vma)
{
	int ret;
	unsigned long pfn;
	long length = vma->vm_end - vma->vm_start;
	unsigned long start = vma->vm_start;
	char *vmalloc_area_ptr = (char *)mem->base;

	if(length > mem->size) {
		DBG("request %d but length %d.\n", (int)length, mem->size);
		return -EIO;
	}

	/* loop over all pages, map it page individually */
	while (length > 0) {
		DBG("mmap %lx %p %ld\n", start, vmalloc_area_ptr, length);

		pfn = vmalloc_to_pfn(vmalloc_area_ptr);
		if ((ret = remap_pfn_range(vma, start, pfn, PAGE_SIZE, PAGE_SHARED)) < 0) {
			DBG("remap_pfn_range failed.\n");
			return ret;
		}
		start += PAGE_SIZE;
		vmalloc_area_ptr += PAGE_SIZE;
		length -= PAGE_SIZE;

	}
	return 0;
}

/*****************************************************************************/

struct VMSharedMemory *vmshm_create(
		const char *name, int size, int perm, int flags, int *err)
{
	struct VMSharedMemory *mem = (struct VMSharedMemory *)0;

	DBG("create.\n");

	mutex_lock(&vmshm_mutex);
	if(lookup(name)) {
		*err = -EEXIST;
		DBG("[%s] Duplicated.\n", mem->name);
		mutex_unlock(&vmshm_mutex);
		goto err1;
	}
	mutex_unlock(&vmshm_mutex);

	if(unlikely(size <= 0/* || size > VMSHM_MAX_CHUNK_SIZE*/)) {
		*err = -EINVAL;
		goto err1;
	}

	mem = kzalloc(sizeof(*mem), GFP_KERNEL);
	if(unlikely(!mem)) {
		*err = -ENOMEM;
		goto err2;
	}

	strcpy(mem->name, name);
	mem->size = (size + PAGE_SIZE - 1) & PAGE_MASK;
	DBG("mem->size = %d\n", mem->size);
	mem->perm = perm & VMSHM_PERM_MASK;
	mem->flags = flags & VMSHM_FLAG_MASK;

	if(mem->flags & VMSHM_FLAG_PHYSICAL) {
		mem->base = kmalloc(mem->size, GFP_KERNEL);
		DBG("Physically continuous memory. %p\n", mem->base);
	}
	else {
		mem->base = vmalloc(mem->size);
		DBG("Virtually continuous memory. %p\n", mem->base);
	}

	if(unlikely(!mem->base)) {
		*err = -ENOMEM;
		goto err3;
	}

	init_waitqueue_head(&mem->wq);
	atomic_set(&mem->refcnt, 0);

	mutex_lock(&vmshm_mutex);
	list_add_tail(&mem->entry, &vmshm_list);
	mutex_unlock(&vmshm_mutex);
	DBG("[%s] %d Bytes created.\n", mem->name, mem->size);

	if(!try_module_get(THIS_MODULE)) {
		DBG("try_module_get failed\n");
		*err = -EINVAL;
		goto err4;
	}
	*err = 0;
	return mem;

err4:
err3:
	kfree(mem);
	mem = (struct VMSharedMemory *)0;
err2:
err1:
	return mem;
}
EXPORT_SYMBOL(vmshm_create);

int vmshm_remove(struct VMSharedMemory *mem)
{
	int ret = -EINVAL;

	mutex_lock(&vmshm_mutex);
	/* refcnt == 0 -> can remove */
	if(!atomic_read(&mem->refcnt)) {
		/* real remove */
		DBG("[%s] %d Bytes removed.\n", mem->name, mem->size);
		remove(mem);
		module_put(THIS_MODULE);
		ret = 0;
	}
	else {
		DBG("[%s] Busy.\n", mem->name);
		ret = -EBUSY;
	}
	mutex_unlock(&vmshm_mutex);
	return ret;
}
EXPORT_SYMBOL(vmshm_remove);

struct VMSharedMemory *vmshm_select(const char *name)
{
	struct VMSharedMemory *mem;

	mutex_lock(&vmshm_mutex);
	mem = lookup(name);
	if(likely(mem))
		atomic_inc(&mem->refcnt);
	mutex_unlock(&vmshm_mutex);

	return mem;
}
EXPORT_SYMBOL(vmshm_select);

void vmshm_unselect(struct VMSharedMemory *mem)
{
	mutex_lock(&vmshm_mutex);
	atomic_dec(&mem->refcnt);
	mutex_unlock(&vmshm_mutex);
}
EXPORT_SYMBOL(vmshm_unselect);

int vmshm_remap(struct VMSharedMemory *mem, struct vm_area_struct *vma)
{
	int ret = -EINVAL;

	BUG_ON(!mem || !vma);

	if(mem->flags & VMSHM_FLAG_PHYSICAL) {
		DBG("Physcial.\n");
		ret = mmap_kmem(mem, vma);
	}
	else {
		DBG("Virtual.\n");
		ret = mmap_vmem(mem, vma);
	}

	return ret;
}
EXPORT_SYMBOL(vmshm_remap);

int vmshm_get_name(struct VMSharedMemory *mem, char *buffer, int len)
{
	int ret = -EINVAL;
	ret = snprintf(buffer, len, "%s", mem->name);
        return ret;
}
EXPORT_SYMBOL(vmshm_get_name);

/*****************************************************************************/

static int vmshm_open(struct inode *inode, struct file *filp)
{
	int ret;

	DBG("open.\n");

	ret = nonseekable_open(inode, filp);
	if(unlikely(ret))
		return ret;

	filp->private_data = (void *)0;
	return 0;
}

static int vmshm_release(struct inode *inode, struct file *filp)
{
	struct VMSharedMemory *mem = 
		(struct VMSharedMemory *)filp->private_data;

	DBG("release.\n");

	if(unlikely(!mem))
		return 0;

	DBG("[%s] Unreference.\n", mem->name);
	atomic_dec(&mem->refcnt);
	filp->private_data = (void *)0;

	return 0;
}

static long vmshm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = -EINVAL;
	struct VMSharedMemory *mem;
	char name[VMSHM_MAX_NAME_LENGTH];

	switch(cmd) {
	case	VMSHM_CREATE: {
		int err;
		struct VMShmCreate crarg;

		ret = copy_from_user(&crarg, 
				(const void __user *)arg, sizeof(crarg));
		if(unlikely(ret)) {
			ret = -EFAULT;
			break;
		}

		ret = strncpy_from_user(name, 
				(const void __user *)crarg.name, VMSHM_MAX_NAME_LENGTH);
		if(unlikely(ret < 0)) {
			break;
		}
		name[ret] = 0;

		if(!name[0]) {
			ret = -EINVAL;
			break;
		}

//		DBG("create size=%d perm=%d flags=%d\n", crarg.size, crarg.perm, crarg.flags);
		mem = vmshm_create(name, crarg.size, crarg.perm, crarg.flags, &err);
		if(unlikely(!mem)) {
			ret = err;
			break;
		}
		else {
			ret = 0;
		}
		break;
	}

	case	VMSHM_REMOVE: {
		ret = strncpy_from_user(name, 
				(const void __user *)arg, VMSHM_MAX_NAME_LENGTH);
		if(unlikely(ret < 0)) {
			break;
		}
		name[ret] = 0;

		if(!name[0]) {
			ret = -EINVAL;
			break;
		}
		mutex_lock(&vmshm_mutex);
		mem = lookup(name);
		mutex_unlock(&vmshm_mutex);
		if(likely(mem))
			ret = vmshm_remove(mem);
		else
			ret = -ENOENT;
		break;
	}

	case	VMSHM_SELECT: {
		if(unlikely(filp->private_data)) {
			ret = -EBUSY;
			break;
		}

		ret = strncpy_from_user(name, 
				(const void __user *)arg, VMSHM_MAX_NAME_LENGTH);
		if(unlikely(ret < 0)) {
			break;
		}
		name[ret] = 0;

		mem = vmshm_select(name);
		if(likely(mem)) {
			filp->private_data = mem;
			/* 크기를 리턴한다. */
			ret = mem->size;
		}
		else
			ret = -ENOENT;
		break;
	}

	case	VMSHM_UNSELECT: {
		mem = (struct VMSharedMemory *)filp->private_data;
		if(unlikely(!mem)) {
			ret = -EINVAL;
			break;
		}
		vmshm_unselect(mem);
		filp->private_data = (void *)0;
		ret = 0;
		break;
	}

	default:
		break;
	}

	return ret;
}

int vmshm_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int ret = -EINVAL;
	struct VMSharedMemory *mem = (struct VMSharedMemory *)filp->private_data;

	if(!mem) {
		DBG("Select first.\n");
		goto done;
	}

	ret = vmshm_remap(mem, vma);

done:
        return ret;
}

/*****************************************************************************/

static struct proc_dir_entry *vmshm_proc_entry;

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
static int vmshm_proc_read(char* page, char **start, 
		off_t off, int count, int* eof, void* data)
{
	int n = 0;
	struct VMSharedMemory *mem;
	mutex_lock(&vmshm_mutex);
	list_for_each_entry(mem, &vmshm_list, entry) {
		n += sprintf(&page[n], "%s %d(%d)\n", mem->name, mem->size, atomic_read(&mem->refcnt));
	}
	mutex_unlock(&vmshm_mutex);
	*eof = 1;
	return n;
}
#else
static int vmshm_proc_show (struct seq_file *m, void *v)
{
	struct VMSharedMemory *mem;
	mutex_lock(&vmshm_mutex);
	list_for_each_entry(mem, &vmshm_list, entry) {
		seq_printf(m, "%s %d(%d)\n", mem->name, mem->size, atomic_read(&mem->refcnt));
	}
	mutex_unlock(&vmshm_mutex);
	return 0;
}

static int vmshm_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, vmshm_proc_show, NULL);
}

static const struct file_operations proc_fops = {
	.open       = vmshm_proc_open,
	.read       = seq_read,
	.llseek     = seq_lseek,
	.release    = single_release,
};
#endif

static void vmshm_proc_init(void)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
	vmshm_proc_entry = create_proc_entry(VMSHM_MODULE_NAME, 0444, (struct proc_dir_entry *)0);
	if(!vmshm_proc_entry) {
		DBG("creation proc entry failed.\n");
		return;
	}
	vmshm_proc_entry->read_proc = vmshm_proc_read;
#else
	vmshm_proc_entry = proc_create_data (VMSHM_MODULE_NAME, 0444, (struct proc_dir_entry *)0, &proc_fops, (void *)0);
	if(!vmshm_proc_entry) {
		DBG("creation proc entry failed.\n");
		return;
	}
#endif
}

static void vmshm_proc_exit(void)
{
	if(!vmshm_proc_entry)
		return;

	remove_proc_entry(VMSHM_MODULE_NAME, (struct proc_dir_entry *)0);
}

/*****************************************************************************/

static struct file_operations vmshm_ops = {
	.owner = THIS_MODULE,
	.open = vmshm_open,
	.release = vmshm_release,
	.unlocked_ioctl = vmshm_ioctl,
	.mmap = vmshm_mmap,
};

static struct miscdevice vmshm_dev = {
	.minor = VMSHM_MINOR,
	.name = VMSHM_MODULE_NAME,
	.fops = &vmshm_ops
};

static int __init vmshm_driver_init(void)
{
	vmshm_proc_init();
	misc_register(&vmshm_dev);
	DBG("driver init.\n");
	return 0;
}

static void __exit vmshm_driver_exit(void)
{
	struct VMSharedMemory *mem;

	mutex_lock(&vmshm_mutex);
	while(!list_empty(&vmshm_list)) {
		mem = list_entry(vmshm_list.next, 
				struct VMSharedMemory, entry);
		if(atomic_read(&mem->refcnt)) {
			ERR("[%s] refcnt is invalid: %d\n", 
					mem->name, atomic_read(&mem->refcnt));
		}
		remove(mem);
	}
	mutex_unlock(&vmshm_mutex);
	DBG("driver exit.\n");
	vmshm_proc_exit();
	misc_deregister(&vmshm_dev);
}

module_init(vmshm_driver_init);
module_exit(vmshm_driver_exit);

MODULE_AUTHOR("Youngdo Lee");
MODULE_LICENSE("GPL");

