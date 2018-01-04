
#include "StatusBar.h"
#undef LOG_TAG
#define LOG_TAG "StatusBar.cpp"
#include "debug.h"

using namespace android;

int StatusBarProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
	StatusBar* stb = (StatusBar*)GetWindowAdditionalData(hWnd);
 	switch(message) {
	case MSG_FONTCHANGED:
		stb->updateWindowFont();
		break;
	case MSG_CREATE:
		{
			if(stb->createSTBWidgets(hWnd) < 0) {
				db_error("create sub widgets failed\n");	
			}
		}
		break;
	case MSG_TIMER:
		{
			if(wParam == STB_TIMER_SOS){
				if(STB_SOS>0){
					ResourceManager *rmx = ResourceManager::getInstance();
					const char *text = rmx->getLabel(LANG_LABEL_SOS_WARNING);
					char dis_buf[40];
					sprintf(dis_buf,"%s",text);
					STB_SOS--;
					stb->setLabelRecordTime(dis_buf, WINDOWPIC_RECORDPREVIEW);
				}else{
					stb->setLabelRecordTime("");
					KillTimer(hWnd, STB_TIMER_SOS);
				}
			} else if(wParam == STB_TIMER_SYSTEM_TIME) {
				stb->refreshSystemTime();
			}
		}
		break;
	case STBM_START_RECORD_TIMER:
	case STBM_RESUME_RECORD_TIMER:
	case STBM_STOP_RECORD_TIMER:
	case STBM_GET_RECORD_TIME:
	case STBM_UPDATE_TIME:
		return stb->handleRecordTimerMsg(message, wParam);
		break;
	case STBM_SETLABEL:
		{
			db_msg("STBM_SETLABEL, wParam is %d, lParam is 0x%lx\n", wParam, lParam);
			stb->setStatusBarLabel((enum ResourceID)wParam, (const char*)lParam);
		}
		break;
	case STBM_SETICON:
		{
			db_msg("STBM_SETICON, wParam is %d, lParam is 0x%lx\n", wParam, lParam);
			stb->setStatusBarIcon((enum ResourceID)wParam, lParam);
		}
		break;
	case STBM_SETLABEL1_TEXT:
		/* lParam: msg str */
		{
			if(wParam == 1) {
				db_msg("*88888888888888******lParam=%s ***",(char*)lParam);
				STB_SOS=5;
				SetTimer(hWnd, STB_TIMER_SOS, 100);
				ResourceManager *rmx = ResourceManager::getInstance();
				const char *text = rmx->getLabel(LANG_LABEL_SOS_WARNING);
				char dis_buf[40];
				sprintf(dis_buf,"%s",text);
				stb->setLabelRecordTime(dis_buf, WINDOWPIC_RECORDPREVIEW);
			}else {
				stb->setLabelRecordTime((char*)lParam, WINDOWPIC_PHOTOGRAPH);
			}
		}
		break;
	case STBM_CLEAR_LABEL1_TEXT:
		{
			if (stb->getCurWindowId() != wParam) {
				break;
			}
			stb->setLabelRecordTime("");
		}
		break;
	case STBM_SET_FILELIST:
		stb->setStatusBarLabel(ID_STATUSBAR_LABEL_PLAYBACK_FILE_LIST, (char*)lParam, 1);
		break;
	case MSG_DESTROY:
		break;
	default:
		stb->msgCallback(message, wParam);
	}

	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

static void timer_call(sigval_t sg)
{
	timer_set *data = (timer_set*)sg.sival_ptr;
	int idx = data->idx;

	if(idx == TIMER_RECORD_IDX) {
		StatusBar* winSB = (StatusBar*)(data->context);
		SendMessage(winSB->getHwnd(), STBM_UPDATE_TIME, 0, 0);
		winSB->needStop();
	}
}

