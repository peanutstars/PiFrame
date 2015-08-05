
#ifndef	__NOTIFY_H__
#define	__NOTIFY_H__

#ifdef  __cplusplus
extern "C" {
#endif	/*__cplusplus*/

void notifyInit(void) ;
void notifyExit(void) ;
void doNotifyBasic (uint32_t eid) ;
void doNotifyStruct (uint32_t eid, void *edata, int edsize) ;
void *doRequestStruct (uint32_t eid, void *edata, int edsize, int timeoutSec) ;
void doReplyStruct (uint32_t eid, uint32_t key, void *edata, int edsize) ;

#ifdef	__cplusplus
};
#endif	/*__cplusplus*/

#endif	/*__NOTIFY_H__*/
