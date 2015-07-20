
#include <sys/types.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>


#include "vmq.h"
#include "pflimit.h"
#include "pfevent.h"
#include "pfconfig.h"
#include "pfdebug.h"

/*****************************************************************************/

static struct VMQueue *eventQ;
static unsigned int _statCount[5] = { 0, 0, 0, 0, 0 };

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

#define DUMP_ACTIVE_COUNT		(500)
int eventHandler(void)
{
	struct PFEvent *event;
	int run = 1;

	while((event = (struct PFEvent *)VMQueueGetItem(eventQ, NULL)) != NULL) {

		//	DBG("event->id = %X, PFE_TEST_CHAR = %X\n", event->id, PFE_TEST_CHAR);
		switch(event->id) {
			case	PFE_TEST:
				{
					_statCount[0] ++;
					//			struct PFETest *test = (struct PFETest *)event;
					//			if (_statCount[0]++ > DUMP_ACTIVE_COUNT) {
					//				DBG("%5d] %08X %10d - PFE_TEST\n", getpid(), test->id, test->sec);
					//				_statCount[0] = 0;
					//			}
					break;
				}
			case	PFE_TEST_CHAR:
				{
					struct PFETestChar *test = (struct PFETestChar *)event;
					if (_statCount[1]++ > DUMP_ACTIVE_COUNT) {
						DBG("%5d] %08X %10d - PFE_TEST_CHAR  %2X\n", getpid(), test->id, test->sec, test->data);
						_statCount[1] = 0;
					}
					break;
				}
			case	PFE_TEST_SHORT:
				{
					struct PFETestShort *test = (struct PFETestShort *)event;
					if (_statCount[2]++ > DUMP_ACTIVE_COUNT) {
						DBG("%5d] %08X %10d - PFE_TEST_SHORT %4X\n", getpid(), test->id, test->sec, test->data);
						_statCount[2] = 0;
					}
					break;
				}
			case	PFE_TEST_INT:
				{
					struct PFETestInt *test = (struct PFETestInt *)event;
					if (_statCount[3]++ > DUMP_ACTIVE_COUNT) {
						DBG("%5d] %08X %10d - PFE_TEST_INT   %8X\n", getpid(), test->id, test->sec, test->data);
						_statCount[3] = 0;
					}
					break;
				}
			case	PFE_TEST_LONG:
				{
					struct PFETestLong *test = (struct PFETestLong *)event;
					if (_statCount[4]++ > DUMP_ACTIVE_COUNT) {
						DBG("%5d] %08X %10d - PFE_TEST_LONG  %16lX\n", getpid(), test->id, test->sec, test->data);
						_statCount[4] = 0;
					}
					break;
				}
			case	PFE_TEST_END:
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
