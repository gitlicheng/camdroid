#ifndef _WINDOWS_H
#define _WINDOWS_H
#include <platform.h>
#include <CedarMediaPlayer.h>
#include <utils/KeyedVector.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>


#define DE2


#include "RecordPreview.h"
#include "CdrCamera.h"
#include "CdrMediaRecorder.h"
#include "PlayBack.h"

#include "PowerManager.h"
#include "StorageManager.h"
#include "ResourceManager.h"
#include "cdr_widgets.h"
#include "CdrIcon.h"
#include "CdrLabel.h"
#include "EventManager.h"
#include "ConnectivityManager.h"
#include "CdrServer.h"
#include "CdrTimer.h"
#include "cardtest.h"

//(2,0)
#define UI_LAY_ID 8


class CdrServer;
class MainWindow;
typedef struct tPic {
    char* data;
    int data_len;
} tPic;

enum {
	PIC_REVERSE = 1,
};

enum cmd_e {
	CMD_CLOSE_LAYER = 1,
	CMD_STOP_PREVIEW = 2,
	CMD_NO_NEED_STOP_PREVIEW = 4,
	CMD_CLEAN_LAYER = 8,
};

enum playerState {
	STATE_ERROR					= -1,
	STATE_IDLE					= 0,
	STATE_PREPARING				= 1,
	STATE_PREPARED				= 2,
	STATE_STARTED				= 3,
	STATE_PAUSED				= 4,
	STATE_PLAYBACK_COMPLETED	= 5,
	STATE_STOPPED				= 6,
	STATE_END					= 7
};

enum {
	FORBID_NORMAL = 0,
	FORBID_FORMAT,
};

enum AvaliableMenu{
	MENU_OBJ_START = 1,
	MENU_OBJ_SYSTEM = MENU_OBJ_START,
	MENU_OBJ_RECORDPREVIEW,
	MENU_OBJ_PHOTO,
	MENU_OBJ_END
};

enum {
	SPEC_CMD_LAYER_CLEAN = 1,
};

#define WINDOW_STATUSBAR		"CDRStatusBar"
#define WINDOW_RECORDPREVIEW	"RecordPreview"
#define WINDOW_MENU				"CDRMenu"
#define WINDOW_PLAYBACKPREVIEW	"PlayBkPreview"
#define WINDOW_PLAYBACK			"PlayBack"
#define DOSBOOTBLOCKSIZE 		512

#define SELF_HAS_PROCESS        0
#define WANT_SYSTEM_PROCESS     1

#define UPDATE_FILE_PATH               MOUNT_PATH"full_img.fex"

extern int RegisterCDRWindows(void);
extern void UnRegisterCDRWindows(void);

typedef enum {
	STATUS_RECORDPREVIEW = 1,
	STATUS_PHOTOGRAPH = 2,
	STATUS_AWMD	= 3,
	RPWINDOWSTATE_INVALID,
}RPWindowState_t;

typedef enum {
	PARK_MON_VTL_10S = 10,
	PARK_MON_VTL_15S = 15,
	PARK_MON_VTL_20S = 20,
	PARK_MON_VTL_30S = 30,
}PARK_MON_VTL_t;


typedef struct {
	unsigned int curWindowID;
	RPWindowState_t RPWindowState;
}windowState_t;

class CdrMain;
class PowerManager;
class MainWindow
{
public:
	MainWindow(){mHwnd = 0;}
	virtual ~MainWindow(){};
	HWND getHwnd() {
		return mHwnd;
	}

	void updateWindowFont(void)
	{
		HWND hChild;

		hChild = GetNextChild(mHwnd, 0);
		while( hChild && (hChild != HWND_INVALID) )
		{
			SetWindowFont(hChild, GetWindowFont(mHwnd));
			hChild = GetNextChild(mHwnd, hChild);
		}
	}
	void showWindow(int cmd) {
		ShowWindow(mHwnd, cmd);
	}
	void keyProc(int keyCode, int isLongPress);
	virtual int releaseResource()=0;
protected:
	HWND mHwnd;
};

class StatusBar : public MainWindow
{
public:
	StatusBar(CdrMain* cdrMain);
	~StatusBar();

