#define LOG_NDEBUG 0
#define LOG_TAG "OtaUpDate"

#ifdef FATFS
#include <fat_user.h>
#endif
#include "OtaUpdate.h"
#include <unistd.h>

#define UBOOT_LEN                  (256 << 10)
#ifdef tiger_cdr
#define BOOT_KERNEL_LEN            (0x280000)
#define ROOTFS_SYSTEM_LEN          (0x470000)
#else                                                     
#define BOOT_KERNEL_LEN            ( 5120 * 512 )  
#define ROOTFS_SYSTEM_LEN          ( 9088 * 512 ) 
#endif
#define CFG_LEN                    (0x80000)
#define BOOTLOGO_LEN               (0x20000)
#define SHUTDOWNLOGOG_LEN          (0x20000)
#define ENV_LEN                    (0x10000)
#define PRIVATE_LEN                (0x10000)
#ifdef FATFS
static inline int fatFileExist(const char *path)
{
	int fd = fat_open(path, FA_OPEN_EXISTING);
	if (fd >= 0) {
		fat_close(fd);
		return 1;
	}
	return 0;
}
#define UPDATE_FILE_PATH			"0:/full_img.fex"
#define FILE_EXSIST(PATH)			(fatFileExist(PATH))
#else
#define UPDATE_FILE_PATH			"/mnt/extsd/full_img.fex"
#define FILE_EXSIST(PATH)			(!access(PATH, F_OK))
#endif
#define BOOT0_MAIGC					"eGON.BT0"

#define BUFFER_LEN					8

#ifdef tiger_cdr
#define FILE_BUFFER					(8<<20)
#else
#define FILE_BUFFER					(14<<20)
#endif

enum {
	UBOOT,
	BOOT,
	ROOTFS,
	BOOTLOGO,
	ENV,
	PRIVATE,
	FULL_IMAGE,
};

FILE * fp;
int  flash_handle = 0;

HWND MainWnd;

static void *watchDogThread(void *p_ctx) {
	int fd = open("/dev/watchdog", O_RDWR);
	ALOGV("watchDogThread  fd=%d..............",fd);
	if(fd < 0) {
		ALOGE("Watch dog device open error.");
		return NULL;
	}
	
	ALOGV("Before WDIOC_SETOPTIONS disable...............");
	int watchdog_enable = WDIOS_DISABLECARD;
	int io_ret = ioctl(fd,WDIOC_SETOPTIONS, &watchdog_enable);
	ALOGV("After WDIOC_SETOPTIONS disable, ret = %d", io_ret);
	int time_out = 10;	//8s, min:1, max:16
	io_ret = ioctl(fd, WDIOC_SETTIMEOUT, &time_out);
	ALOGV("WDIOC_SETTIMEOUT, ret = %d, time_out = %d", io_ret, time_out);
	io_ret = ioctl(fd,WDIOC_GETTIMEOUT, &time_out);
	ALOGV("WDIOC_GETTIMEOUT, ret = %d, time_out = %d", io_ret, time_out);
	
    watchdog_enable = WDIOS_ENABLECARD;
	io_ret = ioctl(fd, WDIOC_SETOPTIONS, &watchdog_enable);
	ALOGV("WDIOC_SETOPTIONS enable, ret = %d", io_ret);
	
	while(1) {
		ioctl(fd, WDIOC_KEEPALIVE, NULL);
		//write(fd,NULL,1);
		usleep(3000000);
	}
}

static int watchDogRun() {
	pthread_t thread_id = 0;
	int err = pthread_create(&thread_id, NULL, watchDogThread, NULL);
	if(err || !thread_id) {
		ALOGE("Create watch dog thread error.");
		return -1;
	}
	
	ALOGV("Create watch dog thread success.");
	return 0;
}

