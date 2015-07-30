#ifndef __TIMER_H__
#define __TIMER_H__

#include <list.h>

struct TimerList {
	struct list_head list ;
	unsigned long expires ;
	void *data ;
	void (*function)(void *) ;
} ;

void timerInit(void) ;
void timerExit(void) ;

#endif /* __TIMER_H__ */
