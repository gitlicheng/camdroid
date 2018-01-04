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
//#define LOG_NDEBUG 0
#define LOG_TAG "BatteryMonitor"
#include <utils/Log.h>

#include "include_battery/BatteryMonitor.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define POWER_SUPPLY_SUBSYSTEM "power_supply"
#define POWER_SUPPLY_SYSFS_PATH "/sys/class/" POWER_SUPPLY_SUBSYSTEM
static int count = 0;
using namespace std;

struct sysfsStringEnumMap {
    const char* s;
    int val;
};

static int mapSysfsString(const char* str, struct sysfsStringEnumMap map[]) {
    for (int i = 0; map[i].s; i++)
        if (!strcmp(str, map[i].s))
            return map[i].val;

    return -1;
}

BatteryMonitor::BatteryMonitor() :
    mUpdateThread(NULL) {
	memset(&mBatteryProperties, 0, sizeof(struct BatteryProperties));
	memset(&mBatteryConfig, 0, sizeof(struct BatteryConfig));
	ALOGD("BatteryMonitor Initialize");
}

BatteryMonitor::~BatteryMonitor() {	
	ALOGD("~BatteryMonitor Destructor");
}

int BatteryMonitor::getBatteryStatus(const char* status) {
    int ret;
    struct sysfsStringEnumMap batteryStatusMap[] = {
        { "Unknown", BATTERY_STATUS_UNKNOWN },
        { "Charging", BATTERY_STATUS_CHARGING },
        { "Discharging", BATTERY_STATUS_DISCHARGING },
        { "Not charging", BATTERY_STATUS_NOT_CHARGING },
        { "Full", BATTERY_STATUS_FULL },
        { NULL, 0 },
    };

    ret = mapSysfsString(status, batteryStatusMap);
    if (ret < 0) {
        ALOGW("Unknown battery status '%s'\n", status);
        ret = BATTERY_STATUS_UNKNOWN;
    }

    return ret;
}

int BatteryMonitor::getBatteryHealth(const char* status) {
    int ret;
    struct sysfsStringEnumMap batteryHealthMap[] = {
        { "Unknown", BATTERY_HEALTH_UNKNOWN },
        { "Good", BATTERY_HEALTH_GOOD },
        { "Overheat", BATTERY_HEALTH_OVERHEAT },
        { "Dead", BATTERY_HEALTH_DEAD },
        { "Over voltage", BATTERY_HEALTH_OVER_VOLTAGE },
        { "Unspecified failure", BATTERY_HEALTH_UNSPECIFIED_FAILURE },
        { "Cold", BATTERY_HEALTH_COLD },
        { NULL, 0 },
    };

    ret = mapSysfsString(status, batteryHealthMap);
    if (ret < 0) {
        ALOGW("Unknown battery health '%s'\n", status);
        ret = BATTERY_HEALTH_UNKNOWN;
    }

    return ret;
}

int BatteryMonitor::readFromFile(const char* path, char* buf, size_t size) {
    char *cp = NULL;

    if (path == NULL)
        return -1;
    int fd = open(path, O_RDONLY, 0);
    if (fd == -1) {
//        ALOGE("Could not open '%s'\n", path);
        return -1;
    }

    ssize_t count = TEMP_FAILURE_RETRY(read(fd, buf, size));
    if (count > 0)
            cp = (char *)memrchr(buf, '\n', count);

    if (cp)
        *cp = '\0';
    else
        buf[0] = '\0';

    close(fd);
    return count;
}

BatteryMonitor::PowerSupplyType BatteryMonitor::readPowerSupplyType(const char* path) {
    const int SIZE = 128;
    char buf[SIZE];
    int length = readFromFile(path, buf, SIZE);
    BatteryMonitor::PowerSupplyType ret;
    struct sysfsStringEnumMap supplyTypeMap[] = {
            { "Unknown", POWER_SUPPLY_TYPE_UNKNOWN },
            { "Battery", POWER_SUPPLY_TYPE_BATTERY },
            { "UPS", POWER_SUPPLY_TYPE_AC },
            { "Mains", POWER_SUPPLY_TYPE_AC },
            { "USB", POWER_SUPPLY_TYPE_USB },
            { "USB_DCP", POWER_SUPPLY_TYPE_AC },
            { "USB_CDP", POWER_SUPPLY_TYPE_AC },
            { "USB_ACA", POWER_SUPPLY_TYPE_AC },
            { "Wireless", POWER_SUPPLY_TYPE_WIRELESS },
            { NULL, 0 },
    };

    if (length <= 0)
        return POWER_SUPPLY_TYPE_UNKNOWN;

    ret = (BatteryMonitor::PowerSupplyType)mapSysfsString(buf, supplyTypeMap);
    if (ret < 0)
        ret = POWER_SUPPLY_TYPE_UNKNOWN;

    return ret;
}