//			StatusBar
//			|      |
// statusBarTop   statusBarBottom
StatusBar::StatusBar(CdrMain *cdrMain): 
	mCdrMain(cdrMain),
	mHwndTop(HWND_INVALID),
	mHwndBottom(HWND_INVALID),
	mCountOnce(false),
	mTimer(NULL),
	mFirstShow(true),
	mLabelDisp(true)
{
	RECT rect;
	ResourceManager *rm;

	mCurWindowState.curWindowID = WINDOWID_RECORDPREVIEW;
	mCurWindowState.RPWindowState = STATUS_RECORDPREVIEW;

	rm = ResourceManager::getInstance();
	mIconOps = new CdrIconOps();
	mLabelOps = new CdrLabelOps();

	GetWindowRect(mCdrMain->getHwnd(), &rect);
	
	// create StatusBar Window, tranparent and Fill up the screen
	mHwnd = CreateWindowEx(WINDOW_STATUSBAR, "",
			WS_VISIBLE | WS_CHILD,
			WS_EX_NONE | WS_EX_USEPARENTFONT | WS_EX_TRANSPARENT,
			WINDOWID_STATUSBAR,
			0, 0, RECTW(rect), RECTH(rect),
			mCdrMain->getHwnd(), (DWORD)this);
	if(mHwnd == HWND_INVALID) {
		db_error("create status bar failed\n");
		return;
	}
	rm->setHwnd(WINDOWID_STATUSBAR, mHwnd);

	showSystemTime();
}

StatusBar::~StatusBar()
{
	db_msg("StatusBar Destructor\n");
	if(mIconOps != NULL) {
		delete mIconOps;
		mIconOps = NULL;
	}

	if(mLabelOps != NULL) {
		delete mLabelOps;
		mLabelOps = NULL;
	}
}

// 1. create statusBarTop widget and StatusBarBottom widget
// 2. create icons and labels on the statusBarTop and StatusBarBottom
// 3. set icons' attr or labels' attr
int StatusBar::createSTBWidgets(HWND hParent)
{
	CDR_RECT rectStatusBar;
	RECT rectWindow;
	ResourceManager *rm;
	XCreateParams createParam;
	CdrLabel* label = NULL;
	int retval = 0;

	rm = ResourceManager::getInstance();

	mBgColor = rm->getResColor(ID_STATUSBAR, COLOR_BGC);
	mFgColor = rm->getResColor(ID_STATUSBAR, COLOR_FGC);
	rm->getResRect(ID_STATUSBAR, rectStatusBar);
	GetWindowRect(mCdrMain->getHwnd(), &rectWindow);

	// 1. **************************************************
	// ---------- create statusBarTop ------------
	mHwndTop = CreateWindowEx(CTRL_STATIC, "",
			WS_CHILD,
			WS_EX_NONE | WS_EX_USEPARENTFONT,
			ID_STATUSBAR_TOP,
			0, 0, rectStatusBar.w, rectStatusBar.h,
			hParent, (DWORD)this);
	if(mHwndTop == HWND_INVALID) {
		db_error("create status bar failed\n");
		return -1;
	}
	SetWindowBkColor(mHwndTop, mBgColor);
	// ++++++++++ create statusBarTop ++++++++++++
	
	// ---------- create statusBarBottom ------------
	mHwndBottom = CreateWindowEx(CTRL_STATIC, "",
			WS_CHILD,
			WS_EX_NONE | WS_EX_USEPARENTFONT,
			ID_STATUSBAR_BOTTOM,
			0, RECTH(rectWindow) - rectStatusBar.h, rectStatusBar.w, rectStatusBar.h,
			hParent, (DWORD)this);
	if(mHwndBottom == HWND_INVALID) {
		db_error("create status bar failed\n");
		return -1;
	}
	SetWindowBkColor(mHwndBottom, mBgColor);
	// ++++++++++ create statusBarBottom ++++++++++++

	// 2. **************************************************
	db_msg("statusBarTop icon count :%u\n", TABLESIZE(s_IconSetsTop) );
	createParam.style = SS_CENTERIMAGE;
	createParam.exStyle = WS_EX_TRANSPARENT;
	createParam.hParent = mHwndTop;
	for(unsigned int i = 0; i < TABLESIZE(s_IconSetsTop); i++ ) {
		if( mIconOps->createCdrIcon(&s_IconSetsTop[i], &createParam) < 0)
			return -1;
	}
	db_msg("statusBarBottom icon count :%u\n", TABLESIZE(s_IconSetsBottom) );
	createParam.hParent = mHwndBottom;
	for(unsigned int i = 0; i < TABLESIZE(s_IconSetsBottom); i++ ) {
		if( mIconOps->createCdrIcon(&s_IconSetsBottom[i], &createParam) < 0)
			return -1;
	}
	db_msg("mIconOps size :%zu\n", mIconOps->size());


	db_msg("statusBarTop label count :%u\n", TABLESIZE(s_LabelSetsTop) );
	createParam.style = SS_SIMPLE;
	createParam.exStyle = WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT;
	createParam.hParent = mHwndTop;
	for(unsigned int i = 0; i < TABLESIZE(s_LabelSetsTop); i++) {
		if( mLabelOps->createCdrLabel(&s_LabelSetsTop[i], &createParam) < 0 )
			return -1;
	}
	db_msg("statusBarBottom label count :%u\n", TABLESIZE(s_LabelSetsBottom) );
	createParam.hParent = mHwndBottom;
	for(unsigned int i = 0; i < TABLESIZE(s_LabelSetsBottom); i++) {
		if( mLabelOps->createCdrLabel(&s_LabelSetsBottom[i], &createParam) < 0 )
			return -1;
	}
	db_msg("mLabel size :%zu\n", mLabelOps->size());

	// hide icons not show on the recordPreview window
	mIconOps->hideIcon(ID_STATUSBAR_ICON_PHOTO_RESOLUTION);
	mIconOps->hideIcon(ID_STATUSBAR_ICON_PHOTO_COMPRESSION_QUALITY);
	mIconOps->hideIcon(ID_STATUSBAR_ICON_PLAYBACK_FILE_STATUS);
	mIconOps->hideIcon(ID_STATUSBAR_LABEL_SYSTEM_TIME);
	mIconOps->showIcon(ID_STATUSBAR_ICON_BAT);
	// 2. **************************************************
	// set attributes
	label = mLabelOps->getLabel(ID_STATUSBAR_LABEL1);
	if(label != NULL) {
		label->setAlign(CdrLabel::alLeft);
		db_msg("status bar fgc is 0x%lx\n", Pixel2DWORD(HDC_SCREEN, mFgColor));
		label->setTextColor(mFgColor);
	}

	return 0;
}