	int createSTBWidgets(HWND hParent);
	void msgCallback(int msgType, unsigned int data);
	int handleRecordTimerMsg(unsigned int message, unsigned int recordType);
	void setRecordVTL(unsigned int vtl);
	void setCountOnce(bool val);

	void setStatusBarLabel(ResourceID id, const char* value);
	void setStatusBarLabel(ResourceID id, const char* value, unsigned int winid);
	void setStatusBarIcon(ResourceID id, unsigned int index);
	void setWindowPic(unsigned int windowPicId);
	void setLabelRecordTime(const char* str);
	void setLabelRecordTime(const char* str, unsigned int winid);
	void needStop();

	void refreshSystemTime(void);
	void showSystemTime(void);
	void hideSystemTime(void);

	void setCurrentWindowState(windowState_t windowState);
	void setCurrentRPWindowState(RPWindowState_t status);
	RPWindowState_t getCurrentRPWindowState(void);
	void setCurWindowId(unsigned int windowId);
	unsigned int getCurWindowId();
	void updateWindowPic(void);
	int getCurRecordVTL();
	int getRecordVTL();
	int releaseResource();
private:
	CdrMain* mCdrMain;
	windowState_t mCurWindowState;
	HWND mHwndTop, mHwndBottom;

	CdrIconOps* mIconOps;
	CdrLabelOps* mLabelOps;
	gal_pixel mFgColor;
	gal_pixel mBgColor;
	DWORD  mRecordTime;
	DWORD  mRecordVTL;
	DWORD  mCurRecordVTL;
	bool mCountOnce;
	CdrTimer *mTimer;
	Mutex mTimerLock;
	bool mLabelDisp;
	bool mFirstShow;

	int updateRecordTime(bool setLabelBlank=false);
	void setLabelDisp(bool val);
	bool getLabelDisp();
	int increaseRecordTime();

	void changeStatusBarAlpha(void);
	void showRecordPreviewModeIcons(void);
	void hideRecordPreviewModeIcons(void);
	void showPhotoModeIcons(void);
	void hidePhotoModeIcons(void);
	void showPlaybackPreviewIcons(void);
	void hidePlaybackPreviewIcons(void);
	void showPeripheralAreaIcons(void);
	void hidePeripheralAreaIcons(void);
};
template <class T> class LimitBase {
public:
	LimitBase(){}
	virtual ~LimitBase(){};
	virtual void force(bool flag)=0;
	virtual void reverse()=0;
public:
	Mutex mLock;
};


class RecordPreview : public MainWindow, public RecordListener, public ErrorListener
{
public:
	RecordPreview(CdrMain* cdrMain);
	~RecordPreview();
	void init();
	int createWindow();
	virtual void recordListener(CdrMediaRecorder* cmr, int what, int extra);
	virtual void errorListener(CdrMediaRecorder* cmr, int what, int extra);
	int startRecord();
	int startRecord(int cam_type);
	int startRecord(int cam_type, CdrRecordParam_t param);
	int stopRecord(int cam_type);
	int stopRecord();
	int stopRecordSync();
	void startPreview();
	void startPreview(int cam_type);
	void stopPreview(int cam_type);
	void stopPreview();

	bool isRecording(int cam_type);
	bool isRecording();
	bool isPreviewing(int cam_type);
	bool isPreviewing();
	CdrDisplay *getCommonDisp(int disp_id);

	int keyProc(int keyCode, int isLongPress);
	int msgCallback(int msgType, int idx);

	int setCamExist(int cam_type, bool bExist);
	bool isCamExist(int cam_type);
	void feedAwmdTimer(int seconds);
	int queryAwmdExpirationTime(time_t* sec, long* nsec);
	void updateAWMDWindowPic();
	void savePicture(void* data, int size, int id);
	int impactOccur();
    void prepareCamera4Playback(bool closeLayer=true);
	void restorePreviewOutSide(unsigned int windowID);