bool BatteryMonitor::getBooleanField(const char* path) {
    const int SIZE = 16;
    char buf[SIZE];

    bool value = false;
    if (readFromFile(path, buf, SIZE) > 0) {
        if (buf[0] != '0') {
            value = true;
        }
    }

    return value;
}

int BatteryMonitor::getIntField(const char* path) {
    const int SIZE = 128;
    char buf[SIZE];

    int value = 0;
    if (readFromFile(path, buf, SIZE) > 0) {
        value = strtol(buf, NULL, 0);
    }
    return value;
}

int BatteryMonitor::updateLoop(void) {
	///struct BatteryProperties mBatteryProperties;

//	ALOGE("BatteryMonitor updateLoop \n");
    mBatteryProperties.chargerAcOnline = false;
    mBatteryProperties.chargerUsbOnline = false;
    mBatteryProperties.chargerWirelessOnline = false;
    mBatteryProperties.batteryStatus = BATTERY_STATUS_UNKNOWN;
    mBatteryProperties.batteryHealth = BATTERY_HEALTH_UNKNOWN;
    mBatteryProperties.batteryCurrentNow = INT_MIN;
    mBatteryProperties.batteryChargeCounter = INT_MIN;

    if (mBatteryConfig.batteryPresentPath != NULL)
        mBatteryProperties.batteryPresent = getBooleanField(mBatteryConfig.batteryPresentPath);
    else
        mBatteryProperties.batteryPresent = true;

    mBatteryProperties.batteryLevel = getIntField(mBatteryConfig.batteryCapacityPath);
    mBatteryProperties.batteryVoltage = getIntField(mBatteryConfig.batteryVoltagePath) / 1000;

    if (mBatteryConfig.batteryCurrentNowPath != NULL)
        mBatteryProperties.batteryCurrentNow = getIntField(mBatteryConfig.batteryCurrentNowPath);

    if (mBatteryConfig.batteryChargeCounterPath != NULL)
        mBatteryProperties.batteryChargeCounter = getIntField(mBatteryConfig.batteryChargeCounterPath);

    mBatteryProperties.batteryTemperature = getIntField(mBatteryConfig.batteryTemperaturePath);

    const int SIZE = 128;
    char buf[SIZE];

    if (readFromFile(mBatteryConfig.batteryStatusPath, buf, SIZE) > 0)
        mBatteryProperties.batteryStatus = getBatteryStatus(buf);

    if (readFromFile(mBatteryConfig.batteryHealthPath, buf, SIZE) > 0)
        mBatteryProperties.batteryHealth = getBatteryHealth(buf);

    if (readFromFile(mBatteryConfig.batteryTechnologyPath, buf, SIZE) > 0)
		strcpy(mBatteryProperties.batteryTechnology, buf);

    unsigned int i;

    for (i = 0; i < mChargerNames.size(); i++) {
        char path[512];
		bzero(path, sizeof(path));
		sprintf(path, "%s/%s/online", POWER_SUPPLY_SYSFS_PATH, mChargerNames[i]);

        if (readFromFile(path, buf, SIZE) > 0) {
            if (buf[0] != '0') {
                sprintf(path, "%s/%s/type", POWER_SUPPLY_SYSFS_PATH, mChargerNames[i]);
                switch(readPowerSupplyType(path)) {
                case POWER_SUPPLY_TYPE_AC:
                    mBatteryProperties.chargerAcOnline = true;
                    break;
                case POWER_SUPPLY_TYPE_USB:
                    mBatteryProperties.chargerUsbOnline = true;
                    break;
                case POWER_SUPPLY_TYPE_WIRELESS:
                    mBatteryProperties.chargerWirelessOnline = true;
                    break;
                default:
                    ALOGW("%s: Unknown power supply type\n", mChargerNames[i]);
                }
            }
        }
    }
	if(0) {	
		//printf("chargerAcOnline = %d. \n", mBatteryProperties.chargerAcOnline);
		//printf("chargerUsbOnline = %d. \n", mBatteryProperties.chargerUsbOnline);
		//printf("chargerWirelessOnline = %d. \n", mBatteryProperties.chargerWirelessOnline);
		//printf("batteryPresent = %d. \n", mBatteryProperties.batteryPresent);
		printf("batteryStatus = %d. \n", mBatteryProperties.batteryStatus);
		//printf("batteryHealth = %d. \n", mBatteryProperties.batteryHealth);
		printf("batteryLevel = %d. \n", mBatteryProperties.batteryLevel);
		printf("batteryVoltage = %d. \n", mBatteryProperties.batteryVoltage);
		//printf("batteryChargeCounter = %d. \n", mBatteryProperties.batteryChargeCounter);
		//printf("batteryTemperature = %d. \n", mBatteryProperties.batteryTemperature);
		//printf("batteryCurrentNow = %d. \n", mBatteryProperties.batteryCurrentNow);
	}
	usleep(1000*1000);

    return mBatteryProperties.chargerAcOnline | mBatteryProperties.chargerUsbOnline | mBatteryProperties.chargerWirelessOnline;
}

