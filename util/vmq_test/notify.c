
#include <assert.h>

#include "vmq.h"
#include "vmevent.h"

#include "notify.h"
#include "debug.h"

/*****************************************************************************/

static struct VMQueue *eventQ;

/*****************************************************************************/


/**
 * @brief Event Queue 에 쓸 준비를 한다.
 */
void notifyInit(void)
{
	eventQ = VMQueueOpen(VME_QUEUE_NAME, VMQ_MODE_PRODUCER);
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

void notifyNoData (int type)
{
	struct VMETest *event;

	event = VMQueueGetBufferWithKey (eventQ, sizeof(*event));
	ASSERT(event);

	VME_INIT_EVENT(event, type);

//	DBG("Key = %X\n", event->key);
	VMQueuePutBuffer (eventQ, event);
}

void notifyChar (unsigned char data)
{
	struct VMETestChar *event;

	event = VMQueueGetBuffer (eventQ, sizeof(*event));
	ASSERT(event);

	VME_INIT_EVENT(event, (VME_TEST_CHAR));
	event->data = data;

	DBG("Key = %X, data = %X\n", event->key, data);
	VMQueuePutBuffer (eventQ, event);
}

void notifyShort (unsigned short data)
{
	struct VMETestShort *event;

	event = VMQueueGetBufferWithKey (eventQ, sizeof(*event));
	ASSERT(event);

	VME_INIT_EVENT(event, (VME_TEST_SHORT));
	event->data = data;

	DBG("Key = %X\n", event->key);
	VMQueuePutBuffer (eventQ, event);
}

void notifyInt (unsigned int data)
{
	struct VMETestInt *event;

	event = VMQueueGetBuffer (eventQ, sizeof(*event));
	ASSERT(event);

	VME_INIT_EVENT(event, (VME_TEST_INT));
	event->data = data;

	DBG("Key = %X\n", event->key);
	VMQueuePutBuffer (eventQ, event);
}

void notifyLong (unsigned long data)
{
	struct VMETestLong *event;

	event = VMQueueGetBufferWithKey (eventQ, sizeof(*event));
	ASSERT(event);

	VME_INIT_EVENT(event, (VME_TEST_LONG));
	event->data = data;

	DBG("Key = %X\n", event->key);
	VMQueuePutBuffer (eventQ, event);
}

