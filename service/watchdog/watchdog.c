#include <stdint.h>
#include <sys/types.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdlib.h>
#include <libgen.h>
#include <pthread.h>
#include <inttypes.h>

#include "list.h"
#include "vmq.h"
#include "pflimit.h"
#include "pfdefine.h"
#include "pftype.h"
#include "pfwatchdog.h"
#include "pfusys.h"
#include "pfdebug.h"

#define PFWD_WDT_PROCESS		"svc.micom"
#define PFWD_CMD_WDT_ALIVE		"killall -SIGUSR1 " PFWD_WDT_PROCESS
#define PFWD_CMD_SYS_REBOOT		"killall -SIGUSR2 " PFWD_WDT_PROCESS
#define PFWD_CMD_WDT_EXIT		"killall -SIGALRM " PFWD_WDT_PROCESS
#define PFWD_CMD_KILL_PROCESS	"killall -9 "

/*****************************************************************************/
typedef struct WatchdogInfo {
	pthread_t			thid;
	int					run;
	struct list_head	root;	/* for struct ListService */
	struct timespec		lts;	/* last time spec */
	struct timespec		request_reboot_ts;
	int					bDumpService;
} watchdog_info_t;

struct ListService {
	struct list_head	list;
	struct list_head	subRoot;
	struct PFWDRegister	info;
	int					bActive;
	int					expireCount;
	uint64_t			aliveCount;
	struct timespec		ats;	/* alive/access time spec */
};

struct ListSubService {
	struct list_head	list;
	struct PFWDRegister *info;
	int					subId;
	int					timeout;
	int					expireCount;
	uint64_t			aliveCount;
	struct timespec		ats;
};
#define REQUEST_REBOOT_TIMEOUT		10
/*****************************************************************************/
int fgDebugMode = 0;
static watchdog_info_t		wdi;
static pthread_mutex_t      lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t       cond = PTHREAD_COND_INITIALIZER;
static LIST_HEAD (eventRoot);	/* for struct ListItem */
static volatile int eventQCount = 0;
/*****************************************************************************/
static void discardServiceList (watchdog_info_t *pWDI);
static struct ListService *getServiceListWithName (watchdog_info_t *pWDI, char *name);
/*****************************************************************************/
struct ListItem {
	struct list_head    list;
	void *              data;
};
static struct ListItem *allocItem (const void *data, int size)
{
	struct ListItem *item = NULL;

	if (data && size > 0) {
		item = (struct ListItem *) calloc (1, sizeof(*item));
		ASSERT (item);

		item->data = (void *) malloc (size);
		ASSERT (item->data);

		INIT_LIST_HEAD (&item->list);
		memcpy (item->data, data, size);
	}
	return item;
}

static void freeItem (struct ListItem *item)
{
	if (item->data)
		free (item->data);
	free (item);
}

static void discardItemList (void)
{
	while ( ! list_empty(&eventRoot)) {
		struct ListItem *item;
		item = list_entry (eventRoot.next, struct ListItem, list);
		list_del (&item->list);
		freeItem (item);
	}
}
/*****************************************************************************/
static void discardListAll (watchdog_info_t *pWDI)
{
	pthread_mutex_lock (&lock);
	discardItemList ();
	discardServiceList (pWDI);
	pthread_mutex_unlock (&lock);
}


void watchdogAddEvent (const struct PFWatchdog *event)
{
	struct ListItem *item;

	item = allocItem (event, PFWD_PAYLOAD_SIZE(event->wdid));
	pthread_mutex_lock (&lock);
	list_add_tail (&item->list, &eventRoot);
	eventQCount ++;
	pthread_cond_signal (&cond);
	pthread_mutex_unlock (&lock);
}