int BatteryMonitor::init(void) {
	char path[128];
	
    DIR* dir = opendir(POWER_SUPPLY_SYSFS_PATH);
    if (dir == NULL) {
//        ALOGE("Could not open %s\n", POWER_SUPPLY_SYSFS_PATH);
    } else {
        struct dirent* entry;

        while ((entry = readdir(dir))) {
            const char* name = entry->d_name;

            if (!strcmp(name, ".") || !strcmp(name, ".."))
                continue;
			
            // Look for "type" file in each subdirectory
			sprintf(path, "%s/%s/type", POWER_SUPPLY_SYSFS_PATH, name);
			ALOGD("PowerSupply path: %s\n", path);
            switch(readPowerSupplyType(path)) {
            case POWER_SUPPLY_TYPE_AC:
            case POWER_SUPPLY_TYPE_USB:
            case POWER_SUPPLY_TYPE_WIRELESS:
				sprintf(path, "%s/%s/online", POWER_SUPPLY_SYSFS_PATH, name);
                if (access(path, R_OK) == 0)
                    mChargerNames.add(path);
                break;

            case POWER_SUPPLY_TYPE_BATTERY:
                if (mBatteryConfig.batteryStatusPath == NULL) {
					sprintf(path, "%s/%s/status", POWER_SUPPLY_SYSFS_PATH, name);
                    if (access(path, R_OK) == 0) {
						mBatteryConfig.batteryStatusPath = (char *)malloc(64);
						strcpy(mBatteryConfig.batteryStatusPath, path);
					}
                }

                if (mBatteryConfig.batteryHealthPath == NULL) {
					sprintf(path, "%s/%s/health", POWER_SUPPLY_SYSFS_PATH, name);
                    if (access(path, R_OK) == 0) {
						mBatteryConfig.batteryHealthPath = (char *)malloc(64);
						strcpy(mBatteryConfig.batteryHealthPath, path);
					}
                }

                if (mBatteryConfig.batteryPresentPath == NULL) {
					sprintf(path, "%s/%s/present", POWER_SUPPLY_SYSFS_PATH, name);
                    if (access(path, R_OK) == 0) {
						mBatteryConfig.batteryPresentPath = (char *)malloc(64);
						strcpy(mBatteryConfig.batteryPresentPath, path);
					}
                }

                if (mBatteryConfig.batteryCapacityPath == NULL) {
					sprintf(path, "%s/%s/capacity", POWER_SUPPLY_SYSFS_PATH, name);
                    if (access(path, R_OK) == 0) {
						mBatteryConfig.batteryCapacityPath = (char *)malloc(64);
						strcpy(mBatteryConfig.batteryCapacityPath, path);
					}
                }

                if (mBatteryConfig.batteryVoltagePath == NULL) {
					sprintf(path, "%s/%s/voltage_now", POWER_SUPPLY_SYSFS_PATH, name);
                    if (access(path, R_OK) == 0) {
						mBatteryConfig.batteryVoltagePath = (char *)malloc(64);
						strcpy(mBatteryConfig.batteryVoltagePath, path);
					} else {
						sprintf(path, "%s/%s/batt_vol", POWER_SUPPLY_SYSFS_PATH, name);
						if (access(path, R_OK) == 0) {
							mBatteryConfig.batteryVoltagePath = (char *)malloc(64);
							strcpy(mBatteryConfig.batteryVoltagePath, path);
						}
					}
                }

                if (mBatteryConfig.batteryCurrentNowPath == NULL) {
					sprintf(path, "%s/%s/current_now", POWER_SUPPLY_SYSFS_PATH, name);
                    if (access(path, R_OK) == 0) {
						mBatteryConfig.batteryCurrentNowPath = (char *)malloc(64);
						strcpy(mBatteryConfig.batteryCurrentNowPath, path);
					}
                }

                if (mBatteryConfig.batteryChargeCounterPath == NULL) {
					sprintf(path, "%s/%s/charge_counter", POWER_SUPPLY_SYSFS_PATH, name);
                    if (access(path, R_OK) == 0) {
						mBatteryConfig.batteryChargeCounterPath = (char *)malloc(64);
						strcpy(mBatteryConfig.batteryChargeCounterPath, path);
					}
                }

                if (mBatteryConfig.batteryTemperaturePath == NULL) {
					sprintf(path, "%s/%s/temp", POWER_SUPPLY_SYSFS_PATH, name);
                    if (access(path, R_OK) == 0) {
						mBatteryConfig.batteryTemperaturePath = (char *)malloc(64);
						strcpy(mBatteryConfig.batteryTemperaturePath, path);
					} else {
						sprintf(path, "%s/%s/batt_temp", POWER_SUPPLY_SYSFS_PATH, name);
						if (access(path, R_OK) == 0) {
							mBatteryConfig.batteryTemperaturePath = (char *)malloc(64);
							strcpy(mBatteryConfig.batteryTemperaturePath, path);
						}
					}
                }

                if (mBatteryConfig.batteryTechnologyPath == NULL) {
					sprintf(path, "%s/%s/technology", POWER_SUPPLY_SYSFS_PATH, name);
                    if (access(path, R_OK) == 0) {
						mBatteryConfig.batteryTechnologyPath = (char *)malloc(64);
						strcpy(mBatteryConfig.batteryTechnologyPath, path);
					}
                }

                break;

            case POWER_SUPPLY_TYPE_UNKNOWN:
                break;
            }
        }
        closedir(dir);
    }

    if (!mChargerNames.size())
        ALOGW("No charger supplies found\n");
    if (mBatteryConfig.batteryStatusPath == NULL)
        ALOGE("BatteryStatusPath not found\n");
    if (mBatteryConfig.batteryHealthPath == NULL)
        ALOGE("BatteryHealthPath not found\n");
    if (mBatteryConfig.batteryPresentPath == NULL)
        ALOGE("BatteryPresentPath not found\n");
    if (mBatteryConfig.batteryCapacityPath == NULL)
        ALOGE("BatteryCapacityPath not found\n");
    if (mBatteryConfig.batteryVoltagePath == NULL)
        ALOGE("BatteryVoltagePath not found\n");
    if (mBatteryConfig.batteryTemperaturePath == NULL)
        ALOGE("BatteryTemperaturePath not found\n");
    if (mBatteryConfig.batteryTechnologyPath == NULL)
        ALOGE("BatteryTechnologyPath not found\n");

	if(mUpdateThread == NULL) {
		mUpdateThread = new UpdateThread(this);
		mUpdateThread->startThread();
	}
	
	return 0;
}