int StatusBar::updateRecordTime(bool setLabelBlank)
{
	char timeStr[30] = "";
	int ret = increaseRecordTime();
	if (ret>=0) {
		sprintf(timeStr, "%02d:%02d", (unsigned int)mRecordTime/60, (unsigned int)mRecordTime%60);
	}
	mTimerLock.lock();
	if (mLabelDisp == true) {
		if (getCurWindowId() == WINDOWID_RECORDPREVIEW) {
			setLabelRecordTime(timeStr, WINDOWPIC_RECORDPREVIEW);
			if(mRecordTime % 2 == 0) {
				mIconOps->showIcon(ID_STATUSBAR_ICON_RECORD_HINT);
				//setStatusBarIcon(ID_STATUSBAR_ICON_RECORD_HINT, 1);
			} else {
				mIconOps->hideIcon(ID_STATUSBAR_ICON_RECORD_HINT);
				//setStatusBarIcon(ID_STATUSBAR_ICON_RECORD_HINT, 0);
			}
		}
		ret = 0;
	} else {
		if (getCurWindowId() == WINDOWID_RECORDPREVIEW) {
			setLabelRecordTime("");
			mIconOps->hideIcon(ID_STATUSBAR_ICON_RECORD_HINT);
			//setStatusBarIcon(ID_STATUSBAR_ICON_RECORD_HINT, 0);
		}
		ret = -1;
	}
	mTimerLock.unlock();
	return ret;
}

void StatusBar::setLabelDisp(bool val)
{
	Mutex::Autolock _l(mTimerLock);
	mLabelDisp = val;
}
bool StatusBar::getLabelDisp()
{
	Mutex::Autolock _l(mTimerLock);
	return mLabelDisp;
}
	
