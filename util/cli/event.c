
#include <sys/types.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>

#include "vmq.h"
#include "pflimit.h"
#include "pfevent.h"
#include "pfquery.h"
#include "pfconfig.h"
#include "pfdebug.h"

#include "monitoring.h"

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

int eventHandler(void)
{
	struct PFEvent *event ;
	int run = 1 ;

	event = (struct PFEvent *)VMQueueGetItem(eventQ, NULL) ;
	ASSERT (event);

	if ( isMonitoring() ) {
		doMonitoringEvents(event) ;
		if (event->id == PFE_SYS_POWER_OFF) {
			run = 0 ;
		}
	} else {
		if ( PFQueryReplyProcess(event) == 0 ) {
			switch(event->id)
			{
				default:
					DBG("Unexpected event(0x%08x) occurrend. Ignore. key[%X]\n", event->id, event->key);
					break;
			}
		}
	}

	VMQueuePutItem(eventQ, event);

	return run;
}
