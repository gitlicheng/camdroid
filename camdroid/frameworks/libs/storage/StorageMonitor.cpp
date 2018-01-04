/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_NDEBUG 0
#define LOG_TAG "StorageMonitor"
#include <utils/Log.h>

#include "include_storage/StorageMonitor.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <sys/un.h>  

#define UNIX_DOMAIN 		"/dev/socket/vold"
#define MMC_DEVPATH 		"/dev/block/mmcblk0"
#define MMC_BLKPATH 		"/dev/block/vold/179:0"


#define CMD_MOUNT 			"11 volume mount /mnt/extsd"
#define CMD_UNMOUNT 		"12 volume unmount /mnt/extsd force"
#define CMD_SHARE 			"13 volume share /mnt/extsd ums"
#define CMD_UNSHARE 		"14 volume unshare /mnt/extsd ums"
#define CMD_FORMAT 			"15 volume format /mnt/extsd"

using namespace std;



int isMounted(const char *checkPath)
{
#if 1
	const char *path = "/proc/mounts";
	FILE *fp;
	char line[255];
	const char *delim = " \t";
	ALOGV(" isMount checkPath=%s \n",checkPath);
	if (!(fp = fopen(path, "r"))) {
		ALOGV(" isMount fopen fail,path=%s\n",path);
		return 0;
	}
	memset(line,'\0',sizeof(line));
	while(fgets(line, sizeof(line), fp))
	{
		char *save_ptr = NULL;
		char *p        = NULL;
		//ALOGV(" isMount line=%s \n",line);
		if (line[0] != '/' || strncmp("/dev/block/vold",line,strlen("/dev/block/vold")) != 0)
		{
			memset(line,'\0',sizeof(line));
			continue;
		}       
		if ((p = strtok_r(line, delim, &save_ptr)) != NULL) {		    			
			if ((p = strtok_r(NULL, delim, &save_ptr)) != NULL){
				ALOGV(" isMount p=%s \n",p);
				if(strncmp(checkPath,p,strlen(checkPath) ) == 0){
					fclose(fp);
					ALOGV(" isMount return 1\n");
					return 1;				
				}				
			}					
		}		
	}//end while(fgets(line, sizeof(line), fp))	
	if(fp){
		fclose(fp);
	}
#endif	
	return 0;
}

StorageMonitor::StorageMonitor() :
	mShared(false),
	mSharing(false),
	mInserted(false),
	mMounted(false),
	mFormated(false),
	mFormating(false),
	mStatus(0),
	mConnectFD(0),
    mUpdateThread(NULL),
    mListener(NULL)	{
	ALOGD("StorageMonitor Initialize");
}

StorageMonitor::~StorageMonitor() {	
	ALOGD("~StorageMonitor Destructor");
}

void StorageMonitor::processStr(char *src)
{
    int index;
	char *save_ptr;
	const char *delim = " \t";
	char *path = NULL;
	char *status = NULL;
	if (src == NULL) {
		return ;
	}
	ALOGE("33333333aaaaaaaaaaaa src=%s",src);
	if (!strtok_r(src, delim, &save_ptr)) {
		ALOGW("Error processStr");
		return ;
	}		
	index = 3;
	while(index--){
		if (!(path = strtok_r(NULL, delim, &save_ptr))) {
			ALOGW("Error parsing path");
			return ;
		}		
	}
	memset(mPath,0,sizeof(mPath));
	strcpy(mPath,path);
	index = 7;
	//ALOGD("Error parsing save_ptr=%s",save_ptr);
	while(index--){
		if (!(status = strtok_r(NULL, delim, &save_ptr))) {
			ALOGW("Error parsing status");
			return ;
		}		
	}
	mStatus = atoi(status);
}

