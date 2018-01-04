#ifndef __PTHREAD_POOL_H__
#define __PTHREAD_POOL_H__

//#undef PTHREAD_DETACH
#define PTHREAD_DETACH

struct th_pool_job {
	struct th_pool_job *next;
	int id;
	void (*exec_func)(void *arg);
	void (*free_func)(void *arg);
	void *func_arg;
};

#ifndef PTHREAD_DETACH
struct th_work {
	struct th_work *prev, *next;
	pthread_t thread_id;
};
#endif

struct th_pool {
	pthread_mutex_t pool_mutex;
	pthread_cond_t notify_cond;

#ifndef PTHREAD_DETACH
	struct th_work *head_work;
#endif

	struct th_pool_job *head_job, *last_job;
	int destroy;
	int max_threads;
	int num_threads;
	int num_idle;
};

int threadpool_init(unsigned int max_threads, struct th_pool **pool_re);
int threadpool_destroy(struct th_pool *pool);
int threadpool_add_job(struct th_pool *pool, int job_id,
		void (*func)(void *fn_arg), void (*free)(void *fn_arg), void *fn_arg);

#endif