	void switchVoice();
	void setSdcardState(bool bExist);
	void otherScreen(int screen);
	void storagePlug(bool plugin);
	bool hasTVD(bool &hasTVD);
	bool needDelayStartAWMD(void);	/* set flag, delay some time after stopRecord, when awmd is open */
	void setDelayStartAWMD(bool value);
	void restoreZOrder();
	unsigned int mAwmdDelayCount;
	bool getUvcState();
	void takePicFinish(int cam_id);
	int CalculateRecordTime(int camID = CAM_CSI);
	void setRecordTime(PARK_MON_VTL_t idx);
	void releaseRecorder(int cam_id);
	
	void sessionInit(void *rtspSession);
	int camChange(int cam_idx, bool bStop=true);
	bool camExisted(int idx);
#ifdef PLATFORM_0
	int getHdl(int disp_id);
	void createVirtualCam();
	void destroyVirtualCam();
	void setVirtualCam(CdrMediaRecorder *cmr);
#endif
	void changeResolution(int level);
	unsigned int  getWindowsID();
	bool getAlignLine();
	void setAlignLine(bool flag);
	void lockFile(bool flag);
	void condLock(bool bLock);
	void adasScreenSwitch(bool flag);
	bool getIsAdasScreen();
	void getAdasCallback();
	int getDuration();
	void needCache(bool val);
	void startPCCam();
	void stopPCCam();
        uint32_t mBitRate ;
	int releaseResource();
	void suspend();
	void resume();
	void checkTFLeftSpace();
	bool allowShowTip();
	void errorOccur(int error);
	int checkStorage();
	RPWindowState_t getRPWindowState();
	void setRPWindowState(RPWindowState_t state);
	bool getAllowTakePic();
	void setRecordTime(int idx);
	void setWatermark(int flag);
	class VTLThread: public Thread{
		public:
			VTLThread(RecordPreview *mp)
				: Thread(false),
				  mrp(mp)
			{
				mRequestExit = false;
				isRun = false;
			}
			~VTLThread(){};
			virtual bool threadLoop() {
				if (mRequestExit) {
					return false;
				}
				mrp->checkTFLeftSpace();
				sleep(3);
				return true;
			}
			void stopThread() {
				isRun = false;
				mRequestExit = true;
			}
			status_t start() {
				ALOGD("   VTLThread start()  ");
				isRun = true;
				mRequestExit = false;
				return run("VTLThread", PRIORITY_NORMAL);
			}
			void setRequestExit(bool flag){
				ALOGD("            mRequestExit:%d",mRequestExit);
				mRequestExit = flag;
			}
			bool isRuning(){
				return isRun;
			}
		private:
			bool isRun;
			RecordPreview *mrp;
			bool mRequestExit;
		};
	void stopPreviewOutside(int flag);
	int parkRecord();
	int specialProc(int cmd, int arg);
	void resetRecorder();
	void set_ui_visible(bool on);
	void reverseMode(bool mode);
	void showPicFullScreen(int mode, bool bDisp);
private:
	class FileLock : public LimitBase<bool> {
	public:
		FileLock(RecordPreview *rm):mRM(rm),mData(false),mForced(false){}
		void force(bool flag);
		void reverse();
		bool getStatus();
	private:
		bool mData;
		bool mForced;
		RecordPreview *mRM;
	};

	int initCamera(int cam_type, bool needNewCamera=true);
	int prepareRecord(int cam_type, CdrRecordParam_t param, Size *resolution=NULL);


	void recordRelease(int cam_type);
	void recordRelease();

	void previewRelease(int cam_type);
	void previewRelease();

	void setSilent(bool mode);
	void setVideoQuality(int idx);

	uint32_t camPreAllocSize(int mDuration , uint32_t bitrate);
	void setVideoBitRate(int idx);
	
	void setPicResolution(int idx);
	void setPicResolution(int width,int height);
	void setPicQuality(int idx);
	void setAWMD(bool val);	
	void setADAS(bool val);
	void switchAWMD();
	void switchAWMD(bool enable);
	int transformPIP(pipMode_t newMode);
	int takePicture(int flag);
	void setDeleteFlag(int cam_type, String8 file);
	void clrDeleteFlag(int cam_type, String8 file);
	String8 getDeleteFlag(int cam_type);
	void setCyclicVTL_Flag(bool flag);
	bool getCyclicVTL_Flag(void);
	CdrMain *mCdrMain;
	CdrCamera *mCamera[CAM_CNT];
	CdrMediaRecorder *mRecorder[CAM_CNT+2];

