#include <assert.h>

#include "vmq.h"
#include "pfevent.h"
#include "pfquery.h"
#include "pfdebug.h"

#include "notify.h"

/*****************************************************************************/

static struct VMQueue *eventQ;

/*****************************************************************************/


void notifyInit(void)
{
	eventQ = VMQueueOpen(PFE_QUEUE_NAME, VMQ_MODE_PRODUCER);
	ASSERT(eventQ);
}

void notifyExit(void)
{
	ASSERT(eventQ);
	VMQueueClose(eventQ);
}

/******************************************************************************
  notify
 *****************************************************************************/

void doNotifyBasic (uint32_t eid)
{
	struct PFEvent *notify;

	notify = VMQueueGetBuffer (eventQ, sizeof(*notify));
	ASSERT(notify);

	PFE_INIT_EVENT(notify, eid);
	notify->key = 0;

	VMQueuePutBuffer (eventQ, notify);
}

void doNotifyStruct (uint32_t eid, void *edata, int edsize)
{
	struct PFEvent *notify;

	notify = VMQueueGetBuffer (eventQ, edsize);
	ASSERT(notify);

	memcpy (notify, edata, edsize);
	PFE_INIT_EVENT(notify, eid);
	notify->key = 0;

	VMQueuePutBuffer (eventQ, notify);
}

void *doRequestStruct (uint32_t eid, void *edata, int edsize, int timeoutSec)
{
	struct PFEvent *request;
	void *reply = NULL;

	request = VMQueueGetBuffer(eventQ, edsize);
	ASSERT (request);

	memcpy (request, edata, edsize) ;
	PFE_INIT_EVENT(request, eid) ;
	VMQueueGetKey (eventQ, &request->key) ;

	PFQueryWaitReplyWithPutBuffer (eventQ, request, timeoutSec, &reply);

	return reply;
}
