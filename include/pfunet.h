#ifndef __PFU_NET_H__
#define __PFU_NET_H__

#ifdef  __cplusplus
extern "C" {
#endif  /*__cplusplus*/

int PFU_isValidStrIPv4 (const char *strIp) ;
int PFU_openTCP (const char *strIp, int port, int fgNonBlock, int timeout/*sec*/) ;

#ifdef  __cplusplus
} ;
#endif  /*__cplusplus*/

#endif /* __PFU_NET_H__ */
