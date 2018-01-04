#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <semaphore.h>
#include <pthread.h>
#include <cutils/log.h>
#include <linux/sched.h>
#include <sys/vfs.h>
#include <fat_user.h>

/////////////////////////////////////////////////
#include "fat_user.h"
static void itimeofday(long *sec, long *usec)
{
#ifdef WIN32
	static long mode = 0, addsec = 0;
	BOOL retval;
	static DWORD freq = 1;
	DWORD qpc;
	if (mode == 0) {
		retval = QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
		freq = (freq == 0)? 1 : freq;
		retval = QueryPerformanceCounter((LARGE_INTEGER*)&qpc);
		addsec = (long)time(NULL);
		addsec = addsec - (long)((qpc / freq) & 0x7fffffff);
		mode = 1;
	}
	retval = QueryPerformanceCounter((LARGE_INTEGER*)&qpc);
	retval = retval * 2;
	if (sec) *sec = (long)(qpc / freq) + addsec;
	if (usec) *usec = (long)((qpc % freq) * 1000000 / freq);

#else
	struct timeval time;
	gettimeofday(&time, NULL);
	if (sec) *sec = time.tv_sec;
	if (usec) *usec = time.tv_usec;
#endif
}

/* get clock in millisecond 64 */
static  DWORD iclock64(void)
{
	long s, u;
	DWORD value;
	itimeofday(&s, &u);
	value = ((DWORD)s) * 1000 + (u / 1000);
	return value;
}

static  unsigned int iclock32(void)
{
	return (unsigned int)(iclock64() & 0xfffffffful);
}


int scan_files (char* _path)
{
	int dir_fd;
	int res;
	int i;
	char *fn;
	char path[128] = {0};
	FAT_FILINFO Finfo;

	strcpy(path, _path);
	printf("scan_files in  path[%s]\n", path);

	if ((dir_fd = fat_opendir(path)) >= 0) {
		i = strlen(path);
		while ((res = fat_readdir(dir_fd, &Finfo)) == 0)
		{
			if (Finfo.fname[0] == '.')
				continue;

			if(Finfo.fname[0] == 0)
				break;

			//fn = Finfo.fname;
			fn = *Finfo.lfname? Finfo.lfname: Finfo.fname;


			if (Finfo.fattrib & AM_DIR) {
				*(path+i) = '/';
				strcpy(path+i+1, fn);
				printf("  FAT_DIR [%s]\n", path);
				res = scan_files(path);
				*(path+i) = '\0';


				if (res != 0) break;
			} else {
				printf("file name [%s]\n", fn);
			}
		}
		fat_closedir(dir_fd);
	}

	return res;
}


/* =================================================================== */
/* =                       speed tester                              = */
/* =================================================================== */
/* FatFs operation */
int test_fatfs_write(const char* file)
{
	char line[64*1024];
	int timeCost;
	int oldtime, begintime;
	int i = 0;
	int fd;
	int write_len;
	unsigned int total_len = 0;

	printf("func: %s\n", __func__);
	puts("description: test write-speed in one thread by fatfs\n");
	
	fd = fat_open(file, FA_WRITE | FA_CREATE_ALWAYS);
	if (fd < 0) {
		printf("[app](%s) fat_open file(%s) fail: (%d) \n\n", __func__, file, fd);
	    	return -1;
	}

	timeCost = iclock32();
	oldtime = timeCost;
	begintime = timeCost;

	for(i=0; i<64; i++) {
		memset(line+(i*1024), 49+i, 1024);
	}

	i = 0;
	while (1) {
		if (i++ > 1024)
			break;

		write_len = fat_write(fd, line, 64*1024);
		total_len += write_len;

		if(iclock32() - oldtime > 1000) {
			fat_sync(fd);
			printf("fd [%d], tid: [%d], pid: [%d], timeCost [%d] ms, size [%d] KB, write speed [%lld] KB/s\n", fd, gettid(), getpid(), iclock32()-oldtime, total_len/1024, (long long)total_len*1000/1024/(iclock32()-oldtime));
			total_len = 0;
			oldtime	 = iclock32();
		}

		if(write_len <= 0) {
			printf("[app](%s) fat_write fail: %d, test_fatfs_write error! \n\n", __func__, write_len);
			return -1;
		}
	}
	fat_sync(fd);
	printf("[app](%s) file: [%s], file_len: [%d] MB, timeCost = [%d] ms, average speed = [%d] KB/s\n\n",   \
		__func__, file, 64 , iclock32()-begintime, 64*1024*1000/(iclock32()-begintime));
	fat_close(fd);
	return 0;
}

void* FatFsWriteThread(void* argv)
{
	int ret;

	ret = test_fatfs_write((char*)argv);

	if (ret < 0) {
		return (void*)"test_fatfs_write fail";
	} else {
		return (void*)"test_fatfs_write success";
	}
}