static void *watchdogGetEvent (void)
{
	void *data = NULL;

	if ( ! list_empty(&eventRoot)) {
		struct ListItem *item = list_entry(eventRoot.next, struct ListItem, list);
		list_del (&item->list);
		data = item->data;
		free (item);
		eventQCount --;
	}

	return data;
}
/*****************************************************************************/
static void removeSubServiceListwithService (struct ListService *service)
{
	while ( ! list_empty(&service->subRoot)) {
		struct ListSubService *sub = list_entry(service->subRoot.next, struct ListSubService, list);
		list_del (&sub->list);
		free (sub);
	}
	free (service);
}
static int addServiceList (watchdog_info_t *pWDI, const struct PFWDRegister *svcInfo)
{
	struct ListService *service = NULL;
	int fgEnterRegister = 1;
	int rv = -1;

	pthread_mutex_lock (&lock);
	service = getServiceListWithName (pWDI, (char *)svcInfo->name);
	if (service) {
		DBG(HL_YELLOW "WD Already registered : reg(%d:%s) req(%d:%s)\n" HL_NONE, 
				service->info.pid, service->info.name, svcInfo->pid, svcInfo->name);

		if ( PFU_isValidProcess (service->info.pid, service->info.name) ) {
			/* Valid - Old Process */
			DBG(HL_YELLOW "WD It maybe execute twice, %d:%s is killed, now.\n" HL_NONE,	svcInfo->pid, svcInfo->name);
			kill (svcInfo->pid, SIGKILL);
			fgEnterRegister = 0;
		} else {
			/* Invalid - Old Process */
			DBG(HL_YELLOW "WD Already registed, but it is invalid process.\n" HL_NONE);
			list_del (&service->list);
			removeSubServiceListwithService (service);
			service = NULL;
		}
	}
	pthread_mutex_unlock (&lock);

	if ( fgEnterRegister && PFU_isValidProcess (svcInfo->pid, svcInfo->name) ) {
		service = (struct ListService *) calloc (1, sizeof(*service));
		ASSERT (service);

		INIT_LIST_HEAD (&service->list);
		INIT_LIST_HEAD (&service->subRoot);
		memcpy (&service->info, svcInfo, sizeof(*svcInfo));

		pthread_mutex_lock (&lock);
		list_add_tail (&service->list, &pWDI->root);
		pthread_mutex_unlock (&lock);

		rv = 0;
		DBG(HL_YELLOW "WD add service(%d:%s)\n" HL_NONE, svcInfo->pid, svcInfo->name);
	} else {
		if (fgEnterRegister) {
			DBG(HL_YELLOW "WD failed to add service(%d:%s)\n" HL_NONE, svcInfo->pid, svcInfo->name);
		}
	}
	return rv;
}
static int removeServiceList (watchdog_info_t *pWDI, const struct PFWatchdog *event)
{
	struct ListService *service = NULL;
	struct list_head *plist;
	int rv = -1;

	pthread_mutex_lock (&lock);
	list_for_each (plist, &pWDI->root) {
		service = list_entry (plist, struct ListService, list);
		if (service->info.pid == event->pid) {
			if ( ! strcmp(service->info.name, event->name)) {
				list_del (&service->list);
				removeSubServiceListwithService (service);
				rv = 0;
				break;
			}
		}
	}
	pthread_mutex_unlock (&lock);

	return rv;
}
static void discardServiceList (watchdog_info_t *pWDI)
{
	while ( ! list_empty(&pWDI->root)) {
		struct ListService *service;
		service = list_entry (pWDI->root.next, struct ListService, list);
		list_del (&service->list);
		removeSubServiceListwithService (service);
	}
}

static struct ListService *getServiceListWithPID (watchdog_info_t *pWDI, pid_t pid)
{
	struct ListService *service = NULL;
	struct list_head *plist;

	list_for_each (plist, &pWDI->root) {
		service = list_entry (plist, struct ListService, list);
		if (service->info.pid == pid) {
			return service;
		}
	}
	return NULL;
}
static struct ListService *getServiceListWithName (watchdog_info_t *pWDI, char *name)
{
	struct ListService *service = NULL;
	struct list_head *plist;

	list_for_each (plist, &pWDI->root) {
		service = list_entry (plist, struct ListService, list);
		if ( ! strcmp(basename(service->info.name), basename(name))) {
			return service;
		}
	}
	return NULL;
}

static struct ListSubService *getSubServiceListWithSubId (struct ListService *service, uint32_t subId)
{
	struct ListSubService *sub;
	struct list_head *plist;

	list_for_each (plist, &service->subRoot) {
		sub = list_entry (plist, struct ListSubService, list);
		if ( sub->subId == subId) {
			return sub;
		}
	}

	return NULL;
}

