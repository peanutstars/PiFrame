/*

   This code is referenced from linux-2.4.37/kernel/timer.c

 */


#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "list.h"
#include "pfdefine.h"
#include "pfuthread.h"
#include "pfdebug.h"

/*****************************************************************************/
// #define DEBUG_TIMER_TEST	

static pthread_mutex_t		mutex ;
static pthread_cond_t		signal ;
static pthread_t			thid ;
static int fgRun = 1 ;

/*****************************************************************************/
static void *serviceThread (void *param)
{
	struct timespec ts;
	int waitRv ;

	while( fgRun )
	{
		SET_SIGNAL_TIME_MSEC(ts, 1000) ;
		LOCK_MUTEX(mutex) ;
		waitRv = WAIT_SIGNAL_TIMEOUT(signal, mutex, ts) ;
		UNLOCK_MUTEX(mutex) ;

	}
	
	return NULL ;
}
/*****************************************************************************/

void serviceInit(void)
{
	INIT_SIGNAL(signal) ;
	INIT_MUTEX(mutex) ;
	CREATE_THREAD(thid, serviceThread, 0x800, NULL, 1) ;
}

void serviceExit(void)
{
	fgRun = 0 ;
	SIGNAL_MUTEX(signal, mutex) ;
}

