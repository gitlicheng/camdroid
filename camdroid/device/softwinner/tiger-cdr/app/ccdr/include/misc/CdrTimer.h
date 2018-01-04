
#ifndef __CDR_TIMER_H__
#define __CDR_TIMER_H__

#include "posixTimer.h"

typedef struct 
{
	int idx;
	void *context;
	timer_t id;
	time_t time;
}timer_set;


class CdrTimer
{
public:
	CdrTimer(void *context, int idx);
	~CdrTimer();

	void init(void(*callback)(sigval_t));
	void startOneShot(unsigned int time_sec);
	void startOneShot(void);

	void startPeriod(unsigned int time_sec);
	void startPeriod(void);

	void stop();
	bool timerStoped();

	void* getContext(void);

private:
	timer_set mTimerData;
};

#endif
