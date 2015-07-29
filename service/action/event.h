#ifndef	__CONFIG_EVENT_H__
#define	__CONFIG_EVENT_H__

/*****************************************************************************/

void eventInit(void);
void eventExit(void);
int  eventGetFd(void);
int  eventHandler(void);

#endif	/*__CONFIG_EVENT_H__*/
