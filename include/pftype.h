#ifndef	__PF_TYPE_H__
#define	__PF_TYPE_H__

#ifdef	__cplusplus
extern "C" {
#endif	/*__cplusplus*/

#include <time.h>
#include "pflimit.h"

#ifdef  __GNUC__
#define __PACKED__					__attribute__((packed))
#define	__TRANSPARENT_UNION__		__attribute__((__transparent_union__)) 
#else
#error	"Can't use this header in non-GNUC compliant compilers."
#endif  /*__GNUC__*/

/*****************************************************************************
 Common Used Data Structure
*****************************************************************************/

struct PFTypeUdevStorage {
	char		id_bus[MAX_DUMMY_LENGTH] ;
	char		id_model[MAX_NAME_LENGTH] ;
	char		devtype[MAX_NAME_LENGTH] ;
	char		devpath[MAX_PATH_LENGTH] ;
	char		action[MAX_DUMMY_LENGTH] ;
	char		devname[MAX_DEVICE_LENGTH] ;
	uint16_t	major ;
	uint16_t	minor ;
};

struct PFTypeUdevNetwork {
	char		id_bus[MAX_DUMMY_LENGTH] ;
	char		id_vendor[MAX_DUMMY_LENGTH] ;
	char		id_serial_short[MAX_DUMMY_LENGTH] ;
	char		netif[MAX_DUMMY_LENGTH] ;
	char		devpath[MAX_PATH_LENGTH] ;
	char		action[MAX_DUMMY_LENGTH] ;
};

struct PFTypeFSCapacity {
	uint32_t    total;          /* kbytes */
	uint32_t    used;           /* kbytes */
	uint32_t    empty;          /* kbytes */
	float       usedPercent;    /* 0.0% */
};
struct PFTypePartInfo {
	char    devname[MAX_DEVICE_LENGTH];
	char    mountPoint[MAX_BASE_PATH_LENGTH];
	int32_t  fsType;
	uint32_t fsRO;
	struct PFTypeFSCapacity fsCap;
};



#ifdef	__cplusplus
};
#endif	/*__cplusplus*/

#endif	/*__PF_TYPE_H__*/
