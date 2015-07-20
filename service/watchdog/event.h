#ifndef	__EVENT_H__
#define	__EVENT_H__

/*****************************************************************************/

void eventInit(void);
void eventExit(void);
int  eventGetFd(void);
void eventHandler(void);

#endif	/*__CONFIG_EVENT_H__*/
