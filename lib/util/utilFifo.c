
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "list.h"
#include "pfdefine.h"
#include "pfuthread.h"
#include "pfdebug.h"
#include "pfufifo.h"

/*****************************************************************************/

struct QItem {
	struct list_head		list ;
	int						size ;
	void *					data ;
} ;
typedef struct QItem QItem ;

struct PFQueue {
	void					*pointer ;
	char					*name ;
	pthread_mutex_t			mutex ;
	pthread_cond_t			signal ;

	struct list_head		queueRoot ;
	struct list_head		emptyRoot ;

	uint32_t				poolCount ;
	QItem					*itemPool ;
} ;

/*****************************************************************************/

static QItem *allocQItem (struct PFQueue *pfq)
{
	QItem *item = NULL ;

	if (pfq->poolCount == 0) {
		item = (QItem *) malloc (sizeof(QItem)) ;
		INIT_LIST_HEAD(&item->list) ;
		item->size = 0 ;
		item->data = NULL ;
	} else {
		if ( ! list_empty(&pfq->emptyRoot)) {
			LOCK_MUTEX(pfq->mutex) ;
			if ( ! list_empty(&pfq->emptyRoot)) {
				item = list_entry(pfq->emptyRoot.next, QItem, list) ;
				list_del(&item->list) ;
			}
			UNLOCK_MUTEX(pfq->mutex) ;
		}
	}

	return item ;
}

static void freeQItem (struct PFQueue *pfq, QItem *item)
{
	if (item->data) {
		free (item->data) ;
		item->data = NULL ;
	}
	item->size = 0 ;
	INIT_LIST_HEAD(&item->list) ;

	if (pfq->poolCount == 0) {
		free (item) ;
	} else {
		LOCK_MUTEX(pfq->mutex) ;
		list_add_tail(&item->list, &pfq->emptyRoot) ;
		SIGNAL(pfq->signal) ;
		UNLOCK_MUTEX(pfq->mutex) ;
	}
}

/*****************************************************************************/

void *PFU_createFIFO(int queueCount)
{
	struct PFQueue *pfq ;
	pfq = (struct PFQueue *) calloc (1, sizeof(*pfq)) ;
	ASSERT(pfq) ;

	pfq->pointer = (void *) pfq ;
	INIT_MUTEX(pfq->mutex) ;
	INIT_SIGNAL(pfq->signal) ;
	INIT_LIST_HEAD(&pfq->queueRoot) ;
	INIT_LIST_HEAD(&pfq->emptyRoot) ;
	pfq->poolCount = queueCount ;
	if (pfq->poolCount > 0) {
		int i;
		pfq->itemPool = (QItem *) malloc (sizeof(QItem) * pfq->poolCount) ;
		for (i=0; i<pfq->poolCount; i++) {
			INIT_LIST_HEAD(&pfq->itemPool[i].list) ;
			pfq->itemPool[i].size = 0 ;
			pfq->itemPool[i].data = NULL ;
			list_add_tail (&pfq->itemPool[i].list, &pfq->emptyRoot) ;
		}
	} else {
		pfq->itemPool = NULL ;
	}

	return pfq ;
}

void PFU_destroyFIFO (PFUFifo *handle)
{
	QItem *item ;
	struct PFQueue *pfq = (struct PFQueue *) handle ;
	ASSERT((pfq->pointer == handle)) ;

	LOCK_MUTEX(pfq->mutex) ;
	while ( ! list_empty(&pfq->queueRoot)) {
		item = list_entry(pfq->queueRoot.next, struct QItem, list) ;
		list_del (&item->list) ;
		if (item->data) {
			free(item->data) ;
			item->data = NULL ;
		}
		item->size = 0 ;
		free (item) ;
	}
	UNLOCK_MUTEX(pfq->mutex) ;
	DESTROY_MUTEX(pfq->mutex) ;
	DESTROY_SIGNAL(pfq->signal) ;

	if (pfq->poolCount > 0 && pfq->itemPool) {
		free (pfq->itemPool) ;
	}
	pfq->itemPool = NULL ;
	pfq->pointer = NULL ;

	free (pfq) ;
}

void PFU_enqueueFIFO (PFUFifo *handle, void *data, int size)
{
	uint32_t retryCount = 0 ;
	QItem *item ;
	struct PFQueue *pfq = (struct PFQueue *) handle ;
	ASSERT((pfq->pointer == handle)) ;

	do
	{
		item = allocQItem(pfq) ;
		if (item == NULL) {
			struct timespec ts ;
			DBG(HL_CYAN "FIFO Item is empty and waiting Item ... %d" HL_NONE "\n", ++retryCount) ;
			SET_SIGNAL_TIME_MSEC(ts, 1000) ;
			LOCK_MUTEX(pfq->mutex) ;
			WAIT_SIGNAL_TIMEOUT(pfq->signal, pfq->mutex, ts) ;
			UNLOCK_MUTEX(pfq->mutex) ;
		}
	}
	while ( ! item) ;

	item->size = size ;
	item->data = malloc(size) ;
	ASSERT(item->data) ;
	memcpy (item->data, data, size) ; 

	LOCK_MUTEX(pfq->mutex) ;
	list_add_tail (&item->list, &pfq->queueRoot) ;
	UNLOCK_MUTEX(pfq->mutex) ;
}

void *PFU_dequeueFIFO (PFUFifo *handle)
{
	void *data = NULL ;
	QItem *item = NULL ;
	struct PFQueue *pfq = (struct PFQueue *) handle ;
	ASSERT((pfq->pointer == handle)) ;

	if ( ! list_empty(&pfq->queueRoot)) {
		LOCK_MUTEX(pfq->mutex) ;
		if ( ! list_empty(&pfq->queueRoot)) {
			item = list_entry(pfq->queueRoot.next, struct QItem, list) ;
			list_del (&item->list) ;
			data = item->data ;
			item->size = 0 ;
			item->data = NULL ;
		}
		UNLOCK_MUTEX(pfq->mutex) ;
	}

	if (item) {
		freeQItem(pfq, item) ;
	}

	return data ;
}
