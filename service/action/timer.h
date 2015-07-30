#ifndef __PFU_TIMER_H__
#define __PFU_TIMER_H__

#include <list.h>

enum ETIMER_FUNC_RETURN {
	ETFUNC_RV_DONE = 0 ,
	ETFUNC_RV_REPEAT,
	ETFUNC_RV_END
} ;
typedef enum ETIMER_FUNC_RETURN ETFReturn ;
typedef uint32_t TimerKey ;

TimerKey PFU_timerAdd (int msec, ETFReturn (*func)(void *), void *param, int free) ;
void PFU_timerDelete (TimerKey key) ;
void PFU_timerInit(void) ;
void PFU_timerExit(void) ;

#endif /* __PFU_TIMER_H__ */
