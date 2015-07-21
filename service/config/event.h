#ifndef	__CONFIG_EVENT_H__
#define	__CONFIG_EVENT_H__

struct VMQueue;

/*****************************************************************************/

void configEventInit(void);
void configEventExit(void);
int  configEventGetFd(void);
int  configEventHandler(void);

#endif	/*__CONFIG_EVENT_H__*/
