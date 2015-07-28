
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "notify.h"
#include "pfevent.h"
#include "pfdefine.h"
#include "pfdebug.h"

#include "method.h"

static void TestDummy (int argc, char **argv)
{
	struct PFETestInt notify;

	notify.data = strtoul(argv[0], NULL, 10) & 1;

	doNotifyStruct (PFE_TEST_INT, &notify, sizeof(notify));
}

static void SystemDummy (int argc, char **argv) 
{
	doNotifyBasic (PFE_SYS_DUMMY);
}

static void SystemPowerOff (int argc, char **argv)
{
	doNotifyBasic (PFE_SYS_POWER_OFF);
}

static void SystemShutdown (int argc, char **argv)
{
	struct PFESystemShutdown notify;
	int fgErr = 1;

	memset (&notify, 0, sizeof(notify));

	do
	{
		if ( ! strcasecmp(argv[0], "reboot")) {
			notify.type = ESYS_SHUTDOWN_REBOOT;
		} else if ( ! strcasecmp(argv[0], "exit")) {
			notify.type = ESYS_SHUTDOWN_EXIT;
		} else {
			break;
		}

		if (argv[2]) {
			notify.waitSecound = strtoul (argv[2], NULL, 10);
			if (notify.waitSecound > 120) {
				DBG("Changed waitSecound from %d to 120\n", notify.waitSecound);
				notify.waitSecound = 120;
			}
		} else {
			notify.waitSecound = 3;
		}

		doNotifyStruct (PFE_SYS_SHUTDOWN, &notify, sizeof(notify));
		fgErr = 0;
	} while (0);

	if (fgErr) {
		ERR("Parameters is wrong. !!\n");
	}
}

struct PFMethod notifySystem[] = {
	{	"TEST_INT",			1,	"<num> : just test for demo",	TestDummy,			PFMT_NONE	},
	{	"SYS_DUMMY",		0,	"n/a : make event of dummy",	SystemDummy,		PFMT_NONE	},
	{	"SYS_POWER_OFF",	0,	"n/a : Power off",				SystemPowerOff,		PFMT_NONE	},
	{	"SYS_SHUTDOWN",		1,	"<reboot|exit> [<waitSecound>] : broadcast reboot or exit after waitSecound",
																SystemShutdown,		PFMT_NONE	},
	{	NULL, }
};