	Mutex mLock;
	Mutex mLockPic;
	Size mVideoSize;
	RPWindowState_t mRPWindowState, mOldRPWindowState;
	pipMode_t mPIPMode,oldPIPMode;
	recordDBInfo_t mRecordDBInfo[CAM_CNT];
	recordDBInfo_t mExtraDBInfo[CAM_CNT];
	int mDuration;
	bool mCamExist[CAM_CNT];
	String8 mNeedDelete[CAM_CNT];
	CDR_RECT rectBig, rectSmall;
	Mutex mImpactLock;
	Mutex mRecordLock;
	Mutex mDeleteLock;
	Mutex mCyclicLock;
	bool mLockFlag[CAM_CNT];
	int mImpactFlag[CAM_CNT];
	String8 mBakFileString[CAM_CNT];
	time_t mBakNow[CAM_CNT];
	bool mHasTVD;
	int mUVCNode;
	bool mAwmdDelayStart;
	Mutex mAwmdLock;
	bool mSilent;
	bool mAllowTakePic[CAM_CNT];

	bool mNeedCyclicVTL;
	bool mStartRtspSession;
	bool mAlignLine;
	bool mIsAdasScreen;
#ifdef PLATFORM_0
	VirtualCamera *mVirHC;
#endif
	Size mSessionSize;
	void *mSession;
	int mCurSessionCam;
	bool mNeedCache;
	bool mImpactLockFlag;
	bool snapflag;
	sp<VTLThread> mVtlThread;
	char strLWM[30];
	bool mLWM;
	bool mTWM;
	bool mNeedStartPreview;
	String8 mOrignFileName[CAM_CNT];
	FileLock *mFilelock;
	BITMAP mReverseBmp;
};

class PlayBackPreview : public MainWindow
{
public:
	PlayBackPreview(CdrMain* pCdrMain);
	~PlayBackPreview();

	int keyProc(int keyCode, int isLongPress);
	int createSubWidgets(HWND hWnd);

	int getFileInfo(void);
	int updatePreviewFilelist(void);
	void showThumbnail_ths();
	void showThumbnail();
	int getPicFileName(String8 &picFile);
	int getBmpPicFromVideo(const char* videoFile, const char* picFile);
	bool isCurrentVideoFile(void);
	void nextPreviewFile();
	void prevPreviewFile();
	void getCurrentFileName(String8 &file);
	void getCurrentFileInfo(PLBPreviewFileInfo_t &info);
	void showPlaybackPreview();
	void deleteFileDialog();
	void showBlank();
	void lockedCurrentFile();
	void fixIdx();
#ifdef PLATFORM_0
	void otherScreen(int screen);
	int beforeOtherScreen();
	int afterOtherScreen(int layer, int screen);
#endif
	int getFileList(char **file_list);
	int msgCallback(int message, int data);
	int mCurID;
	void prepareCamera4Playback(int flag=0);
	int releaseResource();
	class STAThread : public Thread
	{
	public:
		STAThread(PlayBackPreview *playbackpreview): Thread(false),pbp(playbackpreview){	}
		status_t start() {
			return run("STAThread", PRIORITY_NORMAL);
		}
		virtual bool threadLoop() {
			pbp->showThumbnail_ths();
			return false;
		}

	private:
		PlayBackPreview *pbp;
	};
private:
	CdrMain* mCdrMain;
	BITMAP bmpIcon;
	BITMAP bmpImage;
	PLBPreviewFileInfo_t fileInfo;
	int playBackVideo();
#ifdef PLATFORM_0
	int mDispScreen;
	CDR_RECT mDispRect;
#endif
#ifdef FATFS
	tPic mThumbPic;
#endif
	bool mFinishedGen;
	bool firstPic;
	bool mNeedCloseLayer;
};

