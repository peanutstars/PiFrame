
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "vmq.h"
#include "debug.h"

/*****************************************************************************/

#define	CMD_CREATE	1
#define	CMD_REMOVE	2

/*****************************************************************************/

static int s_command = 0;
static char *s_name = (char *)0;
static int s_size = 0;
static int s_pagesize = 0;
static int s_flags = 0;
static int s_count = 0;

/*****************************************************************************/

static const char *DEVICE_PATH = "/dev/vmq";
static const char *helpstr = 
"Usage: vmqctl [options]\n"
"VMQ-Driver controller.\n"
"\n"
" -c, --command               create or remove.\n"
" -n, --name=QNAME            Queue name\n"
" -s, --size=SIZE             shared memory sizes in byte.\n"
" -p, --pagesize=PAGESIZE     page size.\n"
" -f, --flag=FLAGS            if 1, allocate physically contiguous memory.\n";

/*****************************************************************************/

static void help(void)
{
	fprintf(stderr, "%s", helpstr);
	exit(EXIT_SUCCESS);
}

/*****************************************************************************/

int main(int argc, char **argv)
{
	int fd;
	int ret = -1;
	int c, optidx = 0;
	static struct option options[] = {
		{ "command", 1, 0, 'c' },
		{ "name", 1, 0, 'n' },
		{ "size", 1, 0, 's' },
		{ "pagesize", 1, 0, 'p' },
		{ "flag", 1, 0, 'f' },
		{ "count", 1, 0, 'C' },
		{ "help", 0, 0, 'h' },
		{ 0, },
	};

	for( ; ; ) {
		c = getopt_long(argc, argv, "c:n:s:p:f:C:h", options, &optidx);
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
			s_pagesize = atoi(optarg);
			break;
		case 'f':
			s_flags = atoi(optarg);
			break;
		case 'C':
			s_count = atoi(optarg);
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

	fd = open(DEVICE_PATH, O_RDWR);
	if(fd < 0) {
		perror(DEVICE_PATH);
		exit(EXIT_FAILURE);
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
			if(s_pagesize <= 0 || s_pagesize & 0x7f) {
				fprintf(stderr, "Page size %d is invalid.\n", 
						s_pagesize);
				exit(EXIT_FAILURE);
			}
			ret = VMQueueCreate(s_name, 
					s_size, s_pagesize, 0, s_flags, s_count);
			break;
		case	CMD_REMOVE:
			if(!s_name) {
				fprintf(stderr, "name is not setup.\n");
				exit(EXIT_FAILURE);
			}
			ret = VMQueueRemove(s_name);
			break;
		default:
			fprintf(stderr, "Command is not setup.\n");
			exit(EXIT_FAILURE);
			break;
	}

	return ret;
}