void test_DoubleThreadsWriteByFatFs(void)
{
	int fd;
	pthread_t pid1, pid2;
	char *ret1, *ret2;
	char *path1 = "0:doubleThreadsWriteByFatFs_1";
	char *path2 = "0:doubleThreadsWriteByFatFs_2";

	printf("func: %s\n", __func__);
	puts("description: test write-speed in two threads by FatFs\n");
	
	pthread_create(&pid1, NULL, FatFsWriteThread, path1);
	pthread_create(&pid2, NULL, FatFsWriteThread, path2);

	pthread_join(pid1, (void**)&ret1);
	printf("[app](%s) write file(%s) finished! the result ==> (%s)\n", __func__, path1, ret1);
	pthread_join(pid2, (void**)&ret2);
	printf("[app](%s) write file(%s) finished! the result ==> (%s)\n\n", __func__, path2, ret2);
}

int test_fatfs_read(const char *file)
{
	char line[64*1024] = {0};
	int fd;
	int timeCost;
	int oldtime, begintime;
	int read_len = 0;
	unsigned int total_len = 0;
	unsigned int file_len = 0;

	printf("func: %s\n", __func__);
	puts("description: test read-speed in one thread by fatfs\n");
	
	fd = fat_open(file, FA_READ);
	if (fd < 0 ) {
		printf("[app](%s) fat_open file(%s) fail: (%d) \n\n", __func__, file, fd);
		return -1;
	}

	timeCost = iclock32();
	oldtime = timeCost;
	begintime = oldtime;
	
	while (1) {
		read_len = fat_read(fd, line, 64*1024);
		total_len += read_len;
		file_len += read_len;
	
		if(iclock32() - oldtime > 1000)
		{
			printf("fd: [%d], tid: [%d], pid: [%d], timeCost [%d] ms, size [%d] KB, read speed [%lld] kb/s\n", fd, gettid(), getpid(), iclock32()-oldtime, total_len/1024, (long long)total_len*1000/1024/(iclock32()-oldtime));
			total_len = 0;
			oldtime	 = iclock32();
		}
	
		if (read_len < 0) {
			printf("[app](%s) fat_read fail: %d, test_fatfs_read error! \n\n", __func__, read_len);
			return -1;
		} else if (read_len == 0) {
			break;
		}
	}

	printf("[app](%s) file: [%s], file_len: [%d] MB, timeCost = [%d] ms, average speed = [%lld] KB/s\n\n",   \
		__func__, file, file_len/1024/1024 , iclock32() - begintime, (long long)file_len*1000/1024/(iclock32()-begintime));
	fat_close(fd);
	return 0;
}

void* FatFsReadThread(void* argv)
{
	int ret;

	ret = test_fatfs_read((char*)argv);

	if (ret < 0) {
		return (void*)"test_fatfs_read fail";
	} else {
		return (void*)"test_fatfs_read success";
	}
}

void test_DoubleThreadsReadByFatFs(void)
{
	int fd;
	pthread_t pid1, pid2;
	char *ret1, *ret2;
	char *path1 = "0:doubleThreadsWriteByFatFs_1";
	char *path2 = "0:doubleThreadsWriteByFatFs_2";

	printf("func: %s\n", __func__);
	puts("description: test read-speed in two threads by FatFs\n");
	
	pthread_create(&pid1, NULL, FatFsReadThread, path1);
	pthread_create(&pid2, NULL, FatFsReadThread, path2);

	pthread_join(pid1, (void**)&ret1);
	printf("[app](%s) read file(%s) finished! the result ==> (%s)\n", __func__, path1, ret1);
	pthread_join(pid2, (void**)&ret2);
	printf("[app](%s) read file(%s) finished! the result ==> (%s)\n\n", __func__, path2, ret2);
}

void test_fatfs_read_by_hand(void)
{
	int res, fd;
	char buf[128] = {0};
	
	printf("func: %s\n", __func__);
	printf("Input file path and ready to read file:");
	gets(buf);
	test_fatfs_read(buf);
}
/* =================================================================== */



/* =================================================================== */
/* VFS operation */
int test_vfs_write(const char* path)
{
	int write_len;
	unsigned int total_len = 0;
	char line[64*1024];
	int timeCost;
	int oldtime, begintime;
	int i = 0;
	FILE *fptr = NULL;

	printf("func: %s\n", __func__);
	puts("description: test write-speed in one thread by VFS\n");

	if ((fptr = fopen(path, "w")) == NULL) {
		printf("[app](%s) open file(%s) fail: (%s) \n\n", __func__, path, strerror(errno));
		return -1;
	}

	timeCost = iclock32();
	oldtime = timeCost;
	begintime = timeCost;

	for(i=0; i<64; i++) {
		memset(line+(i*1024), 49+i, 1024);
	}

	i = 0;
	while (1) {
		if (i++ > 1024)
			break;

		write_len = fwrite(line, 1, 64*1024, fptr);
		total_len += write_len;

		if(iclock32() - oldtime > 1000) {
			fflush(fptr);
			printf("tid: [%d], pid: [%d], timeCost [%d] ms, size [%d] KB, write speed [%lld] KB/s\n", gettid(), getpid(), iclock32()-oldtime, total_len/1024, (long long)total_len*1000/1024/(iclock32()-oldtime));
			total_len = 0;
			oldtime	 = iclock32();
		}

		if(write_len <= 0) {
			printf("fwrite fail: %d, test_vfs_write error: (%s) \n\n", write_len, strerror(errno));
			return -1;
		}
	}
	fflush(fptr);
	fclose(fptr);
	printf("[app](%s) file: [%s], file_len: [%d] MB, timeCost = [%d] ms, average speed = [%d] KB/s \n\n",   \
	                __func__, path, 64, iclock32() - begintime, 64*1024*1000/(iclock32()-begintime));

	return 0;
}

