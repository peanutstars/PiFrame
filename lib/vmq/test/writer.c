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

static int queue(int fd, int size, char *ptr)
{
	int offset;
	int ret;
	static int val = 0;

	ret = VMQueueGetBuffer(fd, size);
	if(ret < 0) {
		perror("VMQ_GETBUF");
		exit(0);
		return ret;
	}
	offset = ret;

	val = (val + 1) & 0xff;
	memset(&ptr[offset], val, size);

	ret = VMQueuePutBuffer(fd, offset);
	if(ret < 0) {
		perror("VMQ_PUTBUF");
		exit(0);
		return ret;
	}
	printf("Queued %d bytes to %d(%d)\n", size, offset, val);
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

	ret = VMQueueSetup(fd, argv[1], 1);
	if(ret) {
		perror("VMQ_ATTACH");
		exit(EXIT_FAILURE);
	}

	ptr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if(ptr == MAP_FAILED) {
		perror("mmap()");
		exit(EXIT_FAILURE);
	}

	srandom(time(NULL));
	for( ; ; ) {
		queue(fd, random() % 65536 + 1024, ptr);
		//usleep(10000);
	}

	return EXIT_SUCCESS;
}
