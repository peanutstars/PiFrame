#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "vmshm.h"

static const char *DEVICE_PATH = "/dev/vmshm";
static const char *SHM_NAME = "hello-shm";

static void fatal(char **argv)
{
	fprintf(stderr, "Usage: %s <name> <size>\n", argv[0]);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	int val;
	int fd;
	int ret;
	int size;
	char *ptr;
	struct VMShmCreate cr;

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

	ret = VMSharedMemorySelect(fd, argv[1]);
	if(ret) {
		perror("VMSHM_SELECT");
		exit(EXIT_FAILURE);
	}
	printf("VMSHM_SELECT succeed.\n");

	ptr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if(ptr == MAP_FAILED) {
		perror("mmap()");
		exit(EXIT_FAILURE);
	}
	printf("mmap() succeed: %p\n", ptr);

	memset(ptr, 0, size);

	if(munmap(ptr, size)) {
		perror("munmap()");
		exit(EXIT_FAILURE);
	}
	printf("munmap() succeed.\n");

	ret = VMSharedMemoryUnselect(fd);
	if(ret) {
		perror("VMSHM_UNSELECT");
		exit(EXIT_FAILURE);
	}
	printf("VMSHM_UNSELECT succeed.\n");

	return EXIT_SUCCESS;
}
