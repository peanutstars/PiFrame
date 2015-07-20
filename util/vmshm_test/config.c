
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "vmshm.h"
#include "pfconfig.h"

#include "pfdebug.h"

/*****************************************************************************/

#define	VMSHM_DEVICE						"/dev/vmshm"
#define	VMSHM_PROC_PATH						"/proc/vmshm"

/*****************************************************************************/

static struct VMConfig *config = NULL;
static int semid;

/*****************************************************************************/

static void initConfig(void)
{
	int fd;
	int size;
	int flags;
	const char *name = "VMConfig";

	if(getenv("PF_CONFIG_NAME") != NULL)
		name = getenv("PF_CONFIG_NAME");

	fd = open(VMSHM_DEVICE, O_RDWR);
	ASSERT(fd > 0);

	flags = fcntl(fd, F_GETFD);
	fcntl(fd, F_SETFD, flags | FD_CLOEXEC);

	size = VMSharedMemorySelect(fd, name);
	ASSERT(size > 0);

//	config = (struct VMConfig *)mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
	config = (struct VMConfig *)mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	DBG("config = %p, &config=%p\n", config, &config);
	ASSERT(config != MAP_FAILED);

	semid = semget((key_t)PF_CONFIG_KEY, 0, 0);
	ASSERT(semid >= 0);
}

static void lock(void)
{
	struct sembuf sb;

	sb.sem_num = 0;
	sb.sem_op = -1;
	sb.sem_flg = SEM_UNDO;

	while(semop(semid, &sb, 1) < 0) {
	if(errno != EINTR)
		goto err;
	}

	//DBG(">>>>>>>>>>>>> CONFIG LOCK : %d <<<\n", getpid());
	return;

err:
	ASSERT(! "semaLock failed.");
}

static void unlock(void)
{
	struct sembuf sb;

	sb.sem_num = 0;
	sb.sem_op = 1;
	sb.sem_flg = SEM_UNDO;

	while(semop(semid, &sb, 1) < 0) {
	if(errno != EINTR)
		goto err;
	}

	//DBG(">>>>>>>>>>>>> CONFIG UNLOCK : %d <<<\n", getpid());
	return;

err:
	ASSERT(! "semaLock failed.");
}

/*****************************************************************************/

const struct VMConfig *VMConfigGet(void)
{
	if(config) {
		lock();
		return config;
	}

	initConfig();
	lock();
	return config;
}

void VMConfigPut(const struct VMConfig *ptr)
{
	ASSERT(config == ptr);
	unlock();
}
