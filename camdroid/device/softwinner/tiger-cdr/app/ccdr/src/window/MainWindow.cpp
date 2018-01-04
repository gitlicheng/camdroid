
#include "MainWindow.h"
#ifdef LOG_TAG 
#undef LOG_TAG
#endif
#define LOG_TAG "MainWindow.cpp"
#include "debug.h"

#include "posixTimer.h"
#include <DataStruct.h>
using namespace android;

//#define NOT_TO_SHUTDOWN
static timer_set timer_data[4];
static bool isOTAUpdate = false;
int delayShutdownTime[3][3] = {{18, 3},{33, 3},{63, 3}};
static bool pcDialogFinish = true;
static void closeConnectToPCDialog(void)
{
	BroadcastMessage(MSG_CLOSE_CONNECT2PC_DIALOG, 0, 0);
}

void ready2ShutDown(sigval_t sg)
{
#ifdef NOT_TO_SHUTDOWN
	/*ResourceManager* rm = ResourceManager::getInstance();
	if (rm->getResBoolValue(ID_MENU_LIST_DELAY_SHUTDOWN))
	{
		return ;
	}	*/
#endif
	//db_msg("whether to shut down");
	timer_set *data = (timer_set*)sg.sival_ptr;
	int idx = data->idx;
	CdrMain* cdrMain = (CdrMain*)data->context;

	db_msg("timer idx is %d", idx);
	if (cdrMain->getWakeupState()) {
		if(idx == TIMER_PARK_RECORD_IDX){
			RecordPreview * mRecordPre = cdrMain->getRecordPreview();
			if(mRecordPre->isRecording()){
				mRecordPre->stopRecord();
			}
			cdrMain->setWakeupState(false);
			if(!cdrMain->isBatteryOnline()){
				int idx;
				ResourceManager *rm = ResourceManager::getInstance();
				idx = rm->getResIntValue(ID_MENU_LIST_VTL, INTVAL_SUBMENU_INDEX);
				mRecordPre->setRecordTime(idx);
				mRecordPre->startRecord();
				return ;	
			}else{
				cdrMain->shutdown(true);
			}
		}
		return ;
	}
	
	if (cdrMain->isBatteryOnline()) {
		if (idx == TIMER_NOWORK_IDX2) {
			db_info("TIMER_NOWORK_IDX2\n");
			if(cdrMain->isShuttingdown() == false) {
				CloseTipLabel();
				SendNotifyMessage(cdrMain->getHwnd(), MSG_SHOW_TIP_LABEL, LABEL_30S_NOWORK_SHUTDOWN,
					(timer_data[TIMER_NOWORK_IDX].time-timer_data[TIMER_NOWORK_IDX2].time));

			}
			return;
		} else if (idx == TIMER_NOWORK_IDX){
			db_msg("timerid nowork");
			/*
			if (cdrMain->isRecording()) {
				db_msg("isRecording, no need to shutdown");
				goto out;
			}
			if (cdrMain->isPlaying()) {
				db_msg("isStarted, no need to shutdown");
				goto out;
			}
			*/
		}
//   	SendNotifyMessage(cdrMain->getHwnd(), MSG_SHOW_TIP_LABEL, LABEL_SHUTDOWN_NOW, 0);
		//if(cdrMain->getRecordPrevew()->isRecording()){
		//	cdrMain->getRecordPrevew()->stopRecord();
		//}
		db_msg("**************shutdown()");
		cdrMain->shutdown(true);
		db_msg("now shutdown");
		return;
	}

out:
	if(idx == TIMER_NOWORK_IDX) {
		//db_msg("setOneShotTimer");
		setOneShotTimer(timer_data[TIMER_NOWORK_IDX2].time, 0, timer_data[TIMER_NOWORK_IDX2].id);
	}
}

void CdrMain::startShutdownTimer(int idx)
{
	int ret;
	db_msg("startShutdownTimer %d", idx);
	if (timer_data[idx].id == 0) {
		ret = createTimer((void*)&timer_data[idx], &timer_data[idx].id, ready2ShutDown);
		db_msg("createTimer, timerId is 0x%lx\n", (unsigned long)timer_data[idx].id);
	}
	if (timer_data[idx].id) {
		if (idx == TIMER_LOWPOWER_IDX || idx == TIMER_NOWORK_IDX2 || idx == TIMER_PARK_RECORD_IDX) {
			db_msg("setOneShotTimer");
			setOneShotTimer(timer_data[idx].time, 0, timer_data[idx].id);
		} else if(idx == TIMER_NOWORK_IDX){
			db_msg("setPeriodTimer");
			setPeriodTimer(timer_data[idx].time, 0, timer_data[idx].id);
		}
		db_msg("#Warning: System will be shut down");
	}
}

void CdrMain::initTimerData()
{

	ResourceManager * rm = ResourceManager::getInstance();
	int data = rm->getResIntValue(ID_MENU_LIST_DELAY_SHUTDOWN, INTVAL_SUBMENU_INDEX);
	db_msg("***********data=%d************\n",data);
	if( data == 0 ) {
			timer_data[TIMER_NOWORK_IDX].time = delayShutdownTime[0][0];
			timer_data[TIMER_NOWORK_IDX2].time = delayShutdownTime[0][1];
			isAutoShutdownEnabled = true;
	} else if( data == 1 ) {
			timer_data[TIMER_NOWORK_IDX].time = delayShutdownTime[1][0];
			timer_data[TIMER_NOWORK_IDX2].time = delayShutdownTime[1][1];
			isAutoShutdownEnabled = true;
	} else if( data == 2 ) {
			timer_data[TIMER_NOWORK_IDX].time = delayShutdownTime[2][0];
			timer_data[TIMER_NOWORK_IDX2].time = delayShutdownTime[2][1];
			isAutoShutdownEnabled = true;
	} else {
			isAutoShutdownEnabled = false;
	}

	timer_data[TIMER_LOWPOWER_IDX].context = this;
	timer_data[TIMER_LOWPOWER_IDX].id = 0;
	timer_data[TIMER_LOWPOWER_IDX].time = LOWPOWER_SHUTDOWN_TIME;
	timer_data[TIMER_LOWPOWER_IDX].idx = TIMER_LOWPOWER_IDX;
	timer_data[TIMER_NOWORK_IDX].context = this;
	timer_data[TIMER_NOWORK_IDX].id = 0;
//	timer_data[TIMER_NOWORK_IDX].time = NOWORK_SHUTDOWN_TIME;
	timer_data[TIMER_NOWORK_IDX].idx = TIMER_NOWORK_IDX;
	timer_data[TIMER_NOWORK_IDX2].context = this;
	timer_data[TIMER_NOWORK_IDX2].id = 0;
//	timer_data[TIMER_NOWORK_IDX2].time = NOWORK_SHUTDOWN_TIME2;
	timer_data[TIMER_NOWORK_IDX2].idx = TIMER_NOWORK_IDX2;

	timer_data[TIMER_PARK_RECORD_IDX].context = this;
	timer_data[TIMER_PARK_RECORD_IDX].id = 0;
	timer_data[TIMER_PARK_RECORD_IDX].time = PARK_MON_VTL_15S;
	timer_data[TIMER_PARK_RECORD_IDX].idx = TIMER_PARK_RECORD_IDX;
}

void CdrMain::modifyNoworkTimerInterval(time_t  time, time_t  time2)
{
       timer_data[TIMER_NOWORK_IDX].time = time;
       timer_data[TIMER_NOWORK_IDX2].time = time2;
       if (isBatteryOnline())
               noWork(true);
}

void CdrMain::noWork(bool idle)
{
	if (idle && isAutoShutdownEnabled ) {
		if(mShutdowning == true) {
			db_msg("shutting down, not set timer\n");
			return;
		}
		db_msg("no work, resume timer");
		startShutdownTimer(TIMER_NOWORK_IDX);
		startShutdownTimer(TIMER_NOWORK_IDX2);
	} else if(idle == false){
		db_msg("working, stop timer");
//		cancelShutdownTimer(TIMER_NOWORK_IDX);
		stopShutdownTimer(TIMER_NOWORK_IDX);
		stopShutdownTimer(TIMER_NOWORK_IDX2);
		CloseTipLabel();
	}
}

void CdrMain::cancelShutdownTimer(int idx)
{
	db_msg("cancelShutdownTimer");
	if (timer_data[idx].id != 0) {
		db_msg("deleteTimer");
		deleteTimer(timer_data[idx].id);
		timer_data[idx].id = 0;
	}
}

void CdrMain::stopShutdownTimer(int idx)
{
	db_msg("stopShutdownTimer");
	if (timer_data[idx].id != 0) {
		db_msg("stopShutdownTimer");
		stopTimer(timer_data[idx].id);
	}
}

bool CdrMain::isRecording()
{
	return mRecordPreview->isRecording();
}

bool CdrMain::isPlaying()
{
	if (mPlayBack) {
		return mPlayBack->isStarted();
	}
	return false;
}

/************* MainWindow Proc ***************/
static int CDRWinProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
	CdrMain* mCdrMain = (CdrMain*)GetWindowAdditionalData(hWnd);

	//db_msg("message is %s\n", Message2Str(message));
	switch(message) {
	case MSG_FONTCHANGED:
		mCdrMain->updateWindowFont();
		break;
	case MSG_CREATE:
		break;
		/* + + + keyEvent + + + */
	case MSG_PAINT: 
		if(mCdrMain->getIsLowPower()){
			HDC	hdc=BeginPaint(hWnd);
			BITMAP bitImage;
			char *filepath=(char *)"/system/res/others/low_power_notice.png";
			LoadBitmapFromFile(HDC_SCREEN, &bitImage, filepath);
			int screenW,screenH;
			getScreenInfo(&screenW, &screenH);
			FillBoxWithBitmap(hdc,0,0,screenW,screenH,&bitImage);
			EndPaint(hWnd,hdc); 
		}
		break;
	case MSG_KEYDOWN:
		if(mCdrMain->isKeyUp == true) {
			mCdrMain->downKey = wParam;	
			SetTimer(hWnd, ID_TIMER_KEY, LONG_PRESS_TIME);
			mCdrMain->isKeyUp = false;
		}
		break;
	case MSG_KEYUP:
		mCdrMain->isKeyUp = true;
		if(wParam == mCdrMain->downKey)	{
			KillTimer(hWnd, ID_TIMER_KEY);
			db_msg("short press\n");
			mCdrMain->keyProc(wParam, SHORT_PRESS);
		}
		break;
	case MSG_KEYLONGPRESS:
		mCdrMain->downKey = -1;
		db_msg("long press\n");
		mCdrMain->keyProc(wParam, LONG_PRESS);
		break;
	case MSG_TIMER:
		if(wParam == ID_TIMER_KEY) {
			mCdrMain->isKeyUp = true;			
			SendMessage(hWnd, MSG_KEYLONGPRESS, mCdrMain->downKey, 0);
			KillTimer(hWnd, ID_TIMER_KEY);
		}
		break;
		/* - - - keyEvent - - - */
	case MWM_CHANGE_WINDOW:
		mCdrMain->changeWindow(wParam, lParam);
		break;
	case MSG_CLOSE:
		DestroyMainWindow (hWnd);
		PostQuitMessage (hWnd);
		break;
	case MSG_SHOW_TIP_LABEL:
		if (mCdrMain->isShuttingdown()) {
			break;
		}
		if((enum labelIndex)wParam == LABEL_30S_NOWORK_SHUTDOWN){
			char ptext[4]={0};
			sprintf(ptext,"%d",(int)lParam);
			ShowTipLabel(hWnd, (enum labelIndex)wParam, TRUE, 3*1000,true,ptext);
			break;
		}
		if (lParam == 0){
			ShowTipLabel(hWnd, (enum labelIndex)wParam);
		}
		else{
			ShowTipLabel(hWnd, (enum labelIndex)wParam, TRUE, lParam);
		}
		break;
	case MSG_WIFI_SWITCH:
		mCdrMain->wifiEnable((bool)wParam);
		break;
	case MSG_RESET_WIFI_PWD:
		mCdrMain->fResetWifiPWD();
		break;
	case MSG_OTA_UPDATE:
		mCdrMain->otaUpdate();
		//mCdrMain->cdrOtaUpdate();
		break;
	case MSG_KEEP_ADAS_SCREEN_FLAG:
		mCdrMain->keepAdasScreenFlag();
		break;
	case MSG_RECOVER_ADAS_SCREEN_FLAG:
		mCdrMain->recoverAdasScreenFlag();
		break;
	case MSG_DESTROY:
		break;
	default:
		mCdrMain->msgCallback(message, wParam);
	}
	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

