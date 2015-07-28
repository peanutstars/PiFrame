
#include <assert.h>

#include "vmq.h"
#include "pfevent.h"
#include "pfquery.h"

#include "notify.h"
#include "pfdebug.h"

/*****************************************************************************/

static struct VMQueue *eventQ;

/*****************************************************************************/


/**
 * @brief Event Queue 에 쓸 준비를 한다.
 */
void notifyInit(void)
{
	eventQ = VMQueueOpen(PFE_QUEUE_NAME, VMQ_MODE_PRODUCER);
	ASSERT(eventQ);
}

/**
 * @brief Event Queue 에 쓰는것을 종료한다.
 */
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
