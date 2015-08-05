#include <string.h>

#include "pfdefine.h"
#include "pfconfig.h"
#include "pfdebug.h"

/*****************************************************************************/

struct PFConfigSystem				configSystem;

/*****************************************************************************/

static void updateConfigSystem (const struct PFConfig *config, int bootup)
{
	int diff = memcmp(&configSystem, &config->system, sizeof(configSystem));

	if(bootup || diff) {
		memcpy(&configSystem, &config->system, sizeof(configSystem));
	}   
}

/*****************************************************************************/

void configUpdate(int eConfigType)
{
	const struct PFConfig *config;

	config = PFConfigGet();
	ASSERT(config) ;

	switch(eConfigType)
	{
		case EPF_CONFIG_SYSTEM:
			updateConfigSystem (config, 0);
			break;
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
	} else {
		ERR("Config is not initialized !!\n") ;
	}

	PFConfigPut(config) ;
}

void configExit(void)
{
}

