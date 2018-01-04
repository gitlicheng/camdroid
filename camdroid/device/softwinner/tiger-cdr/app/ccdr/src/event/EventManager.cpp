#include "EventManager.h"
#include "StorageManager.h"
#include <TVDecoder.h>
#include <sys/mman.h> 
#include <linux/videodev.h> 
#include "debug.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EventManager"

void get_oneline(const char *pathname, String8 &val);
#define SHARE_VOL 1

static int mGSensorThresholdValue = THRESHOLD_MIDDLE_VALUE;

EventManager::EventManager(bool tvdOnline, bool hasTVD)
	: mUT(NULL)
	, mIT(NULL)
	, mTVDDetectThread(NULL)
	, mListener(NULL)
	, mBatteryOnline(false)
	, mUSBDisconnect(true)
	, mTVDOnline(tvdOnline)
	, mNeedDelay(false)
	, mACOnline(false)
	, mNeedNotifyDiscon(false)
	, mAC_PC(false)
{
	mNeedDelay = mTVDOnline;
	//init(hasTVD);
#ifdef PLATFORM_1
	cfgDataSetString("sys.usb.config", "mass_storage,adb");
#endif
}

EventManager::~EventManager()
{
	exit();
}

static status_t getSystem(int fd, int *system)
{
	struct v4l2_format format;

	memset(&format, 0, sizeof(format));

	format.type = V4L2_BUF_TYPE_PRIVATE;
	if (ioctl (fd, VIDIOC_G_FMT, &format) < 0)
	{
		db_error("<F:%s, L:%d>VIDIOC_G_FMT error!", __FUNCTION__, __LINE__);
		return UNKNOWN_ERROR;
	}

	if((format.fmt.raw_data[16] & 1) == 0)
	{
		ALOGV("<F:%s, L:%d>No signal detected", __FUNCTION__, __LINE__);
		return WOULD_BLOCK;
	}
	if((format.fmt.raw_data[16] & (1 << 4)) != 0)
	{
		*system = TVD_PAL;
	}
	else
	{
		*system = TVD_NTSC;
	}
	//ALOGV("format.fmt.raw_data[16] =0x%x",format.fmt.raw_data[16]);

	return NO_ERROR;
}

static int detectPlugInOut(void)
{
	int i;
	char linebuf[16];
	int val;
	i = 50;

    FILE *fp = fopen(TVD_FILE_ONLINE, "r");
	if (fp == NULL) {
		db_error("fail fopen %s\n", TVD_FILE_ONLINE);
		return -1;
	}
	while(i--) {
		if(fgets(linebuf, sizeof(linebuf), fp) == NULL) {
			db_error("fail fgets %s %s\n", TVD_FILE_ONLINE, strerror(errno));
			fclose(fp);
			return -1;
		}
		ALOGV("linebuf:%s", linebuf);
		val = atoi(linebuf);
		if (val == 0) {
			fclose(fp);
			return 0;
		} else if (val == 1) {
			fclose(fp);
		    return 1;
		}
		fseek(fp, 0, SEEK_SET);
	}
    fclose(fp);
	return -1;
}

