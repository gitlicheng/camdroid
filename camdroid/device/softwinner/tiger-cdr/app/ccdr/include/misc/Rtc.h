#ifndef _RTC_H_
#define _RTC_H_

#include <sys/time.h>
#include <linux/rtc.h>
#include <time.h>

int setDateTime(struct tm* ptm);
time_t getDateTime(struct tm **local_time);
void resetDateTime(void);

#endif	//_RTC_H_

