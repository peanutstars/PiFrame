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

#include "pfdefine.h"
#include "vmq.h"
#include "pfusignal.h"

#include "event.h"
#include "eventWD.h"
#include "watchdog.h"
#include "pfdebug.h"

/*****************************************************************************/

#ifndef SERVICE_NAME
#define	SERVICE_NAME			"svc.watchdog"
#endif

#define	POLL_TIMEOUT			(1000)

/*****************************************************************************/

static int g_run = 1;

/*****************************************************************************/
/* signal handling                                                           */

static void sigTermHandler(int signum)
{
	if (signum == SIGUSR1) {
		watchdogDump();
	} else if (signum == SIGUSR2) {
		DBG("Received SIGUSR2, WDT enter to exit processing ...\n");
		g_run = 0;
	} else {
		DBG("Signal catched - %d\n", signum);
	}
}


static void signalInit(void)
{
	PFU_registerSignal (SIGUSR1, sigTermHandler);
	PFU_registerSignal (SIGUSR2, sigTermHandler);
}

/*****************************************************************************/

static void run(void)
{
	int ret;
	struct pollfd pollfd[2];

	pollfd[0].fd = eventWDGetFd();
	pollfd[0].events = POLLIN;
	pollfd[1].fd = eventGetFd();
	pollfd[1].events = POLLIN;

	for( ; ; )
	{
		if( ! g_run)
			break;

		ret = poll(pollfd, 2, POLL_TIMEOUT);
		if(ret <= 0) {
			if(ret < 0) {
				if(errno != EINTR) {
					perror(SERVICE_NAME "::poll");
					exit(EXIT_FAILURE);
				}
			}
		}

		if (pollfd[0].revents & POLLIN) {
			eventWDHandler ();
		}
		if (pollfd[1].revents & POLLIN) {
			eventHandler ();
		}
	}
}

/*****************************************************************************/

int main(int argc, char **argv)
{
	if ( ! access(PF_DEF_MARK_WATCHDOG_DISABLE, F_OK)) {
		DBG(HL_YELLOW "svc.watchdog is running for debug mode !!\n" HL_NONE);
		fgDebugMode = 1;
	}

	signalInit ();

	eventInit ();
	eventWDInit ();
	watchdogInit ();

	run();

	watchdogExit ();
	eventWDExit ();
	eventExit ();

	return 0;
}