int detectPlugIn(int fd)
{
	struct v4l2_format format;

	int system;
	status_t ret;
//RESET_PARAMS:
	ALOGV("TVD interface=%d, system=%d, format=%d, channel=%d", mInterface, mSystem, mFormat, mChannel);
	memset(&format, 0, sizeof(format));
	format.type = V4L2_BUF_TYPE_PRIVATE;
	format.fmt.raw_data[0] = TVD_CVBS;
	format.fmt.raw_data[1] = TVD_PAL;
	if (ioctl(fd, VIDIOC_S_FMT, &format) < 0)
	{
		db_error("<F:%s, L:%d>VIDIOC_S_FMT error(%s)!", __FUNCTION__, __LINE__, strerror(errno));
		return -1;
	}
	usleep(100000);

	format.fmt.raw_data[2] = TVD_PL_YUV420;

//TVD_CHANNEL_ONLY_1:
	format.fmt.raw_data[8]  = 1;	        //row
	format.fmt.raw_data[9]  = 1;	        //column
	format.fmt.raw_data[10] = 1;		//channel_index0
	format.fmt.raw_data[11] = 0;		//channel_index1
	format.fmt.raw_data[12] = 0;		//channel_index2
	format.fmt.raw_data[13] = 0;		//channel_index3
	if (ioctl(fd, VIDIOC_S_FMT, &format) < 0)
	{
		db_error("<F:%s, L:%d>VIDIOC_S_FMT error(%s)!", __FUNCTION__, __LINE__, strerror(errno));
		return -1;
	}
	for (int i = 0; i<100; i++)
	{
		ret = getSystem(fd, &system);
		if (ret == NO_ERROR)
		{
			//if (mSystem != system)
			//{
			//	mSystem = system;
			//	goto RESET_PARAMS;
			//}
			return 0;
		} else {
			usleep(10 *1000);
		}
	}
	//db_debug("<F:%s, L:%d>No signal detected", __FUNCTION__, __LINE__);
	return -1;
}

	
void EventManager::startDetectTVD()
{
	db_msg("----startDetectTVD");
	if (mTVDDetectThread == NULL) {
		mTVDDetectThread = new TVDDetectThread(this);
		mTVDDetectThread->startThread();
	}
}

void EventManager::detectTVD()
{
	if (mTVDOnline == false) {
		int fd = open ("/dev/video1", O_RDWR|O_NONBLOCK, 0);
		if (fd < 0) {
			//db_msg("fail to open /dev/video1 :%s\n", strerror(errno));
            if (detectPlugInOut() == 1) {
                if (mListener && 0 == mListener->notify(EVENT_TVD_PLUG_IN, 0)) {
                    db_msg("tvd:plug in");
                    mTVDOnline = true;
                }
            }
		} else {
    		if (detectPlugIn(fd) == 0) {
    			db_msg("tvd:plug in");
    			mTVDOnline = true;
    			close(fd);
				if (mListener)
    				mListener->notify(EVENT_TVD_PLUG_IN, 0);
    		} else {
    			close(fd);
    		}
		}
		mNeedDelay = false;
	} else {
		if (mNeedDelay) {
			usleep(500*1000);	//wait for tvin device to be ready
			mNeedDelay = false;
		}
		if (detectPlugInOut() == 0) {
			if (mListener && 0 == mListener->notify(EVENT_TVD_PLUG_OUT, 0)) {
				mTVDOnline = false;
				db_msg("tvd:plug out");
			}
		}
	}
	usleep(200*1000);
}

#ifdef APP_TVOUT
int EventManager::startDetectTVout()
{
	db_msg("----startDetectTVD");
	if (mTVoutDetectThread == NULL) {
		mTVoutDetectThread = new TVoutDetectThread(this);
		mTVoutDetectThread->startThread();
	}
	return 0;
}
void EventManager::detectTVout()
{
	unsigned int arg[4];
	unsigned int dac_status;
	int disp_fd = -1;
	static int current_state = 0;
	arg[0] = 0;
	arg[1] = 0; //DAC 0
	disp_fd = open("/dev/disp", O_RDWR);
	if(disp_fd <= 0){
		db_msg("open /dev/disp error\n");
		return;
	}		
    dac_status = ioctl(disp_fd, DISP_CMD_TV_GET_DAC_STATUS, arg);
	close(disp_fd);
	if(dac_status == 1)
	{
		if(mListener && !current_state)
		{
			db_msg("TVout EVENT_TVOUT_PLUG_IN\n");
			mListener->notify(EVENT_TVOUT_PLUG_IN, 0);
			current_state = 1;
		}
	}else
	{
		if(mListener && current_state)
		{
			db_msg("TVout EVENT_TVOUT_PLUG_OUT\n");
			mListener->notify(EVENT_TVOUT_PLUG_OUT, 0);
			current_state = 0;
		}
	}
	usleep(200*1000);
}

#endif

int EventManager::init(bool hasTVD)
{
	String8 val;
	uevent_init();
	if (mUT == NULL) {
		mUT = new UeventThread(this);
		mUT->startThread();
	}
#if 1
	if (mIT == NULL) {
		mIT = new ImpactThread(this);
		mIT->init();
	}
#endif
	get_oneline(BATTERY_ONLINE_FILE, val);
	mBatteryOnline = (atoi(val) == 1) ? true : false;

	if (hasTVD) {
		startDetectTVD();
	}
#ifdef APP_TVOUT
	startDetectTVout();
#endif
	reverseDetect();
	return 0;
}


