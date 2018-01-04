#include "CdrCamera.h"
#include "posixTimer.h"
#undef LOG_TAG
#define LOG_TAG "CdrCamera.cpp"
#include "debug.h"
//#define SENSOR_DEBUG
CdrCamera::CdrCamera(int cam_type, CDR_RECT &rect, int node)
	: mHC(NULL),
	mCD(NULL),
	mId(cam_type),
	mRect(rect),
	mJpegCallback(NULL),
	mAwmdCallback(NULL),
	mAwmdTimerCallback(NULL),
	mAwmdTimerID(0),
	mNode(0)
{
	ViewInfo sur;
	sur.x = mRect.x;
	sur.y = mRect.y;
	sur.w = mRect.w;
	sur.h = mRect.h;
//	db_error("open camera%d\n",  node);
	mHC = HerbCamera::open(node);
	mNode = node;
	if(mHC == NULL) {
		db_error("fail to open HerbCamera");
		return;
	}
	mCD = new CdrDisplay(cam_type, &sur);
	if(mCD == NULL) {
		db_error("fail to new CedarDisplay");
		return;
	}

	mHlay = mCD->getHandle();
//	db_msg("camera %d, hlay is %d\n", cam_type, mHlay);

	mNeedAWMD = false;
	mAWMDing = false;
	mPreviewing = false;
	mADASing = false;
	mNeedADAS = false;
	mPreviewFlip = HerbCamera::Parameters::PREVIEW_FLIP::NoFlip;
	mWMPos.x = 0;
	mWMPos.y = 0;
}

CdrCamera::~CdrCamera()
{
	db_msg("CdrCamera Destructor\n");
	if (mCD) {
		delete mCD;
		mCD = NULL;
	}
	if (mHC) {
		mHC->stopPreview();
		release();
		delete mHC;
		mHC = NULL;
	}
	if(mAwmdTimerID) {
		stopTimer(mAwmdTimerID);
		deleteTimer(mAwmdTimerID);
		mAwmdTimerID = 0;
	}
}

HerbCamera* CdrCamera::getCamera(void)
{
	return mHC;
}

int CdrCamera::startPreview(void)
{
	Mutex::Autolock _l(mLock);

	db_msg("startPreview %d\n", mId);
	if(mPreviewing == true) {
		db_msg("preview is started\n");
		return -1;
	}
	mHC->startPreview();
	mPreviewing = true;
	if (mNeedAWMD) {
		startAWMD();
	}
	if (mNeedADAS) {
		startAdas();
	}
	return 0;
}

void CdrCamera::stopPreview(void)
{
	Mutex::Autolock _l(mLock);

	db_msg("stopPreview %d\n", mId);
	if(mPreviewing == false) {
		db_msg("preview is already stoped\n");
		return;
	}
	mHC->stopPreview();
	mPreviewing = false;
	db_msg("stopPreview\n");

	stopAWMD();
	disableWaterMark();
	stopAdas();
}

void CdrCamera::setWaterMarkPos(int h)
{
	if (h >= 1000) {
		mWMPos.x = 800;
		mWMPos.y = 600;
	} else if ( h >= 700) {
		mWMPos.x = 720;
		mWMPos.y = 550;
	} else  if (h >= 400) {
		mWMPos.x = 100;
		mWMPos.y = 350;
	} else {
		mWMPos.x = 50;
		mWMPos.y = 100;
	}
}

void CdrCamera::enableWaterMark()
{
	char waterMark[100]= {0};
	if (mHC) {
		sprintf(waterMark ,"%d,%d,1",mWMPos.x ,mWMPos.y);
		mHC->setWaterMark(1, waterMark);
	}	
}

void CdrCamera::enableLicenseWaterMark(bool IsTimeWM ,bool IsLicenseWM ,const char* licenseWM)
{
	char waterMark[100]= {0};
	int licenseWM_x = mWMPos.x + 180;
	int licenseWM_y = mWMPos.y + 50;

	sprintf(waterMark ,"%d,%d,%d,%d,%d,%s",mWMPos.x , mWMPos.y , IsTimeWM , licenseWM_x , licenseWM_y ,licenseWM);
	if (mHC) {
		db_msg(" waterMark :%s", waterMark);
		mHC->setWaterMark(1, waterMark);
	}
}

