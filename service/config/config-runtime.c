#include "pfdefine.h"
#include "pfconfig.h"
#include "pfevent.h"
#include "pfdebug.h"
#include "config.h"
#include "notify.h"

#include "config-util.h"
#include "config-runtime.h"

void updateConfigRuntimeService (const struct PFRuntimeService *service)
{
	struct PFConfig *config = configGet() ;
	int diff = 0 ;
	
	if (memcmp(&config->runtime.service, service, sizeof(*service))) {
		memcpy(&config->runtime.service, service, sizeof(*service)) ;
		diff = 1 ;
	}
	configPut (PF_CONFIG_MASK(EPF_CONFIG_RUNTIME_SERVICE));
	if (diff) {
		notifyConfigUpdate(EPF_CONFIG_RUNTIME_SERVICE) ;
	}
}

