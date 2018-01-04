
#define LOG_TAG "SensorDetect"
#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <linux/input.h>
#include <cutils/atomic.h>
#include <cutils/log.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <cutils/misc.h>
#include <cutils/properties.h>
#include "sensors.h"

int sNumber = 0;
int flag0 = 0;
int flag1 = 0;
int found = 0;
struct sensor_t sSensorList[SUPPORT_SENSORS_NUMBER];
struct status seStatus[SUPPORT_SENSORS_NUMBER];
struct sensor_info gsensorInfo = {";", {0}, 0};
struct sensor_info magSensorInfo = {";", {0}, 0};
struct sensor_info gyrSensorInfo = {";", {0}, 0};
struct sensor_info ligSensorInfo = {";", {0}, 0};
struct sensor_info proSensorInfo = {";", {0}, 0};
struct sensor_info oriSensorInfo = {";", {0}, 0};
struct sensor_info preSensorInfo = {";", {0}, 0};
struct sensor_info tempSensorInfo = {";", {0}, 0};


static struct o_device otherDevice[] = {
    {
        0, "sw-",
    }, {
        0, "axp",
    },
};

static struct o_device ctpDevice[] = {
    {
        0, "gt",
    }, {
        0, "gsl",
    }, {
        0, "ft5x"
    },
};

struct sensor_extend_t preSensorList[] = {

};

struct sensor_extend_t tempSensorList[] = {
    {
        {
            "sunxi-ths", 0,
        },{
            "Temperature sensor",
            "AW.",
            1, SENSORS_TEMPERATURE_HANDLE,
            SENSOR_TYPE_AMBIENT_TEMPERATURE,
            125.0f, 1.0f,
            0.35f, 0,
            0, 0,
            { },
        },
    },
};

struct sensor_extend_t oriSensorList[] = {
    {
        {
            "FreescaleMagnetometer", 0,
        }, {
            "Freescale Orientation sensor",
            "Freescale Semiconductor Inc.",
            1,
            SENSORS_ORIENTATION_HANDLE,
            SENSOR_TYPE_ORIENTATION,
            360.0f,  CONVERT_O,
            0.50f,  100000,
            0, 0,
            { },
        },
    },
};

struct sensor_extend_t proSensorList[] = {
    {
        {
            "proximity", 0,
        }, {
            "LTR-C216R-14 sensor PROXIMITY",
            "LTR 2",
            1,
            SENSORS_PROXIMITY_HANDLE,
            SENSOR_TYPE_PROXIMITY,
            5.0f, 5.0f,
            0.2f, 0,
            0, 0,
            { },
        },
    },{
        {
            "stk3x1x_ps", 0,
        }, {
            "stk3x1x Proximity Sensor",
            "sensortek",
            1,
            SENSORS_PROXIMITY_HANDLE,
            SENSOR_TYPE_PROXIMITY,
            1.0f, 0.1f,
            0.873f, 0,
            0, 0,
            { },
        },
    }

};

struct sensor_extend_t ligSensorList[] = {
    {
        {
            "lightsensor", 0,
        }, {
            "LTR-C216R-14 sensor LIGHT",
            "LTR ",
            1,
            SENSORS_LIGHT_HANDLE,
            SENSOR_TYPE_LIGHT,
            20000.0f, 20.0f,
            0.2f, 0, 
            0, 0,
            { },
        },
    }, {
        {
            "stk3x1x_ls", 0,
        }, {
            "stk3x1x Ambient Light Sensor",
            "sensortek ",
            1,
            SENSORS_LIGHT_HANDLE,
            SENSOR_TYPE_LIGHT,
            4096.0f, 1.0f,
            0.09f, 0, 
            0, 0,
            { },
        },
    },

};

struct sensor_extend_t gyrSensorList[] = {
    {
        {
            "l3gd20_gyr", 0,
        }, {
            "ST 3-axis Gyroscope sensor",
            "STMicroelectronics",
            1,
            SENSORS_GYROSCOPE_HANDLE,
            SENSOR_TYPE_GYROSCOPE,
            (2000.0f*(float)M_PI/180.0f),
            (70.0f / 1000.0f) * ((float)M_PI / 180.0f),
            6.1f, 1190,
            0, 0,
            { },
        },
    },
};

struct sensor_extend_t magSensorList[] = {
    {
        {
            "lsm303d_mag",  0,
        }, {
            "ST 3-axis Magnetic field sensor",
            "STMicroelectronics",
            1,
            SENSORS_MAGNETIC_FIELD_HANDLE,
            SENSOR_TYPE_MAGNETIC_FIELD,
            2000.0f, CONVERT_M,
            6.8f, 16667,
            0, 0,
            { },
        },
    }, {
        {
            "FreescaleMagnetometer", 0,
        }, {
            "Freescale 3-axis Magnetic field sensor",
            "Freescale Semiconductor Inc.",
            1,
            SENSORS_MAGNETIC_FIELD_HANDLE,
            SENSOR_TYPE_MAGNETIC_FIELD,
            1500.0f, 1.0f/20.0f,
            0.50f, 100000,
            0, 0,
            { },
        },
    },
};

