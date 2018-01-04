#define LOG_NDEBUG 0
#define LOG_TAG "ff_user"
#include <utils/Log.h>

#include "fat_user.h"			/* Declarations of FatFs API */

//#include <sys/vfs.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#define ioctl_f_open		100
#define ioctl_f_close		101
#define ioctl_f_read  		102
#define ioctl_f_write 		103
#define ioctl_f_lseek 		104
#define ioctl_f_truncate 	105
#define ioctl_f_sync  		106
#define ioctl_f_opendir  	107	
#define ioctl_f_closedir 	108
#define ioctl_f_readdir  	109
#define ioctl_f_mkdir  		110
#define ioctl_f_unlink 		111
#define ioctl_f_rename 		112
#define ioctl_f_stat  		113
#define ioctl_f_chmod  		114
#define ioctl_f_utime  		115
#define ioctl_f_chdir  		116
#define ioctl_f_getfree   	117	// get card's capacity (total and free blocks)
#define ioctl_f_getlabel 	118
#define ioctl_f_setlabel 	119
#define ioctl_f_mkfs  		120
#define ioctl_f_tell		121
#define ioctl_f_size		122

#define ioctl_card_status	200	// get card's status that exists or not
#define ioctl_card_fstype	201	// get card's filesystem-type
#define ioctl_card_prepare	202 // prepare the filesystem before read/write by fatfs

#define ioctl_card_remount	220	// remount card when switch from vfs
#define ioctl_card_format	221	// check if need to format card

#define MAX_LIST_SIZE		4

static volatile int g_driver_fd = -1;
static volatile int g_driver_fd_cnt = 0;

void* fat_sync_all(void*);
int fat_init()
{
 	if(g_driver_fd != -1) {
		//fprintf(stderr, "/dev/misc_fatfs has opend! g_driver_fd: %d, pid: %d, tid: %d\n", g_driver_fd, getpid(), gettid());
		int ret = fat_card_prepare();
		if (ret == 0) {
			return 0;
		} else {
			printf("[fatfs.so](warning) fat_card_prepare fail, ret: [%d]!\n", ret);
			return -1;
		}
	}
	
	int needInitFlag = __sync_bool_compare_and_swap(&g_driver_fd_cnt, 0, 1);
	if(needInitFlag) {
		if(g_driver_fd != -1) {
		//fprintf(stderr, "fatal error! g_driver_fd[%d] must be -1!\n", g_driver_fd);
		ALOGE("(f:%s, l:%d) fatal error! g_driver_fd[%d] must be -1!", __FUNCTION__, __LINE__, g_driver_fd);
		return 0;
		}
		
		int fd = open("/dev/misc_fatfs", O_RDONLY);
		if (fd < 0) {
			//printf("open /dev/misc_fatfs failed:%s\n", strerror(errno));
			ALOGE("(f:%s, l:%d) fatal error! open /dev/misc_fatfs failed:%s", __FUNCTION__, __LINE__, strerror(errno));
			return -1;
		}
		g_driver_fd = fd;
		pthread_t syncTid;
		pthread_create(&syncTid, NULL, fat_sync_all, NULL);
		//printf("open /dev/misc_fatfs ok! g_driver_fd: %d, pid: %d, tid: %d\n", fd, getpid(), gettid());
		ALOGD("(f:%s, l:%d) open /dev/misc_fatfs ok! g_driver_fd: %d, pid: %d, tid: %d", __FUNCTION__, __LINE__, fd, getpid(), gettid());
		return 0;
	} else { 
		while(-1 == g_driver_fd) {
			//fprintf(stderr, "(f:%s, l:%d) wait /dev/misc_fatfs init...\n", __FUNCTION__, __LINE__);
			ALOGD("(f:%s, l:%d) wait /dev/misc_fatfs init...", __FUNCTION__, __LINE__);
			usleep(10*1000);
		}
		return 0;
	}
}

int fat_deinit()
{
    if(-1 == g_driver_fd)
    {
        return 0;
    }
    int needDestroyFlag = __sync_bool_compare_and_swap(&g_driver_fd_cnt, 1, 0);
    if(needDestroyFlag)
    {
        if(-1 == g_driver_fd)
        {
            fprintf(stderr, "(f:%s, l:%d) fatal error! g_driver_fd should not be -1\n", __FUNCTION__, __LINE__);
            return 0;
        }
		close(g_driver_fd);
		g_driver_fd = -1;
        return 0;
    }
    else
    {
        while(g_driver_fd!=-1)
        {
            fprintf(stderr, "(f:%s, l:%d) wait /dev/misc_fatfs close...\n", __FUNCTION__, __LINE__);
            usleep(10*1000);
        }
        return 0;
    }
}

