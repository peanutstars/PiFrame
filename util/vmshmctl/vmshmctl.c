
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "vmshm-dev.h"
#include "debug.h"

/*****************************************************************************/

static const char *DEVICE_PATH = "/dev/vmshm";
//static const char *SHM_NAME = "hello-shm";

static const char *helpstr = 
"Usage: vmshmctl [options]\n"
"VMShm driver controller.\n"
"\n"
" -c, --command               create or remove.\n"
" -s, --size=SIZE             shared memory sizes in byte.\n"
" -p, --perm=PERMISSION       ignored.\n"
" -f, --flag=FLAGS            if 1, allocate physically contiguous memory.\n";

static void help(void)
{
	fprintf(stderr, "%s", helpstr);
	exit(EXIT_SUCCESS);
}

static int shm_create(char *name, int size, int perm, int flags)
{
	int fd, ret;
	struct VMShmCreate crarg;

	fd = open(DEVICE_PATH, O_RDWR);
	if(fd < 0) {
		perror(DEVICE_PATH);
		goto err1;
	}

	crarg.name = name;
	crarg.size = size;
	crarg.perm = perm;
	crarg.flags = flags;

	ret = ioctl(fd, VMSHM_CREATE, &crarg);
	if(ret) {
		perror("VMSHM_CREATE");
		goto err2;
	}

	printf("Create done.\n");

	return fd;

err2:
	close(fd);
	fd = ret;

err1:
	return fd;
}

static int shm_remove(char *name)
{
	int fd, ret;

	fd = open(DEVICE_PATH, O_RDWR);
	if(fd < 0) {
		perror(DEVICE_PATH);
		goto err1;
	}

	ret = ioctl(fd, VMSHM_REMOVE, name);
	if(ret) {
		perror("VMSHM_REMOVE");
		goto err2;
	}

	printf("Remove done.\n");

	return fd;

err2:
	close(fd);
	fd = ret;

err1:
	return fd;
}

#define	CMD_CREATE	1
#define	CMD_REMOVE	2

static int s_command = 0;
static char *s_name = (char *)0;
static int s_size = 0;
static int s_perm = 0;
static int s_flags = 0;

int main(int argc, char **argv)
{
	int ret = -1;
	int c, optidx = 0;
	static struct option options[] = {
		{ "command", 1, 0, 'c' },
		{ "name", 1, 0, 'n' },
		{ "size", 1, 0, 's' },
		{ "perm", 1, 0, 'p' },
		{ "flag", 1, 0, 'f' },
		{ "help", 0, 0, 'h' },
		{ 0, },
	};

	for( ; ; ) {
		c = getopt_long(argc, argv, "c:n:s:p:f:h", options, &optidx);
		if(c == -1)
			break;

		switch(c) {
		case 'c':
			if(s_command) {
				fprintf(stderr, "command is already setup.\n");
				exit(EXIT_FAILURE);
			}
			
			if(strcasecmp(optarg, "create") == 0) {
				s_command = CMD_CREATE;
			}
			else if(strcasecmp(optarg, "remove") == 0) {
				s_command = CMD_REMOVE;
			}
			else {
				fprintf(stderr, "%s is unknown.\n", optarg);
				exit(EXIT_FAILURE);
			}
			break;

		case 'n':
			if(s_name) {
				fprintf(stderr, "name is already setup.\n");
				exit(EXIT_FAILURE);
			}
			s_name = strdup(optarg);
			ASSERT(s_name);
			break;
		case 's':
			s_size = atoi(optarg);
			break;
		case 'p':
			s_perm = atoi(optarg);
			break;
		case 'f':
			s_flags = atoi(optarg);
			break;
		case 'h':
			help();
			break;
		default:
			fprintf(stderr, "Unknown options.\n");
			exit(EXIT_FAILURE);
			break;
		}
	}

	switch(s_command) {
		case	CMD_CREATE:
			if(!s_name) {
				fprintf(stderr, "name is not setup.\n");
				exit(EXIT_FAILURE);
			}
			if(s_size <= 0) {
				fprintf(stderr, "Size %d is invalid.\n", 
						s_size);
				exit(EXIT_FAILURE);
			}
			ret = shm_create(s_name, s_size, s_perm, s_flags);
			break;
		case	CMD_REMOVE:
			if(!s_name) {
				fprintf(stderr, "name is not setup.\n");
				exit(EXIT_FAILURE);
			}
			ret = shm_remove(s_name);
			break;
		default:
			fprintf(stderr, "Command is not setup.\n");
			exit(EXIT_FAILURE);
			break;
	}

	if(ret < 0) {
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
