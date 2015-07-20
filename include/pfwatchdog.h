
#ifndef __PFWATCHDOG_H__
#define __PFWATCHDOG_H__

#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include "pftype.h"
#include "pflimit.h"

/*****************************************************************************
 WatchDog ID
 +--------+--------+----------------+
 |  (1)   |  (2)   |      (3)       |
 +--------+--------+----------------+

 (1) : type - 8 bits
 (2) : reserved - 12 bits
 (3) : size - 12 bits

*****************************************************************************/

#define PFWD_QUEUE_NAME				"q.watchdog"
#define PFWD_DEFAULT_TIMEOUT		(30)
#define PFWD_DEFAULT_ALIVE_TIMEOUT	(5)
#define PFWD_TYPE(id)				(((id)>>24) & 0xff)
#define PFWD_PAYLOAD_SIZE(id)		((id) & 0xfff)
#define PFWD_INIT(wd, id)		\
	do {						\
		(wd)->wdid = id;		\
		(wd)->pid = getpid();	\
	} while (0)

#define __MKWDID(wdt,size)		((wdt)<<24|(size&0xfff))
#define MKWDID(wdt,type)		__MKWDID(wdt,sizeof(type))

#define __PFWD_STRUCT__			uint32_t wdid; pid_t pid; char name[MAX_NAME_LENGTH];

enum etype_vmwd {
	PFWD_TYPE_REGISTER			= 0x63,
	PFWD_TYPE_UNREGISTER,
	PFWD_TYPE_START,
	PFWD_TYPE_STOP,
	PFWD_TYPE_ALIVE,
	PFWD_TYPE_SUBREGISTER,
	PFWD_TYPE_SUBUNREGISTER,
	PFWD_TYPE_SUBALIVE,
	PFWD_TYPE_END
};
typedef enum emode_vmwd {
	PFWD_MODE_NONE				= 0,
	PFWD_MODE_RESTART,
	PFWD_MODE_REBOOT,
	PFWD_MODE_END
} emode_vmwd_t;

struct PFWatchdog {
	__PFWD_STRUCT__;
};

struct PFWDRegister {
	__PFWD_STRUCT__;
	char		params[MAX_PARAM_LENGTH];
	uint32_t	eMode;							/* emode_vmmd */
	uint32_t	timeout;
};

struct PFWDUnregister {
	__PFWD_STRUCT__;
};

struct PFWDStart {
	__PFWD_STRUCT__;
};

struct PFWDStop {
	__PFWD_STRUCT__;
};

struct PFWDAlive {
	__PFWD_STRUCT__;
};

struct PFWDSubRegister {
	__PFWD_STRUCT__;
	uint32_t	subId;
	uint32_t	timeout;
};

struct PFWDSubUnregister {
	__PFWD_STRUCT__;
	uint32_t	subId;
};

struct PFWDSubAlive {
	__PFWD_STRUCT__;
	uint32_t	subId;
};

#define PFWD_REGISTER		MKWDID(PFWD_TYPE_REGISTER,		struct PFWDRegister)
#define PFWD_UNREGISTER		MKWDID(PFWD_TYPE_UNREGISTER,	struct PFWDUnregister)
#define PFWD_START			MKWDID(PFWD_TYPE_START,			struct PFWDStart)
#define PFWD_STOP			MKWDID(PFWD_TYPE_STOP,			struct PFWDStop)
#define PFWD_ALIVE			MKWDID(PFWD_TYPE_ALIVE,			struct PFWDAlive)
#define PFWD_SUB_REGISTER	MKWDID(PFWD_TYPE_SUBREGISTER,	struct PFWDSubRegister)
#define PFWD_SUB_UNREGISTER	MKWDID(PFWD_TYPE_SUBUNREGISTER,	struct PFWDSubUnregister)
#define PFWD_SUB_ALIVE		MKWDID(PFWD_TYPE_SUBALIVE,		struct PFWDSubAlive)

#endif /* __PFWATCHDOG_H__ */
