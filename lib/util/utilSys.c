
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/vfs.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "pftype.h"
#include "pflimit.h"
#include "pfdefine.h"
#include "pfdebug.h"

#include "pfufile.h"
#include "pfusys.h"
#include "pfunet.h"
/*****************************************************************************/

void PFU_mkdir (const char *dirPath)
{
	if ( access(dirPath, F_OK) ) {
		char cmd[255];
		snprintf (cmd, sizeof(cmd), "mkdir -p %s", dirPath);
		system (cmd);
	}
}
void PFU_rmdir (const char *dirPath)
{
	if ( ! access(dirPath, F_OK) ) {
		if (rmdir(dirPath) < 0) {
			ERR("rmdir(%s) failed, %d,%s !\n", dirPath, errno, strerror(errno));
		}
	}
}

static const struct {
		const char	*name;
		EpfFileSystem id;
	} _fslists[EPF_FS_END] = {
		{	"vfat",		EPF_FS_FAT		},	/* EPF_FS_FAT 	*/
		{	"exfat",	EPF_FS_EXFAT	},  /* EPF_FS_EXFAT	*/
		{	"ntfs",		EPF_FS_NTFS		},  /* EPF_FS_NTFS	*/
		{	"ext4",		EPF_FS_EXT4		},  /* EPF_FS_EXT4	*/
		{	"ext3",		EPF_FS_EXT3		},  /* EPF_FS_EXT3	*/
		{	"ext2",		EPF_FS_EXT2		},  /* EPF_FS_EXT2	*/
		{	"iso9660",	EPF_FS_ISO9660	},  /* EPF_FS_ISO9660*/
		{	"udf",		EPF_FS_UDF		},  /* EPF_FS_UDF	*/
		{	"fuseblk",	EPF_FS_FUSEBLK	},  /* EPF_FS_FUSEBLK*/
		{	"nfs",		EPF_FS_NFS		}	/* EPF_FS_NFS	*/
	};
EpfFileSystem PFU_getFileSystemType (const char *strFsType)
{
	int i;
	EpfFileSystem efs = EPF_FS_UNKNOWN;

	for (i=0; i<EPF_FS_END; i++) {
		if ( ! strcasecmp(_fslists[i].name, strFsType)) {
			efs = _fslists[i].id;
			break;
		}
	}
	return efs;
}
const char *PFU_getFileSystemName (EpfFileSystem efs)
{
	if (efs >= EPF_FS_END) {
		return NULL;
	}
	return _fslists[efs].name;
}


static int getFsTypeOfFuseblk (const char *devname, const char *mntpt)
{
	FILE *fp;
	char buf[256];
	int fsType = EPF_FS_UNKNOWN;
	
	fp = popen ("ps xa", "r");
	if (fp) {
		while ( fgets(buf, sizeof(buf), fp) ) {
			if ( strstr(buf, devname) && strstr(buf, mntpt) ) {
				if ( strstr(buf, "ntfs-3g") ) {
					fsType = EPF_FS_NTFS;
					break;
				}
			}
		}
		pclose (fp);
	}

	return fsType;
}

int PFU_getMountPoint(const char *devname, char *mntpt, int *fsType, int *fsRO)
{
	int len;
	char *e; 
	char *s;
	FILE *fp;
	int mounted = 0;
	char dev[64];
	char linebuf[1024];

	fp = fopen("/proc/mounts", "r");
	ASSERT(fp);

	len = sprintf(dev, "%s", devname);
	while(fgets(linebuf, sizeof(linebuf), fp) != NULL) {
		if(strncmp(linebuf, dev, len) == 0 && linebuf[len] == ' ') {
			mounted = 1;
			e = strchr(&linebuf[len + 1], ' ');
			ASSERT(e);
			*e = 0;
			if(mntpt) {
				strcpy(mntpt, &linebuf[len + 1]);
			}   
			s = ++e;
			e = strchr(e, ' ');
			ASSERT(e);
			*e = 0;
			if (mntpt && fsType) {
				*fsType = (int) PFU_getFileSystemType (s);
				if (*fsType == EPF_FS_FUSEBLK) {
					*fsType = getFsTypeOfFuseblk (devname, mntpt);
				}
			}
			s = ++e;
			if (fsRO) {
				if (strstr(s, "rw,")) {
					*fsRO = 0;
				} else {
					*fsRO = 1;
				}
			}
			break;
		}   
	}   
	fclose(fp);
	return mounted;
}

