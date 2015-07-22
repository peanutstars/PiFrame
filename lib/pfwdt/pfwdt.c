#include <assert.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>

#include "vmq.h"
#include "pfwatchdog.h"

#include "pfdebug.h"

/*****************************************************************************/
struct PFWDInfo {
	struct VMQueue			*eventQ;
	struct PFWDRegister		info;
	int						subCount;
};

static struct PFWDInfo wdi = {
	.eventQ			= NULL,
	.info			= {
		.wdid		= 0,
		.pid		= 0,
		.name		= "\0",
		.params		= "\0",
		.eMode		= EPFWD_MODE_NONE,
		.timeout	= 0,
	}
};
/*****************************************************************************/

static void doSendWDPacket (struct PFWDInfo *pWDI, uint32_t wdid, void *edata, int edsize)
{
	struct PFWatchdog *packet;

	packet = VMQueueGetBuffer (pWDI->eventQ, edsize);
	ASSERT(packet);

	/* Critical Area */
	memcpy (packet, edata, edsize);
	packet->wdid = wdid;

	VMQueuePutBuffer (pWDI->eventQ, packet);
}

static void fillInfo (struct PFWDInfo *pWDI, int argc, char *argv[], EPfWDMode eMode, int timeout)
{
	char params[MAX_PARAM_LENGTH] = "";
	int len = 0;
	int i;

	pWDI->info.pid = getpid();
	strncpy (pWDI->info.name, argv[0], sizeof(pWDI->info.name)-1);
	if (argv[1]) {
		for (i=1; i<argc && len<MAX_PARAM_LENGTH; i++) {
			strcat (params, argv[i]);
			strcat (params, " ");
			len += strlen(argv[i]) + 1;
		}
	} else {
		params[0] = '\0';
	}
	strncpy (pWDI->info.params, params, sizeof(pWDI->info.params)-1);
	pWDI->info.eMode = eMode;
	pWDI->info.timeout = timeout;
}


int PFWatchdogRegister (int argc, char *argv[], EPfWDMode eMode, int timeout)
{
	struct PFWDInfo *pWDI = &wdi;
	int rv = -1;

	if (pWDI->info.pid != 0) {
		ERR("Already WD Registered - %d:%s\n", pWDI->info.pid, pWDI->info.name);
	} else {
		pWDI->eventQ = VMQueueOpen (PFWD_QUEUE_NAME, VMQ_MODE_PRODUCER);
		ASSERT (pWDI->eventQ);

		fillInfo (pWDI, argc, argv, eMode, timeout);
		doSendWDPacket (pWDI, EPFWD_CMD_REGISTER, &pWDI->info, sizeof(pWDI->info));

		rv = 0;
	}
	return rv;
}

int PFWatchdogUnregister (void)
{
	struct PFWDInfo *pWDI = &wdi;
	int rv = -1;
	
	if (pWDI->eventQ) {
		doSendWDPacket (pWDI, EPFWD_CMD_UNREGISTER, &pWDI->info, sizeof(struct PFWDUnregister));
		usleep(200000);
		rv = 0;
	} else {
		ERR("WD is not registered.\n");
	}

	return rv;
}

int PFWatchdogStart (void)
{
	struct PFWDInfo *pWDI = &wdi;
	int rv = -1;
	
	if (pWDI->eventQ) {
		doSendWDPacket (pWDI, EPFWD_CMD_START, &pWDI->info, sizeof(struct PFWDUnregister));
		rv = 0;
	} else {
		ERR("WD is not registered.\n");
	}

	return rv;
}

int PFWatchdogStop (void)
{
	struct PFWDInfo *pWDI = &wdi;
	int rv = -1;
	
	if (pWDI->eventQ) {
		doSendWDPacket (pWDI, EPFWD_CMD_STOP, &pWDI->info, sizeof(struct PFWDUnregister));
		rv = 0;
	} else {
		ERR("WD is not registered.\n");
	}

	return rv;
}

int PFWatchdogAlive (void)
{
	struct PFWDInfo *pWDI = &wdi;
	int rv = -1;
	
	if (pWDI->eventQ) {
		doSendWDPacket (pWDI, EPFWD_CMD_ALIVE, &pWDI->info, sizeof(struct PFWDUnregister));
		rv = 0;
	} else {
		ERR("WD is not registered.\n");
	}

	return rv;
}

#if 0
int PFWatchdog_subRegister (uint32_t *subId, int timeout)
{
	struct PFWDInfo *pWDI = &wdi;
	struct PFWDSubRegister *packet;
	int rv = -1;
	
	*subId = 0;
	if (pWDI->eventQ) {
		packet = VMQueueGetBuffer (pWDI->eventQ, sizeof(*packet));
		ASSERT(packet);

		/* Critical Session */
		memcpy (packet, &pWDI->info, sizeof(struct PFWatchdog));
		packet->wdid = EPFWD_CMD_SUBREGISTER;
		packet->subId = *subId = (getpid()<<16)|(pWDI->subCount++ & 0xFFFF);
		packet->timeout = timeout;

		VMQueuePutBuffer (pWDI->eventQ, packet);
		rv = 0;
	} else {
		ERR ("WD is not register.\n");
	}

	return rv;
}

