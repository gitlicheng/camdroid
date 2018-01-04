#ifndef _EVENT_MANAGER_H
#define _EVENT_MANAGER_H

#include <hardware_legacy/uevent.h>
#include <hardware/sensors.h>

#include <utils/String8.h>

#include <utils/Thread.h>
#include <utils/Log.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "ConfigData.h"

#include "platform.h"
#include "BackCarMonitor.h"



extern int detectPlugIn(int fd);

using namespace android;

enum event_msg {
	EVENT_UVC_PLUG_IN	= 1,
	EVENT_UVC_PLUG_OUT,
	EVENT_CONNECT2PC,
	EVENT_DISCONNECFROMPC,
	EVENT_IMPACT,
	EVENT_BATTERY,
	EVENT_DELAY_SHUTDOWN,
	EVENT_CANCEL_SHUTDOWN,
	EVENT_MOUNT,
	EVENT_REMOVESD,
	EVENT_INSERTSD,
	EVENT_HDMI_PLUGIN,
	EVENT_HDMI_PLUGOUT,
	EVENT_TVD_PLUG_IN,
	EVENT_TVD_PLUG_OUT,
#ifdef APP_TVOUT
	EVENT_TVOUT_PLUG_IN,
	EVENT_TVOUT_PLUG_OUT,
#endif
	EVENT_REVERSE_OFF,
	EVENT_REVERSE_ON
};

enum {
	GSENSOR_CLOSE = 0,
	GSNESOR_LOW ,
	GSNESOR_MIDDLE ,
	GSNESOR_HIGH
};

enum{
	BATTERY_CHARGING = 1,
	BATTERY_DISCHARGING,
	BATTERY_NOTCHARGING,
	BATTERY_FULL
};


#define THRESHOLD_VALUE 18

#define HDMI_NODE "hdmi"
#define HDMI_ONLINE_FILE	"sys/devices/virtual/switch/hdmi/state"


#define UVC_NODE	"video"

#ifdef PLATFORM_0
	#define UVC_ADD		"add@/devices/platform/sw-ehci."
	#define UVC_REMOVE	"remove@/devices/platform/sw-ehci."
	#define USB_NODE				"usb"
	#define USB_ONLINE_FILE			"/sys/class/power_supply/usb/online"
#else
	#define UVC_ADD		"add@/devices/platform/sunxi-"
	#define UVC_REMOVE	"remove@/devices/platform/sunxi-"
	#define USB_NODE				"android0"
	#define USB_ONLINE_FILE			"/sys/class/android_usb/android0/state"
	#define USB_CONNECTED  "CONNECTED"
	#define USB_DISCONNECT "DISCONNECTED"
#endif
#define BATTERY_NODE			"battery"
#define POWER_BATTERY			"power_supply/battery"
#define UEVENT_CHANGE			"change"

#define VOLTAGE_WARNING1	3650000
#define VOLTAGE_WARNING2	3600000
#define BATTERY_VOLTAGE_FILE	"/sys/class/power_supply/battery/voltage_now"
#define BATTERY_ONLINE_FILE		"/sys/class/power_supply/battery/online"
#define BATTERY_CAPACITY			"/sys/class/power_supply/battery/capacity"
#define AC_ONLINE_FILE			"/sys/class/power_supply/ac/online"
#define USB_ONLINE              "/sys/class/power_supply/usb/online"
#define TVD_FILE_ONLINE			"/sys/devices/tvd/tvd"

#define CD_TFCARD_PATH "/dev/block/vold/179:0"
#define CD_TFCARD_LUN_PATH "/sys/class/android_usb/android0/f_mass_storage/lun/file"

#define BATTERY_STATUS_FILE                    "/sys/class/power_supply/battery/status"

//#ifdef FATFS
#define TFCARD_ADD_HEAD			"add@/devices/platform/sunxi-mmc."
#define TFCARD_END				"mmcblk0"
#define TFCARD_REMOVE_HEAD		"remove@/devices/platform/sunxi-mmc."
//#endif

class EventManager;

class EventListener
{
public:
		EventListener(){};
		virtual ~EventListener(){};
		virtual int notify(int message, int val) = 0;
};

