//#define _GNU_SOURCE //与O_DIRECT有关

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <fcntl.h>
#include <limits.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fat_user.h>
#include <errno.h>
#include "cardtest.h"
#include "debug.h"

#define _DETAILS	1

using namespace std;

#define MAX_PATH_NAME	255
#define SECTOR_SHIFT	9
#define SECTOR_SIZE		512


#define K (1024)
#define M (1024 * K)
#define G (1024 * M)

#define OP_WRITE	1
#define OP_READ		2
#define OP_VERIFY	4



#define OPT_RECURSIVE 1
#define OPT_FORCE     2


static int aw_seed = 0x691e5a0f;
static void aw_srand(int seed)
{
	aw_seed = seed;
}

static short aw_rand(int *seed)
{
	int RandMax  = 0x7fff;
	int tmp;

	tmp = (*seed) * 0x000343FD;
	tmp += 0x00269EC3;
	*seed = tmp;
	tmp >>= 16;
	tmp &= RandMax;
	return (short)tmp;
}

static int aw_rand_int(int from, int to)
{
	return from + aw_rand(&aw_seed) % (to - from + 1);
}

static int rand_int(int from, int to)
{
//	srand(time(NULL));
	return from + rand() % (to - from + 1);
}

bool getData(char *dataPtr, int sector, int nsect, int seed)
{
	unsigned char *buf = (unsigned char *)dataPtr;
	int count;
#if _DETAILS && 0
	db_msg("[getData]sector: %d, nsect: %d\n", sector, nsect);
#endif
	count = nsect * SECTOR_SIZE;
	aw_srand(sector/nsect + seed);
	for(int i = 0; i < count; ++i){
		buf[i] = (unsigned char)aw_rand_int(0, 255);
	}
//	printf("sector: %d buf: %02x %02x %02x %02x\n", sector, buf[0], buf[1], buf[2], buf[3] );
	return true;
}

bool checkData(char *dataPtr, int sector, int nsect, int seed)
{
	unsigned char *buf = (unsigned char *)dataPtr;
	int count;
	unsigned char rd_byte;
	count = nsect * SECTOR_SIZE;

#if _DETAILS && 0
	db_msg("[checkData]sector: %d, nsect: %d\n", sector, nsect);
#endif

	aw_srand(sector/nsect + seed);
	for(int i = 0; i < count; ++i){
		rd_byte = (unsigned char)aw_rand_int(0, 255);
		if(buf[i] != rd_byte){
			int idx = i - i % 4;
			db_msg("====>[RandData::checkData]Data not right in sector: %d offset: %d  %02x %02x %02x %02x(%d != %d),seed = %d\n",
				sector + (i >> SECTOR_SHIFT), i % SECTOR_SIZE, buf[idx], buf[idx + 1], buf[idx + 2], buf[idx + 3], buf[i], rd_byte, seed);
			return false;
		}
	}
	return true;
}


/* return -1 on failure, with errno set to the first error */
static int unlink_recursive(const char* name, int flags)
{
    struct stat st;
    DIR *dir;
    struct dirent *de;
    int fail = 0;
	
    /* is it a file or directory? */
    if (lstat(name, &st) < 0)
        return ((flags & OPT_FORCE) && errno == ENOENT) ? 0 : -1;

    /* a file, so unlink it */
    if (!S_ISDIR(st.st_mode))
        return unlink(name);

    /* a directory, so open handle */
    dir = opendir(name);
    if (dir == NULL)
        return -1;

    /* recurse over components */
    errno = 0;
    while ((de = readdir(dir)) != NULL) {
        char dn[PATH_MAX];
        if (!strcmp(de->d_name, "..") || !strcmp(de->d_name, "."))
            continue;
        sprintf(dn, "%s/%s", name, de->d_name);
        if (unlink_recursive(dn, flags) < 0) {
            if (!(flags & OPT_FORCE)) {
                fail = 1;
                break;
            }
        }
        errno = 0;
    }
    /* in case readdir or unlink_recursive failed */
    if (fail || errno < 0) {
        int save = errno;
        closedir(dir);
        errno = save;
        return -1;
    }

    /* close directory handle */
    if (closedir(dir) < 0)
        return -1;

    /* delete target directory */
    return rmdir(name);
}

