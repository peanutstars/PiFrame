#ifndef __PFU_FILE_H__
#define __PFU_FILE_H__

typedef enum _SafeUtilError {
    SAFE_UTIL_ERROR_NO_SPACE = -1, 
    SAFE_UTIL_ERROR_IO = -2, 
} SafeUtilError;

#ifdef  __cplusplus
extern "C" {
#endif  /*__cplusplus*/

int PFU_safeRead(int fd, void *buf, int size) ;
int PFU_safeWrite(int fd, const void *data, int size) ;
int PFU_safePread(int fd, void *buf, int size, off_t offset) ;
int PFU_safePwrite(int fd, const void *data, int size, off_t offset) ;
off_t PFU_getFileSize(const char *path) ;
void *PFU_readWholeFile (const char *path) ;
int PFU_writeStringToFile (const char *path, const char *msg) ;
int PFU_isDir (const char *path) ;
void PFU_rtrim (char *str) ;
int PFU_setFD (int fd, int flags) ;
int PFU_resetFD (int fd, int flags) ;

#ifdef  __cplusplus
} ;
#endif  /*__cplusplus*/

#endif /* __PFU_FILE_H__ */
