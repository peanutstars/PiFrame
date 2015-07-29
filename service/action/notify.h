
#ifndef	__NOTIFY_H__
#define	__NOTIFY_H__

#ifdef  __cplusplus
extern "C" {
#endif	/*__cplusplus*/

struct VMEConfigRequestExport;

void notifyInit(void);
void notifyExit(void);
void notifyConfigUpdate(int klass);
void notifyReplyNormal (uint32_t key, int result);

#ifdef	__cplusplus
};
#endif	/*__cplusplus*/

#endif	/*__NOTIFY_H__*/
