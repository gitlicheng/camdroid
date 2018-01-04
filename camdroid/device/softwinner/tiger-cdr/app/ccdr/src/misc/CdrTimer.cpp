
#include "CdrTimer.h"
#undef LOG_TAG
#define LOG_TAG "CdrTimer"
#include "debug.h"

CdrTimer::CdrTimer(void* context, int idx)
{
	mTimerData.context = context;
	mTimerData.id = 0;
	mTimerData.time = 1;
	mTimerData.idx = idx;
}

CdrTimer::~CdrTimer()
{
	deleteTimer(mTimerData.id);
}

void CdrTimer::init(void(*callback)(sigval_t))
{
	createTimer((void*)&mTimerData, &mTimerData.id, callback);
}

void CdrTimer::startOneShot(unsigned int time_sec)
{
	if(mTimerData.id != 0) {
		mTimerData.time = time_sec;
		setOneShotTimer(time_sec, 0, mTimerData.id);
	} else {
		db_error("Timer not init");
	}
}

void CdrTimer::startPeriod(unsigned int time_sec)
{
	if(mTimerData.id != 0) {
		mTimerData.time = time_sec;
		setPeriodTimer(time_sec, 0, mTimerData.id);
	} else {
		db_error("Timer not init");
	}
}

void CdrTimer::startOneShot(void)
{
	if(mTimerData.id != 0) {
		setOneShotTimer(mTimerData.time, 0, mTimerData.id);
	} else {
		db_error("Timer not init");
	}
}

void CdrTimer::startPeriod(void)
{
	if(mTimerData.id != 0) {
		setPeriodTimer(mTimerData.time, 0, mTimerData.id);
	} else {
		db_error("Timer not init");
	}
}

void CdrTimer::stop()
{
	if(mTimerData.id != 0) {
		stopTimer(mTimerData.id);
	}
}

bool CdrTimer::timerStoped()
{
	time_t sec = 0;
	long int nsec = 0;
	if(mTimerData.id != 0) {
		getExpirationTime(&sec, &nsec, mTimerData.id);
		db_msg("sec %ld, nsec %ld", sec, nsec);
		if (sec == 0 && nsec == 0) {
			return true;
		}
	}

	return false;
}

void* CdrTimer::getContext(void)
{
	return mTimerData.context;
}
