#include <stdlib.h>

#include "balloc.h"
#include "debug.h"

#define	MEM_SIZE	(512 * 1024)
#define	PAGE_SIZE	(4096)

#define	CMD_ALLOC	1001
#define	CMD_FREE	1002

int console(int *cmd, int *param)
{
	char buffer[256];

	printf("[a:alloc, f:free] : ");
	scanf("%s", buffer);
	if(buffer[0] == 'a')
		*cmd = CMD_ALLOC;
	else if(buffer[0] == 'f')
		*cmd = CMD_FREE;
	else
		return -1;

	if(*cmd == CMD_ALLOC)
		printf("[Size in bytes] : ");
	else
		printf("[Index number] : ");
	scanf("%d", param);
	return 0;
}

void loop(struct BuddyAllocator *ba)
{
	int cmd, param, index;
	for( ; ; ) {
		if(console(&cmd, &param))
			continue;
		switch(cmd) {
			case CMD_ALLOC:
				index = ba_alloc(ba, param);
				printf("Allocated Index = %d\n", index);
				break;
			case CMD_FREE:
				ba_free(ba, param);
				printf("Index %d freed.\n", param);
				break;
			default:
				exit(0);
		}
		ba_dump_bitmap(ba);
	}
}


int main(int argc, char **argv)
{
	struct BuddyAllocator *ba;
	ba = ba_create(MEM_SIZE, PAGE_SIZE);
	if(!ba) {
		DBG("ba_create failed.\n");
		return EXIT_FAILURE;
	}
	DBG("ba = %p\n", ba);
	ba_dump(ba);
	ba_dump_bitmap(ba);
	loop(ba);
	ba_remove(ba);
	return 0;
}
