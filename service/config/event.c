#include <time.h>
#include <assert.h>

#include "vmq.h"
#include "pflimit.h"
#include "pfevent.h"
#include "pfquery.h"
#include "pfconfig.h"

#include "config.h"
#include "debug.h"

#include "notify.h"
#include "config-runtime.h"

/*****************************************************************************/

static struct VMQueue *eventQ;

/*****************************************************************************/

void configEventInit(void)
{
	ASSERT(!eventQ);
	eventQ = VMQueueOpen(PFE_QUEUE_NAME, VMQ_MODE_CONSUMER);
	ASSERT(eventQ);
}

void configEventExit(void)
{
	ASSERT(eventQ);
	VMQueueClose(eventQ);
	eventQ = (struct VMQueue *)0;
}

int configEventGetFd(void)
{
	int fd;
	ASSERT(eventQ);
	fd = VMQueueGetFd(eventQ);
	return fd;
}

static void eventHandlerConfig (struct PFEvent *event)
{
	switch (event->id)
	{
		case	PFE_CONFIG_REQUEST_EXPORT:
		{
			struct PFEConfigRequestExport *request = (struct PFEConfigRequestExport *)event;
			int result = configExport (request->mask, request->path);
			if ( request->key ) {
				notifyReplyNormal (request->key, result);
			}
			break;
		}
	}
}

static int eventHandlerSystem (struct PFEvent *event)
{
	int run = 1;
	switch (event->id)
	{
		case	PFE_SYS_POWER_OFF:
			run = 0 ;
			break ;
		case	PFE_SYS_SHUTDOWN:
			run = 0 ;
			break;
	}
	return run ;
}

int configEventHandler(void)
{
	int run = 1;
	struct PFEvent *event;
	event = (struct PFEvent *)VMQueueGetItem(eventQ, NULL);
	ASSERT (event);

	if ( PFQueryReplyProcess (event) == 0 )
	{
		EPfModule moduleId  = PFE_MODULE_ID(event->id);
		switch (moduleId)
		{
			case EPF_MOD_SYSTEM:	run = eventHandlerSystem (event);		break;
			case EPF_MOD_CONFIG:	eventHandlerConfig (event);				break;
			default:
				break;
		}
	}

	VMQueuePutItem(eventQ, event);
	return run;
}