static int addSubServiceList (watchdog_info_t *pWDI, const struct PFWDSubRegister *svcSub)
{
	struct ListService *service;
	struct ListSubService *sub;
	int rv = -1;

	pthread_mutex_lock (&lock);
	service = getServiceListWithName (pWDI, (char *)svcSub->name);
	if (service) {
		if ( PFU_isValidProcess (service->info.pid, service->info.name) ) {
			sub = (struct ListSubService *) calloc (1, sizeof(*sub));
			ASSERT (sub);

			INIT_LIST_HEAD (&sub->list);
			sub->info			= &service->info;
			sub->subId			= svcSub->subId;
			sub->timeout		= svcSub->timeout;
			sub->expireCount	= svcSub->timeout;
			clock_gettime (CLOCK_MONOTONIC, &sub->ats);

			list_add_tail (&sub->list, &service->subRoot);
			rv = 0;
			DBG(HL_YELLOW "WD Sub-register, subId(%4d,%04X).\n" HL_NONE, svcSub->subId>>16, svcSub->subId&0xffff);
		} else {
			/* Invalid - Old Process */
			DBG(HL_YELLOW "WD Already registed, but it is invalid process.\n" HL_NONE);
			list_del (&service->list);
			removeSubServiceListwithService (service);
			service = NULL;
		}
	}
	pthread_mutex_unlock (&lock);

	return rv;
}

static int removeSubServiceList (watchdog_info_t *pWDI, const struct PFWDSubUnregister *svcSub)
{
	struct ListService *service;
	int rv = -1;

	pthread_mutex_lock (&lock);
	service = getServiceListWithPID (pWDI, svcSub->pid);
	if (service) {
		struct ListSubService *sub = getSubServiceListWithSubId (service, svcSub->subId);
		if (sub) {
			list_del (&sub->list);
			free (sub);
			rv = 0;
			DBG(HL_YELLOW "WD Sub-unregister, subId(%4d,%04X).\n" HL_NONE, svcSub->subId>>16, svcSub->subId&0xffff);
		} else {
			DBG(HL_RED "WD Sub-unregister, subId=(%4d,%04X) - Not Exist !!\n" HL_NONE, svcSub->subId>>16, svcSub->subId&0xffff);
		}
	}
	pthread_mutex_unlock (&lock);

	return rv;
}

/*****************************************************************************/

static void dumpSubServie (struct ListService *service)
{
	if ( ! list_empty(&service->subRoot)) {
		struct ListSubService *sub;
		struct list_head *plist;
		list_for_each (plist, &service->subRoot)
		{
			sub = list_entry (plist, struct ListSubService, list);
			printf ("   - %4d:%04X, %3d %" PRIu64 "\n", sub->subId>>16, sub->subId&0xffff, sub->expireCount, sub->aliveCount);
		}
	}
}
static void dumpService (int index, struct ListService *service)
{
	const char *strWDMode[EPFWD_MODE_END + 1] = { "NONE", "RESTART", "REBOOT", "UNKNOWN" };
	uint32_t eMode = (service->info.eMode >= EPFWD_MODE_END) ? EPFWD_MODE_END : service->info.eMode;

	printf ("%2d - %4d:%s p[%s] m[%s] t[%d] - a%d %3d %" PRIu64 "\n", 
			index, service->info.pid, service->info.name, service->info.params,
			strWDMode[eMode], service->info.timeout,
			service->bActive, service->expireCount, service->aliveCount);
	dumpSubServie (service);
}

static void dumpServiceList (watchdog_info_t *pWDI)
{
	struct ListService *service = NULL;
	struct list_head *plist;
	int index = 0;

	printf (HL_GREEN);
	printf ("Dump lists - service [ Q Cnt = %d ]\n", eventQCount);
	list_for_each (plist, &pWDI->root) {
		service = list_entry (plist, struct ListService, list);
		dumpService (++index, service);
	}
	printf (HL_NONE "\n");
	fflush (stdout);
	pWDI->bDumpService = 0;
}

void watchdogDump (void)
{
	watchdog_info_t *pWDI = &wdi;

	pthread_mutex_lock (&lock);
	pWDI->bDumpService = 1;
	pthread_cond_signal (&cond);
	pthread_mutex_unlock (&lock);
}
/*****************************************************************************/

