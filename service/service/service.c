
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "list.h"
#include "pfdefine.h"
#include "pfuthread.h"
#include "pfevent.h"
#include "pfufifo.h"
#include "pfdebug.h"

#include "notify.h"

/*****************************************************************************/

#define SERVICE_FIFO_COUNT	(5)

static pthread_mutex_t		mutex ;
static pthread_cond_t		signal ;
static pthread_t			thid ;
static PFUFifo				*queueHandle = NULL ;
static int fgRun = 1 ;

/*****************************************************************************/

static int doServiceEvent (struct PFEvent *event)
{
	struct PFEServiceSystem *sysevent = (struct PFEServiceSystem *) event ;
	char cmd[MAX_COMMAND_LENGTH] ;
	int fgRequest = 0 ;
	int resultSystem = 0 ;
	int errorNumber = 0 ;

//	DBG("req->eCmdType  : %d\n", sysevent->eCmdType) ;
//	DBG("req->command   : %s\n", sysevent->command) ;
//	DBG("req->resultPath: %s\n", sysevent->resultPath) ;
	switch (event->id)
	{
	case PFE_SERVICE_REQUEST_COMMAND :
		if (event->key) {
			fgRequest = 1 ;
		}
	case PFE_SERVICE_SYSTEM :
		snprintf (cmd, sizeof(cmd), "%s ", sysevent->command) ;
		if (sysevent->resultPath[0] != '\0') {
			strcat(cmd, "> ") ;
			strcat(cmd, sysevent->resultPath) ;
			strcat(cmd, " ") ;
		}
		if (sysevent->eCmdType == ESVC_CMD_BACKGROUND) {
			strcat(cmd, "&") ;
		}
		break;
	default :
		ERR("Unknown Service Event.\n") ;
	}

	DBG("CMD : %s\n", cmd) ;
	resultSystem = system(cmd) ;
	if (resultSystem == -1) {
		errorNumber = errno ;
		ERR2("system(failed)\n") ;
	}

	if (fgRequest) {
		struct PFEServiceReplyCommand reply ;
		reply.result = ERV_OK ;
		reply.resultSystem = resultSystem ;
		reply.errorNumber = errorNumber ;
		doReplyStruct(PFE_SERVICE_REPLY_COMMAND, event->key, &reply, sizeof(reply)) ;
	}

	return WEXITSTATUS(resultSystem) ;
}

/*****************************************************************************/
static void *serviceThread (void *param)
{
	struct timespec ts ;
	void *data ;

	while( fgRun )
	{
		SET_SIGNAL_TIME_MSEC(ts, 1000) ;
		LOCK_MUTEX(mutex) ;
		WAIT_SIGNAL_TIMEOUT(signal, mutex, ts) ;
		UNLOCK_MUTEX(mutex) ;

		data = PFU_dequeueFIFO(queueHandle) ;
		if (data) {
			doServiceEvent(data) ;
			free(data) ;
		}
	}
	
	return NULL ;
}
/*****************************************************************************/


void serviceInit(void)
{
	INIT_SIGNAL(signal) ;
	INIT_MUTEX(mutex) ;
	queueHandle = PFU_createFIFO(SERVICE_FIFO_COUNT) ;
	CREATE_THREAD(thid, serviceThread, 0x1000, NULL, 1) ;
}

void serviceExit(void)
{
	fgRun = 0 ;
	SIGNAL_MUTEX(signal, mutex) ;

}

void serviceAddQueue (struct PFEvent *event)
{
	if (queueHandle) {
		PFU_enqueueFIFO(queueHandle, (void *)event, PFE_PAYLOAD_SIZE(event->id)) ;
	}
	SIGNAL_MUTEX(signal, mutex) ;
}
