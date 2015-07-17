#ifndef	__VMSHM_H__
#define	__VMSHM_H__

#include "vmshm-dev.h"

/*****************************************************************************/

#define	VMSHM_FLAG_MASK		VMSHM_FLAG_PHYSICAL

/*****************************************************************************/

#ifdef	__cplusplus
extern "C" {
#endif	/*__cplusplus*/

extern int VMSharedMemoryCreate(int fd, const char *name, int size, int perm, int flags);
extern int VMSharedMemoryRemove(int fd, const char *name);
extern int VMSharedMemorySelect(int fd, const char *name);
extern int VMSharedMemoryUnselect(int fd);

#ifdef	__cplusplus
};
#endif	/*__cplusplus*/

#endif	/*__VMSHM_H__*/
