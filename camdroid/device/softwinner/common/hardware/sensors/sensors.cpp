/*
 * Copyright (C) 2011 Freescale Semiconductor Inc.
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

#define LOG_TAG "Sensors"
#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>

#include <linux/input.h>

#include <utils/Atomic.h>
#include <utils/Log.h>

#include "sensors.h"

#include "AccelSensor.h"

#include <cutils/properties.h>

static int open_sensors(const struct hw_module_t* module, const char* id,
        struct hw_device_t** device);


static int sensors__get_sensors_list(struct sensors_module_t* module,
        struct sensor_t const** list)
{
    int a = 0;
    *list = sSensorList;
    a = ARRAY_SIZE(sSensorList);

#ifdef DEBUG_SENSOR
    ALOGD("sNumber:%d, a:%d\n", sNumber, a);
#endif
    return sNumber;
}

static struct hw_module_methods_t sensors_module_methods = {
open: open_sensors
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
common: {
tag: HARDWARE_MODULE_TAG,
     version_major: 1,
     version_minor: 1,
     id: SENSORS_HARDWARE_MODULE_ID,
     name: "Sensor module",
     author: "Freescale Semiconductor Inc.",
     methods: &sensors_module_methods,
     dso : NULL,
     reserved : {},
        },
get_sensors_list: sensors__get_sensors_list,
};

struct sensors_poll_context_t {
    struct sensors_poll_device_t device; // must be first

    sensors_poll_context_t();
    ~sensors_poll_context_t();
    int activate(int handle, int enabled);
    int setDelay(int handle, int64_t ns);
    int pollEvents(sensors_event_t* data, int count);

    private:
    int accel;
    int mag;
    int gyro;
    int light;
    int proximity;
    int orn;
    int temperature ;
    int press;

    static const size_t wake = 10;
    static const char WAKE_MESSAGE = 'W';
    struct pollfd mPollFds[11];
    int mWritePipeFd;
    SensorBase* mSensors[10];

    int handleToDriver(int handle) const {
        switch (handle) {
            case ID_A:
                return accel;
            case ID_M:
            case ID_O:
                return mag;
            case ID_GY:
                return gyro;
            case ID_L:
                return light;
            case ID_PX:
                return proximity;
            case ID_T:
                return temperature;
            case ID_P:
                return press;
        }
        return -EINVAL;
    }
};

/*****************************************************************************/

sensors_poll_context_t::sensors_poll_context_t()
{
    int first = -1;

	for(int i = 0; i < 10; i++) {
		mSensors[i]	= 0;
	}
    if((seStatus[ID_A].isUsed == true) && (seStatus[ID_A].isFound == true)) {
        first = first + 1;
        accel = first;
        mSensors[first] = new AccelSensor();
        mPollFds[first].fd = mSensors[accel]->getFd();
        mPollFds[first].events = POLLIN;
        mPollFds[first].revents = 0;
    }

    int wakeFds[2];
    int result = pipe(wakeFds);
    //ALOGE_IF(result<0, "error creating wake pipe (%s)", strerror(errno));
    fcntl(wakeFds[0], F_SETFL, O_NONBLOCK);
    fcntl(wakeFds[1], F_SETFL, O_NONBLOCK);
    mWritePipeFd = wakeFds[1];

    mPollFds[wake].fd = wakeFds[0];
    mPollFds[wake].events = POLLIN;
    mPollFds[wake].revents = 0;
}

sensors_poll_context_t::~sensors_poll_context_t() {
    for (int i=0 ; i < 10 ; i++) {
		if(mSensors[i] != 0)
        delete mSensors[i];
    }
    close(mPollFds[wake].fd);
    close(mWritePipeFd);
}

int sensors_poll_context_t::activate(int handle, int enabled) {

    int index = handleToDriver(handle);
    if (index < 0) return index;
    int err = 0 ;

    if(handle == ID_O || handle ==  ID_M){
        err =  mSensors[accel]->setEnable(handle, enabled);// if handle == orientaion or magnetic ,please enable ACCELERATE Sensor
        if(err)
            return err;
    }
    err |=  mSensors[index]->setEnable(handle, enabled);

    if (enabled && !err) {
        const char wakeMessage(WAKE_MESSAGE);
        int result = write(mWritePipeFd, &wakeMessage, 1);
        //ALOGE_IF(result<0, "error sending wake message (%s)", strerror(errno));
    }
    return err;
}