class PlayBack : public MainWindow
{
public:
	PlayBack(CdrMain *cdrMain);
	~PlayBack();
	int createSubWidgets(HWND hWnd);
	int keyProc(int keyCode, int isLongPress);
	void setDisplay(int hlay);
	int preparePlay(String8 filePath);
	int PreparePlayBack(String8 filePath);
	int startPlayBack();
	void start();
	void pause();
	void seek(int msec);
	void stop();
	void release();
	void reset();
	playerState getCurrentState();
	void initProgressBar(void);
	void resetProgressBar(void);
	void updateProgressBar(void);
	bool isStarted();
	void noWork(bool idle);
	void prepareCamera4Playback();
	int stopPlayback(bool bRestoreAlpha=true);
	void sendToPBP(int mId);
	int mCurId;
	void setPlayerVol(int val);
	void complete();
#ifdef PLATFORM_0
	int restoreAlpha();
#endif
	void stopPlaying();
	class PlayBackListener : public CedarMediaPlayer::OnPreparedListener
							, public CedarMediaPlayer::OnCompletionListener
							, public CedarMediaPlayer::OnErrorListener
							, public CedarMediaPlayer::OnVideoSizeChangedListener
							, public CedarMediaPlayer::OnInfoListener
							, public CedarMediaPlayer::OnSeekCompleteListener
	{
	public:
		PlayBackListener(PlayBack *pb){mPB = pb;};
		void onPrepared(CedarMediaPlayer *pMp);
		void onCompletion(CedarMediaPlayer *pMp);
		bool onError(CedarMediaPlayer *pMp, int what, int extra);
		void onVideoSizeChanged(CedarMediaPlayer *pMp, int width, int height);
		bool onInfo(CedarMediaPlayer *pMp, int what, int extra);
		void onSeekComplete(CedarMediaPlayer *pMp);
	private:
		PlayBack *mPB;
	};
	int releaseResource();
public:
	playerState mCurrentState;
	playerState mTargetState;
	/* if user send the msg MSG_PLB_COMPLETE with wParam 0, 
	 * then mStopFlag will be set to 1, indicate onCompletion not send MSG_PLB_COMPLETE again*/
	unsigned int mStopFlag;	
private:
	CdrMain *mCdrMain;
	CdrDisplay *mCdrDisp;
	CedarMediaPlayer *mCMP;
	PlayBackListener *mPlayBackListener;
	BITMAP bmpIcon;
};

class Menu : public MainWindow
{
public:
	Menu(CdrMain* pCdrMain);
	~Menu();
	int releaseResource();
	class MenuObj
	{
	public:
		MenuObj(class Menu* parent, unsigned int menulistCount, AvaliableMenu menuObjId);
		virtual ~MenuObj();

		/*
		 * if return value is SELF_HAS_PROCESS, then no need to pass to the system proc
		 * if return value is WANT_SYSTEM_PROCESS, then need to pass to the system proc
		 * */ 
		virtual int menuObjWindowProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam) = 0;
		virtual int keyProc(int keyCode, int isLongPress) = 0;
		virtual int HandleSubMenuChange(unsigned int menuIndex, int newSel) = 0;
		virtual int HandleSubMenuChange(unsigned int menuIndex, bool newSel) = 0;
		
		HWND getHwnd()			{ return mHwnd; }
		AvaliableMenu getId()	{ return mMenuObjId; }
		WNDPROC getOldProc()	{ return mOldProc; }

		int getMenuListAttr(menuListAttr_t &attr);
		int getItemImages(MENULISTITEMINFO *mlii);
		int getItemStrings(MENULISTITEMINFO* mlii);
		int getFirstValueStrings(MENULISTITEMINFO* mlii, int menuIndex);
		int getFirstValueImages(MENULISTITEMINFO* mlii, int menuIndex);
		int getSecondValueImages(MENULISTITEMINFO* mlii, MLFlags* mlFlags);
		int createMenuListContents(HWND hMenuList);

		int updateMenuTexts(void);
		int updateSwitchIcons(void);
		void updateMenu(void);
		int updateNewSelected(int oldSel, int newSel);

		int getSubMenuData(unsigned int subMenuIndex, subMenuData_t &subMenuData);
		int ShowSubMenu(unsigned int menuIndex, bool &isModified);

		int getCheckBoxStatus(unsigned int menuIndex, bool &isChecked);
	protected:
		class Menu* mParentContext;
		HWND mHwnd;
		WNDPROC mOldProc;
		menuListAttr_t mMenuListAttr;
		AvaliableMenu mMenuObjId;
		BITMAP *mItemImageArray;
		BITMAP *mValue1ImageArray;
		BITMAP mUnfoldImageLight;
		BITMAP mUnfoldImageDark;
		unsigned int mMenuListCount;