/* Open or create a file */
int fat_open (const char* path, BYTE mode)
{
	int fd = 0;
	FAT_OPEN_ARGS args;
	args.mode = mode;

	fat_init();
	strncpy(args.path, path, sizeof(args.path));

	fd = ioctl(g_driver_fd, ioctl_f_open, &args);
	return fd;
}

/* Close an open file object */
int fat_close (int fd)
{
	fat_init();
	return ioctl(g_driver_fd, ioctl_f_close, fd);
}

/* Read data from a file */
int fat_read (int fd, void* buff, UINT btr)			
{
	FAT_BUFFER fat_buffer;
	int len = 0;
	fat_init();	
	if(btr > FAT_BUFFER_SIZE)
		len = FAT_BUFFER_SIZE;
	else
		len = btr;
	
	memset(&fat_buffer, 0, sizeof(FAT_BUFFER ));
	fat_buffer.len = len;
	fat_buffer.fd = fd;
	
	int rlen = ioctl(g_driver_fd, ioctl_f_read, &fat_buffer);

	if(rlen > 0)
		memcpy(buff, &fat_buffer, rlen);
	
	return rlen;
}
/* Write data to a file */
int fat_write (int fd, const void* buff, UINT btw)
{
	FAT_BUFFER fat_buffer;
	int len = 0;
	fat_init();
	if(btw > FAT_BUFFER_SIZE)
		len = FAT_BUFFER_SIZE;
	else
		len = btw;
	
	memset(&fat_buffer, 0, sizeof(FAT_BUFFER ));
	memcpy(fat_buffer.buf, buff, len);
	fat_buffer.len = len;
	fat_buffer.fd = fd;
	
	return ioctl(g_driver_fd, ioctl_f_write, &fat_buffer);
}

/* Move file pointer of a file object */
int fat_lseek (int fd, DWORD ofs)			
{
	FAT_SEEK fat_seek;
	fat_seek.fd = fd;
	fat_seek.ofs = ofs;
	fat_init();
	
	return ioctl(g_driver_fd, ioctl_f_lseek, &fat_seek);
}

/* Truncate file */
int fat_truncate (int fd)					
{
	fat_init();

	return ioctl(g_driver_fd, ioctl_f_truncate, fd);
}

/* Flush cached data of a writing file */
int fat_sync (int fd)						
{
	fat_init();

	return ioctl(g_driver_fd, ioctl_f_sync, fd);
}

/* Flush cached data to renew card's cluster table */
void* fat_sync_all (void* argv)						
{
//	fat_init();
	int res;
	sleep(5);

	while (1) {
		if (fat_card_prepare() == 0) {
			res = ioctl(g_driver_fd, ioctl_f_sync, MAX_LIST_SIZE);	// 0-MAX_LIST_SIZE: sync the real opened-file; MAX_LIST_SIZE: sync all opened-files
			if (res != 0)
				printf("[fatfs.so] fat_sync_all error: [%d]\n", res);
		}
		sleep(1);						// sync all opened-files per second
	}

	return NULL;
}

/* Open a directory */
int fat_opendir (const char* path)	
{
	fat_init();
	return ioctl(g_driver_fd, ioctl_f_opendir, path);
}

/* Close an open directory */
int fat_closedir (int fd)			
{
	fat_init();
	return ioctl(g_driver_fd, ioctl_f_closedir, fd);
}

/* Read a directory item */
int fat_readdir (int fd, FAT_FILINFO* fno)	
{
	static FAT_FILEINFO_FD fileinfofd;
	int ret = 0;
	fat_init();
	
	fileinfofd.fd = fd;
	
	ret = ioctl(g_driver_fd, ioctl_f_readdir, &fileinfofd);

	memcpy(fno, &fileinfofd.fileinfo, sizeof(FAT_FILINFO));
	return ret;
}

/* Create a sub directory */
int fat_mkdir (const char* path)	
{
	char path_buf[128] = {0};
	strncpy(path_buf, path, 128);
	fat_init();
	return  ioctl(g_driver_fd, ioctl_f_mkdir, path_buf);
}