void* VFSWriteThread(void* argv)
{
	int ret;

	ret = test_vfs_write((char*)argv);

	if (ret < 0) {
		return (void*)"test_vfs_write fail";
	} else {
		return (void*)"test_vfs_write success";
	}
}

void test_DoubleThreadsWriteByVFS(void)
{
	int fd;
	pthread_t pid1, pid2;
	char *ret1, *ret2;
	char *path1 = "/mnt/extsd/doubleThreadsWriteByVFS_1";
	char *path2 = "/mnt/extsd/doubleThreadsWriteByVFS_2";

	printf("func: %s\n", __func__);
	puts("description: test write-speed in two threads by VFS\n");
	
	pthread_create(&pid1, NULL, VFSWriteThread, path1);
	pthread_create(&pid2, NULL, VFSWriteThread, path2);

	pthread_join(pid1, (void**)&ret1);
	printf("[app](%s) write file(%s) finished! the result ==> (%s)\n", __func__, path1, ret1);
	pthread_join(pid2, (void**)&ret2);
	printf("[app](%s) write file(%s) finished! the result ==> (%s)\n\n", __func__, path2, ret2);
}

int test_vfs_read(const char* path)
{
	int read_len;
	unsigned int total_len = 0;		// read or write length for 1s
	unsigned int file_len = 0;		// file's length in unit of byte
	char line[64*1024];
	int timeCost;
	int oldtime, begintime;
	int i = 0;
	FILE *fptr = NULL;

	printf("func: %s\n", __func__);
	puts("description: test read-speed in one thread by VFS\n");

	if ((fptr = fopen(path, "r")) == NULL) {
		printf("[app](%s) open file(%s) fail: (%s) \n\n", __func__, path, strerror(errno));
		return -1;
	}

	timeCost = iclock32();
	oldtime = timeCost;
	begintime = timeCost;

	i = 0;
	while (1) {
		read_len = fread(line, 1, 64*1024, fptr);
		total_len += read_len;
		file_len += read_len;

		if(iclock32() - oldtime > 1000) {
			printf("tid: [%d], pid: [%d], timeCost [%d] ms, size [%d] KB, read speed [%lld] KB/s\n", gettid(), getpid(), iclock32()-oldtime, total_len/1024, (long long)total_len*1000/1024/(iclock32()-oldtime));
			total_len = 0;
			oldtime	 = iclock32();
		}

		if(read_len < 0) {
			printf("fread fail, test_vfs_read error: (%s) \n\n", strerror(errno));
			return -1;
		} else if (read_len == 0){
			break;
		}
	}
	fclose(fptr);
	printf("[app](%s) file: [%s], file_len: [%d] MB, timeCost = [%d] ms, average speed = [%lld] KB/s \n\n",   \
	                __func__, path, file_len/1024/1024, iclock32() - begintime, (long long)file_len*1000/1024/(iclock32()-begintime));

	return 0;
}

void* VFSReadThread(void* argv)
{
	int ret;

	ret = test_vfs_read((char*)argv);

	if (ret < 0) {
		return (void*)"test_vfs_read fail";
	} else {
		return (void*)"test_vfs_read success";
	}
}

void test_DoubleThreadsReadByVFS(void)
{
	int fd;
	pthread_t pid1, pid2;
	char *ret1, *ret2;
	char *path1 = "/mnt/extsd/doubleThreadsWriteByVFS_1";
	char *path2 = "/mnt/extsd/doubleThreadsWriteByVFS_2";

	printf("func: %s\n", __func__);
	puts("description: test read-speed in two threads by VFS\n");
	
	pthread_create(&pid1, NULL, VFSReadThread, path1);
	pthread_create(&pid2, NULL, VFSReadThread, path2);

	pthread_join(pid1, (void**)&ret1);
	printf("[app](%s) read file(%s) finished! the result ==> (%s)\n", __func__, path1, ret1);
	pthread_join(pid2, (void**)&ret2);
	printf("[app](%s) read file(%s) finished! the result ==> (%s)\n\n", __func__, path2, ret2);
}
/* =================================================================== */