int PFWatchdog_subUnregister (uint32_t subId)
{
	struct PFWDInfo *pWDI = &wdi;
	struct PFWDSubUnregister *packet;
	int rv = -1;
	
	if (pWDI->eventQ) {
		packet = VMQueueGetBuffer (pWDI->eventQ, sizeof(*packet));
		ASSERT(packet);
 
		/* Critical Area */
		memcpy (packet, &pWDI->info, sizeof(struct PFWatchdog));
		packet->wdid = EPFWD_CMD_SUBUNREGISTER;
		packet->subId = subId;

		VMQueuePutBuffer (pWDI->eventQ, packet);
		rv = 0;
	} else {
		ERR ("WD is not register.\n");
	}

	return rv;
}

int PFWatchdog_subAlive (uint32_t subId)
{
	struct PFWDInfo *pWDI = &wdi;
	struct PFWDSubAlive *packet;
	int rv = -1;
	
	if (pWDI->eventQ) {
		packet = VMQueueGetBuffer (pWDI->eventQ, sizeof(*packet));
		ASSERT(packet);
 
		/* Critical Area */
		memcpy (packet, &pWDI->info, sizeof(struct PFWatchdog));
		packet->wdid = EPFWD_CMD_SUBALIVE;
		packet->subId = subId;

		VMQueuePutBuffer (pWDI->eventQ, packet);
		rv = 0;
	} else {
		ERR ("WD is not register.\n");
	}

	return rv;
}
#endif

/**********************************************************************/
struct VMWdtHandler {
	struct timespec		wdt_ts;
	uint32_t			subId;
};

struct VMWdtHandler *PFWatchdogSubRegister (int timeout)
{
	struct VMWdtHandler *handle = NULL;
	struct PFWDInfo *pWDI = &wdi;
	struct PFWDSubRegister *packet;

	if (pWDI->eventQ) {
		handle = calloc (1, sizeof(*handle));
		ASSERT (handle);

		clock_gettime(CLOCK_MONOTONIC, &handle->wdt_ts);

		packet = VMQueueGetBuffer (pWDI->eventQ, sizeof(*packet));
		ASSERT(packet);

		/* Critical Session */
		memcpy (packet, &pWDI->info, sizeof(struct PFWatchdog));
		packet->wdid = EPFWD_CMD_SUBREGISTER;
		packet->subId = handle->subId = (getpid()<<16)|(pWDI->subCount++ & 0xFFFF);
		packet->timeout = timeout;

		VMQueuePutBuffer (pWDI->eventQ, packet);
	} else {
		ERR ("WD is not register.\n");
	}

	return handle;
}

int PFWatchdogSubUnregister (struct VMWdtHandler *handle)
{
	struct PFWDInfo *pWDI = &wdi;
	struct PFWDSubUnregister *packet;
	int rv = -1;
	
	if (pWDI->eventQ) {
		if (((handle->subId >> 16) & 0xffff) == (getpid() & 0xffff))
		{
			packet = VMQueueGetBuffer (pWDI->eventQ, sizeof(*packet));
			ASSERT(packet);

			/* Critical Area */
			memcpy (packet, &pWDI->info, sizeof(struct PFWatchdog));
			packet->wdid = EPFWD_CMD_SUBUNREGISTER;
			packet->subId = handle->subId;
			memset (handle, 0, sizeof(*handle));

			VMQueuePutBuffer (pWDI->eventQ, packet);
			free (handle);
			rv = 0;
		} else {
			ERR ("WD, wrong handle.\n");
		}
	} else {
		ERR ("WD is not register.\n");
	}

	return rv;
}

int PFWatchdogSubAlive (struct VMWdtHandler *handle, int period)
{
	struct PFWDInfo *pWDI = &wdi;
	struct PFWDSubAlive *packet;
	struct timespec	ts;
	int rv = -1;
	
	if (pWDI->eventQ) {
		if (((handle->subId >> 16) & 0xffff) == (getpid() & 0xffff))
		{
			clock_gettime (CLOCK_MONOTONIC, &ts);
			if ( (ts.tv_sec - handle->wdt_ts.tv_sec) > period)
			{
				packet = VMQueueGetBuffer (pWDI->eventQ, sizeof(*packet));
				ASSERT(packet);

				/* Critical Area */
				memcpy (packet, &pWDI->info, sizeof(struct PFWatchdog));
				packet->wdid = EPFWD_CMD_SUBALIVE;
				packet->subId = handle->subId;
				handle->wdt_ts = ts;

				VMQueuePutBuffer (pWDI->eventQ, packet);
				rv = 1;
			} else {
				rv = 0;
			}
		} else {
			ERR ("WD, wrong handle.\n");
		}
	} else {
		ERR ("WD is not register.\n");
	}

	return rv;
}