static int open_file(char *fpath, int rw, long long fsize)
{
	int fd;
	char *data;
	struct stat st;
	int open_flags;
	FAT_FILINFO Finfo;
	open_flags = O_RDWR | O_DIRECT;

	if(rw == OP_WRITE)
		open_flags |= O_CREAT;

#if _DETAILS
	db_msg("opening %s for %s!\n",
				fpath, (rw == OP_WRITE) ? "write" : "read");
#endif
	#ifdef FATFS
	fd = fat_open(fpath, FA_WRITE | FA_OPEN_ALWAYS | FA_READ);
	#else
	fd = open(fpath, open_flags, 0777);
	#endif
	if (fd <= 0)
	{
		db_msg("====>Error: open %s failed! %s(%d) \n",
				fpath, strerror(errno), errno);
		return fd;
	}

	if(OP_READ == rw){
		#ifdef FATFS
			fat_stat(fpath,&Finfo);
			if(Finfo.fsize < fsize)
			{
				db_msg("====>Error: file size is not big enough, read test abort!...\n");
				fat_close(fd);
				return -1;
			}
			fat_lseek(fd,0);
		#else
			fstat(fd, &st);
			if(st.st_size < fsize)
			{
				db_msg("====>Error: file size is not big enough, read test abort!...\n");
				close(fd);
				return -1;
		}
		#endif
	}
	return fd;
}

static bool sequence_io(int fd, int rw, int psize, long long fsize)
{
	int ret = true;
	int res;
	struct timeval tpstart,tpend;
	long long docount;
	long long done_size = 0;

	char *data;

	res = posix_memalign((void **)&data, 4096, psize);

	if(res < 0){
			db_msg("====>Error: can't alloc memeory! %s(%d)\n", strerror(errno), errno);
			return false;
	}
	while(done_size < fsize){
		long long dsz = fsize - done_size;
		dsz = (dsz < psize) ? dsz : psize;

		if(rw & OP_WRITE){
			//获取要写的数据
			getData(data, done_size >> SECTOR_SHIFT, psize >> SECTOR_SHIFT, 0);
			#ifdef FATFS
				docount = fat_write(fd, data, dsz);
			#else
				docount = write(fd, data, dsz);
			#endif
			
		}else{
			#ifdef FATFS
				docount = fat_read(fd, data, dsz);
			#else
				docount = read(fd, data, dsz);	
			#endif
		}	
		
		if(docount != dsz){
			db_msg("====>Warning: %s count not equal data_size! offset = %lld\n", (rw == OP_WRITE) ? "write" : "read", done_size);
		}

		if (docount <= 0){
			db_msg("====>Error: %s failed! %s(%d)\n", (rw == OP_WRITE) ? "write" : "read", strerror(errno), errno);
			ret = false;
			break;
		}

		if(rw & OP_VERIFY){
			//校验数据
			if(!checkData(data, done_size >> SECTOR_SHIFT, dsz >> SECTOR_SHIFT, 0)){
				ret = false;
				break;
			}
		}

		done_size += docount;
    }

	free(data);
    return ret;
}

static double Get_IO_Speed(char *fpath, int rw, int psize, long long fsize)
{
	bool res;
	int fd;
	struct timeval tpstart,tpend;
	long long timeuse;

	double iospeed;


	fd = open_file(fpath, rw, fsize);
	if(fd < 0){
		db_msg("-----%s,%d-----",__FUNCTION__,__LINE__);
		return -1.0;
	}

	gettimeofday(&tpstart,NULL);

	res = sequence_io(fd, rw, psize, fsize);
	if(!res){
		db_msg("-----%s,%d-----",__FUNCTION__,__LINE__);
		iospeed = -1.0;
		goto exit;
	}

	gettimeofday(&tpend,NULL);

	timeuse = 1000ULL * 1000 * (tpend.tv_sec-tpstart.tv_sec) + (tpend.tv_usec-tpstart.tv_usec);

	iospeed = fsize / K / ((double)timeuse / 1000.0 / 1000);

exit:
	#ifdef  FATFS
		fat_close(fd);
	#else
		close(fd);
	#endif
	
	return iospeed;
}

