
#ifndef __POSIX_TIMER_H__
#define __POSIX_TIMER_H__

#include <time.h>
#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif


extern int createTimer(void *data, timer_t *timerid, void (*pFunc)(sigval_t));


extern int setOneShotTimer(time_t sec, long nsec, timer_t timerid);

extern int setPeriodTimer(time_t sec, long nsec, timer_t timerid);

extern int getExpirationTime(time_t *sec, long *nsec, timer_t timerid);

extern int stopTimer(timer_t timerid);

extern int deleteTimer(timer_t timerid);


#ifdef __cplusplus
}  /* end of extern "C" */
#endif

#endif