void CdrCamera::disableWaterMark()
{
	if (mHC) {
		mHC->setWaterMark(0, NULL);
	}
}

void CdrCamera::setWhiteBalance(const char *mode)
{
	if (mId != CAM_CSI) {
		return ;
	}
	db_error("setWhiteBalance %s", mode);
	HerbCamera::Parameters params;

	mHC->getParameters(&params);
	//params.dump();

	params.setWhiteBalance(mode);
#ifndef SENSOR_DEBUG
	mHC->setParameters(&params);
#endif
}

void CdrCamera::setContrast(int mode)
{
	db_error("setContrast");
	HerbCamera::Parameters params;

	mHC->getParameters(&params);
	//params.dump();
	mode = 25*mode;
	params.setContrastValue(mode);
#ifndef SENSOR_DEBUG
	mHC->setParameters(&params);
#endif
}

void CdrCamera::setExposure(int mode)
{
	db_error("setExposure");
	HerbCamera::Parameters params;

	mHC->getParameters(&params);
	//params.dump();

	params.setExposureCompensation(mode);
#ifndef SENSOR_DEBUG
	mHC->setParameters(&params);
#endif
}

void CdrCamera::setLightFreq(int mode)
{
	db_error("setLightFreq mode :%d", mode);
	HerbCamera::Parameters params;
	switch(mode) {
		case 0:
			mode = HerbCamera::Parameters::PowerLineFrequency::PLF_50HZ;
			break;
		case 1:
			mode = HerbCamera::Parameters::PowerLineFrequency::PLF_60HZ;
			break;
		default:
			mode = HerbCamera::Parameters::PowerLineFrequency::PLF_AUTO;
			break;
	}
	mHC->getParameters(&params);
	//params.dump();
	params.setPowerLineFrequencyValue(mode);
#ifndef SENSOR_DEBUG
	mHC->setParameters(&params);
#endif
}


CdrDisplay* CdrCamera::getDisp(void)
{
	return mCD;
}

void CdrCamera::onPictureTaken(void *data, int size, HerbCamera* pCamera)
{
	if(mJpegCallback) {
		(*mJpegCallback)(data, size, mCaller, mId);
	}
}

void CdrCamera::onError(int error, HerbCamera *pCamera)
{
	if(mErrorCallback) {
		(*mErrorCallback)(error, mCaller);
	}
}

void CdrCamera::setErrorCallback(void(*callback)(int error, void* caller))
{
	mErrorCallback = callback;
}

void CdrCamera::setJpegCallback(void(*callback)(void* data, int size, void* caller, int id))
{
	mJpegCallback = callback;
}

void CdrCamera::takePicture(void)
{
	if(mHC) {
		mHC->takePicture(NULL, NULL, NULL, this);
	}
}

#ifdef APP_ADAS
void CdrCamera::setAdasCallback(void(*callback)(void* data,void * caller))
{
	mAdasCallback = callback;
}
#endif

void CdrCamera::setPicResolution(PicResolution_t quality)
{
	HerbCamera::Parameters params;
	unsigned int w = 0;
	unsigned int h = 0;
	if(mHC == NULL) {
		db_error("mHC is NULL\n");
		return;
	}
	mHC->getParameters(&params);
	switch(quality) {
	case PicResolution2M:
		{
			w = 1600;
			h = 1200;
		}
		break;
	case PicResolution5M:
		{
			w = 3200;
			h = 1800;
		}
		break;
	case PicResolution8M:
		{
			w = 3840;
			h = 2160;
		}
		break;
	case PicResolution12M:
		{
			w = 4000;
			h = 3000;
		}
		break;
	default:
		{
			db_msg("not support PicResolution:%d\n", quality);
			return ;
		}
		break;
	}
	#ifdef BACK_PICTURE_SOURCE_MODE
		if (mId == CAM_UVC) {
			w = REAL_UVC_W;
			h = REAL_UVC_H;
		}
	#endif
	params.setPictureSize(w, h);
	mHC->setParameters(&params);
}

