#include <assert.h>
#include <stdio.h>
#include <sys/time.h>

#include "pfutimer.h"
#include "pfevent.h"
#include "pfdebug.h"

#include "notify.h"

static TimerKey timerKeyOclock = 0 ;
static TimerKey timerKeyMinute = 0 ;

static ETFReturn timerFuncOclock (void *param)
{
	struct timeval tv ;
	struct tm tm ;
	time_t msec ;
	char strTime[32] ;
	struct PFESystemTime notify ;

	gettimeofday (&tv, NULL) ;
	localtime_r(&tv.tv_sec, &tm) ;
	notify.year = tm.tm_year + 1900 ;
	notify.month = tm.tm_mon + 1 ;
	notify.day = tm.tm_mday ;
	notify.hour = tm.tm_hour ;
	notify.min = tm.tm_min ;
	notify.sec = tm.tm_sec ;
	doNotifyStruct (PFE_SYS_TIME, &notify, sizeof(notify)) ;
	
	strftime (strTime, sizeof(strTime), "%X", &tm) ;
	DBG("Time : %s - %d.%06d\n", strTime, (int)tv.tv_sec, (int)tv.tv_usec) ;
	
	/* Calculate a time for Next Notify */
	gettimeofday (&tv, NULL) ;
	msec = (tv.tv_sec % 3600) * 1000 + (tv.tv_usec / 1000) ;
	msec = 3600000 - msec ;
	DBG("hour msec = %d\n", (int)msec) ;
	PFU_timerSetMsec(timerKeyOclock, msec) ;

	return ETFUNC_RV_REPEAT ;
}

static ETFReturn timerFuncMinute (void *param)
{
	struct timeval tv ;
	struct tm tm ;
	time_t msec ;
	char strTime[32] ;

	gettimeofday (&tv, NULL) ;
	localtime_r(&tv.tv_sec, &tm) ;
	strftime (strTime, sizeof(strTime), "%X", &tm) ;
	DBG("Time : %s - %d.%06d\n", strTime, (int)tv.tv_sec, (int)tv.tv_usec) ;
	
	gettimeofday (&tv, NULL) ;

	msec = (tv.tv_sec % 60) * 1000 + (tv.tv_usec / 1000) ;
	msec = 60000 - msec ;
	DBG("min msec = %d\n", (int)msec) ;
	PFU_timerSetMsec(timerKeyMinute, msec) ;

	return ETFUNC_RV_REPEAT ;
}

void timerInit(void)
{
	PFU_timerInit() ;
	timerKeyOclock = PFU_timerAdd (100, timerFuncOclock, NULL, 0) ;
	timerKeyMinute = PFU_timerAdd (100, timerFuncMinute, NULL, 0) ;
}

void timerExit(void)
{
	if (timerKeyOclock) {
		PFU_timerDelete (timerKeyOclock) ;
	}
	PFU_timerExit() ;
}
