
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <getopt.h>
#include <pthread.h>

#include "vmq.h"
#include "pfconfig.h"
#include "pfevent.h"

#include "event.h"
#include "notify.h"
#include "pfdebug.h"

/*****************************************************************************/

#define	POLL_TIMEOUT			(1000)

/*****************************************************************************/

static volatile int fgRun = 1;

/*****************************************************************************/
/* signal handling                                                           */

static void sigTermHandler(int signum)
{
	DBG("SIGTERM catched.\n");
	fgRun = 0;
}

static void signalInit(void)
{
	struct sigaction act;

	act.sa_handler = sigTermHandler;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	act.sa_restorer = NULL;
	sigaction(SIGTERM, &act, 0); 

	act.sa_handler = sigTermHandler;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	act.sa_restorer = NULL;
	sigaction(SIGINT, &act, 0); 
}

/*****************************************************************************/

static void run(void)
{
	int ret;
	struct pollfd pollfd[1];

	pollfd[0].fd = eventGetFd();
	pollfd[0].events = POLLIN;

	DBG("Receiver [ Start ]\n");
	for( ; ; )
	{
		if( ! fgRun)
			break;

		ret = poll(pollfd, 1, POLL_TIMEOUT);
		if(ret <= 0) {
			if(ret < 0) {
				if(errno != EINTR) {
					perror("dvr.config::poll");
					exit(EXIT_FAILURE);
				}
			}
		}

		if(pollfd[0].revents & POLLIN) {
			fgRun = eventHandler();
			if ( ! fgRun) {
				DBG("ZERO !!!\n");
			}
		}
	}
	DBG("Receiver [ End ]\n");

	eventDumpStatistic();
}

/*****************************************************************************/
enum EVENT_TYPE
{
	ET_NONE			= 0,
	ET_RECEIVER		= 1, 
	ET_SENDER		= 2,
	ET_END
};
enum DATA_TYPE 
{
	DT_NONE = 0,
	DT_CHAR,
	DT_SHORT,
	DT_INT,
	DT_LONG,
	DT_END
};
typedef enum DATA_TYPE data_t;

struct test_option
{
	int		etype;	/* Event Sender / Receiver */
	data_t	dtype;
	int		msec;
	int		sendEnd;
};
typedef struct test_option test_opt_t;

static const char *help_message = \
	"Usage: vmq_test [-m|--mode]=<MODE> [-t|--type]=<TYPE> [-i|--interval]=<MICROSEC>\n"
	"\n"
	"  -m, --mode=<MODE>          mode.\n"
	"                             1: Receiver\n"
	"                             2: Sender\n"
	"                             3: Receiver and Sender\n"
	"  -t, --type=<TYPE>          type.\n"
	"                             0: Make Event without data\n"
	"                             1: Make Event with char\n"
	"                             2: Make Event with short\n"
	"                             3: Make Event with int\n"
	"                             4: Make Event with long\n"
	"  -i, --interval=<MICROSEC>  Micro-secound (Default is 1 sec).\n"
	"  -e, --end                  Sender with a end envent.\n"
	"\n";

static void help(void)
{
    fprintf(stderr, "%s", help_message);
}


static test_opt_t *parse_options (int argc, char *argv[])
{
	test_opt_t *opt = (test_opt_t *) NULL;

	opt = (test_opt_t *) calloc (1, sizeof(*opt));
	if (opt)
	{
		int c, optidx = 0;
		static struct option options[] = { 
			{ "mode",		1, 0, 'm' },
			{ "type",		1, 0, 't' },
			{ "interval",	1, 0, 'i' },
			{ "end",		0, 0, 'e' },
			{ "help",		0, 0, 'h' },
			{ 0, 0, 0, 0 },
		};  

		/* default */
		opt->msec = 1000;

		for( ; ; )
		{ 
			c = getopt_long(argc, argv, "m:t:i:eh", options, &optidx);
			if(c == -1) 
				break;
			switch(c)
			{
				case 'm':
					opt->etype = atoi(optarg) & 0x3;
					if ( opt->etype == ET_NONE) {
						opt->etype = ET_RECEIVER;
					}
					break;
				case 't':
					opt->dtype = (data_t) atoi(optarg);
					break;
				case 'i':
					opt->msec = atoi(optarg);
					break;
				case 'e':
					opt->sendEnd = 1;
					break;
				case 'h':
				default:
					help();
					exit(EXIT_SUCCESS);
					break;
			}   
		}   

		if (opt->etype == ET_NONE) {
			help();
			exit(EXIT_SUCCESS);
		}
	}

	DBG("Parse Option : etype = %X, dtype = %d interver=%dms\n", opt->etype, opt->dtype, opt->msec);

	return opt;
}

