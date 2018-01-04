//#define LOG_NDEBUG 0
#define LOG_TAG "MotionDetect"
#include <cutils/log.h>
#include <sys/mman.h> 
#include <hardware/camera.h>
#include <utils/Thread.h>
#include <type_camera.h>

#include "../CallbackNotifier.h"
#include "MotionDetect.h"


namespace android {

#ifdef MOTION_DETECTION_1M_MEMORY
#define AWMD_RESIZE_WIDTH   160
#define AWMD_RESIZE_HEIGHT  120

#else
#define AWMD_RESIZE_WIDTH   320
#define AWMD_RESIZE_HEIGHT  240
#endif

MotionDetect::MotionDetect(CallbackNotifier *cb)
    : mCallbackNotifier(cb)
    , mThread(NULL)
    , mWaitImgData(false)
    , mEnable(false)
    , mBuf(NULL)
    , mAWMD(NULL)
    , mVar(NULL)
    , mImgWidth(0)
    , mImgHeight(0)
    , mFrameCnt(0)
    , mV4l2Buf(NULL)
{
    ALOGV("constructor");
}

MotionDetect::~MotionDetect()
{
    ALOGV("destructor");
}

status_t MotionDetect::initialize(int width, int height, int thumbWidth, int thumbHeight, int fps)
{
    unsigned char *mask;

    ALOGD("AWMDInitialize, width=%d, height=%d", width, height);

    if (width <= 0 || height <= 0) {
        ALOGE("Input width or height no right!(width=%d, height=%d)", width, height);
        return BAD_VALUE;
    }

    mBuf = (unsigned char*)malloc(AWMD_RESIZE_WIDTH*AWMD_RESIZE_HEIGHT);
    if (mBuf == NULL) {
        ALOGE("Failed to alloc AWMD buffer(%s)", strerror(errno));
        return NO_MEMORY;
    }

#ifndef MOTION_DETECTION_1M_MEMORY
    mAWMD = allocAWMD(width,height,16,16,5);
#else
    mAWMD = allocAWMD(width, height, AWMD_RESIZE_WIDTH, AWMD_RESIZE_HEIGHT, 150);
#endif

    if (mAWMD == NULL) {
        ALOGE("Failed to alloc AWMD");
        goto ALLOC_AWMD_ERR;
    }
    mVar = &mAWMD->variable;
    mAWMD->vpInit(mVar, 6);

#ifndef MOTION_DETECTION_1M_MEMORY
    mask = (unsigned char*)malloc(width * height);
    if (mask == NULL) {
        ALOGE("Failed to alloc AWMD mask(%s)", strerror(errno));
        goto ALLOC_THREAD_ERR;
    }
    memset(mask, 255, width * height);
    //Set interest of ROI region
    mAWMD->vpSetROIMask(mVar, mask, width, height);
    free(mask);

#endif

    //Set the sensitivity
    mAWMD->vpSetSensitivityLevel(mVar, 4);

    //Open the camera shutter and regional block
    mAWMD->vpSetShelterPara(mVar, 0, 0);

    pthread_mutex_init(&mLock, NULL);

    mThread = new DoAWMDThread(this);
    if (mThread == NULL) {
        ALOGE("Failed to alloc DoAWMDThread(%s)", strerror(errno));
        goto ALLOC_THREAD_ERR;
    }
    pthread_mutex_init(&mMutex, NULL);
    pthread_cond_init(&mCond, NULL);
    mThread->startThread();

    mWaitImgData = false;
    mEnable = false;
    mImgWidth = width;
    mImgHeight = height;

    return NO_ERROR;

    ALLOC_THREAD_ERR:
    pthread_mutex_destroy(&mLock);
    freeAWMD(mAWMD);
    ALLOC_AWMD_ERR:
    free(mBuf);
    mBuf = NULL;
    return NO_MEMORY;
}

status_t MotionDetect::destroy()
{
    ALOGD("<AWMDDestroy>");
    pthread_mutex_lock(&mLock);
    mEnable = false;
    pthread_mutex_unlock(&mLock);
    mImgWidth = 0;
    mImgHeight = 0;
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

    if (mAWMD != NULL) {
        freeAWMD(mAWMD);
        mAWMD = NULL;
    }
    if (mBuf != NULL) {
        free(mBuf);
        mBuf = NULL;
    }
    pthread_mutex_unlock(&mLock);
    pthread_mutex_destroy(&mLock);
    return NO_ERROR;
}

status_t MotionDetect::getCurrentFrame(V4L2BUF_t *v4l2_buf)
{
    if (v4l2_buf == NULL) {
        ALOGW("AWMD frame buffer not ready");
        return INVALID_OPERATION;
    }

    pthread_mutex_lock(&mLock);
#if 0
    memcpy(mAWMDBuf, (void*)mCurV4l2Buf->addrVirY, mCurV4l2Buf->width * mCurV4l2Buf->height);
#else
    mAWMD->vpResize(mVar->scene, (unsigned char*)v4l2_buf->addrVirY, mBuf);
#endif
    pthread_mutex_unlock(&mLock);
    return NO_ERROR;
}

bool MotionDetect::thread()
{
	
    int rtl;
    mWaitImgData = true;
    pthread_mutex_lock(&mMutex);
    pthread_cond_wait(&mCond, &mMutex);
    pthread_mutex_unlock(&mMutex);
    mWaitImgData = false;
    pthread_mutex_lock(&mLock);
    if (!mEnable) {
        pthread_mutex_unlock(&mLock);
        ALOGW("AWMD already stop!");
        return true;
    }

    rtl = mAWMD->vpRun(mBuf, mVar);
    pthread_mutex_unlock(&mLock);

    mCallbackNotifier->AWMDDetectionMsg(rtl);

    return true;
}

status_t MotionDetect::camDataReady(V4L2BUF_t *v4l2_buf)
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

status_t MotionDetect::enable(bool enable)
{
    ALOGD("AW motion detection %s", (enable ? "enable" : "disable"));
    pthread_mutex_lock(&mLock);
    mEnable = enable;
    pthread_mutex_unlock(&mLock);

    return NO_ERROR;
}

status_t MotionDetect::setSensitivityLevel(int level)
{
    ALOGD("<AWMDSetSensitivityLevel>level=%d", level);
    if (level < 1 || level > 6) {
        ALOGE("Motion detection sensitivity level must be 1 to 6!!!");
        return BAD_VALUE;
    }
    pthread_mutex_lock(&mLock);
    mAWMD->vpReset(mVar);
    mAWMD->vpSetSensitivityLevel(mVar, level);
    pthread_mutex_unlock(&mLock);
    return NO_ERROR;
}

int MotionDetect::getWidth()
{
    return mImgWidth;
}

int MotionDetect::getHeight()
{
    return mImgHeight;
}

}; /* namespace android */