int StatusBar::increaseRecordTime()
{
	mRecordTime++;

	if(mRecordTime >= mCurRecordVTL / 1000) {
		if (mCountOnce) {
			SendMessage(mHwnd, STBM_STOP_RECORD_TIMER, 0, 0);
		}
		mRecordTime = 0;
		//if(!mCdrMain->getWakeupState()){
		//	SendMessage(mHwnd,STBM_LOCK,0,0);
		//}
		if(mCurRecordVTL == AFTIMEMS) {
			mCurRecordVTL = mRecordVTL;
			db_msg("reset the mCurRecordVTL to %ld ms\n", mCurRecordVTL);
		//	if(!mCdrMain->getWakeupState())
		//		SendMessage(mHwnd, STBM_LOCK, 0, 0);
		}
	}
	return 0;
}

void StatusBar::msgCallback(int msgType, unsigned int data)
{
	ALOGV("recived msg %x, data is 0x%x\n", msgType, data);
	switch(msgType) {
	case STBM_VIDEO_RESOLUTION:
		setStatusBarIcon(ID_STATUSBAR_ICON_VIDEO_RESOLUTION, data);
		break;
	case STBM_LOOPCOVERAGE:
		setStatusBarIcon(ID_STATUSBAR_ICON_LOOP_COVERAGE, data);
		break;
	case STBM_RECORD_SOUND:
		setStatusBarIcon(ID_STATUSBAR_ICON_RECORD_SOUND, data);
		break;
	case STBM_MOUNT_TFCARD:
		setStatusBarIcon(ID_STATUSBAR_ICON_TFCARD, data);
		break;
	case STBM_UVC:
		setStatusBarIcon(ID_STATUSBAR_ICON_UVC, data);
		break;
	case STBM_WIFI:
		setStatusBarIcon(ID_STATUSBAR_ICON_WIFI, data);
		break;
	case STBM_PHOTO_RESOLUTION:
		setStatusBarIcon(ID_STATUSBAR_ICON_PHOTO_RESOLUTION, data);
		break;
	case STBM_PHOTO_COMPRESSION_QUALITY:
		setStatusBarIcon(ID_STATUSBAR_ICON_PHOTO_COMPRESSION_QUALITY, data);
		break;
	case STBM_GSENSOR:
		setStatusBarIcon(ID_STATUSBAR_ICON_GSENSOR, (data > 0) ? 1 : 0 );
		break;
	case STBM_PARK:
		setStatusBarIcon(ID_STATUSBAR_ICON_PARK, (data > 0) ? 1 : 0 );
		break;
	case STBM_AWMD:
		setStatusBarIcon(ID_STATUSBAR_ICON_AWMD, data);
		break;
	case STBM_LOCK:
		setStatusBarIcon(ID_STATUSBAR_ICON_LOCK, data);
		break;
	case MSG_BATTERY_CHANGED:
		setStatusBarIcon(ID_STATUSBAR_ICON_BAT, data);
		break;
	case MSG_BATTERY_CHARGING:
		setStatusBarIcon(ID_STATUSBAR_ICON_BAT, 6);
		break;
	case STBM_FILE_STATUS:
		setStatusBarIcon(ID_STATUSBAR_ICON_PLAYBACK_FILE_STATUS, data);
		break;
	case STBM_COUNT_ONCE:
		setCountOnce((bool)data);
		break;
	default:
		break;
	}
}