int EventManager::exit(void)
{
	if (mIT != NULL) {
		mIT->exit();
		mIT.clear();
		mIT = NULL;
	}
	if (mUT != NULL) {
		mUT->stopThread();
		mUT.clear();
		mUT = NULL;
	}
	if (mTVDDetectThread != NULL) {
		mTVDDetectThread->stopThread();
		mTVDDetectThread.clear();
		mTVDDetectThread = NULL;
	}
	
	return 0;
}

void EventManager::hdmiAudioOn(bool open)
{
#if 1
	if (open) { //hdmi audio AUDIO_DEVICE_OUT_AUX_DIGITAL = 0x400
		cfgDataSetString("audio.routing", "1024");
	} else {	//normal audio AUDIO_DEVICE_OUT_SPEAKER = 0x2
		cfgDataSetString("audio.routing", "2");
	}
#endif
}

void EventManager::setEventListener(EventListener *pListener)
{
	mListener = pListener;
	int level;
	checkBattery(level);
	mListener->notify(EVENT_BATTERY, level);
}

bool EventManager::isBatteryOnline()
{
	return mBatteryOnline;
}

void get_oneline(const char *pathname, String8 &val)
{
	FILE * fp;
	char linebuf[1024];

	if(access(pathname, F_OK) != 0) {
		db_msg("%s not exsist\n", pathname);
		val = "";
		return ;
	}

	if(access(pathname, R_OK) != 0) {
		db_msg("%s can not read\n", pathname);
		val = "";
		return ;
	}

	fp = fopen(pathname, "r");

	if (fp == NULL) {
		val = "";
		return;
	}

	if(fgets(linebuf, sizeof(linebuf), fp) != NULL) {
		fclose(fp);
		val = linebuf;
		return ;
	}

	fclose(fp);
}

bool isUVCStr(const char *str)
{
	char buf[128]={""};
	const char *num;
	int idx;
	getBaseName(str, buf);
	db_msg("isUVCstr :%s", buf);
	if (strncmp(buf, UVC_NODE, strlen(UVC_NODE))) {
		return false;
	}
	num = buf+strlen(UVC_NODE);
	db_msg("isUVCstr num :%s", num);
	idx = atoi(num);
	if (0<idx && idx<64) {
		return true;
	}
	return false;
}

int checkUVC(char *udata)
{
	int ret = 0;
	const char *tmp_start;
	tmp_start = udata;

	if (isUVCStr(udata)) {
		if (!strncmp(tmp_start, UVC_ADD, strlen(UVC_ADD))) {
			db_msg("\nUVC Plug In\n");
			ret = EVENT_UVC_PLUG_IN;
			//mListener->notify(EVENT_UVC_PLUG_IN);	
		} else if (!strncmp(tmp_start, UVC_REMOVE, strlen(UVC_REMOVE))) {
			db_msg("\nUVC Plug Out\n");
			ret = EVENT_UVC_PLUG_OUT;
			//mListener->notify(EVENT_UVC_PLUG_OUT);
		} else {
			db_msg("unkown message for UVC\n");	
			db_msg("tmp_start is %s\n", tmp_start);
		}
	}
	return ret;
}

static inline int capacity2Level(int capacity)
{
	return ((capacity - 1) / 20 + 1);
}

int EventManager::checkACPlug()
{
	int ret = 0;
	String8 val_new("");
	
	get_oneline(BATTERY_ONLINE_FILE, val_new);
	bool battery_online = (atoi(val_new) == 1) ? true : false;
	db_msg("mBatteryOnline:%d, battery_online:%d mUSBDisconnect:%d", mBatteryOnline, battery_online, mUSBDisconnect);
	get_oneline(AC_ONLINE_FILE, val_new);
	bool ac_online = (atoi(val_new) == 1) ? true : false;
	mACOnline = ac_online;
	db_msg("mAConline:%d", mACOnline);
	if (mBatteryOnline==false && battery_online==true) {
		mBatteryOnline = battery_online;
		if (mUSBDisconnect == false || mACOnline) {
			return ret;
		}
		return EVENT_DELAY_SHUTDOWN;
	} else if(mBatteryOnline==true && battery_online==false) {
		mBatteryOnline = battery_online;
		return EVENT_CANCEL_SHUTDOWN;
	}
	
	return ret;
}