		// menu resourceID array according to the menu index
		// the following varibles should be inited in the virtual function initMenuObjParams
		enum ResourceID* mMenuResourceID;	
		unsigned int* mHaveCheckBox;
		unsigned int* mHaveSubMenu;
		unsigned int* mHaveValueString;
		int* mSubMenuContent0Cmd;
		MLFlags* mMlFlags;
		// end of the menuObj params
	};

public:
	int keyProc(int keyCode, int isLongPress);
	int hand_message(unsigned int menuIndex,bool newSel);	
	void Card_speed_disp(TestResult *test_result );
	int Card_speed_disp_lable(void);
	void onFontChanged();
	void onNewSelected(int oldSel, int newSel);
	// called from the MWM_CHANGE_FROM_WINDOW
	void onChangeFromOtherWindow(unsigned int fromId);

	void showAlphaEffect(void);
	void cancelAlphaEffect(void);

	// get the common messagebox data, need be call by the getMessageBoxData in the MenuObj
	int getCommonMessageBoxData(MessageBox_t &messageBoxData);
	int ShowMessageBox(MessageBox_t* data, bool alphaEffect);

	CdrMain * getCdrMain();

	void formatTfCardOutSide(bool param);

	void focusCurrentMenuObj(void);
	int getCurMenuObjId();
	void setSubMenuFlag(bool flag);
	bool getSubMenuFlag();
	int storeData();
	MenuObj* getMenuObj(enum AvaliableMenu menuObjId);
	void clearMenuIndicator(void);
private:
	CdrMain* mCdrMain;
	enum AvaliableMenu mCurMenuObjId;
	KeyedVector <enum AvaliableMenu, class MenuObj*> mMenuObjList;
	KeyedVector <enum AvaliableMenu, PBITMAP> mMenuObjIcons;
	KeyedVector <enum AvaliableMenu, enum LANG_STRINGS> mMenuObjHintIds;
	Vector <AvaliableMenu> mAvailableMenuObjIds;
	HWND mHwndIndicator;

	int createMenuObjs(HWND hwnd);
	MenuObj* getCurrentMenuObj();
	int hideMenuObj(enum AvaliableMenu menuObjId);
	int showMenuObj(enum AvaliableMenu menuObjId);
	void setMenuObjId(enum AvaliableMenu menuObjId);
	void updateMenu(enum AvaliableMenu menuObjId);

	int switch2nextMenuObj(void);
	void initMenuIndicator(void);
	bool mSubMenu;
};

typedef enum {
	TIMER_LOWPOWER_IDX,
	TIMER_NOWORK_IDX,
	TIMER_NOWORK_IDX2,
	TIMER_PARK_RECORD_IDX,
	TIMER_RECORD_IDX,
}timer_idx_enum;

typedef enum{
	KEY_SOUNDS = 1,
	TAKEPIC_SOUNDS,
}sounds_type;

