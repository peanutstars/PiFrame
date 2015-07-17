
#ifndef	__CLIST_H__
#define	__CLIST_H__

#ifdef	__KERNEL__
#include <linux/slab.h>
#include <linux/list.h>
#else
#include <stdlib.h>
#include <assert.h>
#include "list.h"
#endif	/*__KERNEL__*/

/*****************************************************************************/

#define	FASTCALL			static inline

#define	CLIST_HEAD_INIT(name)		LIST_HEAD_INIT(name)
#define	CLIST_HEAD(name)		LIST_HEAD(name)
#define	INIT_CLIST_HEAD(ptr)		INIT_LIST_HEAD(ptr)
#define	clist_empty			list_empty

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

struct clist_item {
	struct list_head list;
	void *data;
};

/*****************************************************************************/

FASTCALL struct clist_item *clist_add(struct list_head *clist, void *data)
{
	struct clist_item *item;
	item = (struct clist_item *)ALLOC(sizeof(*item));
	ASSERT(item);
	item->data = (void *)data;
	list_add_tail(&item->list, clist);
	return item;
}

FASTCALL struct clist_item *clist_lookup(struct list_head *clist, void *ptr)
{
	struct clist_item *item;
	ASSERT(ptr);
	list_for_each_entry(item, clist, list) {
		if(item->data == ptr)
			return item;
	}
	return (struct clist_item *)0;
}

FASTCALL void *clist_del(struct clist_item *item)
{
	void *data = (void *)0;
	list_del(&item->list);
	data = item->data;
	FREE(item);
	return data;
}

#endif	/*__CLIST_H__*/