static bool isBatteryEvent(char *udata)
{
	const char *tmp_start;
	const char *tmp_end;
	tmp_start = udata;
	tmp_end = udata + strlen(udata) - strlen(POWER_BATTERY);
	
	if (strcmp(tmp_end, POWER_BATTERY)) { //battery
		return false;
	}
	if (strncmp(tmp_start, UEVENT_CHANGE, strlen(UEVENT_CHANGE))) { //change
		return false;
	}
	return true;
}

int EventManager::checkBattery(int &level)
{
	int ret = 0;
	String8 val("");
	int battery_capacity = 0;
	
	get_oneline(BATTERY_CAPACITY, val);
	battery_capacity = atoi(val.string());
	db_msg("cur battery capacity:%d%%/%d%%", battery_capacity, 100);
	level = capacity2Level(battery_capacity);
	//if (mBatteryOnline) {
		ret = EVENT_BATTERY;
	//}
	return ret;
}

int EventManager::getBattery()
{
	int ret = 0;
	String8 val("");
	int battery_capacity = 0;
	get_oneline(BATTERY_ONLINE_FILE, val);
	if (atoi(val.string()) != 1) {	//not use battery
		db_msg("has not use battery");
		return -1;
	}
	get_oneline(BATTERY_CAPACITY, val);
	battery_capacity = atoi(val.string());
	return battery_capacity;
}
int EventManager::usbConnnect()
{
	String8 val_usb;
	int ret = -1;
	get_oneline(USB_ONLINE_FILE, val_usb);
	if (strncmp(val_usb.string(),USB_CONNECTED, 9) == 0) {
		ret =  1;
	} else if (strncmp(val_usb.string(),USB_DISCONNECT, 9) == 0) {
		ret =  0;
	}
	return ret;
}

int EventManager::getBatteryStatus()
{
	char status[512]={0};
	FILE * fp;      
	if(access(BATTERY_STATUS_FILE, F_OK) != 0) {
	       db_msg("/sys/class/power_supply/battery/status not exsist\n");  
	}
	if(access(BATTERY_STATUS_FILE, R_OK) != 0) {
	       db_msg("/sys/class/power_supply/battery/status can not read\n");        
	}
	fp = fopen(BATTERY_STATUS_FILE, "r");
	if (fp == NULL) {
	       return 0;
	}       
	if(fgets(status, sizeof(status), fp) == NULL) {
	       db_msg("/sys/class/power_supply/battery/status read failed");
	       fclose(fp);
	       return 0;
	}
	fclose(fp);
	status[strlen(status)-1]='\0';
	if(strcmp(status,"Charging")==0){
	       return BATTERY_CHARGING;
	       
	}else if(strcmp(status,"Discharging")==0){
	       return BATTERY_DISCHARGING;     
	}else if(strcmp(status,"Not charging")==0){
	       return BATTERY_NOTCHARGING;             
	}else if(strcmp(status,"Full")==0){
	       return BATTERY_FULL;
	}
	return 0;

}



