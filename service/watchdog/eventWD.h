#ifndef	__EVENTWD_H__
#define	__EVENTWD_H__

void eventWDInit (void);
void eventWDExit (void);
int  eventWDGetFd (void);
void eventWDHandler (void);

#endif	/*__EVENTWD_H__*/
