#include <stdio.h>

#include "pfdefine.h"
#include "pfdebug.h"

#include "enum2str.h"

#define DSTR(x)				#x

static const char const *strConfigType[EPF_CONFIG_COUNT] = {
	DSTR( EPF_CONFIG_SYSTEM  ),
	DSTR( EPF_CONFIG_NETWORK ),
	DSTR( EPF_CONFIG_RUNTIME ),
	DSTR( EPF_CONFIG_RUNTIME_SERVICE )
} ;
const char *getStrConfigType(enum EPF_CONFIG_TYPE eIdx)
{
	return strConfigType[(int)eIdx] ;
}

static const char const *strServiceCommandType[ESVC_CMD_END] = {
	"fg",
	"bg"
} ;
const char *getStrServiceCommandType(enum ESVC_CMD_TYPE eIdx)
{
	return strServiceCommandType[(int)eIdx] ;
}

void __attribute__((constructor)) __testEnumTypes(void)
{
	int enumCount ; /* This variable is only to prevent warning in compiling */

	enumCount = EPF_CONFIG_COUNT ;
	ASSERT(((enumCount == 4) && "Please check enum EPF_CONFIG_TYPE, it is changed.")) ;
	enumCount = ESVC_CMD_END ;
	ASSERT(((enumCount == 2) && "Please check enum ESVC_CMD_TYPE, it is changed.")) ;
}
