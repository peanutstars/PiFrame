#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "list.h"
#include "pfdefine.h"
#include "pfuthread.h"
#include "pfdebug.h"

#include "timer.h"


/*****************************************************************************/

#define TIMER_TICK_MSEC			50
static pthread_mutex_t		mutex ;
static pthread_cond_t		signal ;
static pthread_t			thid ;
static int fgRun = 1 ;

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

static inline void internal_add_timer(struct timer_list *timer)
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

static inline int timer_pending (const struct timer_list * timer)
{
	return timer->list.next != NULL;
}
static inline int detach_timer (struct timer_list *timer)
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
		struct timer_list *tmp;

		tmp = list_entry(curr, struct timer_list, list);
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
			struct timer_list *timer;
			void (*fn)(unsigned long);
			unsigned long data;

			timer = list_entry(curr, struct timer_list, list);
 			fn = timer->function;
 			data= timer->data;

			detach_timer(timer);
			timer->list.next = timer->list.prev = NULL;
			timer_enter(timer);
			UNLOCK_MUTEX(mutex) ;
			fn(data);
			LOCK_MUTEX(mutex) ;
			timer_exit();
			goto repeat;
		}
		run_timer_list_running = NULL;
		++timer_jiffies; 
		tv1.index = (tv1.index + 1) & TVR_MASK;

		curr = queued.next;
		while (curr != &queued) {
			struct timer_list *timer;

			timer = list_entry(curr, struct timer_list, list);
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

void add_timer (struct timer_list *timer)
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
int mod_timer(struct timer_list *timer, unsigned long expires)
{
	int rv ;
	LOCK_MUTEX(mutex) ;
	timer->expires = expires ;
	rv = detach_timer(timer) ;
	internal_add_timer(timer) ;
	UNLOCK_MUTEX(mutex) ;
	return rv ;
}
int del_timer(struct timer_list *timer)
{
	int rv ;
	LOCK_MUTEX(mutex) ;
	rv = detach_timer(timer) ;
	timer->list.next = timer->list.prev = NULL ;
	UNLOCK_MUTEX(mutex) ;
	return rv ;
}

/*****************************************************************************/

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

static struct timer_list myTimer ;
unsigned long timerCalExpire (uint32_t msec)
{
	unsigned long addTick = msec / TIMER_TICK_MSEC ;
	return (jiffies + (addTick ?: 1)) ;
}

void testTimer(unsigned long param)
{
	struct timeval tv ;
	gettimeofday (&tv, NULL) ;
	DBG("%u.%06d\n", (uint32_t)tv.tv_sec, (int32_t)tv.tv_usec) ;

	if (param) {
		myTimer.expires = timerCalExpire (20) ;
		add_timer (&myTimer) ;
	}
}

void timerInit(void)
{
	INIT_SIGNAL(signal) ;
	INIT_MUTEX(mutex) ;
	init_timervecs() ;
	CREATE_THREAD(thid, timerThread, 0x800, NULL, 1) ;

	memset(&myTimer, 0, sizeof(myTimer)) ;
	myTimer.expires = timerCalExpire (20) ;
	myTimer.data = 1 ;
	myTimer.function = testTimer ;

	testTimer(0) ;
	add_timer (&myTimer) ;
}

void timerExit(void)
{
	fgRun = 0 ;
	SIGNAL_MUTEX(signal, mutex) ;
}