static int sd_upgrade(unsigned char mode,unsigned char *buf ,unsigned int len)
{
	ALOGE("20150525*****%s,%d****",__FUNCTION__,__LINE__);
	unsigned char *upgrade_buf = NULL;
	int offset = 0;
	int ret_len = 0;
	upgrade_buf = (unsigned char *)malloc(FILE_BUFFER);
	unsigned char *tmp_buf ;
	const char *destTem;
	destTem = BOOT0_MAIGC;
	int update_flag = 1;
	int ret;	
	if(NULL == upgrade_buf) {
		ALOGE("open upgrade_buf malloc fail\n");
		return -1;
	}
	memset(upgrade_buf,0,len);
	ALOGE("*****%s,%d****",__FUNCTION__,__LINE__);	
	if(!FILE_EXSIST(UPDATE_FILE_PATH)){
		ALOGE("%s not exist", UPDATE_FILE_PATH);
		free(upgrade_buf);
		return -1;
		
	}
#ifdef FATFS	
	int fp = fat_open(UPDATE_FILE_PATH, FA_READ);
#else
	int fp = open(UPDATE_FILE_PATH,O_RDONLY);
#endif
	if(fp < 0) {
		ALOGE("open fp fail,erro=%s\n",strerror(errno));
		free(upgrade_buf);
		return -1;
	}
	ALOGE("*****%s,%d****\n",__FUNCTION__,__LINE__);
#ifdef FATFS
	//int ret = fat_read(fp,upgrade_buf,len);
	unsigned char * tmp = upgrade_buf;
	while(1) {
	    ret = fat_read(fp,tmp,FILE_BUFFER);
		//ALOGD("*****%d***ret=%d*\n",__LINE__,ret);
	    if (ret == FAT_BUFFER_SIZE)
	    {
			tmp += FAT_BUFFER_SIZE;
	        continue;
	    } else if (ret < 0) {
	        ALOGD ("Reading error, ret %d", ret);
			fat_close(fp);	
			free(upgrade_buf);
			return -1;
		}
		break;
	}
#else
	ret = read(fp,upgrade_buf,len);
#endif
	ALOGD("--------%d***ret=%d*\n",__LINE__,ret);

#ifndef FATFS
	if(ret <= 0){
		close(fp);
		free(upgrade_buf);
		return -1;
	}
#endif
	
	tmp_buf = upgrade_buf;
	tmp_buf = tmp_buf+4;
	ALOGE("[debug_jaosn]:THIS IS FOR TEST line:%d",__LINE__);
	ALOGE("[debug_jaosn]:the boot0 magic len is :%d\n",strlen(BOOT0_MAIGC));
	for(int j = 0; j < strlen(BOOT0_MAIGC); j++)
	{
			
		if(*tmp_buf == *destTem)
		{
				printf("%c",*tmp_buf);
				tmp_buf++;
				destTem++;				
		}else
		{
				printf("[debug_jaosn]:this is a Illegal full_img.fex file\n");
				printf("[debug_jaosn]:tmp_buf= %c;destTem=%c\n",*tmp_buf,*destTem);
				update_flag = 0;
				break;
		}
	}
	
#ifdef FATFS
	fat_close(fp);	
#else
	close(fp);
#endif
	if(update_flag != 1){
		ALOGD("***full_img.fex error!***");
		return -1;
	}
	
	ALOGE("*****%s,%d****\n",__FUNCTION__,__LINE__);
	
    flash_handle = open("/dev/block/mtdblock0",O_RDWR); 
	if(flash_handle < 0) {
		ALOGE("open flash_handle fail\n");
		free(upgrade_buf);
		return -1;
	}
	ret_len = write(flash_handle,upgrade_buf,UBOOT_LEN);
	ALOGE("line=%d,ret_len=%d,;len=%d",__LINE__,ret_len,UBOOT_LEN);
	fdatasync(flash_handle);
	close(flash_handle);
	if(ret_len != UBOOT_LEN){
    	ALOGE("line=%d,ret_len=%d,erro=%s",__LINE__,ret_len,strerror(errno));
		free(upgrade_buf);
		return -1;		
	}

	offset = UBOOT_LEN;
	ALOGE(" upgrade uboot waiting...,ret_len = 0x%x\n",ret_len);
	ALOGE("*****%s,%d****",__FUNCTION__,__LINE__);
	flash_handle = open("/dev/block/mtdblock1",O_RDWR); 
	if(flash_handle < 0) {
		ALOGE("line=%d,open flash_handle fail erro=%s",__LINE__,strerror(errno));
		free(upgrade_buf);
		return -1;
	}
	ret_len = write(flash_handle,upgrade_buf + offset,BOOT_KERNEL_LEN);
	fdatasync(flash_handle);
	close(flash_handle);
	if(ret_len != BOOT_KERNEL_LEN){
    	ALOGE("line=%d,ret_len=%d,erro=%s",__LINE__,ret_len,strerror(errno));
		free(upgrade_buf);
		return -1;		
	}	
	
    ALOGE(" upgrade kernel waiting...,ret_len = %d\n",ret_len);
	ALOGE("*****%s,%d****",__FUNCTION__,__LINE__);
		
	offset += BOOT_KERNEL_LEN;
	flash_handle = open("/dev/block/mtdblock2",O_RDWR); 
	if(flash_handle < 0) {
		ALOGE("line=%d,open flash_handle fail erro=%s",__LINE__,strerror(errno));
		free(upgrade_buf);
		return -1;
	}
	
	ret_len = write(flash_handle,upgrade_buf + offset,ROOTFS_SYSTEM_LEN);
	fdatasync(flash_handle);
	close(flash_handle);
	if(ret_len != ROOTFS_SYSTEM_LEN){
    	ALOGE("ROOTFS_SYSTEM_LEN=%d,erro=%s",ret_len,strerror(errno));
		free(upgrade_buf);
		return -1;
	}	
	ALOGE("upgrade rootfs waiting...,ret_len = %d\n",ret_len);
	ALOGE("*****%s,%d****",__FUNCTION__,__LINE__);

    offset += ROOTFS_SYSTEM_LEN ;
	flash_handle = open("/dev/block/mtdblock3",O_RDWR );
	if(flash_handle < 0){
    	ALOGE("ROOTFS_SYSTEM_LEN=%d,erro=%s",ret_len,strerror(errno));
		free(upgrade_buf);
		return -1;		
	}
	ret_len = write(flash_handle,upgrade_buf+offset,CFG_LEN);
	fdatasync(flash_handle);
	close(flash_handle);
	if(ret_len != CFG_LEN){
    	ALOGE("line=%d,ret_len=%d,erro=%s",__LINE__,ret_len,strerror(errno));
		return -1;
	}
	ALOGD("*****%s,%d*ret_len=%d***",__FUNCTION__,__LINE__,ret_len);
	ALOGE("upgrade cfg waiting,ret_len %d\n",ret_len);

    offset += CFG_LEN ;
	flash_handle = open("/dev/block/mtdblock4",O_RDWR);
	if(flash_handle < 0){
    	ALOGE("line=%d,erro=%s",__LINE__,strerror(errno));
		free(upgrade_buf);
		return -1;		
	}	
	ret_len = write(flash_handle,upgrade_buf+offset,BOOTLOGO_LEN);
	fdatasync(flash_handle);
	close(flash_handle);
	if(ret_len != BOOTLOGO_LEN){
    	ALOGE("line=%d,ret_len=%d,erro=%s",__LINE__,ret_len,strerror(errno));
		free(upgrade_buf);
		return -1;
	}	
	ALOGD("*****%s,%d*ret_len=%d***",__FUNCTION__,__LINE__,ret_len);
	ALOGE(" upgrade bootlogo waiting ...,ret_len = %d\n",ret_len);

	offset += BOOTLOGO_LEN ;
	flash_handle = open("/dev/block/mtdblock5",O_RDWR);
	if(flash_handle < 0){
    	ALOGE("line=%d,erro=%s",__LINE__,strerror(errno));
		free(upgrade_buf);
		return -1;		
	}	
	ret_len = write(flash_handle,upgrade_buf+offset,SHUTDOWNLOGOG_LEN);
	fdatasync(flash_handle);
	close(flash_handle);
	if(ret_len != SHUTDOWNLOGOG_LEN){
    	ALOGE("line=%d,ret_len=%d,erro=%s",__LINE__,ret_len,strerror(errno));
		free(upgrade_buf);
		return -1;
	}	
	ALOGD("*****%s,%d*ret_len=%d***",__FUNCTION__,__LINE__,ret_len);
	ALOGE("test upgrade BOOTLOGO_LEN,ret_len = %d\n",ret_len);
	
	offset += SHUTDOWNLOGOG_LEN;
	
	flash_handle = open("/dev/block/mtdblock6",O_RDWR); 
	if(flash_handle < 0){
    	ALOGE("line=%d,erro=%s",__LINE__,strerror(errno));
		free(upgrade_buf);
		return -1;		
	}	
	ret_len = write(flash_handle,upgrade_buf+offset,ENV_LEN);
	fdatasync(flash_handle);
	close(flash_handle);
	if(ret_len != ENV_LEN){
    	ALOGE("line=%d,ret_len=%d,erro=%s",__LINE__,ret_len,strerror(errno));
		free(upgrade_buf);
		return -1;		
	}
	
	ALOGE("*****%s,%d****",__FUNCTION__,__LINE__);
#if  0 // ndef tiger_cdr
	offset += ENV_LEN ;
	flash_handle = open("/dev/block/mtdblock7",O_RDWR);
	if(flash_handle < 0){
    	ALOGE("line=%d,erro=%s",__LINE__,strerror(errno));
		free(upgrade_buf);
		return -1;		
	}	
	
	ret_len = write(flash_handle,upgrade_buf+offset,PRIVATE_LEN);
	sync();
	close(flash_handle);
	if(ret_len != PRIVATE_LEN){
    	ALOGE("line=%d,ret_len=%d,erro=%s",__LINE__,ret_len,strerror(errno));
		free(upgrade_buf);
		return -1;		
	}	
	ALOGE("test upgrade PRIVATE_LEN,ret_len = %d\n",ret_len);
#endif
	free(upgrade_buf);
	return 0;
}