/* =================================================================== */
/* DirectIO operation */
int test_directio_write(const char* path)
{
	int write_len;
	unsigned int total_len = 0;
	int fd;
	char line[512*1024];
	int timeCost;
	int oldtime, begintime;
	int i = 0;
	int timeTmp, timeGap;
	static int averageSpeed;

	printf("func: %s\n", __func__);
	puts("description: test write-speed in one thread by DirectIO\n");

	if ((fd = open(path, O_CREAT|O_TRUNC|O_RDWR|O_DIRECT, 0666)) < 0) {
		printf("[app](%s) open file(%s) fail: (%s) \n\n", __func__, path, strerror(errno));
		return -1;
	}

	timeCost = iclock32();
	oldtime = timeCost;
	begintime = timeCost;

	for(i=0; i<512; i++) {
		memset(line+(i*1024), 49+i, 1024);
	}

	i = 0;
	while (1) {
		if (i++ > 1024)
			break;

		timeTmp = iclock32();
		write_len = write(fd, line, 512*1024);
//		if ((timeGap = iclock32() - timeTmp) > 300)
//			printf("[warning] write time over threadhold(300ms): %d ms\n", timeGap);
		total_len += write_len;

		if(iclock32() - oldtime > 1000) {
			fsync(fd);
			printf("fd: [%d], tid: [%d], pid: [%d], timeCost [%d] ms, size [%d], write speed [%lld] kb/s\n", fd, gettid(), getpid(), iclock32()-oldtime, total_len, (long long)total_len*1000/1024/(iclock32()-oldtime));
			total_len = 0;
			oldtime	 = iclock32();
		}

		if(write_len <= 0) {
			printf("fd: [%d], test_directio_write error: (%s) \n\n", fd, strerror(errno));
			return -1;
		}
	}
	fsync(fd);
	printf("[app](%s) fd: [%d], tid: [%d], pid: [%d], write timeCost = [%d] ms, average speed = [%d] KB/s \n\n",   \
			__func__, fd, gettid(), getpid(), iclock32() - begintime, (averageSpeed = 512*1024*1000/(iclock32()-begintime)));
	close(fd);

	pthread_exit((void*)averageSpeed);

	return 0;
}

void* DirectIOWriteThread(void* argv)
{
	int ret;

	ret = test_directio_write((char*)argv);

	if (ret < 0) {
		return (void*)"test_directio_write fail";
	} else {
		return (void*)"test_directio_write success";
	}
}

void test_DoubleThreadsWriteByDirectIO(void)
{
	int fd;
	pthread_t pid1, pid2;
	char *ret1, *ret2;
	char *path1 = "/mnt/extsd/doubleThreadsWriteByDirectIO_1";
	char *path2 = "/mnt/extsd/doubleThreadsWriteByDirectIO_2";

	printf("func: %s\n", __func__);
	puts("description: test write-speed in two threads by DirectIO\n");
	
	pthread_create(&pid1, NULL, DirectIOWriteThread, path1);
	pthread_create(&pid2, NULL, DirectIOWriteThread, path2);

	pthread_join(pid1, (void**)&ret1);
	printf("[app](%s) write file(%s) finished! the result ==> (%s)\n", __func__, path1, ret1);
	pthread_join(pid2, (void**)&ret2);
	printf("[app](%s) write file(%s) finished! the result ==> (%s)\n\n", __func__, path2, ret2);
}

int test_directio_read(const char* path)
{
	int read_len = 0;
	unsigned int total_len = 0;
	unsigned int file_len = 0;
	int fd;
	char line[64*1024];
	int timeCost;
	int oldtime, begintime;
	int i = 0;

	printf("func: %s\n", __func__);
	puts("description: test read-speed in one thread by DirectIO\n");

	if ((fd = open(path, O_TRUNC|O_RDWR|O_DIRECT, 0666)) < 0) {
		printf("[app](%s) open file(%s) fail:%s\n\n", __func__, path, strerror(errno));
		return -1;
	}

	timeCost = iclock32();
	oldtime = timeCost;
	begintime = timeCost;

	while (1) {
		printf("before directio read\n");
		read_len = read(fd, line, 64*1024);
		printf("after directio read\n");
		total_len += read_len;
		file_len += read_len;

		if(iclock32() - oldtime > 1000) {
			printf("fd: [%d], tid: [%d], pid: [%d], timeCost [%d] ms, size [%d], read speed [%lld] kb/s\n", fd, gettid(), getpid(), iclock32()-oldtime, total_len, (long long)total_len*1000/1024/(iclock32()-oldtime));
			total_len = 0;
			oldtime	 = iclock32();
		}

		if(read_len < 0) {
			printf("fd: [%d], test_directio_read error: (%s) \n\n", fd, strerror(errno));
			return -1;
		} else if (read_len == 0) {
			break;
		}
	}
	printf("[app](%s) fd: [%d], tid: [%d], pid: [%d], file_len: [%d] MB, read timeCost = [%d] ms, average speed = [%lld] KB/s \n", __func__, fd, gettid(), getpid(), file_len/1024/1024, iclock32() - begintime, (long long)file_len*1000/1024/(iclock32()-begintime));
	close(fd);

	return 0;
}

