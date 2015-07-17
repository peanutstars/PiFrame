
#include <sys/types.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>


#include "vmq.h"
#include "vmlimit.h"
#include "vmevent.h"
#include "vmconfig.h"
#include "debug.h"

/*****************************************************************************/

static struct VMQueue *eventQ;
static unsigned int _statCount[5] = { 0, 0, 0, 0, 0 };

/*****************************************************************************/

void eventInit(void)
{
	ASSERT(!eventQ);
	eventQ = VMQueueOpen(VME_QUEUE_NAME, VMQ_MODE_CONSUMER);
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

#define DUMP_ACTIVE_COUNT		(500)
int eventHandler(void)
{
	struct VMEvent *event;
	int run = 1;

	while((event = (struct VMEvent *)VMQueueGetItem(eventQ, NULL)) != NULL) {

		//	DBG("event->id = %X, VME_TEST_CHAR = %X\n", event->id, VME_TEST_CHAR);
		switch(event->id) {
			case	VME_TEST:
				{
					_statCount[0] ++;
					//			struct VMETest *test = (struct VMETest *)event;
					//			if (_statCount[0]++ > DUMP_ACTIVE_COUNT) {
					//				DBG("%5d] %08X %10d - VME_TEST\n", getpid(), test->id, test->sec);
					//				_statCount[0] = 0;
					//			}
					break;
				}
			case	VME_TEST_CHAR:
				{
					struct VMETestChar *test = (struct VMETestChar *)event;
					if (_statCount[1]++ > DUMP_ACTIVE_COUNT) {
						DBG("%5d] %08X %10d - VME_TEST_CHAR  %2X\n", getpid(), test->id, test->sec, test->data);
						_statCount[1] = 0;
					}
					break;
				}
			case	VME_TEST_SHORT:
				{
					struct VMETestShort *test = (struct VMETestShort *)event;
					if (_statCount[2]++ > DUMP_ACTIVE_COUNT) {
						DBG("%5d] %08X %10d - VME_TEST_SHORT %4X\n", getpid(), test->id, test->sec, test->data);
						_statCount[2] = 0;
					}
					break;
				}
			case	VME_TEST_INT:
				{
					struct VMETestInt *test = (struct VMETestInt *)event;
					if (_statCount[3]++ > DUMP_ACTIVE_COUNT) {
						DBG("%5d] %08X %10d - VME_TEST_INT   %8X\n", getpid(), test->id, test->sec, test->data);
						_statCount[3] = 0;
					}
					break;
				}
			case	VME_TEST_LONG:
				{
					struct VMETestLong *test = (struct VMETestLong *)event;
					if (_statCount[4]++ > DUMP_ACTIVE_COUNT) {
						DBG("%5d] %08X %10d - VME_TEST_LONG  %16lX\n", getpid(), test->id, test->sec, test->data);
						_statCount[4] = 0;
					}
					break;
				}
			case	VME_TEST_END:
				{
					run = 0;
					break;
				}
			default:
				{
					DBG("Unexpected event(0x%08x) occurrend. Ignore.\n", event->id);
					break;
				}
		} // switch

		VMQueuePutItem(eventQ, event);
	} // while

	return run;
}

void eventDumpStatistic (void)
{
	DBG("=== Statistics PID : %d\n", getpid());
	DBG(" %u, %u, %u, %u, %u\n", 
			_statCount[0], _statCount[1], _statCount[2], _statCount[3], _statCount[4]);
}
