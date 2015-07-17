
#ifndef	__QUEUE_H__
#define	__QUEUE_H__

#ifdef	__KERNEL__
#include <linux/slab.h>
#include <linux/list.h>
#else
#include <stdlib.h>
#include <assert.h>
#include "list.h"
#endif	/*__KERNEL__*/

/*****************************************************************************/

#ifdef	FASTCALL
#undef	FASTCALL
#endif	/*FASTCALL*/

#define	FASTCALL			static inline

#define	LIST_QUEUE_HEAD_INIT(name)	LIST_HEAD_INIT(name)
#define	LIST_QUEUE_HEAD(name)		LIST_HEAD(name)
#define	INIT_LIST_QUEUE(ptr)		INIT_LIST_HEAD(ptr)
#define	queue_empty			list_empty

#ifdef	__KERNEL__
#define	ALLOC(sz)	kzalloc(sz, GFP_KERNEL)
#define	FREE(ptr)	kfree(ptr)
#define	ASSERT(cond)	BUG_ON(!cond)
#else
#define	ALLOC(sz)	malloc(sz)
#define	FREE(ptr)	free(ptr)
#define	ASSERT(cond)	assert(cond)
#endif	/*__KERNEL__*/

/*****************************************************************************/

struct list_queue_item {
	struct list_head list;
	void *data;
};

/*****************************************************************************/

FASTCALL void queue_enq(struct list_head *queue, void *data)
{
	struct list_queue_item *item;
	item = (struct list_queue_item *)ALLOC(sizeof(*item));
	ASSERT(item);
	item->data = (void *)data;
	list_add_tail(&item->list, queue);
}

FASTCALL void queue_enq_head(struct list_head *queue, void *data)
{
	struct list_queue_item *item;
	item = (struct list_queue_item *)ALLOC(sizeof(*item));
	ASSERT(item);
	item->data = (void *)data;
	list_add(&item->list, queue);
}

FASTCALL void *queue_deq(struct list_head *queue)
{
	void *data = (void *)0;
	struct list_queue_item * item;

	if(list_empty(queue))
		return data;

	item = list_entry(queue->next, struct list_queue_item, list);
	list_del(&item->list);
	data = item->data;
	FREE(item);
	return data;
}

#endif	/*__QUEUE_H__*/
