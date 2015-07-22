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
#include "vmconfig.h"
#include "vmevent.h"
#include "vmwdt.h"

#include "config.h"
#include "event.h"
#include "notify.h"
#include "debug.h"
#include "vmutilSystem.h"

/*****************************************************************************/

#ifndef SERVICE_NAME
#define	SERVICE_NAME			"svc.config"
#endif

#define	POLL_TIMEOUT			(1000)

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

		ret = poll(pollfd, 1, POLL_TIMEOUT);
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
	VMWatchdogRegister (argc, argv, VMWD_MODE_REBOOT, 30);

//	VMLogInit(SERVICE_NAME);
	notifyInit();
	configEventInit();
	configInit(VM_CONFIG_NAME);

	run();

	configEventExit();
	notifyExit();
	
	configUpdate(1);

	VMWatchdogUnregister ();
	VMUSysUmount ("/data");
	return 0;
}
