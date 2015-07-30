#include <assert.h>
#include <stdio.h>
#include <sys/time.h>

#include "pfutimer.h"
#include "pfevent.h"
#include "pfdebug.h"

#include "notify.h"

struct TimerInfo {
	TimerKey	key ;
	int			hour ;
} ;
static struct TimerInfo timerEveryMinute ;

static void notifyOclock(struct timeval *tv)
{
	struct tm tm ;
	struct PFESystemTime notify ;

	localtime_r(&tv->tv_sec, &tm) ;
	notify.year = tm.tm_year + 1900 ;
	notify.month = tm.tm_mon + 1 ;
	notify.day = tm.tm_mday ;
	notify.hour = tm.tm_hour ;
	notify.min = tm.tm_min ;
	notify.sec = tm.tm_sec ;
	doNotifyStruct (PFE_SYS_TIME, &notify, sizeof(notify)) ;
}

static ETFReturn timerFuncMinute (void *param)
{
	struct TimerInfo *ti = (struct TimerInfo *) param ;
	struct timeval tv ;
	struct tm tm ;
	time_t msec ;
	char strTime[32] ;

	gettimeofday (&tv, NULL) ;
	localtime_r(&tv.tv_sec, &tm) ;
	if (ti->hour != tm.tm_hour) {
		ti->hour = tm.tm_hour ;
		notifyOclock(&tv) ;
	}
	strftime (strTime, sizeof(strTime), "%X", &tm) ;
	DBG("Time : %s - %d.%06d\n", strTime, (int)tv.tv_sec, (int)tv.tv_usec) ;
	
	gettimeofday (&tv, NULL) ;
	msec = 60000 - ((tv.tv_sec % 60) * 1000 + (tv.tv_usec / 1000)) ;
	PFU_timerSetMsec(ti->key, msec) ;

	return ETFUNC_RV_REPEAT ;
}

void timerInit(void)
{
	struct timeval tv ;
	struct tm tm ;

	PFU_timerInit() ;

	/* initialize for o'clock notify. */
	memset(&timerEveryMinute, 0, sizeof(timerEveryMinute)) ;
	gettimeofday (&tv, NULL) ;
	localtime_r(&tv.tv_sec, &tm) ;
	timerEveryMinute.hour = tm.tm_hour ;

	timerEveryMinute.key = PFU_timerAdd (100, timerFuncMinute, &timerEveryMinute, 0) ;
}

void timerExit(void)
{
	if (timerEveryMinute.key) {
		PFU_timerDelete (timerEveryMinute.key) ;
	}
	PFU_timerExit() ;
}
