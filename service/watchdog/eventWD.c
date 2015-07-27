#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>

#include "vmq.h"
#include "pflimit.h"
#include "pfdebug.h"

#include "pfwatchdog.h"
#include "watchdog.h"

/*****************************************************************************/

static struct VMQueue *eventQ;

/*****************************************************************************/

void eventWDInit(void)
{
	ASSERT(!eventQ);
	eventQ = VMQueueOpen(PFWD_QUEUE_NAME, VMQ_MODE_CONSUMER);
	ASSERT(eventQ);
}

void eventWDExit(void)
{
	ASSERT(eventQ);
	VMQueueClose(eventQ);
	eventQ = (struct VMQueue *)0;
}

int eventWDGetFd(void)
{
	int fd;
	ASSERT(eventQ);
	fd = VMQueueGetFd(eventQ);
	return fd;
}

void eventWDHandler(void)
{
	struct PFWatchdog *event;
	int type;
	event = (struct PFWatchdog *)VMQueueGetItem(eventQ, NULL);
	ASSERT (event);

	type = PFWD_TYPE(event->wdid);
	if (type >= EPFWD_CMD_REGISTER && type < EPFWD_CMD_END) {
		watchdogAddEvent (event);
	} else {
		ERR("Unknown watchdog wdid:0x%08X, pid=%d\n", event->wdid, event->pid);
	}	

	VMQueuePutItem(eventQ, event);
}
