
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


static void DumpConfigBase (const struct PFConfig *config)
{
	const struct PFBaseInfo *base = &config->base;

	printf ("<<<< config.base >>>>\n");
	printf ("release            : %d\n", base->release);
	printf ("version            : %s\n", base->version);
	printf ("rescue             : %d\n", base->rescue);
	printf ("model              : %s\n", base->model);
	printf ("dummy              : %s\n", base->dummy);
}
static void DumpConfigSystem (const struct PFConfig *config)
{
	const struct PFConfigSystem *system = &config->system;
	int i;

	printf ("<<<< config.system >>>>\n");
	printf ("name               : %s\n", system->name);
	printf ("subModel           : %s\n", system->subModel);
	printf ("timezone           : %s:%s:%s:%s\n", system->tz.gmtOff, system->tz.zone, system->tz.id, system->tz.name);
	printf ("<<<< config.system.ntp >>>>\n");
	printf ("enable             : %d\n", system->ntp.enable);
	printf ("serverCount        : %d\n", system->ntp.serverCount);
	printf ("server             : ");
	for (i=0; i<system->ntp.serverCount; i++) {
		printf ("%s ", system->ntp.server[i]);
	}
	printf ("\n<<<< config.system.info >>>>\n");
	printf ("pathDefConfig      : %s\n", system->info.pathDefConfig);
	printf ("pathConfig         : %s\n", system->info.pathConfig);
	printf ("baseODD            : %s\n", system->info.baseODD);
	printf ("baseATA            : %s\n", system->info.baseATA);
	printf ("baseUSB            : %s\n", system->info.baseUSB);
	printf ("baseNET            : %s\n", system->info.baseATA);
	printf ("prefixUSB          : %s\n", system->info.prefixUSB);
}
static void DumpConfigNetwork (const struct PFConfig *config)
{
	const struct PFConfigNetwork *net = &config->network;
	const char *strBasicNetIf[EPFC_NI_COUNT] = { "none", "wired", "wireless" };
	const char *strBasicType[EPFC_NC_COUNT] = { "dhcp", "static" };

	printf ("<<<< config.network.basic >>>>\n");
	printf ("interface          : %s\n", strBasicNetIf[net->basic.netif]);
	printf ("type               : %s\n", strBasicType[net->basic.type]);
	printf ("devWired           : %s\n", net->basic.devWired);

	printf ("<<<< config.network.staticIp >>>>\n");
	printf ("ipaddr             : %s\n", net->staticIp.ipaddr);
	printf ("netmask            : %s\n", net->staticIp.netmask);
	printf ("gateway            : %s\n", net->staticIp.gateway);
	printf ("dns1               : %s\n", net->staticIp.dns1);
	printf ("dns2               : %s\n", net->staticIp.dns2);
}
static void DumpConfigRuntime (const struct PFConfig *config)
{
	printf ("Nothing ...\n") ;
}
static void ConfigDump (int argc, char **argv)
{
	struct PFConfig _config ;
	const struct PFConfig *pConfig = PFConfigGet() ;
	const struct PFConfig *config = &_config ;
	memcpy (&_config, pConfig, sizeof(_config)) ;
	PFConfigPut(pConfig) ;

	if ( ! strcmp(argv[0], "runtime")) {
		DumpConfigRuntime(config) ;
	} else if ( ! strcmp(argv[0], "system")) {
		DumpConfigSystem(config) ;
	} else if ( ! strcmp(argv[0], "network")) {
		DumpConfigNetwork(config) ;
	} else if ( ! strcmp(argv[0], "all")) {
		DumpConfigBase(config) ;
		DumpConfigSystem(config) ;
		DumpConfigNetwork(config) ;
		DumpConfigRuntime(config) ;
	} else if ( ! strcmp(argv[0], "init")) {
		printf("init=%d\n", config->initialized) ;
	} else {
		ERR("Not support parameter : %s\n", argv[0]) ;
	}
}

static void ConfigRequestExport (int argc, char **argv)
{
	struct PFEConfigRequestExport request ;
	struct PFEConfigReplyNormal *reply ;
	int mask;
	const char *path;
	
	mask = strtoul(argv[0], NULL, 0);
	path = argv[1];

	do
	{
		if ( ! path) {
			ERR("Parameter, path is wrong.\n") ;
			break;
		}
		memset (&request, 0, sizeof(request)) ;
		request.mask = mask ;
		snprintf (request.path, sizeof(request.path), "%s", path) ;

		reply = doRequestStruct (PFE_CONFIG_REQUEST_EXPORT, &request, sizeof(request), 10) ;
		if (reply) {
			DBG("PFE_CONFIG_REQUEST_EXPORT is returned %s(%d)\n", PFQueryGetStrEResult(reply->result), reply->result) ;
			free (reply) ;
		} else {
			ERR("Timeout( PFE_CONFIG_REQUEST_EXPORT )\n") ;
		}
	} 
	while( 0 ) ;
}

struct PFMethod queryConfig[] = {
	{	"dump",		1,	"<all|system|network|runtime|init> : dump config",		ConfigDump,				PFMT_NONE	},
	{	"export",	2,	"<mask> <path> : export a archived file of config",		ConfigRequestExport,	PFMT_REPLY	},
	{	NULL, }
};