int EventManager::checkUSB(char *udata)
{
	const char *tmp_start;
	const char *tmp_end;
	tmp_start = udata;
	tmp_end = udata + strlen(udata) - strlen(USB_NODE);
	String8 val_usb;
	int ret = -1;
	if (strcmp(tmp_end, USB_NODE)) { //usb
		return -1;
	}

	if (strncmp(tmp_start, UEVENT_CHANGE, strlen(UEVENT_CHANGE))) { //change
		return -1;
	}
	bool usb_online;
	get_oneline(USB_ONLINE, val_usb);
	usb_online = (atoi(val_usb) == 1) ? true : false;
#ifdef PLATFORM_0
	get_oneline(USB_ONLINE_FILE, val_usb);
	usb_online = (atoi(val_usb) == 1) ? true : false;
	if (usb_online) {
		return EVENT_CONNECT2PC;
	}
	mUSBDisconnect = true;
	return EVENT_DISCONNECFROMPC;
#else
	if (usbConnnect() == 1) {
		mUSBDisconnect = false;
		mNeedNotifyDiscon = true;
		if (usb_online == false) {
			mAC_PC = true;
		}
		ret =  EVENT_CONNECT2PC;
	} else if (usbConnnect() == 0){
		mUSBDisconnect = true;
		if (mAC_PC) {
			ret =  EVENT_DISCONNECFROMPC;
		}
	} else {
		ret = -1;
	}
	db_msg("val_usb %s", val_usb.string());
/*
		//db_msg("val_usb:%s", val_usb.string());
	if (strncmp(val_usb.string(),USB_CONNECTED, 9) == 0) {
		mUSBDisconnect = false;
		ret =  EVENT_CONNECT2PC;
	} else if (strncmp(val_usb.string(),USB_DISCONNECT, 9) == 0) {
		mUSBDisconnect = true;
		ret =  EVENT_DISCONNECFROMPC;
	} else {
		db_msg("val_usb %s", val_usb.string());
		ret = -1;
	}
*/
	return ret;
#endif
}
//#ifdef FATFS
int EventManager::checkTFCard(char *udata)
{
	const char *tmp_start;
	const char *tmp_end;
	tmp_start = udata;
	int ret = -1;
	tmp_end = udata + strlen(udata) - strlen(TFCARD_END);
	db_msg("tmp_start:%s",tmp_start);
	db_msg("tmp_end:%s",tmp_end);
	if (strcmp(tmp_end, TFCARD_END)) {
		return -1;
	}
	if (strncmp(tmp_start, TFCARD_ADD_HEAD, strlen(TFCARD_ADD_HEAD))==0) {
		return EVENT_INSERTSD;
	}
	if (strncmp(tmp_start, TFCARD_REMOVE_HEAD, strlen(TFCARD_REMOVE_HEAD))==0) {
		return EVENT_REMOVESD;
	}
	return -1;

}
//#endif
static bool getValue(const char *str)
{
	String8 val;
	get_oneline(str, val);
	return (atoi(val)==1)?true:false;
}

int EventManager::checkHDMI(const char *str)
{
	char buf[32];
	getBaseName(str, buf);
	bool val;
	if (strncmp(buf, HDMI_NODE, strlen(HDMI_NODE))) {
		return 0;
	}
	if (strncmp(str, UEVENT_CHANGE, strlen(UEVENT_CHANGE))) { //change
		return 0;
	}
	val = getValue(HDMI_ONLINE_FILE);
	if (val) {
		return EVENT_HDMI_PLUGIN;
	}
	return EVENT_HDMI_PLUGOUT;
}

int EventManager::ueventLoop(void)
{
	char udata[4096] = {0};

	uevent_next_event(udata, sizeof(udata) - 2);
	
	db_msg("uevent_loop %s\n", udata);
	if(mListener) {
		int ret = 0;
		int val = 4;
		ret = checkUVC(udata);
		if (ret > 0) {
			if (ret == EVENT_UVC_PLUG_IN) {
				mRB->setEnable(true);
			} else if (ret == EVENT_UVC_PLUG_OUT) {
				mRB->setEnable(false);
			}
			mListener->notify(ret, 0);
			return ret;
		}
		ret = checkUSB(udata);
		if (ret > 0) {
			mListener->notify(ret, 0);
			return ret;
		}
		//#ifdef FATFS
		ret = checkTFCard(udata);
		if (ret > 0) {
			mListener->notify(ret, 0);
			return ret;
		}
		//#endif
		if(isBatteryEvent(udata)) {
			ret = checkACPlug();
			if (ret > 0) {
				if (ret == EVENT_DELAY_SHUTDOWN && mNeedNotifyDiscon) {
					mListener->notify(EVENT_DISCONNECFROMPC, 0);
					mNeedNotifyDiscon = false;
				}
				mListener->notify(ret, val);
				return ret;
			}
			ret = checkBattery(val);
			if (ret > 0) {
				mListener->notify(ret, val);
				return ret;
			}
		}
#ifdef PLATFORM_0
		ret = checkHDMI(udata);
		if (ret > 0) {
			mListener->notify(ret, 0);
			return ret;
		}
#endif
		db_msg("The event is not handled\n");	
	}

	return 0;
}
void EventManager::connect2PC()
{
#ifdef PLATFORM_0
	cfgDataSetString("sys.usb.config", "mass_storage");
#endif
#ifdef SHARE_VOL
	StorageManager *sm = StorageManager::getInstance();
	sm->shareVol();
#else
	int fd = open(CD_TFCARD_LUN_PATH, O_RDWR);
	write(fd, CD_TFCARD_PATH, strlen(CD_TFCARD_PATH));
	close(fd);
#endif
}

