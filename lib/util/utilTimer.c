/*

   This code is referenced from linux-2.4.37/kernel/timer.c

 */


#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "list.h"
#include "pfdefine.h"
#include "pfuthread.h"
#include "pfdebug.h"

#include "pfutimer.h"

/*****************************************************************************/

// #define DEBUG_TIMER_TEST	
#define TIMER_TICK_MSEC		(50)
static pthread_mutex_t		mutex ;
static pthread_cond_t		signal ;
static pthread_t			thid ;
static int fgRun = 1 ;

struct TimerList {
	struct list_head entry ;
	uint32_t key ;
	int fgRemove ;
	struct list_head list ;
	unsigned long expires ;
	int msec ;
	ETFReturn (*timerFunc)(void *) ;
	void *param ;
	int fgFree ;
} ;

static void reuse_timer (struct TimerList *timer, ETFReturn etfrv) ;

/*****************************************************************************/
/*
 * Event timer code
 */
#define TVN_BITS 6
#define TVR_BITS 8
#define TVN_SIZE (1 << TVN_BITS)
#define TVR_SIZE (1 << TVR_BITS)
#define TVN_MASK (TVN_SIZE - 1)
#define TVR_MASK (TVR_SIZE - 1)

struct timer_vec {
	int index;
	struct list_head vec[TVN_SIZE];
};

struct timer_vec_root {
	int index;
	struct list_head vec[TVR_SIZE];
};

static struct timer_vec tv5;
static struct timer_vec tv4;
static struct timer_vec tv3;
static struct timer_vec tv2;
static struct timer_vec_root tv1;
static LIST_HEAD(timerListsRoot) ;
static uint32_t timerListKey = 0x05092014 ;

static struct timer_vec * const tvecs[] = {
	(struct timer_vec *)&tv1, &tv2, &tv3, &tv4, &tv5
};

static struct list_head * run_timer_list_running;

#define NOOF_TVECS (sizeof(tvecs) / sizeof(tvecs[0]))

#define timer_enter(t)		do { } while (0)
#define timer_exit()		do { } while (0)

static void init_timervecs (void)
{
	int i;

	for (i = 0; i < TVN_SIZE; i++) {
		INIT_LIST_HEAD(tv5.vec + i);
		INIT_LIST_HEAD(tv4.vec + i);
		INIT_LIST_HEAD(tv3.vec + i);
		INIT_LIST_HEAD(tv2.vec + i);
	}
	for (i = 0; i < TVR_SIZE; i++)
		INIT_LIST_HEAD(tv1.vec + i);
}

static unsigned long timer_jiffies = 0 ;
static unsigned long jiffies = 0 ;

static inline void internal_add_timer(struct TimerList *timer)
{
	/*
	 * must be cli-ed when calling this
	 */
	unsigned long expires = timer->expires;
	unsigned long idx = expires - timer_jiffies;
	struct list_head * vec;

	if (run_timer_list_running)
		vec = run_timer_list_running;
	else if (idx < TVR_SIZE) {
		int i = expires & TVR_MASK;
		vec = tv1.vec + i;
	} else if (idx < 1 << (TVR_BITS + TVN_BITS)) {
		int i = (expires >> TVR_BITS) & TVN_MASK;
		vec = tv2.vec + i;
	} else if (idx < 1 << (TVR_BITS + 2 * TVN_BITS)) {
		int i = (expires >> (TVR_BITS + TVN_BITS)) & TVN_MASK;
		vec =  tv3.vec + i;
	} else if (idx < 1 << (TVR_BITS + 3 * TVN_BITS)) {
		int i = (expires >> (TVR_BITS + 2 * TVN_BITS)) & TVN_MASK;
		vec = tv4.vec + i;
	} else if ((signed long) idx < 0) {
		/* can happen if you add a timer with expires == jiffies,
		 * or you set a timer to go off in the past
		 */
		vec = tv1.vec + tv1.index;
	} else if (idx <= 0xffffffffUL) {
		int i = (expires >> (TVR_BITS + 3 * TVN_BITS)) & TVN_MASK;
		vec = tv5.vec + i;
	} else {
		/* Can only get here on architectures with 64-bit jiffies */
		INIT_LIST_HEAD(&timer->list);
		return;
	}
	/*
	 * Timers are FIFO!
	 */
	list_add(&timer->list, vec->prev);
}

