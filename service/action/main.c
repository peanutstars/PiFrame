#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <assert.h>

#include "vmq.h"
#include "pfdefine.h"
#include "pfconfig.h"
#include "pfevent.h"
#include "pfwdt.h"
#include "pfdebug.h"
#include "pfusignal.h"

#include "event.h"
#include "notify.h"
#include "config.h"
#include "timer.h"

/*****************************************************************************/

#ifndef SERVICE_NAME
#define	SERVICE_NAME			"svc.action"
#endif

/*****************************************************************************/

static int fgRun = 1;

/*****************************************************************************/

static void signalHandler (int signum)
{
	DBG("Catched signal, #%d\n", signum) ;
	if (signum == SIGTERM || signum == SIGINT ) {
		fgRun = 0 ;
	}
}
static void signalInit (void)
{
	PFU_registerSignal(SIGTERM, signalHandler) ;
	PFU_registerSignal(SIGINT, signalHandler) ;
}

static void run(void)
{
	int ret;
	struct pollfd pollfd[1];

	pollfd[0].fd = eventGetFd();
	pollfd[0].events = POLLIN;

	for( ; fgRun ; )
	{
		ret = poll(pollfd, 1, PF_DEF_POLL_TIMEOUT_MSEC);
		if(ret <= 0) {
			if(ret < 0) {
				if(errno != EINTR) {
					ERR2("poll\n") ;
					exit(EXIT_FAILURE);
				}
			}
		}

		if(pollfd[0].revents & POLLIN) {
			fgRun = eventHandler();
		}
	}
}

/*****************************************************************************/

int main(int argc, char **argv)
{
	PFWatchdogRegister (argc, argv, EPFWD_MODE_NONE, 30) ;

	signalInit() ;
	notifyInit() ;
	eventInit() ;
	configInit() ;
	timerInit() ;

	run() ;

	timerExit() ;
	configExit() ;
	eventExit() ;
	notifyExit() ;

	PFWatchdogUnregister () ;

	return 0;
}

