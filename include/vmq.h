#ifndef	__VMQ_H__
#define	__VMQ_H__

/*****************************************************************************/

#define	VMQ_MODE_CONSUMER			0
#define	VMQ_MODE_PRODUCER			1

/*****************************************************************************/

#ifdef	__cplusplus
extern "C" {
#endif	/*__cplusplus*/

struct VMQueue;

int VMQueueCreate(const char *name, 
		int size, int page_size, int perm, int flags, int maxItemCuont);
int VMQueueRemove(const char *name);
struct VMQueue *VMQueueOpen(const char *name, int mode);
void VMQueueClose(struct VMQueue *q);
void *VMQueueGetBuffer(struct VMQueue *q, int size);
void *VMQueueGetBufferWithKey(struct VMQueue *q, int size);
void VMQueuePutBuffer(struct VMQueue *q, void *ptr);
void *VMQueueGetItem(struct VMQueue *q, int *size);
void VMQueuePutItem(struct VMQueue *q, void *ptr);
int VMQueueGetFd(struct VMQueue *q);

#ifdef	__cplusplus
};
#endif	/*__cplusplus*/

#endif	/*__VMQ_H__*/
