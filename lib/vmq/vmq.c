
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "vmq-dev.h"
#include "vmshm-dev.h"

#include "vmq.h"
#include "pfdebug.h"

#if ( __WORDSIZE == 64 )
#define Pointer2Integer		uint64_t
#else
#define Pointer2Integer		uint32_t
#endif

/*****************************************************************************/

#define	VMQ_DEVICE_PATH					"/dev/vmq"
#define	VMSHM_PROC_PATH					"/proc/vmshm"

#define	VMQ_FLAG_MASK					VMSHM_FLAG_PHYSICAL

/*****************************************************************************/

#define ENABLE_NOTIFY_LOCK		1

struct VMQueue {
	int fd;
	int size;
	char *base;
#if ENABLE_NOTIFY_LOCK
	pthread_mutex_t lock;
#endif
};

/*****************************************************************************/

static int getQueueSize(const char *name)
{
	FILE *fp;
	int size;
	int refcnt;
	int rsize = -1;
	char buf[64];

	/* cat /proc/vmshm 
	 * NAME SIZE(REFCNT)\n */

	fp = fopen(VMSHM_PROC_PATH, "r");
	ASSERT(fp);

	while(fscanf(fp, "%s %d(%d)\n", buf, &size, &refcnt) == 3) {
		if(strcmp(name, buf) == 0) {
			rsize = size;
			break;
		}
	}

	fclose(fp);
	return rsize;
}


/*****************************************************************************/

int VMQueueCreate(const char *name, 
		int size, int page_size, int perm, int flags, int maxItemCount)
{
	int fd;
	int ret = -1;
	int fdflags;
	struct VMQCreateParam param;

	fd = open(VMQ_DEVICE_PATH, O_RDWR);
	if(fd < 0) {
		DBG("%s open failed.\n", VMQ_DEVICE_PATH);
		perror(VMQ_DEVICE_PATH);
		goto done;
	}

	fdflags = fcntl(fd, F_GETFD);
	fcntl(fd, F_SETFD, fdflags | FD_CLOEXEC);

	param.name = name;
	param.size = size;
	param.page_size = page_size;
	param.perm = perm;
	param.flags = flags & VMQ_FLAG_MASK;
	param.maxItemCount = maxItemCount;

	ret = ioctl(fd, VMQ_CREATE, &param);
	if(ret < 0) {
		DBG("VMQ_CREATE failed.\n");
		perror("VMQ_CREATE");
	}
	close(fd);

done:
	return ret;
}

int VMQueueRemove(const char *name)
{
	int fd;
	int ret = -1;

	fd = open(VMQ_DEVICE_PATH, O_RDWR);
	if(fd < 0) {
		DBG("%s open failed.\n", VMQ_DEVICE_PATH);
		perror(VMQ_DEVICE_PATH);
		goto done;
	}

	ret = ioctl(fd, VMQ_REMOVE, name);
	if(ret < 0) {
		DBG("VMQ_REMOVE failed.\n");
		perror("VMQ_REMOVE");
	}
	close(fd);

done:
	return ret;
}

struct VMQueue *VMQueueOpen(const char *name, int mode)
{
	int ret = -1;
	int flags;
	struct VMQueue *q;
	struct VMQSetupParam param;

	q = malloc(sizeof(struct VMQueue));
	if(!q) {
		DBG("Memory not enough.\n");
		goto done;
	}

#if ENABLE_NOTIFY_LOCK
	pthread_mutex_init (&q->lock, NULL);
#endif

	q->fd = open(VMQ_DEVICE_PATH, O_RDWR);
	if(q->fd < 0) {
		DBG("%s open failed.\n", VMQ_DEVICE_PATH);
		perror(VMQ_DEVICE_PATH);
		goto err1;
	}

	flags = fcntl(q->fd, F_GETFD);
	fcntl(q->fd, F_SETFD, flags | FD_CLOEXEC);

	q->size = getQueueSize(name);
	if(q->size < 0) {
		DBG("Queue size %d is invalid.\n", q->size);
		goto err2;
	}

