
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include "vmshm.h"
#include "pfdebug.h"

/*****************************************************************************/

int VMSharedMemoryCreate(
		int fd, const char *name, int size, int perm, int flags)
{
	int ret;
	struct VMShmCreate cr;

	cr.name = name;
	cr.size = size;
	cr.perm = perm;
	cr.flags = flags & VMSHM_FLAG_MASK;

	ret = ioctl(fd, VMSHM_CREATE, &cr);
	if(ret < 0) {
		DBG("VMSHM_CREATE failed.\n");
		perror("VMSHM_CREATE");
	}

	return ret;
}

int VMSharedMemoryRemove(int fd, const char *name)
{
	int ret;

	ret = ioctl(fd, VMSHM_REMOVE, name);
	if(ret < 0) {
		DBG("VMSHM_REMOVE failed.\n");
		perror("VMSHM_REMOVE");
	}

	return ret;
}

int VMSharedMemorySelect(int fd, const char *name)
{
	int ret;

	ret = ioctl(fd, VMSHM_SELECT, name);
	if(ret < 0) {
		DBG("VMSHM_SELECT failed.\n");
		perror("VMSHM_SELECT");
	}

	return ret;
}

int VMSharedMemoryUnselect(int fd)
{
	int ret;

	ret = ioctl(fd, VMSHM_UNSELECT);
	if(ret < 0) {
		DBG("VMSHM_UNSELECT failed.\n");
		perror("VMSHM_UNSELECT");
	}

	return ret;
}