class CdrMain : public MainWindow, EventListener,
			  	private ConnectivityManager::ConnectivityListener
{
public:
	CdrMain();
	~CdrMain();
	void initPreview(void* none);
	void initStage2(void* none);
	int msgCallback(int msgType, unsigned int data);
	void exit();
	int createMainWindows(void);
	int createOtherWindows(void);
	void msgLoop();
	HWND getWindowHandle(unsigned int windowID);
	MainWindow* windowID2Window(unsigned int windowID);
	void setCurrentWindowState(windowState_t windowState);
	void setCurrentRPWindowState(RPWindowState_t status);
	RPWindowState_t getCurrentRPWindowState(void);
	void setCurWindowId(unsigned int windowId);
	unsigned int getCurWindowId();
	void changeWindow(unsigned int windowID);
	void changeWindow(unsigned int fromID, unsigned int toID);
	void DispatchKeyEvent(MainWindow *mWin, int keyCode, int isLongPress);
	void keyProc(int keyCode, int isLongPress);
	CdrDisplay *getCommonDisp(int disp_id);
	void configInit();
	int notify(int message, int data);
	void forbidRecord(int val);
	int isEnableRecord();
	void prepareCamera4Playback(bool closeLayer=true);
	bool isScreenOn();
	void setOffTime();
	void setUSBStorageMode(bool enable);
	bool getUSBStorageMode();
	bool getWakeupState();
	void setWakeupState(bool flag);
	void shutdown(int flag = false);
	void standby();
	void noWork(bool idle);
	bool isRecording();
	bool isPlaying();
	bool isBatteryOnline();
	void showShutdownLogo();
	bool isShuttingdown();
	bool mShutdowning;
	void setSTBRecordVTL(unsigned int vtl);
	bool isPhotoWindow();
#ifdef PLATFORM_0
	int getHdl(int disp_id);
#endif
	bool getUvcState();
/* NAT */
	int wifiInit();
	int wifiEnable(int bEnable);
	int wifiSetPwd(const char *old_pwd, const char *new_pwd);
	void handleConnectivity(void *cookie, conn_msg_t msg);
	void sessionChange(bool connect);
	int sessionControl(int cmd, int param_in, int param_out);
	int sessionServerInit();
	int lockFile(const char *file);
	int sessionNotify(int cmd, int param_in);
	int camChange(bool session, int param_in);
	bool isWifiConnecting();
	void otaUpdate();
	bool getFormatTFAdasS();
	void setFormatTFAdasS(bool flag);
	bool getNeedCheckSD();
	void setNeedCheckSD(bool flag);
	void startPCCam();
	void stopPCCam();
	bool getPCCamMode();
	void cdrOtaUpdate();
	bool getPC2Connected();
	void modifyNoworkTimerInterval(time_t  time, time_t  time2);
	void checkShutdown();
	void startUp();
	void startOtherService();
	void playSounds(sounds_type type);
	void stopPreviewOutside(int flag);
	class InitPreviewThread : public Thread
	{
	public:
		InitPreviewThread(CdrMain* pContext) 
			: Thread(false),
			  mWindowContext(pContext) {
		}
		~InitPreviewThread(){}
		virtual bool threadLoop() {
			mWindowContext->initPreview();	
			return false;
		}

		status_t start() {
			return run("InitPreview", PRIORITY_NORMAL);
		}

	private:
		CdrMain* mWindowContext;
	};

	class InitStage2Thread : public Thread
	{
	public:
		InitStage2Thread(CdrMain* pContext) 
			: Thread(false),
			  mWindowContext(pContext) {
		}
		~InitStage2Thread(){}
		virtual bool threadLoop() {
			mWindowContext->initStage2();	
			return false;
		}

		status_t start() {
			return run("InitStage2", PRIORITY_NORMAL);
		}

	private:
		CdrMain* mWindowContext;
	};

		class DataCheckThread : public Thread
	{
	public:
		DataCheckThread(CdrMain *mw): Thread(false),mRM(mw){	}
		status_t start() {
			return run("DataCheckThread", PRIORITY_NORMAL);
		}
		virtual bool threadLoop() {
			mRM->checkDataValid();
			return false;
		}
		void stopThread()
		{
			requestExitAndWait();
		}

	private:
		CdrMain *mRM;
	};

	void checkDataValid();
	void fResetWifiPWD();
	void initCdrTime();
	bool checkSDFs();
	bool getIsLowPower();
	void keepAdasScreenFlag();
	void recoverAdasScreenFlag();
	StatusBar * getStatusBar();
	void TinyPlayAudio(char *path);

	class TinyParkAudioThread: public Thread
	{
		public:
			TinyParkAudioThread(CdrMain *mre,char *fpath)
				: Thread(false), mCdr(mre),filepath(fpath)
			{
			
			}
			~TinyParkAudioThread(){}
			virtual bool threadLoop() {
				mCdr->TinyPlayAudio(filepath);
				return false;
			}
			status_t start() {
				return run("TinyParkAudioThread", PRIORITY_NORMAL);
			}
		private:
			CdrMain* mCdr;
			char * filepath;
	};

	int releaseResource();
	RecordPreview * getRecordPreview();
	void restoreFromStandby();
	int enterParkMode();
	void parkRecordTimer(bool flag);
private:
	void initTimerData();
	void startShutdownTimer(int idx);
	void cancelShutdownTimer(int idx);
	void stopShutdownTimer(int idx);
	int initPreview();
	int initStage2();
	StatusBar *mSTB;
	RecordPreview *mRecordPreview;
	PlayBackPreview* mPlayBackPreview;
	PlayBack *mPlayBack;
	Menu* mMenu;
	sp <InitPreviewThread> mInitPT;
	sp <InitStage2Thread> mInitStage2T;
	PowerManager *mPM;
	EventManager *mEM;
	bool mEnableRecord;
	bool usbStorageConnected;
	bool isWakenUpOnParking;
	unsigned int mTime;
	Condition  mPOR;
	Mutex mLock;
	bool mOtherWindowInited;
	ConnectivityManager *mConnManager;
	CdrServer *mCdrServer;
	bool mSessionConnect;
	int mLowPower;
	bool mOtherDialogOpened;
	bool mOriginAdasScreen,m2pcAdasScreen,mFormatTFAdasS,mNeedCheckSD,mOTAAdasScreen;
	TinyParkAudioThread *mTinyParkAudioTh;
	int mGSensorVal;
	bool mPCCam;
	bool m2pcConnected;
	bool isAutoShutdownEnabled;
	bool mShowLogoInTime;
	sp<DataCheckThread> mDataCheckThread;
	bool mReverseChanged;
public:
	bool mReverseMode;
	WPARAM downKey;
	bool isKeyUp;
	bool isLongPress;
	bool mFinishConfig;
};