int PFU_isMounted (const char *mntpt)
{
	FILE *fp;
	int mounted = 0;
	char mountPoint[MAX_BASE_PATH_LENGTH];
	char linebuf[1024];
	int len;

	len = mntpt ? strlen (mntpt) : 0;
	if (len == 0) {
		return mounted;
	}

	/* trim */
	while (len > 1 && mntpt[len-1] == '/') {
		len--;
	}
	memset (mountPoint, 0, sizeof(mountPoint));
//	snprintf(mountPoint, sizeof(mountPoint), "%s ", mntpt);
	strncpy (mountPoint, mntpt, len);
	strcat (mountPoint, " ");

	fp = fopen("/proc/mounts", "r");
	if (fp) {
		while (fgets(linebuf, sizeof(linebuf), fp) != NULL) {
			if (strstr(linebuf, mountPoint)) {
				mounted = 1;
				break;
			}   
		}   
		fclose(fp);
	}
	return mounted;
}

int PFU_getMountDevice (const char *mntpt, char *devName, int devNameSize)
{
	FILE *fp;
	char linebuf[1024];
	char mountPoint[MAX_PATH_LENGTH];
	int isMounted = 0;
	int len;

	if (devName) {
		memset(devName, 0, devNameSize);
	}

	len = mntpt ? strlen(mntpt) : 0;
	if (len == 0) {
		return isMounted;
	}
	snprintf (mountPoint, sizeof(mountPoint), "%s ", mntpt);

	fp = fopen("/proc/mounts", "r");
	if (fp) {
		while (fgets(linebuf, sizeof(linebuf), fp) != NULL) {
			if ( strstr(linebuf, mountPoint)) {
				char *p = strchr(linebuf, ' ');
				if (p) {
					*p = '\0';
					snprintf (devName, devNameSize, "%s", linebuf);
					isMounted = 1;
				}
				break;
			}
		}
		fclose (fp);
	}

	return isMounted;
}

int PFU_doUmount (const char *mountPoint)
{
	int rv = -1;
	if (umount (mountPoint) < 0) {
		if (errno == EBUSY) {
			rv = -2;
			ERR ("umount(%s) failed, EBUSY !!\n", mountPoint);
		} else {
			ERR ("umount failed(errno=%d), %s.\n", errno, mountPoint);
		}
	} else {
		PFU_rmdir(mountPoint);
		rv = 0;
	}

	return rv;
}