struct sensor_extend_t gsensorList[] = {
    {
        {
            "stk831x", LSG_STK831X,
        }, {
            "STK 3-axis Accelerometer",
            "STK",
            1, 0,
            SENSOR_TYPE_ACCELEROMETER,
            RANGE_A,
            GRAVITY_EARTH/21.0f,
            0.30f, 20000,
            0, 0,
            { },
        },
    }, {
        {
            "bma250", LSG_BMA250,
        }, {
            "Bosch 3-axis Accelerometer",
            "Bosch",
            1, 0,
            SENSOR_TYPE_ACCELEROMETER,
            4.0f*9.81f,
            (4.0f*9.81f)/1024.0f,
            0.2f, 0,
			0, 0,
			{ },
        },
    },{
        {
            "mma8452", LSG_MMA8452,
        }, {
            "MMA8452 3-axis Accelerometer",
            "Freescale Semiconductor Inc.",
            1, 0,
            SENSOR_TYPE_ACCELEROMETER,
            RANGE_A,
            GRAVITY_EARTH/1024.0f,
            0.30f, 20000,
			0, 0,
			{ },
        },
    }, {
        {
            "mma7660", LSG_MMA7660,
        }, {
            "MMA7660 3-axis Accelerometer",
                "Freescale Semiconductor Inc.",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                RANGE_A,
                GRAVITY_EARTH/21.0f,
                0.30f, 20000,
                0, 0,
                { },
        },
    }, {
        {
            "mma865x", LSG_MMA865X,
        }, {
            "MMA865x 3-axis Accelerometer",
                "Freescale Semiconductor Inc.",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                RANGE_A,
                GRAVITY_EARTH/1024.0f,
                0.30f, 20000,
                0, 0,
                { },
        },
    },  {
        {
            "afa750", LSG_AFA750,
        }, {
            "AFA750 3-axis Accelerometer",
                "AFA",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                4.0f*9.81f,
                GRAVITY_EARTH/4096.0f,
                0.8f, 0,
                0, 0,
                { },
        },
    },  {
        {
            "lis3de_acc", LSG_LIS3DE_ACC,
        }, {
            "lis3de detect Accelerometer",
                "ST",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                4.0f*9.81f,
                (4.0f*9.81f)/1024.0f,
                0.2f, 0,
                0, 0,
                { },
        },
    }, {
        {
            "lis3dh_acc", LSG_LIS3DH_ACC,
        }, {
            "lis3dh detect Accelerometer",
                "ST",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                4.0f*9.81f,
                (4.0f*9.81f)/256.0f,
                0.2f, 0,
                0, 0,
                { },
        },
    }, {
        {
            "lsm303d_acc", LSG_LSM303D_ACC,
        }, {
            "lsm303d detect Accelerometer",
                "ST",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                4.0f*9.81f,
                (4.0f*9.81f)/256.0f,
                0.2f, 0,
                0, 0,
                { },
        },
    }, {
        {
            "FreescaleAccelerometer",
                LSG_FXOS8700_ACC,
        }, {
            "Freescale 3-axis Accelerometer",
                "Freescal",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                2.0f*9.81f,
                9.81f/16384.0f,
                0.30f,  20000,
                0, 0,
                { },
        },
    }, {
        {
            "kxtik", LSG_KXTIK,
        }, {
            "Kionix 3-axis Accelerometer",
                "Kionix, Inc.",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                RANGE_A,
                RESOLUTION_A,
                0.23f, 20000,
                0, 0,
                { },
        },
    }, {
        {
            "dmard10", LSG_DMARD10,
        }, {
            "DMARD10 3-axis Accelerometer",
                "DMT",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                (GRAVITY_EARTH * 2.0f),
                GRAVITY_EARTH / 1024,
                0.145f, 0,
                0, 0,
                { },
        },
    }, {
        {
            "dmard06", LSG_DMARD06,
        }, {
            "DMARD06 3-axis Accelerometer",
                "DMT",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                (GRAVITY_EARTH * 3.0f),
                GRAVITY_EARTH/256.0f,
                0.5f, 0,
                0, 0,
                { },
        },
    },{
        {
            "mxc622x", LSG_MXC622X,
        }, {
            "mxc622x 2-axis Accelerometer",
                "MEMSIC",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                2.0,
                GRAVITY_EARTH/64.0f,
                0.005, 0,
                0, 0,
                { },
        },
    },{
        {
            "mc32x0", LSG_MC32X0,
        }, {
            "mc32x0 3-axis Accelerometer",
                "MCXX",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                2.0,
                GRAVITY_EARTH/64.0f,
                0.005, 0,
                0, 0,
               { },
        },
    }, {
        {
            "lis35de", LSG_LIS35DE,
        },  {
                "lis3dh detect Accelerometer",
                "ST",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                4.0f*9.81f,
                (4.0f*9.81f)/15.0f,
                0.2f, 0,
                0, 0,
                { },
            },
    },
	
