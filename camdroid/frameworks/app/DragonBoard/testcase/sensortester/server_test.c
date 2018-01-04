#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <errno.h>
#include <dragonboard.h>

#define FIFO_ACC "fifo_acc"
#define FIFO_RTC "fifo_rtc"

void* threadSensor(void* argv)
{
	int fifoFd, retVal;
	struct fifo_param fifo_data;

	if ((fifoFd = open(FIFO_ACC, O_RDONLY)) < 0) {
		if (mkfifo(FIFO_ACC, 0666) < 0) {
			printf("[%s] mkfifo for %s failed(%s)\n", __FILE__, FIFO_ACC, strerror(errno));
			return NULL;
		} else {
			fifoFd = open(FIFO_ACC, O_RDONLY);
		}
	}

	while (1) {
		read(fifoFd, &fifo_data, sizeof(fifo_param_t));
		printf("\n[sensor]: <%5.2f, %5.2f, %5.2f>\n\n", fifo_data.val.acc[0], fifo_data.val.acc[1], fifo_data.val.acc[2]);
	}
	close(fifoFd);

	return NULL;
}

void* threadRTC(void* argv)
{
	int fifoFd, retVal;
	struct fifo_param fifo_data;

	if ((fifoFd = open(FIFO_RTC, O_RDONLY)) < 0) {
		if (mkfifo(FIFO_RTC, 0666) < 0) {
			printf("[%s] mkfifo for %s failed(%s)\n", __FILE__, FIFO_RTC, strerror(errno));
			return NULL;
		} else {
			fifoFd = open(FIFO_RTC, O_RDONLY);
		}
	}

	while (1) {
		read(fifoFd, &fifo_data, sizeof(fifo_param_t));
		printf("\n[rtc]: %d-%d-%d %d:%d:%d\n\n", fifo_data.val.rtc.tm_mon + 1, fifo_data.val.rtc.tm_mday, fifo_data.val.rtc.tm_year + 1900, fifo_data.val.rtc.tm_hour, fifo_data.val.rtc.tm_min, fifo_data.val.rtc.tm_sec);
	}
	close(fifoFd);

	return NULL;
}

int main(void)
{
	int fifoFd, retVal;
	pthread_t tidSensor, tidRTC;
	fifo_param_t fifo_data;

	tidSensor = pthread_create(&tidSensor, NULL, threadSensor, NULL);
	if (tidSensor != 0) {
		printf("[%s] create thread [threadSensor] failed(%s)\n", __FILE__, strerror(errno));
	}
	tidRTC = pthread_create(&tidRTC, NULL, threadRTC, NULL);
	if (tidRTC != 0) {
		printf("[%s] create thread [threadRTC] failed(%s)\n", __FILE__, strerror(errno));
	}

	while (1) {
		sleep(10);
	}

/*
	if ((fifo = open(FIFO_ACC, O_RDONLY)) < 0) {
		retval = mkfifo(FIFO_ACC, 0666);
		if (retval < 0) {
			perror("mkfifo failed");
			return -1;
		}
		fifo = open(FIFO_ACC, O_RDONLY);
		if (fifo < 0) {
			perror("open fifo failed");
			return -1;
		}
	}
	if ((fifo = open(FIFO_ACC, O_RDONLY)) < 0) {
		retval = mkfifo(FIFO_ACC, 0666);
		if (retval < 0) {
			perror("mkfifo failed");
			return -1;
		}
		fifo = open(FIFO_ACC, O_RDONLY);
		if (fifo < 0) {
			perror("open fifo failed");
			return -1;
		}
	}

	while (1) {
		read(fifo, &fifo_data, sizeof(fifo_param_t));
		puts("\n[Server get info from client]:");
		printf("type: %c, name: %s, result: %c, value:<%5.1f, %5.1f, %5.1f>\n",
			fifo_data.type, fifo_data.name, fifo_data.result,
			fifo_data.val.acc[0], fifo_data.val.acc[1], fifo_data.val.acc[2]);
	}
*/

	return 0;
}