/* Delete an existing file or directory */
int fat_unlink (const char* path)								
{
	char path_buf[128] = {0};
	strncpy(path_buf, path, 128);
	fat_init();
	return  ioctl(g_driver_fd, ioctl_f_unlink, path_buf);
}

/* Rename/Move a file or directory */
int fat_rename (const char* path_old, const char* path_new)	
{
	char two_path[256] = {0};
	strncpy(two_path, path_old, 128);
	strncpy(two_path+128, path_new, 128);
	fat_init();
	return  ioctl(g_driver_fd, ioctl_f_rename, two_path);
}

/* Get file status */
int fat_stat (const char* path, FAT_FILINFO* fno)					
{
	FAT_FILEINFO_FD fileinfofd;
	int ret = 0;
	fat_init();
	strncpy(fileinfofd.path, path, 128);
	ret = ioctl(g_driver_fd, ioctl_f_stat, &fileinfofd);
	memcpy(fno, &fileinfofd.fileinfo, sizeof(FAT_FILINFO));
	return ret;
}

/* Change attribute of the file/dir */
int fat_chmod (const char* path, BYTE attr, BYTE mask)			
{
	FAT_FILE_ATTR  file_attr;
	int ret = 0;
	fat_init();
	
	strncpy(file_attr.path, path, 128);
	file_attr.attr = attr;
	file_attr.mask = mask;
	ret = ioctl(g_driver_fd, ioctl_f_chmod, &file_attr);

	return ret;
}

/* Change current directory */
int fat_chdir (const char* path)								
{
	fat_init();
	return  ioctl(g_driver_fd, ioctl_f_chdir, path);
}

/* Format the card with fatfs + 64KB/cluster */
int fat_mkfs (const char* path)								
{
	int ret;
	fat_init();
	ret = ioctl(g_driver_fd, ioctl_f_mkfs, path);
	if (ret == 0) {
		return  ioctl(g_driver_fd, ioctl_card_remount, 0);
	} else {
		printf("[fatfs.so](warning) can NOT format the card!\n");
		return -1;
	}
}
	
/* Additional API for app */
long long fat_tell(int fd)
{
	FAT_FILE_LEN file_pos;
	fat_init();

	file_pos.fd = fd;
	ioctl(g_driver_fd, ioctl_f_tell, &file_pos);
    
	return file_pos.len;
}

long long fat_size(int fd)
{
	FAT_FILE_LEN file_size;
	fat_init();

	file_size.fd = fd;
	ioctl(g_driver_fd, ioctl_f_size, &file_size);

	return file_size.len;
}

// int fat_get_card_status(void) -> int fat_card_status(void)
int fat_card_status(void)
{
	fat_init();
	return ioctl(g_driver_fd, ioctl_card_status, 0);
}

// @return:	0 - everything is ok;
//			1 - card NOT exist;
//			2 - card NOT fatfs;
//			3 - card NOT 64KB/cluster;
int fat_card_prepare(void)
{
//	fat_init();
	if (g_driver_fd == -1) {
		fat_init();
	}
	return ioctl(g_driver_fd, ioctl_card_prepare, 0);
}

// int fat_get_card_fstype(void) -> int fat_card_fstype(void)
int fat_card_fstype(void)
{
	fat_init();
	ioctl(g_driver_fd, ioctl_card_remount, 0);
	return ioctl(g_driver_fd, ioctl_card_fstype, 0);
}

int fat_statfs(const char* path, FAT_STATFS *buf)
{
	fat_init();
	if (fat_card_status() == 1)
		return ioctl(g_driver_fd, ioctl_f_getfree, buf);
	else {
		memset(buf, 0, sizeof(FAT_STATFS));
		return -1;
	}
}

int fat_card_remount(void)
{
	fat_init();
	return ioctl(g_driver_fd, ioctl_card_remount, 0);
}

/* Judge if need to format card by cluster-size size and fs-type */
/* return value: 0-not need to format; 1-must be formatted */
int fat_card_format(void)
{
	fat_init();
	return ioctl(g_driver_fd, ioctl_card_format, 0);
}

int fat_allocate(int fd, int mode, int offset, int len)
{
	printf("[Warning] this api(%s) has NOT been implemented!\n", __func__);
	return -1;
}


