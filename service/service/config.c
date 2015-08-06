#include <string.h>

#include "pfdefine.h"
#include "pfconfig.h"
#include "pfdebug.h"

/*****************************************************************************/

//#define ___USE_RUNTIME_SERVICE___

struct PFConfigSystem				configSystem;
#ifdef ___USE_RUNTIME_SERVICE___
struct PFRuntimeService				configRuntimeService ;
#endif

/*****************************************************************************/

static void updateConfigSystem (const struct PFConfig *config, int bootup)
{
	int diff = memcmp(&configSystem, &config->system, sizeof(configSystem)) ;

	if(bootup || diff) {
		memcpy(&configSystem, &config->system, sizeof(configSystem));
	}   
}

#ifdef ___USE_RUNTIME_SERVICE___
static void updateConfigRuntimeService (const struct PFConfig *config, int bootup)
{
	int diff = memcmp(&configRuntimeService, &config->runtime.service, sizeof(configRuntimeService)) ;
	
	if (diff || diff) {
		memcpy(&configRuntimeService, &config->runtime.service, sizeof(configRuntimeService)) ;
	}
}
#endif

/*****************************************************************************/

void configUpdate(int eConfigType)
{
	const struct PFConfig *config;

	config = PFConfigGet();
	ASSERT(config) ;

	switch(eConfigType)
	{
	case EPF_CONFIG_SYSTEM :
		updateConfigSystem (config, 0) ;
		break ;
#ifdef ___USE_RUNTIME_SERVICE___
	case EPF_CONFIG_RUNTIME_SERVICE :
		updateConfigRuntimeService(config, 0) ;
		break;
#endif
	}

	PFConfigPut(config);
}

void configInit(void)
{
	const struct PFConfig *config ;

	config = PFConfigGet() ;
	ASSERT(config) ;

#if 0
	while ( ! config->initialized ) {
		PFConfigPut(config) ;
		usleep(300000) ; /* 300 ms */
		config = PFConfigGet() ;
	}
#endif

	if ( config->initialized ) {
		/* update config datum */
		updateConfigSystem (config, 1) ; 
#ifdef ___USE_RUNTIME_SERVICE___
		updateConfigRuntimeService(config, 1) ;
#endif
	} else {
		ERR("Config is not initialized !!\n") ;
	}

	PFConfigPut(config) ;
}

void configExit(void)
{
}