static int serviceStartStopWD (watchdog_info_t *pWDI, const struct PFWatchdog *event, int active)
{
	struct ListService *service = NULL;
	int rv = -1;

	pthread_mutex_lock (&lock);
	service = getServiceListWithPID (pWDI, event->pid);
	if (service) {
		if ( ! strcmp(service->info.name, event->name)) {
			service->bActive = active;
			service->expireCount = service->info.timeout + 1;
			clock_gettime (CLOCK_MONOTONIC, &service->ats);
			rv = 0;
		}
	}
	pthread_mutex_unlock (&lock);

	return rv;
}
static int serviceAliveWD (watchdog_info_t *pWDI, const struct PFWatchdog *event)
{
	struct ListService *service = NULL;
	int rv = -1;

	pthread_mutex_lock (&lock);
	service = getServiceListWithPID (pWDI, event->pid);
	if (service) {
		if ( ! strcmp(service->info.name, event->name)) {
			if (service->bActive == 0) {
				service->bActive = 1;
				DBG(HL_YELLOW "WD AUTO STARTED - %d:%s\n" HL_NONE, service->info.pid, service->info.name);
			}
			service->expireCount = service->info.timeout + 1;
			service->aliveCount ++;
			clock_gettime (CLOCK_MONOTONIC, &service->ats);
			rv = 0;
		} else {
			DBG(HL_RED "WD %d:%s - Not Exist !!\n" HL_NONE, service->info.pid, service->info.name);
		}
	}
	pthread_mutex_unlock (&lock);

	return rv;
}

static int serviceSubAliveWD (watchdog_info_t *pWDI, const struct PFWDSubAlive *event)
{
	struct ListService *service;
	int rv = -1;

	pthread_mutex_lock (&lock);
	service = getServiceListWithPID (pWDI, event->pid);
	if (service) {
		if ( ! strcmp(service->info.name, event->name)) {
			struct ListSubService *sub = getSubServiceListWithSubId (service, event->subId);
			if (sub) {
				sub->expireCount = sub->timeout + 1;
				sub->aliveCount ++;
				clock_gettime (CLOCK_MONOTONIC, &sub->ats);
				rv = 0;
			} else {
				DBG(HL_RED "WD %d:%s, subId=%4d:%04X - Not Exist !!\n" HL_NONE, 
						service->info.pid, service->info.name, event->subId>>16, event->subId&0xffff);
			}
		} else {
			DBG(HL_RED "WD %d:%s - Not Exist !!\n" HL_NONE, service->info.pid, service->info.name);
		}
	}
	pthread_mutex_unlock (&lock);

	return rv;
}

/*****************************************************************************/

void processWDStop (watchdog_info_t *pWDI)
{
	pWDI->run = 0;
	pthread_mutex_lock (&lock);
	pthread_cond_signal (&cond);
	pthread_mutex_unlock (&lock);
}

static void processWDEvent (watchdog_info_t *pWDI, const struct PFWatchdog *event)
{
	switch (event->wdid)
	{
		case	PFWD_ALIVE:
			// DBG("WD ALIVE (%d:%s)\n", event->pid, event->name);
			serviceAliveWD (pWDI, event);
			break;
		case	PFWD_SUB_ALIVE:
			// DBG("WD SUB ALIVE (%d:%s)\n", event->pid, event->name);
			serviceSubAliveWD (pWDI, (const struct PFWDSubAlive *)event);
			break;
		case	PFWD_REGISTER:
			DBG("WD REGISTER (%d:%s)\n", event->pid, event->name);
			addServiceList (pWDI, (const struct PFWDRegister *)event);
			break;
		case	PFWD_UNREGISTER:
			DBG("WD UNREGISTER (%d:%s)\n", event->pid, event->name);
			removeServiceList (pWDI, event);
			break;
		case	PFWD_START:
			DBG("WD START (%d:%s)\n", event->pid, event->name);
			serviceStartStopWD (pWDI, event, 1);
			break;
		case	PFWD_STOP:
			DBG("WD STOP (%d:%s)\n", event->pid, event->name);
			serviceStartStopWD (pWDI, event, 0);
			break;
		case	PFWD_SUB_REGISTER:
			DBG("WD SUB REGISTER (%d:%s)\n", event->pid, event->name);
			addSubServiceList (pWDI, (const struct PFWDSubRegister *)event);
			break;
		case	PFWD_SUB_UNREGISTER:
			DBG("WD SUB UNREGISTER (%d:%s)\n", event->pid, event->name);
			removeSubServiceList (pWDI, (const struct PFWDSubUnregister *)event);
			break;
	}
}