void EventManager::disconnectFromPC()
{
#ifdef PLATFORM_1
	//cfgDataSetString("sys.usb.config", "mass_storage,adb");
    StorageManager *sm = StorageManager::getInstance();
	sm->unShareVol();
#else
	cfgDataSetString("sys.usb.config", "uvc");
#endif
}

char const* getSensorName(int type) {
    switch(type) {
        case SENSOR_TYPE_ACCELEROMETER:
            return "Acc";
        case SENSOR_TYPE_MAGNETIC_FIELD:
            return "Mag";
        case SENSOR_TYPE_ORIENTATION:
            return "Ori";
        case SENSOR_TYPE_GYROSCOPE:
            return "Gyr";
        case SENSOR_TYPE_LIGHT:
            return "Lux";
        case SENSOR_TYPE_PRESSURE:
            return "Bar";
        case SENSOR_TYPE_TEMPERATURE:
            return "Tmp";
        case SENSOR_TYPE_PROXIMITY:
            return "Prx";
        case SENSOR_TYPE_GRAVITY:
            return "Grv";
        case SENSOR_TYPE_LINEAR_ACCELERATION:
            return "Lac";
        case SENSOR_TYPE_ROTATION_VECTOR:
            return "Rot";
        case SENSOR_TYPE_RELATIVE_HUMIDITY:
            return "Hum";
        case SENSOR_TYPE_AMBIENT_TEMPERATURE:
            return "Tam";
    }
    return "ukn";
}
void EventManager::setGsensorThresholdValue(unsigned int data)
{
	if(data == GSNESOR_LOW)
		mGSensorThresholdValue = THRESHOLD_LOW_VALUE;
	else if(data == GSNESOR_HIGH)
		mGSensorThresholdValue = THRESHOLD_HIGH_VALUE;
	else if(data == GSNESOR_MIDDLE)
		mGSensorThresholdValue = THRESHOLD_MIDDLE_VALUE;
	else if(data == GSENSOR_CLOSE)
		mGSensorThresholdValue = -1;
}

ImpactThread::ImpactThread(EventManager *em)
	: sensorCount(0),
	  device(NULL),
	  sensorList(NULL),
	  mEM(em)
{
	ALOGV("ImpactMonitor Constructot\n");
}

ImpactThread::~ImpactThread()
{

}

int ImpactThread::init(void)
{
	if(impactInit() < 0) {
		db_msg("init sensors failed\n");
		exit();
		return -1;
	}
	if (sensorCount > 0) {
		startThread();
	}
	
	return 0;
}

int ImpactThread::impactInit(void)
{
	int err;
	struct sensors_module_t* module;

	err = hw_get_module(SENSORS_HARDWARE_MODULE_ID, (hw_module_t const**)&module);
	if (err != 0) {
		db_error("hw_get_module() failed (%s)\n", strerror(-err));
		return -1;
	}

	err = sensors_open(&module->common, &device);
	if (err != 0) {
		db_error("sensors_open() failed (%s)\n", strerror(-err));
		return -1;
	}

	sensorCount = module->get_sensors_list(module, &sensorList);
	db_debug("%d sensors found:\n", sensorCount);
	if (sensorCount == 0) {
		return -1;
	}
	for (int i=0 ; i < sensorCount ; i++) {
		db_msg("%s\n"
				"\tvendor: %s\n"
				"\tversion: %d\n"
				"\thandle: %d\n"
				"\ttype: %d\n"
				"\tmaxRange: %f\n"
				"\tresolution: %f\n"
				"\tpower: %f mA\n",
				sensorList[i].name,
				sensorList[i].vendor,
				sensorList[i].version,
				sensorList[i].handle,
				sensorList[i].type,
				sensorList[i].maxRange,
				sensorList[i].resolution,
				sensorList[i].power);
	}

	for (int i = 0 ; i < sensorCount ; i++) {
		err = device->activate(device, sensorList[i].handle, 0);
		if (err != 0) {
			db_error("deactivate() for '%s'failed (%s)\n",
					sensorList[i].name, strerror(-err));
			return -1;
		}
	}

	for (int i = 0 ; i < sensorCount ; i++) {
		err = device->activate(device, sensorList[i].handle, 1);
		if (err != 0) {
			db_error("activate() for '%s'failed (%s)\n",
					sensorList[i].name, strerror(-err));
			return -1;
		}
		device->setDelay(device, sensorList[i].handle, ms2ns(200));
	}

	db_debug("init sensors success\n");
	return 0;
}

