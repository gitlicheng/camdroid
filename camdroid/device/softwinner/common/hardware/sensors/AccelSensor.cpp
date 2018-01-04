/*
 * Copyright (C) 2012 Freescale Semiconductor Inc.
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
#define LOG_TAG "AccelSensors"
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <dlfcn.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include "AccelSensor.h"

#define ACC_DATA_NAME    gsensorInfo.sensorName
#define ACC_SYSFS_PATH   "/sys/class/input"
#define ACC_SYSFS_DELAY  "delay"
#define ACC_SYSFS_ENABLE "enable"
#define ACC_EVENT_X ABS_X
#define ACC_EVENT_Y ABS_Y
#define ACC_EVENT_Z ABS_Z

AccelSensor::AccelSensor()
        : SensorBase(NULL, ACC_DATA_NAME),
        mEnabled(0),
        mPendingMask(0),
        convert(0.0),
        direct_x(0),
        direct_y(0), 
        direct_z(0), 
        direct_xy(-1),
        mInputReader(16),
        mDelay(0)
{
#ifdef DEBUG_SENSOR
        ALOGD("sensorName:%s,classPath:%s,lsg:%f\n",
                gsensorInfo.sensorName, gsensorInfo.classPath, gsensorInfo.priData);
#endif
        if (strlen(gsensorInfo.sensorName)) {
                if(! gsensor_cfg())
                        ALOGE("gsensor config error!\n"); 
        }
        
        memset(&mPendingEvent, 0, sizeof(mPendingEvent));
        memset(&mAccData, 0, sizeof(mAccData));
	
        mPendingEvent.version = sizeof(sensors_event_t);
        mPendingEvent.sensor = ID_A;
        mPendingEvent.type = SENSOR_TYPE_ACCELEROMETER;
        mPendingEvent.acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;
        
	mUser = 0;
	
#ifdef DEBUG_SENSOR	
	ALOGD("%s:data_fd:%d\n", __func__, data_fd);
#endif	
	
	if(!strcmp(ACC_DATA_NAME, "lsm303d_acc")) {
	        sprintf(gsensorInfo.classPath, "%s/%s/%s", gsensorInfo.classPath, 
	                "device", "accelerometer");
	        ALOGD("gsensorInfo.classPath:%s", gsensorInfo.classPath);
	}
}

AccelSensor::~AccelSensor() {
        
}

char* AccelSensor::get_cfg_value(char *buf) {
        int j = 0;
        int k = 0; 
        char* val;
        
        val = strtok(buf, "=");
        if (val != NULL){
                val = strtok(NULL, " \n\r\t");
        }
        buf = val;
        
        return buf;
}

int AccelSensor::gsensor_cfg()
{
        FILE *fp;
        int name_match = 0;
        char buf[128] = {0};
        char * val;
        
        if((fp = fopen(GSENSOR_CONFIG_PATH, "rb")) == NULL) {
                ALOGD("can't not open file!\n");
                return 0;
        }
        
        while(fgets(buf, LINE_LENGTH, fp))
        {
                if (!strncmp(buf, GSENSOR_NAME, strlen(GSENSOR_NAME))) {
                        val = get_cfg_value(buf);
                        #ifdef DEBUG_SENSOR
                                ALOGD("val:%s\n",val);
                        #endif
                        name_match = (strncmp(val, gsensorInfo.sensorName, strlen(gsensorInfo.sensorName))) ? 0 : 1;
                                
                        if (name_match)  {
                                convert = (GRAVITY_EARTH/gsensorInfo.priData);
                                #ifdef DEBUG_SENSOR
                                        ALOGD("lsg: %f,convert:%f", gsensorInfo.priData, convert);
                                #endif
                                memset(&buf, 0, sizeof(buf));
                                continue;
                        } 
                        
                }  
                
                if(name_match ==0){
                        memset(&buf, 0, sizeof(buf));
                        continue;
                }else if(name_match < 5){
                        name_match++;
                        val = get_cfg_value(buf); 
                        #ifdef DEBUG_SENSOR
                                ALOGD("val:%s\n", val);
                        #endif
                        
                       if (!strncmp(buf,GSENSOR_DIRECTX, strlen(GSENSOR_DIRECTX))){                                
                                  direct_x = (strncmp(val, TRUE,strlen(val))) ? convert * (-1) : convert;      
                       }
                       
                       if (!strncmp(buf, GSENSOR_DIRECTY, strlen(GSENSOR_DIRECTY))){                                         
                                  direct_y =(strncmp(val, TRUE,strlen(val))) ? convert * (-1) : convert;      
                       }
                       
                      if (!strncmp(buf, GSENSOR_DIRECTZ, strlen(GSENSOR_DIRECTZ))){
                                 direct_z =(strncmp(val, TRUE,strlen(val))) ? convert * (-1) : convert; 
                       }
                       
                       if (!strncmp(buf,GSENSOR_XY, strlen(GSENSOR_XY))){
                                 direct_xy = (strncmp(val, TRUE,strlen(val))) ? 0 : 1; 
                       }
                       
                
                }else{
                        name_match = 0;
                        break;
                }
                memset(&buf, 0, sizeof(buf));
        }
        
    char property[PROPERTY_VALUE_MAX];
    property_get("ro.sf.rotation", property, 0);
    switch (atoi(property)) {
        case 90:
            direct_y = (-1) * direct_y;
            direct_xy = (direct_xy == 1) ? 0 : 1;
            break;
        case 180:
            direct_x = (-1) * direct_x;
            direct_y = (-1) * direct_y;
            break;
        case 270:
            direct_x = (-1) * direct_x;
            direct_xy = (direct_xy == 1) ? 0 : 1;
            break;
    }
        #ifdef DEBUG_SENSOR
                ALOGD("direct_x: %f,direct_y: %f,direct_z: %f,direct_xy:%d,sensor_name:%s \n",
                        direct_x, direct_y, direct_z, direct_xy, gsensorInfo.sensorName);
        #endif
        
        if((direct_x == 0) || (direct_y == 0) || (direct_z == 0) || (direct_xy == (-1)) || (convert == 0.0)) {
                return 0;
        }
        
        fclose(fp);
        return 1;
    
}

int AccelSensor::setEnable(int32_t handle, int en) {
	int err = 0;
        
	//ALOGD("enable:  handle:  %ld, en: %d", handle, en);
	if(handle != ID_A && handle != ID_O && handle != ID_M)
		return -1;
		
	if(en)
		mUser++;
	else{
		mUser--;
		if(mUser < 0)
			mUser = 0;
	}

	if(mUser > 0)
		err = enable_sensor();
	else
		err = disable_sensor();
		
	if(handle == ID_A ) {
		if(en)
         	        mEnabled++;
		else
			mEnabled--;
		if(mEnabled < 0)
			mEnabled = 0;
        }
        
	//update_delay();
#ifdef DEBUG_SENSOR
	ALOGD("AccelSensor enable %d ,usercount %d, handle %d ,mEnabled %d, err %d", 
	        en, mUser, handle, mEnabled, err);
#endif

        return 0;
}

int AccelSensor::setDelay(int32_t handle, int64_t ns) {
      // ALOGD("delay:  handle:  %ld, ns: %lld", handle, ns);
        if (ns < 0)
                return -EINVAL;
                
#ifdef DEBUG_SENSOR                
        ALOGD("%s: ns = %lld", __func__, ns);
#endif
        mDelay = ns;
        
        return update_delay();
}

int AccelSensor::update_delay() {
        return set_delay(mDelay);
}

int AccelSensor::readEvents(sensors_event_t* data, int count) {
        if (count < 1)
                return -EINVAL;

        ssize_t n = mInputReader.fill(data_fd);
        if (n < 0)
                return n;

        int numEventReceived = 0;
        input_event const* event;
        
        while (count && mInputReader.readEvent(&event)) {
                int type = event->type;
                
                if ((type == EV_ABS) || (type == EV_REL) || (type == EV_KEY)) {
                        processEvent(event->code, event->value);
                        mInputReader.next();
                } else if (type == EV_SYN) {
                        int64_t time = timevalToNano(event->time);
                        
			if (mPendingMask) {
				mPendingMask = 0;
				mPendingEvent.timestamp = time;
				
				if (mEnabled) {
					*data++ = mPendingEvent;
					mAccData = mPendingEvent;
					count--;
					numEventReceived++;
				}				
			}
			
                        if (!mPendingMask) {
                                mInputReader.next();
                        }
                        
                } else {
                        ALOGE("AccelSensor: unknown event (type=%d, code=%d)",
                                type, event->code);
                        mInputReader.next();
                }
        }

        return numEventReceived;
}

void AccelSensor::processEvent(int code, int value) {

        switch (code) {
                case ACC_EVENT_X :
                        mPendingMask = 1;
                        
                        if(direct_xy) {
                                mPendingEvent.acceleration.y= value * direct_y;
                        }else {
                                mPendingEvent.acceleration.x = value * direct_x;
                        }
                        
                        break;

                case ACC_EVENT_Y :
                        mPendingMask = 1;
                        
                        if(direct_xy) {
                                mPendingEvent.acceleration.x = value * direct_x;
                        }else {
                                mPendingEvent.acceleration.y = value * direct_y;
                        }
                        		
                        break;

                case ACC_EVENT_Z :
                        mPendingMask = 1;
                        mPendingEvent.acceleration.z = value * direct_z ;
                        break;
        }
        
#ifdef DEBUG_SENSOR
        ALOGD("Sensor data:  x,y,z:  %f, %f, %f\n", mPendingEvent.acceleration.x,
						    mPendingEvent.acceleration.y,
						    mPendingEvent.acceleration.z); 
#endif	

}

int AccelSensor::writeEnable(int isEnable) {

        char buf[2];  
        int err = -1 ;     
        
	if(gsensorInfo.classPath[0] == ICHAR)
		return -1;
	
	int bytes = sprintf(buf, "%d", isEnable);
		
        if(!strcmp(ACC_DATA_NAME, "lsm303d_acc")) {                
                err = set_sysfs_input_attr(gsensorInfo.classPath,"enable_device",buf,bytes);        
        }else {			
	        err = set_sysfs_input_attr(gsensorInfo.classPath,"enable",buf,bytes);
        }
        
	return err;
}

int AccelSensor::writeDelay(int64_t ns) {
	if(gsensorInfo.classPath[0] == ICHAR)
		return -1;

	if (ns > 10240000000LL) {
		ns = 10240000000LL; /* maximum delay in nano second. */
	}
	if (ns < 312500LL) {
		ns = 312500LL; /* minimum delay in nano second. */
	}

        char buf[80];
        int bytes = sprintf(buf, "%lld", ns/1000 / 1000);
        
        if(!strcmp(ACC_DATA_NAME, "lsm303d_acc")) {             
                int err = set_sysfs_input_attr(gsensorInfo.classPath,"pollrate_us",buf,bytes);        
        } else {
                int err = set_sysfs_input_attr(gsensorInfo.classPath,"delay",buf,bytes);
        }

        return 0;

}

int AccelSensor::enable_sensor() {
	return writeEnable(1);
}

int AccelSensor::disable_sensor() {
	return writeEnable(0);
}

int AccelSensor::set_delay(int64_t ns) {
	return writeDelay(ns);
}

int AccelSensor::getEnable(int32_t handle) {
	return (handle == ID_A) ? mEnabled : 0;
}

/*****************************************************************************/