static void watchdogAction (watchdog_info_t *pWDI, struct ListService *service)
{
	char cmd[1024];

	switch (service->info.eMode)
	{
		case	EPFWD_MODE_NONE:
			/* nothing */
			if (fgDebugMode) {
				ERR("\n");
				ERR("\n");
				ERR("Please check program, %s, it is NONE mode.\n", service->info.name);
				ERR("\n");
				system ("/system/script/vmrun stop &");
			}
			break;
		case	EPFWD_MODE_RESTART:
			if (fgDebugMode) {
				ERR("\n");
				ERR("\n");
				ERR("It is debug mode, Do NOT Execute RESTART !!. \n");
				ERR("Please check program, %s.\n", service->info.name);
				ERR("\n");
				system ("/system/script/vmrun stop &");
			} else {
				snprintf (cmd, sizeof(cmd), PFWD_CMD_KILL_PROCESS "%s", basename(service->info.name));
				DBG(HL_YELLOW "WD RESTART - %s\n" HL_NONE, cmd);
				system (cmd);
				usleep (50000);
				snprintf (cmd, sizeof(cmd), "%s %s &", service->info.name, service->info.params);
				DBG(HL_YELLOW "WD RESTART - %s\n" HL_NONE, cmd);
				sync();
				sync();
				system (cmd);
			}
			break;
		case	EPFWD_MODE_REBOOT:
			if (fgDebugMode) {
				ERR("\n");
				ERR("\n");
				ERR("It is debug mode, Do NOT Execute REBOOT !!. \n");
				ERR("Please check program, %s.\n", service->info.name);
				ERR("\n");
				system ("/system/script/vmrun stop &");
			} else {
				DBG("##############################################################\n");
				DBG("####################### DEBUG MESSAGE ########################\n");
				DBG("##############################################################\n");
				DBG(HL_YELLOW "WD REBOOT - " PFWD_CMD_SYS_REBOOT "\n" HL_NONE);
				DBG(HL_YELLOW "Please check program, %s.\n" HL_NONE, service->info.name);
				sync();
				sync();
				system ("dmesg");
				sync();
				sync();
				ERR("############# Request Micom Watchdog ##############\n");
				ERR("## svc.watchdog exit to force !!\n");
				ERR("############# Request Micom Watchdog ##############\n");
				exit (1);

				system (PFWD_CMD_SYS_REBOOT) ;
				clock_gettime (CLOCK_MONOTONIC, &pWDI->request_reboot_ts);
			}
			break;
	}

	removeSubServiceListwithService (service);
}

static int watchdogSubServiceUpdate (struct ListService *service, struct timespec *cts)
{
	struct ListSubService *sub;
	struct list_head *plist;
	int fgActWD = 0;

	if ( ! list_empty(&service->subRoot)) {
		list_for_each (plist, &service->subRoot)
		{
			sub = list_entry (plist, struct ListSubService, list);
			
			sub->expireCount -= (cts->tv_sec - sub->ats.tv_sec);
			sub->ats = *cts;

			if (sub->expireCount < 0) {
				fgActWD = 1;
				DBG(HL_YELLOW "WD SubService(%d:%04X) Expired - %d:%s\n" HL_NONE, 
						sub->subId>>16, sub->subId&0xffff, sub->info->pid, sub->info->name);
				break;
			}
		}
	}

	return fgActWD;
}

static void watchdogServiceUpdate (watchdog_info_t *pWDI, struct timespec *cts)
{
	struct ListService *service;
	struct list_head *plist;

	pthread_mutex_lock (&lock);
	list_for_each (plist, &pWDI->root)
	{
		int fgActWD = 0;

		service = list_entry (plist, struct ListService, list);
		if ( PFU_isValidProcess(service->info.pid, service->info.name) ) {
			if (service->bActive) {
				service->expireCount -= (cts->tv_sec - service->ats.tv_sec);
				service->ats = *cts;
			}
			if (service->expireCount < 0) {
				fgActWD = 1;
			}
		} else {
			/* process is vanished into thin air. */
			fgActWD = 2;
		}
		if ( ! fgActWD) {
			fgActWD = watchdogSubServiceUpdate (service, cts);
		}

		if (fgActWD) {
			struct list_head *pPrev= plist->prev;
			ERR("WD %d:%s - %s\n", service->info.pid, service->info.name, (fgActWD==1) ? "Expired" : "Vanished");
			list_del (&service->list);
			plist = pPrev;
			watchdogAction (pWDI, service);
			dumpServiceList (pWDI);
		}
	}
	pthread_mutex_unlock (&lock);
}

