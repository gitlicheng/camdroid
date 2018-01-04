#include "RecordPreview.h"
#undef LOG_TAG
#define LOG_TAG "RecordPreview"
#include "debug.h"
#include "EventManager.h"
#include "StorageManager.h"
#include "windows.h"
#include <stdlib.h>


static const char *WHITE_BALANCE[] = {
	"auto",
	"daylight",
	"cloudy-daylight",
	"incandescent",
	"fluorescent"
};

static inline bool IF_VideoMode(RPWindowState_t state)
{
	return state == STATUS_RECORDPREVIEW || state == STATUS_AWMD;
}

static inline bool IF_PhotoMode(RPWindowState_t state)
{
	return state == STATUS_PHOTOGRAPH;
}

static inline bool IF_AWMDMode(RPWindowState_t state)
{
	return state == STATUS_AWMD;
}

static inline bool IF_PreviewMode(RPWindowState_t state)
{
	return state == STATUS_RECORDPREVIEW;
}

static inline bool IF_ShortPress(int code)
{
	return code == SHORT_PRESS;
}

int RecordPreview::keyProc(int keyCode, int flag)
{
	switch(keyCode) {

	case CDR_KEY_OK:
		if(IF_PhotoMode(mRPWindowState)) {
			takePicture(1);
			break;
		}
		if(IF_ShortPress(flag)) {
			if(IF_VideoMode(mRPWindowState)) {
				if (isRecording())
					stopRecord();
				else 
					startRecord();
			} else if(IF_PhotoMode(mRPWindowState))
				takePicture(1);
		}
	break;

	case CDR_KEY_MENU:
		if(!getAllowTakePic())
			break;
		if( !isRecording() ) {
			if(!IF_AWMDMode(mRPWindowState)) {
				mOldRPWindowState = mRPWindowState;
			}
			else {
				mOldRPWindowState = STATUS_RECORDPREVIEW;
				break;
			}
			return WINDOWID_MENU;
		}
		break;

	case CDR_KEY_MODE:
		if(IF_ShortPress(flag)) {
			if(IF_PreviewMode(mRPWindowState) && !isRecording()) {
				mRPWindowState = STATUS_PHOTOGRAPH;
				mCdrMain->setCurrentRPWindowState(mRPWindowState);
			} else if(IF_PhotoMode(mRPWindowState)) {
				db_debug("go to PlayBackPreview\n");
				mRPWindowState = STATUS_RECORDPREVIEW;
				mCdrMain->setCurrentRPWindowState(mRPWindowState);
				if(!IF_AWMDMode(mRPWindowState))
					mOldRPWindowState = mRPWindowState;
				else
					mOldRPWindowState = STATUS_RECORDPREVIEW;
				return WINDOWID_PLAYBACKPREVIEW;
			}
		}
		break;
	case CDR_KEY_LEFT:
		if(IF_ShortPress(flag)) {
			if (mCamExist[CAM_CSI] && mCamExist[CAM_UVC]) {
				if(mPIPMode == UVC_ON_CSI)
					transformPIP(CSI_ON_UVC);
				else if(mPIPMode == CSI_ON_UVC)
					transformPIP(UVC_ONLY);
				else if(mPIPMode == UVC_ONLY)
					transformPIP(CSI_ONLY);
				else if(mPIPMode == CSI_ONLY)
					transformPIP(UVC_ON_CSI);
			} else {
				if(FILE_EXSIST(UPDATE_FILE_PATH)){
					mCdrMain->cdrOtaUpdate();
				}
			}
		} else {
			switchAWMD();
		}
		break;
	case CDR_KEY_RIGHT:
		if(IF_ShortPress(flag)) {
			db_msg("*****************mRPWindowState:%d",mRPWindowState);
			if(IF_PreviewMode(mRPWindowState))
				switchVoice();
		} else if(isRecording()) {
			mFilelock->reverse();
		}
		break;
	}
	return WINDOWID_RECORDPREVIEW;
}

void RecordPreview::lockFile(bool flag)
{
	mFilelock->force(flag);
}

void RecordPreview::condLock(bool bLock)
{
	HWND hStatusBar = mCdrMain->getWindowHandle(WINDOWID_STATUSBAR);
	SendMessage(hStatusBar, STBM_LOCK, bLock, 0);
}

void RecordPreview::switchVoice()
{
	bool voiceStatus;
	HWND hStatusBar;
	ResourceManager* rm;
	if (mRecorder[CAM_CSI]) {
		rm = ResourceManager::getInstance();	
		voiceStatus = mRecorder[CAM_CSI]->getVoiceStatus();
		hStatusBar = mCdrMain->getWindowHandle(WINDOWID_STATUSBAR);
		rm->setResBoolValue(ID_MENU_LIST_RECORD_SOUND, voiceStatus);
	}
}

int RecordPreviewProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{

	static bool mShowTime=false;
	char mWaterMark[22];
	memset(mWaterMark,0,sizeof(mWaterMark));
	static int mShowAlignT = 1;

	RecordPreview* recordPreview = (RecordPreview*)GetWindowAdditionalData(hWnd);
	switch(message) {
	case MSG_CREATE:
		break;
	case MSG_CDR_KEY:
		db_debug("key code is %d\n", wParam);
		return recordPreview->keyProc(wParam, lParam);
	case MSG_DRAW_ALIGN_LINE:
		recordPreview->setAlignLine((bool)wParam);
		break;
	case MSG_ADAS_SCREEN:
		recordPreview->adasScreenSwitch((bool)wParam);
		break;
	case MWM_CHANGE_FROM_WINDOW:
		recordPreview->restorePreviewOutSide(wParam);
		usleep(200 * 1000);		/* 让 layer开始渲染后才允许绘制窗口 */
		{
			ResourceManager *rm = ResourceManager::getInstance();
			if (rm->getResBoolValue(ID_MENU_LIST_SMARTALGORITHM)) {
				recordPreview->adasScreenSwitch((bool)wParam);
			}
			db_msg("xxxxxxxx\n");
		}
		recordPreview->showPicFullScreen(PIC_REVERSE, true);
		break;
	case MSG_SHOWWINDOW:
		if(wParam == SW_SHOWNORMAL) {
		} else {
			recordPreview->stopPreviewOutside(CMD_NO_NEED_STOP_PREVIEW);
			recordPreview->showPicFullScreen(PIC_REVERSE, false);
		}
		break;
	case MSG_SHOW_TIP_LABEL:
		if (recordPreview->allowShowTip()) {
			ShowTipLabel(hWnd, (enum labelIndex)wParam, TRUE, 3000);
		}
		break;
	default:
		recordPreview->msgCallback(message, (int)wParam);
	}

	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

void RecordPreview::takePicFinish(int cam_id)
{
	Mutex::Autolock _l(mLockPic);
	mAllowTakePic[cam_id] = true;
}

void RecordPreview::errorOccur(int error)
{
	db_msg(" ");
	if (error == CAMERA_ERROR_SELECT_TIMEOUT) {
		db_msg("select time out");
		suspend();
		resume();
		mCdrMain->startUp();
	}
}

void RecordPreview::getAdasCallback()
{
	if(mCdrMain->getCurWindowId() == WINDOWID_RECORDPREVIEW){
		adasScreenSwitch(true);
	}
}
#ifdef APP_ADAS

static void adasCallback(void *data,void *caller)
{
	RecordPreview *rp;
	if(caller == NULL){
		db_error("caller not initialized\n");
		return;
	}
	rp = (RecordPreview *)caller;
	rp->getAdasCallback();
}

#endif
static void jpegCallback(void *data, int size, void* caller, int id)

{
	RecordPreview* rp;
	db_debug("picture call back %d data %p", id, data);
	if(caller == NULL) {
		db_error("caller not initialized\n");
		return;
	}
	rp = (RecordPreview*)caller;
	rp->savePicture(data, size, id);
	rp->takePicFinish(id);
}

static void errorCallback(int error, void *caller)
{
	RecordPreview* rp;
	db_debug("errorCallback error %d", error);
	if(caller == NULL) {
		db_error("caller not initialized\n");
		return;
	}
	rp = (RecordPreview*)caller;
	rp->errorOccur(error);
}

static void awmdTimerCallback(sigval_t val)
{
	db_msg("posix timer data: %p\n", val.sival_ptr);
	RecordPreview *rp = (RecordPreview *)val.sival_ptr;
	rp->stopRecord();
}

static void awmdCallback(int value, void *pCaller)
{
	RecordPreview *rp = (RecordPreview *)pCaller;
	CdrRecordParam_t param;
	StorageManager *sm = StorageManager::getInstance();
	bool mounted = sm->isMount();
//	db_msg("awmd : value- %d\n", value);
	if(!IsWindowVisible(rp->getHwnd()) || !mounted) {
		return;
	}

	if(rp->needDelayStartAWMD() == true) {
//		db_msg("need to delay awmd\n");
		rp->mAwmdDelayCount++;
		/* skip 100 times , after stop record */
		if(rp->mAwmdDelayCount >= 100) {
			db_msg("clear awmd delay flag\n");
			rp->mAwmdDelayCount = 0;
			rp->setDelayStartAWMD(false);
		}
		return;
	}

	if(value == 1) {
		if(rp->isRecording(CAM_CSI) == false) {
			//param.fileSize = AWMD_PREALLOC_SIZE(rp->getDuration());
			param.video_type = VIDEO_TYPE_NORMAL;
			param.duration = rp->getDuration();
			rp->startRecord();
			rp->lockFile(true);
			rp->feedAwmdTimer(5);
		} else if(rp->isRecording(CAM_CSI) == true) {
			time_t sec;
			long nsec;
			if(rp->queryAwmdExpirationTime(&sec, &nsec) < 0) {
				db_error("query awmd timer expiration time failed\n");
				return;
			}
			if( (sec == 4 && nsec <= 500 * 1000 * 1000) || sec < 4)
				rp->feedAwmdTimer(5);
		}
	}
}

int RecordPreview::initCamera(int cam_type, bool needNewCamera)
{
	CDR_RECT rect;
	unsigned int width = 1280, height = 720;	//real size
	unsigned int preview_w  = 640, preview_h = 360; //preview size
	int node = 0;
	int ret = -1;
	if(mCamExist[cam_type] == false)
		return ret;
	if (cam_type == CAM_CSI) {
		rect = rectBig;
		node = CAM_CSI;
		width  = REAL_CSI_W;
		height = REAL_CSI_H;
		preview_w = PREVIEW_W_csi;
		preview_h = PREVIEW_H_csi;
	}
	if (cam_type == CAM_UVC) {
		rect = rectSmall;
		mUVCNode = CAM_UVC;
		node = mUVCNode;
		width  = REAL_UVC_W;
		height = REAL_UVC_H;
		if(mHasTVD) {
			node = CAM_TVD;
			width = 720;
			height = 480;
		}
		preview_w = PREVIEW_W_uvc;
		preview_h = PREVIEW_H_uvc;
	}
	if (needNewCamera) {
		mCamera[cam_type] = new CdrCamera(cam_type, rect, node);
	}
	ret = mCamera[cam_type]->initCamera(width, height, preview_w, preview_h);
	if (ret < 0) {
		delete mCamera[cam_type];
		mCamera[cam_type] = NULL;
		mCamExist[cam_type] = false;
		return ret;
	} else {
		mCamExist[cam_type] = true;
	}
	mCamera[cam_type]->setCaller(this);
#ifdef APP_ADAS
	mCamera[cam_type]->setAdasCaller(this);
#endif
	if (cam_type == CAM_CSI) {
		mCamera[cam_type]->setJpegCallback(jpegCallback);
		mCamera[cam_type]->setErrorCallback(errorCallback);
		mCamera[cam_type]->setAwmdCallBack(awmdCallback);
		mCamera[cam_type]->setAwmdTimerCallBack(awmdTimerCallback);
		#ifdef APP_ADAS
		mCamera[cam_type]->setAdasCallback(adasCallback);
		#endif
	}else if(cam_type == CAM_UVC){
		mCamera[cam_type]->setJpegCallback(jpegCallback);
		mCamera[cam_type]->setErrorCallback(errorCallback);
	}
	return 0;
}

void RecordPreview::startPreview(int cam_type)
{
    ResourceManager* rm;
    rm = ResourceManager::getInstance();
	int ret = 0;
	if(mCamera[cam_type]) {
		ret = mCamera[cam_type]->startPreview();
		if (ret < 0) {
			return;
		}
		if(cam_type == CAM_UVC){
			int idx = rm->getResIntValue(ID_MENU_LIST_VIDEO_RESOLUTION, INTVAL_SUBMENU_INDEX);
			if (mCamera[CAM_UVC]) {
				mCamera[CAM_UVC]->setWaterMarkPos(VideoSize_HD_H_uvc);
			}
		}
		mCamera[cam_type]->setLWaterMark(mTWM, mLWM, strLWM);
		if(cam_type == CAM_UVC){
            bool isFlip = rm->getResBoolValue(ID_MENU_LIST_ALIGNLINE);
            mCamera[CAM_UVC]->setCameraFlip(isFlip);
        }
	}
}

void RecordPreview::startPreview()
{
	Mutex::Autolock _l(mLock);
	if (mCamExist[CAM_CSI]) {
		startPreview(CAM_CSI);
	}
	if (mCamExist[CAM_UVC]) {
		startPreview(CAM_UVC);
	}
}

void RecordPreview::setDeleteFlag(int cam_type, String8 file)
{
	Mutex::Autolock _l(mDeleteLock);
	db_msg("setDeleteFlag %s!", file.string());
	mNeedDelete[cam_type] = file;
}

void RecordPreview::clrDeleteFlag(int cam_type, String8 file)
{
	Mutex::Autolock _l(mDeleteLock);
	if (file == mNeedDelete[cam_type]) {
		db_msg("clrDeleteFlag!");
		mNeedDelete[cam_type] = "";
	} else {
		db_msg("should not be cleared %s", file.string());
	}
}

String8 RecordPreview::getDeleteFlag(int cam_type)
{
	Mutex::Autolock _l(mDeleteLock);
	return mNeedDelete[cam_type];
}


void RecordPreview::setCyclicVTL_Flag(bool flag)
{
	Mutex::Autolock _l(mCyclicLock);
	mNeedCyclicVTL = flag;
}


bool  RecordPreview::getCyclicVTL_Flag(void)
{
	Mutex::Autolock _l(mCyclicLock);
	return mNeedCyclicVTL;
}

bool RecordPreview::isPreviewing(int cam_type)
{
	if(mCamera[cam_type])
		return mCamera[cam_type]->isPreviewing();

	return false;
}
bool RecordPreview::isPreviewing()
{
	if(isPreviewing(CAM_CSI) == true && isPreviewing(CAM_UVC))
		return true;
	return false;
}

void RecordPreview::errorListener(CdrMediaRecorder* cmr, int what, int extra)
{
	db_msg("_______onError");
	//	stopRecord();
}

void RecordPreview::recordListener(CdrMediaRecorder* cmr, int what, int extra)
{
	int cameraID;
    int ret;
	CdrRecordParam_t recordParam;
	Elem elem;
	StorageManager* sm = StorageManager::getInstance();
	cameraID = cmr->getCameraID();
	cmr->getRecordParam(recordParam);
	db_msg("cameraID is %d, extra is %d\n", cameraID, extra);
	String8 needDeleteFile;
	switch(what) {
	case MEDIA_RECORDER_INFO_NEED_SET_NEXT_FD: 
		{
			if(mCdrMain->getWakeupState()) {
				break;
			}
			needDeleteFile = getDeleteFlag(cameraID);
#ifdef FATFS
			if (!needDeleteFile.isEmpty()) {
				if(sm->fileSize(needDeleteFile) == 0) {
					db_error("MEDIA_RECORDER_INFO_NEED_SET_NEXT_FD %s is empty, will be deleted\n",needDeleteFile.string());
					sm->deleteFile(needDeleteFile.string());
					clrDeleteFlag(cameraID, needDeleteFile );
				}
			}
#endif
			db_msg("Please set the next fd.");
			ret = sm->generateFile(mBakNow[cameraID], mBakFileString[cameraID], cameraID, recordParam.video_type, getCyclicVTL_Flag(),true);
			if(ret < 0) {
				db_error("get fd failed\n");
				clrDeleteFlag(cameraID, mRecordDBInfo[cameraID].fileString);
				stopRecord();
				if ( ret == RET_DISK_FULL || ret == RET_IO_NO_RECORD) {
					SendNotifyMessage(mCdrMain->getWindowHandle(WINDOWID_RECORDPREVIEW), MSG_SHOW_TIP_LABEL, LABEL_TFCARD_FULL, 0);
				}
			}
			setDeleteFlag(cameraID, mBakFileString[cameraID]);
			cmr->setNextFile(mBakFileString[cameraID], recordParam.fileSize, extra);
		}
		break; 
	case MEDIA_RECORDER_INFO_RECORD_FILE_DONE: 
		{
			String8 newName("");
			db_msg("one file done %s", mRecordDBInfo[cameraID].fileString.string());
			if (mRecordDBInfo[cameraID].fileString.isEmpty()) {
				db_error("filename is empty. it's an accident ");
				break;
			}
			if(sm->fileSize(mRecordDBInfo[cameraID].fileString) < (256 * 1024)) { //file is too small
				sm->deleteFile(mRecordDBInfo[cameraID].fileString.string());
				db_msg("file %s size is 0kb", mRecordDBInfo[cameraID].fileString.string());
				break;
			} else {
				elem.file = (char*)mRecordDBInfo[cameraID].fileString.string();
				if (mFilelock->getStatus()) {
					elem.info = (char*)INFO_LOCK;
					newName   = sm->lockFile(elem.file);
					elem.file = (char*)newName.string();
					if ((mRecorder[CAM_UVC] && cameraID == CAM_UVC) || (!mRecorder[CAM_UVC] && cameraID == CAM_CSI)) {
						if(mImpactLockFlag == false) {
							mFilelock->force(false);
						}
						mImpactLockFlag = false;
					}
				} else {
					elem.info = (char*)mRecordDBInfo[cameraID].infoString.string();
				}
				elem.type = (char*)TYPE_VIDEO;
				elem.time = mRecordDBInfo[cameraID].time;
				sm->dbAddFile(&elem);
				if (mFilelock->getStatus())
					mCdrMain->lockFile(newName.string());
			}
			mOrignFileName[cameraID] = elem.file;
			clrDeleteFlag(cameraID, mRecordDBInfo[cameraID].fileString);
			mRecordDBInfo[cameraID].fileString = mBakFileString[cameraID];
			mRecordDBInfo[cameraID].time = mBakNow[cameraID];
			mRecordDBInfo[cameraID].infoString = INFO_NORMAL;
		}
		break;
	}
}

int RecordPreview::createWindow(void)
{
	RECT rect;
	CDR_RECT STBRect;
	ResourceManager* rm;
	HWND hParent;
	HWND hStatusBar;

	rm = ResourceManager::getInstance();
	rm->getResRect(ID_STATUSBAR, STBRect);
	hParent = mCdrMain->getHwnd();
	GetWindowRect(hParent, &rect);

	rect.top += STBRect.h;

	mHwnd = CreateWindowEx(WINDOW_RECORDPREVIEW, "",
			WS_CHILD | WS_VISIBLE,
			WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT,
			WINDOWID_RECORDPREVIEW,
			rect.left, rect.top, RECTW(rect), RECTH(rect),
			hParent, (DWORD)this);
	if(mHwnd == HWND_INVALID) {
		db_error("create record Preview window failed\n");
		return -1;
	}
	rm->setHwnd(WINDOWID_RECORDPREVIEW, mHwnd);

	CreateWindowEx(CTRL_STATIC, "",
			WS_CHILD | SS_BITMAP | SS_CENTERIMAGE,
			WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT,
			ID_RECORDPREVIEW_IMAGE,
			0, 0, RECTW(rect), RECTH(rect),
			mHwnd, 0);

	hStatusBar = mCdrMain->getWindowHandle(WINDOWID_STATUSBAR);
	if(mCamExist[CAM_UVC] == true)
		SendNotifyMessage(hStatusBar, STBM_UVC, 1, 0);
	else
		SendNotifyMessage(hStatusBar, STBM_UVC, 0, 0);

	return 0;
}

bool checkDev(int cam_type, bool &hasTVD, int &uvcnode)
{

	char dev[32];
	int i = 0;
	struct v4l2_capability cap; 
	int ret;
	for (i = 0; i < 8; ++i) {
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
			if (strcmp((char*)cap.driver, "sunxi-vfe") == 0 && (cam_type==CAM_CSI)) {
				close(fd);
				return true;
			} else if (strcmp((char*)cap.driver, "uvcvideo") == 0 && (cam_type==CAM_UVC)) {
				db_debug("uvcvideo device node is %s", dev);
				hasTVD = false;	//Tvd could not exist at all.
				close(fd);
				uvcnode = i;
				return true;
			} else if (strcmp((char*)cap.driver, "tvd") == 0 && (cam_type==CAM_UVC) ) {
				db_debug("TVD device node is %s", dev);
				hasTVD = true;
				if (detectPlugIn(fd) == 0) {
					db_msg("tvd:plug in");
					close(fd);
					return true;
				}
			}
			close(fd);
			fd = -1;
		}
	}
	db_error("Could not find device for device ID %d!", cam_type);
	return false;
}