int BatteryMonitor::exit(void) {
	if(mUpdateThread != NULL) {
		mUpdateThread->stopThread();
		mUpdateThread.clear();
		mUpdateThread = NULL;
	}
    if (mBatteryConfig.batteryStatusPath != NULL){
		free(mBatteryConfig.batteryStatusPath);
		mBatteryConfig.batteryStatusPath = NULL;
	}
    if (mBatteryConfig.batteryHealthPath != NULL){
		free(mBatteryConfig.batteryHealthPath);
		mBatteryConfig.batteryHealthPath = NULL;
	}
    if (mBatteryConfig.batteryPresentPath != NULL){
		free(mBatteryConfig.batteryPresentPath);
		mBatteryConfig.batteryPresentPath = NULL;
	}
    if (mBatteryConfig.batteryCapacityPath != NULL){
		free(mBatteryConfig.batteryCapacityPath);
		mBatteryConfig.batteryCapacityPath = NULL;
	}
    if (mBatteryConfig.batteryVoltagePath != NULL){
		free(mBatteryConfig.batteryVoltagePath);
		mBatteryConfig.batteryVoltagePath = NULL;
	}
    if (mBatteryConfig.batteryTemperaturePath != NULL){
		free(mBatteryConfig.batteryTemperaturePath);
		mBatteryConfig.batteryTemperaturePath = NULL;
	}
    if (mBatteryConfig.batteryTechnologyPath != NULL){
		free(mBatteryConfig.batteryTechnologyPath);
		mBatteryConfig.batteryTechnologyPath = NULL;
	}
    if (mBatteryConfig.batteryCurrentNowPath != NULL){
		free(mBatteryConfig.batteryCurrentNowPath);
		mBatteryConfig.batteryCurrentNowPath = NULL;
	}
    if (mBatteryConfig.batteryChargeCounterPath != NULL){
		free(mBatteryConfig.batteryChargeCounterPath);
		mBatteryConfig.batteryChargeCounterPath = NULL;
	}
	
	return 0;
}

int BatteryMonitor::getStatus(void) {
	return mBatteryProperties.batteryStatus;
}

int BatteryMonitor::getLevel(void) {
	return mBatteryProperties.batteryLevel;
}

int BatteryMonitor::getVoltage(void) {
	return mBatteryProperties.batteryVoltage;
}

