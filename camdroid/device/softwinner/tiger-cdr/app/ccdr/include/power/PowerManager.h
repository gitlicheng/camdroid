#ifndef _POWER_MANAGER_H
#define _POWER_MANAGER_H

#include <utils/Thread.h>
#include <utils/Log.h>
#include <utils/Mutex.h>

#include <cutils/android_reboot.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <standby/IStandBy.h>

#define DISP_DEV "/dev/disp"
#include "windows.h"
enum
{
	SCREEN_ON = 0,
	SCREEN_OFF
};

#define DISP_CMD_LCD_ON		0x140
#define DISP_CMD_LCD_OFF	0x141

#define DISP_DEVICE_SWITCH	0x0F
#define DISP_BLANK  		0x0C


#ifdef DE2
typedef enum
{
	DISP_OUTPUT_TYPE_NONE   = 0,
	DISP_OUTPUT_TYPE_LCD    = 1,
	DISP_OUTPUT_TYPE_TV     = 2,
	DISP_OUTPUT_TYPE_HDMI   = 4,
	DISP_OUTPUT_TYPE_VGA    = 8,
}disp_output_type;

#endif

using namespace android;

#define BATTERY_HIGH_LEVEL 5
#define BATTERY_LOW_LEVEL 1
#define BATTERY_CHANGE_BASE 0

#define CPU_FREQ	"480000"

#define SCALING_MAX_FREQ  "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"
#define SCALING_GOVERNOR  "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
#define SCALING_SETSPEED  "/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed" 

#define CPUINFO_CUR_FREQ  "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"  

enum scaling_mode
{
	PERFORMANCE = 0,
	USERSPACE,
	POWERSAVE,
	ONDEMAND,
	CONSERVATIVE,
};

#endif