		{
		{
            "da380", LSG_DAM380,
        },  {
			    "da380 3-axis Accelerometer",
				"Bosch",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                2.0f*9.81f,
                (2.0f*9.81f)/1024.0f,
                0.2f, 0,
                0, 0,
                { },
            },
    },

};

void printkSensor(int number)
{
    ALOGD("name:%s\n", sSensorList[number].name);
    ALOGD("vendor:%s\n", sSensorList[number].vendor);
    ALOGD("handle:%d\n", sSensorList[number].handle);
}

void setSensorData(int number,struct sensor_t *sensorList)
{
    sSensorList[number].name = sensorList->name;
    sSensorList[number].vendor = sensorList->vendor;
    sSensorList[number].version = sensorList->version;
    sSensorList[number].handle = sensorList->handle;
    sSensorList[number].type = sensorList->type;
    sSensorList[number].maxRange = sensorList->maxRange;
    sSensorList[number].resolution = sensorList->resolution;
    sSensorList[number].power = sensorList->power;
    sSensorList[number].minDelay = sensorList->minDelay;
    memcpy(sSensorList[number].reserved,sensorList->reserved,
            sizeof(sSensorList[number].reserved));
}

int getDevice(struct sensor_extend_t list[], struct sensor_info *info,
        char buf[], char classPath[], int number )
{
    int ret = 0;

    if ((!strlen(buf)) || (list == NULL)) {
        return 0;
    }

    while(ret < number){
        if (!strncmp(buf, list[ret].sensors.name, strlen(buf))) {

            info->priData = list[ret].sensors.lsg;
            setSensorData(sNumber, &list[ret].sList);

            strncpy(info->sensorName, buf,strlen(buf));
            strncpy(info->classPath, classPath, strlen(classPath));
#ifdef DEBUG_SENSOR
            ALOGD("sensorName:%s,classPath:%s,lsg:%f\n",
                    info->sensorName,info->classPath, info->priData);
#endif

            return 1 ;
        }
        ret++;
    }

    return 0;

}

int otherDeviceDetect(char buf[])
{
    int number0 = 0, number1 = 0;
    int re_value = 0;
    number0 = ARRAY_SIZE(otherDevice);
    number1 = ARRAY_SIZE(ctpDevice);
#ifdef DEBUG_SENSOR
    ALOGD("buf:%s, flag0:%d, flag1:%d\n", buf, flag0, flag1);
    ALOGD("otherDevice[0].isFind:%d,otherDevice[1].isFind:%d\n",
            otherDevice[0].isFind,
            otherDevice[1].isFind);
#endif

    if(!flag0) {
        while(number0--) {
            if(!otherDevice[number0].isFind){
                if (!strncmp(buf, otherDevice[number0].name,
                            strlen(otherDevice[number0].name))) {

                    otherDevice[number0].isFind = 1;
                    re_value = 1;
                }
            }
        }
    }
    if(!flag1) {
        while(number1--) {
            if(!ctpDevice[number1].isFind){
                if (!strncmp(buf, ctpDevice[number1].name,
                            strlen(ctpDevice[number1].name))) {

                    ctpDevice[number1].isFind = 1;
                    flag1 = 1;
                    re_value = 1;

                }
            }
        }
    }

    if((otherDevice[0].isFind == 1) && (otherDevice[1].isFind == 1))
        flag0 = 1;

    if((flag0 == 1) && (flag1 == 1)) {
        return 2;
    }

    if(re_value == 1)
        return 1;

    return 0;
}

void statusInit(void)
{
    seStatus[ID_A].isUsed  = true;
    seStatus[ID_A].isFound = false;

    seStatus[ID_M].isUsed  = true;
    seStatus[ID_M].isFound = false;

    seStatus[ID_GY].isUsed  = true;
    seStatus[ID_GY].isFound = false;

    seStatus[ID_L].isUsed  = true;
    seStatus[ID_L].isFound = false;

    seStatus[ID_PX].isUsed  = true;
    seStatus[ID_PX].isFound = false;

    seStatus[ID_O].isUsed  = false;
    seStatus[ID_O].isFound = false;

    seStatus[ID_T].isUsed  = true;
    seStatus[ID_T].isFound = false;

    seStatus[ID_P].isUsed  = false;
    seStatus[ID_P].isFound = false;

}

