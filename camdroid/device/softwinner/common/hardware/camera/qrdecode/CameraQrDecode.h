#ifndef __CAMERA_QR_DECODE_H__
#define __CAMERA_QR_DECODE_H__

#include <qrdecode_interface.h>
#include "../PhotographAlgorithmBase.h"


namespace android {

class CallbackNotifier;

class CameraQrDecode : public PhotographAlgorithmBase
{
public:
    CameraQrDecode(CallbackNotifier *cb);
    ~CameraQrDecode();
    bool thread();
    status_t camDataReady(V4L2BUF_t *v4l2_buf);
    status_t initialize(int width, int height, int thumbWidth, int thumbHeight, int fps);
    status_t destroy();
    status_t enable(bool enable);


protected:
    class DoQrDecodeThread : public Thread
    {
        CameraQrDecode* mQrDecodeDevice;
        bool                mRequestExit;
    public:
        DoQrDecodeThread(CameraQrDecode* dev)
            : Thread(false)
            , mQrDecodeDevice(dev)
            , mRequestExit(false)
        {
        }
        void startThread()
        {
            run("DoQrDecodeThread", PRIORITY_URGENT_DISPLAY);
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
            return mQrDecodeDevice->thread();
        }
    };

private:
    status_t getCurrentFrame(V4L2BUF_t *v4l2_buf);

    CallbackNotifier *mCallbackNotifier;
    sp<DoQrDecodeThread> mThread;
    pthread_mutex_t mMutex;
    pthread_mutex_t mLock;
    pthread_cond_t mCond;
    bool mWaitImgData;
    bool mEnable;
    char *mBuf;
    int mBufSize;
    int mImgWidth;
    int mImgHeight;
    uint32_t mFrameCnt;
};

}; /* namespace android */

#endif

