
#ifndef	__VMSHM_DEV_H__
#define	__VMSHM_DEV_H__

#include <linux/ioctl.h>

#define	VMSHM_MAX_NAME_LENGTH	64

struct VMShmCreate {
	const char *name;
	int size;
	int perm;
	int flags;
};

#define	VMSHM_CREATE		_IOW('s', 1, struct VMShmCreate)
#define	VMSHM_REMOVE		_IOW('s', 2, const char *)
#define	VMSHM_SELECT		_IOW('s', 3, const char *)
#define	VMSHM_UNSELECT		_IO('s', 4)

#define	VMSHM_FLAG_PHYSICAL	0x01

#ifdef	__KERNEL__

struct vm_area_struct;
struct VMSharedMemory;

extern struct VMSharedMemory *vmshm_create(
		const char *name, int size, int perm, int flags, int *err);
extern int vmshm_remove(struct VMSharedMemory *mem);
extern struct VMSharedMemory *vmshm_select(const char *name);
extern void vmshm_unselect(struct VMSharedMemory *mem);
extern int vmshm_remap(struct VMSharedMemory *mem, struct vm_area_struct *vma);
extern int vmshm_get_name(struct VMSharedMemory *mem, char *buffer, int len);

#endif	/*__KERNEL__*/

#endif	/*__VMSHM_DEV_H__*/
