#define LOG_NDEBUG 1
#define LOG_TAG "SemaphoreMutex"
#include "cutils/log.h"

#include <cutils/log.h>
#include <cutils/properties.h>
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include "SemaphoreMutex.h"


/** Initializes the semaphore at a given value
 *
 * @param tsem the semaphore to initialize
 * @param val the initial value of the semaphore
 *
 */
int ipc_sem_init(ipc_sem_t* tsem, unsigned int val)
{
	int i;

	i = pthread_cond_init(&tsem->condition, NULL);
	if (i!=0)
		return -1;

	i = pthread_mutex_init(&tsem->mutex, NULL);
	if (i!=0)
		return -1;

	tsem->semval = val;

	return 0;
}

/** Destroy the semaphore
 *
 * @param tsem the semaphore to destroy
 */
void ipc_sem_deinit(ipc_sem_t* tsem)
{
	pthread_cond_destroy(&tsem->condition);
	pthread_mutex_destroy(&tsem->mutex);
}

/** Decreases the value of the semaphore. Blocks if the semaphore
 * value is zero.
 *
 * @param tsem the semaphore to decrease
 */
void ipc_sem_down(ipc_sem_t* tsem)
{
	pthread_mutex_lock(&tsem->mutex);

	ALOGV("semdown:%p val:%d",tsem,tsem->semval);
	while (tsem->semval == 0)
	{
		ALOGV("semdown wait:%p val:%d",tsem,tsem->semval);
		pthread_cond_wait(&tsem->condition, &tsem->mutex);
		ALOGV("semdown wait end:%p val:%d",tsem,tsem->semval);
	}

	tsem->semval--;
	pthread_mutex_unlock(&tsem->mutex);
}

/** Increases the value of the semaphore
 *
 * @param tsem the semaphore to increase
 */
void ipc_sem_up(ipc_sem_t* tsem)
{
	pthread_mutex_lock(&tsem->mutex);

	ALOGV("semup signal:%p val:%d",tsem,tsem->semval);
	if(tsem->semval == 0) {
		pthread_cond_signal(&tsem->condition);
	}
	tsem->semval++;
	ALOGV("semup signal end:%p val:%d",tsem,tsem->semval);

	pthread_mutex_unlock(&tsem->mutex);
}

/** Reset the value of the semaphore
 *
 * @param tsem the semaphore to reset
 */
void ipc_sem_reset(ipc_sem_t* tsem, unsigned int val)
{
	pthread_mutex_lock(&tsem->mutex);

	if(tsem->semval == 0) {
		pthread_cond_signal(&tsem->condition);
	}
	tsem->semval = val;

	pthread_mutex_unlock(&tsem->mutex);
}

/** Wait on the condition.
 *
 * @param tsem the semaphore to wait
 */
void ipc_sem_wait(ipc_sem_t* tsem)
{
	pthread_mutex_lock(&tsem->mutex);

	pthread_cond_wait(&tsem->condition, &tsem->mutex);

	pthread_mutex_unlock(&tsem->mutex);
}

void ipc_sem_timedwait(ipc_sem_t* tsem,long sec,long nsec)
{
	pthread_mutex_lock(&tsem->mutex);
    struct timespec sync_timer;
    sync_timer.tv_sec = time(NULL) + sec;
    sync_timer.tv_nsec = nsec;    
	pthread_cond_timedwait(&tsem->condition, &tsem->mutex,&sync_timer);
	pthread_mutex_unlock(&tsem->mutex);
}

/** Signal the condition,if waiting
 *
 * @param tsem the semaphore to signal
 */
void ipc_sem_signal(ipc_sem_t* tsem)
{
	pthread_mutex_lock(&tsem->mutex);

	pthread_cond_signal(&tsem->condition);

	pthread_mutex_unlock(&tsem->mutex);
}