void* DirectIOReadThread(void* argv)
{
	int ret;

	ret = test_directio_read((char*)argv);

	if (ret < 0) {
		return (void*)"test_directio_read fail";
	} else {
		return (void*)"test_directio_read success";
	}
}

void test_DoubleThreadsReadByDirectIO(void)
{
	int fd;
	pthread_t pid1, pid2;
	char *ret1, *ret2;
	char *path1 = "/mnt/extsd/doubleThreadsWriteByDirectIO_1";
	char *path2 = "/mnt/extsd/doubleThreadsWriteByDirectIO_2";

	printf("func: %s\n", __func__);
	puts("description: test read-speed in two threads by DirectIO\n");
	
	pthread_create(&pid1, NULL, DirectIOReadThread, path1);
	pthread_create(&pid2, NULL, DirectIOReadThread, path2);

	pthread_join(pid1, (void**)&ret1);
	printf("[app](%s) read file(%s) finished! the result ==> (%s)\n", __func__, path1, ret1);
	pthread_join(pid2, (void**)&ret2);
	printf("[app](%s) read file(%s) finished! the result ==> (%s)\n", __func__, path2, ret2);
}
/* =================================================================== */



/* =================================================================== */
/* double processed write by FatFs */
void test_DoubleProcessesWriteByFatFs(void)
{
	pid_t pid;
	int fd, res;
	
	printf("func: %s\n", __func__);
	puts("description: write 10 files in double processes each with 64M-size by FatFs");
	
	pid = fork();
	if (pid == 0) {
		printf("(child process) pid: %d, ppid: %d\n", getpid(), getppid());
		int i;
		char buf[64];
		for (i = 0; i < 10; i++) {
			sprintf(buf, "0:test_DoubleProcessesWrite0%d", i);
			test_fatfs_write(buf);
		}
		printf("\n(child process) end of write\n\n");
	} else {
		printf("(father process) pid: %d, ppid: %d\n", getpid(), getppid());
		int i;
		char buf[64];
		for (i = 10; i < 20; i++) {
			sprintf(buf, "0:test_DoubleProcessesWrite%d", i);
			test_fatfs_write(buf);
		}
		printf("(father process) end of write\n\n");
	}
}
/* =================================================================== */

void test_getCardInfo(void)
{
    FAT_STATFS diskinfo;
    int res;

    printf("func: %s\n", __func__);
    puts("description: get card information(total and free blocks by sectors)");

    res = fat_statfs("0:", &diskinfo);
    if (res < 0) {
        printf("[app] fat_statfs res: %d, maybe card is NOT exist\n\n", res);
        return ;
    }
    printf("[app] f_type: %d, f_bsize: %d, f_blocks: %llu, f_bfree: %llu\n\n", diskinfo.f_type, diskinfo.f_bsize, diskinfo.f_blocks, diskinfo.f_bfree);
}

void test_getCardStatus(void)
{
    int res;

    printf("func: %s\n", __func__);
    puts("description: get card status(inserted or not)");

    res = fat_card_status();
    printf("[app](%s) card status: %d\n\n", __func__, res);
}


char *writingFilePath = "0:test_getWritingFileSize";
void* threadGetFileSize(void* argv)
{
	int fd, res;
	FAT_FILINFO filinfo;

	sleep(2);
	printf("[app](%s) thread2 get file's size by fat_stat()\n", __func__);
	res = fat_stat(writingFilePath, &filinfo);
	if (res < 0) {
		printf("[app](%s) fat_stat fail: %d\n\n", __func__, res);
		return (void*)"fat_stat() fail";
	}
	printf("[app](%s) read file(%s) size...\n", __func__, writingFilePath);
	printf("[app](%s) fat_stat res: %d, filinfo.fsize: %d, fdate: %#x, ftime: %#x, fattrib: %#x\n", __func__, res, filinfo.fsize, filinfo.fdate, filinfo.ftime, filinfo.fattrib);
	printf("[app](%s) fname: %s, lfname: %s, lfsize: %u\n\n", __func__, filinfo.fname, filinfo.lfname, filinfo.lfsize);

	return (void*)"fat_stat() ok";
}
void test_getWritingFileSize(void)
{
	int fd, len, pid, i;
	char *ret;
	pthread_t tid;
	char buf[64 * 1024];
	
	printf("func: %s\n", __func__);
	puts("description: one thread is writing a file, then the other thread get its size by fat_stat()");
	
	fd = fat_open(writingFilePath, FA_WRITE | FA_OPEN_ALWAYS);
	if (fd < 0) {
	    printf("[app](%s) fat_open(%s) fail: %d\n\n", __func__, writingFilePath, fd);
	    return ;
	}
	
	pthread_create(&tid, NULL, threadGetFileSize, NULL);
	
	printf("[app](%s) thread1 will write a file with 128M-size, and wait until it return...\n\n", __func__);
	for (i = 0; i < 64; i++)
	    memset(&buf[i * 1024], 0x30 + i, 1024);
	for (i = 0; i < 2048; i++) {
	    len = fat_write(fd, buf, 64 * 1024);
	    if (len < 0) {
	        printf("[app](%s) fat_write fail: %d\n\n", __func__, len);
	        return ;
	    }
	}
	pthread_join(tid, (void**)&ret);
	printf("[app](%s) thread1 write file(%s) end\n", __func__, writingFilePath);
	printf("[app](%s) thread2 return result: [%s]\n\n", __func__, ret);
	fat_close(fd);
}

