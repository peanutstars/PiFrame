#ifndef	__PF_EVENT_H__
#define	__PF_EVENT_H__

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
	ERV_FAIL_UNKNOWN,
	ERV_FAIL_BUSY,
	ERV_FAIL_NODEV,
	ERV_FAIL_PARAM,
	ERV_FAIL_TIMEOUT,
	ERV_RESULT_END
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
#define	PFE_INIT_EVENT(event, eid)	\
	do { 							\
		(event)->id = (eid);		\
		(event)->sec = time(NULL);	\
	} while(0)
#define PFE_QUERY_ID_START			(100)

/*****************************************************************************/

#define	__MKEID(mod, id, size, flags)	(((mod) << 24)|((id) << 16)|((size&0xfff) << 4)|(flags&0xf))

#define	MKEID(mod, id, type)			__MKEID((mod), (id), sizeof(type), 0)
#define	MKEIDF(mod, id, type, flag)		__MKEID((mod), (id), sizeof(type), flag) 
//#define	MKEID_R(mod, id, type)			__MKEID((mod), (id), sizeof(type), PFE_FLAG_REPORT)
//#define	MKEID_N(mod, id)				__MKEID((mod), (id), 0, 0)
//#define	MKEID_NR(mod, id)				__MKEID((mod), (id), 0, PFE_FLAG_REPORT)

#define	__PFE_STRUCT__					uint32_t id; int sec; uint32_t key


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
};
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
/******************************************************************************
 * Service Config
 *****************************************************************************/
struct PFEConfigUpdate {
	__PFE_STRUCT__;
	int classId;
};

#define PFE_CONFIG_UPDATE			MKEID(EPF_MOD_CONFIG, 0, struct PFEConfigUpdate)

struct PFEConfigReplyNormal {
	__PFE_STRUCT__;
	int32_t	result;
};
struct PFEConfigRequestExport {
	__PFE_STRUCT__;
	uint32_t	mask;
	char path[MAX_PATH_LENGTH];
};

#define PFE_CONFIG_REPLY_NORMAL		MKEIDF(EPF_MOD_CONFIG, 100, struct PFEConfigReplyNormal,   PFE_FLAG_REPLY)
#define PFE_CONFIG_REQUEST_EXPORT	MKEIDF(EPF_MOD_CONFIG, 101, struct PFEConfigRequestExport, PFE_FLAG_NONE)

/******************************************************************************
 * other events
 *****************************************************************************/

#endif	/*__PF_EVENT_H__*/