void CdrCamera::setPicResolution(int width,int height)
{
	HerbCamera::Parameters params;
	
	if(mHC == NULL) {
		db_error("mHC is NULL\n");
		return;
	}
	mHC->getParameters(&params);
	
	params.setPictureSize(width,height);
	mHC->setParameters(&params);
}
void CdrCamera::setPicQuality(int percent)
{
	db_msg("setPicQuality percent:%d", percent);
	HerbCamera::Parameters params;
	if(mHC == NULL) {
		db_error("mHC is NULL\n");
		return;
	}
	mHC->getParameters(&params);
	params.setJpegQuality(percent);
	mHC->setParameters(&params);
}

void CdrCamera::setPictureSizeMode(int mode)
{ 
	HerbCamera::Parameters params;
	mHC->getParameters(&params);
	params.setPictureSizeMode(mode);
	mHC->setParameters(&params);

}
int CdrCamera::getPictureSizeMode()
{ 
	HerbCamera::Parameters params;
	mHC->getParameters(&params);
	return params.getPictureSizeMode();

}

void CdrCamera::setCameraFlip(bool flip)
{
	db_msg(" flip=%d",flip);
	if (flip) {
		mPreviewFlip = HerbCamera::Parameters::PREVIEW_FLIP::LeftRightFlip;
	} else {
		mPreviewFlip = HerbCamera::Parameters::PREVIEW_FLIP::NoFlip;
	}
	if (mHC) {
		HerbCamera::Parameters params;
		mHC->getParameters(&params);
		params.setPreviewFlip(mPreviewFlip);
		mHC->setParameters(&params);
	}
}

int CdrCamera::initCamera(unsigned int width, unsigned int height, unsigned int preview_sz_w, unsigned int preview_sz_h)
{
	HerbCamera::Parameters params;
	v4l2_queryctrl ctrl;
	if (!mHC) {
		return -1;
	}
	mHC->getParameters(&params);
	//params.dump();
	params.setPreviewSize(preview_sz_w, preview_sz_h); //isp small size
	params.setVideoSize(width, height);	//real sensor size
	ALOGD("(f:%s, l:%d) setPreviewSize[%dx%d], VideoSize[%dx%d]", __FUNCTION__, __LINE__, preview_sz_w, preview_sz_h, width, height);
	params.setPreviewFrameRate(FRONT_CAM_FRAMERATE);
	params.setPreviewFlip(mPreviewFlip);
    params.setPictureMode(CameraParameters::AWEXTEND_PICTURE_MODE_FAST);
    params.setPictureSizeMode(CameraParameters::AWEXTEND_PICTURE_SIZE_MODE::UseParameterPictureSize);
#ifndef SENSOR_DEBUG
	if(mId == CAM_CSI) {
		params.setContrastValue(ctrl.default_value);
		params.setBrightnessValue(ctrl.default_value);
		params.setSaturationValue(ctrl.default_value);
		params.setHueValue(ctrl.default_value);
	}
#endif
	mHC->setParameters(&params);
	mHC->setPreviewDisplay(mHlay);
	mCD->setBottom();
	mHC->setErrorCallback(this);
	return 0;
}

void CdrCamera::startAWMD()
{
	if(!mAwmdTimerID) {
		if(mAwmdTimerCallback == NULL) {
			db_error("%s %d, awmd timer call back not set\n", __FUNCTION__, __LINE__);
			return;
		}
		if(createTimer(mCaller, &mAwmdTimerID, mAwmdTimerCallback) < 0) {
			db_error("%s %d, create timer failed\n", __FUNCTION__, __LINE__);
			return;	
		}
	}

	db_msg("%s %d\n", __FUNCTION__, __LINE__);
	if (mHC && (mAWMDing == false)) {
		db_msg("%s %d\n", __FUNCTION__, __LINE__);
		mHC->setAWMoveDetectionListener(this);
		mHC->startAWMoveDetection();
		mHC->setAWMDSensitivityLevel(1);
		
		mAWMDing = true;
	}
	
}

void CdrCamera::stopAWMD()
{
	db_msg("%s %d\n", __FUNCTION__, __LINE__);
	if (mHC && (mAWMDing)) {
		db_msg("%s %d\n", __FUNCTION__, __LINE__);
		mHC->stopAWMoveDetection();
		if(mAwmdTimerID) {
			stopTimer(mAwmdTimerID);
			deleteTimer(mAwmdTimerID);
			mAwmdTimerID = 0;
		}
		mAWMDing = false;
	}
}