static bool inline isKeyEvent(int msg)
{
	if (msg != MSG_KEYDOWN && msg != MSG_KEYUP && msg != MSG_KEYLONGPRESS) {
		return false;	
	}
	return true;
}

int MsgHook(void* context, HWND dst_wnd, int msg, WPARAM wparam, LPARAM lparam)
{
	int ret = HOOK_GOON;
	
	CdrMain *cdrMain = (CdrMain*)context;
	HWND hMainWnd = cdrMain->getHwnd();

	if(cdrMain->mShutdowning == true || !cdrMain->mFinishConfig) {
		db_msg("shutdowning or resource problems");
		return !HOOK_GOON;
	}
	if (isKeyEvent(msg)) {
		if(msg == MSG_KEYDOWN && (wparam == CDR_KEY_POWER || cdrMain->isScreenOn())){
			StorageManager *sm = StorageManager::getInstance();
			if (cdrMain->isPhotoWindow() && wparam == CDR_KEY_OK && sm->isMount()) {
                 //cdrMain->playSounds(TAKEPIC_SOUNDS);
           } else {
				cdrMain->playSounds(KEY_SOUNDS);
           }
		}
		if (wparam == CDR_KEY_POWER) {  //power key, post to main proc
			SendMessage(hMainWnd, msg, wparam,lparam);
			return !HOOK_GOON; //need not other work
		#ifdef ALLKEY_AWAKE_SCREEN
		} else if (!cdrMain->isScreenOn()) {
			cdrMain->playSounds(KEY_SOUNDS);
			cdrMain->keyProc(CDR_KEY_POWER, false);
			return !HOOK_GOON;
		#endif
		} else {
			if (cdrMain->isWifiConnecting()) {
				return !HOOK_GOON;
			}
			if(cdrMain->isScreenOn()){
				cdrMain->setOffTime();
			}
		}
	}
	/* v3 todo :temporarily cancle */
	if (!cdrMain->isScreenOn()) { //screen off
		return !HOOK_GOON; //nothing to do
	}
	return ret;
}

CdrMain::CdrMain()
	: mShutdowning(false)
	, mSTB(NULL)
	, mRecordPreview(NULL)
	, mPlayBackPreview(NULL)
	, mPlayBack(NULL)
	, mMenu(NULL)
	, mPM(NULL)
	, mEM(NULL)
	, mEnableRecord(FORBID_NORMAL)
	, usbStorageConnected(false)
	, isWakenUpOnParking(false)
	, mTime(TIME_INFINITE)
	, mOtherWindowInited(false)
	, mConnManager(NULL)
	, mSessionConnect(false)
	, mFormatTFAdasS(false)
	, mNeedCheckSD(true)
	, mTinyParkAudioTh(NULL)
	, mGSensorVal(1)
	, downKey(0)
	, isKeyUp(true)
	, isLongPress(false)
	, mFinishConfig(false)
	, mPCCam(false)
	, m2pcConnected(false)
	, mReverseMode(false)
	, mReverseChanged(false)
{
	#ifdef COMPILE_VERSION
		sys_log("[Firmware Version: %s]\n", COMPILE_VERSION);
	#endif
	sp<ProcessState> proc(ProcessState::self());
	ProcessState::self()->startThreadPool();

	struct timespec measureTime1;
	clock_gettime(CLOCK_MONOTONIC, &measureTime1);
//	db_msg("Test Time: start cdr, time is %ld secs, %ld nsecs\n", measureTime1.tv_sec, measureTime1.tv_nsec);
	
	mLowPower = EventManager::getBattery();
	
	if (mLowPower < 0) {
		mLowPower = LOW_POWER+1;
		sys_log("===no need to check battery capacity ===");
	} else {
		sys_log("===power capacity: %d%%/%d%%===", mLowPower, 100);
	}
	EventManager::int2set(false);
	int batteryStatus = EventManager::getBatteryStatus();
	if(mLowPower > LOW_POWER || (batteryStatus == BATTERY_CHARGING)){
		mRecordPreview =  new RecordPreview(this);
	}
	mOtherDialogOpened = false;
	mOriginAdasScreen = false;
	mOTAAdasScreen = false;
	mDataCheckThread = new DataCheckThread(this);
}



CdrMain::~CdrMain()
{
#ifdef APP_WIFI
	if (mConnManager) {
		delete mConnManager;
		mConnManager = NULL;
	}
	if (mCdrServer) {
		delete mCdrServer;
		mCdrServer = NULL;
	}
#endif
	if(mRecordPreview) {
		delete mRecordPreview;
		mRecordPreview = NULL;
	}
	if (mEM) {
		delete mEM;
		mEM = NULL;
	}
	if (mPM) {
		delete mPM;
		mPM = NULL;
	}
	if(mSTB) {
		delete mSTB;
		mSTB = NULL;
	}
	if(mPlayBackPreview) {
		delete mPlayBackPreview;
		mPlayBackPreview = NULL;
	}
	if(mPlayBack) {
		delete mPlayBack;
		mPlayBack= NULL;
	}
	if(mMenu) {
		delete mMenu;
		mMenu= NULL;
	}
	if (mDataCheckThread != NULL) {
		mDataCheckThread->stopThread();
		mDataCheckThread.clear();
		mDataCheckThread = 0;
	}
	for (int i=0; i<2; i++) {
		if (timer_data[i].id  != 0) {
			stopTimer(i);
			deleteTimer(i);
			timer_data[i].id = 0;
		}
	}
	UnRegisterCDRWindows();
	UnRegisterCDRControls();
	ExitGUISafely(0);
	MainWindowThreadCleanup(mHwnd);
	if(mTinyParkAudioTh)
		delete mTinyParkAudioTh;
}

bool CdrMain::isPhotoWindow()
{
	return (!mOtherDialogOpened && getCurWindowId() == WINDOWID_RECORDPREVIEW && getCurrentRPWindowState() == STATUS_PHOTOGRAPH);
}

bool CdrMain::isBatteryOnline()
{
	if (mEM) {
		return mEM->isBatteryOnline();
	}
	return false;
}

void CdrMain::playSounds(sounds_type type)
{
	switch(type)
	{
		case 1:
			initKeySound("/system/res/others/key.wav");
			break;
		case 2:
			initKeySound("/system/res/others/shutter.wav");
			break;
		default:
			break;
	}
    if (isBatteryOnline())
       noWork(true);
	playKeySound();
}
void CdrMain::initPreview(void* none)
{
	if(mLowPower<=LOW_POWER)
		return ;
	initCdrTime();
	status_t err;	
	mInitPT = new InitPreviewThread(this);
	err = mInitPT->start();
	if (err != OK) {
		mInitPT.clear();
	}
}

int CdrMain::initPreview(void)
{
	struct timespec measureTime1;

	mRecordPreview->init();
	mPOR.signal();

	mRecordPreview->startPreview();
	sys_log("startPreview finished\n");

	return 0;
}

void CdrMain::initStage2(void* none)
{
	status_t err;

	mInitStage2T = new InitStage2Thread(this);
	err = mInitStage2T->start();
	if (err != OK) {
		db_msg("initStage2 start failed\n");
		mInitPT.clear();
	}
}

int CdrMain::wifiInit()
{
	ResourceManager *rm = ResourceManager::getInstance();
	bool flag = rm->getResBoolValue(ID_MENU_LIST_WIFI);
	if(flag)
	{
		mConnManager =	new ConnectivityManager();
		mConnManager->setConnectivityListener((void*)this, this);
		int ret = mConnManager->start();
		HWND hwnd=getWindowHandle(WINDOWID_STATUSBAR);
		SendMessage(hwnd, STBM_WIFI, (WPARAM)flag, 0);
		//rm->setResBoolValue(ID_MENU_LIST_WIFI, true);
	}
	return 0;
}

int CdrMain::wifiEnable(int bEnable)
{
	int ret=0;
#ifdef APP_WIFI
	if(!bEnable && !mConnManager){
		return -1;
	}
	if(!mConnManager)
	{
		mConnManager =  new ConnectivityManager();
		mConnManager->setConnectivityListener((void*)this, this);
		ret = mConnManager->start();
		return ret;
	}else{
		ret = mConnManager->setWifiEnable(bEnable);
		db_msg("******ret=%d  ------",ret);
		return  ret;  //0  yes  -1  error
	}
#endif
	return ret;
}

int CdrMain::wifiSetPwd(const char *old_pwd, const char *new_pwd)
{
	int ret = 0;
	ret = mConnManager->setPwd(old_pwd, new_pwd);
	return ret;
}

void CdrMain::handleConnectivity(void *cookie, conn_msg_t msg)
{
	ALOGD("handleConnectivity()..msg = %d", msg);
	if(CONN_CHANNEL_CHANGE == msg) {
	}
	else if(CONN_AVAILABLE == msg) {
	}
	else if(CONN_UNAVAILABLE == msg){
	}
	ALOGD("handleConnectivity()..exit, msg = %d", msg);
	return;
}
extern void debug_start();

void *icur_thread(void *arg)
{
	while(!isOTAUpdate)
	{
		sync();
		system("echo 3 > /proc/sys/vm/drop_caches");
		sleep(3);
	}
	return NULL;
}