static int mount_fat (int id, const char *devname, const char *mountPoint, int ro, int remount, int executable) 
{
	unsigned long flags;
	char mountData[255];
	int rc;

	flags = MS_NODEV | MS_NOSUID | MS_DIRSYNC;
	flags |= (executable ? 0 : MS_NOEXEC);
	flags |= (ro ? MS_RDONLY : 0); 
	flags |= (remount ? MS_REMOUNT : 0); 

//	sprintf (mountData, "utf8,uid=%d,gid=%d,fmask=%o,dmask=%o,shortname=mixed",
	sprintf (mountData, "utf8,shortname=mixed");

	rc = mount(devname, mountPoint, "vfat", flags, mountData);

	if (rc && errno == EROFS) {
		ERR("%s appears to be a read only filesystem - retrying mount RO", devname);
		flags |= MS_RDONLY;
		rc = mount(devname, mountPoint, "vfat", flags, mountData);
	}   

#if 0
	if (rc == 0 && createLost) {
		char *lost_path;
		asprintf(&lost_path, "%s/LOST.DIR", mountPoint);
		if (access(lost_path, F_OK)) {
			/*  
			 * Create a LOST.DIR in the root so we have somewhere to put
			 * lost cluster chains (fsck_msdos doesn't currently do this)
			 */
			if (mkdir(lost_path, 0755)) {
				SLOGE("Unable to create LOST.DIR (%s)", strerror(errno));
			}
		}
		free(lost_path);
	}
#endif

	if (rc == 0) {
		rc = id;
	}
	return rc;
}
static int mount_ntfs (int id, const char *devname, const char *mountPoint, int ro, int remount, int executable) 
{
#if 0
	unsigned long flags;
	char mountData[255];
	int rc = -1;

	flags = MS_NODEV | MS_NOSUID | MS_DIRSYNC;
	flags |= (executable ? 0 : MS_NOEXEC);
	flags |= (ro ? MS_RDONLY : 0); 
	flags |= (remount ? MS_REMOUNT : 0); 

//	sprintf (mountData, "utf8,uid=%d,gid=%d,fmask=%o,dmask=%o,shortname=mixed",
	sprintf (mountData, "utf8");

	rc = mount(devname, mountPoint, "ntfs-3g", flags, mountData);

	if (rc && errno == EROFS) {
		ERR("%s appears to be a read only filesystem - retrying mount RO", devname);
		flags |= MS_RDONLY;
		rc = mount(devname, mountPoint, "ntfs-3g", flags, mountData);
	}   

	if (rc == 0) {
		rc = id;
	}
	return rc;
#else
	char cmd[1024];
	char mountData[255];
	int rv;

	DBG("NTFS\n");
//	sprintf (mountData, "utf8,uid=%d,gid=%d,fmask=%o,dmask=%o,shortname=mixed",
	sprintf (mountData, "utf8");
	if (executable) {
		strcat(mountData, ",exec");
	} else {
		strcat(mountData, ",noexec");
	}

	if (remount) {
		snprintf (mountData, sizeof(mountData), "remount,%s", ro?"ro":"rw");
		snprintf (cmd, sizeof(cmd), "mount -t ntfs-3g -o%s %s", mountData, mountPoint);
		rv = system (cmd);
		DBG("%s - rv:%d\n", cmd, WEXITSTATUS(rv));
		if (WEXITSTATUS(rv) == 0) {
			rv = id;
		} else {
			rv = -2;
		}
	} else {
		snprintf (cmd, sizeof(cmd), "mount -t ntfs-3g -o%s %s %s", mountData, devname, mountPoint);
		rv = system (cmd);
		DBG("%s - rv:%d\n", cmd, WEXITSTATUS(rv));
		if (WEXITSTATUS(rv) == 0) {
			if (ro) {
				snprintf (cmd, sizeof(cmd), "mount -t ntfs-3g -oremount,ro %s", mountPoint);
				rv = system (cmd);
				DBG("%s - rv:%d\n", cmd, WEXITSTATUS(rv));
				if (WEXITSTATUS(rv) == 0) {
					rv = id;
				} else {
					rv = -3;
				}
			} else {
				rv = id;
			}
		} else {
			rv = -4;
		}
	}

	return rv;
#endif
}
static int mount_ext (int id, const char *devname, const char *mountPoint, int ro, int remount, int executable) 
{
	const char *fsname[EPF_FS_END] = {	"",			/* EPF_FS_FAT 	*/
										"",         /* EPF_FS_EXFAT	*/
										"",         /* EPF_FS_NTFS	*/
										"ext4",     /* EPF_FS_EXT4	*/
										"ext3",     /* EPF_FS_EXT3	*/
										"ext2",     /* EPF_FS_EXT2	*/
										"",         /* EPF_FS_ISO9660*/
										"",         /* EPF_FS_UDF	*/
										"",			/* EPF_FS_FUSEBLK*/
										""	};      /* EPF_FS_NFS	*/
	unsigned long flags;
	int rc = -1; 

	if (id < EPF_FS_EXT4 || id >= EPF_FS_ISO9660) {
		return rc;
	}

	flags = MS_NOATIME | MS_NODEV | MS_NOSUID | MS_DIRSYNC;

	flags |= (executable ? 0 : MS_NOEXEC);
	flags |= (ro ? MS_RDONLY : 0); 
	flags |= (remount ? MS_REMOUNT : 0); 

	rc = mount(devname, mountPoint, fsname[id], flags, NULL);

	if (rc && errno == EROFS) {
		ERR("%s appears to be a read only filesystem - retrying mount RO", devname);
		flags |= MS_RDONLY;
		rc = mount(devname, mountPoint, "ext4", flags, NULL);
	}   

	if (rc == 0) {
		rc = id;
	}
	return rc; 

}
static int (*mountFunc[EPF_FS_END]) (int , const char *, const char *, int, int, int) = {
	mount_fat,	/* EPF_FS_FAT 	*/
	NULL,		/* EPF_FS_EXFAT	*/
	mount_ntfs, /* EPF_FS_NTFS	*/
	mount_ext,  /* EPF_FS_EXT4	*/
	mount_ext,  /* EPF_FS_EXT3	*/
	mount_ext,  /* EPF_FS_EXT2	*/
	NULL,		/* EPF_FS_ISO9660*/
	NULL,		/* EPF_FS_UDF	*/
	NULL,       /* EPF_FS_FUSEBLK*/
	NULL        /* EPF_FS_NFS	*/
};