static void *otaUpdateThread(void *ptx)
{
	ALOGE("******in otaupdate*****");
	watchDogRun();
	char ip_buf[20];
    int i = -2;
	int ip =0,ip2;
	char ipaddr[32];
    char gateway[32];
    uint32_t prefixLength;
    char dns1[32];
    char dns2[32];
    char server[32];
    uint32_t lease;
    char vendorInfo[32];
	int ret;
	int len = 0x500000;

	ret = sd_upgrade(FULL_IMAGE,NULL,FILE_BUFFER);
    if (ret < 0) {
		ALOGE("warning upgrade fail exit");
	   	SendMessage(MainWnd,MSG_UPDATE_OVER,0,0);
		kill(getpid(),SIGKILL);
    }
	ALOGE(" 20150525 waiting *****-----------------****");
    sleep(120);
	ALOGE("****%d**out otaupdate*****",__LINE__);
   	SendMessage(MainWnd,MSG_UPDATE_OVER,1,0);
	kill(getpid(),SIGKILL);
    return NULL;
}


OtaUpdate::OtaUpdate(HWND hwnd){
	mHwnd = hwnd;
}


int OtaUpdate::startUpdate()
{
	MainWnd = mHwnd;
	pthread_t thread_id = 0;
	int err = pthread_create(&thread_id, NULL, otaUpdateThread, NULL);
	if(err || !thread_id) {
		ALOGE("Create OtaUpdate  thread error.");
		return -1;
	}
	ALOGV("Create OtaUpdate thread success.");
	return 0;
}