int StorageMonitor::updateLoop(void) {
	int ret = -1;
	char snd_buf[512];
	memset(snd_buf, 0, sizeof(snd_buf));
	int i = 0;
//	ret = read(mConnectFD, snd_buf, sizeof(snd_buf)-1); 
	while (1) {
		ret = read(mConnectFD, &snd_buf[i], 1);
		if (snd_buf[i] == '\0') {
			break;
		} else if (ret < 0) {
			ALOGE("read data error ret = %d \n", ret);
			return 0;
		}
		i++;
	} 
    // ret = recv( mConnectFD, snd_buf,sizeof(snd_buf)-1, 0);
	if(ret < 0) {
		ALOGE("read data error ret = %d \n", ret);
        usleep(10*1024);
		return 0;
	}
	int oStatus = mStatus;
	mStatus = -1;
	ALOGV("StorageMonitor::updateLoop read snd_buf(ret:%d): ", strlen(snd_buf));
	processStr(snd_buf);
	ALOGV("line=%d, notify: mStatus = %d, mPath = %s\n", __LINE__,mStatus, mPath);
	if(mListener != NULL) {
		switch(mStatus) {
			case STORAGE_STATUS_NO_MEDIA: {
				mInserted = false;
			}
			break;
			case STORAGE_STATUS_IDLE: {
				ALOGD("line=%d,mShared=%d, mSharing = %d, mFormated=%d, mFormating = %d, mInserted = %d",__LINE__, mShared, mSharing, mFormated, mFormating, mInserted);
				if(mFormated == true) {
					if(mFormating == false) {
						mFormating = true;
						write(mConnectFD, CMD_FORMAT, sizeof(CMD_FORMAT));
					} else {
						mFormated = false;
						mFormating = false;
						write(mConnectFD, CMD_MOUNT, sizeof(CMD_MOUNT));
					}					
				} else {
                    if(STORAGE_STATUS_IDLE == oStatus || STORAGE_STATUS_CHECKING == oStatus){
                        ALOGE("checking sdcard fail!");
                    }else{
    					if(mShared == true) {
    						    if(STORAGE_STATUS_REMOVE == oStatus){
                                    break;
                                }
    						    write(mConnectFD, CMD_SHARE, sizeof(CMD_SHARE));				
    					} else {//
    						if(mInserted == false) {
                                if(STORAGE_STATUS_NO_MEDIA == oStatus){//正常插入TF卡
                                    write(mConnectFD, CMD_MOUNT, sizeof(CMD_MOUNT));
                                }else {//正常拔出卡
                                    ALOGD("1111oStatus=%d",oStatus);
                                    break;
                                }
    						}else{
                                if(STORAGE_STATUS_SHARED == oStatus || STORAGE_STATUS_PENDING == oStatus){
                                    write(mConnectFD, CMD_MOUNT, sizeof(CMD_MOUNT));
                                }else if(STORAGE_STATUS_UNMOUNTING == oStatus){
                                    ALOGE("line=%d,Status=%d",__LINE__,oStatus);//不会走这里
                                    write(mConnectFD, CMD_SHARE, sizeof(CMD_SHARE));
                                }else{
                                    ALOGE("line=%d,oStatus=%d",__LINE__,oStatus);
                                }
    						}						
    					}                    
                    }//end if(STORAGE_STATUS_IDLE == oStatus
				}
			}
			break;
			case STORAGE_STATUS_PENDING: {
				mInserted = true;
			}
			break;
			case STORAGE_STATUS_CHECKING: {			    
				mInserted = true;
			}
			break;
			case STORAGE_STATUS_MOUNTED: {
				mMounted = true;
				mInserted = true;
				if(mShared == true) {
					mShared = false;
				}
			}
			break;
			case STORAGE_STATUS_UNMOUNTING: {
				mMounted = false;
			}
			break;
			case STORAGE_STATUS_SHARED: {
			    ALOGD("-11111111---mShared=%d, mSharing = %d", mShared, mSharing);
			    mSharing = true;
				mInserted = true;
			}
			break;
			case STORAGE_STATUS_REMOVE: {
				mInserted = false;
			}
			break;			
			case STORAGE_STATUS_FORMATTING: {
			
			}
			break;
		}
		ALOGD("call mListener->notify, mStatus = %d, mInserted = %d, mPath = %s.\n", mStatus, mInserted, mPath);
		if(mStatus != -1){
            mListener->notify(0, mStatus, mPath);
		}else{
        	mStatus = oStatus;
		}
		
	}
	usleep(10*1024);
	ALOGD("updateLoop finished\n");
    return 0;
}