static inline int timer_pending (const struct TimerList * timer)
{
	return timer->list.next != NULL;
}
static inline int detach_timer (struct TimerList *timer)
{
	if (!timer_pending(timer))
		return 0;
	list_del(&timer->list);
	return 1;
}
static inline void cascade_timers(struct timer_vec *tv)
{
	/* cascade all the timers from tv up one level */
	struct list_head *head, *curr, *next;

	head = tv->vec + tv->index;
	curr = head->next;
	/*
	 * We are removing _all_ timers from the list, so we don't  have to
	 * detach them individually, just clear the list afterwards.
	 */
	while (curr != head) {
		struct TimerList *tmp;

		tmp = list_entry(curr, struct TimerList, list);
		next = curr->next;
		list_del(curr); // not needed
		internal_add_timer(tmp);
		curr = next;
	}
	INIT_LIST_HEAD(head);
	tv->index = (tv->index + 1) & TVN_MASK;
}
static inline void run_timer_list(void)
{
	LOCK_MUTEX(mutex) ;
	while ((long)(jiffies - timer_jiffies) >= 0) {
		LIST_HEAD(queued);
		struct list_head *head, *curr;
		if (!tv1.index) {
			int n = 1;
			do {
				cascade_timers(tvecs[n]);
			} while (tvecs[n]->index == 1 && ++n < NOOF_TVECS);
		}
		run_timer_list_running = &queued;
repeat:
		head = tv1.vec + tv1.index;
		curr = head->next;
		if (curr != head) {
			struct TimerList *timer;
			ETFReturn (*fn)(void *);
			void *param;

			timer = list_entry(curr, struct TimerList, list);
 			fn = timer->timerFunc ;
 			param= timer->param;

			detach_timer(timer);
			timer->list.next = timer->list.prev = NULL;
			timer_enter(timer);
			UNLOCK_MUTEX(mutex) ;
#if 0
			fn(param); // Call a user defined function.
#else
			reuse_timer(timer, (timer->fgRemove ? ETFUNC_RV_DONE : fn(param))) ;
#endif
			LOCK_MUTEX(mutex) ;
			timer_exit();
			goto repeat;
		}
		run_timer_list_running = NULL;
		++timer_jiffies; 
		tv1.index = (tv1.index + 1) & TVR_MASK;

		curr = queued.next;
		while (curr != &queued) {
			struct TimerList *timer;

			timer = list_entry(curr, struct TimerList, list);
			curr = curr->next;
			internal_add_timer(timer);
		}			
	}
	UNLOCK_MUTEX(mutex) ;
}

static void timer_process(void)
{
	jiffies ++ ;
	run_timer_list() ;
}
/*****************************************************************************/

static unsigned long inline calculateExpireTime (uint32_t msec)
{
	return (jiffies + ((msec/TIMER_TICK_MSEC) ?: 1)) ;
}

static void add_timer (struct TimerList *timer)
{
	LOCK_MUTEX(mutex) ;
	if (timer_pending(timer)) {
		goto bug ;
	}
	internal_add_timer(timer) ;
	UNLOCK_MUTEX(mutex) ;
	return ;
bug :
	UNLOCK_MUTEX(mutex) ;
	ASSERT( ! "Timer Bug !!\n") ;
}
#if 0
static int mod_timer(struct TimerList *timer, unsigned long expires)
{
	int rv ;
	LOCK_MUTEX(mutex) ;
	timer->expires = expires ;
	rv = detach_timer(timer) ;
	internal_add_timer(timer) ;
	UNLOCK_MUTEX(mutex) ;
	return rv ;
}
static int del_timer(struct TimerList *timer)
{
	int rv ;
	LOCK_MUTEX(mutex) ;
	rv = detach_timer(timer) ;
	timer->list.next = timer->list.prev = NULL ;
	UNLOCK_MUTEX(mutex) ;
	return rv ;
}
#endif
static void reuse_timer (struct TimerList *timer, ETFReturn etfrv)
{
	LOCK_MUTEX(mutex) ;
	if (etfrv == ETFUNC_RV_REPEAT) {
		timer->expires = calculateExpireTime(timer->msec) ;
		UNLOCK_MUTEX(mutex) ;
		add_timer (timer) ;
	} else {
		if (timer->fgFree && timer->param) {
			free (timer->param) ;
		}
		if ( ! timer->fgRemove) {
			list_del(&timer->entry) ;
		}
		UNLOCK_MUTEX(mutex) ;
		free (timer) ;
	}
}

/*****************************************************************************/