static void processWDUpdate (watchdog_info_t *pWDI)
{
	struct timespec cts;

	clock_gettime (CLOCK_MONOTONIC, &cts);
	
	if (pWDI->request_reboot_ts.tv_sec != 0) {
		if ((cts.tv_sec - pWDI->request_reboot_ts.tv_sec) >= REQUEST_REBOOT_TIMEOUT) {
			ERR("WD Software Reboot - Hardware Watchdog wrong ??\n");
			system ("reboot");
		}
	} else {
		if ((cts.tv_sec - pWDI->lts.tv_sec) >= PF_DEF_WATCHDOG_PERIOD) {
			watchdogServiceUpdate (pWDI, &cts);
			pWDI->lts = cts;
		}
	}
}

static void *processWatchdog (void *param)
{
	watchdog_info_t *pWDI = (watchdog_info_t *) param;
	struct PFWatchdog *event;
	struct timespec ts;
	struct timespec mwdt_ts;
	uint32_t dumpCount = 0;

	clock_gettime (CLOCK_MONOTONIC, &mwdt_ts);

	DBG("<< Start processWatchdog >>\n");
	while ( pWDI->run ) {
		clock_gettime (CLOCK_MONOTONIC, &ts);
		ts.tv_sec += 1;
		dumpCount ++;

		pthread_mutex_lock (&lock);
		pthread_cond_timedwait (&cond, &lock, &ts);

		event = watchdogGetEvent ();
		pthread_mutex_unlock (&lock);

		if ( ! pWDI->run) {
			break;
		}

		if (pWDI->bDumpService || (dumpCount & 0x7F)==0x7F) {
			pthread_mutex_lock (&lock);
			dumpServiceList (pWDI);
			pthread_mutex_unlock (&lock);
		}

		if ((ts.tv_sec - mwdt_ts.tv_sec) > PFWD_DEFAULT_ALIVE_TIMEOUT) {
			mwdt_ts = ts;
			/* send a signal of alive to svc.micom */
			if ( ! fgDebugMode) {
//				DBG("@@ WD Alive @@\n");
				system (PFWD_CMD_WDT_ALIVE) ;
			}
		}

		do
		{
			if (event) {
				processWDEvent (pWDI, event);
				free (event);
			}

			/*	add 20150424 by hslee
				event Q is increased when the evnet comes at one point and at the same time,
				but event process thread is processing only one evnet per signal. 
				And then it adds a follow codec */
			pthread_mutex_lock (&lock);
			event = watchdogGetEvent ();
			pthread_mutex_unlock (&lock);
		}
		while (event);

		processWDUpdate (pWDI);
	}
	DBG("<< End processWatchdog >>\n");
	return NULL;
}
/*****************************************************************************/

void watchdogInit (void)
{
	watchdog_info_t *pWDI = &wdi;

	memset (pWDI, 0, sizeof(*pWDI));

	pthread_condattr_t cattr;
	pthread_condattr_init (&cattr);
	pthread_condattr_setclock (&cattr, CLOCK_MONOTONIC);
	pthread_cond_init (&cond, &cattr);

	pWDI->run = 1;
	INIT_LIST_HEAD(&pWDI->root);
	clock_gettime (CLOCK_MONOTONIC, &pWDI->lts);

	pthread_create (&pWDI->thid, NULL, processWatchdog, pWDI);
	pthread_detach(pWDI->thid);

}
void watchdogExit (void)
{
	watchdog_info_t *pWDI = &wdi;

	system (PFWD_CMD_WDT_EXIT) ;

	if ( pWDI->run ) {
		processWDStop (pWDI);
	}
	usleep(200000);
	discardListAll (pWDI);
	pthread_cond_destroy (&cond);
}
