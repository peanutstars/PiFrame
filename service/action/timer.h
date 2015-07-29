#ifndef __TIMER_H__
#define __TIMER_H__

#include <list.h>

struct timer_list {
	struct list_head list ;
	unsigned long expires ;
	unsigned long data ;
	void (*function)(unsigned long) ;
} ;

void timerInit(void) ;
void timerExit(void) ;

#endif /* __TIMER_H__ */