void test_getWritingFileSizeByHand(void)
{
	int res;
	char buf[64];
	FAT_FILINFO filinfo;
	
	printf("func: %s\n", __func__);
	puts("description: get file's size which is being written");

	printf("Input file path: ");
	gets(buf);
	res = fat_stat(buf, &filinfo);
	printf("[app](%s) read file(%s) info by fat_stat()...\n", __func__, buf);
	printf("[app](%s) fat_stat res: %d, filinfo.fsize: %d, fdate: %#x, ftime: %#x, fattrib: %#x\n", __func__, res, filinfo.fsize, filinfo.fdate, filinfo.ftime, filinfo.fattrib);
	printf("[app](%s) fname: %s, lfname: %s, lfsize: %u\n\n", __func__, filinfo.fname, filinfo.lfname, filinfo.lfsize);
}

void test_openFileTwice(const char *file)
{
	int fd, wlen, rlen;

	printf("func: %s\n", __func__);
	puts("description: open the same file twice and check fd");
	
	fd = fat_open(file, FA_READ | FA_WRITE | FA_OPEN_ALWAYS);
	if (fd < 0) {
	    printf("[app](%s) fat_open(%s) fail, fd: %d \n\n", __func__, file, fd);
	    return ;
	} else {
	    printf("[app](%s) fat_open(%s) ok, fd: %d\n", __func__, file, fd);
	}
	
	char *buf = "www.allwinnertech.com";
	wlen = fat_write(fd, buf, strlen(buf));
	printf("[app](%s) fat_write wlen: %d\n", __func__, wlen);
	
	printf("ready to fat_open file(%s) and fat_write again!\n", file);
	fd = fat_open(file, FA_READ | FA_WRITE | FA_OPEN_ALWAYS);
	if (fd < 0) {
	    printf("[app](%s) open file: %s failed, fd: %d\n\n", __func__, file, fd);
	    return ;
	} else {
	    printf("[app](%s) open file: %s ok, fd: %d\n", __func__, file, fd);
	}
	wlen = fat_write(fd, buf, strlen(buf));
	printf("[app](%s) fat_write wlen: %d\n", __func__, wlen);
	printf("[app](%s) fat_close(fd)\n\n", __func__);
	fat_close(fd);
	
	char rbuf[128] = {0};
	fd = fat_open(file, FA_READ | FA_WRITE | FA_OPEN_ALWAYS);
	if (fd < 0) {
	    printf("[app](%s) open file: %s failed, fd: %d\n\n", __func__, file, fd);
	    return ;
	} else {
	    printf("[app](%s) open file: %s ok, fd: %d\n", __func__, file, fd);
	}
	rlen = fat_read(fd, rbuf, 128);
	printf("[app](%s) rlen: %d, rbuf: (%s)\n\n", __func__, rlen, rbuf);
	fat_close(fd);
}

void test_writeAndRead(const char* file)
{
	int fd, rlen, wlen;

	printf("func: %s\n", __func__);
	puts("description: write a file and read it");
	
	fd = fat_open(file, FA_READ | FA_WRITE | FA_OPEN_ALWAYS);
	if (fd < 0) {
	    printf("[app](%s) open file: %s failed, fd: %d\n\n", __func__, file, fd);
	    return ;
	} else {
	    printf("[app](%s) open file: %s OK, fd: %d\n", __func__, file, fd);
	}
	
	char *wbuf = "www.allwinnertech.com";
	wlen = fat_write(fd, wbuf, strlen(wbuf));
	printf("[app](%s) fat_write wlen: %d\n", __func__, wlen);
	char rbuf[128] = {0};
	fat_lseek(fd, 0);
	rlen = fat_read(fd, rbuf, 128);
	printf("[app](%s) fat_read rlen: %d, rbuf: (%s)\n\n", __func__, rlen, rbuf);
	fat_close(fd);
}