bool Get_Card_Speed(char *dirpath, struct TestResult *test_result, struct TestInfo *test_info, int *status)
{
	bool ret = true;
	char fpath[MAX_PATH_NAME];
	int cycle;
	int psize;
	long long fsize;
	double rspeed, wspeed;
	FAT_FILINFO Finfo;
	int i;
	snprintf(fpath, MAX_PATH_NAME, "%scard_speed_test.tmp", dirpath);
	#ifdef FATFS
		int res = fat_stat(fpath, &Finfo);
		if (res < 0 || Finfo.fsize == 0) {
			db_msg("fail to get %s'size, size is %d,res :%d\n",fpath, Finfo.fsize,res);
			Finfo.fsize = 0;
			
		}
		 fat_unlink(fpath);
	#else
		if(access(fpath, F_OK) == 0){
			unlink(fpath);
		}
	#endif
	if(test_info != NULL){
		fsize = test_info->mFileSize;
		psize = test_info->mPackageSize;
		cycle = test_info->mCycles;
	}else{
		fsize = 20 * M;
		psize = 64 * K;
		cycle = 1;
	}
	for(i = 0; i < cycle; ++i){
		wspeed = Get_IO_Speed(fpath, OP_WRITE, psize, fsize);
		if(wspeed < 0.0){
			db_msg("-----%s,%d-----",__FUNCTION__,__LINE__);
			ret = false;
			break;
		}
		test_result->mWriteSpeed += wspeed;
		db_msg("[%s] cycle %d, write_speed = %12.2lfKB/s, read_speed = %12.2lfKB/s\n", __FUNCTION__, cycle, wspeed, rspeed);
		
		rspeed = Get_IO_Speed(fpath, OP_READ, psize, fsize);
		if(rspeed < 0.0){
			db_msg("-----%s,%d-----",__FUNCTION__,__LINE__);
			ret = false;
			break;
		}
#ifdef FATFS
		 fat_unlink(fpath);
#else
		remove(fpath);
#endif

		db_msg("[%s] cycle %d, write_speed = %12.2lfKB/s, read_speed = %12.2lfKB/s\n", __FUNCTION__, cycle, wspeed, rspeed);

		
		test_result->mReadSpeed += rspeed;
	}
	test_result->mWriteSpeed /= cycle;
	test_result->mReadSpeed /= cycle;
	db_msg("test_result->mWriteSpeed  = %12.2lfKB/s;test_result->mReadSpeed =  %12.2lfKB/s \n",test_result->mWriteSpeed,test_result->mReadSpeed);

	return ret;
}


static long long get_free_space(char *path)
{
	struct statfs diskInfo;
	long long blocksize;
	long long availsize = 0;

	if(statfs(path, &diskInfo) < 0){
		db_msg("====>Error: statfs %s failed! %s(%d)\n",
			path, strerror(errno), errno);
		return -1;
	}
	if(strncmp(path, "/dev/", 5) == 0){
#if _DETAILS
		db_msg("Target is block devfile!\n");
#endif
		return 0;
	}
	blocksize = diskInfo.f_bsize;
	availsize = blocksize * diskInfo.f_bavail;

#if _DETAILS
	long long totalsize = blocksize * diskInfo.f_blocks;
	long long freesize = blocksize * diskInfo.f_bfree;
	db_msg("[get_fs_space] blocksize %lld, totalsize %lld, freesize %lld, availsize %lld\n",
			blocksize, totalsize, freesize, availsize);
#endif
	return availsize;
}

static bool WriteCheck(char *fpath, int psize, long long fsize)
{
	bool ret = true;
	int fd;

	fd = open_file(fpath, OP_WRITE, fsize);
	if(fd < 0){
		return false;
	}
	ret = sequence_io(fd, OP_WRITE, psize, fsize);
	if(!ret && errno != ENOSPC){
		close(fd);
		return false;
	}

	if(errno == ENOSPC){
		if(get_free_space(fpath) != 0){
			db_msg("====>Error: write failed!\n");
			close(fd);
			return false;
		}
		fsize = lseek64(fd, 0, SEEK_END);
		db_msg("No space left, file size is %lld KB!\n", fsize / K);
	}

	close(fd);

	fd = open_file(fpath, OP_WRITE, fsize);
	if(fd < 0){
		return false;
	}
	ret = sequence_io(fd, OP_READ | OP_VERIFY, psize, fsize);
	if(!ret){
		close(fd);
		return false;
	}
	close(fd);

	return ret;
}

#define CHECK_FILE_SIZE		(100 * M)
#define CHECK_PKG_SIZE		(4 * K)
#define LAST_SIZE		(CHECK_FILE_SIZE + CHECK_FILE_SIZE / 2)

bool Check_Card_Ok(char *dirpath, int *status)
{
	bool ret = true;
	int count = 0;
	char test_dir[MAX_PATH_NAME];
	char fpath[MAX_PATH_NAME];
	long long free_space;
	long long fsize = CHECK_FILE_SIZE;
	int psize = CHECK_PKG_SIZE;

	snprintf(test_dir, MAX_PATH_NAME, "%s/card_check_test", dirpath);

	if(access(test_dir, F_OK) == 0){
		unlink_recursive(test_dir, OPT_FORCE);
	}

	mkdir(test_dir, 0777);

	while(1){
		snprintf(fpath, MAX_PATH_NAME, "%s/card_check_test_%d.tmp", test_dir, count);
		ret = WriteCheck(fpath, psize, fsize);

		if(!ret){
			db_msg("====>Error: WriteCheck %s Failed!\n", fpath);
			break;
		}
#if _DETAILS
		else
			db_msg("WriteCheck %s Succeed!\n", fpath);
#endif
		count++;
		if(get_free_space(test_dir) == 0){
			break;
		}
	}

	if(get_free_space(test_dir) != 0){
		db_msg("====>Warning: Checking is not covered in the whole disk!\n");
	}
#if _DETAILS
	db_msg("WriteCheck completely Succeed!\n");
#endif

	unlink_recursive(test_dir, OPT_FORCE);
	return ret;
}