RecordPreview::RecordPreview(CdrMain *cdrMain)
	: mCdrMain(cdrMain),
	mRPWindowState(STATUS_RECORDPREVIEW),
	mOldRPWindowState(STATUS_RECORDPREVIEW), 
	mUVCNode(1),
	mAwmdDelayStart(false),
	mStartRtspSession(false),
	mNeedCache(true),
	mImpactLockFlag(false),
	mNeedStartPreview(false),
	mLWM(0),
	mTWM(0)
{
	mVideoSize.width  = 1920;
	mVideoSize.height = 1080;
	mBitRate = (14*1024*1024);
	mRecorder[CAM_CSI] = NULL;
	mRecorder[CAM_UVC] = NULL;
	mRecorder[CAM_CSI_NET] = NULL;
	mRecorder[CAM_UVC_NET] = NULL;
	mCamera[CAM_CSI] = NULL;
	mCamera[CAM_UVC] = NULL;
	mDuration = 60*1000;
	int big_w, big_h;
	getScreenInfo(&big_w, &big_h);
	rectBig.x = BIG_RECT_X;
	rectBig.y = BIG_RECT_Y;
	rectBig.w = big_w;
	rectBig.h = big_h;
	rectSmall.w = rectBig.w*2/5;
	rectSmall.h = rectSmall.w*rectBig.h/rectBig.w;
	rectSmall.x = rectBig.w - rectSmall.w;
	rectSmall.y = 30;

	mHasTVD = false;
	mCamExist[CAM_CSI] = checkDev(CAM_CSI, mHasTVD, mUVCNode);
	mCamExist[CAM_UVC] = checkDev(CAM_UVC, mHasTVD, mUVCNode);
	mNeedDelete[CAM_CSI]="";
	mNeedDelete[CAM_UVC]="";
	mNeedCyclicVTL =true;
	//mSilent = true;
	mLockFlag[CAM_UVC]= false;
	mLockFlag[CAM_CSI]= false;
	if(mCamExist[CAM_CSI] == false && mCamExist[CAM_UVC] == false)
		mPIPMode = NO_PREVIEW;
	else if(mCamExist[CAM_CSI] == true && mCamExist[CAM_UVC] == true)
		mPIPMode = UVC_ON_CSI;
	else if(mCamExist[CAM_CSI] == true)
		mPIPMode = CSI_ONLY;
	else
		mPIPMode = UVC_ONLY;
	mSessionSize.width   = mVideoSize.width / 4;
	mSessionSize.height  = mVideoSize.height / 4;
	mAllowTakePic[CAM_CSI] = true;
	mAllowTakePic[CAM_UVC] = true;
	db_msg("mPIPMode is %d\n", mPIPMode);
#ifdef PLATFORM_0
	mVirHC = NULL;
	createVirtualCam();
#endif
	mSession = NULL;
	mCurSessionCam = LOCAL2SESSION(CAM_CSI);
	mAlignLine=false;
	mIsAdasScreen=false;
	mVtlThread = new VTLThread(this);
	memset(strLWM, 0, sizeof(strLWM));
	mOrignFileName[CAM_CSI] = "";
	mOrignFileName[CAM_UVC] = "";
	mFilelock = new FileLock(this);
	memset(&mReverseBmp, 0, sizeof(BITMAP));
}

RecordPreview::~RecordPreview()
{
	for(int i=0; i<CAM_CNT+2;  i++)
	{
		if (mRecorder[i]) {
			stopRecord();
			delete mRecorder[i];
			mRecorder[i] = NULL;
		}
	}
	for(int i=0; i<CAM_CNT;  i++)
	{
		if (mCamera[i]) {
			delete mCamera[i];
			mCamera[i] = NULL;
		}
	}
	if(mVtlThread != NULL){
		mVtlThread->stopThread();
		mVtlThread.clear();
		mVtlThread = 0;
	}
	if (mFilelock) {
		delete mFilelock;
		mFilelock = NULL;
	}
}

bool RecordPreview::hasTVD(bool &hasTVD)
{
	hasTVD = mHasTVD;
	db_msg("hasTVD:%d\n", hasTVD);
	return (mHasTVD && mCamExist[CAM_UVC]);
}

void RecordPreview::init(void)
{	
	CdrDisplay* cd;	
	if(mCamExist[CAM_CSI] == true) {
		initCamera(CAM_CSI);
		startPreview(CAM_CSI);
		mRecorder[CAM_CSI] = new CdrMediaRecorder(CAM_CSI);
		mRecorder[CAM_CSI]->setOnInfoCallback(this);
		mRecorder[CAM_CSI]->setOnErrorCallback(this);
	}
	struct timespec measureTime1;
	clock_gettime(CLOCK_MONOTONIC, &measureTime1);
	db_msg("Test Time: start uvc , time is %ld secs, %ld nsecs\n", measureTime1.tv_sec, measureTime1.tv_nsec);
	if(mCamExist[CAM_UVC] == true) {
		initCamera(CAM_UVC);
		mRecorder[CAM_UVC] = new CdrMediaRecorder(CAM_UVC);
		mRecorder[CAM_UVC]->setOnInfoCallback(this);
		mRecorder[CAM_UVC]->setOnErrorCallback(this);
		if(mCamExist[CAM_CSI] && mCamera[CAM_CSI]) {
			cd = mCamera[CAM_CSI]->getDisp();
			cd->setBottom();
		}
	}
}

static void display_setareapercent(CDR_RECT *rect, int percent)
{
	int valid_width  = (percent*rect->w)/100;
	int valid_height = (percent*rect->h)/100;
	
	rect->x = (rect->w - valid_width) / 2;
	rect->w = valid_width;
	rect->y = (rect->h - valid_height) / 2;
	rect->h = valid_height;
}

//rect align with parent_rect on the top-right
static void display_setareapercent(CDR_RECT *rect, const CDR_RECT *parent_rect, int percent)
{
	int valid_width  = (percent*rect->w)/100;
	int valid_height = (percent*rect->h)/100;
	
	rect->x = (parent_rect->x + parent_rect->w) - valid_width;
	rect->w = valid_width;
	rect->y = parent_rect->y;
	rect->h = valid_height;
}

void RecordPreview::otherScreen(int screen)
{
	CdrDisplay *cdCSI, *cdUVC;
	int hlay_uvc = 0;
	cdCSI = getCommonDisp(CAM_CSI);
	cdUVC = getCommonDisp(CAM_UVC);
	if (cdUVC) {
		hlay_uvc = cdUVC->getHandle();
	}
	if (cdCSI) {
		cdCSI->otherScreen(screen, hlay_uvc);
	}
	if (screen == 1) {
		rectBig.x = 0;
		rectBig.y = 0;
		rectBig.w = 1280;
		rectBig.h = 720;
		display_setareapercent(&rectBig, 95);
		rectSmall.x = 920;
		rectSmall.y = 0;
		rectSmall.w = 360;
		rectSmall.h = 240;
		display_setareapercent(&rectSmall, &rectBig, 95);
	} else if (screen == 2) {
		rectBig.x = 0;
		rectBig.y = 0;
		rectBig.w = 720;
		rectBig.h = 480;
		display_setareapercent(&rectBig, 95);
		rectSmall.x = 480;
		rectSmall.y = 0;
		rectSmall.w = 240;
		rectSmall.h = 160;
		display_setareapercent(&rectSmall, &rectBig, 95);
	} else {
		rectBig.x = 0;
		rectBig.y = 0;
		rectBig.w = 480;
		rectBig.h = 272;
		rectSmall.x = 300;
		rectSmall.y = 22;
		rectSmall.w = 180;
		rectSmall.h = 102;
	}
	restoreZOrder();
}

void RecordPreview::storagePlug(bool plugin)
{
	if (plugin) { //tfcar plug in
		setSdcardState(true);
	} else {	//tfcard plug out
		setSdcardState(false);
		stopRecord();
/*
		if(mCamera[CAM_UVC]) {
			stopPreview(CAM_UVC);
			delete mCamera[CAM_UVC];
			mCamera[CAM_UVC] = NULL;

			initCamera(CAM_UVC);
			startPreview(CAM_UVC);
		}
*/
	}
}

void RecordPreview::restoreZOrder()
{
	
	CdrDisplay *cdCSI = getCommonDisp(CAM_CSI);
	CdrDisplay *cdUVC = getCommonDisp(CAM_UVC);
	switch(mPIPMode) {
		case NO_PREVIEW:
		break;
		case CSI_ONLY:
			
			if (cdCSI) {
				cdCSI->setRect(rectBig);
				cdCSI->setBottom();
			}
			if (cdUVC) {
				//db_msg("UVC layer should be close!!!");
				cdUVC->setBottom();
			}
		break;
		case UVC_ONLY:
			
			if (cdUVC) {
				cdUVC->setRect(rectBig);
				cdUVC->setBottom();
			}
			if (cdCSI) {
				//db_msg("CSI layer should be close!!!");
				cdCSI->setBottom();
			}
		break;
		case CSI_ON_UVC:
			if (cdCSI) {
				cdCSI->setRect(rectSmall);
			}
			cdCSI->setBottom();
			if (cdUVC) {
				cdUVC->setRect(rectBig);
			}
			cdUVC->setBottom();
		break;
		case UVC_ON_CSI:
			if (cdUVC) {
				cdUVC->setRect(rectSmall);
			}
			cdUVC->setBottom();
			if (cdCSI) {
				cdCSI->setRect(rectBig);
			}
			cdCSI->setBottom();
		break;
	};
}

int RecordPreview::transformPIP(pipMode_t newMode)
{
	CdrDisplay *cdCSI, *cdUVC;
	int ret = 0;
	db_msg("mPIPMode is %d, newMode is %d\n", mPIPMode, newMode);
	switch(mPIPMode) {
	case NO_PREVIEW:	
		{
			if(newMode == UVC_ONLY) {
				if(mCamExist[CAM_UVC] == false)
					break;
				if(!mCamera[CAM_UVC])	{
					initCamera(CAM_UVC);
				}
				cdUVC = getCommonDisp(CAM_UVC);
				cdUVC->setRect(rectBig);
				startPreview(CAM_UVC);
				mPIPMode = UVC_ONLY;
				db_msg("-----------PIP_MODE: NO_PREVIEW ---> UVC_ONLY\n");
			}
		}
		break;
	case CSI_ONLY:
		{
			if(newMode == UVC_ON_CSI) {
				if(mCamExist[CAM_UVC] == false)
					break;
				if(!mCamera[CAM_UVC]){
					ret = initCamera(CAM_UVC);
					if (ret < 0) {
						return ret;
					}
					cdCSI = getCommonDisp(CAM_CSI);
					cdCSI->setBottom();
					startPreview(CAM_UVC);
					mPIPMode = UVC_ON_CSI;
					db_msg("-----------PIP_MODE: CSI_ONLY ---> UVC_ON_CSI\n");
				} else {
					cdUVC = getCommonDisp(CAM_UVC);
					cdUVC->setRect(rectSmall);
					cdUVC->open();

					cdCSI = getCommonDisp(CAM_CSI);
					cdCSI->setBottom();
					startPreview(CAM_UVC);
					mPIPMode = UVC_ON_CSI;
					db_msg("-----------PIP_MODE: CSI_ONLY ---> UVC_ON_CSI\n");
				}
			} else if(newMode == CSI_ONLY) {
				db_msg("-----------PIP_MODE: CSI_ONLY ---> CSI_ONLY\n");
			}
		}
		break;
	case UVC_ONLY:
		{
			if(newMode == CSI_ONLY) {
				if(mCamExist[CAM_CSI] == false)
					break;
				if(!mCamera[CAM_CSI]) {
					initCamera(CAM_CSI);
					startPreview(CAM_CSI);

					cdUVC = getCommonDisp(CAM_UVC);
					cdUVC->close();
					cdUVC->setRect(rectSmall);
					mPIPMode = CSI_ONLY;
					db_msg("-----------PIP_MODE: UVC_ONLY ---> CSI_ONLY\n");
				} else {
					cdUVC = getCommonDisp(CAM_UVC);
					cdCSI = getCommonDisp(CAM_CSI);
					cdCSI->setRect(rectSmall);
					cdUVC->setBottom();

					cdUVC->exchange(cdCSI->getHandle(), 1);
					cdCSI->open();
					cdUVC->close();
					startPreview(CAM_CSI);
					mPIPMode = CSI_ONLY;
					db_msg("-----------PIP_MODE: UVC_ONLY ---> CSI_ONLY\n");
				}
			} else if(newMode == NO_PREVIEW) {
				cdUVC = getCommonDisp(CAM_UVC);
				cdUVC->close();
				mPIPMode = NO_PREVIEW;
				db_msg("-----------PIP_MODE: UVC_ONLY ---> NO_PREVIEW\n");
			} else if (newMode == UVC_ON_CSI) {
				db_msg("-----------PIP_MODE: UVC_ONLY ---> UVC_ON_CSI\n");
				cdUVC = getCommonDisp(CAM_UVC);
				cdCSI = getCommonDisp(CAM_CSI);
				cdUVC->exchange(cdCSI->getHandle(), 1);
				cdCSI->open();
				mPIPMode = UVC_ON_CSI;
			}
		}
		break;
	case CSI_ON_UVC:
		if(newMode == UVC_ONLY) {
			cdCSI = getCommonDisp(CAM_CSI);
			cdCSI->setRect(rectSmall);
			cdCSI->close();

			mPIPMode = UVC_ONLY;
			db_msg("-----------PIP_MODE: CSI_ON_UVC ---> UVC_ONLY\n");
		} else if(newMode == CSI_ONLY) {
			cdUVC = getCommonDisp(CAM_UVC);
			cdCSI = getCommonDisp(CAM_CSI);
			cdCSI->setRect(rectSmall);
			cdUVC->setRect(rectBig);
			cdUVC->setBottom();

			cdCSI->exchange(cdUVC->getHandle(), 1);
			cdUVC->close();
			mPIPMode = CSI_ONLY;
			db_msg("-----------PIP_MODE: CSI_ON_UVC ---> CSI_ONLY\n");
		}
		break;
	case UVC_ON_CSI:
		cdUVC = getCommonDisp(CAM_UVC);	
		cdCSI = getCommonDisp(CAM_CSI);
		cdUVC->setRect(rectSmall);
		cdCSI->setRect(rectBig);
		cdCSI->setBottom();
		if(newMode == CSI_ONLY) {
			cdUVC->close();
			mPIPMode = CSI_ONLY;
			db_msg("-----------PIP_MODE: UVC_ON_CSI ---> CSI_ONLY\n");
		} else if(newMode == CSI_ON_UVC) {
			cdCSI->exchange(cdUVC->getHandle(), 1);
			mPIPMode = CSI_ON_UVC;
			db_msg("-----------PIP_MODE: UVC_ON_CSI ---> CSI_ON_UVC\n");
		} else if(newMode == UVC_ONLY) {
			db_msg("-----------PIP_MODE: UVC_ON_CSI ---> UVC_ONLY\n");
			cdCSI->exchange(cdUVC->getHandle(), 1);
			cdCSI->close();
			mPIPMode = UVC_ONLY;
		}
		break;
	}
	return ret;
}