int PFU_doMountDisk (const char *devname, const char *mountPoint, int ro, int executable)
{
	int i;
	int rv;

	PFU_mkdir (mountPoint);
	for (i=EPF_FS_FAT; i<EPF_FS_END; i++) {
		if (mountFunc[i]) {
			rv = mountFunc[i] (i, devname, mountPoint, ro, 0, executable);
			if (rv >= 0) {
				DBG("Mounted(%d) : %s, %s - ro:%d, exec:%d\n", 
						rv, devname, mountPoint, ro, executable);
				break;
			}
		}
	}
	if (rv < 0) {
		PFU_rmdir (mountPoint);
	}

	return rv;
}

int PFU_doMountODD (const char *devname, const char *mountPoint)
{
	unsigned long flags;
	char mountData[255];
	int rc;

	flags = MS_NODEV | MS_NOEXEC | MS_NOSUID | MS_DIRSYNC | MS_RDONLY;

	strcpy (mountData, "iocharset=utf8");

	do
	{
		rc = mount(devname, mountPoint, "udf", flags, mountData);
		if (rc == 0) {
			rc = EPF_FS_UDF;
			break;
		}

		rc = mount(devname, mountPoint, "iso9660", flags, mountData);
		if (rc == 0) {
			rc = EPF_FS_ISO9660;
			break;
		}
		rc = EPF_FS_UNKNOWN;
	} while (0);
	return rc;
}

int PFU_doMountCIFS (const char *pathCIFS, const char *mountPoint)
{
	unsigned long flags;
	char mountData[255];
	int rc;

	flags = MS_NODEV | MS_NOEXEC | MS_NOSUID | MS_DIRSYNC | MS_RDONLY;

	strcpy (mountData, "iocharset=utf8,username=,password=");
	rc = mount(pathCIFS, mountPoint, "cifs", flags, mountData);
	if (rc == 0) {
		DBG("mount cifs %s to %s.\n", pathCIFS, mountPoint);
	} else {
		ERR2("Failed to mount %s to %s\n", pathCIFS, mountPoint);
	}
	return rc;
}

