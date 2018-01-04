#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#define TAG "ddrtester"
#include <dragonboard.h>

#define FIFO_DEV  "fifo_ddr"

int main(int argc, char **argv)
{
	int retVal, fifoFd;

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

	while (1) {
		write(fifoFd, "P[DDR] pass", 15);
		sleep(5);
	}
	close(fifoFd);

	return 0;
}
