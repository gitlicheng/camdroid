/*
 * Copyright (C) 2008 The Android Open Source Project
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
#define LOG_TAG "SensorBase"
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <stdlib.h>
#include <cutils/log.h>

#include <linux/input.h>

#include "SensorBase.h"
SensorBase::SensorBase(const char* dev_name, const char* data_name)
        : dev_name(dev_name), data_name(data_name),
        dev_fd(-1),
        data_fd(-1)
{

        if (data_name) {
                data_fd = openInput(data_name);
        }
}

SensorBase::~SensorBase() {
        if (data_fd >= 0) {
                close(data_fd);
        }
    
        if (dev_fd >= 0) {
                close(dev_fd);
        }
}

int SensorBase::open_device() {
        if (dev_fd < 0 && dev_name) {
                dev_fd = open(dev_name, O_RDONLY);
                ALOGE_IF(dev_fd<0, "Couldn't open %s (%s)", dev_name, strerror(errno));
        }
        
        return 0;
}

int SensorBase::close_device() {
        if (dev_fd >= 0) {
                close(dev_fd);
                dev_fd = -1;
        }
        
        return 0;
}
int SensorBase::getFd() const 
{    
	if (!data_name) {        
	        return dev_fd;    
	}
	   
	return data_fd;
}
int SensorBase::setEnable(int32_t handle, int enabled) {
	return 0;
}
int SensorBase::getEnable(int32_t handle) {
	return 0;
}

int SensorBase::setDelay(int32_t handle, int64_t ns) {
        return 0;
}

bool SensorBase::hasPendingEvents() const {
	return false;
}
void  processEvent(int code, int value) {

}

int64_t SensorBase::getTimestamp() {
        struct timespec t;
        
        t.tv_sec = t.tv_nsec = 0;
        clock_gettime(CLOCK_MONOTONIC, &t);
        
        return int64_t(t.tv_sec)*1000000000LL + t.tv_nsec;
}
int SensorBase::set_sysfs_input_attr(char *class_path,const char *attr, char *value, int len)
{
	char path[256];
	int fd;

	if (class_path == NULL || *class_path == '\0'
	    || attr == NULL || value == NULL || len < 1) {
		return -EINVAL;
	}
	
	snprintf(path, sizeof(path), "%s/%s", class_path, attr);
	path[sizeof(path) - 1] = '\0';
	fd = open(path, O_RDWR);
	
	if (fd < 0) {
		ALOGD("Could not open (write-only) SysFs attribute \"%s\" (%s).", attr, strerror(errno));
		close(fd);
		return -errno;
	}
	
	if (write(fd, value, len) < 0) {  
	        ALOGD("path:%s", path);     
	        ALOGD("Could not write SysFs attribute \"%s\" (%s).", attr, strerror(errno));
		close(fd);
		return -errno;
	}
	
	close(fd);

	return 0;
}

int SensorBase::openInput(const char* inputName) {
        int fd = -1;
	int input_id = -1;
        const char *dirname = "/dev/input";
	const char *inputsysfs = "/sys/class/input";
        char devname[PATH_MAX];
        char *filename;
        DIR *dir;
        struct dirent *de;

        dir = opendir(dirname);
        if(dir == NULL)
                return -1;
                
        strcpy(devname, dirname);
        filename = devname + strlen(devname);
        *filename++ = '/';
        
        while((de = readdir(dir))) {
                if(de->d_name[0] == '.' &&
                        (de->d_name[1] == '\0' ||
                        (de->d_name[1] == '.' && de->d_name[2] == '\0')))
                        continue;
                        
                strcpy(filename, de->d_name);
                fd = open(devname, O_RDONLY);

                if (fd>=0) {
                        char name[80];
                
                        if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
                                name[0] = '\0';
                        }
//
//                        ALOGD("name is %s \n", name);
//		        ALOGD("inputName is %s \n", inputName);	
                        if (!strcmp(name, inputName)) {
		
                                break;
                        } else {
                                close(fd);
                                fd = -1;
                        }
                }
        }
        
        closedir(dir);
#ifdef DEBUG_IF
        ALOGE_IF(fd<0, "couldn't find '%s' input device", inputName);
#endif
        
        return fd;
}
int SensorBase::readEvents(sensors_event_t* data, int count) {
  	return 0;
}



