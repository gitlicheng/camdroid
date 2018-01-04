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

#ifndef HEALTHD_BATTERYMONITOR_H
#define HEALTHD_BATTERYMONITOR_H

#include <utils/Vector.h>
#include <utils/Mutex.h>
#include <utils/Thread.h>

using namespace android;

struct BatteryProperties {
    bool chargerAcOnline;
    bool chargerUsbOnline;
    bool chargerWirelessOnline;
    bool batteryPresent;
    int batteryStatus;
    int batteryHealth;
    int batteryLevel;
    int batteryVoltage;
    int batteryCurrentNow;
    int batteryChargeCounter;
    int batteryTemperature;
    char batteryTechnology[64];
};

struct BatteryConfig {
    char* batteryStatusPath;
    char* batteryHealthPath;
    char* batteryPresentPath;
    char* batteryCapacityPath;
    char* batteryVoltagePath;
    char* batteryTemperaturePath;
    char* batteryTechnologyPath;
    char* batteryCurrentNowPath;
    char* batteryChargeCounterPath;
};

class BatteryMonitor {
public:
	BatteryMonitor();
	~BatteryMonitor();

public:
  
	enum {
		BATTERY_STATUS_UNKNOWN = 1, // equals BatteryManager.BATTERY_STATUS_UNKNOWN constant
		BATTERY_STATUS_CHARGING = 2, // equals BatteryManager.BATTERY_STATUS_CHARGING constant
		BATTERY_STATUS_DISCHARGING = 3, // equals BatteryManager.BATTERY_STATUS_DISCHARGING constant
		BATTERY_STATUS_NOT_CHARGING = 4, // equals BatteryManager.BATTERY_STATUS_NOT_CHARGING constant
		BATTERY_STATUS_FULL = 5, // equals BatteryManager.BATTERY_STATUS_FULL constant
	};

	// must be kept in sync with definitions in BatteryManager.java
	enum {
		BATTERY_HEALTH_UNKNOWN = 1, // equals BatteryManager.BATTERY_HEALTH_UNKNOWN constant
		BATTERY_HEALTH_GOOD = 2, // equals BatteryManager.BATTERY_HEALTH_GOOD constant
		BATTERY_HEALTH_OVERHEAT = 3, // equals BatteryManager.BATTERY_HEALTH_OVERHEAT constant
		BATTERY_HEALTH_DEAD = 4, // equals BatteryManager.BATTERY_HEALTH_DEAD constant
		BATTERY_HEALTH_OVER_VOLTAGE = 5, // equals BatteryManager.BATTERY_HEALTH_OVER_VOLTAGE constant
		BATTERY_HEALTH_UNSPECIFIED_FAILURE = 6, // equals BatteryManager.BATTERY_HEALTH_UNSPECIFIED_FAILURE constant
		BATTERY_HEALTH_COLD = 7, // equals BatteryManager.BATTERY_HEALTH_COLD constant
	};
	
    enum PowerSupplyType {
        POWER_SUPPLY_TYPE_UNKNOWN = 0,
        POWER_SUPPLY_TYPE_AC,
        POWER_SUPPLY_TYPE_USB,
        POWER_SUPPLY_TYPE_WIRELESS,
        POWER_SUPPLY_TYPE_BATTERY
    };

	int init(void);
	int exit(void);
	
	int getStatus(void) ;
	int getLevel(void);
	int getVoltage(void);

	class UpdateThread : public Thread {
	public:
		UpdateThread(BatteryMonitor* handle) :
						Thread(false),
						mBM(handle){
		}
			  
		void startThread() {
			run("UpdateThread", PRIORITY_NORMAL);
		}
		
		void stopThread() {
			requestExitAndWait();
		}
		
		virtual bool threadLoop() {
			mBM->updateLoop();
			return true;
		}
		
	private:
		BatteryMonitor* mBM;
	};

private:
	struct BatteryProperties mBatteryProperties;
    struct BatteryConfig mBatteryConfig;
    Vector<char*> mChargerNames;
	sp<UpdateThread> mUpdateThread;
	
	int	updateLoop(void);
    int getBatteryStatus(const char* status);
    int getBatteryHealth(const char* status);
    int readFromFile(const char* path, char* buf, size_t size);
    PowerSupplyType readPowerSupplyType(const char* path);
    bool getBooleanField(const char* path);
    int getIntField(const char* path);
};
extern int Get_Battery_Status(void) ;
#endif // HEALTHD_BATTERY_MONTIOR_H
