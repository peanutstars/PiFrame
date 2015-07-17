#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "vmq.h"

static const char *DEVICE_PATH = "/dev/vmq";
static const char *SHM_NAME = "hello-vmq";

static void fatal(char **argv)
{
	fprintf(stderr, "Usage: %s <name> <size>\n", argv[0]);
	exit(EXIT_FAILURE);
}

static int dequeue(int fd, char *ptr)
{
	int val;
	int ret;
	int size, offset;

	ret = VMQueueGetItem(fd, &offset);
	if(ret < 0) {
		perror("VMQ_GETITEM");
		exit(0);
		return ret;
	}
	size = ret;

	val = *(unsigned char *)(ptr + offset);

	ret = VMQueuePutItem(fd, offset);
	if(ret < 0) {
		perror("VMQ_PUTITEM");
		exit(0);
		return ret;
	}
	printf("Dequeued %d bytes from %d(%d)\n", size, offset, val);
	return 0;

}

int main(int argc, char **argv)
{
	int val;
	int fd;
	int ret;
	int size;
	char *ptr;
	struct VMQSetup setup;

	if(argc != 3)
		fatal(argv);

	size = atoi(argv[2]);
	if(!size)
		fatal(argv);

	fd = open(DEVICE_PATH, O_RDWR);
	if(fd < 0) {
		perror(DEVICE_PATH);
		exit(EXIT_FAILURE);
	}

	ret = VMQueueSetup(fd, argv[1], 0);
	if(ret) {
		perror("VMQ_SETUP");
		exit(EXIT_FAILURE);
	}

	ptr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if(ptr == MAP_FAILED) {
		perror("mmap()");
		exit(EXIT_FAILURE);
	}

	srandom(time(NULL));
	for( ; ; ) {
		dequeue(fd, ptr);
		//usleep(10000);
	}

	return EXIT_SUCCESS;
}