int RecordPreview::setCamExist(int cam_type, bool bExist)
{
	db_msg("-----------cam_type: %d %d\n", cam_type, bExist);
	unsigned int curWindowID;
	int hasTVD = cam_type;
	int ret = 0;

	if (cam_type == CAM_TVD) {
		mHasTVD = bExist;
		cam_type = CAM_UVC;
	} else if (cam_type == CAM_UVC) {
		mHasTVD = !bExist;
	}

	if (mCamExist[cam_type] == bExist) {
		return -1;
	}

	Mutex::Autolock _l(mLock);
	mCamExist[cam_type] = bExist;
	curWindowID = mCdrMain->getCurWindowId();
	if(curWindowID != WINDOWID_PLAYBACKPREVIEW && curWindowID != WINDOWID_PLAYBACK) {
		/* RecordPreview and Menu */
		if(bExist == true) {
			/* plug in */	
			if(mPIPMode == NO_PREVIEW)
				transformPIP(UVC_ONLY);
			else if(mPIPMode == CSI_ONLY) {
				usleep(5 * 1000);
				ret = transformPIP(UVC_ON_CSI);
				if (ret < 0) {
					return ret;
				}
			} 
		} else {
			/* plug out */
			stopRecord(CAM_UVC_NET);
			stopRecord(CAM_UVC);	
			if(mCamExist[CAM_CSI])
				transformPIP(CSI_ONLY);	
			else
				transformPIP(NO_PREVIEW);
			if (cam_type == CAM_UVC) {
				stopPreview(CAM_UVC);
			}
			if(mCamera[CAM_UVC]) {
				if (hasTVD != CAM_TVD) {
					delete mCamera[CAM_UVC];
					mCamera[CAM_UVC] = NULL;
				}
			}
		}
	} else {
		/* PlayBackPreview and PlayBack */
		if(bExist == false) {
			if(mCamera[CAM_UVC]) {
				stopPreview(CAM_UVC);
				if (hasTVD != CAM_TVD) {
					delete mCamera[CAM_UVC];
					mCamera[CAM_UVC] = NULL;
				}
			}
		}
	}
	mLock.unlock();
	if (cam_type == CAM_UVC && bExist == false && mCurSessionCam == CAM_UVC_NET) {
		camChange(CAM_CSI, false);
		return 1;
	}
	return 0;
}
#ifdef PLATFORM_0
void RecordPreview::createVirtualCam()
{
	if (!mVirHC) {
		mVirHC = new VirtualCamera();
		int w = mSessionSize.width;
		int h = mSessionSize.height;
		ALOGD("(f:%s, l:%d) wxh[%dx%d]", __FUNCTION__, __LINE__, w, h);
		mVirHC->initCamera(w, h, w, h);
	}
}

void RecordPreview::destroyVirtualCam()
{
	if (mVirHC) {
		delete mVirHC;
		mVirHC = NULL;
	}
}

void RecordPreview::setVirtualCam(CdrMediaRecorder *cmr)
{
	ALOGE("%s %d", __FUNCTION__, __LINE__);
	if (cmr && mVirHC) {
		cmr->setRecorderOutputThumb(mVirHC->getCamera());
	}
}
#endif

int RecordPreview::prepareRecord(int recorder_idx, CdrRecordParam_t param, Size *resolution)
{
	int ret = -1;
	int retval;
	HerbCamera *mHC, *virHC=NULL;
	Size size(1280, 720);
	int framerate = BACK_CAM_FRAMERATE;
	int fileSize = 0;
	bool hasSession = false;
	int cam_idx = recorder_idx;
	
	if (recorder_idx >= CAM_CNT) {
		cam_idx  = recorder_idx - CAM_CNT;
		hasSession = true;
	}
	if (!hasSession)
     {
		time_t now;
		String8 file;
		StorageManager* sm = StorageManager::getInstance();
		ret = sm->generateFile(now, file, cam_idx, param.video_type, getCyclicVTL_Flag());
		if (ret < 0) {
			db_error("~~fail to generate file, retval is %d\n", ret);
			if (ret == RET_DISK_FULL)
				ShowTipLabel(mHwnd, LABEL_TFCARD_FULL, true, 3000);
			return ret;
		}
		setDeleteFlag(cam_idx, file);

		mRecordDBInfo[cam_idx].fileString = file;
		mRecordDBInfo[cam_idx].time = now;
		mRecordDBInfo[cam_idx].infoString = INFO_NORMAL;
		mRecorder[recorder_idx]->setRecordParam(param);
		if (!getCyclicVTL_Flag()){
			int time;
			time = CalculateRecordTime(cam_idx)*1000;
			if (param.video_type == VIDEO_TYPE_NORMAL){
				mDuration = time;
				param.duration = time;
			}
		}
		mRecorder[recorder_idx]->setDuration(param.duration);
		fileSize = param.fileSize;
		
	}
	Mutex::Autolock _l(mLock);
	mHC = mCamera[cam_idx]->getCamera();
	if (cam_idx == CAM_CSI) {
		size = mVideoSize;
		framerate = FRONT_CAM_FRAMERATE;
	} else if (cam_idx == CAM_UVC) {
		size.width  = VideoSize_FHD_W_uvc;
		size.height = VideoSize_FHD_H_uvc;
		if (mHasTVD) {
			size.width = 720;
			size.height = 480;
		}
	}
	if (resolution != NULL) {
		mSessionSize.width  = resolution->width;
		mSessionSize.height = resolution->height;
	}
	if (hasSession) {
		size.width  = mSessionSize.width;
		size.height = mSessionSize.height;
#ifdef PLATFORM_0
		if(mRecorder[cam_idx]->getRecorderState() == RECORDER_RECORDING)
        {
            ALOGD("(f:%s, l:%d) isRecording is true!", __FUNCTION__, __LINE__);
            mHC = mVirHC->getCamera();
        }
#endif
        ALOGD("(f:%s, l:%d) record video size[%dx%d]!", __FUNCTION__, __LINE__, size.width, size.height);
	}
	if (mNeedCache) {
		mRecorder[recorder_idx]->setCacheTime();
	} else {
		mRecorder[recorder_idx]->setCacheTime(0);
	}
	retval = mRecorder[recorder_idx]->initRecorder(mHC, size, framerate, mRecordDBInfo[cam_idx].fileString, fileSize,mBitRate);
	if (retval < 0) {
		resetRecorder();
		db_msg("some error occurs, so reset the recorder");
		sleep(1);
		mRecorder[recorder_idx]->initRecorder(mHC, size, framerate, mRecordDBInfo[cam_idx].fileString, fileSize,mBitRate);
	}
	//mRecorder[recorder_idx]->setSilent(mSilent);
	return retval;
}


int RecordPreview::startRecord(int recorder_idx, CdrRecordParam_t param)
{
	int retval;
	HWND hStatusBar;
	int isEnableRecord;
	int cam_idx = recorder_idx;
	bool hasSession = false;

	if (recorder_idx >= CAM_CNT) {
		cam_idx = recorder_idx - CAM_CNT;
		hasSession = true;
	}
	
	{
		Mutex::Autolock _l(mRecordLock);
	//	if (!hasSession) {
			if(mRecorder[recorder_idx] == NULL) {
				if (hasSession) {
					mRecorder[recorder_idx] = new CdrMediaRecorder(recorder_idx, mSession);
				} else {
					mRecorder[recorder_idx] = new CdrMediaRecorder(recorder_idx);
				}
				mRecorder[recorder_idx]->setOnInfoCallback(this);
				mRecorder[recorder_idx]->setOnErrorCallback(this);
			}
			if(mRecorder[recorder_idx]->getRecorderState() == RECORDER_RECORDING) {
				db_error("cam_type: %d, recording is started\n", recorder_idx);
				return -1;
			}
			isEnableRecord = mCdrMain->isEnableRecord();
			if (isEnableRecord != FORBID_NORMAL) {
				if(isEnableRecord == FORBID_FORMAT){
					SendMessage(mHwnd, MSG_SHOW_TIP_LABEL,LABEL_FORMATING_TFCARD,0);
				}
				db_msg("xxxxxxxx\n");
				return -1;
			}
	//	}
		retval = prepareRecord(recorder_idx, param);
		if (!hasSession) {
			if (retval < 0 ) {
				db_error("fail to prepare");
				if (mRecorder[CAM_CSI]->getRecorderState() != RECORDER_RECORDING) {
					if(retval == RET_NOT_MOUNT) {
						//ShowTipLabel(mHwnd, LABEL_NO_TFCARD);
						SendNotifyMessage(mHwnd, MSG_SHOW_TIP_LABEL, LABEL_NO_TFCARD, 0);
					} else if(retval == RET_IO_NO_RECORD) {
						//ShowTipLabel(mHwnd, LABEL_TFCARD_FULL);
						SendNotifyMessage(mHwnd, MSG_SHOW_TIP_LABEL, LABEL_TFCARD_FULL, 0);
					}
				}
				return retval;
			}
			if(cam_idx == CAM_CSI) {
				mCdrMain->setSTBRecordVTL(param.duration);
				hStatusBar = mCdrMain->getWindowHandle(WINDOWID_STATUSBAR);
				SendNotifyMessage(hStatusBar, STBM_START_RECORD_TIMER, 0, 0);
			}
			mBakFileString[cam_idx].empty();
			mBakNow[cam_idx] = 0;
		}

		if(!getCyclicVTL_Flag()&& !mVtlThread->isRuning()){
			mVtlThread->start();
		}
		retval = mRecorder[recorder_idx]->start();
	}
	return 0;
}


