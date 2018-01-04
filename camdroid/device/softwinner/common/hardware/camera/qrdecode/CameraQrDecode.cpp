//#define LOG_NDEBUG 0
#define LOG_TAG "CameraQrDecode"
#include <cutils/log.h>
#include <sys/mman.h> 
#include <hardware/camera.h>
#include <utils/Thread.h>
#include <type_camera.h>

#include "CallbackNotifier.h"
#include "CameraQrDecode.h"


namespace android {

CameraQrDecode::CameraQrDecode(CallbackNotifier *cb)
    : mCallbackNotifier(cb)
    , mThread(NULL)
    , mWaitImgData(false)
    , mEnable(false)
    , mBuf(NULL)
    , mBufSize(0)
    , mImgWidth(0)
    , mImgHeight(0)
    , mFrameCnt(0)
{
    ALOGV("constructor");
}

CameraQrDecode::~CameraQrDecode()
{
    ALOGV("destructor");
}

status_t CameraQrDecode::initialize(int width, int height, int thumbWidth, int thumbHeight, int fps)
{
    unsigned char *mask;

    ALOGD("initialize, width=%d, height=%d", width, height);

    if (width < 2 || height < 2) {
        ALOGE("Input width or height no right!(width=%d, height=%d)", width, height);
        return BAD_VALUE;
    }

    mBufSize = (width >> 1) * (height >> 1);
    mBuf = (char*)malloc(mBufSize);
    if (mBuf == NULL) {
        ALOGE("Failed to alloc qrdecode buffer(%s)", strerror(errno));
        return NO_MEMORY;
    }

    pthread_mutex_init(&mLock, NULL);

    mThread = new DoQrDecodeThread(this);
    if (mThread == NULL) {
        ALOGE("Failed to alloc DoQrDecodeThread(%s)", strerror(errno));
        goto ALLOC_THREAD_ERR;
    }
    pthread_mutex_init(&mMutex, NULL);
    pthread_cond_init(&mCond, NULL);
    mThread->startThread();

    mWaitImgData = false;
    mEnable = false;

    return NO_ERROR;

ALLOC_THREAD_ERR:
    pthread_mutex_destroy(&mLock);
    free(mBuf);
    mBuf = NULL;
    return NO_MEMORY;
}

status_t CameraQrDecode::destroy()
{
    ALOGD("<Destroy>");
    pthread_mutex_lock(&mLock);
    mEnable = false;
    pthread_mutex_unlock(&mLock);
    mImgWidth = 0;
    mImgHeight = 0;
    mBufSize = 0;
    mWaitImgData = false;
    pthread_mutex_lock(&mMutex);
    pthread_cond_signal(&mCond);
    pthread_mutex_unlock(&mMutex);

    pthread_mutex_lock(&mLock);
    if (mThread != NULL)
    {
        mThread->stopThread();
        pthread_cond_signal(&mCond);
        mThread.clear();
        mThread = NULL;
    }
    pthread_mutex_destroy(&mMutex);
    pthread_cond_destroy(&mCond);
    if (mBuf != NULL) {
        free(mBuf);
        mBuf = NULL;
    }
    pthread_mutex_unlock(&mLock);

    pthread_mutex_destroy(&mLock);

    return NO_ERROR;
}

status_t CameraQrDecode::getCurrentFrame(V4L2BUF_t *v4l2_buf)
{
    if (v4l2_buf == NULL) {
        ALOGW("frame buffer not ready");
        return INVALID_OPERATION;
    }

    pthread_mutex_lock(&mLock);
    if (v4l2_buf->isThumbAvailable) {
        int size = v4l2_buf->thumbWidth * v4l2_buf->thumbHeight;
        if (size > mBufSize) {
            mImgWidth = v4l2_buf->thumbWidth >> 1;
            mImgHeight = v4l2_buf->thumbHeight >> 1;
            for (int i = 0; i < mImgHeight; i++) {
                char *pbuf = mBuf + i*mImgWidth;
                char *py = (char*)v4l2_buf->thumbAddrVirY + i*v4l2_buf->thumbWidth*2;
                for (int j = 0; j < mImgWidth; j++) {
                    *pbuf = *py;
                    ++pbuf;
                    py += 2;
                }
            }
        } else {
            memcpy(mBuf, (void*)v4l2_buf->thumbAddrVirY, size);
            mImgWidth = v4l2_buf->thumbWidth;
            mImgHeight = v4l2_buf->thumbHeight;
        }
    } else {
        mImgWidth = v4l2_buf->width >> 1;
        mImgHeight = v4l2_buf->height >> 1;
        for (int i = 0; i < mImgHeight; i++) {
            char *pbuf = mBuf + i*mImgWidth;
            char *py = (char*)v4l2_buf->addrVirY + i*v4l2_buf->width*2;
            for (int j = 0; j < mImgWidth; j++) {
                *pbuf = *py;
                ++pbuf;
                py += 2;
            }
        }
    }
    pthread_mutex_unlock(&mLock);
    return NO_ERROR;
}

bool CameraQrDecode::thread()
{
    char result[1028] = "";
    mWaitImgData = true;
    pthread_mutex_lock(&mMutex);
    pthread_cond_wait(&mCond, &mMutex);
    pthread_mutex_unlock(&mMutex);
    mWaitImgData = false;
    pthread_mutex_lock(&mLock);
    if (!mEnable) {
        pthread_mutex_unlock(&mLock);
        ALOGW("already stop!");
        return true;
    }

    int rtl = zbar_dec(mImgWidth, mImgHeight, (char*)mBuf, &result[sizeof(int)], 1024);
    pthread_mutex_unlock(&mLock);
    if (rtl > 0) {
        result[1027] = 0;
        memcpy(result, &rtl, sizeof(int));
        mCallbackNotifier->qrdecodeMsg(result, 1028);
    }

    return true;
}

status_t CameraQrDecode::camDataReady(V4L2BUF_t *v4l2_buf)
{
    if (!mWaitImgData || !mEnable)
        return NO_ERROR;

    if (++mFrameCnt % 2 != 0) {
        return NO_ERROR;
    }
    if (getCurrentFrame(v4l2_buf) != NO_ERROR) {
        return UNKNOWN_ERROR;
    }

    pthread_mutex_lock(&mMutex);
    pthread_cond_signal(&mCond);
    pthread_mutex_unlock(&mMutex);
    return NO_ERROR;
}

status_t CameraQrDecode::enable(bool enable)
{
    ALOGD("QrDecode %s", (enable ? "enable" : "disable"));
    pthread_mutex_lock(&mLock);
    mEnable = enable;
    pthread_mutex_unlock(&mLock);

    return NO_ERROR;
}

}; /* namespace android */

