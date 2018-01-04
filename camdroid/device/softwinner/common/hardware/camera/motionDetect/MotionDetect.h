#ifndef __MOTION_DETECT_H__
#define __MOTION_DETECT_H__

#include <include_awmd/AWMD_Interface.h>
#include "PhotographAlgorithmBase.h"


namespace android {

class CallbackNotifier;

class MotionDetect : public PhotographAlgorithmBase
{
public:
    MotionDetect(CallbackNotifier *cb);
    ~MotionDetect();
    bool thread();
    status_t camDataReady(V4L2BUF_t *v4l2_buf);
    status_t initialize(int width, int height, int thumbWidth, int thumbHeight, int fps);
    status_t destroy();
    status_t enable(bool enable);

    status_t setSensitivityLevel(int level);
    int getWidth();
    int getHeight();
    

protected:
    class DoAWMDThread : public Thread
    {
        MotionDetect*   mAWMDDevice;
        bool                mRequestExit;
    public:
        DoAWMDThread(MotionDetect* dev)
            : Thread(false)
            , mAWMDDevice(dev)
            , mRequestExit(false)
        {
        }
        void startThread()
        {
            run("AWMDThread", PRIORITY_URGENT_DISPLAY);
        }
        void stopThread()
        {
            mRequestExit = true;
        }
        virtual bool threadLoop()
        {
            if (mRequestExit) {
                return false;
            }
            return mAWMDDevice->thread();
        }
    };

private:
    status_t getCurrentFrame(V4L2BUF_t *v4l2_buf);

    CallbackNotifier *mCallbackNotifier;
    sp<DoAWMDThread> mThread;
    pthread_mutex_t mMutex;
    pthread_mutex_t mLock;
    pthread_cond_t mCond;
    bool mWaitImgData;
    bool mEnable;
    unsigned char *mBuf;
    AWMD *mAWMD;
    AWMDVar *mVar;
    int mImgWidth;
    int mImgHeight;
    uint32_t mFrameCnt;
    V4L2BUF_t *mV4l2Buf;
};

}; /* namespace android */

#endif

