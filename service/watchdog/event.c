#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <stdlib.h>

#include "vmq.h"
#include "pflimit.h"
#include "pfevent.h"
#include "pfdebug.h"


/*****************************************************************************/

static struct VMQueue *eventQ;

/*****************************************************************************/

void eventInit(void)
{
	ASSERT(!eventQ);
	eventQ = VMQueueOpen(PFE_QUEUE_NAME, VMQ_MODE_CONSUMER);
	ASSERT(eventQ);
}

void eventExit(void)
{
	ASSERT(eventQ);
	VMQueueClose(eventQ);
	eventQ = (struct VMQueue *)0;
}

int eventGetFd(void)
{
	int fd;
	ASSERT(eventQ);
	fd = VMQueueGetFd(eventQ);
	return fd;
}

void eventHandler(void)
{
	struct PFEvent *event = (struct PFEvent *)VMQueueGetItem(eventQ, NULL);
	ASSERT (event);

	switch(event->id)
	{
#if 0
		case	PFE_SYS_SHUTDOWN:
			kill(getpid(), SIGUSR2);
			break;
#endif
		default:
			break;
	}

	VMQueuePutItem(eventQ, event);
}