int StatusBar::handleRecordTimerMsg(unsigned int message, unsigned int recordType)
{
	switch(message) {
	case STBM_START_RECORD_TIMER:
		{
			db_msg("recordType is %d, mRecordVTL is %ld ms, mCurRecordVTL is %ld ms\n", recordType, mRecordVTL, mCurRecordVTL);
			if(mRecordVTL == 0) {
				db_msg("start record timer failed, record vtl not initialized\n");
				break;
			}
			if (mTimer == NULL) {
				mTimer = new CdrTimer(this, TIMER_RECORD_IDX);
				mTimer->init(timer_call);
			}
			mTimerLock.lock();
			mRecordTime = 0;
			setLabelRecordTime("00:00", WINDOWPIC_RECORDPREVIEW);
			mLabelOps->showLabel(ID_STATUSBAR_LABEL_RECORD_TIME);
			setStatusBarIcon(ID_STATUSBAR_ICON_RECORD_HINT, 1);
			mTimer->startPeriod(1);
			mLabelDisp = true;
			mTimerLock.unlock();
			if(recordType == 1) {
				/* if recordType is impact record, set the mCurRecordVTL with AFTIMEMS
				 * when time expired, compare the mCurRecordVTL with mRecordVTL, 
				 * if not equal, then reset mCurRecordVTL with mRecordVTL
				 * */
				if(!mCdrMain->getWakeupState()){
					mCurRecordVTL = AFTIMEMS;
					SendMessage(mHwnd, STBM_LOCK, 1, 0);
				}
			}
		}
		break;
	case STBM_STOP_RECORD_TIMER:
		{
			db_msg("recieved STBM_STOP_RECORD!\n");
			mTimerLock.lock();
			mLabelDisp = false;
			setLabelRecordTime("");
			mLabelOps->hideLabel(ID_STATUSBAR_LABEL_RECORD_TIME);
			mTimerLock.unlock();

			setStatusBarIcon(ID_STATUSBAR_ICON_RECORD_HINT, 0);
 			//SendMessage(mHwnd, STBM_LOCK, 0, 0);
		}
		break;
	case STBM_GET_RECORD_TIME:
		return mRecordTime;
		break;
	case STBM_UPDATE_TIME:
		updateRecordTime();
		break;
	}

	return 0;
}

/*
 * set the record video time length, unit(ms)
 * */
void StatusBar::setRecordVTL(unsigned int vtl)
{
	mRecordVTL = vtl;
	mCurRecordVTL = vtl;

}

void StatusBar::setCountOnce(bool value)
{
	mCountOnce = value;
}

void StatusBar::needStop()
{
	mTimerLock.lock();
	if (!mLabelDisp) {
		mTimer->stop();
	}
	mTimerLock.unlock();
}

void StatusBar::setStatusBarLabel(ResourceID id, const char* value)
{
	CdrLabel* label = NULL;

	if(mLabelOps != NULL) {
		label = mLabelOps->getLabel(id);
	}
	if(label != NULL) {
		label->setText( value );
	}
}

void StatusBar::setStatusBarLabel(ResourceID id, const char* value, unsigned int windx)
{
	CdrLabel* label = NULL;

	if(mLabelOps != NULL) {
		label = mLabelOps->getLabel(id);
	}
	if (windx){
		label->setAlign(CdrLabel::alLeft);
	}else{
		label->setAlign(CdrLabel::alCenter);
	}
	if(label != NULL) {
		label->setText( value );
	}
}
void StatusBar::setStatusBarIcon(ResourceID id, unsigned int index)
{
	if(mIconOps != NULL) {
		mIconOps->setIconIndex(id, index);
	}
}

void StatusBar::setWindowPic(unsigned int windowPicId)
{
	switch(windowPicId) {
	case WINDOWPIC_RECORDPREVIEW:
		setStatusBarIcon(ID_STATUSBAR_ICON_WINDOWPIC, 0);
		break;
	case WINDOWPIC_PHOTOGRAPH:
		setStatusBarIcon(ID_STATUSBAR_ICON_WINDOWPIC, 1);
		break;
	case WINDOWPIC_PLAYBACKPREVIEW:
		setStatusBarIcon(ID_STATUSBAR_ICON_WINDOWPIC, 2);
		break;
	case WINDOWPIC_MENU:
		setStatusBarIcon(ID_STATUSBAR_ICON_WINDOWPIC, 3);
		break;
	}
}

void StatusBar::setLabelRecordTime(const char* str)
{
	setStatusBarLabel(ID_STATUSBAR_LABEL_RECORD_TIME, str);
}

void StatusBar::setLabelRecordTime(const char* str, unsigned int winid)
{
	setStatusBarLabel(ID_STATUSBAR_LABEL_RECORD_TIME, str, winid);
}

