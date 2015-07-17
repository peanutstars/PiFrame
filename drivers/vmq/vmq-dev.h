
#ifndef	__VMQ_DEV_H__
#define	__VMQ_DEV_H__

#include <linux/ioctl.h>
#include "../vmshm/vmshm-dev.h"

#define	VMQ_MAX_NAME_LENGTH	VMSHM_MAX_NAME_LENGTH

struct VMQCreateParam {
	const char *name;	/* 생성할 큐의 이름 */
	int size;			/* 큐의 크기 */
	int page_size;		/* 최소 할당 단위 */
	int perm;			/* unused */
	int flags;			/* unused */
	int maxItemCount;	/* 최대 아이템 개수(0일경우 무제한) */
};

struct VMQSetupParam {
	const char *name;	/* 사용할 큐 */
#define	VMQ_TYPE_PRODUCER	0
#define	VMQ_TYPE_CONSUMER	1
	int type;		/* VMQ_TYPE_* */
};

struct VMQBufferInfo {
	int size;		/* I/O */
	int offset;		/* Out */
};

struct VMQItemInfo {
	int size;
	int offset;
};

#define	VMQ_CREATE		_IOW('q', 1, struct VMQCreateParam)
#define	VMQ_REMOVE		_IOW('q', 2, const char *)

#define	VMQ_SETUP		_IOW('q', 11, struct VMQSetupParam)

#define	VMQ_GETBUF		_IOWR('q', 21, struct VMQBufferInfo)
#define	VMQ_PUTBUF		_IOWR('q', 22, struct VMQBufferInfo)

#define	VMQ_GETITEM		_IOWR('q', 31, struct VMQItemInfo)
#define	VMQ_PUTITEM		_IOWR('q', 32, struct VMQItemInfo)

#define VMQ_GETKEY		_IOW('q', 41, unsigned int *)

#endif	/*__VMQ_DEV_H__*/