int CdrMain::initStage2(void)
{
	StorageManager* sm;
	ResourceManager* rm;
	int result;

	/* wait for the preview is started */
	mLock.lock();
	result  = mPOR.waitRelative(mLock, seconds(1));
	mLock.unlock();
	rm = ResourceManager::getInstance();
	if(rm->initStage2() < 0) {
		db_error("ResourceManager initStage2 failed\n");
		return -1;
	}
	if(createOtherWindows() < 0) {
		return -1;
	}
	bool hasTVD;
	bool tvdOnline;
	mPM = new PowerManager(this);
	tvdOnline = mRecordPreview->hasTVD(hasTVD);
	mEM = new EventManager(tvdOnline, hasTVD);
	mEM->setEventListener(this);
	mEM->init(hasTVD);
	/*if(!mEM->isBatteryOnline()) {
		db_msg("battery is charging\n");
		SendMessage(mSTB->getHwnd(), MSG_BATTERY_CHARGING, 0, 0);
	}*/
	initTimerData();
//	startShutdownTimer(TIMER_NOWORK_IDX);
	if (mEM->isBatteryOnline()) {
		db_msg("AC PLUG OUT");
		noWork(true);
		int ret = mPM->setBatteryLevel();
		SendMessage(mSTB->getHwnd(), MSG_BATTERY_CHANGED, ret, 0);
	} else {
		db_msg("AC PLUG IN");
		SendMessage(mSTB->getHwnd(), MSG_BATTERY_CHARGING, 0, 0);
		noWork(false);
	}
	sm = StorageManager::getInstance();
	sm->setEventListener(this);
#ifdef FATFS
	int sdcard_status = sm->isInsert();
	if (sdcard_status) {
#else
	if(sm->isMount() == true) {
#endif
		db_msg("mounted\n");
		SendMessage(mSTB->getHwnd(), STBM_MOUNT_TFCARD, 1, 0);
	} else {
		db_msg("not mount\n");
		SendMessage(mSTB->getHwnd(), STBM_MOUNT_TFCARD, 0, 0);
	}
	bool isDrawAlign = rm->getResBoolValue(ID_MENU_LIST_ALIGNLINE);
	SendMessage(getWindowHandle(WINDOWID_RECORDPREVIEW), MSG_DRAW_ALIGN_LINE, isDrawAlign, 0);
	bool isParkOn = rm->getSubMenuCurrentIndex(ID_MENU_LIST_PARK);
	SendMessage(getWindowHandle(WINDOWID_STATUSBAR), STBM_PARK, isParkOn, 0);
	bool isVoiceOn = rm->getResBoolValue(ID_MENU_LIST_RECORD_SOUND);
	SendMessage(getWindowHandle(WINDOWID_STATUSBAR), STBM_RECORD_SOUND, isVoiceOn, 0);
	if (!mRecordPreview->camExisted(CAM_UVC)) {
		mEM->checkDevMore(CAM_UVC);
	}
	configInit();
#ifdef APP_WIFI
	wifiInit();
	sessionServerInit();
#endif
	//mPM->watchdogRun();
	mFinishConfig = true;
	if(!(rm->getResBoolValue(ID_MENU_LIST_SMARTALGORITHM))&&!getFormatTFAdasS()){
		mFormatTFAdasS = true;
		if(sm->isInsert()){
			if( !checkSDFs() && !m2pcConnected){ 
				mNeedCheckSD = true;
				SendNotifyMessage(mMenu->getHwnd(), MSG_FORMAT_TFDIALOG, (WPARAM)0, 0);
			}
		}
	}
	char prop_buf[8] = "";;
	cfgDataGetString("persist.sys.mem.debug", prop_buf, "false");
	if (0 == strcmp(prop_buf, "true")) {
		debug_start();
	}
	pthread_t icurTh = 0;
	int ir_err = pthread_create(&icurTh, NULL, icur_thread, NULL);
	if(ir_err || !icurTh) {
		ALOGE("Create OtaUpdate  thread error.");
		return -1;
	}
	cfgDataGetString("ro.cdr.debug", prop_buf, "true");
	if (0 == strcmp(prop_buf, "false")) {
		startOtherService();
	}
	mPM->connect2StandbyService();
	
#if  0   //  debug  system boot
	sleep(30);
	system("reboot");
#endif	
	
	return 0;
}

void CdrMain::startOtherService()
{
	cfgDataSetString("ctl.start", "debuggerd");
	cfgDataSetString("ctl.start", "adbd");
	cfgDataSetString("ctl.start", "standby");
}

void CdrMain::cdrOtaUpdate()
{

	db_msg("****Update*****");
	int retval;
	const char* ptr;
	MessageBox_t messageBoxData;
	ResourceManager* rm;
	String8 textInfo;

	rm = ResourceManager::getInstance();
	memset(&messageBoxData, 0, sizeof(messageBoxData));
	messageBoxData.dwStyle = MB_OKCANCEL | MB_HAVE_TITLE | MB_HAVE_TEXT;
	messageBoxData.flag_end_key = 0; /* end the dialog when endkey keydown */

	ptr = rm->getLabel(LANG_LABEL_TIPS);
	if(ptr == NULL) {
	       db_error("get deletefile title failed\n");
	       return;
	}
	messageBoxData.title = ptr;

	ptr = rm->getLabel(LANG_LABEL_OTA_UPDATE);
	if(ptr == NULL) {
	       db_error("get deletefile text failed\n");
	       return;
	}
	messageBoxData.text = ptr;

	ptr = rm->getLabel(LANG_LABEL_SUBMENU_OK);
	if(ptr == NULL) {
	       db_error("get ok string failed\n");
	       return;
	}
	messageBoxData.buttonStr[0] = ptr;

	ptr = rm->getLabel(LANG_LABEL_SUBMENU_CANCEL);
	if(ptr == NULL) {
	       db_error("get ok string failed\n");
	       return;
	}
	messageBoxData.buttonStr[1] = ptr;

	messageBoxData.pLogFont = rm->getLogFont();

	rm->getResRect(ID_MENU_LIST_MB, messageBoxData.rect);
	messageBoxData.fgcWidget = rm->getResColor(ID_MENU_LIST_MB, COLOR_FGC);
	messageBoxData.bgcWidget = rm->getResColor(ID_MENU_LIST_MB, COLOR_BGC);
	messageBoxData.linecTitle = rm->getResColor(ID_MENU_LIST_MB, COLOR_LINEC_TITLE);
	messageBoxData.linecItem = rm->getResColor(ID_MENU_LIST_MB, COLOR_LINEC_ITEM);
	messageBoxData.confirmCallback = NULL;
	messageBoxData.confirmData = (void*)this;

	db_msg("this is %p\n", this);
	db_msg("***mHwdn:%x, curHWND:%x**ID:%d*",mHwnd,getWindowHandle(getCurWindowId()),getCurWindowId());

	if(m2pcConnected)
			return ;
	mOTAAdasScreen = false;
	mOTAAdasScreen = mRecordPreview->getIsAdasScreen();
	if(mOTAAdasScreen){
			mRecordPreview->adasScreenSwitch(false);
	}

	retval = showMessageBox(getWindowHandle(getCurWindowId()), &messageBoxData);
	db_msg("retval is %d\n", retval);
	if (retval == IDC_BUTTON_OK){
	       db_msg("Update ***************");
		   isOTAUpdate = true;
	       otaUpdate();
	}else{
	       db_msg("****no update***********");
		   if(mOTAAdasScreen){
                 mRecordPreview->adasScreenSwitch(true); 
                 mOTAAdasScreen = false;
           }
	}
}

bool CdrMain::getPC2Connected()
{
	return m2pcConnected;
}

bool CdrMain::getFormatTFAdasS(){
	return mFormatTFAdasS;
}

void CdrMain::setFormatTFAdasS(bool flag)
{
	mFormatTFAdasS = flag;
}

bool CdrMain::getNeedCheckSD()
{
	return mNeedCheckSD;
}

void CdrMain::setNeedCheckSD(bool flag)
{
	mNeedCheckSD = flag;
}

unsigned int getClustSize(char *devname)
{
    int fd;
    unsigned char block[DOSBOOTBLOCKSIZE]={0};
    unsigned int BytesPerSec = 0;
    unsigned int SecPerClust = 0;
    unsigned int BytesPerClust = 0;
	
    fd = open(devname, O_RDONLY, 0);
	if (fd < 0) {
		db_msg("fail to open %s", devname);
		return 0;
	}
    if (read(fd, block, sizeof(block) ) < sizeof(block) ) {
        db_msg("Could not read boot block");
        return 0;
    }
	close(fd);
	
    if (block[510] != 0x55 || block[511] != 0xaa) {
        db_msg("Invalid signature in boot block block[510]%x block[511]%x", block[510], block[511]);
        return 0;
    }

    if(!memcmp(&block[3], "EXFAT   ", 8)){
        db_msg("exFAT filesystem is not supported");
        return 0;
    }

    if(!memcmp(&block[3], "NTFS", 4)){
        db_msg("NTFS filesystem is not supported");
        return 0;
    }
	
    if(!memcmp(&block[82], "FAT32   ", 8))
        db_msg("FAT32 filesystem is supported\n");

    BytesPerSec = block[11] + (block[12] << 8);
    if (BytesPerSec != 512 && BytesPerSec != 1024 && BytesPerSec != 2048 && BytesPerSec != 4096) {
        db_msg("Invalid sector in boot block");
		return 0;
    } 

    SecPerClust = block[13];
    if (SecPerClust&(SecPerClust-1) != 0) {
        db_msg("Invalid cluster in boot block");
        return 0;
    } 

    BytesPerClust = SecPerClust*BytesPerSec;
	db_msg("BytesPerClust %u\n",BytesPerClust);
	
    return BytesPerClust;
}

void CdrMain::checkDataValid()
{
	char *tmp_file = MOUNT_PATH"video/~tmp";
	int fd = open(tmp_file, O_WRONLY|O_CREAT, 666);
	if (fd < 0) {
		return;
	}
	const char * str = "read the tmp file.";
	int bytes = strlen(str);
	int num = write(fd, str, bytes);
	fsync(fd);
	close(fd);
	if (num != bytes) {
		remove(tmp_file);
		if (isRecording()) {
			mRecordPreview->stopRecord();
		}
		mRecordPreview->resetRecorder();
		BroadcastMessage(MSG_CLOSE_TIP_LABEL, 0, 0);
		SendMessage(mHwnd, MSG_SHOW_TIP_LABEL, LABEL_INVAILD_TFCATD, 0);
		return ;
	}

	sleep(2);
	StorageManager *sm = StorageManager::getInstance();
	if (!sm->isMount()) {
		return;
	}
	fd = open(tmp_file, O_RDONLY|O_CREAT, 666);
	if (fd < 0) {
		remove(tmp_file);
		if (isRecording()) {
			mRecordPreview->stopRecord();
		}
		mRecordPreview->resetRecorder();
		BroadcastMessage(MSG_CLOSE_TIP_LABEL, 0, 0);
		SendMessage(mHwnd, MSG_SHOW_TIP_LABEL, LABEL_INVAILD_TFCATD, 0);
		return ;
	}
	char buffer[32]={0};
	num = read(fd, buffer, 128);
	if (num < 0 || 0 != strcmp(str, buffer)) {
		close(fd);
		remove(tmp_file);
		if (isRecording()) {
			mRecordPreview->stopRecord();
		}
		mRecordPreview->resetRecorder();
		BroadcastMessage(MSG_CLOSE_TIP_LABEL, 0, 0);
		SendMessage(mHwnd, MSG_SHOW_TIP_LABEL, LABEL_INVAILD_TFCATD, 0);
		return ;
	}
	close(fd);
	remove(tmp_file);
}

bool CdrMain::checkSDFs(){
#ifdef FATFS
	int ret = fat_card_format();
	mNeedCheckSD = false;
	db_msg("*************ret:%d",ret);
	if(ret == 1)	//means that card is not fat32/64kb
		return false;	//false for format
	else
		return true;
#endif
	char device[256];
	const char *path = MOUNT_POINT;
	char mount_path[256];
	char rest[256];
	FILE *fp;
	int reval=0;
	char line[1024];

	if (!(fp = fopen("/proc/mounts", "r"))) {
		db_msg("Error opening /proc/mounts\n");
		return false;
	}

	while(fgets(line, sizeof(line), fp)) {
		line[strlen(line)-1] = '\0';
		sscanf(line, "%255s %255s %255s\n", device, mount_path, rest);
		if (!strcmp(mount_path, path)) {
			fclose(fp);
			goto checkfat;
		}
	}

	fclose(fp);
	db_msg("Volume is not mounted\n");
	return false;
//	sscanf(&argv[1][0],"%s",path);	 //从参数2取得路径
checkfat:
#define CLUST_SIZE 65536
	reval = getClustSize(device);
	if(reval==CLUST_SIZE) {
		#ifndef FATFS
			if (mNeedCheckSD) {
				mDataCheckThread->stopThread();
				mDataCheckThread->start();
			}
		#endif
		mNeedCheckSD = false;
		return true;
	}
	else {
		mNeedCheckSD = false;
		return false;
	}
}


void CdrMain::initCdrTime(){
   time_t timer;
   struct tm *tblock;
   timer = time(NULL);
   tblock = localtime(&timer);
   if(tblock->tm_year + 1900 >=2015)
   {
       return ;
   }else{
       tblock->tm_year=2015-1900;
       tblock->tm_mon=0;
       tblock->tm_mday=1;
       tblock->tm_hour=0;
       tblock->tm_min=0;
       tblock->tm_sec=0;
       if(setDateTime(tblock)!=0){
          db_msg("***init cdr time failed***");
       }
   }       
}

int CdrMain::sessionServerInit()
{
	int ret = 0;
	
	mCdrServer = new CdrServer(this);
	ret = mCdrServer->start();

	return ret;
}

void CdrMain::sessionChange(bool connect)
{
	if (connect) {
		db_msg("----------------connect!");
		mRecordPreview->sessionInit(mCdrServer->getSession());
		mRecordPreview->startRecord(CAM_CSI_NET);
	} else {
		mRecordPreview->stopRecord(CAM_CSI_NET);
#ifdef PLATFORM_0
		mRecordPreview->stopRecord(CAM_UVC_NET);
#endif
	}
}

void CdrMain::checkShutdown()
{
	db_msg("mShutdowning %d", mShutdowning);
	if (mShutdowning) {
		showShutdownLogo();
	}
}

void CdrMain::restoreFromStandby()
{
	startUp();
	bool sdcard_status = 0;
	StorageManager* smt;
	smt = StorageManager::getInstance();
	sdcard_status = smt->isInsert();
	if (sdcard_status) {
		SendMessage(mSTB->getHwnd(), STBM_MOUNT_TFCARD, sdcard_status, 0);
		SendMessage(mMenu->getHwnd(), STBM_MOUNT_TFCARD, sdcard_status, 0);
	}
	bool bValid = mEM->checkValidVoltage();
	if (!bValid) {
        shutdown();
	}
}

void CdrMain::shutdown(int flag)
{
	db_msg("shutdown %d", flag);
	ResourceManager* rm = ResourceManager::getInstance();
	EventManager::int2set(rm->getSubMenuCurrentIndex(ID_MENU_LIST_PARK));
	if(rm->getSubMenuCurrentIndex(ID_MENU_LIST_PARK))
	        cfgDataSetString("persist.app.park.config", "1");
	else
	        cfgDataSetString("persist.app.park.config", "0");
	if (rm->getResBoolValue(ID_MENU_LIST_STANDBY_MODE) && flag)
	{
		standby();
		noWork(true);
		restoreFromStandby();
		return ;
	}
	SendMessage(getWindowHandle(WINDOWID_RECORDPREVIEW),MSG_ADAS_SCREEN,0,0);
	mShutdowning = true;
	if (!mMenu->getSubMenuFlag()) {	/* waiting for the submenu is closed*/
		showShutdownLogo();
	}
	mEM->setGsensorSensitivity(rm->getSubMenuCurrentIndex(ID_MENU_LIST_PARK));
	rm->syncConfigureToDisk();
	mRecordPreview->stopRecordSync();
	mRecordPreview->stopPreview();
	mPlayBack->stopPlayback(false);
	mPM->powerOff();
}

void CdrMain::standby()
{
	mRecordPreview->suspend();
	db_msg("standby\n");
	mPM->enterStandbyModem();
	mRecordPreview->resume();
	mPM->standbyOver();
	//need to check park mode or start record
}

//#define IGNORE_UVC_STATE
int CdrMain::notify(int message, int val)
{
	int ret = 0;
	StorageManager *smt = StorageManager::getInstance();
	if(mShutdowning == true) {
		db_info("shutting down, ignore events\n");	
		return 0;
	}

	switch(message) {
	case EVENT_UVC_PLUG_IN:
		mRecordPreview->setCamExist(CAM_UVC, true);
#ifdef IGNORE_UVC_STATE
		if(getCurWindowId() == WINDOWID_RECORDPREVIEW)
#endif
		{
			SendMessage(mSTB->getHwnd(), STBM_UVC, 1, 0);
		}
		break;
	case EVENT_TVD_PLUG_IN:
		mRecordPreview->setCamExist(CAM_TVD, true);
#ifdef IGNORE_UVC_STATE
		if(getCurWindowId() == WINDOWID_RECORDPREVIEW)
#endif
		{
			SendMessage(mSTB->getHwnd(), STBM_UVC, 1, 0);
		}

		break;
	case EVENT_UVC_PLUG_OUT:
		ret = mRecordPreview->setCamExist(CAM_UVC, false);
#ifdef APP_WIFI
		if (ret > 0) {
			mCdrServer->reply(NAT_CMD_UVC_DISCONNECT, 0);
		}
#endif
#ifdef IGNORE_UVC_STATE
		if(getCurWindowId() == WINDOWID_RECORDPREVIEW)
#endif
		{
			SendMessage(mSTB->getHwnd(), STBM_UVC, 0, 0);
		}
		break;
	case EVENT_TVD_PLUG_OUT:
		mRecordPreview->setCamExist(CAM_TVD, false);
#ifdef IGNORE_UVC_STATE
		if(getCurWindowId() == WINDOWID_RECORDPREVIEW)
#endif
		{
			SendMessage(mSTB->getHwnd(), STBM_UVC, 0, 0);
		}
		break;
	case EVENT_CONNECT2PC:
		{
			if (mPCCam || pcDialogFinish == false) {
				break;
			}
			mEM->checkPCconnect(true);
			db_msg("EVENT_CONNECT2PC\n");
			if(!isScreenOn()){
				mPM->screenOn();
			}
			m2pcConnected = true;
			mOtherDialogOpened = true;
           	m2pcAdasScreen = false;
			//if (!isRecording() && !isPlaying()) {
				noWork(false);
			if(mOTAAdasScreen){
				 break;
			}			
			SendNotifyMessage(mHwnd, MSG_CONNECT2PC, 0, 0);
			if(mRecordPreview->getIsAdasScreen() && !m2pcAdasScreen){
	           SendMessage(getWindowHandle(WINDOWID_RECORDPREVIEW),MSG_ADAS_SCREEN,0,0);
	           m2pcAdasScreen = true;
            }
			//}
		}
		break;
	case EVENT_DISCONNECFROMPC:
		{
			db_msg("EVENT_DISCONNECTFROMPC\n");
			if (mEM->checkPCconnect() || pcDialogFinish == true) {
				break;
			}
			db_msg("checkPCconnect : false");
			#ifdef FATFS
			if(getUSBStorageMode()){
				StorageManager * sm = StorageManager::getInstance();
				sm -> reMountFATFS();
			}
			#endif
			int ret;
			if (mPCCam) {
//				String8 val_usb;
//				usleep(800*1000);	//ignore the unstable status
//				if(mEM->usbConnnect() != 0) {
//					break;
//				}
				stopPCCam();
			}
			m2pcConnected = false;

			mOtherDialogOpened = false;
			if(m2pcAdasScreen)
				SendMessage(getWindowHandle(WINDOWID_RECORDPREVIEW),MSG_ADAS_SCREEN,1,0);
			if (mEM->isBatteryOnline()) {
				ret = mPM->getBatteryLevel();
				SendMessage(mSTB->getHwnd(), MSG_BATTERY_CHANGED, ret, 0);
			}
			SendMessage(mHwnd, MSG_DISCONNECT_FROM_PC, 0, 0);
			//if (!isRecording() && !isPlaying()) {
//				noWork(false);
				if(mEM->isBatteryOnline()) {
					noWork(true);
				}
			//}
			mNeedCheckSD = true;
		}
		break;
	case EVENT_IMPACT:
		if (getCurWindowId() == WINDOWID_RECORDPREVIEW) {
			mRecordPreview->impactOccur();
		}
		break;
	case EVENT_BATTERY:
	{
		if(mEM->isBatteryOnline()) {
			int status= EventManager::getBatteryStatus();
			db_msg("**getBatteryStatus %d***", status);
			ret = mPM->setBatteryLevel();
			if(ret<=BATTERY_LOW_LEVEL){
				if(status == BATTERY_DISCHARGING || status == BATTERY_NOTCHARGING){
					db_msg("**low_power shutdown***");
					SendNotifyMessage(mHwnd, MSG_SHOW_TIP_LABEL, LABEL_LOW_POWER_SHUTDOWN, 0);
					sleep(3);
					shutdown(); 
				}
			}
			SendMessage(mSTB->getHwnd(), MSG_BATTERY_CHANGED, ret, 0);
		}else{
			//db_msg("********ignore first time(AC plug out turn on cdr)*******ret0");
			static int count = 0;
			if(count > 0)
				SendMessage(mSTB->getHwnd(), MSG_BATTERY_CHARGING, 0, 0);
			if(count == 0)
				count ++;
		}
	}
		break;
	case EVENT_MOUNT:
		mNeedCheckSD = true;
		if (!val) {	//unmount
			if (getCurWindowId() == WINDOWID_RECORDPREVIEW) {
				mRecordPreview->storagePlug(false);
				mPlayBack->stopPlaying();
			}
			if (getCurWindowId() == WINDOWID_RECORDPREVIEW) {
				db_msg("-----------umount, stopRecord and setSdcardState(false)");
 			} else if (getCurWindowId() == WINDOWID_PLAYBACKPREVIEW) {
				db_msg("current window is playbackview");
				SendMessage(mSTB->getHwnd(), STBM_CLEAR_LABEL1_TEXT, WINDOWID_PLAYBACKPREVIEW, 0);
				SendMessage(mHwnd, MWM_CHANGE_WINDOW, WINDOWID_PLAYBACKPREVIEW, WINDOWID_RECORDPREVIEW);
			} else if (getCurWindowId() == WINDOWID_PLAYBACK) {
				db_msg("current window is playback");
				SendMessage(mPlayBack->getHwnd(), MSG_PLB_COMPLETE, 0, 0);
				mRecordPreview->specialProc(SPEC_CMD_LAYER_CLEAN, 3);
				SendMessage(mPlayBackPreview->getHwnd(), MSG_UPDATE_LIST, 0, 0);
				SendNotifyMessage(mHwnd, MWM_CHANGE_WINDOW, WINDOWID_PLAYBACK, WINDOWID_PLAYBACKPREVIEW);
			}
		} else {
			if (getCurWindowId() == WINDOWID_PLAYBACKPREVIEW) {
				usleep(100*1000);	//wait for the thread to update database
				SendMessage(mPlayBackPreview->getHwnd(), MSG_UPDATE_LIST, 0, 0);
			}
			mRecordPreview->storagePlug(true);
 			mPM->setStandby_status(true);
		}
		break;
	case EVENT_DELAY_SHUTDOWN:
		db_msg("·AC PLUG OUT·");
		if(mEM->isBatteryOnline()) {
			int ret=-1;
			ret = mPM->setBatteryLevel();
			if(ret<=BATTERY_LOW_LEVEL){
				db_msg("**low_power shutdown***");
				SendNotifyMessage(mHwnd, MSG_SHOW_TIP_LABEL, LABEL_LOW_POWER_SHUTDOWN, 0);
				sleep(3);
				shutdown(); 
			}
			SendMessage(mSTB->getHwnd(), MSG_BATTERY_CHANGED, ret, 0);
		}
		//if (!isRecording()&&!isPlaying()) {
		//	startShutdownTimer(TIMER_LOWPOWER_IDX);
		//	db_msg("xxxxxxxxxx\n");
		//	SendNotifyMessage(mHwnd, MSG_SHOW_TIP_LABEL, LABEL_10S_SHUTDOWN, 0);
		//}
		if (mEM->isBatteryOnline()) {
			noWork(true);
		}
		break;
	case EVENT_CANCEL_SHUTDOWN:
		db_msg("·AC PLUG IN·");
		if(!mEM->isBatteryOnline()) {
			SendMessage(mSTB->getHwnd(), MSG_BATTERY_CHARGING, 0, 0);
		}
		noWork(false);
		//if (!isRecording()&& !isPlaying()) {
		//	cancelShutdownTimer(TIMER_LOWPOWER_IDX);
		//	db_msg("xxxxxxxxxx\n");
		//	CloseTipLabel();
		//	noWork(false);
		//}
		break;	
#ifdef PLATFORM_0
	case EVENT_HDMI_PLUGIN:
		{
			mEM->hdmiAudioOn(true);
			if ( getCurWindowId() == WINDOWID_PLAYBACKPREVIEW ) {
				mPlayBackPreview->beforeOtherScreen();
			}
			mRecordPreview->otherScreen(1);
			if ( getCurWindowId() == WINDOWID_PLAYBACKPREVIEW ) {
				mPlayBackPreview->afterOtherScreen(getHdl(0), 1);
			}
			mPlayBackPreview->otherScreen(1);
		}
		break;
	case EVENT_HDMI_PLUGOUT: 
		{
			mEM->hdmiAudioOn(false);
			if ( getCurWindowId() == WINDOWID_PLAYBACKPREVIEW ) {
				mPlayBackPreview->beforeOtherScreen();
			}
			mRecordPreview->otherScreen(0);
			if ( getCurWindowId() == WINDOWID_PLAYBACKPREVIEW ) {
				mPlayBackPreview->afterOtherScreen(getHdl(0), 0);
			}
			mPlayBackPreview->otherScreen(0);
		}
		break;
#endif
#ifdef APP_TVOUT
	case EVENT_TVOUT_PLUG_IN:
		{
			mRecordPreview->otherScreen(2);
		}
		break;
	case EVENT_TVOUT_PLUG_OUT: 
		{
			mRecordPreview->otherScreen(0);
		}
		break;
#endif
	case EVENT_REMOVESD:
	{
		mRecordPreview->storagePlug(false);
		BroadcastMessage(MSG_CLOSE_TIP_LABEL, 0, 0);
		if (mPM->getStandby_status() && !getWakeupState()){
			SendNotifyMessage(mHwnd, MSG_SHOW_TIP_LABEL, LABEL_SD_PLUGOUT, 3000);
		}
		SendMessage(mSTB->getHwnd(), STBM_MOUNT_TFCARD, 0, 0);
		SendMessage(mMenu->getHwnd(), STBM_MOUNT_TFCARD, 0, 0);
		StorageManager * sm = StorageManager::getInstance();
		//sm->dbReset();
		mNeedCheckSD = true;
		if (getCurWindowId() == WINDOWID_PLAYBACKPREVIEW) {
			SendMessage(mPlayBackPreview->getHwnd(), MSG_UPDATE_LIST, 0, 0);
		}
		//BroadcastMessage(MSG_CLOSE_TIP_LABEL, 0, 0);
	}
		break;
	case EVENT_INSERTSD:
	{
		mRecordPreview->storagePlug(true);
		BroadcastMessage(MSG_CLOSE_TIP_LABEL, 0, 0);
		if (mPM->getStandby_status() && !getWakeupState()){
			SendNotifyMessage(mHwnd, MSG_SHOW_TIP_LABEL, LABEL_SD_PLUGIN, 3000);
		}
		SendMessage(mSTB->getHwnd(), STBM_MOUNT_TFCARD, 1, 0);
		SendMessage(mMenu->getHwnd(), STBM_MOUNT_TFCARD, 1, 0);
		StorageManager * sm = StorageManager::getInstance();
		#ifdef FATFS
			sm->reMountFATFS();
		#endif
		sm->needCheckAgain(true);	//to avoid problem that tfcard status is wrong then quickly plug in/out
		mNeedCheckSD = true;
		break;
	}
	case EVENT_REVERSE_OFF: {
			mReverseMode = false;
			db_msg("EVENT_REVERSE_OFF~");
			unsigned int winId = getCurWindowId();
			changeWindow(winId,WINDOWID_RECORDPREVIEW);
			//mRecordPreview->reverseMode(false);
			ShowWindow(getWindowHandle(WINDOWID_STATUSBAR), SW_SHOWNORMAL);
			mReverseChanged = false;
		}
		break;
	case EVENT_REVERSE_ON: {
			db_msg("EVENT_REVERSE_ON~");
			BroadcastMessage(MSG_CLOSE_TIP_LABEL, 0, 0);
			mReverseChanged = true;
			mReverseMode = true;
			unsigned int winId = getCurWindowId();
			changeWindow(winId,WINDOWID_RECORDPREVIEW);
			//mRecordPreview->reverseMode(true);
			ShowWindow(getWindowHandle(WINDOWID_STATUSBAR), SW_HIDE);
		}
		break;
	}
	return ret;
}

static void connectToPCCallback(HWND hWnd, int id, int nc, DWORD context)
{
	CdrMain* cdrMain;
	cdrMain = (CdrMain*)context;
	ResourceManager* rm;
	const char* ptr;
	HWND hDlg;

	int screenW,screenH;
	getScreenInfo(&screenW, &screenH);
	hDlg = GetParent(hWnd);
	rm = ResourceManager::getInstance();
	if(nc == BN_CLICKED) {
		if(id == ID_CONNECT2PC)	{
			if(cdrMain->getUSBStorageMode() == false) {
				db_msg("open usb storage mode\n");
				cdrMain->setUSBStorageMode(true);
				ptr = rm->getLabel(LANG_LABEL_CLOSE_USB_STORAGE_DEVICE);
				if(ptr){
				//	SetWindowText(hWnd, ptr);
					ShowWindow(GetDlgItem(hDlg,ID_CONNECT2PC+1*40),SW_HIDE);
					ShowWindow(GetDlgItem(hDlg,ID_CONNECT2PC+2*40),SW_HIDE);
					ShowWindow(GetDlgItem(hDlg,ID_CONNECT2PC+3*40),SW_HIDE);
					ShowWindow(GetDlgItem(hDlg,ID_CONNECT2PC),SW_HIDE);
					ShowWindow(GetDlgItem(hDlg,ID_CONNECT2PC+1),SW_HIDE);
					ShowWindow(GetDlgItem(hDlg,ID_CONNECT2PC+2),SW_HIDE);
					UpdateWindow(hDlg,true);
					SendMessage(hDlg, MSG_UPDATE_BG, (WPARAM)"/system/res/others/usb_storage_modem.png", 0);
				}
				else {
					db_error("get label failed\n");
				}
				
			} else if(cdrMain->getUSBStorageMode() == true) {
				db_msg("close usb storage mode, hDlg is 0x%x\n", hDlg);
				cdrMain->setUSBStorageMode(false);	
				ptr = rm->getLabel(LANG_LABEL_OPEN_USB_STORAGE_DEVICE);
				if(ptr){
					SetWindowText(hWnd, ptr);
					ShowWindow(GetDlgItem(hDlg,ID_CONNECT2PC+1),SW_SHOW);
				}
				else {
					db_error("get label failed\n");
				}				
			}
		} else if(id == ID_CONNECT2PC + 1) {
			db_msg("charge mode, hDlg is 0x%x\n", hDlg);
			SendNotifyMessage(hDlg, MSG_CLOSE_CONNECT2PC_DIALOG, 0, 0);
			if(!cdrMain->isBatteryOnline()) {
				db_msg("battery is charging\n");
				SendMessage(cdrMain->getWindowHandle(WINDOWID_STATUSBAR), MSG_BATTERY_CHARGING, 0, 0);
			}
		}
		else if (id == ID_CONNECT2PC + 2) {
			if(cdrMain->getPCCamMode() == false) {
				db_msg("open PCCamMode\n");
				ShowWindow(GetDlgItem(hDlg,ID_CONNECT2PC+1*40),SW_HIDE);
				ShowWindow(GetDlgItem(hDlg,ID_CONNECT2PC+2*40),SW_HIDE);
				ShowWindow(GetDlgItem(hDlg,ID_CONNECT2PC+3*40),SW_HIDE);
				ShowWindow(GetDlgItem(hDlg,ID_CONNECT2PC),SW_HIDE);
				ShowWindow(GetDlgItem(hDlg,ID_CONNECT2PC+1),SW_HIDE);
				ShowWindow(GetDlgItem(hDlg,ID_CONNECT2PC+2),SW_HIDE);
				UpdateWindow(hDlg,true);
				SendMessage(hDlg, MSG_UPDATE_BG, (WPARAM)"/system/res/others/pccam_modem.png", 0);
			}
			cdrMain->startPCCam();
		}
	}
}

static int connectToPCDialog(HWND hWnd, void* context)
{
	int retval = 0;
	PopUpMenuInfo_t info;
	ResourceManager* rm;
	static bool hasOpened = false;

	if(hasOpened == true) {
		db_info("dialog has been opened\n");
		return 2;
	}

	rm = ResourceManager::getInstance();

	memset(&info, 0, sizeof(info));
	info.pLogFont = rm->getLogFont();
	retval = rm->getResRect(ID_CONNECT2PC, info.rect);
	if(retval < 0) {
		db_error("get connectToPC rect failed\n");	
		return -1;
	}

	retval = rm->getResIntValue(ID_CONNECT2PC, INTVAL_ITEMWIDTH);
	if(retval < 0) {
		db_error("get connectToPC item_width failed");
		return -1;
	}
	info.item_width = retval;

	retval = rm->getResIntValue(ID_CONNECT2PC, INTVAL_ITEMHEIGHT);
	if(retval < 0) {
		db_error("get connectToPC item_height failed");
		return -1;
	}
	info.item_height = retval;

	info.style = POPMENUS_V | POPMENUS_VCENTER;
	info.item_nrs = 3;
	info.id = ID_CONNECT2PC;
	info.name[0] = rm->getLabel(LANG_LABEL_OPEN_USB_STORAGE_DEVICE);
	if(info.name[0] == NULL) {
		db_error("get the LANG_LABEL_OPEN_USB_STORAGE_DEVICE failed\n");
		return -1;
	}
	info.name[1] = rm->getLabel(LANG_LABEL_CHARGE_MODE);
	if(info.name[1] == NULL) {
		db_error("get the LANG_LABEL_CHARGE_MODE failed\n");
		return -1;
	}
	info.name[2] = rm->getLabel(LANG_LABEL_OPEN_PCCAM);
	if(info.name[2] == NULL) {
		db_error("get the LANG_LABEL_OPEN_PCCAM failed\n");
		return -1;
	}
	info.callback[0] = connectToPCCallback;
	info.callback[1] = connectToPCCallback;
	info.callback[2] = connectToPCCallback;
	info.context = (DWORD)context;

	info.end_key = CDR_KEY_MODE;
	info.flag_end_key = 1;		/* key up to end the dialog */
	info.push_key = CDR_KEY_OK;
	info.table_key = CDR_KEY_RIGHT;

	info.bgc_widget = rm->getResColor(ID_CONNECT2PC, COLOR_BGC);
	info.bgc_item_focus = rm->getResColor(ID_CONNECT2PC, COLOR_BGC_ITEMFOCUS);
	info.bgc_item_normal = rm->getResColor(ID_CONNECT2PC, COLOR_BGC_ITEMNORMAL);
	info.mainc_3dbox = rm->getResColor(ID_CONNECT2PC, COLOR_MAIN3DBOX);

	hasOpened = true;
	if (!pcDialogFinish)
		return -1;
	pcDialogFinish = false;
	retval = PopUpMenu(hWnd, &info);
	pcDialogFinish = true;
	db_msg("retval is %d\n", retval);
	hasOpened = false;

	return retval;
}

int CdrMain::msgCallback(int msgType, unsigned int data)
{
	ResourceManager* rm = ResourceManager::getInstance();
	int idx = (int)data;
	int ret = 0;
	switch(msgType) {
	case MSG_RM_LANG_CHANGED:
		{
			PLOGFONT logFont;
			logFont = rm->getLogFont();
			if(logFont == NULL) {
				db_error("invalid log font\n");
				return -1;
			}
			db_msg("type:%s, charset:%s, family:%s\n", logFont->type, logFont->charset, logFont->family);
			SetWindowFont(mHwnd, logFont);
		}
		break;
	case MSG_RM_SCREENSWITCH:
		{
			db_msg("MSG_RM_SCREENSWITCH, data is %d, idx is %d\n", data, idx);
			if (idx < 3) {
				if(mPM) {
					mTime = (idx+1)*10;
					mPM->setOffTime(mTime);
				}
			} else {
				if(mPM) {
					mTime = TIME_INFINITE;
					mPM->setOffTime(mTime);
				}
			}
		}
		break;
	case MSG_CONNECT2PC:
		db_msg("connect2Pc dialog\n");
		if (pcDialogFinish == false) {
			break;
		}
		if (!isShuttingdown()) {
			int ret = connectToPCDialog(mHwnd, this);
			if(ret == 0 || ret == IDCANCEL){
				mOtherDialogOpened = false;
				if(m2pcAdasScreen){
						SendMessage(getWindowHandle(WINDOWID_RECORDPREVIEW),MSG_ADAS_SCREEN,1,0);
						m2pcAdasScreen = false;
				}
				db_msg("****connectToPCDialog finish*****");
				mNeedCheckSD = true;
				m2pcConnected = false;
			}
		}
		break;
	case MSG_DISCONNECT_FROM_PC:
		if (!pcDialogFinish) {
			db_msg("close connect2Pc dialog\n");
			closeConnectToPCDialog();
			setUSBStorageMode(false);
		}
		break;
	case MSG_SESSIONCONNECT:
		SendNotifyMessage(mHwnd, MSG_SHOW_TIP_LABEL, LABEL_WIFI_CONNECT, 0);
		break;
	case MSG_SESSIONDISCONNECT:
		BroadcastMessage(MSG_CLOSE_TIP_LABEL, 0, 0);
		break;
	case MSG_RM_FORMAT_TFCARD:
		{
			StorageManager *sm = StorageManager::getInstance();
			ret = sm->format(NULL);
		}
		break;
	case MSG_RM_GET_CAPACITY:
		{
			StorageManager *sm = StorageManager::getInstance();
			aw_cdr_tf_capacity *capacity = (aw_cdr_tf_capacity *)data;
			ret = sm->getCapacity(&(capacity->remain), &(capacity->total));
		}
		break;
	case MSG_GSENSOR_SENSITIVITY:
		{
			mEM->setGsensorThresholdValue(data);
		}
		break;
	case MSG_PARK_SENSITIVITY:
		{
			mGSensorVal = data;
		}
		break;
	case MSG_AUTO_SHUTDOWN:
		db_msg("*********data:%d********",data);
		if( data == 0 ) {
			SendNotifyMessage(mHwnd, MSG_SHOW_TIP_LABEL, LABEL_15S_SHUTDOWN, 0);
				isAutoShutdownEnabled = true;
				modifyNoworkTimerInterval(delayShutdownTime[0][0],delayShutdownTime[0][1]);
		} else if( data == 1 ) {
			SendNotifyMessage(mHwnd, MSG_SHOW_TIP_LABEL, LABEL_30S_SHUTDOWN, 0);
				isAutoShutdownEnabled = true;
				modifyNoworkTimerInterval(delayShutdownTime[1][0],delayShutdownTime[1][1]);
		} else if( data == 2 ) {
			SendNotifyMessage(mHwnd, MSG_SHOW_TIP_LABEL, LABEL_60S_SHUTDOWN, 0);
				isAutoShutdownEnabled = true;
				modifyNoworkTimerInterval(delayShutdownTime[2][0],delayShutdownTime[2][1]);
		} else if( data == 3 ) {
		    SendNotifyMessage(mHwnd, MSG_SHOW_TIP_LABEL, LABEL_CANCEL_SHUTDOWN, 0);
				isAutoShutdownEnabled = false;
				noWork(false);
		}
		break;
	case MSG_FRESET_DELAY_SHUTDOWN:
		if(data < 3){
			isAutoShutdownEnabled = true;
			modifyNoworkTimerInterval(delayShutdownTime[data][0],delayShutdownTime[data][1]);
		}else{
			isAutoShutdownEnabled = false;
			noWork(false);
		}
		break;
	default:
		break;
	}
	return ret;
}

void CdrMain::forbidRecord(int type)
{
	mEnableRecord = type;
}

int CdrMain::isEnableRecord()
{
	return mEnableRecord;
}

static void *lowPowerShutDown(void *ptx)
{
	db_error("low power shut down");
	sleep(5);
	android_reboot(ANDROID_RB_POWEROFF, 0, 0);
    return NULL;
}

int CdrMain::createMainWindows(void)
{
	CDR_RECT rect;
	MAINWINCREATE CreateInfo;
	ResourceManager* rm = ResourceManager::getInstance();
	if(rm->initStage1() < 0) {
		db_error("ResourceManager initStage1 failed\n");
		return -1;
	}

	if(RegisterCDRControls() < 0) {
		db_error("register CDR controls failed\n");
		return -1;
	}
	if(RegisterCDRWindows() < 0) {
		db_error("register CDR windows failed\n");
		return -1;
	}

	CreateInfo.dwStyle = WS_VISIBLE;
	CreateInfo.dwExStyle = WS_EX_NONE | WS_EX_AUTOSECONDARYDC;
	CreateInfo.spCaption = "";
	CreateInfo.hMenu = 0;
	CreateInfo.hCursor = GetSystemCursor(0);
	CreateInfo.hIcon = 0;
	CreateInfo.MainWindowProc = CDRWinProc;

	rm->getResRect(ID_SCREEN, rect);
	CreateInfo.lx = rect.x;
	CreateInfo.ty = rect.y;
	CreateInfo.rx = rect.w;
	CreateInfo.by = rect.h;
	
	CreateInfo.iBkColor = RGBA2Pixel(HDC_SCREEN, 0x00, 0x00, 0x00, 0x00);		/* the color key */
	CreateInfo.dwAddData = (DWORD)this;
	CreateInfo.hHosting = HWND_DESKTOP;

	mHwnd = CreateMainWindow(&CreateInfo);
	if (mHwnd == HWND_INVALID) {
		db_error("create Mainwindow failed\n");
		return -1;
	}
	ShowWindow(mHwnd, SW_SHOWNORMAL);
	rm->setHwnd(WINDOWID_MAIN, mHwnd);

	if(mLowPower<=LOW_POWER ) {
	   EventManager::int2set(rm->getResBoolValue(ID_MENU_LIST_PARK));
       int batteryStatus = EventManager::getBatteryStatus();
       db_msg("***batteryStatus:%d***",batteryStatus);
       if(batteryStatus==BATTERY_DISCHARGING || batteryStatus==BATTERY_NOTCHARGING){
			MSG msg;
			pthread_t thread_id = 0;
			char parkconfig[4]={0};
			cfgDataGetString("persist.app.park.config",parkconfig,"0");
			sys_log("************parkconfig:%s",parkconfig);
			if(strcmp(parkconfig,"1")==0)
				EventManager::int2set(true);
			int err = pthread_create(&thread_id, NULL, lowPowerShutDown, NULL);
			if(err || !thread_id) {
				db_msg("Create lowPowerShutDown  thread error.");
				return -1;
			}
			while(GetMessage(&msg,mHwnd))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			MainWindowThreadCleanup(mHwnd);	
			return -1;
		}
	}
		
	mSTB = new StatusBar(this);
	if(mSTB->getHwnd() == HWND_INVALID) {
		db_error("create StatusBar failed\n");
		return -1;
	}

	if(mRecordPreview->createWindow() < 0) {
		db_error("create RecordPreview Window failed\n");
		return -1;
	}

	ShowWindow(mRecordPreview->getHwnd(), SW_SHOWNORMAL);
	RegisterKeyMsgHook((void*)this, MsgHook);
	return 0;
}

int CdrMain::createOtherWindows(void)
{


	/* create PlayBackPreview */
	mPlayBackPreview = new PlayBackPreview(this);
	if(mPlayBackPreview->getHwnd() == HWND_INVALID) {
		db_error("create PlayBackPreview Window failed\n");
		return -1;
	}
	db_msg("xxxxxxxxxxxxxxxxxxxxDEBUG\n");

	/* create PlayBack */
	mPlayBack = new PlayBack(this);
	if(mPlayBack->getHwnd() == HWND_INVALID) {
		db_error("create PlayBack Window failed\n");
		return -1;
	}
	db_msg("xxxxxxxxxxxxxxxxxxxxDEBUG\n");

	/* CreateMenu */
	mMenu = new Menu(this);
	if(mMenu->getHwnd() == HWND_INVALID) {
		db_error("create PlayBack Window failed\n");
		return -1;
	}
	db_msg("xxxxxxxxxxxxxxxxxxxxDEBUG\n");
	mOtherWindowInited = true;
	return 0;
}

void CdrMain::msgLoop(void)
{
	MSG Msg;
	while (GetMessage (&Msg, this->mHwnd)) {
		TranslateMessage (&Msg);
		DispatchMessage (&Msg);
	}
}

HWND CdrMain::getWindowHandle(unsigned int windowID)
{
	switch(windowID) {
	case WINDOWID_RECORDPREVIEW:
		return mRecordPreview->getHwnd();
		break;
	case WINDOWID_PLAYBACKPREVIEW:
		return mPlayBackPreview->getHwnd();
		break;
	case WINDOWID_PLAYBACK:
		return mPlayBack->getHwnd();
		break;
	case WINDOWID_MENU:
		return mMenu->getHwnd();
		break;
	case WINDOWID_STATUSBAR:
		return mSTB->getHwnd();
		break;
	default:
		db_error("invalid window id: %d\n", windowID);
	}

	return HWND_INVALID;
}

MainWindow* CdrMain::windowID2Window(unsigned int windowID)
{
	switch(windowID) {
	case WINDOWID_RECORDPREVIEW:
		return mRecordPreview;
		break;
	case WINDOWID_PLAYBACKPREVIEW:
		return mPlayBackPreview;
		break;
	case WINDOWID_PLAYBACK:
		return mPlayBack;
		break;
	case WINDOWID_MENU:
		return mMenu;
		break;
	case WINDOWID_STATUSBAR:
		return mSTB;
		break;
	default:
		db_error("invalid window id: %d\n", windowID);
	}

	return NULL;
}

void CdrMain::setCurrentWindowState(windowState_t windowState)
{
	if(mSTB != NULL)
		mSTB->setCurrentWindowState(windowState);
}

void CdrMain::setCurrentRPWindowState(RPWindowState_t status)
{
	if(mSTB != NULL)
		mSTB->setCurrentRPWindowState(status);
}

RPWindowState_t CdrMain::getCurrentRPWindowState(void)
{
	if(mSTB != NULL)
		return mSTB->getCurrentRPWindowState();

	return RPWINDOWSTATE_INVALID;
}

void CdrMain::setCurWindowId(unsigned int windowId)
{
	if(mSTB != NULL)
		mSTB->setCurWindowId(windowId);
}

unsigned int CdrMain::getCurWindowId()
{
	if(mSTB != NULL)
		return mSTB->getCurWindowId();

	return WINDOWID_INVALID;
}

void CdrMain::changeWindow(unsigned int windowID)
{
	HWND toHwnd;
	if (isShuttingdown() || (isRecording() && !mReverseChanged)) {
		return ;
	}
	if ((getCurWindowId() == WINDOWID_RECORDPREVIEW) && (isRecording() && !mReverseChanged)) {
		return ;
	}
	toHwnd = getWindowHandle(windowID);
	if(toHwnd == HWND_INVALID) {
		db_error("invalid toHwnd\n");	
		return;
	}
	db_debug("%d %d", toHwnd, windowID);
	//avoid some problems when force to change from menu to recordpreview on reverse mode
	if (getCurWindowId() == WINDOWID_MENU) {
		mMenu->clearMenuIndicator();
	}
	/*
	 * hide curWindowID and show the toHwnd
	 * */
	ShowWindow(getWindowHandle(getCurWindowId()), SW_HIDE);
	if (getCurWindowId() == WINDOWID_RECORDPREVIEW) {
		SendMessage(getWindowHandle(WINDOWID_RECORDPREVIEW),MSG_ADAS_SCREEN,0,0);
	}
	ShowWindow(toHwnd, SW_SHOWNORMAL);
	SendNotifyMessage(toHwnd, MWM_CHANGE_FROM_WINDOW, getCurWindowId(), 0);

	setCurWindowId(windowID);
}

void CdrMain::changeWindow(unsigned int fromID, unsigned int toID)
{
	if(fromID == getCurWindowId()) {
		changeWindow(toID);
	}
}

void CdrMain::keyProc(int keyCode, int isLongPress)
{
	int ret;
	db_msg("key %x %d\n", keyCode, isLongPress);
	MainWindow* curWin;
	if(keyCode == CDR_KEY_POWER) {
		if (mPM->getStandbyFlag())	//the wrong key info when resume
			return;
		if (isLongPress) {
			CloseTipLabel();
			//ret = CdrDialog(mHwnd, CDR_DIALOG_SHUTDOWN);
			ret = DIALOG_OK;	//to shutdown directly
			if(ret == DIALOG_OK) {
				//				mPM->powerOff();
				shutdown();
			} else if(ret == DIALOG_CANCEL) {
				db_msg("cancel\n");	
			} else {
				db_msg("ret is %d\n", ret);	
			}
		} else {
			db_msg("xxxxxxxxxx\n");
			if (mPM) {
				mPM->screenSwitch();
			}
		}
		return;
	}
	curWin = windowID2Window(getCurWindowId());
	db_msg("curWindowID is %d\n", getCurWindowId());
	if (curWin && (mOtherWindowInited==true) && !mReverseMode) {
		db_msg("xxxxxxxxxx\n");
		DispatchKeyEvent(curWin, keyCode, isLongPress);
	}
}

CdrDisplay *CdrMain::getCommonDisp(int disp_id)
{
	return mRecordPreview->getCommonDisp(disp_id);
}

#ifdef PLATFORM_0
int CdrMain::getHdl(int disp_id)
{
	return mRecordPreview->getHdl(disp_id);
}
#endif

/*
 * DispatchKeyEvent message to mWin
 * */
void CdrMain::DispatchKeyEvent(MainWindow *mWin, int keyCode, int isLongPress)
{
	HWND hWnd;
	unsigned int windowID;

	hWnd = mWin->getHwnd();
	if( hWnd && (hWnd != HWND_INVALID)) {
		windowID = SendMessage(hWnd, MSG_CDR_KEY, keyCode, isLongPress);
		db_msg("windowID is %d\n", windowID);
		if((windowID != getCurWindowId()) )
			changeWindow(windowID);
	}
}

bool CdrMain::isScreenOn()
{
	if (mPM) {
		return mPM->isScreenOn();
	}
	return true;
}

void CdrMain::setOffTime()
{
	if(mPM)
		mPM->setOffTime(mTime);
}

int CdrMain::enterParkMode()
{
	int ret = 0;
	char parkconfig[4]={0};
	db_msg(" ");
	cfgDataGetString("persist.app.park.config",parkconfig,"0");
	if(mEM->isBatteryOnline() && (strcmp(parkconfig, "1")==0 )){
		isWakenUpOnParking = true;
		//开始录像15S后关机
		db_msg("PM: parking monitor detect crash!\n");
			unsigned int winId = getCurWindowId();
			if(winId == WINDOWID_RECORDPREVIEW){
				RPWindowState_t state = mRecordPreview->getRPWindowState();
				if(state == STATUS_PHOTOGRAPH){
					mRecordPreview->setRPWindowState(STATUS_RECORDPREVIEW);
				}
			}else if(winId == WINDOWID_MENU){
				mMenu->storeData();
				changeWindow(winId,WINDOWID_RECORDPREVIEW);
			}else{
				changeWindow(winId,WINDOWID_RECORDPREVIEW);
			}
			mRecordPreview->needCache(false);
			mRecordPreview->storagePlug(true);
			noWork(false);
			ret = mRecordPreview->parkRecord();
			if (ret < 0) {
				return ret;
			}
			StorageManager *sm = StorageManager::getInstance();
			sm->geneImpactTimeFile();
		
		return -1;
	}else{
		if(!access(GENE_IMPACT_TIME_FILE, F_OK)) {
			ResourceManager *rm = ResourceManager::getInstance();
			int data = rm->getResIntValue(ID_MENU_LIST_VTL, INTVAL_SUBMENU_INDEX);
			SendMessage(getWindowHandle(WINDOWID_STATUSBAR), STBM_LOOPCOVERAGE, data, 0);	
			SendMessage(getWindowHandle(WINDOWID_RECORDPREVIEW), MSG_RM_VIDEO_TIME_LENGTH, data, 0);
			BroadcastMessage(MSG_CLOSE_TIP_LABEL, 0, 0);
			SendNotifyMessage(mHwnd, MSG_SHOW_TIP_LABEL, LABEL_IMPACT_NUM, 0);
			mTinyParkAudioTh = new TinyParkAudioThread(this,(char*)"/system/res/others/parknotice.wav");
			int ret = mTinyParkAudioTh->start();
			if (ret != OK) {
				db_msg("TinyParkAudioThread start failed\n");
			}
		}
	}
	return 0;
}


void CdrMain::startUp()
{
	bool val;
	ResourceManager* rm = ResourceManager::getInstance();
	if (rm->getSubMenuCurrentIndex(ID_MENU_LIST_PARK) ) {
		
		int ret = enterParkMode();
		if (ret < 0) {
			return ;
		}
	}
	
	val = rm->getResBoolValue(ID_MENU_LIST_AWMD);
	if(val == true) {
		db_info("AWMD mode\n");
	} else {
		val = rm->getResBoolValue(ID_MENU_LIST_POR); //power on record
		
		if (val == true) {
			db_info("POR: power on record\n");
			if (getCurWindowId() == WINDOWID_RECORDPREVIEW) {
				mRecordPreview->storagePlug(true);
				mRecordPreview->startRecord();
			}
		}
	}
}

void CdrMain::configInit()
{
	ResourceManager* rm = ResourceManager::getInstance();
	int ret = 0;
	/*TODO*/
	int time = rm->getResIntValue(ID_MENU_LIST_SS, INTVAL_SUBMENU_INDEX);
	if (time < 3) {
		time = (time+1)*10;
	} else {
		time = TIME_INFINITE;
	}
	mPM->setOffTime(time);
	rm->notifyAll();
	startUp();
	//mPM->adjustCpuFreq();
}

void CdrMain::exit()
{
	delete mEM;
	mEM = NULL;
	delete mPM;
	mPM = NULL;
}

void CdrMain::prepareCamera4Playback(bool closeLayer)
{
	if(mRecordPreview)
		mRecordPreview->prepareCamera4Playback(closeLayer);
}

void CdrMain::setUSBStorageMode(bool enable)
{
	if(enable == true) {
		if(usbStorageConnected == false) {
			db_msg("enable USB Storage mode\n");
			if (isRecording()) {
				mRecordPreview->stopRecordSync();
			}
			mPlayBack->stopPlaying();
			StorageManager *sm = StorageManager::getInstance();
			sm->stopUpdate();
			mEM->connect2PC();
			usbStorageConnected = true;
			if ( getCurWindowId()== WINDOWID_PLAYBACKPREVIEW) {
				SendMessage(mHwnd, MWM_CHANGE_WINDOW, WINDOWID_PLAYBACKPREVIEW, WINDOWID_RECORDPREVIEW);
			}

		}
	} else {
		if(usbStorageConnected == true)	{
			db_msg("disable USB Storage mode\n");
			mEM->disconnectFromPC();
			StorageManager *sm = StorageManager::getInstance();
			sm->dbReset();
			usbStorageConnected = false;
		}
	}
}

static unsigned char *getShutdownLogo()
{
	int fd;
	unsigned char *buf = NULL;
	int buf_size = 512*1024;

	fd = open("/dev/block/mtdblock5", O_RDWR);
    if( fd < 0) {
    	db_error("open shutlogo fail\n");
    }
	buf  = (unsigned char *)malloc(buf_size);
	read(fd, buf, buf_size);
#ifdef DEBUG_SHUTDOWN_LOGO	
	for(int i = 0; i< 100; i++) {
			db_error("buf[%d]=0x%x\n",i*4,*(unsigned int *)(buf+i*4));
	}
#endif
	return buf;

}

void CdrMain::showShutdownLogo(void)
{
	HWND hStatusBar, hMenu, hPBP;
	CdrDisplay* cd;
	unsigned char *buf = NULL;

	cd = getCommonDisp(CAM_CSI);
	if(cd)
		ck_off(cd->getHandle());
	cd = getCommonDisp(CAM_UVC);
	if(cd)
		ck_off(cd->getHandle());
	hStatusBar = getWindowHandle(WINDOWID_STATUSBAR);
	hMenu = getWindowHandle(WINDOWID_MENU);
	hPBP = getWindowHandle(WINDOWID_PLAYBACKPREVIEW);
	ShowWindow(hStatusBar, SW_HIDE);
	SendMessage(hStatusBar, MSG_CLOSE, 0, 0);	//cancel statusbar timer
	ShowWindow(hMenu, SW_HIDE);
	ShowWindow(hPBP, SW_HIDE);
//	showImageWidget(mHwnd, IMAGEWIDGET_POWEROFF);
	buf = getShutdownLogo();
	
	CloseTipLabel();
    showImageWidget(mHwnd, IMAGEWIDGET_POWEROFF, buf);
	  free(buf);

}

bool CdrMain::getUSBStorageMode(void)
{
	return usbStorageConnected;
}
bool CdrMain::getWakeupState(void )
{
	return isWakenUpOnParking;
}

void CdrMain::setWakeupState(bool flag)
{
	isWakenUpOnParking = flag;
}

bool CdrMain::isShuttingdown(void)
{
	return mShutdowning;
}

void CdrMain::setSTBRecordVTL(unsigned int vtl)
{
	mSTB->setRecordVTL(vtl);
}

bool CdrMain::getUvcState()
{
#ifdef IGNORE_UVC_STATE
	return false;
#else
	return mRecordPreview->getUvcState();
#endif
}

int CdrMain::camChange(bool session, int param_in)
{
	int ret = 0;

	if (session) {
		if (param_in < CAM_CNT) {
			ret = mRecordPreview->camChange(param_in);
		}
	} else {
		if (param_in) {
			ret = mRecordPreview->startRecord();
		} else {
			ret = mRecordPreview->stopRecord();
		}
	}
	return ret;
}

int CdrMain::sessionControl(int cmd, int param_in, int param_out)
{	
    db_msg(" ");
	ResourceManager* rm = ResourceManager::getInstance();
       if (cmd == NAT_CMD_GET_CONFIG ) {
       //set recording status to resource manager when it wants to get config
       rm->setRecordingStatus(mRecordPreview->isRecording());
       } else if (cmd == NAT_CMD_REQUEST_VIDEO) {
               return camChange(true, param_in);
       } else if (cmd == NAT_CMD_RECORD_SWITCH) {      //switch local recorder
               return camChange(false, param_in);
       } else if (cmd == NAT_CMD_SET_VIDEO_SIZE) {
 	      		aw_cdr_set_video_quality_index *p = (aw_cdr_set_video_quality_index*)param_in;
				mRecordPreview->changeResolution(p->video_quality_index);
				return mCdrServer->reply(NAT_CMD_SET_VIDEO_SIZE_ACK, 0);
	   } else if (cmd == NAT_WIFI_CONNECT) {
	   			aw_cdr_wifi_state *p = (aw_cdr_wifi_state*)param_in;
				if (p->state == 0) {
					mSessionConnect = false;
					SendMessage(mHwnd, MSG_SESSIONDISCONNECT, 0, 0);
				}
				else {
					mSessionConnect = true;
					SendMessage(mHwnd, MSG_SESSIONCONNECT, 0, 0);
				}
				return mCdrServer->reply(NAT_WIFI_CONNECT_ACK, 0);
	   }
	   else if (cmd == NAT_CMD_LIMIT_SPEED){
	   			db_msg("AAAAA NAT_CMD_LIMIT_SPEED =  \n");	
				aw_cdr_limit_speed *p = (aw_cdr_limit_speed *)param_in;
				if (NULL != p){
					setWriteSleepTime(p->speedlimit);
					db_msg("AAAAA p->speedlimit =  %d\n", p->speedlimit);	
					return mCdrServer->reply(NAT_CMD_LIMIT_SPEED_ACK, 0);
				}
				else{
					db_msg("AAAAA p->speedlimit =  error \n");	
					return mCdrServer->reply(NAT_CMD_LIMIT_SPEED_ACK, -1);
				}
	   }
	   	
       db_msg(" ");
       return rm->sessionControl(cmd, param_in, param_out);
}

int CdrMain::lockFile(const char *file)
{
	return sessionNotify(NAT_EVENT_SOS_FILE, (int)file);
}

int CdrMain::sessionNotify(int cmd, int param_in)
{
	int ret = 0;
#ifdef APP_WIFI
	if (mCdrServer) {
		ret = mCdrServer->notify(cmd, param_in);
	}
#endif
	return ret;
}

bool CdrMain::isWifiConnecting()
{
	return mSessionConnect;
}

void CdrMain::otaUpdate()
{
	db_msg("***getpid=%d***",getpid());
    if(mRecordPreview->isRecording())
		mRecordPreview->stopRecord();
	mRecordPreview->stopPreview();
	mPM->enterStandby(getpid());
}

void CdrMain::fResetWifiPWD()
{
#ifdef APP_WIFI
	if(!mConnManager){
		mConnManager =  new ConnectivityManager();
	}
	if(mConnManager->fResetWifiPWD("66666666")!=0){
		db_msg("factory reset wifi's password failed");
	}
#endif
}

bool CdrMain::getIsLowPower()
{
	if(mLowPower<=LOW_POWER) {
		return TRUE;
	}else{
		return false;
	}
}

void CdrMain::keepAdasScreenFlag()
{
	if(getCurWindowId()== WINDOWID_RECORDPREVIEW&& mRecordPreview->getIsAdasScreen()){
		SendMessage(getWindowHandle(WINDOWID_RECORDPREVIEW),MSG_ADAS_SCREEN,0,0);
		mOriginAdasScreen = true;
	}
}

void CdrMain::recoverAdasScreenFlag()
{
	if(mOriginAdasScreen){
		SendMessage(getWindowHandle(WINDOWID_RECORDPREVIEW),MSG_ADAS_SCREEN,1,0);
		mOriginAdasScreen = false;
	}
}

StatusBar * CdrMain::getStatusBar(){
	return mSTB;
}

void CdrMain::TinyPlayAudio(char *path)
{
    char player[128]="tinyplay ";
    system("tinymix 16 1");
    system("tinymix 1 29");
    strcat(player, path);
    system(player);
}

void CdrMain::startPCCam()
{
	db_msg(" ");
	mPCCam = true;
	if (isRecording()) {
		mRecordPreview->stopRecordSync();
	}
	mRecordPreview->startPCCam();
}

void CdrMain::stopPCCam()
{
	db_msg(" ");
	mPCCam = false;
	mRecordPreview->stopPCCam();
}

bool CdrMain::getPCCamMode()
{
	return mPCCam;
}

int CdrMain::releaseResource()
{
       return 0;
}

RecordPreview * CdrMain::getRecordPreview()
{
	return mRecordPreview;
}

void CdrMain::stopPreviewOutside(int flag)
{
	mRecordPreview->stopPreviewOutside(flag);
}

void CdrMain::parkRecordTimer(bool flag)
{
	if(flag){
		db_msg("start TIMER_PARK_RECORD_IDX");
		startShutdownTimer(TIMER_PARK_RECORD_IDX);
	}else{
		db_msg("stop TIMER_PARK_RECORD_IDX");
		stopShutdownTimer(TIMER_PARK_RECORD_IDX);
	}
}

int CDRMain(int argc, const char *argv[], CdrMain* cdrMain)
{
	struct timespec measureTime1;
	clock_gettime(CLOCK_MONOTONIC, &measureTime1);
	db_msg("Test Time: int CDRMain time is %ld secs, %ld nsecs\n", measureTime1.tv_sec, measureTime1.tv_nsec);

	if(cdrMain->createMainWindows() < 0) {
		delete cdrMain;
		return -1;
	}
	clock_gettime(CLOCK_MONOTONIC, &measureTime1);
	db_msg("Test Time: after createMainWindows time is %ld secs, %ld nsecs\n", measureTime1.tv_sec, measureTime1.tv_nsec);

	cdrMain->initStage2(NULL);
	clock_gettime(CLOCK_MONOTONIC, &measureTime1);
	db_msg("Test Time: after initStage2 time is %ld secs, %ld nsecs\n", measureTime1.tv_sec, measureTime1.tv_nsec);

	cdrMain->msgLoop();

	delete cdrMain;
	return 0;
}

#ifdef _MGRM_THREADS
#include <minigui/dti.c>
#endif