static int getIpFromUrl (char *strBuf, int bufSize, const char *url)
{
	int ip[4];
	int rv = -1;

	if ( 4 == sscanf(url, "%d.%d.%d.%d", ip, ip+1, ip+2, ip+3) ) {
		rv = snprintf (strBuf, bufSize, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
		DBG("nfs ip=%s\n", strBuf);
	}
	return rv;
}

static int mountNfs (const char *strIp, const char *url, const char *mountPoint)
{
	unsigned long flags;
	char mountData[128];
	char *nfsUrl = strchr(url, ':');
	int rc = -1;

	if (nfsUrl) {
		flags = 0;	//MS_NODEV | MS_NOEXEC | MS_NOSUID | MS_DIRSYNC | MS_RDONLY;
		snprintf (mountData, sizeof(mountData), "nolock,addr=%s", strIp);
		DBG("nfs options=%s\n", mountData);
		rc = mount(url, mountPoint, "nfs", flags, mountData);
		if (rc == 0) {
			DBG("mount nfs %s into %s.\n", url, mountPoint);
		} else {
			rc = -errno;
			ERR2("Failed to mount %s into %s\n", url, mountPoint);
		}
	}
	return rc;
}

int PFU_doMountNFS (const char *url, const char *mountPoint)
{
	char strIp[24];
	int rc;

	do
	{
		if ((rc = getIpFromUrl (strIp, sizeof(strIp), url)) <= 0) {
			ERR("URL Format is wrong, %s.\n", url);
			break;
		}

		/* this code save time, check to alive a remote nfs server */
		rc = PFU_openTCP (strIp, PF_DEF_SERVICE_PORT_NFS, 1, 5);
		if (rc < 0) {
			ERR("Remote NFS Daemon is not available.\n");
			break;
		}
		close (rc);

		/* TODO : check already to be mounted. */

		rc = mountNfs (strIp, url, mountPoint);

	} while (0);

	return rc;
}

int PFU_doRemount (int fsType, const char *devname, const char *mountPoint, int ro)
{
	if (fsType < 0 || fsType >= EPF_FS_END) {
		return -2;
	}
	if (mountFunc[fsType]) {
		return mountFunc[fsType] (fsType, devname, mountPoint, ro, 1, 0);
	} else {
		return -3;
	}
}

int PFU_getDiskCapacity (const char *mountPoint, struct PFTypeFSCapacity *fsCap)
{
	struct statfs s;
	int rv = -1;

	memset (fsCap, 0, sizeof(*fsCap));

	do
	{
		fsblkcnt_t blocks_used;

		if (access(mountPoint, F_OK)) {
			// ERR("It is not access to mount point.\n");
			break;
		}
		rv = statfs (mountPoint, &s);
		if (rv < 0) {
			ERR("statfs(%s) error - %d, %s !!\n", mountPoint, errno, strerror(errno));
			break;
		}

		blocks_used = s.f_blocks - s.f_bfree;
		if (blocks_used + s.f_bavail) {
			fsblkcnt_t blocks_tmp = blocks_used + s.f_bavail;
			fsCap->usedPercent = (blocks_used * 100.0 + (blocks_tmp>>1)) / blocks_tmp;
		}
		s.f_bsize >>= 10;
		fsCap->total = s.f_blocks * s.f_bsize;
		fsCap->used  = (s.f_blocks - s.f_bfree) * s.f_bsize;
		fsCap->empty = s.f_bavail * s.f_bsize;
	}
	while (0);

	return rv;
}

int PFU_getDiskPartCount (const char *devSCSI)
{
	char sfile[MAX_BASE_PATH_LENGTH];
	int i;
	int count;

	for (i=0,count=0; i<MAX_ATA_PART_COUNT; i++) {
		snprintf (sfile, sizeof(sfile), "/sys/block/%s/%s%d", devSCSI, devSCSI, i+1);
		if ( ! access(sfile, F_OK)) {
			count++;
			continue;
		}
		break;
	}

	return count;
}

int PFU_isValidProcess (pid_t pid, const char *programName)
{
	char cmdlinePath[MAX_BASE_PATH_LENGTH];
	char tBuf[MAX_PATH_LENGTH];
	int rv = 0;
	int fd;

	snprintf (cmdlinePath, sizeof(cmdlinePath), "/proc/%d/cmdline", (int)pid);

	if ( access(cmdlinePath, F_OK)) {
		/* Not exist */
	} else {
		fd = open (cmdlinePath, O_RDONLY);
		if (fd >= 0) {
			if (PFU_safeRead (fd, tBuf, sizeof(tBuf)) > 0) {
				if ( ! strcmp(tBuf, programName)) {
					rv = 1;
				}
			}
			close (fd);
		}
	}
	return rv;
}