void help(void)
{
	puts("Test-Num definition: 0-scan card");
	puts("                     --------------------------------- [ read/write test ] ---------------------------------");
	puts("                     single thread read/write  -> 1-test_fatfs_write;    2-test_fatfs_read;    3-test_vfs_write;    4-test_vfs_read;    5-test_directio_write;    6-test_directio_read(abort);");
	puts("                     double threads read/write -> 7-test_DoubleThreadsWriteByFatFs;     8-test_DoubleThreadsReadByFatFs;     9-test_DoubleThreadsWriteByVFS;     a-test_DoubleThreadsReadByVFS;");
	puts("                                                  b-test_DoubleThreadsWriteByDirectIO;  c-test_DoubleThreadsReadByDirectIO;  d-test_DoubleProcessesWriteByFatFs; e-test_fatfs_read_by_hand;\n");
	puts("                     --------------------------------------- [ help ] --------------------------------------");
	puts("                     help info   ->  q-quit the program;    h-help;\n");
	puts("                     ------------------------------------ [ other test ] -----------------------------------");
	puts("                     other test  ->  A-test_getCardStatus;  B-test_cardPrepare;    C-test_checkFormat;    D-test_remountCard;    E-test_getCardFsType;    F-test_getCardInfo;    G-test_mkfs;");
	puts("                                     H-test_chmodByHand;    I-test_getWritingFileSize;       J-test_getWritingFileSizeByHand;    K-test_openFileTwice;    L-test_writeAndRead;");
	puts("                                     O-write card by DirectIO until card is full, then make statistical analysis;\n");
}

void test_getCardFsType(void)
{
	printf("func: %s\n", __func__);
	puts("description: get card's fs-type(1-fat12; 2-fat16; 3-fat32; 4-others)");
	printf("[app](%s) card's fs-type: %d\n\n", __func__, fat_card_fstype());
}

void test_remountCard(void)
{
	printf("func: %s\n", __func__);
	puts("description: remount card");
	printf("[app](%s) fat_card_remount res: %d\n\n", __func__, fat_card_remount());
}

void test_checkFormat(void)
{
	int ret;
	printf("func: %s\n", __func__);
	puts("description: check if need to format card or not, 0-not need formt, 1-need format");
	ret = fat_card_format();
	printf("[app](%s) fat_card_format res: %d\n", __func__, ret);
	if (ret) {
		printf("[app](%s) card need to be formatted!\n\n", __func__);
	} else {
		printf("[app](%s) card NOT need to be formatted\n\n", __func__);
	}
}

void test_cardPrepare(void)
{
	int ret;
	printf("func: %s\n", __func__);
	puts("description: parepare before read/write");
	ret = fat_card_prepare();
	printf("[app](%s) fat_card_prepare res: %d\n", __func__, ret);
	switch (ret) {
		case 0:
			puts("\tEverything is ok, you can read or write\n");
			break;
		case 1:
			printf("\tCard NOT exist!\n\n");
			break;
		case 2:
			printf("\tCard NOT fatfs!\n\n");
			break;
		case 3:
			printf("\tCard NOT 64KB/cluster!\n\n");
			break;
		default:
			puts("\tunknown retval!\n\n");
			break;
	}
}

void test_chmodByHand(void)
{
	int ret;
	FAT_FILINFO info;
	char file[64];
	printf("func: %s\n", __func__);
	puts("description: input file and wanted attribution to set");

	printf("input file's path:  ");
	gets(file);
	ret = fat_stat(file, &info);
	if (0 == ret) {
		printf("[app](%s) %s -> fattrib: %#x\n", __FUNCTION__, file, info.fattrib);
	} else {
		printf("[app](%s) %s fail, ret: [%d]\n\n", __FUNCTION__, file, ret);
		return;
	}

	ret = fat_chmod(file, AM_RDO, AM_RDO);
	if (0 == ret) {
		printf("[app](%s) set attrib(READONLY) sucess, ready to query...\n", __FUNCTION__);
		ret = fat_stat(file, &info);
		if (0 == ret) {
			printf("[app](%s) %s -> fattrib: %#x\n", __FUNCTION__, file, info.fattrib);
		} else {
			printf("[app](%s) %s fail, ret: [%d]\n\n", __FUNCTION__, file, ret);
			return;
		}
	} else {
		printf("[app](%s) set attrib fail!\n\n", __FUNCTION__);
	}
	
}