class ImpactThread : public Thread
{
public:
	ImpactThread(EventManager *em);
	~ImpactThread();
	int init(void);
	int exit(void);
	void startThread()
	{
		run("ImpactThread", PRIORITY_NORMAL);
	}
	void stopThread()
	{
		requestExitAndWait();
	}
	bool threadLoop()
	{
		impactLoop();
		return true;
	}
private:
	int sensorCount;
	struct sensors_poll_device_t* device;
	struct sensor_t const *sensorList;
	
	EventManager *mEM;
	int impactInit(void);
	int impactLoop(void);
};


class EventManager : public ReverseEvent
{
public:
	EventManager(bool tvdOnline, bool hasTVD);
	~EventManager();
	int init(bool hasTVD);
	int exit(void);
	void connect2PC();
	void disconnectFromPC();
	int impactEvent();
	int checkUSB(char *udata);
	bool isBatteryOnline();
	static int getBattery();
	static int getBatteryStatus();
	int checkHDMI(const char *str);
	void detectTVD();
	void startDetectTVD();
	void hdmiAudioOn(bool open);
	void setGsensorThresholdValue(unsigned int data);
#ifdef APP_TVOUT
	void detectTVout();
	int startDetectTVout();
#endif
	void checkDevMore(int idx);
	int usbConnnect();
	bool checkValidVoltage();
	class UeventThread : public Thread
	{
	public:
		UeventThread(EventManager *em)
			: Thread(false)
			, mEM(em)
		{
		}
		void startThread()
		{
			run("UeventThread", PRIORITY_NORMAL);
		}
		void stopThread()
		{
			requestExitAndWait();
		}
		bool threadLoop()
		{
			mEM->ueventLoop();
			return true;
		}
	private:
		EventManager *mEM;
	};

	class TVDDetectThread : public Thread
	{
	public:
		TVDDetectThread(EventManager *em)
			: Thread(false),
			  mEM(em)
		{}
		void startThread()
		{run("TVDDetectThread", PRIORITY_NORMAL);}
		void stopThread()
		{requestExitAndWait();}
		bool threadLoop()
		{
			mEM->detectTVD();
			return true;
		}
	private:
		EventManager *mEM;
		bool mNeedStop;
		int mFD;
	};
#ifdef APP_TVOUT
	class TVoutDetectThread : public Thread
	{
	public:
		TVoutDetectThread(EventManager *em)
			: Thread(false),
			  mEM(em)
		{}
		void startThread()
		{run("TVoutDetectThread", PRIORITY_NORMAL);}
		void stopThread()
		{requestExitAndWait();}
		bool threadLoop()
		{
			mEM->detectTVout();
			return true;
		}
	private:
		EventManager *mEM;
	};
#endif
	class PCDisconnectTh : public Thread
	{
	public:
		PCDisconnectTh(EventManager *em)
			: Thread(false),
			  mEM(em)
		{}
		void startThread()
		{run("PCDisconnectTh", PRIORITY_NORMAL);}
		void stopThread()
		{requestExitAndWait();}
		bool threadLoop()
		{
			mEM->detectPCDisconnect();
			return true;
		}
	private:
		EventManager *mEM;
	};
	void setEventListener(EventListener *pListener);
	static void int2set(bool enable);
	void setGsensorSensitivity(int level);
	int	checkTFCard(char *udata);
	bool checkPCconnect(bool thread=false);
	void detectPCDisconnect();
	void reverseEvent(int cmd, int arg);
	void reverseDetect();

private:
	sp<UeventThread> mUT;
	sp<ImpactThread> mIT;
	sp<TVDDetectThread>mTVDDetectThread;
	EventListener *mListener;
	int ueventLoop(void);
	int checkBattery(int &level);
	int checkACPlug();
	bool mBatteryOnline;
	bool mUSBDisconnect;
	bool mTVDOnline;
	bool mNeedDelay;
	bool mACOnline;
#ifdef APP_TVOUT
	sp<TVoutDetectThread> mTVoutDetectThread;
#endif
	bool mNeedNotifyDiscon;
	bool mAC_PC;	//ac-usb use the common usb-port
	ReverseBase *mRB;
};

#endif
