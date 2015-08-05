#ifndef	__PF_EVENT_H__
#define	__PF_EVENT_H__

#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "pfconfig.h"
#include "pfsvc.h"
#include "pftype.h"
#include "pflimit.h"

/*****************************************************************************
 *
 * EVENT ID의 구조
 *
 * +--------+--------+--------+--------+
 * |  (1)   |   (2)  |   (3)  |  (4)   |
 * +--------+--------+--------+--------+
 *
 * (1) : module index[pfsvc.h:EPF_MOD_XXXXX] (8 bit)
 * (2) : event id (8 bit)
 * (3) : payload size (12  bit)
 * (4) : flags (4 bit)
 *
 *****************************************************************************/

typedef enum
{
	ERV_OK					= 0,
	ERV_FAIL,
	ERV_FAIL_BUSY,
	ERV_FAIL_NODEV,
	ERV_FAIL_PARAM,
	ERV_FAIL_TIMEOUT,
	ERV_RESULT_COUNT
} EResult ;

/*****************************************************************************/

#define PFE_FLAG_NONE				(0)
#define	PFE_FLAG_REPORT				(1)
#define PFE_FLAG_REPLY				(2)

/*****************************************************************************/

#define	PFE_QUEUE_NAME				"q.event"
#define PFE_MODULE_ID(id)			(((id) >> 24) & 0xff)
#define PFE_EVENT_ID(id)			(((id) >> 16) & 0xff)
#define PFE_EVENT_FLAG(id)			((id) & 0xf)
#define	PFE_PAYLOAD_SIZE(id)		(((id) >> 4) & 0xfff)
#define	PFE_IS_REPORT(id)			(id & PFE_FLAG_REPORT)
#define PFE_IS_REPLY(id)			(id & PFE_FLAG_REPLY)
#define	PFE_INIT_EVENT(event, eid)				\
	do {										\
		(event)->id = (eid);					\
		(event)->evtsec = time(NULL);			\
		(event)->srcPid = (int32_t) getpid();	\
	} while(0)
#define PFE_QUERY_ID_START			(100)

/*****************************************************************************/

#define	__MKEID(mod, id, size, flags)	(((mod) << 24)|((id) << 16)|((size&0xfff) << 4)|(flags&0xf))

#define	MKEID(mod, id, type)			__MKEID((mod), (id), sizeof(type), 0)
#define	MKEIDF(mod, id, type, flag)		__MKEID((mod), (id), sizeof(type), flag) 

#define	__PFE_STRUCT__					uint32_t id; int evtsec; int32_t srcPid ; uint32_t key
#define	__PFE_STRUCT_QRV__				uint32_t id; int evtsec; int32_t srcPid ; uint32_t key; int32_t result


/*****************************************************************************/

struct PFEvent {
	__PFE_STRUCT__ ;
} ;

/*****************************************************************************/

/******************************************************************************
 * sys.test
 *****************************************************************************/
struct PFETest {
	__PFE_STRUCT__ ;
} ;
struct PFETestChar {
	__PFE_STRUCT__ ;
	unsigned char	data ;
} ;
struct PFETestShort {
	__PFE_STRUCT__ ;
	unsigned short	data ;
} ;
struct PFETestInt {
	__PFE_STRUCT__ ;
	unsigned int	data ;
} ;
struct PFETestLong {
	__PFE_STRUCT__ ;
	unsigned long	data ;
} ;

#define PFE_TEST					MKEID(EPF_MOD_TEST, 0, struct PFETest)
#define PFE_TEST_CHAR				MKEID(EPF_MOD_TEST, 1, struct PFETestChar)
#define PFE_TEST_SHORT				MKEID(EPF_MOD_TEST, 2, struct PFETestShort)
#define PFE_TEST_INT				MKEID(EPF_MOD_TEST, 3, struct PFETestInt)
#define PFE_TEST_LONG				MKEID(EPF_MOD_TEST, 4, struct PFETestLong)
#define PFE_TEST_END				MKEID(EPF_MOD_TEST, 5, struct PFETest)

/******************************************************************************
 * Event Lists
 *****************************************************************************/
