#include <assert.h>

#include "vmq.h"
#include "pfevent.h"

#include "notify.h"
#include "pfdebug.h"

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

void notifyConfigUpdate(int eConfigType)
{
	struct PFEConfigUpdate *event;

	DBG("NOTIFY::ConfigUpdate(%d)\n", eConfigType);
	event = VMQueueGetBuffer(eventQ, sizeof(*event));
	ASSERT(event);
	PFE_INIT_EVENT(event, (PFE_CONFIG_UPDATE));
	event->eConfigType = eConfigType;
	VMQueuePutBuffer(eventQ, event);
}

void notifyReplyNormal (uint32_t key, int result)
{
	struct PFEConfigReplyNormal *event;
	
	event = VMQueueGetBuffer(eventQ, sizeof(*event));
	ASSERT(event);

	PFE_INIT_EVENT(event, (PFE_CONFIG_REPLY_NORMAL));
	event->key = key;
	event->result = result;

	VMQueuePutBuffer(eventQ, event);
}
