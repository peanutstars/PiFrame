
#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

#include "pflimit.h"
#include "pfufile.h"
#include "pfdebug.h"

/*****************************************************************************/

int PFU_safeRead(int fd, void *buf, int size)
{
	int left, cnt;
	char *ptr = (char *)buf;

	ASSERT(buf);

	left = size;
	do {
		cnt = read(fd, ptr, left);
		if(cnt != left) {
			if(cnt < 0) {
				if(errno == EINTR)
					continue;

				else if(errno == EIO)
					return SAFE_UTIL_ERROR_IO;

				printf ("safeRead, errno=%d:%s\n", errno, strerror(errno));
//				ASSERT(! "I can't handle error\n");
				return -1;
			}
			else if(cnt == 0) {
				return size - left;
			}
		}
		left -= cnt;
		ptr += cnt;
	} while(left > 0);

	return size;
}

int PFU_safeWrite(int fd, const void *data, int size)
{
	int left, cnt;
	const char *ptr = (const char *)data;

	ASSERT(data);

//#define	CHECK_ALIGN
#ifdef	CHECK_ALIGN
	{
		off_t pos;
		pos = lseek(fd, 0, SEEK_CUR);
		if(pos & 0xfff)
			printf("POSITION ALIGN ERROR = %lld\n", (long long)pos);
		if(size & 0xfff)
			printf("SIZE ALIGN ERROR = %lld\n", (long long)size);
	}
#endif	/*CHECK_ALIGN*/
	left = size;
	do {
		cnt = write(fd, ptr, left);
		if(cnt != left) {
			if(cnt < 0) {
				if(errno == ENOSPC) {
					DBG("ENOSPC\n");
					return SAFE_UTIL_ERROR_NO_SPACE;
				}
				else if(errno == EINTR)
					continue;
				else if(errno == EIO)
					return SAFE_UTIL_ERROR_IO;

				perror("error");
				ASSERT(! "I can't handle error\n");
			} 
		}
		left -= cnt;
		ptr += cnt;
	} while(left > 0);
	return size;
}

int PFU_safePread(int fd, void *buf, int size, off_t offset)
{
	int left, cnt;
	char *ptr = (char *)buf;

	ASSERT(buf);

	left = size;
	do {
		cnt = pread(fd, ptr, left, offset);
		if(cnt != left) {
			if(cnt < 0) {
				if(errno == EINTR)
					continue;

				else if(errno == EIO)
					return SAFE_UTIL_ERROR_IO;

				DBG("errno = %d\n", errno);
				ASSERT(! "I can't handle error\n");
			}
		}
		left -= cnt;
		ptr += cnt;
		offset += cnt;
	} while(left > 0);

	return size;
}

int PFU_safePwrite(int fd, const void *data, int size, off_t offset)
{
	int left, cnt;
	const char *ptr = (const char *)data;

	ASSERT(data);

	left = size;
	do {
		cnt = pwrite(fd, ptr, left, offset);
		if(cnt != left) {
			if(cnt < 0) {
				if(errno == ENOSPC) {
					DBG("ENOSPC\n");
					return SAFE_UTIL_ERROR_NO_SPACE;
				}
				else if(errno == EINTR)
					continue;

				else if(errno == EIO)
					return SAFE_UTIL_ERROR_IO;

				perror("ERROR");
				ASSERT(! "I can't handle error\n");
			} 
		}
		left -= cnt;
		ptr += cnt;
		offset += cnt;
	} while(left > 0);
	return size;
}

off_t PFU_getFileSize(const char *path)
{
	struct stat st;

	if(stat(path, &st) < 0) {
		perror(path);
		return -1;
	}

	return st.st_size;
}

void * PFU_readWholeFile (const char *path)
{
	off_t fsize;
	void *content = (void *) NULL;
	int fd;

	fsize = PFU_getFileSize (path);
//	ASSERT(fsize <= MAX_FILE_BUFFER_SIZE);
	if (fsize > MAX_FILE_BUFFER_SIZE) {
		fsize = MAX_FILE_BUFFER_SIZE;
	}
	if (fsize == 0 && ! strncmp(path, "/proc/", 6)) {
		fsize = MAX_FILE_BUFFER_SIZE;
	}
	if (fsize > 0)
	{
		content = (void *) malloc (fsize);
		ASSERT(content);
		
		fd = open (path, O_RDONLY);
		if (fd > 0)
		{
			int rdSize = PFU_safeRead (fd, content, fsize);
			int fgErr = 0;

			if (rdSize < 0) {
				fgErr = 1;
			} else if ( (int)fsize != rdSize ) {
				if ( ! strncmp(path, "/proc/", 6) || ! strncmp(path, "/sys/", 5) ) {
					/* pass */
				} else {
					fgErr = 1;
				}
			}
			if (fgErr) {
				ERR ("Read %s, but file size error. Wanted size=%jd, Readed size=%d\n", path, fsize, rdSize);
				free (content);
				content = (void *) NULL;
			}
			close(fd);
		}
	}

	return content;
}

int PFU_writeStringToFile (const char *path, const char *msg)
{
	FILE *fp = fopen(path, "w");
	int rv = -1;
	if (fp) {
		rv = fprintf(fp, "%s", msg);
		fclose(fp);
	}
	return rv;
}

int PFU_isDir (const char *path)
{
	struct stat st;

	if(stat(path, &st) < 0) {
		perror(path);
		return -1;
	}

	return ((st.st_mode & S_IFMT) == S_IFDIR) ? 1 : 0;
}

void PFU_rtrim (char *str)
{
	char *p;
	int i=0;
	char tc[] = "\r\n";

	while (tc[i] != '\0') {
		p = strchr(str, tc[i]);
		if (p) {
			*p = '\0';
			break;
		}
		i++;
	}
}

int PFU_setFD (int fd, int flags)
{
	int _flags = fcntl (fd, F_GETFD);
	return fcntl (fd, F_SETFD, _flags | flags);
}
int PFU_resetFD (int fd, int flags)
{
	int _flags = fcntl (fd, F_GETFD);
	_flags &= ~flags;
	return fcntl (fd, F_SETFD, _flags);
}

