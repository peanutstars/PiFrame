
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


static void serviceCommand (int argc, char **argv)
{
	struct PFEServiceSystem notify ;

	do
	{
		memset (&notify, 0, sizeof(notify)) ;
		notify.eCmdType = ESVC_CMD_FOREGROUND ;

		snprintf(notify.command, sizeof(notify.command), "%s", argv[0]) ;

		if (argc >= 2 && argv[1]) {
			if ( ! strcasecmp(argv[1], "fg")) {
			} else if ( ! strcasecmp(argv[1], "bg")) {
				notify.eCmdType = ESVC_CMD_BACKGROUND ;
			} else {
				ERR("Unknown parameter - %s\n", argv[1]) ;
				break;
			}
		}
		if (argc >= 3 && argv[2]) {
			snprintf (notify.resultPath, sizeof(notify.resultPath), "%s", argv[2]) ;
		}

		DBG("req.eCmdType   : %d\n", notify.eCmdType) ;
		DBG("req.command    : %s\n", notify.command) ;
		DBG("req.resultPath : %s\n", notify.resultPath) ;
		
		doNotifyStruct(PFE_SERVICE_SYSTEM, &notify, sizeof(notify)) ;
	}
	while (0) ;
}

static void serviceCommandQuery (int argc, char **argv)
{
	struct PFEServiceRequestCommand request ;
	struct PFEServiceReplyCommand *reply ;
	int timeoutSecond = 0;

	do
	{
		memset (&request, 0, sizeof(request)) ;
		request.eCmdType = ESVC_CMD_FOREGROUND ;

		snprintf(request.command, sizeof(request.command), "%s", argv[0]) ;

		if (argc >= 2 && argv[1]) {
			if ( ! strcasecmp(argv[1], "fg")) {
			} else if ( ! strcasecmp(argv[1], "bg")) {
				request.eCmdType = ESVC_CMD_BACKGROUND ;
			} else {
				ERR("Unknown parameter - %s\n", argv[1]) ;
				break;
			}
		}
		if (argc >= 3 && argv[2]) {
			snprintf (request.resultPath, sizeof(request.resultPath), "%s", argv[2]) ;
		}
		if (argc >= 4 && argv[3]) {
			timeoutSecond = strtoul(argv[3], NULL, 0) ;
		}
		
		reply = doRequestStruct (PFE_SERVICE_REQUEST_COMMAND, &request, sizeof(request), timeoutSecond) ;
		if (reply) {
			DBG("PFE_SERVICE_REQUEST_COMMAND is returned %s(%d)\n", PFQueryGetStrEResult(reply->result), reply->result) ;
			DBG("PFE_SERVICE_REPLY_COMMAND, resultSystem : %d(%Xh)\n", reply->resultSystem, reply->resultSystem) ;
			if (reply->resultSystem == -1) {
				DBG("   Error : %s\n", strerror(reply->errorNumber)) ;
			} else {
				int status = reply->resultSystem ;
				if (WIFEXITED(status)) {
					DBG("   exited, status=%d\n", WEXITSTATUS(status));
				} else if (WIFSIGNALED(status)) {
					DBG("   killed by signal %d\n", WTERMSIG(status));
				} else if (WIFSTOPPED(status)) {
					DBG("   stopped by signal %d\n", WSTOPSIG(status));
				} else if (WIFCONTINUED(status)) {
					DBG("   continued\n");
				}
			}
			free (reply) ;
		} else {
			ERR("Timeout( PFE_SERVICE_REQUEST_COMMAND )\n") ;
		}
	}
	while (0) ;
}

struct PFMethod queryService[] = {
	{	"cmd",		1,	"<cmd> [<fg|bg> <rvPath>] : execute command in svc.service.",	serviceCommand,		PFMT_NONE	},
	{	"cmdnRv",	1,	"<cmd> [<fg|gb> <rvPath> <sec>] : execute command with query.",	serviceCommandQuery,PFMT_REPLY	},
	{	NULL, }
} ;

