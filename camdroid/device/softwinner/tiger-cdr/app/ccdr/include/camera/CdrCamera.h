#ifndef _CDRCAMERA_H
#define _CDRCAMERA_H

#include <signal.h>
#include <HerbCamera.h>
#include "CdrDisplay.h"

#include "camera.h"
#include "misc.h"
using namespace android;

class CdrCamera : public HerbCamera::PictureCallback,
				  public HerbCamera::AWMoveDetectionListener,
				  public HerbCamera::ErrorCallback
#ifdef APP_ADAS
				  ,public HerbCamera::AdasDetectionListener
#endif
{
public:
	CdrCamera(int cam_type, CDR_RECT &rect, int node);
	~CdrCamera();
	void requestSurface(void);
	int initCamera(unsigned int width, unsigned int height, unsigned int preview_sz_w, unsigned int preview_sz_h);
	int startPreview(void);
	HerbCamera* getCamera(void);
	void stopPreview(void);
	void release();
	CdrDisplay* getDisp(void);
	void setJpegCallback(void(*callback)(void* data, int size, void* caller, int id));
	void onPictureTaken(void *data, int size, HerbCamera* pCamera);
	void setErrorCallback(void(*callback)(int error, void* caller));
	void onError(int error, HerbCamera *pCamera);
	void takePicture(void);
	void setPicResolution(PicResolution_t quality);
	void setPicResolution(int width,int height);
	void setPicQuality(int percent);
	void setAwmdCallBack(void (*callback)(int value, void *caller));
	void onAWMoveDetection(int value, HerbCamera *pCamera);
	void setWhiteBalance(const char *mode);
	void setContrast(int mode);
	void setExposure(int mode);
	void setLightFreq(int mode);
	void setAWMD(bool val);
	void setLWaterMark(bool IsTimeWM ,bool IsLicenseWM ,const char *licenseWM);
	bool isPreviewing();
	void setAwmdTimerCallBack(void (*callback)(sigval_t val));
	void feedAwmdTimer(int seconds);
	int queryAwmdExpirationTime(time_t* sec, long* nsec);
	void setCaller(void* caller);
	int getInputSource();
	int getTVinSystem();
	void setWaterMarkPos(int h);
#ifdef APP_ADAS
	//start for test adas
	void onAdasDetection(void *data, int size, HerbCamera *pCamera);
	//end for test adas
	void setAdasCallback(void(*callback)(void* data,void * caller));
	void setAdasCaller(void *caller);
	void startAdas();
	void stopAdas();
	void setAdas(bool val);
#endif
	void setPictureSizeMode(int mode);
    int  getPictureSizeMode();
	void setCameraFlip(bool flip);
	void setUvcMode(int mode);
	void suspend();
	void resume();
private:
	void startAWMD();
	void stopAWMD();
	void enableWaterMark();
	void enableLicenseWaterMark(bool IsTimeWM ,bool IsLicenseWM ,const char* licenseWM);
	void disableWaterMark();
	HerbCamera		*mHC;
	CdrDisplay		*mCD;
	int mId;
	int mHlay;
	CDR_RECT mRect;
	void (*mJpegCallback)(void* data, int size, void* caller, int id);
	void (*mErrorCallback)(int error, void *caller);
	void (*mAwmdCallback)(int value, void *caller);
	void (*mAwmdTimerCallback)(sigval_t val);

	timer_t mAwmdTimerID;
	bool mNeedAWMD;
	bool mAWMDing;
	bool mADASing;
	Mutex mLock;
	bool mPreviewing;
	void *mCaller;
#ifdef APP_ADAS
	void (*mAdasCallback)(void *data,void *caller);
	bool mNeedADAS;
	void *mAdasCaller;
#endif
	int mPreviewFlip;
	pos_t mWMPos;
	int mNode;
};
#endif
