#include <time.h>
#include <assert.h>

#include "vmq.h"
#include "pflimit.h"
#include "pfevent.h"
#include "pfquery.h"
#include "pfconfig.h"
#include "pfdebug.h"

#include "config.h"
#include "service.h"

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

static void eventHandlerConfig (struct PFEvent *event)
{
	switch (event->id)
	{
	case PFE_CONFIG_UPDATE :
		configUpdate(((struct PFEConfigUpdate *)event)->eConfigType) ;
		break;
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

static void eventHandlerService (struct PFEvent *event)
{
	switch (event->id)
	{
	case PFE_SERVICE_SYSTEM :
	case PFE_SERVICE_REQUEST_COMMAND :
		serviceAddQueue(event) ;
		break ;
	}
}

int eventHandler(void)
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
			case EPF_MOD_SYSTEM:	run = eventHandlerSystem(event);		break;
			case EPF_MOD_CONFIG:	eventHandlerConfig(event);				break;
			case EPF_MOD_SERVICE:	eventHandlerService(event);				break;
			default:
				break;
		}
	}

	VMQueuePutItem(eventQ, event);
	return run;
}
