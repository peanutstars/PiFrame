
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pfdefine.h"
#include "pfconfig.h"
#include "pfevent.h"
#include "pfquery.h"

#include "notify.h"
#include "method.h"
#include "pfdebug.h"

static int _fgRunMonitoring = 0 ;

int isMonitoring (void)
{
	return _fgRunMonitoring ;
}

struct EventInfo {
	struct PFEvent *event ;
	char strHeader[64] ;
	char strEvent[32] ;
	char strBody[512] ;
} ;

static void sprintEventHeader(struct EventInfo *ei)
{
	time_t rawTime = ei->event->sec ;
	struct tm tm ;
	char strTime[16] ;

	localtime_r (&rawTime, &tm) ;
	strftime (strTime, sizeof(strTime), "%X", &tm) ;

	snprintf (ei->strHeader, sizeof(ei->strHeader), "@%s]%5d:%3d:%3d:%4d:%Xh:%08XK",
			strTime, ei->event->srcPid, 
			PFE_MODULE_ID(ei->event->id), PFE_EVENT_ID(ei->event->id),
			PFE_PAYLOAD_SIZE(ei->event->id), PFE_EVENT_FLAG(ei->event->id), ei->event->key) ;
}

#define CASE(x)		case x : strcat(ei->strEvent, #x)

static void printSystemEvent(struct EventInfo *ei)
{
	struct PFEvent *event = ei->event ;
	switch (event->id)
	{
	CASE(PFE_SYS_DUMMY) ;	
		break ;
	CASE(PFE_SYS_POWER_OFF) ;
		break ;
	CASE(PFE_SYS_SHUTDOWN) ;
		break ;
	default : 
		strcat (ei->strEvent, "SYSTEM") ;
	}
	printf ("%s %s %s\n", ei->strHeader, ei->strEvent, ei->strBody) ;
}
static void printServiceEvent(struct EventInfo *ei)
{
	struct PFEvent *event = ei->event ;
	switch (event->id)
	{
	default :
		strcat (ei->strEvent, "SERVICE") ;
	}
	printf ("%s %s %s\n", ei->strHeader, ei->strEvent, ei->strBody) ;
}
static void printConfigEvent(struct EventInfo *ei)
{
	struct PFEvent *event = ei->event ;
	switch (event->id)
	{
	CASE(PFE_CONFIG_UPDATE) ;
		break ;
	CASE(PFE_CONFIG_REPLY_NORMAL) ;
		break ;
	CASE(PFE_CONFIG_REQUEST_EXPORT) ;
		break ;
	default :
		strcat (ei->strEvent, "CONFIG") ;
	}
	printf ("%s %s %s\n", ei->strHeader, ei->strEvent, ei->strBody) ;
}
static void printActionEvent(struct EventInfo *ei)
{
	struct PFEvent *event = ei->event ;
	switch (event->id)
	{
	default :
		strcat (ei->strEvent, "ACTION") ;
	}
	printf ("%s %s %s\n", ei->strHeader, ei->strEvent, ei->strBody) ;
}

void doMonitoringEvents (struct PFEvent *event)
{
	struct EventInfo ei = { 
		.event = event, 
		.strHeader = "",
		.strEvent = "",
		.strBody = ""
	} ;

	sprintEventHeader (&ei) ;

	switch (PFE_MODULE_ID(event->id))
	{
	case EPF_MOD_SYSTEM :	printSystemEvent(&ei) ;		break ;
	case EPF_MOD_SERVICE :	printServiceEvent(&ei) ;	break ;
	case EPF_MOD_CONFIG :	printConfigEvent(&ei) ;		break ;
	case EPF_MOD_ACTION :	printActionEvent(&ei) ;		break ;
	default :
		printf("%s\n", ei.strHeader) ;
	}
}

static void Monitoring (int argc, char **argv)
{
	_fgRunMonitoring = 1 ;

	printf ("\n----- Start Monitoring -----\n\n") ;
}

struct PFMethod monitoring[] = {
	{	"start",		0,	"n/a : Monitoring events",			Monitoring,				PFMT_NONE	},
	{	NULL, }
};