void CdrCamera::startAdas()
{
	db_msg(" mADASing %d ", mADASing);
	if (mHC&& (mADASing == false)) {
		mHC->setAdasDetectionListener(this);
		mHC->adasStartDetection();
		mADASing = true;
	}
}


void CdrCamera::stopAdas()
{
	db_msg(" mADASing %d ", mADASing);
	if (mHC && mADASing) {
		mHC->adasStopDetection();
		mADASing = false;
	}
}

void CdrCamera::setAdas(bool val)
{
	if (mId != CAM_CSI) {
		return ;
	}
	mNeedADAS = val;
	db_msg("%s %d, val is %d\n", __FUNCTION__, __LINE__, val);
	if (mHC && (mPreviewing == true)) {
		db_msg("%s %d\n", __FUNCTION__, __LINE__);
		if (val) {
			startAdas();
		} else {
			stopAdas();
		}
	}
}


void CdrCamera::setAWMD(bool val)
{
	Mutex::Autolock _l(mLock);
	if (mId != CAM_CSI) {
		return ;
	}
	mNeedAWMD = val;
	if (mHC && (mPreviewing == true)) {
		if (val) {
			startAWMD();
		} else {
			stopAWMD();
		}
	}
}

#ifdef APP_ADAS
// for testing adas
void CdrCamera::onAdasDetection(void *data, int size, HerbCamera *pCamera){
	//for testing adas

	
	if(mAdasCallback){
	   (*mAdasCallback)(data,mAdasCaller);
	}
	
	
}

void CdrCamera::setAdasCaller(void *caller)
{
	mAdasCaller = caller;
}

#endif

void CdrCamera::setLWaterMark(bool IsTimeWM ,bool IsLicenseWM , const char *licenseWM )
{
	Mutex::Autolock _l(mLock);
	if (mHC && (mPreviewing == true)){
		if (IsLicenseWM) {
			enableLicenseWaterMark(IsTimeWM ,IsLicenseWM ,licenseWM);
		} else if (IsTimeWM) { //T
			enableWaterMark();
		} else {
			disableWaterMark();
		}
	}
}

void CdrCamera::release()
{
	mHC->release();
}

void CdrCamera::setAwmdCallBack(void (*callback)(int value, void *caller))
{
	mAwmdCallback = callback;
}

void CdrCamera::onAWMoveDetection(int value, HerbCamera *pCamera)
{
	//	db_msg("%s %d\n", __FUNCTION__, __LINE__);
	if(mAwmdCallback) {
		(*mAwmdCallback)(value, mCaller);
	}
}

bool CdrCamera::isPreviewing(void)
{
	Mutex::Autolock _l(mLock);
	return mPreviewing;
}

void CdrCamera::setAwmdTimerCallBack(void (*callback)(sigval_t val))
{
	mAwmdTimerCallback = callback;
}

void CdrCamera::feedAwmdTimer(int seconds)
{
	if(mAwmdTimerID)	
		setOneShotTimer(seconds, 0, mAwmdTimerID);
}

int CdrCamera::queryAwmdExpirationTime(time_t* sec, long* nsec)
{
	if(mAwmdTimerID) {
		if(getExpirationTime(sec, nsec, mAwmdTimerID) < 0) {
			db_error("%s %d, get awmd timer expiration time failed\n", __FUNCTION__, __LINE__);
			return -1;
		}
	} else
		return -1;

	return 0;
}

void CdrCamera::setCaller(void* caller)
{
	mCaller = caller;
}

int CdrCamera::getInputSource()
{
	int val;
	mHC->getInputSource(&val);
	return val;
}
int CdrCamera::getTVinSystem()
{
	int val;
	mHC->getTVinSystem(&val);
	return val;
}

void CdrCamera::setUvcMode(int mode)
{
	mHC->setUvcGadgetMode(mode);
}

void CdrCamera::suspend()
{
	release();
}

void CdrCamera::resume()
{
	mHC = HerbCamera::open(mNode);
}