void StatusBar::refreshSystemTime(void)
{
	struct tm* localTime;
	char timeStr[30] = {0};

	getDateTime(&localTime);

	if(mFirstShow){
		mFirstShow=false;
		return ;
	}else{
		sprintf(timeStr, "%04d-%02d-%02d  %02d:%02d:%02d",
			localTime->tm_year + 1900, localTime->tm_mon + 1, localTime->tm_mday,
			localTime->tm_hour, localTime->tm_min, localTime->tm_sec);
	}

	setStatusBarLabel(ID_STATUSBAR_LABEL_SYSTEM_TIME, timeStr, 1);
}

void StatusBar::showSystemTime(void)
{
	if( !IsTimerInstalled(mHwnd, STB_TIMER_SYSTEM_TIME) )
		SetTimer(mHwnd, STB_TIMER_SYSTEM_TIME, 100);
	mLabelOps->showLabel(ID_STATUSBAR_LABEL_SYSTEM_TIME);
}

void StatusBar::hideSystemTime(void)
{
	if(IsTimerInstalled(mHwnd, STB_TIMER_SYSTEM_TIME))
		KillTimer(mHwnd, STB_TIMER_SYSTEM_TIME);
	setStatusBarLabel(ID_STATUSBAR_LABEL_SYSTEM_TIME, "", 1);
	mLabelOps->hideLabel(ID_STATUSBAR_LABEL_SYSTEM_TIME);
}

void StatusBar::setCurrentWindowState(windowState_t windowState)
{
	db_msg("setCurrentWindowState, curWindowID is %d\n", windowState.curWindowID);
	if(windowState.curWindowID < WINDOWID_INVALID && windowState.RPWindowState < RPWINDOWSTATE_INVALID) {
		mCurWindowState = windowState;
		updateWindowPic();
	}
}

void StatusBar::setCurrentRPWindowState(RPWindowState_t status)
{
	db_msg("setRPwindowState, state is %d\n", status);
	if(status < RPWINDOWSTATE_INVALID) {
		mCurWindowState.RPWindowState = status;
		updateWindowPic();
	}
}

RPWindowState_t StatusBar::getCurrentRPWindowState(void)
{
	return mCurWindowState.RPWindowState;
}

void StatusBar::setCurWindowId(unsigned int windowId)
{
	db_msg("setCurWindowId, curWindowID is %d\n", windowId);
	if(windowId < WINDOWID_INVALID) {
		mCurWindowState.curWindowID = windowId;
		updateWindowPic();
	}
}

int StatusBar::getCurRecordVTL()
{
    return (int)mRecordTime;
}

int StatusBar::getRecordVTL()
{
    return (int)(mRecordVTL/1000);
}

unsigned int StatusBar::getCurWindowId()
{
	return mCurWindowState.curWindowID;
}

// 1. show and hide statusBarTop or statusBarBottom
// 2. turn on or turn off the alpha element of the statusBar back color
// 3. show or hide icons in different window
void StatusBar::updateWindowPic(void)
{
	ResourceManager *rm;
	RPWindowState_t state;
	unsigned int windowId;

	rm = ResourceManager::getInstance();
	windowId = getCurWindowId();
	state = mCurWindowState.RPWindowState;
	// 1. *************************************************
	/*
	 * in RecordPreview show statusBarBottom
	 * outside RecordPreview hide statusBarBottom
	 * */
	if(windowId == WINDOWID_RECORDPREVIEW) {
		if( ! IsWindowVisible(mHwndBottom) ) {
			db_msg("show statusBarBottom");
				ShowWindow(mHwndBottom, SW_SHOWNORMAL);
		}
	} else {
		if( IsWindowVisible(mHwndBottom) ) {
			db_msg("hide statusBarBottom");
			ShowWindow(mHwndBottom, SW_HIDE);
		}
	}
	/*
	 * in Playback and Menu hide the statusBarTop
	 * outside Playback and Menu show the statusBarTop
	 * */ 
	if(windowId == WINDOWID_PLAYBACK || windowId == WINDOWID_MENU) {
		if( IsWindowVisible(mHwndTop) ) {
			db_msg("hide statusBarTop");
			ShowWindow(mHwndTop, SW_HIDE);
		}
	} else {
		if( ! IsWindowVisible(mHwndTop) ) {
			db_msg("show statusBarTop");
			ShowWindow(mHwndTop, SW_SHOWNORMAL);
		}
	}

	// 2. *************************************************
	changeStatusBarAlpha();

	// 3. *************************************************
	switch( windowId ) {
	case WINDOWID_RECORDPREVIEW:
		{
			hidePlaybackPreviewIcons();
			if(state == STATUS_PHOTOGRAPH) {
				setWindowPic(WINDOWPIC_PHOTOGRAPH); 

				hideRecordPreviewModeIcons();
				showPhotoModeIcons();
			} else {
				setWindowPic(WINDOWPIC_RECORDPREVIEW); 

				hidePhotoModeIcons();
				showRecordPreviewModeIcons();
			}
			showPeripheralAreaIcons();
		}
		break;
	case WINDOWID_PLAYBACKPREVIEW:
		{
			setWindowPic(WINDOWPIC_PLAYBACKPREVIEW);

			hideRecordPreviewModeIcons();
			hidePhotoModeIcons();
			hidePeripheralAreaIcons();

			showPlaybackPreviewIcons();
		}
		break;
	case WINDOWID_MENU:
		{
			setWindowPic(WINDOWPIC_MENU); 

			hideRecordPreviewModeIcons();
			hidePhotoModeIcons();
			hidePlaybackPreviewIcons();
			mIconOps->hideIcon(ID_STATUSBAR_ICON_WINDOWPIC);

			showPeripheralAreaIcons();
		}
		break;
	default:
		break;
	}

}

