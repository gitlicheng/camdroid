#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/rtc.h>

#define TAG "rtctester"
#include <dragonboard.h>

#define RTC_DEV   "/dev/rtc0"
#define FIFO_DEV  "fifo_rtc"

int GetRTCTime(int fd, struct rtc_time* prtc)
{
	int retVal;

	if ((retVal = ioctl(fd, RTC_RD_TIME, prtc)) < 0) {			// check out block or not when dev not exist
		db_error("get rtc time failed(%s)\n", strerror(errno));
		return -1;
	} else {
		return 0;
	}
}

// this part not done
//int CheckRTCData(struct rtc_time* rtc_tm)
//{
//}

int main(int argc, char **argv)
{
	int retVal, rtcFd, fifoFd;
	struct fifo_param fifo_data;
	struct rtc_time rtc_tm;
	char data_buf[30];

	// this part works well since then, so no need to notify server if open fifo failed
	// if we want to notify server, use other ipc tools, such as message, shared memory
	if ((fifoFd = open(FIFO_DEV, O_WRONLY)) < 0) {		// fifo's write-endian block until read-endian open 
		if (mkfifo(FIFO_DEV, 0666) < 0) {
			db_error("mkfifo failed(%s)\n", strerror(errno));
			return -1;
		} else {
			fifoFd = open(FIFO_DEV, O_WRONLY);
		}
	}
	
	if ((rtcFd = open(RTC_DEV, O_RDWR)) < 0) {
	//	db_error("open %s failed(%s)\n", RTC_DEV, strerror(errno));
	//	fifo_data.type = 'A';
	//	strlcpy(fifo_data.name, RTC, 10);
	//	fifo_data.result = 'F';
	//	write(fifoFd, &fifo_data, sizeof(fifo_param_t));

		// add notify-info to tell server(fifo's read-endian) that client will close fifo's write-endian
		// If not, the server should use other method to dectect wheather fifo's write-endian is closed
		// D--Detect fifo's state
		// write(fifoFd, "D[RTC] close fifo", 18);
		// write(fifoFd, &fifo_data, sizeof(fifo_param_t));
		strlcpy(data_buf, "F[RTC] fail", 30);
		write(fifoFd, data_buf, 30);
		close(fifoFd);
		return -1;
	} else {
//		fifo_data.type = 'A';
//		strlcpy(fifo_data.name, RTC, 10);
//		fifo_data.result = 'W';
//		write(fifoFd, &fifo_data, sizeof(fifo_param_t));
		strlcpy(data_buf, "W[RTC] waiting", 30);
		write(fifoFd, data_buf, 30);
	}
	db_msg("open rtc dev OK\n");			// use debug level [db_msg] when open dev success, use [db_debug] for get data

	while (1) {
		retVal = GetRTCTime(rtcFd, &rtc_tm);
//		retVal = CheckRTCData(&rtc_tm);		// 0-OK; !0-Fail
		if (retVal == 0) {
//			fifo_data.type = 'A';
//			strlcpy(fifo_data.name, RTC, 10);
//			fifo_data.result = 'P';			// P-pass; F-fail
//			fifo_data.val.rtc = rtc_tm;
//			write(fifoFd, &fifo_data, sizeof(fifo_param_t));
			sprintf(data_buf, "P[RTC] %.2d:%.2d:%.2d", rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);
			write(fifoFd, data_buf, 30);
		} else {
//			db_error("rtc data NOT correct\n");
//			fifo_data.type = 'A';
//			strlcpy(fifo_data.name, RTC, 10);
//			fifo_data.result = 'F';			// need finish the tester?(2015.01.06)
//			write(fifoFd, &fifo_data, sizeof(fifo_param_t));
			sprintf(data_buf, "F[RTC] rtc data err");
			write(fifoFd, data_buf, 30);
		}
//		memset(&fifo_data, 0, sizeof(fifo_param_t));
		sleep(1);
	}

/*
	puts("test rtc...");
	rtcFd = open(RTC_DEV, O_RDONLY);
	if (rtcFd < 0) {
		perror("can NOT open rtc dev");
	} else {
		printf("rtcFd = %d\n", rtcFd);
	}
	while (1) {
		retVal = ioctl(rtcFd, RTC_RD_TIME, &rtc_tm);
		if (retVal < 0) {
			perror("ioctl failed");
			return -1;
		} else {
			printf("retVal: %d\n", retVal);
			printf("data: %d-%d-%d, time: %d:%d:%d\n", rtc_tm.tm_mon, rtc_tm.tm_mday, rtc_tm.tm_year+1900, rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);
		}
		sleep(1);
	}
*/

	close(fifoFd);
	close(rtcFd);

	return 0;
}