class PowerManager
{
public:
	PowerManager(CdrMain *cm);
	~PowerManager();

	void setOffTime(unsigned int time);
	void pulse();
	void powerOff();
	int screenOff();
	int screenOn();

	int readyToOffScreen();
	void screenSwitch();
	int setBatteryLevel();
	int getBatteryLevel(void);
	void reboot();
	bool isScreenOn();
	void adjustCpuFreq();
	int getStandby_status();
	void setStandby_status(bool flag);
	void connect2StandbyService();
	void enterStandby();
	void watchdogRun();
	void watchdogThread();
	void enterStandby(int pid=-1);
	void enterStandbyModem();
	int standbySuspend();
	int standbyResume();
	void standbyOver();
		
	class ScreenSwitchThread : public Thread
	{
	public:
		ScreenSwitchThread(PowerManager *pm)
			: Thread(false)
			, mPM(pm)
		{
		}
		void startThread()
		{
			run("ScreenSwitchThread", PRIORITY_NORMAL);
		}
		void stopThread()
		{
			requestExitAndWait();
		}
		bool threadLoop()
		{
			mPM->readyToOffScreen();
			return true;
		}
	private:
		PowerManager *mPM;
	};

	class poweroffThread : public Thread
	{
	public:
		poweroffThread(PowerManager* pm) : mPM(pm){}
		~poweroffThread(){}
		virtual bool threadLoop() {
			mPM->poweroff();	
			return false;
		}

		status_t start() {
			return run("poweroffThread", PRIORITY_NORMAL);
		}

	private:
		PowerManager* mPM;
	};

	class WatchDogThread : public Thread
	{
	public:
		WatchDogThread(PowerManager *pm)
			: Thread(false)
			, mPM(pm)
		{
		}
		void startThread()
		{
			run("WatchDogThread", PRIORITY_NORMAL);
		}
		void stopThread()
		{
			requestExitAndWait();
		}
		bool threadLoop()
		{
			mPM->watchdogThread();
			return false;
		}
	private:
		PowerManager *mPM;
	};
	bool getStandbyFlag();

private:
	CdrMain *mCdrMain;
	sp<ScreenSwitchThread> mSS;
	sp<poweroffThread> mPO;
	sp<WatchDogThread> mWD;
	int mDispFd;
	unsigned int mOffTime;
	Mutex mLock;
	Condition mCon;
	int mState;
	int mBatteryLevel;
	int poweroff();
	bool isEnterStandby;
	bool isEnterStandby2;
	sp<IStandbyService> mStandbyService;
	bool standbyFlag;
	bool mEnterStandby;
	bool mRestoreScreenOn;
};


#endif //_WINDOW_H
