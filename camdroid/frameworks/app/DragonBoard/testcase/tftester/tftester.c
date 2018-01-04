// judge tf card by detect file /dev/block/mmcblk0 not by /mnt/extsd

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#define TAG "tftester"
#include <dragonboard.h>

#define FIFO_DEV  "fifo_tf"

int main(int argc, char **argv)
{
	int retVal, fifoFd, tfFd;

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
		if ((tfFd = open("/dev/block/mmcblk0", O_RDONLY)) < 0) {
			write(fifoFd, "F[TF] fail", 15);
		} else {
			write(fifoFd, "P[TF] pass", 15);
			close(tfFd);
		}
		usleep(200000);			// careful! detect TF card every 20ms
	}
	close(fifoFd);

	return 0;
}
