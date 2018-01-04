#ifndef SEMAPHORE_MUTEX_H__
#define SEMAPHORE_MUTEX_H__

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef pthread_mutex_t	ipc_mutex_t;

typedef struct ipc_sem_t {
	pthread_cond_t 	condition;
	pthread_mutex_t mutex;
	unsigned int 	semval;
}ipc_sem_t;

int  ipc_sem_init(ipc_sem_t* tsem, unsigned int val);
void ipc_sem_deinit(ipc_sem_t* tsem);
void ipc_sem_down(ipc_sem_t* tsem);
void ipc_sem_up(ipc_sem_t* tsem);
void ipc_sem_reset(ipc_sem_t* tsem, unsigned int val);
void ipc_sem_wait(ipc_sem_t* tsem);
void ipc_sem_timedwait(ipc_sem_t* tsem,long sec,long nsec);
void ipc_sem_signal(ipc_sem_t* tsem);

#define ipc_mutex_init(x)		pthread_mutex_init(x, NULL)
#define ipc_mutex_lock(x)		pthread_mutex_lock(x)
#define ipc_mutex_unlock(x)		pthread_mutex_unlock(x)
#define ipc_mutex_deinit(x)		pthread_mutex_destroy(x)

#ifdef __cplusplus
}
#endif

#endif /* SEMAPHORE_MUTEX_H__ */