static void *timerThread (void *param)
{
	struct timespec ts;

	while( fgRun )
	{
		SET_SIGNAL_TIME_MSEC(ts, TIMER_TICK_MSEC) ;
		LOCK_MUTEX(mutex) ;
		WAIT_SIGNAL_TIMEOUT(signal, mutex, ts) ;
		UNLOCK_MUTEX(mutex) ;

		timer_process() ;
	}
	
	return NULL ;
}

/*****************************************************************************/

#ifdef DEBUG_TIMER_TEST
ETFReturn testTimer(void *param)
{
	static int count = 0;
	struct timeval tv ;
	gettimeofday (&tv, NULL) ;
	DBG("%u.%06d - %d\n", (uint32_t)tv.tv_sec, (int32_t)tv.tv_usec, count) ;

	if (count++ >= 500) {
		return ETFUNC_RV_DONE ;
	}
	return ETFUNC_RV_REPEAT ;
}
void testTimerAdder(int msec)
{
	struct TimerList *tl ;

	tl = (struct TimerList *) calloc (1, sizeof(*tl)) ; 
	ASSERT(tl) ;
	tl->expires = calculateExpireTime (msec) ;
	tl->msec = msec ;
	tl->param = NULL ;
	tl->timerFunc= testTimer ;
	tl->fgFree = 1 ;

	testTimer(NULL) ;
	add_timer (tl) ;
}
#endif /* DEBUG_TIMER_TEST */

static TimerKey inline getTimerKey (void)
{
	TimerKey key ;
	key = timerListKey ++ ;
	if (timerListKey == 0) {
		timerListKey ++ ;
	}
	return key ;
}

static struct TimerList *allocTimerItem (int msec, ETFReturn (*func)(void *), void *param, int free)
{
	struct TimerList *tl;

	tl = (struct TimerList *) calloc (1, sizeof(*tl)) ;
	ASSERT(tl) ;
	
	INIT_LIST_HEAD(&tl->entry) ;	
	tl->expires = calculateExpireTime(msec) ;
	tl->msec = msec ;
	tl->timerFunc = func ;
	tl->param = param ;
	tl->fgFree = free ;

	return tl ;
}

/*****************************************************************************/

TimerKey PFU_timerAdd (int msec, ETFReturn (*func)(void *), void *param, int free)
{
	struct TimerList *tl = allocTimerItem(msec, func, param, free) ;
	TimerKey key ;

	LOCK_MUTEX(mutex) ;
	key = tl->key = getTimerKey() ;
	list_add(&tl->entry, &timerListsRoot) ;
	UNLOCK_MUTEX(mutex) ;
	add_timer(tl) ;

	return key ;
}

/* MUST be Called with LOCK */
static struct TimerList *timerGetTimerList (TimerKey key)
{
	struct TimerList *tl = NULL ;
	struct list_head *plist ;
	int fgFound = 0 ;

	if ( ! list_empty(&timerListsRoot)) {
		list_for_each (plist, &timerListsRoot) {
			tl = list_entry(plist, struct TimerList, entry) ;
//			DBG("msec = %d, key = %X\n", tl->msec, tl->key) ;
			if (tl->key == key) {
				fgFound = 1 ;
				break;
			}
		}
		if ( ! fgFound) {
			tl = NULL ;
		}
	}
	return tl ;
}

void PFU_timerDelete (TimerKey key)
{
	struct TimerList *tl = NULL ;

	if (key == 0) {
		return ;
	}

	if ( ! list_empty(&timerListsRoot)) {
		LOCK_MUTEX(mutex) ;
		tl = timerGetTimerList(key) ;
		if (tl) {
			tl->fgRemove = 1 ;
		}
		UNLOCK_MUTEX(mutex) ;
	}
}
void PFU_timerSetMsec (TimerKey key, int msec)
{
	struct TimerList *tl = NULL ;

	if (key == 0) {
		return ;
	}

	if ( ! list_empty(&timerListsRoot)) {
		LOCK_MUTEX(mutex) ;
		tl = timerGetTimerList(key) ;
		if (tl) {
			tl->msec = msec ;
		}
		UNLOCK_MUTEX(mutex) ;
	}
}

void PFU_timerInit(void)
{
	INIT_SIGNAL(signal) ;
	INIT_MUTEX(mutex) ;
	init_timervecs() ;
	CREATE_THREAD(thid, timerThread, 0x1000, NULL, 1) ;
}

void PFU_timerExit(void)
{
	fgRun = 0 ;
	SIGNAL_MUTEX(signal, mutex) ;
}