void test_writeUntilFull(void)
{
	int i, j;
	int timeBegin, timeCost;
	pthread_t tid1, tid2;
	struct statfs buf;
	char path1[64] = {0};
	char path2[64] = {0};
	int pathLen;
	int speed[2][64];
	void* speedptr;
	
	printf("func: %s\n", __func__);
	puts("description: two threads write card by directio until it is full, then make statistical analysis!");

	statfs("/mnt/extsd", &buf);
	printf("[statfs()] f_type: %#x, f_bsize: %u, f_blocks: %llu, f_bfree: %llu\n\n", buf.f_type, buf.f_bsize, buf.f_blocks, buf.f_bfree);

	strcpy(path1, "/mnt/extsd/test_writeUnitilFull_Thread1_file");
	strcpy(path2, "/mnt/extsd/test_writeUnitilFull_Thread2_file");

	pathLen = strlen(path1);
	timeBegin = iclock32();
	for (i = 0; ;i++) {
		statfs("/mnt/extsd", &buf);
		if (buf.f_bfree < 16 * 1024) {
			printf("[statfs()] f_type: %u, f_bsize: %u, f_blocks: %llu, f_bfree: %llu\n\n", buf.f_type, buf.f_bsize, buf.f_blocks, buf.f_bfree);
			printf("\n\nCard is full, make statistical analysis...\n\n");
			break;
		}
		printf("\n-----------------double threads write card with 512MB by directio %dth test-----------------------\n", i+1);
		sprintf(path1 + pathLen, "%02d", i);
		sprintf(path2 + pathLen, "%02d", i);
		printf("path1: %s\npath2: %s\n\n", path1, path2);
		pthread_create(&tid1, NULL, test_directio_write, path1);
		pthread_create(&tid2, NULL, test_directio_write, path2);

		pthread_join(tid1, &speedptr);
		speed[0][i] = (int)speedptr;
		pthread_join(tid2, &speedptr);
		speed[1][i] = (int)speedptr;
		if (speed[0][i] == -1)
			break;
		printf("write speed[0][%d]: %d KB/s, speed[1][%d]: %d KB/s\n", i, speed[0][i], i, speed[1][i]);
	}
	puts("\n\n=============== end of write by directio ===================\n\n");
	printf("\n\n[statistic]: write file number each thread: %d\n", i);
	printf("             file size: 512 MB\n");
	printf("             time for write until card is full: %d\n", (timeCost = iclock32() - timeBegin));
	printf("             average write-speed for double threads: %lld KB/s\n\n", (long long)2*i*512*1024*1000/timeCost);
	for (j = 0; j < i; j++) {
		printf("             thread1_file%02d write-speed: %d\n", j, speed[0][j]);
		printf("             thread2_file%02d write-speed: %d\n", j, speed[1][j]);
	}
	puts("\n==================== end of test ============================");	
}

void test_mkfs(void)
{
	int res;
	
	printf("func: %s\n", __func__);
	puts("description: formate the card with fat32 file-system");
	
	res = fat_mkfs("0:");
	printf("[app](%s) fat_mkfs res: %d\n\n", __func__, res);
}


int main(int argc, char **argv)
{
	char cmd = 0;
#if 0
	int i = atoi(argv[1]);
	if(i == 1)
	{
		 test_fatfs_write("0:test_fatfs_write_1");
	}
	else
	{
		 test_fatfs_write("0:test_fatfs_write_2");
	}
#else
	help();
	char buf[32];
	do {
		printf("Input Test-Num:");
		gets(buf);
		switch(*buf) {
		case '0':
			scan_files("0:");
			break;
		case '1':
			test_fatfs_write("0:test_fatfs_write");
			break;
		case '2':
			test_fatfs_read("0:test_fatfs_write");
			break;
		case '3':
			test_vfs_write("/mnt/extsd/test_vfs_write");
			break;
		case '4':
			test_vfs_read("/mnt/extsd/test_vfs_write");
			break;
		case '5':
			test_directio_write("/mnt/extsd/test_directio_write");
			break;
		case '6':
			test_directio_read("/mnt/extsd/test_directio_write");
			break;
		case '7':
			test_DoubleThreadsWriteByFatFs();
			break;
		case '8':
			test_DoubleThreadsReadByFatFs();
			break;
		case '9':
			test_DoubleThreadsWriteByVFS();
			break;
		case 'a':
			test_DoubleThreadsReadByVFS();
			break;
		case 'b':
			test_DoubleThreadsWriteByDirectIO();
			break;
		case 'c':
			test_DoubleThreadsReadByDirectIO();
			break;
		case 'd':
			test_DoubleProcessesWriteByFatFs();
			break;
		case 'e':
			test_fatfs_read_by_hand();
			break;
		case 'h':
			help();
		case 'q':
			return 0;
		case 'A':
			test_getCardStatus();
			break;
		case 'B':
			test_cardPrepare();
			break;
		case 'C':
			test_checkFormat();
			break;
		case 'D':
			test_remountCard();
			break;
		case 'E':
			test_getCardFsType();
			break;
		case 'F':
			test_getCardInfo();
			break;
		case 'G':
			test_mkfs();
			break;
		case 'H':
			test_chmodByHand();
			break;
		case 'I':
			test_getWritingFileSize();
			break;
		case 'J':
			test_getWritingFileSizeByHand();
			break;
		case 'K':
			test_openFileTwice("0:test_openFileTwice");
			break;
		case 'L':
			test_writeAndRead("0:test_writeAndRead");
			break;
		case 'O':
			test_writeUntilFull();
			break;
		default:
			printf("error command [%s]!\n\n", buf);
			help();
			break;
		}

	}
	while(1);

#endif
    return 0;
}

