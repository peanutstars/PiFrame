
#ifndef	__NOTIFY_H__
#define	__NOTIFY_H__

#ifdef  __cplusplus
extern "C" {
#endif	/*__cplusplus*/

void notifyInit(void);
void notifyExit(void);


/******************************************************************************
  notify
 *****************************************************************************/
void doNotifyBasic (uint32_t eid);
void doNotifyStruct (uint32_t eid, void *edata, int edsize);
void *doRequestStruct (uint32_t eid, void *edata, int edsize, int timeoutSec);

/******************************************************************************
  query.config
 *****************************************************************************/
void *doConfigRequestExport (int mask, const char *path);


/******************************************************************************
  query.dzserver
 *****************************************************************************/
struct VMEDzserverRequestPlay;
void *doDzserverRequestPlay (struct VMEDzserverRequestPlay *play);


#ifdef	__cplusplus
};
#endif	/*__cplusplus*/

#endif	/*__NOTIFY_H__*/
