#ifndef __PFQUERY_H__
#define __PFQUERY_H__

#ifdef	__cplusplus
extern "C" {
#endif	/*__cplusplus*/

struct VMQueue ;
struct PFEvent ;

int  PFQueryWaitReplyWithPutBuffer (struct VMQueue *eventQ, struct PFEvent *request, int timeout, void **reply);
int  PFQueryReplyProcess (struct PFEvent *event);
void PFQueryWakeUpAll (void);
const char *PFQueryGetStrEResult(int erv) ;

#ifdef	__cplusplus
};
#endif	/*__cplusplus*/

#endif /* __PFQUERY_H__ */
