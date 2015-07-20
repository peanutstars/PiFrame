#ifndef __PFU_DEBUG_H__
#define __PFU_DEBUG_H__

#ifdef  __cplusplus
extern "C" {
#endif  /*__cplusplus*/

const char *PFU_getStringERusult (EResult rv) ;
void PFU_hexDump (const char *desc, const void *addr, const int len) ;

#ifdef  __cplusplus
} ;
#endif  /*__cplusplus*/

#endif /* __PFU_DEBUG_H__ */