int StorageMonitor::init(void) {
	int ret = -1;
    static struct sockaddr_un srv_addr;  
	srv_addr.sun_family = AF_UNIX;   
	strcpy(srv_addr.sun_path, UNIX_DOMAIN);	
	
	mConnectFD = socket(PF_UNIX,SOCK_STREAM, 0);   
	if(mConnectFD < 0) {      
		return -1;   
	} else {
		ALOGV("mConnectFD = %d .\n", mConnectFD);
		ret = connect(mConnectFD, (struct sockaddr*)&srv_addr, sizeof(srv_addr));   
		if(ret == -1) {
			ALOGE("  while connect erro 3  \n");
			close(mConnectFD);   
			return -1;   
		}	
		if(0 == access(MMC_DEVPATH, F_OK)) {
		    mInserted = true;
			//write(mConnectFD, CMD_MOUNT, sizeof(CMD_MOUNT));
		}
#if 1		
		if(1 == isMounted("/mnt/extsd")) {
			mMounted = true;
		}
#endif		
		ALOGD("--dddddd-----------------mInserted = %d, mMounted = %d.\n", mInserted, mMounted);
		if(mUpdateThread == NULL) {
			mUpdateThread = new UpdateThread(this);
			mUpdateThread->startThread();
		}
	}
	
	return 0;
}

int StorageMonitor::exit(void) {
    ALOGV("line=%d,exit start",__LINE__);
	if(mConnectFD > 0) {
		close(mConnectFD);
		mConnectFD = -1;
	}    
	if(mUpdateThread != NULL) {
		mUpdateThread->stopThread();
		mUpdateThread.clear();
		mUpdateThread = NULL;
	}
    ALOGV("line=%d,exit end",__LINE__);
	return 0;
}

int StorageMonitor::setMonitorListener(StorageMonitorListener * listener) {
	mListener = listener;
	return 0;
}

int StorageMonitor::mountToPC() {
	int ret = 0;
    
	if (mShared == true) {
		return 0;
	}
	if (mConnectFD < 0) {
		return mConnectFD;
	}
	mShared = true;
	ALOGD("mountToPC start");
	ret = write(mConnectFD, CMD_UNMOUNT, sizeof(CMD_UNMOUNT));
	if (ret < 0) {
		return ret;
	}
	write(mConnectFD, CMD_SHARE, sizeof(CMD_SHARE));
	return ret;
}

int StorageMonitor::unmountFromPC() {
	int ret = 0;

	if (mShared == false) {
		return 0;
	}
	if (mConnectFD < 0) {
		return mConnectFD;
	}
	ALOGD("unmountFromPC start");
	mShared = false;
	ret = write(mConnectFD, CMD_UNSHARE, sizeof(CMD_UNSHARE));
	if (ret < 0) {
		return ret;
	}
	write(mConnectFD, CMD_MOUNT, sizeof(CMD_MOUNT));
	return ret;
}

int StorageMonitor::formatSDcard() {
	int ret = 0;

	if (mInserted == false) {
		return -1;
	}
	if (mConnectFD < 0) {
		return mConnectFD;
	}
	ALOGD("formatSDcard start,mMounted=%d",mMounted);
	mFormated = true;
	if (mMounted == true) {
		ret = write(mConnectFD, CMD_UNMOUNT, sizeof(CMD_UNMOUNT));
		if (ret < 0) {
			return ret;
		}
	} else {
		mFormating = true;
		ret = write(mConnectFD, CMD_FORMAT, sizeof(CMD_FORMAT));
		if (ret < 0) {
			return ret;
		}
	}
	return 0;
}

bool StorageMonitor::isInsert() {
	return mInserted;
}

bool StorageMonitor::isMount() {
	return mMounted;
}

bool StorageMonitor::isShare() {
	return mShared;
}



bool StorageMonitor::isFormated() {
       return mFormating || mFormated;
}