struct PFESystem {
    __PFE_STRUCT__ ;
} ;
enum ESYS_SHUTDOWN_TYPE {
	ESYS_SHUTDOWN_REBOOT       = 0x74800814,
	ESYS_SHUTDOWN_EXIT,
} ;
struct PFESystemShutdown {
    __PFE_STRUCT__ ;
    uint32_t	type ;
    uint32_t	waitSecound ;
} ;
struct PFESystemTime {
    __PFE_STRUCT__ ;
	uint32_t	year ;		/* 2015 */
	uint32_t	month ;		/* 1 ~ 12 */
	uint32_t	day ;		/* 1 ~ 31 */
	uint32_t	hour ;		/* 0 ~ 24 */
	uint32_t	min ;		/* 0 ~ 60 */
	uint32_t	sec ;		/* 0 ~ 60 */
} ;
#define PFE_SYS_DUMMY               MKEID(EPF_MOD_SYSTEM, 0, struct PFESystem)
#define PFE_SYS_POWER_OFF           MKEID(EPF_MOD_SYSTEM, 1, struct PFESystem)
#define PFE_SYS_SHUTDOWN            MKEID(EPF_MOD_SYSTEM, 2, struct PFESystemShutdown)
#define PFE_SYS_TIME				MKEID(EPF_MOD_SYSTEM, 3, struct PFESystemTime)

/******************************************************************************
 * Service Config
 *****************************************************************************/
struct PFEConfigUpdate {
	__PFE_STRUCT__ ;
	uint32_t eConfigType ;
} ;

#define PFE_CONFIG_UPDATE			MKEID(EPF_MOD_CONFIG, 0, struct PFEConfigUpdate)

struct PFEConfigReplyNormal {
	__PFE_STRUCT_QRV__ ;
} ;
struct PFEConfigRequestExport {
	__PFE_STRUCT__ ;
	uint32_t	mask ;
	char path[MAX_PATH_LENGTH] ;
} ;

#define PFE_CONFIG_REPLY_NORMAL		MKEIDF(EPF_MOD_CONFIG, 100, struct PFEConfigReplyNormal,   PFE_FLAG_REPLY)
#define PFE_CONFIG_REQUEST_EXPORT	MKEIDF(EPF_MOD_CONFIG, 101, struct PFEConfigRequestExport, PFE_FLAG_NONE)

/******************************************************************************
 * Service Service
 *****************************************************************************/
enum ESVC_CMD_TYPE {
	ESVC_CMD_FOREGROUND = 0,
	ESVC_CMD_BACKGROUND,
	ESVC_CMD_END
} ;
struct PFEServiceSystem {
	__PFE_STRUCT__ ;
	uint32_t		eCmdType ;
	char			command[MAX_COMMAND_LENGTH] ;
	char			resultPath[MAX_BASE_PATH_LENGTH] ;
} ;

#define PFE_SERVICE_SYSTEM			MKEID(EPF_MOD_SERVICE, 0, struct PFEServiceSystem)

struct PFEServiceReply {
	__PFE_STRUCT_QRV__ ;
} ;
struct PFEServiceRequestCommand {
	__PFE_STRUCT__ ;
	uint32_t		eCmdType ;
	char			command[MAX_COMMAND_LENGTH] ;
	char			resultPath[MAX_BASE_PATH_LENGTH] ;
} ;
struct PFEServiceReplyCommand {
	__PFE_STRUCT_QRV__ ;
	int32_t			resultSystem ;
} ;

#define PFE_SERVICE_REPLY			MKEIDF(EPF_MOD_SERVICE, 100, struct PFEServiceReply,			PFE_FLAG_REPLY)
#define PFE_SERVICE_REQUEST_COMMAND	MKEIDF(EPF_MOD_SERVICE, 101, struct PFEServiceRequestCommand,	PFE_FLAG_NONE)
#define PFE_SERVICE_REPLY_COMMAND	MKEIDF(EPF_MOD_SERVICE, 102, struct PFEServiceReplyCommand,		PFE_FLAG_REPLY)

/******************************************************************************
 * other events
 *****************************************************************************/

#endif	/*__PF_EVENT_H__*/
