
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

#include "enum2str.h"

static int _fgRunMonitoring = 0 ;

int isMonitoring (void)
{
	return _fgRunMonitoring ;
}

struct EventInfo {
	struct PFEvent *event ;
	char strHeader[64] ;
	char strEvent[32] ;
	char strBody[768] ;
} ;

static void sprintEventHeader(struct EventInfo *ei)
{
	time_t rawTime = ei->event->evtsec ;
	struct tm tm ;
	char strTime[16] ;

	localtime_r (&rawTime, &tm) ;
	strftime (strTime, sizeof(strTime), "%X", &tm) ;

	snprintf (ei->strHeader, sizeof(ei->strHeader), "@%s]%5d:%3d:%3d:%4d:%Xh:%08XK",
			strTime, ei->event->srcPid, 
			PFE_MODULE_ID(ei->event->id), PFE_EVENT_ID(ei->event->id),
			PFE_PAYLOAD_SIZE(ei->event->id), PFE_EVENT_FLAG(ei->event->id), ei->event->key) ;
}

#define CASE(x)		case x : { strcat(ei->strEvent, #x)
#define BREAK		break ; }

static void printSystemEvent(struct EventInfo *ei)
{
	switch (ei->event->id)
	{
	CASE(PFE_SYS_DUMMY) ;	
		BREAK ;
	CASE(PFE_SYS_POWER_OFF) ;
		BREAK ;
	CASE(PFE_SYS_SHUTDOWN) ;
		BREAK ;
	CASE(PFE_SYS_TIME) ;
		struct PFESystemTime *event = (struct PFESystemTime *)ei->event ;
		snprintf (ei->strBody, sizeof(ei->strBody), "%u.%u.%u %02u:%02u:%02u",
				event->year, event->month, event->day, event->hour, event->min, event->sec) ;
		BREAK ;
	default : 
		strcat (ei->strEvent, "SYSTEM") ;
	}
	printf ("%s %s %s\n", ei->strHeader, ei->strEvent, ei->strBody) ;
}
static void printServiceEvent(struct EventInfo *ei)
{
	switch (ei->event->id)
	{
	CASE(PFE_SERVICE_SYSTEM) ;
		struct PFEServiceSystem *event = (struct PFEServiceSystem *)ei->event ;
		snprintf(ei->strBody, sizeof(ei->strBody), "T:%s C:'%s' R:%s",
				getStrServiceCommandType(event->eCmdType), event->command, event->resultPath) ;
		BREAK ;
	CASE(PFE_SERVICE_REQUEST_COMMAND) ;
		struct PFEServiceSystem *event = (struct PFEServiceSystem *)ei->event ;
		snprintf(ei->strBody, sizeof(ei->strBody), "T:%s C:'%s' R:%s",
				getStrServiceCommandType(event->eCmdType), event->command, event->resultPath) ;
		BREAK ;
	CASE(PFE_SERVICE_REPLY_COMMAND) ;
		struct PFEServiceReplyCommand *reply = (struct PFEServiceReplyCommand *)ei->event ;
		snprintf (ei->strBody, sizeof(ei->strBody), "QR:%d resultSystem:%d(%Xh), errorNumber:%d\n", 
				reply->result, reply->resultSystem, reply->resultSystem, reply->errorNumber) ;
		BREAK ;
	default :
		strcat (ei->strEvent, "SERVICE") ;
	}
	printf ("%s %s %s\n", ei->strHeader, ei->strEvent, ei->strBody) ;
}


static void printConfigEvent(struct EventInfo *ei)
{
	switch (ei->event->id)
	{
	CASE(PFE_CONFIG_UPDATE) ;
		struct PFEConfigUpdate *event = (struct PFEConfigUpdate *) ei->event ;
		snprintf (ei->strBody, sizeof(ei->strBody), "eConfigType:%u,%s\n", event->eConfigType, getStrConfigType(event->eConfigType)) ;
		BREAK ;
	CASE(PFE_CONFIG_REPLY_NORMAL) ;
		BREAK ;
	CASE(PFE_CONFIG_REQUEST_EXPORT) ;
		BREAK ;
	default :
		strcat (ei->strEvent, "CONFIG") ;
	}
	printf ("%s %s %s\n", ei->strHeader, ei->strEvent, ei->strBody) ;
}
static void printActionEvent(struct EventInfo *ei)
{
	switch (ei->event->id)
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

