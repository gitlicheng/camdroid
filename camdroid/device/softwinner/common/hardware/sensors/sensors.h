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

#ifndef ANDROID_SENSORS_H
#define ANDROID_SENSORS_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <linux/input.h>

#include <hardware/hardware.h>
#include <hardware/sensors.h>

__BEGIN_DECLS

#define SUPPORT_SENSORS_NUMBER          (10)
#define ICHAR                           (';')
/*****************************************************************************/

#define ARRAY_SIZE(a)                   (sizeof(a) / sizeof(a[0]))

#define ID_A                            (0)
#define ID_M                            (1)
#define ID_GY                           (2)
#define ID_L                            (3)
#define ID_PX                           (4)
#define ID_O                            (5)
#define ID_T                            (6)
#define ID_P                            (7)

//#define DEBUG_SENSOR

#define HWROTATION_0                    (0)
#define HWROTATION_90                   (1)
#define HWROTATION_180                  (2)
#define HWROTATION_270                  (3)

#define CONVERT_O                       (1.0f/100.0f)
#define CONVERT_O_Y                     (CONVERT_O)
#define CONVERT_O_P                     (CONVERT_O)
#define CONVERT_O_R                     (CONVERT_O)

/*****************************************************************************/

/*
 * The SENSORS Module
 */

/*****************************************************************************/

#define EVENT_TYPE_ACCEL_X              ABS_X
#define EVENT_TYPE_ACCEL_Y              ABS_Y
#define EVENT_TYPE_ACCEL_Z              ABS_Z

#define EVENT_TYPE_YAW                  ABS_RX
#define EVENT_TYPE_PITCH                ABS_RY
#define EVENT_TYPE_ROLL                 ABS_RZ
#define EVENT_TYPE_ORIENT_STATUS        ABS_WHEEL

#define EVENT_TYPE_MAGV_X               ABS_X
#define EVENT_TYPE_MAGV_Y               ABS_Y
#define EVENT_TYPE_MAGV_Z               ABS_Z

#define EVENT_TYPE_LIGHT                ABS_MISC
#define EVENT_TYPE_TEMPERATURE	        ABS_MISC
#define EVENT_TYPE_PROXIMITY            ABS_DISTANCE

#define CONVERT_M                       (1.0f/16.0f)
#define EVENT_TYPE_PRESSURE	        ABS_PRESSURE

#define PROXIMITY_THRESHOLD_GP2A        5.0f
/*****************************************************************************/

#define DELAY_OUT_TIME                  0x7FFFFFFF
#define LIGHT_SENSOR_POLLTIME           2000000000
#define SENSOR_STATE_MASK               (0x7FFF)

#define SENSORS_ACCELERATION            (1<<ID_A)
#define SENSORS_MAGNETIC_FIELD          (1<<ID_M)
#define SENSORS_GYROSCOPE               (1<<ID_GY)
#define SENSORS_LIGHT                   (1<<ID_L)
#define SENSORS_PROXIMITY               (1<<ID_PX)
#define SENSORS_ORIENTATION             (1<<ID_O)
#define SENSORS_TEMPERATURE             (1<<ID_T)
#define SENSORS_PRESSURE                (1<<ID_P)


#define SENSORS_ACCELERATION_HANDLE     0
#define SENSORS_MAGNETIC_FIELD_HANDLE   1
#define SENSORS_GYROSCOPE_HANDLE        2
#define SENSORS_LIGHT_HANDLE            3
#define SENSORS_PROXIMITY_HANDLE        4
#define SENSORS_ORIENTATION_HANDLE      5
#define SENSORS_TEMPERATURE_HANDLE      6
#define SENSORS_PRESSURE_HANDLE         7

/*****************************************************************************/

/*****************************************************************************/
#define NUMOFACCDATA                    8
#define LSG_LIS35DE                     (15.0f)
#define LSG_MMA7660                     (21.0f)
#define LSG_MXC622X                     (64.0f)
#define LSG_MC32X0                      (64.0f)
#define LSG_LIS3DE_ACC                  (65.0f)
#define LSG_BMA250                      (256.0f)
#define LSG_DAM380                      (256.0f)
#define LSG_STK831X                     (21.0f)
#define LSG_DMARD06                     (256.0f)
#define LSG_DMARD10                     (1024.0f)
#define LSG_MMA8452                     (1024.0f)
#define LSG_KXTIK                       (1024.0f)
#define LSG_MMA865X                     (1024.0f)
#define LSG_LIS3DH_ACC                  (1024.0f)
#define LSG_LSM303D_ACC                 (1024.0f)
#define LSG_AFA750                      (4096.0f)
#define LSG_FXOS8700_ACC                (16384.0f)
#define RANGE_A                         (2*GRAVITY_EARTH)
#define RESOLUTION_A                    (RANGE_A/(256*NUMOFACCDATA))
/*****************************************************************************/

struct sensor_info{
        char sensorName[64];
        char classPath[128];
        float priData;
};
struct sensors_data{
        char name[64];
        float lsg;
};
struct sensor_extend_t{
        struct sensors_data sensors;
        struct sensor_t sList;
};

struct status{
        bool isUsed;
        bool isFound;
};

struct o_device{
        int isFind;
        char name[32];
};
/*****************************************************************************/

extern int insmodDevice(void);
extern int sensorsDetect(void);
extern struct sensor_t sSensorList[SUPPORT_SENSORS_NUMBER];
extern struct status seStatus[SUPPORT_SENSORS_NUMBER];
extern struct sensor_info gsensorInfo;
extern struct sensor_info magSensorInfo;
extern struct sensor_info gyrSensorInfo;
extern struct sensor_info ligSensorInfo;
extern struct sensor_info proSensorInfo;
extern struct sensor_info oriSensorInfo;
extern struct sensor_info tempSensorInfo;
extern struct sensor_info preSensorInfo;

extern int sNumber;


/*****************************************************************************/

__END_DECLS

#endif  // ANDROID_SENSORS_H