int searchDevice(char buf[], char classPath[])
{
    int ret = 0;

    if(!found) {
        found = otherDeviceDetect(buf);

#ifdef DEBUG_SENSOR
        ALOGD("found:%d\n", found);
#endif

        if(found == 1) {
            found = 0;
            return 1;
        }else if(found == 2) {
            return 1;
        }
    }

    if((seStatus[ID_A].isUsed == true) && (seStatus[ID_A].isFound == false)) {

        ret = getDevice(gsensorList, &gsensorInfo, buf, classPath,
                ARRAY_SIZE(gsensorList));

        if(ret == 1){
            seStatus[ID_A].isFound = true;
            sNumber++;
            return 1;
        }
    }


    if((seStatus[ID_M].isUsed == true) && (seStatus[ID_M].isFound == false)) {
        ret = getDevice(magSensorList, &magSensorInfo, buf, classPath,
                ARRAY_SIZE(magSensorList));

        if(ret == 1){
            seStatus[ID_M].isFound = true;
            sNumber++;

            if(!strncmp(buf, magSensorList[1].sensors.name, strlen(buf))) {
                setSensorData(sNumber, &oriSensorList[0].sList);
                sNumber++;
            }
            return 1;
        }
    }

    if((seStatus[ID_GY].isUsed == true) && (seStatus[ID_GY].isFound == false)) {
        ret = getDevice(gyrSensorList, &gyrSensorInfo, buf, classPath,
                ARRAY_SIZE(gyrSensorList));

        if(ret == 1){
            seStatus[ID_GY].isFound = true;
            sNumber++;
            return 1;
        }
    }

    if((seStatus[ID_L].isUsed == true) && (seStatus[ID_L].isFound == false)) {
        ret = getDevice(ligSensorList, &ligSensorInfo, buf, classPath,
                ARRAY_SIZE(ligSensorList));

        if(ret == 1){
            seStatus[ID_L].isFound = true;
            sNumber++;
            return 1;
        }
    }

    if((seStatus[ID_PX].isUsed == true) && (seStatus[ID_PX].isFound == false)) {
        ret = getDevice(proSensorList, &proSensorInfo, buf, classPath,
                ARRAY_SIZE(proSensorList));

        if(ret == 1){
            seStatus[ID_PX].isFound = true;
            sNumber++;
            return 1;
        }
    }

    if((seStatus[ID_O].isUsed == true) && (seStatus[ID_O].isFound == false)) {
        ret = getDevice(oriSensorList, &oriSensorInfo, buf, classPath,
                ARRAY_SIZE(oriSensorList));

        if(ret == 1){
            seStatus[ID_O].isFound = true;
            sNumber++;
            return 1;
        }
    }

    if((seStatus[ID_T].isUsed == true) && (seStatus[ID_T].isFound == false)) {
        ret = getDevice(tempSensorList, &tempSensorInfo, buf, classPath,
                ARRAY_SIZE(tempSensorList));

        if(ret == 1){
            seStatus[ID_T].isFound = true;
            sNumber++;
            return 1;
        }
    }

    if((seStatus[ID_P].isUsed == true) && (seStatus[ID_P].isFound == false)) {
        ret = getDevice(preSensorList, &preSensorInfo, buf, classPath,
                ARRAY_SIZE(preSensorList));

        if(ret == 1){
            seStatus[ID_T].isFound = true;
            sNumber++;
            return 1;
        }
    }

    return 0;
}

int sensorsDetect(void)
{
    char *dirname =(char *) "/sys/class/input";
    char buf[256];
    char classPath[256];
    int res;
    DIR *dir;
    struct dirent *de;
    int fd = -1;
    int ret = 0;

    memset(&buf,0,sizeof(buf));
    statusInit();

    dir = opendir(dirname);
    if (dir == NULL)
        return -1;

    while((de = readdir(dir))) {
        if (strncmp(de->d_name, "input", strlen("input")) != 0) {
            continue;
        }

        sprintf(classPath, "%s/%s", dirname, de->d_name);
        snprintf(buf, sizeof(buf), "%s/name", classPath);

        fd = open(buf, O_RDONLY);
        if (fd < 0) {
            continue;
        }

        if ((res = read(fd, buf, sizeof(buf))) < 0) {
            close(fd);
            continue;
        }
        buf[res - 1] = '\0';

#ifdef DEBUG_SENSOR
        ALOGD("buf:%s\n", buf);
#endif

        ret = searchDevice(buf, classPath);

        close(fd);
        fd = -1;
    }

    closedir(dir);

    return 0;
}