uint32_t RecordPreview::camPreAllocSize(int mDuration , uint32_t bitrate)
{
	return ALIGN32M((MINUTE(mDuration)*bitrate/8)) ;
}

int RecordPreview::checkStorage()
{
	if(mCdrMain->getNeedCheckSD()){
		StorageManager* smt;
		smt = StorageManager::getInstance();
		if(smt->isInsert()){
			#ifdef FATFS
				if(!mCdrMain->checkSDFs()){
			#else
				if(!mCdrMain->checkSDFs() || !smt->isMount()) {
			#endif
					ResourceManager *rm = ResourceManager::getInstance();
					bool isAdasScreen = rm->getResBoolValue(ID_MENU_LIST_SMARTALGORITHM);
					if(allowShowTip())
						SendNotifyMessage(mCdrMain->getWindowHandle(WINDOWID_MENU), MSG_FORMAT_TFDIALOG, (WPARAM)isAdasScreen, 0);
					return  -1;
				}
			}
	}
	return 0;
}

int RecordPreview::startRecord(int cam_type)
{
	int ret = checkStorage();
	if (ret < 0) {
		return ret;
	}
	CdrRecordParam_t param;
	param.video_type = VIDEO_TYPE_NORMAL;
	if(cam_type == CAM_CSI)
		param.fileSize = camPreAllocSize(mDuration,mBitRate);//CSI_PREALLOC_SIZE(mDuration);
	else
		param.fileSize = UVC_PREALLOC_SIZE(mDuration);
	param.duration = mDuration;

	return startRecord(cam_type, param);
}
int RecordPreview::startRecord()
{
	int ret;
	mImpactLockFlag = false;
	db_msg("xxxxxxxx\n");
	if(mCamera[CAM_CSI]) {
		if((ret = startRecord(CAM_CSI)) < 0) {
			db_error("startRecord CAM_CSI failed, ret is %d\n", ret);
			return -1;
		}
	}
	if(mCamera[CAM_UVC]) {
		if((ret = startRecord(CAM_UVC)) < 0) {
			db_error("startRecord CAM_UVC failed, ret is %d\n", ret);
			return -1;
		}
	}
	//mCdrMain->noWork(false);
	//StorageManager *sm = StorageManager::getInstance();
	//sm->startSyncThread();
	return 0;
}


int RecordPreview::stopRecord(int recorder_idx)
{
	int ret = 0;
	StorageManager* sm = StorageManager::getInstance();
	HWND hStatusBar;
	String8 needDeleteFile;
	int cam_idx = recorder_idx;
	bool hasSession = false;
	if (recorder_idx >= CAM_CNT) {
		cam_idx = recorder_idx - CAM_CNT;
		hasSession = true;
	}

	{
		Mutex::Autolock _l(mRecordLock);		
		db_msg("mCamExist recorder_idx :%d", recorder_idx);
		if(mRecorder[recorder_idx] == NULL) {
			db_msg("mRecorder[%d] is NULL\n", recorder_idx);
			return 0;
		}
		if(mRecorder[recorder_idx]->getRecorderState() != RECORDER_RECORDING) {
			db_error("!!!!!cam_type: %d, recording is stopped\n", recorder_idx);
			return -1;
		}
		if (!hasSession) {
			if(cam_idx == CAM_CSI) {
				hStatusBar = mCdrMain->getWindowHandle(WINDOWID_STATUSBAR);
				SendNotifyMessage(hStatusBar, STBM_STOP_RECORD_TIMER, 0, 0);
			}
		}
		ret = mRecorder[recorder_idx]->stop();
		db_error("mRecorder[recorder_idx]->stop() ret:%d\n", ret);
		if (ret < 0) {
			return ret;
		}
		mRecorder[recorder_idx]->stopImpactVideo();
		//if (hasSession) {
			needDeleteFile = getDeleteFlag(cam_idx);
			db_msg("~~~~~~~~~~STOP record ,check need delete file:%d %s\n", !needDeleteFile.isEmpty(), needDeleteFile.string());
			if (!needDeleteFile.isEmpty()) {
				if(sm->fileSize(needDeleteFile) == 0) {
					sm->deleteFile(needDeleteFile.string());
					clrDeleteFlag(cam_idx, needDeleteFile );
				}
			}
		//}
	}

	if(mOrignFileName[recorder_idx] != ""){
		mOrignFileName[recorder_idx] = "";
	}
	db_msg("stopRecord finished\n");
	return ret;
}

int RecordPreview::stopRecord()
{
	int ret = 0;
	mImpactLockFlag = false;
	if(mCdrMain->getWakeupState()){
		mCdrMain->parkRecordTimer(false);
		mCdrMain->setWakeupState(false);
	}
	if(!getCyclicVTL_Flag() && mVtlThread->isRuning()){
		mVtlThread->stopThread();
	}
	if(mRPWindowState == STATUS_AWMD) {
		if(isRecording(CAM_CSI)) {
			db_msg("need delay start the awmd record\n");
			setDelayStartAWMD(true);
		}
	}
	ret = stopRecord(CAM_CSI);
	if (ret < 0) {
		db_msg("fail to stop recorder csi");
	}

	ret = stopRecord(CAM_UVC);
	if (ret < 0) {
		db_msg("fail to stop recorder uvc");
	}
	StorageManager *sm = StorageManager::getInstance();
	sm->startSyncThread();

	if(mOrignFileName[CAM_CSI] != ""){
		mOrignFileName[CAM_CSI] = "" ;
	}
	if(mOrignFileName[CAM_UVC] != ""){
		mOrignFileName[CAM_UVC] = "" ;
	}

	return ret;
}


int RecordPreview::stopRecordSync()
{
	int ret = 0;

	if(mRPWindowState == STATUS_AWMD) {
		if(isRecording(CAM_CSI)) {
			db_msg("need delay start the awmd record\n");
			setDelayStartAWMD(true);
		}
	}
	ret = stopRecord(CAM_CSI);
	if (ret < 0) {
		db_msg("fail to stop recorder csi");
	}

	ret = stopRecord(CAM_UVC);
	if (ret < 0) {
		db_msg("fail to stop recorder uvc");
	}
	StorageManager *sm = StorageManager::getInstance();
	sm->storageSync();
	return ret;

}

void RecordPreview::recordRelease(int cam_type)
{
	if (mRecorder[cam_type]) {
		mRecorder[cam_type]->release();
	}
}

void RecordPreview::recordRelease()
{
	recordRelease(CAM_CSI);
}

void RecordPreview::previewRelease(int cam_type)
{
	if(mCamera[cam_type])
		mCamera[cam_type]->release();
}

void RecordPreview::previewRelease()
{
	previewRelease(CAM_CSI);
}

void RecordPreview::stopPreview(int cam_type)
{
	if(mCamera[cam_type]) {
		mCamera[cam_type]->stopPreview();
	}
}

void RecordPreview::stopPreview()
{
	stopPreview(CAM_CSI);
	stopPreview(CAM_UVC);
}

void RecordPreview::stopPreviewOutside(int flag)
{
	if (flag & CMD_NO_NEED_STOP_PREVIEW) {
		mNeedStartPreview = false;
	} else if (flag & CMD_STOP_PREVIEW) {
		stopPreview(CAM_CSI);
		mNeedStartPreview = true;
	}
	if (flag & CMD_CLOSE_LAYER) {
		CdrDisplay *cdCSI = mCamera[CAM_CSI]->getDisp();
		cdCSI->close();
	} else if (flag & CMD_CLEAN_LAYER) {
		CdrDisplay *cdCSI = mCamera[CAM_CSI]->getDisp();
		cdCSI->clean();
	}
}

CdrDisplay *RecordPreview::getCommonDisp(int disp_id)
{
	if(mCamera[disp_id])
		return mCamera[disp_id]->getDisp();
	else
		return NULL;
}

#ifdef PLATFORM_0
int RecordPreview::getHdl(int disp_id)
{
       CdrDisplay *cd = getCommonDisp(disp_id);
       return cd->getHandle();
}
#endif
void RecordPreview::setSilent(bool mode)
{
	db_error("-------------------setSilent:%d\n", mode);
	bool flag = mode?false:true;
	if (mRecorder[CAM_CSI]) {
		mRecorder[CAM_CSI]->setSilent(flag);
	}
	if (mRecorder[CAM_UVC]) {
		mRecorder[CAM_UVC]->setSilent(flag);
	}
	//mSilent = mode;
}

void RecordPreview::setVideoQuality(int idx)
{
	Mutex::Autolock _l(mLock);
	db_error("----------------setVideoQuality, %d", idx);
	if (idx == 0) {
		mVideoSize = Size(VideoSize_FHD_W_csi, VideoSize_FHD_H_csi);
	} else if (idx == 1) {
		mVideoSize = Size(VideoSize_HD_W_csi, VideoSize_HD_H_csi);
	}
	mSessionSize.width   = mVideoSize.width  / 4;
	mSessionSize.height  = mVideoSize.height / 4;
	if (mCamera[CAM_CSI]) {
		mCamera[CAM_CSI]->setWaterMarkPos(mVideoSize.height);
	}
	if (mCamera[CAM_UVC]) {
		mCamera[CAM_UVC]->setWaterMarkPos(VideoSize_HD_H_uvc);
	}
}

void RecordPreview::setVideoBitRate(int idx)
{
	Mutex::Autolock _l(mLock);
	ResourceManager* rm = ResourceManager::getInstance();
	int current ;
	current = rm->getSubMenuCurrentIndex(ID_MENU_LIST_VIDEO_RESOLUTION);

	db_msg(">>>>>>>setVideoBitRate[%d],current[%d]", idx,current);
	if (mCamera[CAM_CSI]) {
		if(current){
			db_msg(">>>>>>>>>720P");
			if(idx == 0)
				mBitRate = VideoQlt_STANDARD_csi_M_lite<<20;
			else if(idx == 1)
				mBitRate = VideoQlt_HIGH_csi_M_lite<<20;
			else
				mBitRate = VideoQlt_SUPER_csi_M_lite<<20;
		}else{
				db_msg(">>>>>>>>>1080P");
			if(idx == 0)
				mBitRate = VideoQlt_STANDARD_csi_M<<20;
			else if(idx == 1)
				mBitRate = VideoQlt_HIGH_csi_M<<20;
			else
				mBitRate = VideoQlt_SUPER_csi_M<<20;
		}
	}
}

void RecordPreview::setPicQuality(int idx)
{
	Mutex::Autolock _l(mLock);
	db_error("----------------setPicResolution %d", idx);
	int percent = 100-(2-idx)*20;	//60 80 100
	if (mCamera[CAM_CSI]) {
		mCamera[CAM_CSI]->setPicQuality(percent);
	}
	if (mCamera[CAM_UVC]) {
		mCamera[CAM_UVC]->setPicQuality(percent);
	}
}

void RecordPreview::setPicResolution(int idx)
{
	Mutex::Autolock _l(mLock);
	db_error("----------------setPicResolution %d", idx);
	if (mCamera[CAM_CSI]) {
		mCamera[CAM_CSI]->setPicResolution((PicResolution_t)idx);
	}
	if (mCamera[CAM_UVC]) {
		mCamera[CAM_UVC]->setPicResolution((PicResolution_t)idx);
	}
}
 
void RecordPreview::setPicResolution(int width,int height)
{
	Mutex::Autolock _l(mLock);
	if (mCamera[CAM_CSI]) {
		mCamera[CAM_CSI]->setPicResolution(width,height);
	}
	
} 

 int RecordPreview::CalculateRecordTime(int camID)
 {
 #ifdef LIMIT_LOOP_TIME_MIN
	return LIMIT_LOOP_TIME_MIN*60;
 #else
	int size,ret;
	float rate = 0;
	if(camID == CAM_CSI){
		rate = FRONT_CAM_BITRATE/(8.0*1024*1024);
	}else if(camID == CAM_UVC){
		rate = BACK_CAM_BITRATE/(8.0*1024*1024);
	}
	size = (int)(rate*3600);
	if(size < MAX_FILE_SIZE){
		ret = 60*60;
	}else{
		ret = (int)MAX_FILE_SIZE/rate;
	}
	db_msg("*******************ret:%d,min",ret);
	return ret;
#endif
	
 }
void RecordPreview::setRecordTime(int idx)
{
	Mutex::Autolock _l(mLock);
	db_msg("----------------setRecordTime %d!", idx);
	switch(idx) {
		case 0:
			idx = RTL_0;
			break;
		case 1:
			idx = RTL_1;
			break;
		case 2:
			idx = RTL_2;
			break;
		case 3:
			idx = RTL_AVAILABLE;
			break;
		default:
			return;
	}
	int time=0;
	
	if (idx > RTL_2){
		setCyclicVTL_Flag(false);
		time =  CalculateRecordTime()*1000;
		db_msg("       -----------time:%d",time);
	}else{
		setCyclicVTL_Flag(true);
		time = (idx*60*1000);
	}
	db_msg("time=%d minute\n", time/(1000*60));
	mDuration = time;
#if 0
	if (mRecorder[CAM_CSI]) {
		mRecorder[CAM_CSI]->setDuration(time);
	}
#endif
}

void RecordPreview::setRecordTime(PARK_MON_VTL_t value)
{
	Mutex::Autolock _l(mLock);
	db_msg("----------------setRecordTime %ds!", value);
	if(value < 0 || value > 30)
		return;
	int time = (value*1000);
//	mDuration = time;
	StatusBar * mSTB = mCdrMain->getStatusBar();
	mSTB->setRecordVTL(time);
#if 0
	if (mRecorder[CAM_CSI]) {
		mRecorder[CAM_CSI]->setDuration(time);
	}
#endif
}


void RecordPreview::setAWMD(bool val)
{
	db_msg("----------------setAWMD %d!", val);
	if (mCamera[CAM_CSI]) {
		mCamera[CAM_CSI]->setAWMD(val);
	}
}

void RecordPreview::setADAS(bool val)
{
	db_msg("----------------setADAS %d!", val);
	if (mCamera[CAM_CSI]) {
		mCamera[CAM_CSI]->setAdas(val);
	}
}

void RecordPreview::setWatermark(int flag)
{
	ResourceManager * rm = ResourceManager::getInstance();
	mTWM = flag&WATERMARK_TWM;
	mLWM = flag&WATERMARK_LWM;
	strcpy(strLWM, rm->menuDataLWM.lwaterMark);
	if(mCamera[CAM_CSI]) {
		mCamera[CAM_CSI]->setLWaterMark(mTWM, mLWM, strLWM);
	}
	if(mCamera[CAM_UVC]) {
		mCamera[CAM_UVC]->setLWaterMark(mTWM, mLWM, strLWM);
	}
}

int RecordPreview::msgCallback(int msgType, int idx)
{
	ResourceManager* rm = ResourceManager::getInstance();
	switch(msgType) {
	case MSG_REFRESH:
		{
			idx = rm->getResIntValue(ID_MENU_LIST_VIDEO_RESOLUTION, INTVAL_SUBMENU_INDEX);
			setVideoQuality(idx);

			idx = rm->getResIntValue(ID_MENU_LIST_VIDEO_BITRATE, INTVAL_SUBMENU_INDEX);
			setVideoBitRate(idx);			

			idx = rm->getResIntValue(ID_MENU_LIST_PHOTO_RESOLUTION, INTVAL_SUBMENU_INDEX);
			setPicResolution(idx);

			idx = rm->getResIntValue(ID_MENU_LIST_VTL, INTVAL_SUBMENU_INDEX);
			setRecordTime(idx);

			idx = rm->getResIntValue(ID_MENU_LIST_WB, INTVAL_SUBMENU_INDEX);
			if (mCamera[CAM_CSI]) {
				mCamera[CAM_CSI]->setWhiteBalance(WHITE_BALANCE[idx]);
			}
			idx = rm->getResIntValue(ID_MENU_LIST_CONTRAST, INTVAL_SUBMENU_INDEX);
			if (mCamera[CAM_CSI]) {
				mCamera[CAM_CSI]->setContrast(idx);
			}
			idx = rm->getResIntValue(ID_MENU_LIST_EXPOSURE, INTVAL_SUBMENU_INDEX);
			if (mCamera[CAM_CSI]) {
				mCamera[CAM_CSI]->setExposure(idx-3);
			}
			idx = rm->getResIntValue(ID_MENU_LIST_LIGHT_FREQ, INTVAL_SUBMENU_INDEX);
			if (mCamera[CAM_CSI]) {
				mCamera[CAM_CSI]->setLightFreq(idx);
			}
			bool val;
			val = rm->getResBoolValue(ID_MENU_LIST_RECORD_SOUND);
			setSilent(!val);

			val = rm->getResBoolValue(ID_MENU_LIST_RECORD_SOUND);
			setSilent(val);

			db_msg(">>>>>>>>>>>>>>>>>record msgback");

			val = rm->getResBoolValue(ID_MENU_LIST_AWMD);
			setAWMD(val);
#ifdef APP_ADAS
			val = rm->getResBoolValue(ID_MENU_LIST_SMARTALGORITHM);
			setADAS(val);
#endif
			updateAWMDWindowPic();
		}
		break;
	case MSG_RM_VIDEO_RESOLUTION:
		{
			setVideoQuality(idx);
		}
		break;
	case MSG_RM_VIDEO_BITRATE:
		{
			setVideoBitRate(idx);
		}
		break;		
	case MSG_RM_PIC_RESOLUTION:
		{
			setPicResolution(idx);
		}
		break;
	case MSG_RM_PIC_QUALITY:
		{
			setPicQuality(idx);
		}
		break;
	case MSG_RM_VIDEO_TIME_LENGTH:
		{
			setRecordTime(idx);
		}
		break;
	case MSG_RM_WHITEBALANCE:
		{
			if (mCamera[CAM_CSI]) {
				mCamera[CAM_CSI]->setWhiteBalance(WHITE_BALANCE[idx]);
			}
		}
		break;
	case MSG_RM_CONTRAST:
		{
			if (mCamera[CAM_CSI]) {
				mCamera[CAM_CSI]->setContrast(idx);
			}
		}
		break;
	case MSG_RM_EXPOSURE:
		{
			if (mCamera[CAM_CSI]) {
				mCamera[CAM_CSI]->setExposure(idx-3);
			}
		}
		break;
	case MSG_RM_LIGHT_FREQ:
		{
			if (mCamera[CAM_CSI]) {
				mCamera[CAM_CSI]->setLightFreq(idx);
			}
		}
	break;
	case MSG_RM_RECORD_SOUND:
		{
			// idx 1: enable record sound
			// idx 0: disable record sound
			setSilent((bool)idx);
		}
		break;
	case MSG_AWMD:
		setAWMD((bool)idx);
		updateAWMDWindowPic();
		break;
	#ifdef APP_ADAS
	case MSG_ADAS_SWITCH:
		setADAS((bool)idx);
		break;
	#endif
	case MSG_WATER_MARK:
		setWatermark((int)idx);
		break;
	case MSG_RM_TAKE_PICTURE:
		takePicture(1);
		break;
	default:
		break;
	}
	return 0;
}

void RecordPreview::showPicFullScreen(int mode, bool bDisp)
{
	if (bDisp && mCdrMain->mReverseMode) {
		const char *path;
		int screenW,screenH;
		getScreenInfo(&screenW, &screenH);
		if (mode == PIC_REVERSE) {
			path = "/system/res/others/back_car.png";
		}
		LoadBitmapFromFile(HDC_SCREEN,&mReverseBmp,path);
		HWND retWnd = GetDlgItem(mHwnd, ID_RECORDPREVIEW_IMAGE);
		SendMessage(retWnd, STM_SETIMAGE, (WPARAM)&mReverseBmp, 0);
		ShowWindow(retWnd, SW_SHOWNORMAL);
		//FillBoxWithBitmap(HDC_SCREEN,0,30,screenW,screenH-60,&mReverseBmp);
	} else if (!bDisp && mReverseBmp.bmBits){
		int screenW,screenH;
		getScreenInfo(&screenW, &screenH);
		HWND retWnd = GetDlgItem(mHwnd, ID_RECORDPREVIEW_IMAGE);
		ShowWindow(retWnd, SW_HIDE);
		UnloadBitmap(&mReverseBmp);
		mReverseBmp.bmBits = NULL;
		SetBrushColor(HDC_SCREEN,RGBA2Pixel(HDC_SCREEN, 0x00, 0x00, 0x00, 0x00));
		FillBox(HDC_SCREEN,0,30,screenW,screenH-60);
	}
}

void RecordPreview::feedAwmdTimer(int seconds)
{
	mImpactLockFlag = true;
	if(mCamera[CAM_CSI])
		mCamera[CAM_CSI]->feedAwmdTimer(seconds);
}

int RecordPreview::queryAwmdExpirationTime(time_t* sec, long* nsec)
{
	if(mCamera[CAM_CSI])
		return mCamera[CAM_CSI]->queryAwmdExpirationTime(sec, nsec);
	else
		return -1;
}


void RecordPreview::updateAWMDWindowPic()
{
	bool awmd;
	ResourceManager* rm;

	rm = ResourceManager::getInstance();
	awmd = rm->getResBoolValue(ID_MENU_LIST_AWMD);
	if(awmd == true) {
		db_msg("updateAWMDWindowPic true\n");
		mRPWindowState = STATUS_AWMD;
		mCdrMain->setCurrentRPWindowState(mRPWindowState);
	} else {
		db_msg("updateAWMDWindowPic false\n");
		mRPWindowState = mOldRPWindowState;
		//	mRPWindowState = STATUS_RECORDPREVIEW;	
		mCdrMain->setCurrentRPWindowState(mRPWindowState);
	}
}

void RecordPreview::switchAWMD()
{
	if(mRPWindowState != STATUS_RECORDPREVIEW && mRPWindowState != STATUS_AWMD)
		return;
	if(mRPWindowState == STATUS_AWMD) {
		db_msg("disable awmd\n");	
		switchAWMD(false);
		setDelayStartAWMD(false);
	} else {
		db_msg("enable awmd\n");	
		switchAWMD(true);
	}
}

void RecordPreview::switchAWMD(bool enable)
{
	ResourceManager* rm;
	time_t sec = 0;
	long nsec = 0;
	CdrRecordParam_t param;

	rm = ResourceManager::getInstance();
	if(enable == true) {
		stopRecord();
		rm->setResBoolValue(ID_MENU_LIST_AWMD, true);
		mRPWindowState = STATUS_AWMD;
		mCdrMain->setCurrentRPWindowState(mRPWindowState);
		usleep(500);
		setAWMD(true);
	} else {
		if(queryAwmdExpirationTime(&sec, &nsec) < 0) {
			db_error("query awmd timer expiration time failed\n");
		}
		setAWMD(false);
		if(isRecording(CAM_CSI) == true) {
			if(sec > 0 || (sec == 0 && nsec > 500 * 1000 * 1000)) { 
				param.video_type = VIDEO_TYPE_NORMAL;
				param.fileSize = camPreAllocSize(mDuration,mBitRate);//CSI_PREALLOC_SIZE(mDuration);
				param.duration = mDuration;
				db_msg("set duration %d ms\n", mDuration);
				mRecorder[CAM_CSI]->setDuration(mDuration);
				mRecorder[CAM_CSI]->setRecordParam(param);
			}
		}
		rm->setResBoolValue(ID_MENU_LIST_AWMD, false);
		mRPWindowState = STATUS_RECORDPREVIEW;
		mCdrMain->setCurrentRPWindowState(mRPWindowState);
	}
}

int RecordPreview::takePicture(int flag)
{
	HWND hStatusBar;
	const char* ptr = NULL;
	StorageManager* sm;
	ResourceManager* rm;
	int ret = 0;
	ret = checkStorage();
	if (ret < 0) {
		return ret;
	}
	if (mCamera[CAM_CSI])
		mCamera[CAM_CSI]->setPictureSizeMode(CameraParameters::AWEXTEND_PICTURE_SIZE_MODE::UseParameterPictureSize);
	if (mCamera[CAM_UVC])
		mCamera[CAM_UVC]->setPictureSizeMode(CameraParameters::AWEXTEND_PICTURE_SIZE_MODE::UseParameterPictureSize);
	if(mCamera[CAM_CSI] || mCamera[CAM_UVC]) {
		sm = StorageManager::getInstance();
		if(sm->isMount() == false) {
			if (!mStartRtspSession)
				ShowTipLabel(mHwnd, LABEL_NO_TFCARD);
			return -1;
		}
		if(sm->isDiskFull() == true) {
			if (!mStartRtspSession)
				ShowTipLabel(mHwnd, LABEL_TFCARD_FULL);
			return -1;
		}
		rm = ResourceManager::getInstance();
		hStatusBar = mCdrMain->getWindowHandle(WINDOWID_STATUSBAR);
		{
			Mutex::Autolock _l(mLockPic);
			if (mCamera[CAM_CSI]) {
				if (mAllowTakePic[CAM_CSI]) {
					db_msg("csi take pic");
					mAllowTakePic[CAM_CSI] = false;
					mCdrMain->playSounds(TAKEPIC_SOUNDS);
					mCamera[CAM_CSI]->takePicture();
				} else {
					db_msg("csi last action hasn't finished");
					return -1;
				}
			}
			if (mCamera[CAM_UVC]) {
				if (mAllowTakePic[CAM_UVC]) {
					mAllowTakePic[CAM_UVC] = false;
					db_msg("uvc take pic");
					//mCdrMain->playSounds(TAKEPIC_SOUNDS);
					mCamera[CAM_UVC]->takePicture();
				} else {
					db_msg("uvc last action hasn't finished");
					return -1;
				}
			}
		}
		if (!mStartRtspSession) {
			if(flag==1)
				ptr = rm->getLabel(LANG_LABEL_TAKE_PICTURE);
			else if(flag==2)
				ptr = rm->getLabel(LANG_LABEL_SCREEN_SHOT);
			if(ptr) {
				SendMessage(hStatusBar, STBM_SETLABEL1_TEXT, 0, (LPARAM)ptr);
			}
		}
	}
	return ret;
}

void RecordPreview::savePicture(void*data, int size, int id)
{
	int ret;
	time_t now;
	HWND hStatusBar;
	StorageManager* sm;
	String8 file;
	Elem elem;

	sm = StorageManager::getInstance();
	hStatusBar = mCdrMain->getWindowHandle(WINDOWID_STATUSBAR);
	if(snapflag)
		ret = sm->generateFile(now, file, id, PHOTO_TYPE_SNAP);
    else
	ret = sm->generateFile(now, file, id, PHOTO_TYPE_NORMAL);
    if(snapflag)
		snapflag=0;
	if(ret <  0) {
		db_error("generate picture file failed, retval is %d\n", ret);
		return;	
	}
	db_msg("picture is %s\n", file.string());

	sm->savePicture(data, size, file);
	elem.file = (char*)file.string();
	elem.time = now;
	elem.type = (char*)TYPE_PIC;
	elem.info = (char*)"";
	sm->dbAddFile(&elem);

	usleep(1000 * 1000);
	SendMessage(hStatusBar, STBM_CLEAR_LABEL1_TEXT, WINDOWID_RECORDPREVIEW, 0);
}

bool RecordPreview::isRecording(int cam_type)
{
	Mutex::Autolock _l(mRecordLock);
	if(mRecorder[cam_type]) {
		if(mRecorder[cam_type]->getRecorderState() == RECORDER_RECORDING)	
			return true;
	}
	return false;
}
bool RecordPreview::isRecording()
{
	if(isRecording(CAM_CSI) == true || isRecording(CAM_UVC) == true)
		return true;
	return false;
}


int RecordPreview::parkRecord()
{
	int ret = 0;
	setRecordTime(2);  //record 5min video
	mCdrMain->setWakeupState(true);
	ret = startRecord();
	if (ret < 0) {
		resetRecorder();
		db_msg("some error occurs, so reset the recorder");
		sleep(1);	//when suspend, the tfcard may be not ready, so wait 1s.
		ret = startRecord();
	}
	if (ret < 0) {
		mCdrMain->noWork(true);
		mCdrMain->setWakeupState(false);
		return ret;
	}
	setRecordTime(PARK_MON_VTL_15S);  //statubar 15s
	mCdrMain->parkRecordTimer(true);
	mFilelock->force(true);
	return 0;
}

int RecordPreview::impactOccur()
{

	int ret = 0;
	StorageManager * sm = StorageManager::getInstance();
	if(!IF_PreviewMode(mRPWindowState)){
		return 0;
	}
	if(allowShowTip() && !isRecording()){
		ret = startRecord();
	}
	if (ret < 0) {
		return ret;
	}
	db_msg(" impactOccur() mCdrMain->getWakeupState():%d  ",mCdrMain->getWakeupState());
	if(mCdrMain->getWakeupState()){
		HWND hStatusBar;
		hStatusBar = mCdrMain->getWindowHandle(WINDOWID_STATUSBAR);
		SendMessage(hStatusBar, STBM_START_RECORD_TIMER, 1, 0);
		mCdrMain->parkRecordTimer(false);
		mCdrMain->parkRecordTimer(true);
	}else{
		db_msg("impact Occur mRPWindowState:%d,WINID:%d",mRPWindowState,mCdrMain->getCurWindowId());
		mFilelock->force(true);
		StatusBar * mSTB = mCdrMain->getStatusBar();	
		int curTime = mSTB->getCurRecordVTL();
		int reTime = mSTB->getRecordVTL();
		if(curTime>(reTime-15)){  //lock the next file
			mImpactLockFlag = true;
		}else if(curTime<15){//lock the pre file
			String8 nfile;
			db_msg("mOrignFileName[CAM_CSI]:%s, " ,mOrignFileName[CAM_CSI].string());
			if(mOrignFileName[CAM_CSI] != "" && strstr(mOrignFileName[CAM_CSI], FLAG_SOS) == NULL){	
				sm->setFileInfo(mOrignFileName[CAM_CSI], true, nfile);
				mOrignFileName[CAM_CSI] = "";
			}
			db_msg("mOrignFileName[CAM_UVC]:%s, " ,mOrignFileName[CAM_UVC].string());
			if(mOrignFileName[CAM_UVC] != "" && strstr(mOrignFileName[CAM_UVC], FLAG_SOS) == NULL){
				sm->setFileInfo( mOrignFileName[CAM_UVC], true, nfile);
				mOrignFileName[CAM_UVC] = "";
			}
		}
	}
	SendMessage(mHwnd, MSG_SHOW_TIP_LABEL,LABEL_SOS_WARNING, 0);
	return 0;
}

/*
 * camera CSI should stop preview, keep layer open, and keep layer rectBig
 * camera UVC should stop preview, keep layer close
 * */
void RecordPreview::prepareCamera4Playback(bool closeLayer)
{
	CdrDisplay *cdCSI = NULL, *cdUVC = NULL;

	if(mCamera[CAM_CSI]) {
		cdCSI = mCamera[CAM_CSI]->getDisp();
		cdCSI->open();
		cdCSI->setRect(rectBig);
		//stopPreview(CAM_CSI);
//		if (cdCSI && closeLayer) {
//			cdCSI->clean();
//		}
	}

	if(mCamera[CAM_UVC]) {
		cdUVC = mCamera[CAM_UVC]->getDisp();
		cdUVC->close();
		//stopPreview(CAM_UVC);
		//if(cdUVC && closeLayer) {
		//	cdUVC->clean();
		//}
	}
}

void RecordPreview::restorePreviewOutSide(unsigned int windowID)
{
	CdrDisplay *cdCSI = NULL, *cdUVC = NULL;//, *top = NULL, *bot=NULL;
	pipMode_t newPipMode;
	if(windowID == WINDOWID_MENU && !mCdrMain->mReverseMode) {
		db_info("return from menu\n");
		return;
	}
	
	if(!mCamera[CAM_CSI])
		return;

	if(!mCamExist[CAM_UVC]) {
		newPipMode = CSI_ONLY;
	} else {
		if(!mCamera[CAM_UVC]) {
			initCamera(CAM_UVC);
		}
		newPipMode = UVC_ON_CSI;
	}
	db_msg("pipmode :%d, mPIPmode:%d", newPipMode, mPIPMode);

	if((mPIPMode==CSI_ON_UVC || mPIPMode==UVC_ONLY)&& mCamera[CAM_UVC]){
		cdUVC = mCamera[CAM_UVC]->getDisp();
		startPreview(CAM_UVC);
		cdCSI = mCamera[CAM_CSI]->getDisp();
		startPreview(CAM_CSI);
		cdUVC->setRect(rectBig);
		cdCSI->setRect(rectSmall);
		cdUVC->open();
		cdUVC->setBottom();
		if (!mCdrMain->mReverseMode) {
			db_msg(" ");
			cdCSI->open();
		}
		cdUVC->exchange(cdCSI->getHandle(),1);
		mPIPMode = UVC_ON_CSI;
		return ;
	}
	
	if (newPipMode == UVC_ON_CSI) {
		cdUVC = mCamera[CAM_UVC]->getDisp();
		cdUVC->clean();
		startPreview(CAM_UVC);
		cdUVC->setBottom();
		cdUVC->setRect(rectSmall);
	}
	cdCSI = mCamera[CAM_CSI]->getDisp();
	if (mNeedStartPreview) {
		startPreview(CAM_CSI);
	}
	cdCSI->setBottom();
	cdCSI->setRect(rectBig);
	cdCSI->open();
	mPIPMode = newPipMode;
	if (mCdrMain->mReverseMode) {
		transformPIP(UVC_ONLY);
	}
}

void RecordPreview::setSdcardState(bool bExist)
{
	if (mRecorder[CAM_CSI]) {
		mRecorder[CAM_CSI]->setSdcardState(bExist);
	}
	if (mRecorder[CAM_UVC]) {
		mRecorder[CAM_UVC]->setSdcardState(bExist);
	}
}
bool RecordPreview::needDelayStartAWMD(void)
{
	Mutex::Autolock _l(mAwmdLock);
	return mAwmdDelayStart;
}

void RecordPreview::setDelayStartAWMD(bool value)
{
	Mutex::Autolock _l(mAwmdLock);
	mAwmdDelayStart = value;
	if(value == true)
		mAwmdDelayCount = 0;
}

bool RecordPreview::getUvcState()
{
	return mCamExist[CAM_UVC];
}

void RecordPreview::releaseRecorder(int cam_id)
{
	if(mRecorder[cam_id]) {
		delete mRecorder[cam_id];
		mRecorder[cam_id] = NULL;
	}
}
void RecordPreview::sessionInit(void *rtspSession)
{
	mStartRtspSession = true;
	mSessionSize.width   = mVideoSize.width  / 4;
	mSessionSize.height  = mVideoSize.height / 4;

	if (mCamExist[CAM_CSI]) {
		if (!mRecorder[CAM_CSI_NET]) {
			mRecorder[CAM_CSI_NET] = new CdrMediaRecorder(CAM_CSI_NET, rtspSession);
			mRecorder[CAM_CSI_NET]->setOnInfoCallback(this);
			mRecorder[CAM_CSI_NET]->setOnErrorCallback(this);
#ifdef PLATFORM_0
			if(mRecorder[CAM_CSI]->getRecorderState() == RECORDER_RECORDING){	
				setVirtualCam(mRecorder[CAM_CSI]);
			}
#endif
		}
	}
#ifdef PLATFORM_1
	if (mCamExist[CAM_UVC])  {
		if (!mRecorder[CAM_UVC_NET]) {
			mRecorder[CAM_UVC_NET] = new CdrMediaRecorder(CAM_UVC_NET, rtspSession);
			mRecorder[CAM_UVC_NET]->setOnInfoCallback(this);
			mRecorder[CAM_UVC_NET]->setOnErrorCallback(this);
			
		}
	}
#endif
	mSession = rtspSession;
	mCurSessionCam = LOCAL2SESSION(CAM_CSI);
}

int RecordPreview::camChange(int cam_to, bool bStop)
{
	int ret = -1;
	int cam_from = SESSION2LOCAL(mCurSessionCam);
	cam_to = CAM_OTHER(cam_from);
	db_msg(" change from cam%d to cam%d", cam_from, cam_to);
	if(mCamExist[cam_to] && mCamExist[cam_from] && bStop) {
		if( (ret = stopRecord(mCurSessionCam) ) < 0) {
			db_error("stopRecord cam%d failed, ret is %d\n", cam_from, ret);
			return ret;
		}
	}
	ret = -1;
	if(mCamExist[cam_to]) {
		if( (ret = startRecord(LOCAL2SESSION(cam_to)) ) < 0) {
			db_error("startRecord cam%d failed, ret is %d\n", cam_to, ret);
			return -1;
		}
		mCurSessionCam = LOCAL2SESSION(cam_to);
	}
	return ret;
}

void RecordPreview::changeResolution(int level)
{
	int max_h = mVideoSize.height;
	int max_w = mVideoSize.width;
	int w;
	int h;
	void *session = NULL;
	db_msg("level : %d", level);
	if (level == 0) {
		w = max_w / 2;
		h = max_h / 2;
	} else {
		w = max_w / 4;
		h = max_h / 4;
	}
	db_msg("final_w : %d, final_h : %d", w, h);
	stopRecord(mCurSessionCam);
	CdrRecordParam_t param;
	param.duration = 30*1000;
	param.fileSize = 10*1024;
	param.video_type = VIDEO_TYPE_NORMAL;
	Size size;
	size.width  = w;
	size.height = h;
	prepareRecord(mCurSessionCam, param, &size);
	mRecorder[mCurSessionCam]->start();
}

unsigned int RecordPreview::getWindowsID()
{
	return mCdrMain->getCurWindowId();
}

bool RecordPreview::getAlignLine()
{
	if(mAlignLine)
		return mAlignLine;
	return FALSE;
}

void RecordPreview::setAlignLine(bool flag)
{
	if(mCamera[CAM_UVC]) {
		mCamera[CAM_UVC]->setCameraFlip(flag);
	}
}

RPWindowState_t RecordPreview::getRPWindowState()
{
	return mRPWindowState;
}

void RecordPreview::setRPWindowState(RPWindowState_t state)
{
	mRPWindowState = state;
	mCdrMain->setCurrentRPWindowState(mRPWindowState);
}

bool RecordPreview::getAllowTakePic()
{
	if(mAllowTakePic[CAM_CSI] && mAllowTakePic[CAM_UVC]){
		return true;
	}else{
		return false;
	}
}

void RecordPreview::adasScreenSwitch(bool flag)
{
	db_msg("<**RecordPreview::adasScreenSwitch**flag:%d***>",flag);
	if (mIsAdasScreen == flag || mCdrMain->getCurWindowId() != WINDOWID_RECORDPREVIEW) {
		return;
	}
	CdrDisplay *cdCSI;
	cdCSI = getCommonDisp(CAM_CSI);
	if(flag){
		cdCSI->openAdasScreen();
	} else {
		cdCSI->closeAdasScreen();
	}
	mIsAdasScreen = flag;

    if(!mCdrMain->getFormatTFAdasS()&& mIsAdasScreen){
		StorageManager* smt;
		smt = StorageManager::getInstance();
		if(smt->isInsert()){
			if(!mCdrMain->checkSDFs()){
				mCdrMain->setFormatTFAdasS(true);
				 SendNotifyMessage(mCdrMain->getWindowHandle(WINDOWID_MENU), MSG_FORMAT_TFDIALOG, (WPARAM)mIsAdasScreen, 0);

			}
		}
	}
	
}

bool RecordPreview::getIsAdasScreen()
{
	return mIsAdasScreen;
}


bool RecordPreview::camExisted(int idx)
{
	return mCamExist[idx];
}

int RecordPreview::getDuration()
{
	return mDuration;
}

void RecordPreview::needCache(bool val)
{
	mNeedCache = val;
}

void RecordPreview::startPCCam()
{
	//HerbCamera::Parameters params;
	//mCamera[CAM_CSI]->getParameters(&params);
	//params.setVideoSize(1920, 1080);
    //params.setPreviewSize(640, 360);
    //params.setPreviewFrameRate(30);
    //mCamera[CAM_CSI]->setParameters(&params);
    cfgDataSetString("sys.usb.config", "webcam,adb");
	usleep(200*1000);
	mCamera[CAM_CSI]->setUvcMode(1);
}

void RecordPreview::stopPCCam()
{
	mCamera[CAM_CSI]->setUvcMode(0);
	cfgDataSetString("sys.usb.config", "mass_storage,adb");
}

int RecordPreview::releaseResource()
{
	return 0;
}

void RecordPreview::suspend()
{
	//stopRecordSync();
	storagePlug(false);
	stopPreview();
	CdrDisplay *cdCSI = NULL;
	cdCSI = getCommonDisp(CAM_CSI);
	cdCSI->clean();
	CdrDisplay *cdUVC = NULL;
	cdUVC = getCommonDisp(CAM_UVC);
	if(cdUVC)
		cdUVC->clean();
	cdCSI->close(UI_LAY_ID);
	if (mCamera[CAM_CSI]) {
		mCamera[CAM_CSI]->suspend();
	}
	if (mCamera[CAM_UVC]) {
		mCamera[CAM_UVC]->suspend();
	}
}

void RecordPreview::resume()
{
	if (mCamera[CAM_CSI]) {
		mCamera[CAM_CSI]->resume();
	}
	if (mCamera[CAM_UVC]) {
		mCamera[CAM_UVC]->resume();
	}
	initCamera(CAM_CSI, false);
	initCamera(CAM_UVC, false);
	startPreview();
	set_ui_visible(true);
}

void RecordPreview::set_ui_visible(bool on)
{
	CdrDisplay *cdCSI = NULL;
	cdCSI = getCommonDisp(CAM_CSI);
	if (on) {
		cdCSI->open(UI_LAY_ID);
	} else {
		cdCSI->close(UI_LAY_ID);
	}
}

void RecordPreview::reverseMode(bool on)
{
	if (on) {
		set_ui_visible(false);
	} else {
		set_ui_visible(true);
	}
}

void RecordPreview::checkTFLeftSpace()
{
	int ret ;
	long long as  = availSize(MOUNT_PATH);
	if(as< MIN_FILE_SPCE){
		db_msg("availSize<50M stopRecord");
		if(isRecording()){
			stopRecord();
		}
		mVtlThread->stopThread();
		return ;
	}
}

bool RecordPreview::allowShowTip()
{
	return !mCdrMain->getPC2Connected();
}

int RecordPreview::specialProc(int cmd, int arg)
{
	switch(cmd) {
		case SPEC_CMD_LAYER_CLEAN: {
			CdrDisplay *cdCSI, *cdUVC;
			if (arg & 1) {
				cdCSI = getCommonDisp(CAM_CSI);
				if (cdCSI)
					cdCSI->clean();
			}
			if (arg & 2) {
				cdUVC = getCommonDisp(CAM_UVC);
				if (cdUVC)
					cdUVC->clean();
			}
		}
			break;
	}
	return 0;
}

void RecordPreview::resetRecorder()
{
	if (mRecorder[CAM_CSI]) {
		mRecorder[CAM_CSI]->reset();
	}
	if (mRecorder[CAM_UVC]) {
		mRecorder[CAM_UVC]->reset();
	}
}

void RecordPreview::FileLock::force(bool flag)
{
	Mutex::Autolock _l(mLock);
	if (mForced == flag) {
		return;
	}
	mData = flag;
	mForced = flag;
	mRM->condLock(flag);
}

void RecordPreview::FileLock::reverse()
{
	Mutex::Autolock _l(mLock);
	if (mForced) {
		return;
	}
	if (mData == true) {
		mRM->condLock(false);
		mData = false;
	} else {
		mRM->condLock(true);
		mData = true;
	}
}

bool RecordPreview::FileLock::getStatus()
{
	Mutex::Autolock _l(mLock);
	return mData;
}
