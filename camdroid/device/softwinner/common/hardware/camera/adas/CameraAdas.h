#ifndef __CAMERA_ADAS_H__
#define __CAMERA_ADAS_H__

#include <Adas_interface.h>
#include "../PhotographAlgorithmBase.h"

#include "AdasCars.h"



//#define ADAS_WAIT_CAP_FRAME

namespace android {

class CameraAdas : public PhotographAlgorithmBase
{
public:
    CameraAdas(CallbackNotifier *cb, void *dev);
    ~CameraAdas();

	status_t initialize(int width, int height, int thumbWidth, int thumbHeight, int fps);
	status_t destroy();
	status_t camDataReady(V4L2BUF_t *v4l2_buf);
	status_t enable(bool enable) {return NO_ERROR;}
    bool thread() {return true;}

	status_t getCurrentFrame(FRAMEIN &imgIn);
	status_t writeData(SAVEPARA& para);
	status_t readData(ADASIN& adasin);
	inline CallbackNotifier *getCallbackNotifier(void) { return mCallbackNotifier; }
	inline void *getV4L2CameraDevice() { return mV4L2CameraDevice; }
	inline int getPreviewWidth() { return mSmallImgWidth; }
	inline int getPreviewHeight() { return mSmallImgHeight; }
	inline bool getStatus() { return mStarted; }
    status_t setLaneLineOffsetSensity(int level);
    status_t setDistanceDetectLevel(int level);
	status_t adasSetMode(int mode);
	status_t adasSetTipsMode(int mode);
    status_t setCarSpeed(float speed);
	void setAdMemeryOx();
	void setAdasIfData(ADASOUT_IF *data);
	int isAdasWarn(ADASOUT &adas_out);
	inline void adasSignal() { mAdasCon.signal(); }
	inline void adasLock() {mAdasLock.lock();}
	inline void adasUnlock() {mAdasLock.unlock();}
	inline void adasWait() {mAdasCon.wait(mAdasLock);}
	void adasUpdateScreen();
	inline bool getIsDrawNow() { return mIsDrawNow; }
	inline void setIsDrawNow(bool flag) { mIsDrawNow = flag; }
	static void setAdasAudioVol(float val);
	status_t adasSetGsensorData(int val0, float val1, float val2);
	int adasCount;

class DrawAdasThread: public Thread{
	public:
		DrawAdasThread(CameraAdas *mre)
			: Thread(false),
			  mRP(mre)
		{
			mRequestExit = false;
		}
		~DrawAdasThread(){};
		virtual bool threadLoop() {
			mRP->adasLock();
			mRP->adasWait();
			if (mRequestExit) {
				return false;
			}
			mRP->setIsDrawNow(false);
			mRP->adasUpdateScreen();
			mRP->adasUnlock();
			mRP->setIsDrawNow(true);
			return true;
		}
		void stopThread() {
			mRequestExit = true;
        }
		status_t start() {
			return run("DeviceDrawAdasThread", PRIORITY_NORMAL);
		}
	private:
		CameraAdas *mRP;
		bool mRequestExit;
	
};



private:
    status_t getCurrentFrame(V4L2BUF_t *v4l2_buf) {return NO_ERROR;}

	CallbackNotifier *mCallbackNotifier;
    void *mV4L2CameraDevice;
	V4L2BUF_t *mV4l2Buf;
#ifdef ADAS_WAIT_CAP_FRAME
	pthread_mutex_t mMutex;
	pthread_cond_t mCond;
#else
    pthread_mutex_t mlock;
#endif
	volatile bool mStarted;
	int mBigImgWidth;
	int mBigImgHeight;
	int mSmallImgWidth;
	int mSmallImgHeight;

    int mLaneLineOffsetSensity;
    int mDistanceDetectLevel;
    int mCarSpeed;
	AdasCars *mAdasCars;
	Mutex mAdasLock;
	Condition mAdasCon;
	ADASOUT_IF madasdata;
	sp<DrawAdasThread> mAdasTh;
	bool mIsDrawNow;
	int isStop;
	float aSpeed;
	float vSpeed;
	int Adasmode;
};

}; /* namespace android */

#endif

