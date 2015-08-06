#include <stdio.h>

#include "pfdefine.h"
#include "pfdebug.h"

#include "enum2str.h"

#define DSTR(x)				#x

static const char const * strConfigType[EPF_CONFIG_COUNT] = {
	DSTR( EPF_CONFIG_SYSTEM  ),
	DSTR( EPF_CONFIG_NETWORK ),
	DSTR( EPF_CONFIG_RUNTIME )
} ;
const char *getStrConfigType(enum EPF_CONFIG_TYPE eIdx)
{
	return strConfigType[(int)eIdx] ;
}


void __attribute__((constructor)) __checkEnumTypes(void)
{
	int enumCount ; /* This variable is only to prevent warning in compiling */

	enumCount = EPF_CONFIG_COUNT ;
	ASSERT(((enumCount == 3) && EPF_CONFIG_COUNT)) ;
	DBG("Checked to convert enum to string.\n") ;
}
