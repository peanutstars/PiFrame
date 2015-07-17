
#ifndef	__BALLOC_H__
#define	__BALLOC_H__

struct BuddyAllocator;

extern struct BuddyAllocator *ba_create(int size, int page_size);
extern void ba_remove(struct BuddyAllocator *ba);
extern int ba_alloc(struct BuddyAllocator *ba, int len);
extern void ba_free(struct BuddyAllocator *ba, int index);
extern void ba_dump(struct BuddyAllocator *ba);
extern void ba_dump_bitmap(struct BuddyAllocator *ba);

#endif	/*__BALLOC_H__*/