/*****************************************************************************/

static pthread_t thid;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond = PTHREAD_COND_INITIALIZER;

void *thread_notify (void *args)
{
	test_opt_t *opt = (test_opt_t *) args;
	struct timeval now;
	struct timeval tvStart, tvEnd;
	struct timespec ts;
	long count = 0;
	uint64_t nsecInterval = opt->msec * 1000000;
	uint64_t temptime;

	DBG("Start Notify Thread\n");
	gettimeofday (&tvStart, NULL);
	for( ; ; ) 
	{
		if (nsecInterval != 0) {
			gettimeofday(&now, NULL);	/* It is just testing code and there is no connection with changing system time */
			//		DBG("%ld.%06ld\n", now.tv_sec, now.tv_usec);

			temptime = now.tv_usec * 1000 + nsecInterval;
			ts.tv_sec  = now.tv_sec;
			while (temptime >= 1000000000)
			{
				temptime -= 1000000000;
				ts.tv_sec ++;
			}
			ts.tv_nsec = temptime;

			//		DBG("%ld.%09ld\n", ts.tv_sec, ts.tv_nsec);
			pthread_mutex_lock(&lock);
			pthread_cond_timedwait(&cond, &lock, &ts);
			if( ! fgRun ) {
				pthread_mutex_unlock(&lock);
				break;
			}   
			pthread_mutex_unlock(&lock);
		} else {
			if (count >= 1000000) {
				break;
			}
		}

		switch (opt->dtype)
		{
		case DT_NONE:
			notifyNoData (PFE_TEST);
			break;
		case DT_CHAR:
			notifyChar (count);
			break;
		case DT_SHORT:
			notifyShort (count);
			break;
		case DT_INT:
			notifyInt (count);
			break;
		case DT_LONG:
			notifyLong (count);
			break;
		default:
			break;
		}

		count ++;
	}
	gettimeofday (&tvEnd, NULL);
	timersub (&tvEnd, &tvStart, &now);
	DBG("Notify         : %ld\n", count);
	DBG("Estimated Time : %ld.%06ld\n", now.tv_sec, now.tv_usec);

	if ( nsecInterval == 0 && opt->sendEnd ) {
		DBG("Notify( PFE_TEST_END )\n");
		notifyNoData (PFE_TEST_END) ;
		fgRun = 0 ;
	} else if ( nsecInterval == 0 ) {
		fgRun = 0 ;
	}

	return NULL;
}

/*****************************************************************************/

int main(int argc, char **argv)
{
	test_opt_t *opt = (test_opt_t *) NULL;

	opt = parse_options (argc, argv);

	signalInit();
	if (opt->etype & ET_SENDER) {
		notifyInit();
		DBG("SENDER\n");
	    pthread_create(&thid, NULL, thread_notify, (void *)opt);
	    pthread_detach(thid);
	}
	if (opt->etype & ET_RECEIVER) {
		DBG("RECEIVER\n");
		eventInit();
		run();
	}
	else {
		for ( ; ; )
		{
			if( ! fgRun)
				break;

			usleep(100000); /* 100ms */
		}
	}


	if (opt->etype & ET_RECEIVER) {
		eventExit();
	}
	if (opt->etype & ET_SENDER) {
		notifyExit();
	}
	
	if (opt)
		free (opt);

	return 0;
}

