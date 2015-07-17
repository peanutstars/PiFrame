/*****************************************************************************
 * 
 * Buddy allocator with single level bitmap.
 *
 *****************************************************************************/

#ifdef	__KERNEL__
#include <linux/kernel.h>
#include <mm.h>
#else	/*__KERNEL__*/
#include <stdlib.h>
#include <string.h>
#endif	/*__KERNEL__*/

#include "debug.h"

/*****************************************************************************/

#define	BA_MIN_PAGE_SIZE	(1 << 7)
#define	BA_MIN_PAGE_MASK	(BA_MIN_PAGE_SIZE - 1)

#ifdef	__KERNEL__

#define	ALLOC(sz)		kmalloc(sz, GFP_KERNEL)
#define	FREE(ptr)		kfree(ptr)
#else
#define	ALLOC(sz)		malloc(sz)
#define	FREE(ptr)		free(ptr)
#endif	/*__KERNEL__*/

#ifndef	likely
#define likely(x)       __builtin_expect((x),1)
#endif	/*likely*/
#ifndef	unlikely
#define unlikely(x)     __builtin_expect((x),0)
#endif	/*unlikely*/


#define	BA_ORDER(ba, index)		(ba)->bitmap[index].order
#define	BA_NEXT_INDEX(ba, index)	(index+(1<<(ba)->bitmap[index].order))
#define	BA_IS_FREE(ba, index)		(!(ba)->bitmap[index].allocated)
#define	BA_BUDDY_INDEX(ba, index)	(index ^ (1 << BA_ORDER(ba, index)))
#define	MIN(a, b)			((a) > (b) ? (b) : (a))

/*****************************************************************************/

struct bitmap {
	unsigned allocated	: 1;
	unsigned order		: 7;
};

struct BuddyAllocator {
	int size;
	int page_size;
	int nr_pages;
	int max_order;
	struct bitmap *bitmap;
};

/*****************************************************************************/

static unsigned long ba_order(unsigned long len, int page_size)
{
	int i;
	len = (len + page_size - 1) / page_size;
	len--;
	for(i = 0; i < sizeof(len) * 8; i++)
		if(len >> i == 0)
			break;
	return i;
}

struct BuddyAllocator *ba_create(int size, int page_size)
{
	int i, index;
	struct BuddyAllocator *ba = (struct BuddyAllocator *)0;

	if(unlikely(page_size & BA_MIN_PAGE_MASK)) {
		DBG("Page size invalid.\n");
		goto done;
	}

	ba = (struct BuddyAllocator *)ALLOC(sizeof(*ba));
	if(unlikely(!ba)) {
		DBG("Memory not enough.\n");
		goto done;
	}

	ba->size = size;
	ba->page_size = page_size;
	ba->nr_pages = size / page_size;
	ba->max_order = ba_order(size, page_size);
	ba->bitmap = (struct bitmap *)ALLOC(
			sizeof(struct bitmap) * ba->nr_pages);
	memset(ba->bitmap, 0, sizeof(struct bitmap) * ba->nr_pages);

	for(index = 0, i = sizeof(long) * 8 - 1; i >= 0; i--) {
		if(ba->nr_pages & (1 << i)) {
			BA_ORDER(ba, index) = i;
			index = BA_NEXT_INDEX(ba, index);
		}
	}
done:
	return ba;
}

void ba_remove(struct BuddyAllocator *ba)
{
	FREE(ba->bitmap);
	FREE(ba);
}

int ba_alloc(struct BuddyAllocator *ba, int len)
{
	int curr = 0;
	int end = ba->nr_pages;
	int best = -1;
	unsigned long order = ba_order(len, ba->page_size);

	if(order > ba->max_order) {
		goto done;
	}
	DBG("order = %d\n", (int)order);

	while(curr < end) {
		if(BA_IS_FREE(ba, curr)) {
			if(BA_ORDER(ba, curr) == (unsigned char)order) {
				best = curr;
				break;
			}
			if(BA_ORDER(ba, curr) > (unsigned char)order &&
				(best < 0 ||
				BA_ORDER(ba, curr) < BA_ORDER(ba, best)))
				best = curr;
		}
		curr = BA_NEXT_INDEX(ba, curr);
	}

	if(best < 0)
		goto done;

	DBG("best = %d\n", best);

	while(BA_ORDER(ba, best) > (unsigned char)order) {
		int buddy;
		BA_ORDER(ba, best) -= 1;
		buddy = BA_BUDDY_INDEX(ba, best);
		BA_ORDER(ba, buddy) = BA_ORDER(ba, best);
	}
	ba->bitmap[best].allocated = 1;

done:
	return best;
}

void ba_free(struct BuddyAllocator *ba, int index)
{
	int buddy, curr = index;

	ba->bitmap[index].allocated = 0;

	do {
		buddy = BA_BUDDY_INDEX(ba, curr);
		if(BA_IS_FREE(ba, buddy) &&
				BA_ORDER(ba, buddy) == BA_ORDER(ba, curr)) {
			BA_ORDER(ba, buddy)++;
			BA_ORDER(ba, curr)++;
			curr = MIN(buddy, curr);
		}
		else {
			break;
		}
	} while(curr < ba->nr_pages);
}

void ba_dump(struct BuddyAllocator *ba)
{
#ifdef	DEBUG
	DBG("Memory Size = %d\n", ba->size);
	DBG("Page Size   = %d\n", ba->page_size);
	DBG("Page Count  = %d\n", ba->nr_pages);
	DBG("Max. Order  = %d\n", ba->max_order);
#endif	/*DEBUG*/
}

#ifdef	__KERNEL__
#define	PRINT	printk
#else
#define	PRINT	printf
#endif	/*__KERNEL__*/

void ba_dump_bitmap(struct BuddyAllocator *ba)
{
#ifdef	DEBUG
	static char c[] = { '#', '.' };
	int i;
	int isfree;
	int order, index, curr, end;

	for(i = 0; i < ba->nr_pages; i++) {
		PRINT("%d", i % 10);
	}
	PRINT("\n");

	index = 0;
	curr = 0;
	end = ba->nr_pages;
	do {
		order = BA_ORDER(ba, index);
		isfree = BA_IS_FREE(ba, index) ? 1 : 0;
		for(i = (1 << order) - 1; i >= 0; i--) {
			PRINT("%c", c[isfree]);
		}
		index = BA_NEXT_INDEX(ba, index);
	} while(index < end);
	PRINT("\n");
#endif	/*DEBUG*/
}