int ImpactThread::exit(void)
{
	int err;
	for (int i = 0 ; i < sensorCount ; i++) {
		err = device->activate(device, sensorList[i].handle, 0);
		if (err != 0) {
			db_error("deactivate() for '%s'failed (%s)\n", sensorList[i].name, strerror(-err));
		}
	}

	ALOGV("------------------------------------------\n");
	err = sensors_close(device);
	ALOGV("------------------------------------------\n");
	if (err != 0) {
		db_error("sensors_close() failed (%s)\n", strerror(-err));
	}
	if (sensorCount > 0) {
		stopThread();
	}

	return 0;
}

int ImpactThread::impactLoop(void)
{
	static const size_t numEvents = 16;
	sensors_event_t buffer[numEvents];
	int n;
	n = device->poll(device, buffer, numEvents);
	if (n < 0) {
		db_error("poll() failed\n");
		return -1;
	}
	for (int i=0 ; i<n ; i++) {
		sensors_event_t& data = buffer[i];

		if (data.version != sizeof(sensors_event_t)) {
			db_error("incorrect event version (version=%d, expected=%u", data.version, sizeof(sensors_event_t));
			return -1;
		}

		switch(data.type) {
		case SENSOR_TYPE_ACCELEROMETER:
		case SENSOR_TYPE_MAGNETIC_FIELD:
		case SENSOR_TYPE_ORIENTATION:
		case SENSOR_TYPE_GYROSCOPE:
		case SENSOR_TYPE_GRAVITY:
		case SENSOR_TYPE_LINEAR_ACCELERATION:
		case SENSOR_TYPE_ROTATION_VECTOR:
			if(mGSensorThresholdValue == -1)
				break;
			if (data.data[0] > mGSensorThresholdValue ||
					data.data[1] > mGSensorThresholdValue ||
					data.data[2] > mGSensorThresholdValue)
			{

			db_msg("mGsensorThresholdValue = %d (%f %f %f) type%d\n", mGSensorThresholdValue,
				data.data[0],
				data.data[1],
				data.data[2],
				data.type);
				db_msg("impact occur!");
				mEM->impactEvent();
			}
			break;

		case SENSOR_TYPE_LIGHT:
		case SENSOR_TYPE_PRESSURE:
		case SENSOR_TYPE_TEMPERATURE:
		case SENSOR_TYPE_PROXIMITY:
		case SENSOR_TYPE_RELATIVE_HUMIDITY:
		case SENSOR_TYPE_AMBIENT_TEMPERATURE:
			ALOGV("sensor=%s, time=%lld, value=%f\n", getSensorName(data.type), data.timestamp, data.data[0]);
			break;

		default:
			db_msg("sensor=%d, time=%lld, value=<%f,%f,%f, ...>\n",
					data.type,
					data.timestamp,
					data.data[0],
					data.data[1],
					data.data[2]);
			break;
		}
	}

	return 0;
}

int EventManager::impactEvent()
{
	if (mListener) {
		mListener->notify(EVENT_IMPACT, 0);
	}
	return 0;
}

int getGsensorNode()
{
	int i = 0;
	
	char path[64] = {0};
	for(i=0; i<8; i++) {
		sprintf(path, "/sys/class/input/input%d/int2_enable", i);
		if (access(path, F_OK) == 0) {
	        return i;
	    }
	}
	return -1;
}

