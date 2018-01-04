
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#define LOG_TAG "posixTimer.cpp"
#include <debug.h>
#include "posixTimer.h"

int createTimer(void *data, timer_t *timerid, void (*pFunc)(sigval_t))
{
	db_msg("create timer data is 0x%p\n", data);
    struct sigevent se;
    memset(&se, 0, sizeof (se));
    se.sigev_notify = SIGEV_THREAD;
    se.sigev_notify_function = pFunc;
    se.sigev_value.sival_ptr = data;

    return timer_create(CLOCK_MONOTONIC, &se, timerid);
}

int setOneShotTimer(time_t sec, long nsec, timer_t timerid)
{
	struct itimerspec ts, ots;
	//db_msg("begain to set the one shot timer, timer id = 0x%lx\n", (unsigned long)timerid);

	ts.it_value.tv_sec = sec;
	ts.it_value.tv_nsec = nsec;
	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;

	return timer_settime(timerid, 0, &ts, &ots);
}

int setPeriodTimer(time_t sec, long nsec, timer_t timerid)
{
    struct itimerspec ts, ots; 
	db_msg("begain to set the period timer, timer id is 0x%lx\n", (unsigned long)timerid);	

	ts.it_value.tv_sec  = sec;
	ts.it_value.tv_nsec = nsec;
	ts.it_interval.tv_sec  = ts.it_value.tv_sec;
	ts.it_interval.tv_nsec = ts.it_value.tv_nsec;

	return timer_settime(timerid, 0, &ts, &ots);
}

int getExpirationTime(time_t *sec, long *nsec, timer_t timerid)
{
	struct itimerspec cur_value;	

	if(timer_gettime(timerid, &cur_value) < 0) {
		return -1;
	}
	*sec  = cur_value.it_value.tv_sec;
	*nsec = cur_value.it_value.tv_nsec;
	return 0;
}

int stopTimer(timer_t timerid)
{
    struct itimerspec ts; 

	ts.it_value.tv_sec  = 0;
	ts.it_value.tv_nsec = 0;
	ts.it_interval.tv_sec  = 0;
	ts.it_interval.tv_nsec = 0;

	return timer_settime(timerid, 0, &ts, NULL);
}

int deleteTimer(timer_t timerid)
{
	db_msg("deleting the posix timer, timer id = 0x%lx\n", (unsigned long)timerid);
	return timer_delete(timerid);
}