	param.name = name;
	param.type = (mode == VMQ_MODE_PRODUCER) ? 
						VMQ_TYPE_PRODUCER : VMQ_TYPE_CONSUMER;
	ret = ioctl(q->fd, VMQ_SETUP, &param);
	if(ret < 0) {
		DBG("VMQ_SETUP failed.\n");
		perror("VMQ_SETUP");
		goto err3;
	}

	q->base = (char *)mmap(NULL, 
			q->size, PROT_READ|PROT_WRITE, MAP_SHARED, q->fd, 0);
	if(q->base == MAP_FAILED) {
		DBG("VMQ [%s] mmap failed.\n", name);
		perror(name);
		goto err4;
	}
//	DBG("[%s] is opened as a %s.\n", name, mode == VMQ_MODE_CONSUMER ? "consumer" : "producer");
	return q;

err4:
err3:
err2:
	close(q->fd);

err1:
	free(q);
	q = (struct VMQueue *)0;

done:
	return q;
}

void VMQueueClose(struct VMQueue *q)
{
	ASSERT(q);
	munmap(q->base, q->size);
#if ENABLE_NOTIFY_LOCK
	pthread_mutex_destroy (&q->lock);
#endif
	close(q->fd);
	free(q);
}

void *VMQueueGetBuffer(struct VMQueue *q, int size)
{
	int ret;
	struct VMQBufferInfo bi;
	void *ptr = (void *)0;

	ASSERT(q);
	ASSERT(size > 0);

#if ENABLE_NOTIFY_LOCK
	pthread_mutex_lock (&q->lock);
#endif

	bi.size = size;
	ret = ioctl(q->fd, VMQ_GETBUF, &bi);
	if(ret < 0) {
		if(errno == EINTR)
			goto done;
		perror("VMQ_GETBUF");
		goto done;
	}

	ptr = &q->base[bi.offset];

	if (size >= 12) {
		unsigned int *array = (unsigned int *)ptr;
		array[2] = 0;
	}
done:
#if ENABLE_NOTIFY_LOCK
	if (ptr == NULL) {
		pthread_mutex_unlock (&q->lock);
	}
#endif
	return ptr;
}

void VMQueuePutBuffer(struct VMQueue *q, void *ptr)
{
	int ret;
	struct VMQBufferInfo bi;

	ASSERT(q);
	ASSERT(ptr);

	bi.offset = (Pointer2Integer)ptr - (Pointer2Integer)q->base;
	ret = ioctl(q->fd, VMQ_PUTBUF, &bi);
	if(ret < 0) {
		ERR2("VMQ_PUTBUF\n");
	}
#if ENABLE_NOTIFY_LOCK
	pthread_mutex_unlock (&q->lock);
#endif
}

void *VMQueueGetItem(struct VMQueue *q, int *size)
{
	int ret;
	struct VMQItemInfo ii;
	void *ptr = (void *)0;

	ASSERT(q);

	ret = ioctl(q->fd, VMQ_GETITEM, &ii);
	if(ret < 0) {
		ERR2("VMQ_GETITEM\n");
		goto done;
	}

	ptr = &q->base[ii.offset];
	if(size)
		*size = ii.size;

done:
	return ptr;
}

void VMQueuePutItem(struct VMQueue *q, void *ptr)
{
	int ret;
	struct VMQItemInfo ii;

	ASSERT(q);
	ASSERT(ptr);

	ii.offset = (Pointer2Integer)ptr - (Pointer2Integer)q->base;
	ret = ioctl(q->fd, VMQ_PUTITEM, &ii);
	if(ret < 0) {
		DBG("VMQ_PUTITEM failed\n");
		perror("VMQ_PUTITEM");
	}
}


int VMQueueGetFd(struct VMQueue *q)
{
	ASSERT(q);
	return q->fd;
}

uint32_t VMQueueGetKey (struct VMQueue *q, uint32_t *pKey)
{
	uint32_t key = 0 ;
	int ret ;
	ASSERT(q);
	ret = ioctl(q->fd, VMQ_GETKEY, &key) ;
	if (ret < 0) {
		if (errno == EINTR)
			goto done;
		perror ("VMQ_GETKEY");
		goto done;
	}
	if (*pKey) {
		*pKey = key ;
	}
done:
	return key ;
}