void EventManager::setGsensorSensitivity(int level)
{
	char path[64] = {0}; 
	int idx = getGsensorNode(); 
	char value[2]={0}; 
	if (level == GSNESOR_HIGH) {
		value[0] = '3';
	} else if (level == GSNESOR_LOW) {
		value[0] = 'f';
	} else {
		value[0] = '5';
	}
	sprintf(path, "/sys/class/input/input%d/slope_th", idx);
	int fd = open(path, O_RDWR);
	if( fd > 0 ){
		write(fd,value,2);
		close(fd);
	}
}

void EventManager::int2set(bool enable)
{
	char path[64] = {0};
	int idx = getGsensorNode();
	char value[2]={0};
	
	if (idx < 0) {
		return ;
	}
	sprintf(path, "/sys/class/input/input%d/int2_enable", idx);
	
	int fd = open(path, O_RDWR);
	if( fd > 0 ){
		value[0] = enable ? '1' : '0';
		db_msg(":disable int2 initially ");
		write(fd,value,2);
		close(fd);
	}
}

void EventManager::checkDevMore(int idx)
{
	char dev[32];
	int i = 0;
	struct v4l2_capability cap;
	int ret = 0;

	for (i = 1; i < 8; ++i) {
		sprintf(dev, "/dev/video%d", i);
		if (!access(dev, F_OK)) {
			int fd = open(dev, O_RDWR | O_NONBLOCK, 0);
			if (fd < 0) {
				continue;
			}
			ret = ioctl (fd, VIDIOC_QUERYCAP, &cap);
			if (ret < 0) {
				close(fd);
				continue;
			}
			if (strcmp((char*)cap.driver, "uvcvideo") == 0) {
				db_debug("uvcvideo device node is %s", dev);
				close(fd);
				mListener->notify(EVENT_UVC_PLUG_IN, 0);
			} else if (strcmp((char*)cap.driver, "tvd") == 0) {
				db_debug("TVD device node is %s", dev);
				if (detectPlugIn(fd) == 0) {
					db_msg("tvd:plug in");
					close(fd);
					mListener->notify(EVENT_UVC_PLUG_IN, 0);
				}
			}
			close(fd);
			fd = -1;
		}
	}
}

bool EventManager::checkValidVoltage()
{
	String8 val("4200000");
	int voltage_now = 0;
	get_oneline(BATTERY_VOLTAGE_FILE, val);
	voltage_now = atoi(val);
	if (voltage_now < VOLTAGE_WARNING1 || voltage_now < VOLTAGE_WARNING2) {
		sys_log("voltage_now %d < VOLTAGE_WARNING1:%d, VOLTAGE_WARNING2:%d",
			voltage_now, VOLTAGE_WARNING1, VOLTAGE_WARNING2);
		return false;
	}
	return true;
}

void EventManager::detectPCDisconnect()
{
	char *check_file = USB_ONLINE;
	String8 val("0");
	get_oneline(check_file, val);
	bool online = (bool)atoi(val.string());
	while (online) {
		usleep(100*1000);
		get_oneline(check_file, val);
		online = (bool)atoi(val.string());
	}
	mListener->notify(EVENT_DISCONNECFROMPC, 0);
	db_msg("thread detectPCDisconnect finished");
}

bool EventManager::checkPCconnect(bool thread)
{
	char *check_file = USB_ONLINE;
	String8 val("0");
	if (mAC_PC) {
		check_file = AC_ONLINE_FILE;
		get_oneline(check_file, val);
		bool online = (bool)atoi(val.string());
		return online;
	} else if (thread) {
		sp<PCDisconnectTh> pcdTH = new PCDisconnectTh(this);
		pcdTH->startThread();
		pcdTH.clear();
		pcdTH = NULL;
	}
	return false;
}

void EventManager::reverseEvent(int cmd, int arg)
{
	if (cmd == REVERSE_EVENT_OFF) {
		mListener->notify(EVENT_REVERSE_OFF, 0);
	} else if (cmd == REVERSE_EVENT_ON){
		mListener->notify(EVENT_REVERSE_ON, 0);
	}
}

void EventManager::reverseDetect()
{
	mRB = new BackCarMonitor();
	mRB->setListener(this);
	mRB->setEnable(true);
}