// --------------------------------------------------------------------
// in RecordPreview window statusBar may have alpha value
// outside RecordPreview window statusBar should turn off alpha value
void StatusBar::changeStatusBarAlpha(void)
{
	//db_msg("mBgColor DWORD is 0x%lx\n", Pixel2DWORD(HDC_SCREEN, mBgColor));
	if( GetAValue(Pixel2DWORD(HDC_SCREEN, mBgColor)) != 0xFF ) {
		// check if the backgroud color have alpha value
		gal_pixel colorTop, colorBottom, newColor;
		DWORD dwordTop, dwordBottom;
		colorTop = GetWindowBkColor(mHwndTop);
		dwordTop = Pixel2DWORD(HDC_SCREEN, colorTop);
		colorBottom = GetWindowBkColor(mHwndBottom);
		dwordBottom = Pixel2DWORD(HDC_SCREEN, colorBottom);
		//db_msg("dwordTop is 0x%lx\n", dwordTop);
		//db_msg("dwordBottom is 0x%lx\n", dwordBottom);
		if(getCurWindowId() == WINDOWID_RECORDPREVIEW) {
			// turn on statusBar alpha value
			if(GetAValue(dwordTop) == 0xFF) {
				SetWindowBkColor(mHwndTop, mBgColor);
				InvalidateRect(mHwndTop, NULL, TRUE);
			}
			if(GetAValue(dwordBottom) == 0xFF) {
				SetWindowBkColor(mHwndBottom, mBgColor);
				InvalidateRect(mHwndBottom, NULL, TRUE);
			}
		} else {
			// turn off statusBar alpha value
			//db_msg("dwordTop A Value is 0x%x\n", GetAValue(dwordTop));
			//db_msg("dwordBottom A Value is 0x%x\n", GetAValue(dwordBottom));
			if(GetAValue(dwordTop) < 0xFF) {
				newColor = RGBA2Pixel(HDC_SCREEN, GetRValue(dwordTop), GetGValue(dwordTop), GetBValue(dwordTop), 0xFF);
				SetWindowBkColor(mHwndTop, newColor);
				//db_msg("newColor DWORD is 0x%lx\n", Pixel2DWORD(HDC_SCREEN, newColor) );
				InvalidateRect(mHwndTop, NULL, TRUE);
			}
			if(GetAValue(dwordBottom) < 0xFF) {
				newColor = RGBA2Pixel(HDC_SCREEN, GetRValue(dwordBottom), GetGValue(dwordBottom), GetBValue(dwordBottom), 0xFF);
				SetWindowBkColor(mHwndBottom, newColor);
				//db_msg("newColor DWORD is 0x%lx\n", Pixel2DWORD(HDC_SCREEN, newColor) );
				InvalidateRect(mHwndBottom, NULL, TRUE);
			}
		}
	}
}

