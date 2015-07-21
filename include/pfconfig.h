#ifndef	__PF_CONFIG_H__
#define	__PF_CONFIG_H__

#include <stdint.h>
#include "pflimit.h"
#include "pftype.h"

#ifdef	__cplusplus
extern "C" {
#endif	/*__cplusplus*/

/*****************************************************************************/

#define	PF_CONFIG_KEY					0x35465170U
#define	PF_CONFIG_NAME					"PFConfig"

/*****************************************************************************/

struct PFBaseInfo {
	int release ;
	int rescue ;
	char version[MAX_VERSION_LENGTH] ;
	char model[MAX_NAME_LENGTH] ;
	char dummy[MAX_DUMMY_LENGTH] ;
} ;

struct PFConfigSystemNTP {
	int enable ;
	int serverCount ;
	char server[MAX_NTP_SERVER_COUNT][MAX_ADDR_LENGTH] ;
} ;

struct PFConfigSystemInfo {
	char pathDefConfig[MAX_BASE_PATH_LENGTH] ;	// /system/config/_MODEL_     : Default Config
	char pathConfig[MAX_BASE_PATH_LENGTH] ;		// /data/config               : It cans modify from the default config
	                                            //                              except the fixed GUI data

	char baseODD[MAX_NAME32_LENGTH] ;		// /mnt/odd
	char baseATA[MAX_NAME32_LENGTH] ;		// /mnt/ata
	char baseUSB[MAX_NAME32_LENGTH] ;		// /mnt/usb - /mnt/usb/mediaXX
	char baseNET[MAX_NAME32_LENGTH] ;		// /mnt/net - samba, nfs, ...
	char prefixUSB[MAX_NAME16_LENGTH] ;		// media -> /mnt/usb/mediaXX
} ;
struct PFConfigSystemTimezone {
	char gmtOff[MAX_NAME8_LENGTH] ;
	char zone[MAX_NAME8_LENGTH] ;
	char id[MAX_TIMEZONE_LENGTH] ;
	char name[MAX_TIMEZONE_LENGTH] ;
} ;
struct PFConfigSystem {
	char							name[MAX_NAME_LENGTH] ;
	char							subModel[MAX_NAME_LENGTH] ;
	struct PFConfigSystemTimezone	tz ;
	struct PFConfigSystemNTP		ntp ;
	struct PFConfigSystemInfo		info ;
} ;

/*****************************************************************************/

enum ENetworkBasicInterface {
	EPFC_NBI_NONE		= 0,
	EPFC_NBI_WIRED,
	EPFC_NBI_WIRELESS,
	EPFC_NBI_END
} ;
enum NetworkBasicType {
	EPFC_NBT_DHCP		= 0,
	EPFC_NBT_STATIC,
	EPFC_NBT_END
} ;

struct PFConfigNetworkBasic {
    unsigned int netif ;
	unsigned int type ;
	char devWired[MAX_DUMMY_LENGTH] ;
} ;

struct PFConfigNetworkStatic {
	char ipaddr[MAX_ADDR_LENGTH] ;
	char netmask[MAX_ADDR_LENGTH] ;
	char gateway[MAX_ADDR_LENGTH] ;
	char dns1[MAX_ADDR_LENGTH] ;
	char dns2[MAX_ADDR_LENGTH] ;
} ;

struct PFConfigNetwork {
	struct PFConfigNetworkBasic	basic;
	struct PFConfigNetworkStatic staticIp;
} ;

/*****************************************************************************/
struct PFConfigRuntime {
	// TODO : Add data for runtime.
	int runtime ;
} ;
/*****************************************************************************/

struct PFConfig {
	int						initialized ;
	struct PFBaseInfo		base ;
	struct PFConfigSystem	system ;
	struct PFConfigNetwork	network ;

	struct PFConfigRuntime	runtime ;
	int dummy[16];
};

/*****************************************************************************/

const struct PFConfig *PFConfigGet(void);
void PFConfigPut(const struct PFConfig*);

#ifdef	__cplusplus
};
#endif	/*__cplusplus*/

/*****************************************************************************/

#endif	/*__PF_CONFIG_H__*/
