#ifndef __PFU_SIGNAL_H__
#define __PFU_SIGNAL_H__

#ifdef  __cplusplus
extern "C" {
#endif  /*__cplusplus*/

void PFU_registerSignal (int signum, void (*sigfunc)(int)) ;

#ifdef  __cplusplus
} ;
#endif  /*__cplusplus*/

#endif /* __PFU_SIGNAL_H__ */
