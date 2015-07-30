#ifndef __TIMER_H__
#define __TIMER_H__

#include <list.h>

enum ETIMER_FUNC_RETURN {
	ETFUNC_RV_DONE = 0 ,
	ETFUNC_RV_REPEAT,
	ETFUNC_RV_END
} ;
typedef enum ETIMER_FUNC_RETURN ETFuncReturn ;

void timerInit(void) ;
void timerExit(void) ;

#endif /* __TIMER_H__ */