int sensors_poll_context_t::setDelay(int handle, int64_t ns) {

    int index = handleToDriver(handle);
    int err = -1;
    if (index < 0)
        return index;

    if(handle == ID_O || handle ==  ID_M){
        mSensors[accel]->setDelay(handle, ns);
    }

    err = mSensors[index]->setDelay(handle, ns);

    //        if(ns == 0) {
    //                mSensors[index]->setEnable(handle, 0);
    //        }else{
    //                mSensors[index]->setEnable(handle, 1);
    //        }
    //
    return err;
}

int sensors_poll_context_t::pollEvents(sensors_event_t* data, int count)
{
    int nbEvents = 0;
    int n = 0;

    do {
        // see if we have some leftover from the last poll()
        for (int i=0 ; count && (i < sNumber) ; i++) {
            SensorBase* const sensor(mSensors[i]);

#ifdef DEBUG_SENSOR
            ALOGD("count:%d, mPollFds[%d].revents:%d, sensor->hasPendingEvents():%d\n",
                    count, i, mPollFds[i].revents, sensor->hasPendingEvents());
#endif

            if ((mPollFds[i].revents & POLLIN) || (sensor->hasPendingEvents())) {

                int nb = sensor->readEvents(data, count);
                if (nb < count) {
                    // no more data for this sensor
                    mPollFds[i].revents = 0;
                }
                count -= nb;
                nbEvents += nb;
                data += nb;
            }
        }

        if (count) {
            // we still have some room, so try to see if we can get
            // some events immediately or just wait if we don't have
            // anything to return
            //n = poll(mPollFds, numFds, nbEvents ? 0 : -1);
            do {
                n = poll(mPollFds, sNumber, nbEvents ? 0 : -1);
            } while (n < 0 && errno == EINTR);

            if (n<0) {
                //ALOGE("poll() failed (%s)", strerror(errno));
                return -errno;
                        }

                        //ALOGD("wake:%d,mPollFds[wake].revents:%d\n",wake, mPollFds[wake].revents);
                        if (mPollFds[wake].revents & POLLIN) {
                                char msg;
                                int result = read(mPollFds[wake].fd, &msg, 1);
                                //ALOGE_IF(result<0, "error reading from wake pipe (%s)", strerror(errno));
                                //ALOGE_IF(msg != WAKE_MESSAGE, "unknown message on wake queue (0x%02x)", int(msg));
                                mPollFds[wake].revents = 0;
                        }
                }
        // if we have events and space, go read them
        } while (n && count);

        return nbEvents;
}

/*****************************************************************************/

static int poll__close(struct hw_device_t *dev)
{
        sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;

        if (ctx) {
                delete ctx;
        }
        return 0;
}

static int poll__activate(struct sensors_poll_device_t *dev,
        int handle, int enabled) {

        sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
        return ctx->activate(handle, enabled);
}

static int poll__setDelay(struct sensors_poll_device_t *dev,
        int handle, int64_t ns) {

        sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
        return ctx->setDelay(handle, ns);
}

static int poll__poll(struct sensors_poll_device_t *dev,
        sensors_event_t* data, int count) {

        sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
        return ctx->pollEvents(data, count);
}

/*****************************************************************************/

/** Open a new instance of a sensor device using name */
static int open_sensors(const struct hw_module_t* module, const char* id,
                        struct hw_device_t** device)
{
        int status = -EINVAL;

        insmodDevice();/*Automatic detection, loading device drivers */
        property_set("sys.sensors", "1");/*Modify the  enable and delay interface  group */
        sensorsDetect();/*detect device,filling sensor_t structure */

        sensors_poll_context_t *dev = new sensors_poll_context_t();
        memset(&dev->device, 0, sizeof(sensors_poll_device_t));

        dev->device.common.tag = HARDWARE_DEVICE_TAG;
        dev->device.common.version  = 0;
        dev->device.common.module   = const_cast<hw_module_t*>(module);
        dev->device.common.close    = poll__close;
        dev->device.activate        = poll__activate;
        dev->device.setDelay        = poll__setDelay;
        dev->device.poll            = poll__poll;

        *device = &dev->device.common;
        status = 0;

        return status;
}
