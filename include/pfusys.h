#ifndef __PFU_SYS_H__
#define __PFU_SYS_H__

#include "pflimit.h"


#ifdef  __cplusplus
extern "C" {
#endif  /*__cplusplus*/

struct PFTypeFSCapacity ;

void PFU_mkdir (const char *dirPath) ;
void PFU_rmdir (const char *dirPath) ;
EpfFileSystem PFU_getFileSystemType (const char *strFsType) ;
const char *PFU_getFileSystemName (EpfFileSystem efs) ;
int PFU_getMountPoint(const char *devname, char *mntpt, int *fsType, int *fsRO) ;
int PFU_isMounted (const char *mntpt) ;
int PFU_getMountDevice (const char *mntpt, char *devName, int devNameSize) ;
int PFU_doUmount (const char *mountPoint) ;
int PFU_doMountDisk (const char *devname, const char *mountPoint, int ro, int executable) ;
int PFU_doMountODD (const char *devname, const char *mountPoint) ;
int PFU_doMountCIFS (const char *pathCIFS, const char *mountPoint) ;
int PFU_doMountNFS (const char *url, const char *mountPoint) ;
int PFU_doRemount (int fsType, const char *devname, const char *mountPoint, int ro) ;
int PFU_getDiskCapacity (const char *mountPoint, struct PFTypeFSCapacity *fsCap) ;
int PFU_getDiskPartCount (const char *devSCSI) ;
int PFU_isValidProcess (pid_t pid, const char *programName) ;

#ifdef  __cplusplus
} ;
#endif  /*__cplusplus*/

#endif /* __PFU_SYS_H__ */
