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

#include "config.h"
#include "event.h"
#include "notify.h"
#include "pfdebug.h"
//#include "vmutilSystem.h"

/*****************************************************************************/

#ifndef SERVICE_NAME
#define	SERVICE_NAME			"svc.config"
#endif

/*****************************************************************************/

static int config_run = 1;

/*****************************************************************************/

static void run(void)
{
	int ret;
	struct pollfd pollfd[1];

	pollfd[0].fd = configEventGetFd();
	pollfd[0].events = POLLIN;

	for( ; ; )
	{
		if(!config_run)
			break;

		ret = poll(pollfd, 1, PF_DEF_POLL_TIMEOUT_MSEC);
		if(ret <= 0) {
			if(ret < 0) {
				if(errno != EINTR) {
					perror(SERVICE_NAME "::poll");
					exit(EXIT_FAILURE);
				}
			}
		}

		if(pollfd[0].revents & POLLIN) {
			config_run = configEventHandler();
		}

		configUpdate(0);
	}
}

/*****************************************************************************/

int main(int argc, char **argv)
{
	PFWatchdogRegister (argc, argv, EPFWD_MODE_REBOOT, 30);

	notifyInit();
	configEventInit();
	configInit(PFCONFIG_NAME);

	run();

	configEventExit();
	notifyExit();
	
	configUpdate(1);

	PFWatchdogUnregister ();

	return 0;
}

