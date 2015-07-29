
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
#include <pthread.h>

#include "vmq.h"
#include "pfdefine.h"
#include "pfconfig.h"
#include "pfevent.h"
#include "pfquery.h"
#include "pfusignal.h"

#include "event.h"
#include "notify.h"
#include "method.h"
#include "monitoring.h"
#include "pfdebug.h"

/*****************************************************************************/

static int g_run = 1;
static int g_thrun = 0;

/*****************************************************************************/

extern struct PFMethod monitoring[] ;
extern struct PFMethod notifySystem[] ;
extern struct PFMethod queryConfig[] ;

struct PFMethodPool {
	const char *name ;
	struct PFMethod *method ;
} ;

struct PFMethodPool methodPool[] = {
	{	"monitoring",	monitoring		} ,
	{	"notify",		notifySystem	} ,
	{	"config",		queryConfig		} ,
	{	NULL,							}
} ;

/*****************************************************************************/
/* signal handling                                                           */

static void sigTermHandler(int signum)
{
	DBG("Signal catched - %d\n", signum);
	g_run = 0;
}


static void signalInit(void)
{
	PFU_registerSignal (SIGTERM, sigTermHandler);
	PFU_registerSignal (SIGINT, sigTermHandler);
}

/*****************************************************************************/

static void *do_request_thread(void *args)
{
	int ret;
	struct pollfd pollfd[1];

	pollfd[0].fd = eventGetFd();
	pollfd[0].events = POLLIN;

	for( ; ; )
	{
		if( ! g_run)
			break;

		g_thrun = 1;
		ret = poll(pollfd, 1, PF_DEF_POLL_TIMEOUT_MSEC);
		if(ret <= 0) {
			if(ret < 0) {
				if(errno != EINTR) {
					perror("cli::poll") ;
					exit(EXIT_FAILURE);
				}
			}
		}

		if(pollfd[0].revents & POLLIN) {
			eventHandler();
		}
	}

	PFQueryWakeUpAll ();

	g_thrun = 0;
	return NULL;
}

/*****************************************************************************/
static void help(int argc, char **argv)
{
	static const char *usage = "usage: cli <cmd> <method> [<param>, ...]";
	struct PFMethod *method;
	int x, y;

	fprintf(stderr, "%s\n\n", usage);
	if (argc < 2) {
		for (x=0; methodPool[x].name; x++) {
			method = methodPool[x].method;
			for (y=0; method[y].name; y++) {
				fprintf(stderr, "\t%s %s  - %s\n", methodPool[x].name, method[y].name, method[y].help);
			}
		}
	}
	else {
		for (x=0; methodPool[x].name; x++) {
			method = methodPool[x].method;
			if ( ! strcmp(methodPool[x].name, argv[1])) {
				for (y=0; method[y].name; y++)
				fprintf(stderr, "\t%s %s  - %s\n", methodPool[x].name, method[y].name, method[y].help);
				break;
			}
		}
	}
	exit (EXIT_FAILURE);
}

static void methodHandler(struct PFMethod *methods, int argc, char **argv)
{
	pthread_t thid;
	struct PFMethod *m; 

	for(m = methods; m->name; m++) {
		if(strcmp(m->name, argv[2]) == 0)
		{
			if (m->minArgc > (argc - 3)) {
				help (argc, argv);
			}

			if (m->type == PFMT_REPLY)
			{
				pthread_create(&thid, NULL, do_request_thread, NULL);
				pthread_detach(thid);

				while( ! g_thrun) {
					usleep(10000);		/* 10 ms */
				}
			}

			m->stub(argc - 3, &argv[3]);
			return;
		}   
	}   
	fprintf(stderr, "The %s's sub-command, '%s' is not found.\n", argv[1], argv[2]);
	fprintf(stderr, "Please check syntax of command and try again.\n\n");

	exit(EXIT_FAILURE);
}


/*****************************************************************************/

int main(int argc, char **argv)
{
	int i;
	int fg_executed = 0;

	if ( ! getenv("LD_LIBRARY_PATH") ) {
		setenv ("LD_LIBRARY_PATH", "/system/lib", 1);
	}

	if (argc < 3) {
		help (argc, argv);
	}

	signalInit();
	notifyInit();
	eventInit();

	for(i = 0; methodPool[i].name; i++) {
		if(strcmp(argv[1], methodPool[i].name) == 0) {
			methodHandler (methodPool[i].method, argc, argv);
			fg_executed = 1;
			break;
		}   
	}   

	if ( ! fg_executed) {
		fprintf(stderr, "The category of command is not found - %s\n\n", argv[1]);
	}

	if ( isMonitoring() ) {
		do_request_thread (NULL);
	}

	eventExit();
	notifyExit();

	return EXIT_SUCCESS;
}