void StatusBar::showRecordPreviewModeIcons(void)
{
	// show icons in recordPreview status
	// area 1
	mIconOps->showIcon(ID_STATUSBAR_ICON_WINDOWPIC);
	mIconOps->showIcon(ID_STATUSBAR_ICON_VIDEO_RESOLUTION);
	mIconOps->showIcon(ID_STATUSBAR_ICON_LOOP_COVERAGE);
	mIconOps->showIcon(ID_STATUSBAR_ICON_RECORD_SOUND);

	// area 3
	mIconOps->showIcon(ID_STATUSBAR_ICON_GSENSOR);
	mIconOps->showIcon(ID_STATUSBAR_ICON_PARK);
	mIconOps->showIcon(ID_STATUSBAR_ICON_AWMD);
	mIconOps->showIcon(ID_STATUSBAR_ICON_UVC);

	mIconOps->showIcon(ID_STATUSBAR_ICON_LOCK);
}

void StatusBar::hideRecordPreviewModeIcons(void)
{
	// hide icons in recordPreview status
	// area 1
	mIconOps->hideIcon(ID_STATUSBAR_ICON_VIDEO_RESOLUTION);
	mIconOps->hideIcon(ID_STATUSBAR_ICON_LOOP_COVERAGE);
	mIconOps->hideIcon(ID_STATUSBAR_ICON_RECORD_SOUND);

	// area 3
	mIconOps->hideIcon(ID_STATUSBAR_ICON_GSENSOR);
	mIconOps->hideIcon(ID_STATUSBAR_ICON_PARK);
	mIconOps->hideIcon(ID_STATUSBAR_ICON_AWMD);
	mIconOps->hideIcon(ID_STATUSBAR_ICON_UVC);

	mIconOps->hideIcon(ID_STATUSBAR_ICON_LOCK);
}

void StatusBar::showPhotoModeIcons(void)
{
	// show icons in photoGraph status
	// area 1
	mIconOps->showIcon(ID_STATUSBAR_ICON_WINDOWPIC);
	mIconOps->showIcon(ID_STATUSBAR_ICON_PHOTO_RESOLUTION);
	mIconOps->showIcon(ID_STATUSBAR_ICON_PHOTO_COMPRESSION_QUALITY);
	mLabelOps->showLabel(ID_STATUSBAR_LABEL_RECORD_TIME);
	mIconOps->showIcon(ID_STATUSBAR_ICON_UVC);

}

void StatusBar::hidePhotoModeIcons(void)
{
	// hide icons in photoGraph status
	// area 1
	mIconOps->hideIcon(ID_STATUSBAR_ICON_PHOTO_RESOLUTION);
	mIconOps->hideIcon(ID_STATUSBAR_ICON_PHOTO_COMPRESSION_QUALITY);
	mIconOps->hideIcon(ID_STATUSBAR_ICON_UVC);
}

void StatusBar::showPlaybackPreviewIcons(void)
{
	// show icons int playbackPreview window

	// area 1
	mIconOps->showIcon(ID_STATUSBAR_ICON_WINDOWPIC);
	mIconOps->showIcon(ID_STATUSBAR_ICON_PLAYBACK_FILE_STATUS);

	mLabelOps->showLabel(ID_STATUSBAR_LABEL_PLAYBACK_FILE_LIST);
}

void StatusBar::hidePlaybackPreviewIcons(void)
{
	// hide icons int playbackPreview window

	// area 1
	mIconOps->hideIcon(ID_STATUSBAR_ICON_PLAYBACK_FILE_STATUS);
	mLabelOps->hideLabel(ID_STATUSBAR_LABEL_PLAYBACK_FILE_LIST);
}

void StatusBar::showPeripheralAreaIcons(void)
{
	mIconOps->showIcon(ID_STATUSBAR_ICON_TFCARD);
}

void StatusBar::hidePeripheralAreaIcons(void)
{
	mIconOps->hideIcon(ID_STATUSBAR_ICON_TFCARD);
}

int StatusBar::releaseResource()
{
	return 0;
}