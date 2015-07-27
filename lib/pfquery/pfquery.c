
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "vmq.h"
#include "list.h"
#include "pferror.h"
#include "pfevent.h"
#include "pfdebug.h"

/*****************************************************************************/
static pthread_mutex_t		lock = PTHREAD_MUTEX_INITIALIZER;
static LIST_HEAD(waitLRoot);

struct PFQueryItem {
	pthread_cond_t		cond;
	struct list_head	list;
	int key;					/* Unique-key */
	void *pReply;
};
/*****************************************************************************/

static struct PFQueryItem *allocQueryItem (int key)
{
	struct PFQueryItem *item;
	pthread_condattr_t cattr;

	item = (struct PFQueryItem *) calloc (1, sizeof(*item));
	ASSERT(item);

	pthread_condattr_init (&cattr);
	pthread_condattr_setclock (&cattr, CLOCK_MONOTONIC);
	pthread_cond_init (&item->cond, &cattr);

	INIT_LIST_HEAD(&item->list);
	item->key = key;
	
	return item;
}

static void freeQueryItem (struct PFQueryItem *item)
{
	pthread_cond_destroy (&item->cond);
	free (item);
}

static int setReplyQueryItem (struct PFEvent *event)
{
	struct PFQueryItem *item = NULL;
	struct list_head *plist;
	int ret = 0 ;

	if (! list_empty(&waitLRoot)) {	/* Apply DCL */
		pthread_mutex_lock (&lock);
		if ( ! list_empty(&waitLRoot))
		{
			list_for_each (plist, &waitLRoot) {
				item = list_entry (plist, struct PFQueryItem, list);
				if (item->key == event->key)
				{
					int replySize = PFE_PAYLOAD_SIZE(event->id);
	
					item->pReply = (void *) malloc (replySize);
					ASSERT(item->pReply);
	
					memcpy (item->pReply, event, replySize);
					ret = 1 ;
	
					pthread_cond_signal (&item->cond);
					break;
				}
			}
		}
		pthread_mutex_unlock (&lock);
	}

	return ret;
}

/*****************************************************************************/

/**
 * @brief 대개 큐에 key를 등록하고, timeout 동안 대기한다.
 *        이베트를 받으면 결과 값을 reply 포인터로 전달한다.
 * @param key, Notify를 전달한 key 값 (반드시 유일한 값이어야 함)
 * @param timeout, 초단위 대기 시간, 0초(무한대기 - 하루로 고정)
 * @param reply, 요청에 대한 응답 ( XXX : 반드시 free() 시켜야 함 ),
 *        시간 초과시 NULL
 * @return 성공시 0, 실패시 -1 전달
 */
#define DEF_TS_SECOUND			1000000000
#define DEF_TS_STEP_UNIT		 250000000
#define DEF_TS_STEP_COUNT		(DEF_TS_SECOUND/DEF_TS_STEP_UNIT)
#define DEF_ONE_DAY_SECOUND		86400
int PFQueryWaitReplyWithPutBuffer (struct VMQueue *eventQ, struct PFEvent *request, int timeout, void **reply)
{
	uint32_t waitCount = ((! timeout) ? DEF_ONE_DAY_SECOUND : timeout) * DEF_TS_STEP_COUNT;
	struct PFQueryItem *item = allocQueryItem (request->key);
	struct timespec	ts;
	int ret = -1;
	int fgRun = 1;

	pthread_mutex_lock (&lock);
	list_add_tail (&item->list, &waitLRoot);
	pthread_mutex_unlock (&lock);

	VMQueuePutBuffer (eventQ, request);

	while ( fgRun && (--waitCount != 0) )
	{
		clock_gettime (CLOCK_MONOTONIC, &ts);
		ts.tv_nsec     +=  DEF_TS_STEP_UNIT;
		if (ts.tv_nsec  >  DEF_TS_SECOUND) {
			ts.tv_nsec -=  DEF_TS_SECOUND;
			ts.tv_sec  += 1;
		}

		pthread_mutex_lock (&lock);
		pthread_cond_timedwait (&item->cond, &lock, &ts);
		if (item->pReply) {
			fgRun = 0;
		}
		pthread_mutex_unlock (&lock);
	}

	pthread_mutex_lock (&lock);
	list_del (&item->list);
	pthread_mutex_unlock (&lock);

	if (item->pReply) {
		*reply = item->pReply;
		ret = 0;
	}
	else {
		*reply = NULL;
	}
	freeQueryItem (item);

	return ret;
}

/**
 * @brief 이벤트가 발생한 경우, 해당 이벤트 대기 목록에서 찾아, 
 *        이벤트 데이터를 복사하여 전달한다.
 * @param event 발생 이벤트 포인터
 * @return 대기 이벤트가 존재하여 전달한 경우 0, 그 이외의 상황은 -1 전달
 */
int PFQueryReplyProcess (struct PFEvent *event)
{
	int ret = 0 ;

	/* It is not marked to "FLAG REPLY", the caller will be recevied itself instantly. */
	if (PFE_IS_REPLY(event->id) && event->key) {
//	if (event->key) {
		ret = setReplyQueryItem (event);
	}

	return ret;
}

/**
 * @brief 대기하고 있는 Query 를 모두 깨운다.
 *        긴급히 종료 해야 하는 상황에서 사용한다.
 */
void PFQueryWakeUpAll (void)
{
	struct PFQueryItem *item = NULL;
	struct list_head *plist;

	pthread_mutex_lock (&lock);
	if ( ! list_empty (&waitLRoot) ) {
		list_for_each (plist, &waitLRoot) {
			item = list_entry (plist, struct PFQueryItem, list);
			
			pthread_cond_signal (&item->cond);
		}
	}
	pthread_mutex_unlock (&lock);
}

/*****************************************************************************/
