
#include "CameraDebug.h"
#if DBG_V4L2_CAMERA
#define LOG_NDEBUG 0
#endif
#define LOG_TAG "V4L2CameraDevice"
#include <cutils/log.h>

#include <sys/mman.h> 
#include <videodev2.h>
#include <linux/videodev.h> 

#ifdef USE_MP_CONVERT
#include <g2d_driver.h>
#endif

#include "V4L2CameraDevice2.h"
#include "CallbackNotifier.h"
#include "PreviewWindow.h"
#include "CameraHardware2.h"

#include <memoryAdapter.h>

#ifdef FACE_DETECTION
#include <FDAPI.h>
#endif

#define CHECK_NO_ERROR(a)                       \
    if (a != NO_ERROR) {                        \
        if (mCameraFd >= 0) {                   \
            mCameraDev = CAMERA_UNKNOWN;        \
            close(mCameraFd);                   \
            mCameraFd = -1;                     \
        }                                       \
        return EINVAL;                          \
    }


#ifdef LOGV
#undef LOGV
#define LOGV(fmt, arg...)	ALOGV("<F:%s,L:%d,Camera[%d]> " fmt, __FUNCTION__, __LINE__, mCameraDev, ##arg)
#endif

#ifdef LOGD
#undef LOGD
#define LOGD(fmt, arg...)	ALOGD("<F:%s,L:%d,Camera[%d]> " fmt, __FUNCTION__, __LINE__, mCameraDev, ##arg)
#endif

#ifdef LOGI
#undef LOGI
#define LOGI(fmt, arg...)	ALOGI("<F:%s,L:%d,Camera[%d]> " fmt, __FUNCTION__, __LINE__, mCameraDev, ##arg)
#endif

#ifdef LOGW
#undef LOGW
#define LOGW(fmt, arg...)	ALOGW("<F:%s,L:%d,Camera[%d]> " fmt, __FUNCTION__, __LINE__, mCameraDev, ##arg)
#endif

#ifdef LOGE
#undef LOGE
#define LOGE(fmt, arg...)	ALOGE("<F:%s,L:%d,Camera[%d]> " fmt, __FUNCTION__, __LINE__, mCameraDev, ##arg)
#endif


extern "C" void *uvc_init(const char *devname);
extern "C" int uvc_exit(void *uvc_dev);
extern "C" int uvc_video_process(void *uvc_dev, V4L2BUF_t *pbuf);

namespace android {
    
// defined in HALCameraFactory.cpp
extern void getCallingProcessName(char *name);

uint64_t CameraGetNowTimeUs(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

void V4L2CameraDevice::calculateCrop(Rect * rect, int new_zoom, int max_zoom, int width, int height)
{
#if 0
    if (max_zoom == 0)
    {
        rect->left      = 0;
        rect->top       = 0;
        rect->right     = width -1;
        rect->bottom    = height -1;
    }
    else
    {
        int new_ratio = (new_zoom * 2 * 100 / max_zoom + 100);
        rect->left      = (width - (width * 100) / new_ratio)/2;
        rect->top       = (height - (height * 100) / new_ratio)/2;
        rect->right     = rect->left + (width * 100) / new_ratio -1;
        rect->bottom    = rect->top  + (height * 100) / new_ratio - 1;
    }
#else
    rect->left = (width - width * 10 / (10 + new_zoom)) / 2;
    rect->top = (height - height * 10 / (10 + new_zoom)) / 2;
    rect->right = width - rect->left - 1;
    rect->bottom = height - rect->top - 1;
#endif

    // LOGD("crop: [%d, %d, %d, %d]", rect->left, rect->top, rect->right, rect->bottom);
}

void V4L2CameraDevice::showformat(int format, const char *str)
{
    switch(format){
    case V4L2_PIX_FMT_YUYV :
        ALOGD("The %s foramt is V4L2_PIX_FMT_YUYV",str);
        break;
    case V4L2_PIX_FMT_MJPEG :
        ALOGD("The %s foramt is V4L2_PIX_FMT_MJPEG",str);
        break;
    case V4L2_PIX_FMT_YVU420 :
        ALOGD("The %s foramt is V4L2_PIX_FMT_YVU420",str);
        break;
    case V4L2_PIX_FMT_NV12 :
        ALOGD("The %s foramt is V4L2_PIX_FMT_NV12",str);
        break;
    case V4L2_PIX_FMT_NV21 :
        ALOGD("The %s foramt is V4L2_PIX_FMT_NV21",str);
        break;
    case V4L2_PIX_FMT_H264:
        ALOGD("The %s foramt is V4L2_PIX_FMT_H264",str);
        break;
    default:
        ALOGD("The %s format can't be showed",str);
    }
}

void V4L2CameraDevice::YUYVToNV12(const void* yuyv, void *nv12, int width, int height)
{
    uint8_t* Y  = (uint8_t*)nv12;
    uint8_t* UV = (uint8_t*)Y + width * height;
    
    for(int i = 0; i < height; i += 2)
    {
        for (int j = 0; j < width; j++)
        {
            *(uint8_t*)((uint8_t*)Y + i * width + j) = *(uint8_t*)((uint8_t*)yuyv + i * width * 2 + j * 2);
            *(uint8_t*)((uint8_t*)Y + (i + 1) * width + j) = *(uint8_t*)((uint8_t*)yuyv + (i + 1) * width * 2 + j * 2);
            *(uint8_t*)((uint8_t*)UV + ((i * width) >> 1) + j) = *(uint8_t*)((uint8_t*)yuyv + i * width * 2 + j * 2 + 1);
        }
    }
}

void V4L2CameraDevice::YUYVToNV21(const void* yuyv, void *nv21, int width, int height)
{
    uint8_t* Y  = (uint8_t*)nv21;
    uint8_t* VU = (uint8_t*)Y + width * height;
    
    for(int i = 0; i < height; i += 2)
    {
        for (int j = 0; j < width; j++)
        {
            *(uint8_t*)((uint8_t*)Y + i * width + j) = *(uint8_t*)((uint8_t*)yuyv + i * width * 2 + j * 2);
            *(uint8_t*)((uint8_t*)Y + (i + 1) * width + j) = *(uint8_t*)((uint8_t*)yuyv + (i + 1) * width * 2 + j * 2);

            if (j % 2)
            {
                if (j < width - 1)
                {
                    *(uint8_t*)((uint8_t*)VU + ((i * width) >> 1) + j) = *(uint8_t*)((uint8_t*)yuyv + i * width * 2 + (j + 1) * 2 + 1);
                }
            }
            else
            {
                if (j > 1)
                {
                    *(uint8_t*)((uint8_t*)VU + ((i * width) >> 1) + j) = *(uint8_t*)((uint8_t*)yuyv + i * width * 2 + (j - 1) * 2 + 1);         
                }
            }
        }
    }
}

#ifdef USE_MP_CONVERT
void V4L2CameraDevice::YUYVToYUV420C(const void* yuyv, void *yuv420, int width, int height)
{
    g2d_blt     blit_para;
    int         err;
    
    blit_para.src_image.addr[0]      = (int)yuyv;
    blit_para.src_image.addr[1]      = (int)yuyv + width * height;
    blit_para.src_image.w            = width;         /* src buffer width in pixel units */
    blit_para.src_image.h            = height;        /* src buffer height in pixel units */
    blit_para.src_image.format       = G2D_FMT_IYUV422;
    blit_para.src_image.pixel_seq    = G2D_SEQ_NORMAL;          /* not use now */
    blit_para.src_rect.x             = 0;                       /* src rect->x in pixel */
    blit_para.src_rect.y             = 0;                       /* src rect->y in pixel */
    blit_para.src_rect.w             = width;           /* src rect->w in pixel */
    blit_para.src_rect.h             = height;          /* src rect->h in pixel */

    blit_para.dst_image.addr[0]      = (int)yuv420;
    blit_para.dst_image.addr[1]      = (int)yuv420 + width * height;
    blit_para.dst_image.w            = width;         /* dst buffer width in pixel units */         
    blit_para.dst_image.h            = height;        /* dst buffer height in pixel units */
    blit_para.dst_image.format       = G2D_FMT_PYUV420UVC;
    blit_para.dst_image.pixel_seq    = (mVideoFormat == V4L2_PIX_FMT_NV12) ? G2D_SEQ_NORMAL : G2D_SEQ_VUVU;          /* not use now */
    blit_para.dst_x                  = 0;                   /* dst rect->x in pixel */
    blit_para.dst_y                  = 0;                   /* dst rect->y in pixel */
    blit_para.color                  = 0xff;                /* fix me*/
    blit_para.alpha                  = 0xff;                /* globe alpha */ 

    blit_para.flag = G2D_BLT_NONE; // G2D_BLT_FLIP_HORIZONTAL;

    err = ioctl(mG2DHandle, G2D_CMD_BITBLT, (unsigned long)&blit_para);             
    if(err < 0)     
    {
        LOGE("ioctl, G2D_CMD_BITBLT failed");
        return;
    }
}

void V4L2CameraDevice::NV21ToYV12(const void* nv21, void *yv12, int width, int height)
{
    g2d_blt     blit_para;
    int         err;
    int         u, v;
    if (mVideoFormat == V4L2_PIX_FMT_NV21)
    {
        u = 1;
        v = 2;
    }
    else
    {
        u = 2;
        v = 1;
    }
    
    blit_para.src_image.addr[0]      = (int)nv21;
    blit_para.src_image.addr[1]      = (int)nv21 + width * height;
    blit_para.src_image.w            = width;         /* src buffer width in pixel units */
    blit_para.src_image.h            = height;        /* src buffer height in pixel units */
    blit_para.src_image.format       = G2D_FMT_PYUV420UVC;
    blit_para.src_image.pixel_seq    = G2D_SEQ_NORMAL;//G2D_SEQ_VUVU;          /*  */
    blit_para.src_rect.x             = 0;                       /* src rect->x in pixel */
    blit_para.src_rect.y             = 0;                       /* src rect->y in pixel */
    blit_para.src_rect.w             = width;           /* src rect->w in pixel */
    blit_para.src_rect.h             = height;          /* src rect->h in pixel */

    blit_para.dst_image.addr[0]      = (int)yv12;                           // y
    blit_para.dst_image.addr[u]      = (int)yv12 + width * height;          // v
    blit_para.dst_image.addr[v]      = (int)yv12 + width * height * 5 / 4;  // u
    blit_para.dst_image.w            = width;         /* dst buffer width in pixel units */         
    blit_para.dst_image.h            = height;        /* dst buffer height in pixel units */
    blit_para.dst_image.format       = G2D_FMT_PYUV420;
    blit_para.dst_image.pixel_seq    = G2D_SEQ_NORMAL;          /* not use now */
    blit_para.dst_x                  = 0;                   /* dst rect->x in pixel */
    blit_para.dst_y                  = 0;                   /* dst rect->y in pixel */
    blit_para.color                  = 0xff;                /* fix me*/
    blit_para.alpha                  = 0xff;                /* globe alpha */ 
    
    blit_para.flag = G2D_BLT_NONE;
    
    err = ioctl(mG2DHandle , G2D_CMD_BITBLT, (unsigned long)&blit_para);                
    if(err < 0)
    {
        LOGE("NV21ToYV12 ioctl, G2D_CMD_BITBLT failed");
        return;
    }
}
#endif

DBG_TIME_AVG_BEGIN(TAG_CONTINUOUS_PICTURE);

V4L2CameraDevice::V4L2CameraDevice(CameraHardware* camera_hal,
                                   PreviewWindow * preview_window, 
                                   CallbackNotifier * cb)
    : mCameraHardware(camera_hal)
    , mPreviewWindow(preview_window)
    , mCallbackNotifier(cb)
    , mCameraDeviceState(STATE_CONSTRUCTED)
    , mCaptureThreadState(CAPTURE_STATE_NULL)
    , mCameraFd(-1)
    , mCameraDev(CAMERA_UNKNOWN)
    , mFrameRate(0)
    , mTakePictureState(TAKE_PICTURE_NULL)
    , mIsPicCopy(false)
    , mFrameWidth(0)
    , mFrameHeight(0)
    , mThumbWidth(0)
    , mThumbHeight(0)
    , mRealBufferNum(DEFAULT_BUFFER_NUM)
    , mReqBufferNum(DEFAULT_BUFFER_NUM)
    , mUseHwEncoder(false)
    , mNewZoom(0)
    , mLastZoom(-1)
    , mMaxZoom(0xffffffff)
    , mCaptureFormat(V4L2_PIX_FMT_NV21)
    , mVideoFormat(V4L2_PIX_FMT_NV21)
#ifdef USE_MP_CONVERT
    , mG2DHandle(-1)
#endif
    , mCanBeDisconnected(false)
    , mContinuousPictureStarted(false)
    , mContinuousPictureCnt(0)
    , mContinuousPictureMax(0)
    , mContinuousPictureStartTime(0)
    , mContinuousPictureLast(0)
    , mContinuousPictureAfter(0)
#if 0  // face
    , mFaceDectectLast(0)
    , mFaceDectectAfter(0)
    , mCurrentV4l2buf(NULL)
    , mVideoHint(false)
#endif
    , mIsThumbUsedForVideo(false)
    , mVideoWidth(640)
    , mVideoHeight(480)
    , mSonixUsbCameraDevice(NULL)
    , mDecHandle(NULL)
    , mTVDecoder(NULL)
    , mUvcDev(NULL)
    , mpV4l2Buf(NULL)
    , mMemMapLen(0)
    , mpVideoPicBuf(NULL)
    , mSubChannelEnable(false)
#ifdef MOTION_DETECTION_ENABLE
    , mMotionDetect(NULL)
#endif
#ifdef WATERMARK_ENABLE
    , mWaterMarkEnable(false)
    , mWaterMarkCtrlRec(NULL)
    , mWaterMarkCtrlPrev(NULL)
#endif
#ifdef ADAS_ENABLE
    , mAdas(NULL)
    , mAdasLaneLineOffsetSensity(0)
    , mAdasDistanceDetectLevel(0)
    , mAdasCarSpeed(-1.0000)
#endif
#ifdef QRDECODE_ENABLE
    , mpQrDecode(NULL)
#endif
{
    LOGV("construct");

    memset(&mHalCameraInfo, 0, sizeof(mHalCameraInfo));
    memset(&mRectCrop, 0, sizeof(Rect));
#if 0
    memset(&mVideoBuffer, 0, sizeof(mVideoBuffer));
#endif
    memset(&mPicBuffer, 0, sizeof(V4L2BUF_t));

    mPreviewFlip = CameraParameters::AWEXTEND_PREVIEW_FLIP::NoFlip;
    mPreviewRotation = 0;
    // init preview buffer queue
    OSAL_QueueCreate(&mQueueBufferPreview, DEFAULT_BUFFER_NUM);
    OSAL_QueueCreate(&mQueueBufferPicture, 4);

    pthread_mutex_init(&mConnectMutex, NULL);
    pthread_cond_init(&mConnectCond, NULL);
    
    // init capture thread
    mCaptureThread = new DoCaptureThread(this);
    pthread_mutex_init(&mCaptureMutex, NULL);
    pthread_cond_init(&mCaptureCond, NULL);
    pthread_mutex_lock(&mCaptureMutex);
    mCaptureThreadState = CAPTURE_STATE_PAUSED;
    pthread_mutex_unlock(&mCaptureMutex);
    mCaptureThread->startThread();

    // init preview thread
    mPreviewThread = new DoPreviewThread(this);
    pthread_mutex_init(&mPreviewMutex, NULL);
    pthread_cond_init(&mPreviewCond, NULL);
    mPreviewThread->startThread();

    // init picture thread
    mPictureThread = new DoPictureThread(this);
    pthread_mutex_init(&mPictureMutex, NULL);
    pthread_cond_init(&mPictureCond, NULL);
    mPictureThread->startThread();
    
    // init continuous picture thread
    mContinuousPictureThread = new DoContinuousPictureThread(this);
    pthread_mutex_init(&mContinuousPictureMutex, NULL);
    pthread_cond_init(&mContinuousPictureCond, NULL);
    mContinuousPictureThread->startThread();

    if(mCallbackNotifier)
    {
        mCallbackNotifier->setMutexForReleaseBufferLock(&mReleaseBufferLock);
    }

#ifdef WATERMARK_ENABLE
    pthread_mutex_init(&mWaterMarkLock, NULL);
#endif
}

V4L2CameraDevice::~V4L2CameraDevice()
{
    LOGV("disconstruct");

    if (mCaptureThread != NULL)
    {
        mCaptureThread->stopThread();
        pthread_cond_signal(&mCaptureCond);
        mCaptureThread.clear();
        mCaptureThread = 0;
    }

    if (mPreviewThread != NULL)
    {
        mPreviewThread->stopThread();
        pthread_cond_signal(&mPreviewCond);
        mPreviewThread.clear();
        mPreviewThread = 0;
    }

    if (mPictureThread != NULL)
    {
        mPictureThread->stopThread();
        pthread_cond_signal(&mPictureCond);
        mPictureThread.clear();
        mPictureThread = 0;
    }

    if (mContinuousPictureThread != NULL)
    {
        mContinuousPictureThread->stopThread();
        pthread_cond_signal(&mContinuousPictureCond);
        mContinuousPictureThread.clear();
        mContinuousPictureThread = 0;
    }

    pthread_mutex_destroy(&mCaptureMutex);
    pthread_cond_destroy(&mCaptureCond);

    pthread_mutex_destroy(&mPreviewMutex);
    pthread_cond_destroy(&mPreviewCond);
    
    pthread_mutex_destroy(&mPictureMutex);
    pthread_cond_destroy(&mPictureCond);
    
    pthread_mutex_destroy(&mConnectMutex);
    pthread_cond_destroy(&mConnectCond);
    
    pthread_mutex_destroy(&mContinuousPictureMutex);
    pthread_cond_destroy(&mContinuousPictureCond);
    
    OSAL_QueueTerminate(&mQueueBufferPreview);
    OSAL_QueueTerminate(&mQueueBufferPicture);

    if(mCallbackNotifier)
    {
        mCallbackNotifier->setMutexForReleaseBufferLock(NULL);
    }

#ifdef WATERMARK_ENABLE
    pthread_mutex_destroy(&mWaterMarkLock);
#endif
}

/****************************************************************************
 * V4L2CameraDevice interface implementation.
 ***************************************************************************/

status_t V4L2CameraDevice::connectDevice(HALCameraInfo * halInfo)
{
    LOGV();

    Mutex::Autolock locker(&mObjectLock);
    
    if (isConnected()) 
    {
        LOGW("camera device is already connected.");
        return NO_ERROR;
    }

    // open v4l2 camera device
    status_t ret = openCameraDev(halInfo);
    if (ret != NO_ERROR)
    {
        LOGE("openCameraDev error!");
        return ret;
    }

    memcpy((void*)&mHalCameraInfo, (void*)halInfo, sizeof(HALCameraInfo));

#ifdef USE_MP_CONVERT
    if (mCameraDev == CAMERA_USB)
    {
        // open MP driver
        mG2DHandle = open("/dev/g2d", O_RDWR, 0);
        if (mG2DHandle < 0)
        {
            LOGE("open /dev/g2d failed(%s)", strerror(errno));
            return -1;
        }
        LOGV("open /dev/g2d OK");
    }
#endif 

    ret = MemAdapterOpen();
    if (ret < 0) {
        LOGE("MemAdapterOpen failed!");
        return UNKNOWN_ERROR;
    }
    LOGV("MemAdapterOpen ok");

    /* There is a device to connect to. */
    mCameraDeviceState = STATE_CONNECTED;
    LOGD("connectDevice OK!");
    return NO_ERROR;
}

status_t V4L2CameraDevice::disconnectDevice()
{
    LOGV();
    
    Mutex::Autolock locker(&mObjectLock);
    
    if (!isConnected()) 
    {
        LOGW("camera device is already disconnected.");
        return NO_ERROR;
    }
    
    if (isStarted()) 
    {
        LOGE("Cannot disconnect from the started device.");
        return -EINVAL;
    }
    
    // close v4l2 camera device
    closeCameraDev();
    
#ifdef USE_MP_CONVERT
    if(mG2DHandle >= 0)
    {
        close(mG2DHandle);
        mG2DHandle = -1;
    }
#endif

    MemAdapterClose();

    /* There is no device to disconnect from. */
    mCameraDeviceState = STATE_CONSTRUCTED;

    LOGD("disconnectDevice OK");
    return NO_ERROR;
}

status_t V4L2CameraDevice::startDevice(int width, int height, int sub_w, int sub_h,
    uint32_t pix_fmt, int framerate, bool normalPic)
{
    LOGD("wxh:%dx%d, swxsh:%dx%d, fmt: 0x%x, framerate=%d, normalPic=%d",
        width, height, sub_w, sub_h, pix_fmt, framerate, normalPic);
    
    Mutex::Autolock locker(&mObjectLock);

    if (!isConnected()) 
    {
        LOGE("camera device is not connected.");
        return EINVAL;
    }

    if (isStarted()) 
    {
        LOGE("camera device is already started.");
        return EINVAL;
    }

#if 0  // face
    mCurrentV4l2buf = NULL;

    mVideoHint = video_hint;
#endif
    pthread_mutex_lock(&mConnectMutex);
    mCanBeDisconnected = false;
    pthread_mutex_unlock(&mConnectMutex);

    mSubChannelEnable = false;

    if (mCameraDev == CAMERA_TVD)
    {
        //CHECK_NO_ERROR(mTVDecoder->v4l2SetVideoParams(&mFrameWidth, &mFrameHeight));
        mCameraDeviceState = STATE_STARTED;
#if 0  // face
        mFaceDectectAfter = 1000000 / 15;
#endif
        mVideoFormat = pix_fmt;
        showformat(mVideoFormat, "mVideoFormat");
        return NO_ERROR;
    }
    // set capture mode and fps
    // CHECK_NO_ERROR(v4l2setCaptureParams(framerate));  // do not check this error
    v4l2setCaptureParams(framerate);
    // set v4l2 device parameters, it maybe change the value of mFrameWidth and mFrameHeight.
    CHECK_NO_ERROR(v4l2SetVideoParams(width, height, sub_w, sub_h, pix_fmt));

    CHECK_NO_ERROR(initV4l2Buffer());

    if (!normalPic) {
        int frameRate = 0;
        if (getFrameRate(&frameRate) >= 0) {
            LOGV("set frameRate %d, really frameRate %d", mFrameRate, frameRate);
            mFrameRate = frameRate;
            mCameraHardware->setPreviewFrameRate(mFrameRate);
        }
    }
    setPreviewRate();

    if(mCaptureFormat == V4L2_PIX_FMT_H264 || mCaptureFormat == V4L2_PIX_FMT_MJPEG) {
        int scale = 0;
        mDecHandle = libveInit(mCaptureFormat, pix_fmt, mFrameWidth, mFrameHeight, sub_w, sub_h, mFrameRate, &scale);
        mReleasePicBufIdx = -1;
        if (mDecHandle != NULL) {
            mSubChannelEnable = (scale != 0);
            if (mSubChannelEnable) {
                mThumbWidth = sub_w;
                mThumbHeight = sub_h;
            }
            if (initVideoPictureBuffer() != NO_ERROR) {
                libveExit(mDecHandle);
                mDecHandle = NULL;
            }
        }
    }
    LOGD("sub channel is %s enable", mSubChannelEnable ? "" : "not");

    // v4l2 request buffers
    int buf_cnt = (mTakePictureState == TAKE_PICTURE_NORMAL) ? 1 : mReqBufferNum;
    CHECK_NO_ERROR(v4l2ReqBufs(&buf_cnt));
    mRealBufferNum = buf_cnt;
#if DEBUG_V4L2_CAMERA_USED_BUFFER_CNT
    mUsedBufCnt = 0;
#endif

    // v4l2 query buffers
    CHECK_NO_ERROR(v4l2QueryBuf());

    // stream on the v4l2 device
    CHECK_NO_ERROR(v4l2StartStreaming());

    mCameraDeviceState = STATE_STARTED;

#if 0  // face
    mFaceDectectAfter = 1000000 / 15;
#endif
    mVideoFormat = pix_fmt;
    showformat(mVideoFormat, "mVideoFormat");

    if (mFrameWidth != width || mFrameHeight != height) {
        LOGE("checkout code! Is cameraDev[%d] USBCamera? mFrameWidth[%d] != width[%d] or mFrameHeight[%d] != height[%d], need resetCaptureSize()", 
            mCameraDev, mFrameWidth, width, mFrameHeight, height);
        mCameraHardware->resetCaptureSize(mFrameWidth, mFrameHeight);
    }

#ifdef FACE_DETECTION
    mFD = awInit(width, height);
    if(mFD == NULL) {
        LOGE("Init face detect fail.");
    }
    else {
        LOGD("Init face detect success.");
    }
#endif  //FACE_DETECTION

    return NO_ERROR;
}

status_t V4L2CameraDevice::cameraDrawBlack(void)
{
    V4L2BUF_t *v4l2buf = &mpV4l2Buf->v4l2buf;
    int buffer_len = v4l2buf->width * v4l2buf->height * 3 / 2;  
    int addrVirY = (int)MemAdapterPalloc(buffer_len);
    int addrPhyY = (int)MemAdapterGetPhysicAddress((void*)addrVirY);

    memset((void*)addrVirY, 0x00, v4l2buf->width * v4l2buf->height);
    memset((void*)(addrVirY + v4l2buf->width * v4l2buf->height), 0x80, v4l2buf->width * v4l2buf->height/ 2);

    v4l2buf->addrPhyY       = addrPhyY; 
    v4l2buf->addrVirY       = addrVirY; 

    mPreviewWindow->onNextFrameAvailable((void*)&v4l2buf);

    MemAdapterPfree((void*)addrVirY);

    return NO_ERROR;
}


status_t V4L2CameraDevice::stopDevice()
{
    LOGD();

    pthread_mutex_lock(&mConnectMutex);
    if (!mCanBeDisconnected)
    {
        LOGW("wait until capture thread pause or exit");
        pthread_cond_wait(&mConnectCond, &mConnectMutex);
    }
    pthread_mutex_unlock(&mConnectMutex);
    
    Mutex::Autolock locker(&mObjectLock);

#ifdef ADAS_ENABLE
    Mutex::Autolock _l(&mAlgorithmLock);
#endif
    
    if (!isStarted()) 
    {
        LOGW("camera device is not started.");
        return NO_ERROR;
    }
    if (mCameraDev == CAMERA_TVD)
    {
        mCameraDeviceState = STATE_CONNECTED;
        mLastZoom = -1;
#if 0  // face
        mCurrentV4l2buf = NULL;
#endif
        return NO_ERROR;
    }

    // v4l2 device stop stream
    v4l2StopStreaming();

    // v4l2 device unmap buffers
    v4l2UnmapBuf();

    mCameraDeviceState = STATE_CONNECTED;

    mLastZoom = -1;

#if 0  // face
    mCurrentV4l2buf = NULL;
#endif

    if (mUvcDev != NULL) {
        uvc_exit(mUvcDev);
        mUvcDev = NULL;
    }

#ifdef FACE_DETECTION
    if(mFD) {
        LOGD("Before awRelease()..");
        awRelease(mFD);
        LOGD("After awRelease()..");
        mFD = NULL;
    }
#endif  //FACE_DETECTION

    destroyV4l2Buffer();
    if (mDecHandle != NULL)
    {
        destroyVideoPictureBuffer();
        libveExit(mDecHandle);
        mDecHandle = NULL;
    }

    mSubChannelEnable = false;
    
    return NO_ERROR;
}

status_t V4L2CameraDevice::startDeliveringFrames()
{
    LOGV();

    pthread_mutex_lock(&mCaptureMutex);

    if (mCaptureThreadState == CAPTURE_STATE_NULL)
    {
        LOGE("error state of capture thread");
        pthread_mutex_unlock(&mCaptureMutex);
        return EINVAL;
    }

    if (mCaptureThreadState == CAPTURE_STATE_STARTED)
    {
        LOGW("capture thread has already started");
        pthread_mutex_unlock(&mCaptureMutex);
        return NO_ERROR;
    }

    // singal to start capture thread
    mCaptureThreadState = CAPTURE_STATE_STARTED;
    pthread_cond_signal(&mCaptureCond);
    pthread_mutex_unlock(&mCaptureMutex);

    return NO_ERROR;
}

status_t V4L2CameraDevice::stopDeliveringFrames()
{
    LOGV();
    
    pthread_mutex_lock(&mCaptureMutex);
    if (mCaptureThreadState == CAPTURE_STATE_NULL)
    {
        LOGE("error state of capture thread");
        pthread_mutex_unlock(&mCaptureMutex);
        return EINVAL;
    }

    if (mCaptureThreadState == CAPTURE_STATE_PAUSED)
    {
        LOGW("capture thread has already paused");
        pthread_mutex_unlock(&mCaptureMutex);
        return NO_ERROR;
    }

    mCaptureThreadState = CAPTURE_STATE_PAUSED;
    pthread_mutex_unlock(&mCaptureMutex);

    return NO_ERROR;
}

status_t V4L2CameraDevice::initSonixUsbCameraDevice(char *dev_name)
{
    char dev_node[16];
    int ret;

    LOGV("dev_name %s", dev_name);
    mSonixUsbCameraDevice = new SonixUsbCameraDevice();
    if (mSonixUsbCameraDevice == NULL) {
        LOGE("failed to alloc SonixUsbCameraDevice!");
        return NO_MEMORY;
    }
    if (mSonixUsbCameraDevice->isSonixUVCChip(mCameraFd) < 0) {
        delete mSonixUsbCameraDevice;
        mSonixUsbCameraDevice = NULL;
        return NO_ERROR;
    }

    LOGV("The usb camera is sonix chip.");
    if (mSonixUsbCameraDevice->initCameraParam(mCameraFd) < 0) {
        delete mSonixUsbCameraDevice;
        mSonixUsbCameraDevice = NULL;
        return UNKNOWN_ERROR;
    }
    if (mSonixUsbCameraDevice->updateInternalTime(mCameraFd) < 0) {
        delete mSonixUsbCameraDevice;
        mSonixUsbCameraDevice = NULL;
        return UNKNOWN_ERROR;
    }
    mVideoWidth = 1280;             //init the resolution
    mVideoHeight = 720;
    for (int i = 1; i < 255; i++) { //fix uvc device name
        
        sprintf(dev_node, "/dev/video%d", i);
        if(!strcmp(dev_node,dev_name))
            continue;
        
        ret = access(dev_node, F_OK);
        if (ret == 0) {
            ret = mSonixUsbCameraDevice->openH264CameraDevice(dev_node, mVideoWidth, mVideoHeight);
            if (ret < 0) {
                LOGE("openH264CameraDevice failed!");
                mSonixUsbCameraDevice->useH264DevRec(false);
                return UNKNOWN_ERROR;
            }
            LOGV("useH264DevRec true");
            mSonixUsbCameraDevice->useH264DevRec(true);
            return NO_ERROR;
        }
    }

    LOGV("useH264DevRec false");
    mSonixUsbCameraDevice->useH264DevRec(false);
    return NO_ERROR;
}
status_t V4L2CameraDevice::deinitSonixUsbCameraDevice()
{
    if (mSonixUsbCameraDevice == NULL) {
        return NO_ERROR;
    }
    if (mSonixUsbCameraDevice->isUseH264DevRec()) {
        mSonixUsbCameraDevice->closeH264CameraDevice();
    }
    delete mSonixUsbCameraDevice;
    mSonixUsbCameraDevice = NULL;
    return NO_ERROR;
}


/****************************************************************************
 * Worker thread management.
 ***************************************************************************/

int V4L2CameraDevice::v4l2WaitCameraReady()
{
    fd_set fds;     
    struct timeval tv;
    int r;

    FD_ZERO(&fds);
    FD_SET(mCameraFd, &fds);        
    
    /* Timeout */
    tv.tv_sec  = 2;
    tv.tv_usec = 0;
    
    r = select(mCameraFd + 1, &fds, NULL, NULL, &tv);
    if (r == -1) 
    {
        LOGE("select err, %s", strerror(errno));
        return -1;
    } 
    else if (r == 0) 
    {
        LOGE("select timeout");
        return -1;
    }

    return 0;
}

void V4L2CameraDevice::singalDisconnect()
{
    pthread_mutex_lock(&mConnectMutex);
    mCanBeDisconnected = true;
    pthread_cond_signal(&mConnectCond);
    pthread_mutex_unlock(&mConnectMutex);
}

#ifdef FACE_DETECTION
static void rectangle(unsigned char *YNV12,int imgWidth,int imgHeight,Rects *rects,int YPixelValue,int UVPixelValue,int thick) {
    unsigned char *imgData = YNV12;
    unsigned char *tempImgData = NULL;
    int headX = 0;
    int headY = 0;
    int headWidth = 0;
    int headHeight = 0;
    int i = 0,j = 0;
    int headNum = rects->num;
    int ovWidth = thick;
    int lineWidth = 0;
    int lineHeight = 0;
    int width = imgWidth;
    int value = YPixelValue;
    
    for(i = 0; i < headNum; ++i)
    {
        headX = rects->x[i];
        headY = rects->y[i];
        headWidth = rects->width[i];
        headHeight = rects->height[i];
        
        
        lineWidth = headWidth >> 2;
        lineHeight = headHeight >> 2;
    
        tempImgData = imgData + headY * width + headX;
        
        for(j = 0;j < ovWidth; ++j)
        {
            memset(tempImgData,value,lineWidth);
            
            tempImgData += headWidth - lineWidth;
            memset(tempImgData,value,lineWidth);
            
            tempImgData += lineWidth + width - headWidth;
        }
        
        for(j = 0;j < lineHeight - ovWidth; ++j)
        {
            memset(tempImgData,value,ovWidth);
            
            tempImgData += headWidth - ovWidth;
            memset(tempImgData,value,ovWidth);
            
            tempImgData += ovWidth + width - headWidth;
        }
        
        tempImgData += (headHeight - (lineHeight << 1)) * width;
        for(j = 0;j < lineHeight - ovWidth; ++j)
        {
            memset(tempImgData,value,ovWidth);
            
            tempImgData += headWidth - ovWidth;
            memset(tempImgData,value,ovWidth);
            
            tempImgData += ovWidth + width - headWidth;
        }
        
        for(j = 0;j < ovWidth; ++j)
        {
            memset(tempImgData,value,lineWidth);
            
            tempImgData += headWidth - lineWidth;
            memset(tempImgData,value,lineWidth);
            
            tempImgData += lineWidth + width - headWidth;
        }
        
    }

    value = UVPixelValue;
    imgData = YNV12 + imgWidth * imgHeight;

    for(i = 0;i < headNum; ++i)
    {
        headX = rects->x[i];
        headY = rects->y[i] >> 1;
        headWidth = rects->width[i];
        headHeight = rects->height[i] >> 1;
        
        //LOGD("**********headNum %d X %d Y %d***********",headNum,headX,headY);
        
        lineWidth = headWidth >> 2;
        lineHeight = headHeight >> 2;
    
        tempImgData = imgData + headY * width + headX;
        
        for(j = 0;j < ovWidth; ++j)
        {
            memset(tempImgData,value,lineWidth);
            
            tempImgData += headWidth - lineWidth;
            memset(tempImgData,value,lineWidth);
            
            tempImgData += lineWidth + width - headWidth;
        }
        
        for(j = 0;j < lineHeight - ovWidth; ++j)
        {
            memset(tempImgData,value,ovWidth << 1);
            
            tempImgData += headWidth - (ovWidth << 1);
            memset(tempImgData,value,ovWidth << 1);
            
            tempImgData += (ovWidth << 1) + width - headWidth;
        }
        
        tempImgData += (headHeight - (lineHeight << 1)) * width;
        for(j = 0;j < lineHeight - ovWidth; ++j)
        {
            memset(tempImgData,value,ovWidth << 1);
            
            tempImgData += headWidth - (ovWidth << 1);
            memset(tempImgData,value,ovWidth << 1);
            
            tempImgData += (ovWidth << 1) + width - headWidth;
        }
        
        for(j = 0;j < ovWidth; ++j)
        {
            memset(tempImgData,value,lineWidth);
            
            tempImgData += headWidth - lineWidth;
            memset(tempImgData,value,lineWidth);
            
            tempImgData += lineWidth + width - headWidth;
        }
    }
}
#endif  //FACE_DETECTION

bool V4L2CameraDevice::captureThread()
{
    pthread_mutex_lock(&mCaptureMutex);
    // stop capture
    while (mCaptureThreadState == CAPTURE_STATE_PAUSED)
    {
        singalDisconnect();
        // wait for signal of starting to capture a frame
        LOGV("capture thread paused");
        pthread_cond_wait(&mCaptureCond, &mCaptureMutex);
        LOGV("capture thread paused done[%d]", mCaptureThreadState);
    }

    // thread exit
    if (mCaptureThreadState == CAPTURE_STATE_EXIT)
    {
        singalDisconnect();
        LOGV("capture thread exit");
        pthread_mutex_unlock(&mCaptureMutex);
        return false;
    }
    pthread_mutex_unlock(&mCaptureMutex);

    if (mCameraDev == CAMERA_TVD)
    {
        if (mTVDecoder->isSystemChange() == true)
        {
            LOGW("detect TV System Change during capture!");
            TVinresetDevice();
            //mCallbackNotifier->TvinSystemChangeMsg(mTVDecoder->getSysValue());
        }
    }

    int ret = v4l2WaitCameraReady();
    pthread_mutex_lock(&mCaptureMutex);
    // stop capture or thread exit
    if (mCaptureThreadState == CAPTURE_STATE_PAUSED || mCaptureThreadState == CAPTURE_STATE_EXIT)
    {
        singalDisconnect();
        LOGW("should stop capture now");
        pthread_mutex_unlock(&mCaptureMutex);
        return true;
    }

    if (ret != 0)
    {
#if DEBUG_V4L2_CAMERA_USED_BUFFER_CNT
        LOGW("wait v4l2 buffer time out, mUsedBufCnt(%d)", mUsedBufCnt);
#endif
        pthread_mutex_unlock(&mCaptureMutex);
        mCallbackNotifier->onCameraDeviceError(CAMERA_ERROR_SELECT_TIMEOUT);
        return true;
    }

    // get one video frame
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(v4l2_buffer));
    ret = getPreviewFrame(&buf);
    if (ret != OK)
    {
        int preview_num = OSAL_GetElemNum(&mQueueBufferPreview);
        int picture_num = OSAL_GetElemNum(&mQueueBufferPicture);
        LOGD("preview_num: %d, picture_num: %d", preview_num, picture_num);

        if (preview_num >= 2)
        {
            V4L2BUF_t * pbuf = (V4L2BUF_t *)OSAL_Dequeue(&mQueueBufferPreview);
            if (pbuf != NULL)
            {
                releasePreviewFrame(pbuf->index);
            }
        }

        pthread_mutex_unlock(&mCaptureMutex);
        usleep(10000);
        return true;
    }
#if 0
    else    //store uvc mjpeg stream.
    {
        if(mCameraDev == CAMERA_USB)
        {
            if(0 == buf.bytesused)
            {
                ALOGW("[%s](f:%s, l:%d) fatal error! buf.index[%d], buf.bytesused==0", strrchr(__FILE__, '/')+1, __FUNCTION__, __LINE__, buf.index);
                releasePreviewFrameDirectly(buf.index);
                return true;
            }
            int64_t jpegPts = (int64_t)((int64_t)buf.timestamp.tv_usec + (((int64_t)buf.timestamp.tv_sec) * 1000000));
            char picPath[256];
            snprintf(picPath, 256, "/mnt/extsd/uvc[%lldms][bufIdx%d][%dByte].jpg", jpegPts/1000, buf.index, buf.bytesused);
            int fd = open(picPath, O_CREAT|O_RDWR, 0777);
            if(fd >= 0)
            {
                write(fd, (void*)mMapMem.mem[buf.index], buf.bytesused);
                close(fd);
                ALOGD("[%s](f:%s, l:%d) write [%s] done!", strrchr(__FILE__, '/')+1, __FUNCTION__, __LINE__, picPath);
            }
            else
            {
                ALOGE("[%s](f:%s, l:%d) fatal error! open file[%s] fail!", strrchr(__FILE__, '/')+1, __FUNCTION__, __LINE__, picPath);
            }
        }
    }
#endif
    // deal with this frame
    nsecs_t frameTimestamp = (int64_t)((int64_t)buf.timestamp.tv_usec + (((int64_t)buf.timestamp.tv_sec) * 1000000));
    //LOGD("frameTimestamp[%lld]ms", frameTimestamp/1000);

    if (mLastZoom != mNewZoom)
    {
        LOGV("mMaxZoom=%d, mLastZoom=%d, mNewZoom=%d", mMaxZoom, mLastZoom, mNewZoom);
//      float widthRate = (float)mFrameWidth / (float)mVideoWidth;
//      float heightRate = (float)mFrameHeight / (float)mVideoHeight;
//      if (mIsThumbUsedForVideo && (widthRate > 4.0) && (heightRate > 4.0))
//      {
//          calculateCrop(&mRectCrop, mNewZoom, mMaxZoom, mFrameWidth/2, mFrameHeight/2);   // for cts, to do 
//      }
//      else
        {
            // the main frame crop
            calculateCrop(&mRectCrop, mNewZoom, mMaxZoom, mFrameWidth, mFrameHeight);
        }
        //mCameraHardware->setNewCrop(&mRectCrop);

        // the sub-frame crop
        if (mSubChannelEnable)
        {
            calculateCrop(&mThumbRectCrop, mNewZoom, mMaxZoom, mThumbWidth, mThumbHeight);
        }

        mLastZoom = mNewZoom;

        mPreviewWindow->resetHwdLayerSrc();

        LOGV("CROP: [%d, %d, %d, %d]", mRectCrop.left, mRectCrop.top, mRectCrop.right, mRectCrop.bottom);
        LOGV("thumb CROP: [%d, %d, %d, %d]", mThumbRectCrop.left, mThumbRectCrop.top, mThumbRectCrop.right, mThumbRectCrop.bottom);
    }

    // V4L2BUF_t for preview and HW encoder
    V4L2BUF_t v4l2_buf;
    void *pMem = mpV4l2Buf[buf.index].memory;
    if (mDecHandle == NULL) {
        if(buf.m.offset >= 0x40000000 ) {
            buf.m.offset -= 0x40000000;
        }
        v4l2_buf.isDecodeSrc = 0;
        v4l2_buf.addrPhyY       = buf.m.offset & 0x0fffffff;
        v4l2_buf.addrVirY       = (unsigned int)pMem;
        v4l2_buf.addrPhyC       = v4l2_buf.addrPhyY + ALIGN_16B(mFrameWidth) * ALIGN_16B(mFrameHeight);
        v4l2_buf.addrVirC       = v4l2_buf.addrVirY + ALIGN_16B(mFrameWidth) * ALIGN_16B(mFrameHeight);
        v4l2_buf.timeStamp      = frameTimestamp;
        v4l2_buf.width          = ALIGN_16B(mFrameWidth);
        v4l2_buf.height         = ALIGN_16B(mFrameHeight);

        if (mSubChannelEnable) {
//            CameraParameters parms;
//            char *temp = mCameraHardware->getParameters();
//            String8 str_parms(temp);
//            mCameraHardware->putParameters(temp);
//            parms.unflatten(str_parms);
            v4l2_buf.isThumbAvailable       = 1;
            v4l2_buf.thumbUsedForPreview    = 1;
            v4l2_buf.thumbUsedForPhoto      = 0;
            if(mIsThumbUsedForVideo == true) {
                v4l2_buf.thumbUsedForVideo      = 1;
            } else {
                v4l2_buf.thumbUsedForVideo      = 0;
            }
            v4l2_buf.thumbAddrPhyY          = v4l2_buf.addrPhyY + ALIGN_4K(ALIGN_16B(mFrameWidth) * ALIGN_16B(mFrameHeight) * 3 / 2);   // to do
            v4l2_buf.thumbAddrVirY          = v4l2_buf.addrVirY + ALIGN_4K(ALIGN_16B(mFrameWidth) * ALIGN_16B(mFrameHeight) * 3 / 2);   // to do
            v4l2_buf.thumbAddrPhyC          = v4l2_buf.thumbAddrPhyY + ALIGN_16B(mThumbWidth) * ALIGN_16B(mThumbHeight);
            v4l2_buf.thumbAddrVirC          = v4l2_buf.thumbAddrVirY + ALIGN_16B(mThumbWidth) * ALIGN_16B(mThumbHeight);
            v4l2_buf.thumbWidth             = ALIGN_16B(mThumbWidth);
            v4l2_buf.thumbHeight            = ALIGN_16B(mThumbHeight);
            v4l2_buf.thumb_crop_rect.left   = mThumbRectCrop.left;
            v4l2_buf.thumb_crop_rect.top    = mThumbRectCrop.top;
            v4l2_buf.thumb_crop_rect.width  = mThumbRectCrop.right - mThumbRectCrop.left + 1;
            v4l2_buf.thumb_crop_rect.height = mThumbRectCrop.bottom - mThumbRectCrop.top + 1;
            v4l2_buf.thumbFormat            = mVideoFormat;
            if(CameraParameters::AWEXTEND_PREVIEW_FLIP::LeftRightFlip == mPreviewFlip) {
                CameraHal_LRFlipFrame(v4l2_buf.thumbFormat, v4l2_buf.thumbWidth, v4l2_buf.thumbHeight, (char*)v4l2_buf.thumbAddrVirY, (char*)v4l2_buf.thumbAddrVirC);
                MemAdapterFlushCache((void*)v4l2_buf.thumbAddrVirY, v4l2_buf.thumbWidth * v4l2_buf.thumbHeight*3/2);
            }
        } else {
            v4l2_buf.isThumbAvailable       = 0;
        }
        //LOGD("frameTimestamp=%lld", frameTimestamp);
    } else {
#if 0
        static uint64_t save = 0;
        if (save == 100) {
            char filename[256];
            snprintf(filename, 256, "/mnt/extsd/mjpeg_raw_%d.jpg", mFrameHeight);
            int fd = open(filename, O_RDWR | O_CREAT, 0666);
            write(fd, mMapMem.mem[buf.index], buf.bytesused);
            close(fd);
        }
        save++;
#endif
        //int64_t tmp =  CameraGetNowTimeUs();
        //ALOGD("(f:%s, l:%d) jpegSize[%d]KB!", __FUNCTION__, __LINE__, buf.bytesused/1024);
        libveDecode(mDecHandle, pMem, buf.bytesused, frameTimestamp);
        //int64_t dec_time = (CameraGetNowTimeUs() - tmp) /1000;
        //static int64_t cnt = 0;
        //static int64_t sum = 0;
        //sum += dec_time;
        //cnt++;
        //LOGD("libveDecode time=%lld, average=%lld", dec_time, sum/cnt);
        releasePreviewFrameDirectly(buf.index);
        if (getUnusedPicBuffer((int*)&buf.index) != NO_ERROR) {
            LOGE("Need release buffer!");
            pthread_mutex_unlock(&mCaptureMutex);
            return true;
        }
        if (getDecodeFrame((int)buf.index) != NO_ERROR) {
            pthread_mutex_unlock(&mCaptureMutex);
            return true;
        }

        VideoPictureBuffer *pvbuf = mpVideoPicBuf+buf.index;
        v4l2_buf.isDecodeSrc = 1;
        v4l2_buf.addrVirY       = (unsigned int)pvbuf->picture[0]->pData0;
        v4l2_buf.addrVirC       = (unsigned int)pvbuf->picture[0]->pData1;
        v4l2_buf.addrPhyY       = (unsigned int)MemAdapterGetPhysicAddress((void*)v4l2_buf.addrVirY);
        v4l2_buf.addrPhyC       = (unsigned int)MemAdapterGetPhysicAddress((void*)v4l2_buf.addrVirC);
        v4l2_buf.timeStamp      = pvbuf->picture[0]->nPts;
        v4l2_buf.width          = ALIGN_16B(pvbuf->picture[0]->nWidth);
        v4l2_buf.height         = ALIGN_16B(pvbuf->picture[0]->nHeight);

        if (mSubChannelEnable) {
            VideoPictureBuffer *pvbuf = mpVideoPicBuf+buf.index;
            v4l2_buf.isThumbAvailable       = 1;
            v4l2_buf.thumbUsedForPreview    = 1;
            v4l2_buf.thumbUsedForPhoto      = 0;
            v4l2_buf.thumbAddrVirY          = (unsigned int)pvbuf->picture[1]->pData0;
            v4l2_buf.thumbAddrVirC          = (unsigned int)pvbuf->picture[1]->pData1;
            v4l2_buf.thumbAddrPhyY          = (unsigned int)MemAdapterGetPhysicAddress((void*)v4l2_buf.thumbAddrVirY);
            v4l2_buf.thumbAddrPhyC          = (unsigned int)MemAdapterGetPhysicAddress((void*)v4l2_buf.thumbAddrVirC);
            v4l2_buf.thumbWidth             = ALIGN_16B(pvbuf->picture[1]->nWidth);
            v4l2_buf.thumbHeight            = ALIGN_16B(pvbuf->picture[1]->nHeight);
            v4l2_buf.thumb_crop_rect.left   = mThumbRectCrop.left;
            v4l2_buf.thumb_crop_rect.top    = mThumbRectCrop.top;
            v4l2_buf.thumb_crop_rect.width  = mThumbRectCrop.right - mThumbRectCrop.left + 1;
            v4l2_buf.thumb_crop_rect.height = mThumbRectCrop.bottom - mThumbRectCrop.top + 1;
            v4l2_buf.thumbFormat            = mVideoFormat;
            //LOGD("sub ePixelFormat=%d", pvbuf->picture[1]->ePixelFormat);
            if(CameraParameters::AWEXTEND_PREVIEW_FLIP::LeftRightFlip == mPreviewFlip) {
                CameraHal_LRFlipFrame(v4l2_buf.thumbFormat, v4l2_buf.thumbWidth, v4l2_buf.thumbHeight, (char*)v4l2_buf.thumbAddrVirY, (char*)v4l2_buf.thumbAddrVirC);
                MemAdapterFlushCache((void*)v4l2_buf.thumbAddrVirY, v4l2_buf.thumbWidth * v4l2_buf.thumbHeight*3/2);
            }
        } else {
            v4l2_buf.isThumbAvailable       = 0;
        }
#if 0
        LOGD("main ePixelFormat=%d, width=%d, height=%d, nPts=%lld, frameTimestamp=%lld",
            pvbuf->picture[0]->ePixelFormat,
            pvbuf->picture[0]->nWidth,
            pvbuf->picture[0]->nHeight,
            pvbuf->picture[0]->nPts,
            frameTimestamp
            );
#endif
    }

    v4l2_buf.index              = buf.index;
    v4l2_buf.crop_rect.left     = mRectCrop.left;
    v4l2_buf.crop_rect.top      = mRectCrop.top;
    v4l2_buf.crop_rect.width    = mRectCrop.right - mRectCrop.left + 1;
    v4l2_buf.crop_rect.height   = mRectCrop.bottom - mRectCrop.top + 1;
    v4l2_buf.format             = mVideoFormat;
    v4l2_buf.bytesused          = buf.bytesused;
    v4l2_buf.refCnt = 1;

    memset(v4l2_buf.roi_area, 0, sizeof(Venc_ROI_Config)*ROI_AREA_NUM);

    V4l2Buffer_tag *pBuffer = mpV4l2Buf + v4l2_buf.index;
    V4L2BUF_t *pV4l2Buf = &pBuffer->v4l2buf;
    V4l2PreviewBuffer_tag *pPrevbuf = &pBuffer->prevbuf;

    memcpy(pV4l2Buf, &v4l2_buf, sizeof(V4L2BUF_t));
    if (v4l2_buf.isThumbAvailable == 1 && v4l2_buf.thumbUsedForPreview == 1) {
        int val = 0;
        if (mPreviewRotation > 0) {
            val = ALIGN_16B(v4l2_buf.thumbWidth) * ALIGN_16B(v4l2_buf.thumbHeight);
        }
        pPrevbuf->addrPhyY = v4l2_buf.thumbAddrPhyY + ALIGN_4K(val * 3 / 2);
        pPrevbuf->addrVirY = v4l2_buf.thumbAddrVirY + ALIGN_4K(val * 3 / 2);
        pPrevbuf->addrPhyC = pPrevbuf->addrPhyY + val;
        pPrevbuf->addrVirC = pPrevbuf->addrVirY + val;
        pPrevbuf->format = v4l2_buf.thumbFormat;
        if (mPreviewRotation == 90 || mPreviewRotation == 270) {
            pPrevbuf->width = v4l2_buf.thumbHeight;
            pPrevbuf->height = v4l2_buf.thumbWidth;
            pPrevbuf->crop_rect.top = v4l2_buf.thumb_crop_rect.left;
            pPrevbuf->crop_rect.left = v4l2_buf.thumb_crop_rect.top;
            pPrevbuf->crop_rect.width = v4l2_buf.thumb_crop_rect.height;
            pPrevbuf->crop_rect.height = v4l2_buf.thumb_crop_rect.width;
        } else {
            pPrevbuf->width = v4l2_buf.thumbWidth;
            pPrevbuf->height = v4l2_buf.thumbHeight;
            pPrevbuf->crop_rect.top = v4l2_buf.thumb_crop_rect.top;
            pPrevbuf->crop_rect.left = v4l2_buf.thumb_crop_rect.left;
            pPrevbuf->crop_rect.width = v4l2_buf.thumb_crop_rect.width;
            pPrevbuf->crop_rect.height = v4l2_buf.thumb_crop_rect.height;
        }
    } else {
        pPrevbuf->addrPhyY = v4l2_buf.addrPhyY;
        pPrevbuf->addrVirY = v4l2_buf.addrVirY;
        pPrevbuf->addrPhyC = v4l2_buf.addrPhyC;
        pPrevbuf->addrVirC = v4l2_buf.addrVirC;
        pPrevbuf->format = v4l2_buf.format;
        pPrevbuf->width = v4l2_buf.width;
        pPrevbuf->height = v4l2_buf.height;
        pPrevbuf->crop_rect.top = v4l2_buf.crop_rect.top;
        pPrevbuf->crop_rect.left = v4l2_buf.crop_rect.left;
        pPrevbuf->crop_rect.width = v4l2_buf.crop_rect.width;
        pPrevbuf->crop_rect.height = v4l2_buf.crop_rect.height;
    }

#if 0
    if (mCameraDev == CAMERA_USB)
    {
        static uint64_t s_time = 0;
        uint64_t time = CameraGetNowTimeUs();
        LOGD("Frame interval %lldms", (time - s_time) / 1000);
        s_time = time;
    }
#endif


#ifdef MOTION_DETECTION_ENABLE
    if (mMotionDetect != NULL) {
        mMotionDetect->camDataReady(pV4l2Buf);
    }
#endif

#ifdef FACE_DETECTION
    if (mFD != NULL) {
        unsigned char *pbuf = (unsigned char *)pV4l2Buf->addrVirY;
        awRun(mFD, pbuf);
        Rects rects;
        memset(&rects, 0, sizeof(Rects));
        awGetRect(mFD, &rects);
        rectangle(pbuf, pV4l2Buf->width, pV4l2Buf->height, &rects, 150, 54, 2);
    }
#endif  //FACE_DETECTION

#ifdef QRDECODE_ENABLE
    if (mpQrDecode != NULL) {
        mpQrDecode->camDataReady(pV4l2Buf);
    }
#endif

#ifdef WATERMARK_ENABLE
    if (mWaterMarkEnable) {
        pthread_mutex_lock(&mWaterMarkLock);
        if (mWaterMarkCtrlRec != NULL) {
            mWaterMarkIndata.y = (unsigned char *)pV4l2Buf->addrVirY;
            mWaterMarkIndata.c = (unsigned char *)pV4l2Buf->addrVirY + ALIGN_16B(pV4l2Buf->width) * ALIGN_16B(pV4l2Buf->height);
            mWaterMarkIndata.width = pV4l2Buf->crop_rect.width;
            mWaterMarkIndata.height = pV4l2Buf->crop_rect.height;
            mWaterMarkIndata.main_channel = 1;
            char buffer[128];
            getTimeString(buffer, 128);
            doWaterMarkMultiple(&mWaterMarkIndata, mWaterMarkCtrlRec, buffer);
            MemAdapterFlushCache((void*)pV4l2Buf->addrVirY, pV4l2Buf->width * pV4l2Buf->height*3/2);
        }
#ifndef NO_PREVIEW_WATERMARK
        if (mSubChannelEnable)
        {
            if (mWaterMarkCtrlPrev != NULL) {
                mWaterMarkIndata.y = (unsigned char *)pV4l2Buf->thumbAddrVirY;
                mWaterMarkIndata.c = (unsigned char *)pV4l2Buf->thumbAddrVirY + ALIGN_16B(pV4l2Buf->thumbWidth) * ALIGN_16B(pV4l2Buf->thumbHeight);
                mWaterMarkIndata.width = pV4l2Buf->thumb_crop_rect.width;
                mWaterMarkIndata.height = pV4l2Buf->thumb_crop_rect.height;
                mWaterMarkIndata.scale = (float)mThumbHeight/(float)mFrameHeight;
                mWaterMarkIndata.main_channel = 0;
                char buffer[128];
                getTimeString(buffer, 128);
                doWaterMarkMultiple(&mWaterMarkIndata, mWaterMarkCtrlPrev, buffer);
                MemAdapterFlushCache((void*)pV4l2Buf->thumbAddrVirY, pV4l2Buf->thumbWidth * pV4l2Buf->thumbHeight*3/2);
            }
        }
#endif
        pthread_mutex_unlock(&mWaterMarkLock);
    }
#endif

#ifdef ADAS_ENABLE
    if (mAdas != NULL) {
        mAdas->camDataReady(pV4l2Buf);
        Mutex::Autolock _l(mRoiLock);
        memcpy(pV4l2Buf->roi_area, mRoiArea, sizeof(Venc_ROI_Config)*ROI_AREA_NUM);
    }
#endif

#if 0  // face
    if (!mVideoHint)
    {
        // face detection only use when picture mode
        mCurrentV4l2buf = pV4l2Buf;
    }
#endif


    if (mTakePictureState == TAKE_PICTURE_NORMAL)
    {
        int frameSize = pV4l2Buf->width * pV4l2Buf->height * 3 >> 1;

        //copy picture buffer
        memcpy(&mPicBuffer, pV4l2Buf, sizeof(V4L2BUF_t));
        mPicBuffer.addrVirY = (unsigned int)MemAdapterPalloc(frameSize);
        if (mPicBuffer.addrVirY == 0) {
            LOGE("MemAdapterPalloc failed!!");
            goto DEC_REF;
        }
        mPicBuffer.addrPhyY = (unsigned int)MemAdapterGetPhysicAddress((void*)mPicBuffer.addrVirY);
        MemAdapterFlushCache((void*)pV4l2Buf->addrVirY, frameSize);
        memcpy((void*)mPicBuffer.addrVirY, (void*)pV4l2Buf->addrVirY, frameSize);
        MemAdapterFlushCache((void*)mPicBuffer.addrVirY, frameSize);

        // enqueue picture buffer
        ret = OSAL_Queue(&mQueueBufferPicture, &mPicBuffer);
        if (ret != 0)
        {
            LOGW("picture queue full");
            pthread_cond_signal(&mPictureCond);
            goto DEC_REF;
        }

        mIsPicCopy = true;
        mCaptureThreadState = CAPTURE_STATE_PAUSED;
        mTakePictureState = TAKE_PICTURE_NULL;
        pthread_cond_signal(&mPictureCond);
        goto DEC_REF;
    }
    else
    {
        Mutex::Autolock locker(&mReleaseBufferLock);

        //get the isp pic var for encoder
        pV4l2Buf->ispPicVar = getPicVar() ;
        //LOGD("ycy hal: ispPicVar is %08x\n",mV4l2buf[v4l2_buf.index].ispPicVar);

        ret = OSAL_Queue(&mQueueBufferPreview, pBuffer);
        if (ret != 0)
        {
            LOGW("preview queue full");
            goto DEC_REF;
        }

        // add reference count
        pV4l2Buf->refCnt++;
        // signal a new frame for preview
        pthread_cond_signal(&mPreviewCond);

        if (mTakePictureState == TAKE_PICTURE_FAST || mTakePictureState == TAKE_PICTURE_RECORD)
        {
            //LOGD("xxxxxxxxxxxxxxxxxxxx buf.reserved: %x", buf.reserved);
            //if (mHalCameraInfo.fast_picture_mode)
            //{
            //    if (buf.reserved == 0xFFFFFFFF)
            //    {
            //        //LOGD("xxxxxxxxxxxxxxxxxxxx buf.reserved: 0xFFFFFFFF");
            //        //goto DEC_REF;
            //    }
            //}

            // enqueue picture buffer
            ret = OSAL_Queue(&mQueueBufferPicture, pV4l2Buf);
            if (ret != 0)
            {
                LOGW("picture queue full");
                pthread_cond_signal(&mPictureCond);
                goto DEC_REF;
            }

            // add reference count
            pV4l2Buf->refCnt++;
            mIsPicCopy = false;
            mTakePictureState = TAKE_PICTURE_NULL;
            pthread_cond_signal(&mPictureCond);
        }

        if ((mTakePictureState == TAKE_PICTURE_CONTINUOUS
            || mTakePictureState == TAKE_PICTURE_CONTINUOUS_FAST)
            && isContinuousPictureTime())
        {
            if (mHalCameraInfo.fast_picture_mode)
            {
                if (buf.reserved == 0xFFFFFFFF)
                {
                    goto DEC_REF;
                }
            }

            // enqueue picture buffer
            ret = OSAL_Queue(&mQueueBufferPicture, pV4l2Buf);
            if (ret != 0)
            {
                // LOGV("continuous picture queue full");
                pthread_cond_signal(&mContinuousPictureCond);
                goto DEC_REF;
            }

            // add reference count
            pV4l2Buf->refCnt++;
            mIsPicCopy = false;
            pthread_cond_signal(&mContinuousPictureCond);
        }
    }

DEC_REF:
    releasePreviewFrame(pV4l2Buf->index);

    pthread_mutex_unlock(&mCaptureMutex);
    return true;
}

bool V4L2CameraDevice::previewThread()
{
    V4l2Buffer_tag *pbuffer = (V4l2Buffer_tag*)OSAL_Dequeue(&mQueueBufferPreview);
    if (pbuffer == NULL)
    {
        // LOGV("picture queue no buffer, sleep...");
        pthread_mutex_lock(&mPreviewMutex);
        pthread_cond_wait(&mPreviewCond, &mPreviewMutex);
        pthread_mutex_unlock(&mPreviewMutex);
        return true;
    }
    V4L2BUF_t *pbuf = &pbuffer->v4l2buf;

    Mutex::Autolock locker(&mObjectLock);
    if (mpV4l2Buf[pbuf->index].memory == NULL || pbuf->addrPhyY == 0)
    {
        LOGV("preview buffer have been released...");
        return true;
    }

    if (mUvcDev != NULL) {
        uvc_video_process(mUvcDev, pbuf);
    } else {
        mCallbackNotifier->onNextFrameAvailable((void*)pbuf, mUseHwEncoder, pbuf->index);
        if (mPreviewCnt++ % mPreviewRate == 0) {
            mPreviewWindow->onNextFrameAvailable((void*)&pbuffer->prevbuf);
        }
    }
    
    if (mSonixUsbCameraDevice != NULL) {
        mSonixUsbCameraDevice->updateInternalTime(mCameraFd);
    }

    releasePreviewFrame(pbuf->index);

    return true;
}

// singal picture
bool V4L2CameraDevice::pictureThread()
{
    V4L2BUF_t * pbuf = (V4L2BUF_t *)OSAL_Dequeue(&mQueueBufferPicture);
    if (pbuf == NULL)
    {
        LOGV("picture queue no buffer, sleep...");
        pthread_mutex_lock(&mPictureMutex);
        pthread_cond_wait(&mPictureCond, &mPictureMutex);
        pthread_mutex_unlock(&mPictureMutex);
        return true;
    }

    //int64_t tm0 = getSystemTimeUs();
    mCameraHardware->notifyPictureMsg((void*)pbuf);

    mCallbackNotifier->takePicture(pbuf, false, true);
    mCallbackNotifier->takePicture(pbuf);
    mCallbackNotifier->takePictureEnd();

    if (!mIsPicCopy) {
        releasePreviewFrame(pbuf->index);
    }
    if (mPicBuffer.addrVirY != 0) {
        MemAdapterPfree((void*)mPicBuffer.addrVirY);
        mPicBuffer.addrVirY = 0;
    }

    //LOGV("takePicture time %lldms", (getSystemTimeUs() - tm0)/1000);
    return true;
}

// continuous picture
bool V4L2CameraDevice::continuousPictureThread()
{
    V4L2BUF_t * pbuf = (V4L2BUF_t *)OSAL_Dequeue(&mQueueBufferPicture);
    if (pbuf == NULL)
    {
        LOGV("continuousPictureThread queue no buffer, sleep...");
        pthread_mutex_lock(&mContinuousPictureMutex);
        pthread_cond_wait(&mContinuousPictureCond, &mContinuousPictureMutex);
        pthread_mutex_unlock(&mContinuousPictureMutex);
        return true;
    }

    Mutex::Autolock locker(&mObjectLock);
    if (mpV4l2Buf[pbuf->index].memory == NULL || pbuf->addrPhyY == 0)
    {
        LOGV("picture buffer have been released...");
        return true;
    }

    DBG_TIME_AVG_AREA_IN(TAG_CONTINUOUS_PICTURE);

    // reach the max number of pictures
    if (mContinuousPictureCnt >= mContinuousPictureMax)
    {
        mTakePictureState = TAKE_PICTURE_NULL;
        stopContinuousPicture();
        mCallbackNotifier->takePictureEnd();
        releasePreviewFrame(pbuf->index);
        return true;
    }

    // apk stop continuous pictures
    if (!mContinuousPictureStarted)
    {
        mTakePictureState = TAKE_PICTURE_NULL;
        releasePreviewFrame(pbuf->index);
        return true;
    }

    bool ret = mCallbackNotifier->takePicture((void*)pbuf, true);
    if (ret)
    {
        mContinuousPictureCnt++;
    
        DBG_TIME_AVG_AREA_OUT(TAG_CONTINUOUS_PICTURE);
    }
    else
    {
        // LOGW("do not encoder jpeg");
    }

    releasePreviewFrame(pbuf->index);

    return true;
}

void V4L2CameraDevice::startContinuousPicture()
{
    LOGV();

    mContinuousPictureCnt = 0;
    mContinuousPictureStarted = true;
    mContinuousPictureStartTime = systemTime(SYSTEM_TIME_MONOTONIC);

    DBG_TIME_AVG_INIT(TAG_CONTINUOUS_PICTURE);
}

void V4L2CameraDevice::stopContinuousPicture()
{
    LOGV();

    if (!mContinuousPictureStarted)
    {
        LOGD("Continuous picture has already stopped");
        return;
    }
    
    mContinuousPictureStarted = false;

    nsecs_t time = (systemTime(SYSTEM_TIME_MONOTONIC) - mContinuousPictureStartTime)/1000000;
    LOGD("Continuous picture cnt: %d, use time %lld(ms)", mContinuousPictureCnt, time);
    if (time != 0)
    {
        LOGD("Continuous picture %f(fps)", (float)mContinuousPictureCnt/(float)time * 1000);
    }

    DBG_TIME_AVG_END(TAG_CONTINUOUS_PICTURE, "picture enc");
}

void V4L2CameraDevice::setContinuousPictureNumber(int cnt)
{
    LOGV();
    mContinuousPictureMax = cnt;
}

void V4L2CameraDevice::setContinuousPictureInterval(int timeMs)
{
    LOGV();
    mContinuousPictureAfter = timeMs * 1000;
}

bool V4L2CameraDevice::isContinuousPictureTime()
{
    if (mTakePictureState == TAKE_PICTURE_CONTINUOUS_FAST)
    {
        return true;
    }
    
    timeval cur_time;
    gettimeofday(&cur_time, NULL);
    const uint64_t cur_mks = cur_time.tv_sec * 1000000LL + cur_time.tv_usec;
    if ((cur_mks - mContinuousPictureLast) >= mContinuousPictureAfter) {
        mContinuousPictureLast = cur_mks;
        return true;
    }
    return false;
}

#if 0  // face
void V4L2CameraDevice::waitFaceDectectTime()
{
    timeval cur_time;
    gettimeofday(&cur_time, NULL);
    const uint64_t cur_mks = cur_time.tv_sec * 1000000LL + cur_time.tv_usec;
    
    if ((cur_mks - mFaceDectectLast) >= mFaceDectectAfter)
    {
        mFaceDectectLast = cur_mks;
    }   
    else
    {
        usleep(mFaceDectectAfter - (cur_mks - mFaceDectectLast));
        gettimeofday(&cur_time, NULL);
        mFaceDectectLast = cur_time.tv_sec * 1000000LL + cur_time.tv_usec;
    }
}

int V4L2CameraDevice::getCurrentFaceFrame(void * frame)
{
    if (frame == NULL)
    {
        LOGE("getCurrentFrame: error in null pointer");
        return -1;
    }

    pthread_mutex_lock(&mCaptureMutex);
    // stop capture
    if (mCaptureThreadState != CAPTURE_STATE_STARTED)
    {
        LOGW("capture thread dose not started");
        pthread_mutex_unlock(&mCaptureMutex);
        return -1;
    }
    pthread_mutex_unlock(&mCaptureMutex);
    
    waitFaceDectectTime();

    Mutex::Autolock locker(&mObjectLock);
    
    if (mCurrentV4l2buf == NULL
        || mCurrentV4l2buf->addrVirY == 0)
    {
        LOGW("frame buffer not ready");
        return -1;
    }

    if ((mCurrentV4l2buf->isThumbAvailable == 1)
        && (mCurrentV4l2buf->thumbUsedForPreview == 1))
    {
        memcpy(frame, 
                (void*)(mCurrentV4l2buf->addrVirY + ALIGN_4K(ALIGN_32B(mCurrentV4l2buf->width) * mCurrentV4l2buf->height * 3 / 2)), 
                ALIGN_32B(mCurrentV4l2buf->thumbWidth) * mCurrentV4l2buf->thumbHeight);
    }
    else
    {
        memcpy(frame, (void*)mCurrentV4l2buf->addrVirY, mCurrentV4l2buf->width * mCurrentV4l2buf->height);
    }

    return 0;
}
#endif

// -----------------------------------------------------------------------------
// extended interfaces here <***** star *****>
// -----------------------------------------------------------------------------
status_t V4L2CameraDevice::openCameraDev(HALCameraInfo * halInfo)
{
    LOGV();

    int ret = -1;
    struct v4l2_input inp;
    struct v4l2_capability cap; 

    LOGD("V4L2CameraDevice::openCameraDev, device_id(%d), device_name(%s)", halInfo->device_id, halInfo->device_name);
    if (halInfo == NULL)
    {
        LOGE("error HAL camera info");
        return UNKNOWN_ERROR;
    }

    char dev_node[64];
    int fd = -1;

    if (halInfo->device_id > 2 || halInfo->device_id < 0) {
        LOGE("Invalid device ID %d", halInfo->device_id);
        return BAD_VALUE;
    }

    for (int i = 0; i < 255; ++i) {
        snprintf(dev_node, 64, "/dev/video%d", i);
        ret = access(dev_node, F_OK);
        if (ret != 0) {
            continue;
        }
        fd = open(dev_node, O_RDWR | O_NONBLOCK, 0);
        if (fd < 0) {
            continue;
        }
        ret = ioctl (fd, VIDIOC_QUERYCAP, &cap);
        if (ret < 0) {
            close(fd);
            continue;
        }
        LOGV("%s driver name %s", dev_node, cap.driver);
        if (halInfo->device_id == 0) {
            if (strcmp((char*)cap.driver, "sunxi-vfe") == 0) {
                LOGD("sunxi_csi device node is %s", dev_node);
                mCameraDev = CAMERA_CSI;
                break;
            }
        } else if (halInfo->device_id == 1){
            if (strcmp((char*)cap.driver, "uvcvideo") == 0) {
                LOGD("uvcvideo device node is %s", dev_node);
                mCameraDev = CAMERA_USB;
                break;
            }
        } else if (halInfo->device_id == 2) {
            if (strcmp((char*)cap.driver, "tvd") == 0) {
                LOGD("TVD device node is %s", dev_node);
                mCameraDev = CAMERA_TVD;
                break;
            }
        }
        close(fd);
        fd = -1;
    }
    if (fd < 0) {
        LOGE("Could not find device for device ID %d!", halInfo->device_id);
        return NO_INIT;
    }
    mCameraFd = fd;
    LOGI("Camera device is %d", mCameraDev);

    // check v4l2 device capabilities
    ret = ioctl (mCameraFd, VIDIOC_QUERYCAP, &cap);
    if (ret < 0)
    {
        LOGE("Error opening device: unable to query device.");
        goto END_ERROR;
    }

    if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) 
    {
        LOGE("Error opening device: video capture not supported."); 
        goto END_ERROR;
    }

    if ((cap.capabilities & V4L2_CAP_STREAMING) == 0)
    {
        LOGE("Capture device does not support streaming i/o");
        goto END_ERROR;
    }

    if (mCameraDev == CAMERA_USB)
    {
        //initSonixUsbCameraDevice(halInfo->device_name);
        // try to support this format: NV21, YUYV
        // we do not support mjpeg camera now
        if (tryFmt(V4L2_PIX_FMT_NV21) == OK)
        {
            mCaptureFormat = V4L2_PIX_FMT_NV21;
        }
        else if (tryFmt(V4L2_PIX_FMT_H264) == OK)
        {
            mCaptureFormat = V4L2_PIX_FMT_H264;
        }
        else if(tryFmt(V4L2_PIX_FMT_MJPEG) == OK)
        {
            mCaptureFormat = V4L2_PIX_FMT_MJPEG;        // maybe usb camera
        }
        else if(tryFmt(V4L2_PIX_FMT_YUYV) == OK)
        {
            mCaptureFormat = V4L2_PIX_FMT_YUYV;     // maybe usb camera
        }
        else
        {
            LOGE("driver should surpport NV21/NV12 or YUYV format, but it not!");
            goto END_ERROR;
        }
        showformat(mCaptureFormat, "uvc mCaptureFormat");
    }
    else if (mCameraDev == CAMERA_CSI)
    {
        // uvc do not need to set input
        inp.index = halInfo->device_id;
        if (-1 == ioctl (mCameraFd, VIDIOC_S_INPUT, &inp))
        {
            LOGE("VIDIOC_S_INPUT error!");
            goto END_ERROR;
        }
        showformat(mCaptureFormat, "csi mCaptureFormat");
    }
    else if (mCameraDev == CAMERA_TVD)
    {
        mTVDecoder = new TVDecoder(mCameraFd);
        if (mTVDecoder == NULL) {
            LOGE("Failed to alloc TVDecoder(%s)!", strerror(errno));
            goto END_ERROR;
        }
        if (TVinStartDevice() != NO_ERROR) {
            LOGE("Start TV in device error!");
            goto END_ERROR;
        }
    }

    return NO_ERROR;

END_ERROR:
    if (mCameraFd >= 0)
    {
        mCameraDev == CAMERA_UNKNOWN;
        close(mCameraFd);
        mCameraFd = -1;
    }
    return UNKNOWN_ERROR;
}

void V4L2CameraDevice::closeCameraDev()
{
    LOGV();

    if (mCameraDev == CAMERA_TVD) {
        TVinStopDevice();
    }
    if (mCameraFd >= 0)
    {
        mCameraDev == CAMERA_UNKNOWN;
        close(mCameraFd);
        mCameraFd = -1;
    }

    //deinitSonixUsbCameraDevice();

}

status_t V4L2CameraDevice::v4l2SetVideoParams(int width, int height, int sub_w, int sub_h, uint32_t pix_fmt)
{
    struct v4l2_format format;

    LOGV("main:%dx%d, sub:%dx%d, pfmt: 0x%x", width, height, sub_w, sub_h, pix_fmt);
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width  = width;
    format.fmt.pix.height = height;
    if (mCaptureFormat == V4L2_PIX_FMT_YUYV)
    {
        format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    }
    else if(mCaptureFormat == V4L2_PIX_FMT_MJPEG)
    {
        format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    }
    else if (mCaptureFormat == V4L2_PIX_FMT_H264)
    {
        format.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
    }
    else
    {
        format.fmt.pix.pixelformat = pix_fmt;
    }
    format.fmt.pix.field = V4L2_FIELD_NONE;

    if (mCameraDev == CAMERA_CSI && mHalCameraInfo.fast_picture_mode)
    {
        if ((sub_w > 0 && sub_w < width) && (sub_h > 0 && sub_h < height)) {
            struct v4l2_pix_format sub_fmt;
            memset(&sub_fmt, 0, sizeof(sub_fmt));
            format.fmt.pix.subchannel = &sub_fmt;
            format.fmt.pix.subchannel->width = sub_w;
            format.fmt.pix.subchannel->height = sub_h;
            format.fmt.pix.subchannel->pixelformat = pix_fmt;
            format.fmt.pix.subchannel->field = V4L2_FIELD_NONE;
            format.fmt.pix.subchannel->rot_angle = mPreviewRotation;
            mSubChannelEnable = true;
            LOGV("sub_w=%d, sub_h=%d", sub_w, sub_h);
        } else {
            mSubChannelEnable = false;
        }
    }

    if (ioctl(mCameraFd, VIDIOC_S_FMT, &format) < 0)
    {
        LOGE("VIDIOC_S_FMT failed(%s)", strerror(errno));
        return UNKNOWN_ERROR;
    }

    mFrameWidth = format.fmt.pix.width;
    mFrameHeight = format.fmt.pix.height;

    if (mCameraDev == CAMERA_CSI && mHalCameraInfo.fast_picture_mode && mSubChannelEnable)
    {
        mThumbWidth = format.fmt.pix.subchannel->width;
        mThumbHeight = format.fmt.pix.subchannel->height;
    } else {
        mThumbWidth = 0;
        mThumbHeight = 0;
    }

    LOGV("camera params: main:%dx%d, sub:%dx%d, pfmt: %d, pfield: %d",
        mFrameWidth, mFrameHeight, mThumbWidth, mThumbHeight, pix_fmt, V4L2_FIELD_NONE);

    return NO_ERROR;
}

int V4L2CameraDevice::v4l2ReqBufs(int * buf_cnt)
{
    LOGV();
    int ret = UNKNOWN_ERROR;
    struct v4l2_requestbuffers rb;

    LOGV("TO VIDIOC_REQBUFS count: %d", *buf_cnt);

    memset(&rb, 0, sizeof(rb));
    rb.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    rb.memory = V4L2_MEMORY_MMAP;
    rb.count  = *buf_cnt;

    ret = ioctl(mCameraFd, VIDIOC_REQBUFS, &rb);
    if (ret < 0)
    {
        LOGE("VIDIOC_REQBUFS failed(%s)", strerror(errno));
        return ret;
    }

    *buf_cnt = rb.count;
    LOGV("VIDIOC_REQBUFS count: %d", *buf_cnt);

    return OK;
}

int V4L2CameraDevice::v4l2QueryBuf()
{
    LOGV();
    int ret = UNKNOWN_ERROR;
    struct v4l2_buffer buf;

    for (int i = 0; i < mRealBufferNum; i++)
    {
        memset (&buf, 0, sizeof (struct v4l2_buffer));
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;

        ret = ioctl (mCameraFd, VIDIOC_QUERYBUF, &buf);
        if (ret < 0)
        {
            LOGE("VIDIOC_QUERYBUF failed(%s)", strerror(errno));
            return ret;
        }

        mpV4l2Buf[i].memory = mmap (0, buf.length,
                                PROT_READ | PROT_WRITE, 
                                MAP_SHARED, 
                                mCameraFd, 
                                buf.m.offset);
        mMemMapLen = buf.length;
        LOGV("index: %d, mem: %x, len: %x, offset: %x", i, (int)mpV4l2Buf[i].memory, buf.length, buf.m.offset);
 
        if (mpV4l2Buf[i].memory == MAP_FAILED)
        {
            LOGE("mmap failed(%s)", strerror(errno));
            return -1;
        }

        // start with all buffers in queue
        ret = ioctl(mCameraFd, VIDIOC_QBUF, &buf);
        if (ret < 0)
        {
            LOGE("VIDIOC_QBUF failed(%s)", strerror(errno));
            return ret;
        }

#if 0
        if (mIsUsbCamera)       // star to do
        {
            int buffer_len = mFrameWidth * mFrameHeight * 3 / 2;

            mVideoBuffer.buf_vir_addr[i] = (unsigned int)MemAdapterPalloc(buffer_len);
            mVideoBuffer.buf_phy_addr[i] = (unsigned int)MemAdapterGetPhysicAddress((void*)mVideoBuffer.buf_vir_addr[i]);

            LOGV("video buffer: index: %d, vir: %x, phy: %x, len: %x", 
                    i, mVideoBuffer.buf_vir_addr[i], mVideoBuffer.buf_phy_addr[i], buffer_len);

            memset((void*)mVideoBuffer.buf_vir_addr[i], 0x10, mFrameWidth * mFrameHeight);
            memset((void*)(mVideoBuffer.buf_vir_addr[i] + mFrameWidth * mFrameHeight), 
                    0x80, mFrameWidth * mFrameHeight / 2);
        }
#endif
    }

    return OK;
}

int V4L2CameraDevice::v4l2StartStreaming()
{
    LOGV();
    int ret = UNKNOWN_ERROR;
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ret = ioctl (mCameraFd, VIDIOC_STREAMON, &type);
    if (ret < 0)
    {
        LOGE("VIDIOC_STREAMON failed(%s)", strerror(errno));
        return ret;
    }

    return OK;
}

int V4L2CameraDevice::v4l2StopStreaming()
{
    LOGV();
    int ret = UNKNOWN_ERROR;
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ret = ioctl (mCameraFd, VIDIOC_STREAMOFF, &type);
    if (ret < 0)
    {
        LOGE("VIDIOC_STREAMOFF failed(%s)", strerror(errno));
        return ret;
    }
    LOGV("V4L2Camera::v4l2StopStreaming OK");

    return OK;
}

int V4L2CameraDevice::v4l2UnmapBuf()
{
    LOGV();
    int ret = UNKNOWN_ERROR;

    for (int i = 0; i < mRealBufferNum; i++)
    {
        ret = munmap(mpV4l2Buf[i].memory, mMemMapLen);
        if (ret < 0)
        {
            LOGE("munmap failed(%s)", strerror(errno));
            return ret;
        }

        mpV4l2Buf[i].memory = NULL;
#if 0
        if (mVideoBuffer.buf_vir_addr[i] != 0)
        {
            MemAdapterPfree((void*)mVideoBuffer.buf_vir_addr[i]);

            mVideoBuffer.buf_phy_addr[i] = 0;
        }
#endif
    }
#if 0
    mVideoBuffer.buf_unused = NB_BUFFER;
    mVideoBuffer.read_id = 0;
    mVideoBuffer.read_id = 0;
#endif

    return OK;
}

void V4L2CameraDevice::releasePreviewFrame(int index)
{
    int ret = UNKNOWN_ERROR;
    struct v4l2_buffer buf;
    V4L2BUF_t *pV4l2Buf = &mpV4l2Buf[index].v4l2buf;

    Mutex::Autolock locker(&mReleaseBufferLock);
    //LOGV("index=%d, refcnt=%d", index, mV4l2buf[index].refCnt);

    // decrease buffer reference count first, if the reference count is no more than 0, release it.
    if (pV4l2Buf->refCnt > 0 && --pV4l2Buf->refCnt == 0)
    {
        if (pV4l2Buf->isDecodeSrc == 0) {
            memset(&buf, 0, sizeof(v4l2_buffer));
            buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = index;
            
            // LOGD("r ID: %d", buf.index);
            ret = ioctl(mCameraFd, VIDIOC_QBUF, &buf);
            if (ret != 0)
            {
                LOGE("VIDIOC_QBUF failed(%s)", strerror(errno));
            }
#if DEBUG_V4L2_CAMERA_USED_BUFFER_CNT
            if(mCameraDev == CAMERA_CSI) {
                if (mUsedBufCnt > 0) {
                    mUsedBufCnt--;
                } else {
                    LOGE("fatal error, mUsedBufCnt is 0");
                }
            }
#endif
        }
        else
        {
            releaseDecodeFrame(index);
        }
    }
}

int V4L2CameraDevice::getPreviewFrame(v4l2_buffer *buf)
{
    int ret = UNKNOWN_ERROR;

    buf->type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf->memory = V4L2_MEMORY_MMAP;

    ret = ioctl(mCameraFd, VIDIOC_DQBUF, buf);
    if (ret < 0)
    {
        LOGE("VIDIOC_DQBUF failed(%s)", strerror(errno));
        return ret;             // can not return false
    }
    if (buf->index >= (unsigned int)mRealBufferNum) {
        LOGE("fatal error! buf->index[%d]>mRealBufferNum[%d]!!", buf->index, mRealBufferNum);
        return -1;
    }
#if DEBUG_V4L2_CAMERA_USED_BUFFER_CNT
    if(mCameraDev == CAMERA_CSI) {
        if (mUsedBufCnt < mRealBufferNum) {
            mUsedBufCnt++;
        } else {
            LOGE("fatal error: mUsedBufCnt == %d", mRealBufferNum);
        }
    }
#endif
    return OK;
}

int V4L2CameraDevice::tryFmt(int format)
{   
    struct v4l2_fmtdesc fmtdesc;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    for(int i = 0; i < 12; i++)
    {
        fmtdesc.index = i;
        if (-1 == ioctl (mCameraFd, VIDIOC_ENUM_FMT, &fmtdesc))
        {
            break;
        }
        LOGV("format index = %d, name = %s, v4l2 pixel format = %x\n", i, fmtdesc.description, fmtdesc.pixelformat);

        if (fmtdesc.pixelformat == (__u32)format)
        {
            return OK;
        }
    }

    LOGW("tryFmt failed(%s)", strerror(errno));
    return -1;
}

int V4L2CameraDevice::tryFmtSize(int *width, int *height)
{
    int ret = -1;
    struct v4l2_format fmt;

    LOGV("%dx%d", *width, *height);


    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width  = *width;
    fmt.fmt.pix.height = *height;
    if (mCaptureFormat == V4L2_PIX_FMT_YUYV)
    {
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    }
    else if (mCaptureFormat == V4L2_PIX_FMT_MJPEG)
    {
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    }
    else if (mCaptureFormat == V4L2_PIX_FMT_H264)
    {
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
    }
    else
    {
        fmt.fmt.pix.pixelformat = mVideoFormat;
    }
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    ret = ioctl(mCameraFd, VIDIOC_TRY_FMT, &fmt);
    if (ret < 0)
    {
        LOGE("VIDIOC_TRY_FMT failed(%s)", strerror(errno));
        return ret;
    }

    // driver surpport this size
    *width = fmt.fmt.pix.width;
    *height = fmt.fmt.pix.height;

    return 0;
}

int V4L2CameraDevice::setFrameRate(int rate)
{
    LOGV("setFrameRate(%d)", rate);
    mFrameRate = rate;
    return OK;
}

int V4L2CameraDevice::getFrameRate(int *fps)
{
    LOGV();
    int ret = -1;

    struct v4l2_streamparm parms;
    parms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ret = ioctl (mCameraFd, VIDIOC_G_PARM, &parms);
    if (ret < 0)
    {
        LOGE("VIDIOC_G_PARM failed(%s)", strerror(errno));
        return ret;
    }

    int numerator = parms.parm.capture.timeperframe.numerator;
    int denominator = parms.parm.capture.timeperframe.denominator;

    LOGV("frame rate: numerator = %d, denominator = %d", numerator, denominator);

    if (numerator != 0
        && denominator != 0)
    {
        *fps = denominator / numerator;
        return 0;
    }
    else
    {
        LOGW("unsupported frame rate: %d/%d", denominator, numerator);
        *fps = 30;
        return -2;
    }
}

status_t V4L2CameraDevice::setPreviewRate()
{
    if (mFrameRate > 250) {
        mPreviewRate = 10;
    } else if (mFrameRate > 220) {
        mPreviewRate = 8;
    } else if (mFrameRate > 190) {
        mPreviewRate = 7;
    } else if (mFrameRate > 160) {
        mPreviewRate = 6;
    } else if (mFrameRate > 130) {
        mPreviewRate = 5;
    } else if (mFrameRate > 100) {
        mPreviewRate = 4;
    } else if (mFrameRate > 70) {
        mPreviewRate = 3;
    } else if (mFrameRate > 40) {
        mPreviewRate = 2;
    } else {
        mPreviewRate = 1;
    }
    mPreviewCnt = 0;
    LOGD("mFrameRate=%d, mPreviewRate=%d", mFrameRate, mPreviewRate);
    return NO_ERROR;
}

int V4L2CameraDevice::setPreviewFlip(int previewFlip)
{
    mPreviewFlip = previewFlip;
    return NO_ERROR;
}

int V4L2CameraDevice::setPreviewRotation(int rotation)
{
    mPreviewRotation = rotation;
    return NO_ERROR;
}

int V4L2CameraDevice::setImageEffect(int effect)
{
    LOGV("effect=%d", effect);
    int ret = -1;
    struct v4l2_control ctrl;

    if(mCameraDev == CAMERA_CSI){
        ctrl.id = V4L2_CID_COLORFX;
        ctrl.value = effect;
        ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
        if (ret < 0) {
            LOGE("VIDIOC_S_CTRL failed(%s)", strerror(errno));
        } else {
            LOGV("setImageEffect ok");
        }
    }
    return ret;
}

int V4L2CameraDevice::setWhiteBalance(int wb)
{
    struct v4l2_control ctrl;
    int ret = -1;

    LOGV("setWhiteBalance(%d)", wb);
    if(mCameraDev == CAMERA_CSI)
    {
        ctrl.id = V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE;
        ctrl.value = wb;
        ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
        
        if (ret < 0) {
            LOGE("VIDIOC_S_CTRL failed(%s)", strerror(errno));
        } else {
            LOGV("setWhiteBalance ok");
        }
    }
    
    return ret;
}

// set contrast
int V4L2CameraDevice::setContrastValue(int value)
{
    int ret = -1;
    struct v4l2_control ctrl;

    LOGV("setContrastValue(%d)", value);
    if(mCameraDev == CAMERA_CSI){
        ctrl.id = V4L2_CID_CONTRAST;
        ctrl.value = value;
        ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
        if (ret < 0) {
            LOGE("VIDIOC_S_CTRL failed(%s)", strerror(errno));
        } else {
            LOGV("setContrastValue ok");
        }
    }

    return ret;
}

status_t V4L2CameraDevice::getContrastCtrl(struct v4l2_queryctrl *ctrl)
{
    int ret = -1;

    if(mCameraDev == CAMERA_CSI){
        ctrl->id  = V4L2_CID_CONTRAST;
        ret = ioctl(mCameraFd, VIDIOC_QUERYCTRL, ctrl);
        if (ret < 0) {
            LOGE("VIDIOC_QUERYCTRL failed(%s)", strerror(errno));
        } else {
            LOGV("getContrastCtrl OK");
        }
    }
    return ret;
}

int V4L2CameraDevice::setBrightnessValue(int value)
{
    struct v4l2_control ctrl;
    int ret = -1;

    LOGV("setBrightnessValue(%d)", value);
    if(mCameraDev == CAMERA_CSI){
        ctrl.id = V4L2_CID_BRIGHTNESS;
        ctrl.value = value;
        ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
        if (ret < 0) {
            LOGE("VIDIOC_S_CTRL failed(%s)", strerror(errno));
        } else {
            LOGV("setBrightnessValue OK");
        }
    }
    return ret;
}

int V4L2CameraDevice::getBrightnessCtrl(struct v4l2_queryctrl *ctrl)
{
    int ret = -1;

    if(mCameraDev == CAMERA_CSI){
        ctrl->id  = V4L2_CID_BRIGHTNESS;
        ret = ioctl(mCameraFd, VIDIOC_QUERYCTRL, ctrl);
        if (ret < 0) {
            LOGE("VIDIOC_QUERYCTRL failed(%s)", strerror(errno));
        } else {
            LOGV("getBrightnessCtrl OK");
        }
    }
    return ret;
}

int V4L2CameraDevice::setSaturationValue(int value)
{
    struct v4l2_control ctrl;
    int ret = -1;

    LOGV("setSaturationValue(%d)", value);
    if(mCameraDev == CAMERA_CSI){
        ctrl.id = V4L2_CID_SATURATION;
        ctrl.value = value;
        ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
        if (ret < 0) {
            LOGE("VIDIOC_S_CTRL failed(%s)", strerror(errno));
        } else {
            LOGV("setSaturationValue OK");
        }
    }
    return ret;
}

int V4L2CameraDevice::getSaturationCtrl(struct v4l2_queryctrl *ctrl)
{
    int ret = -1;

    if(mCameraDev == CAMERA_CSI){
        ctrl->id  = V4L2_CID_SATURATION;
        ret = ioctl(mCameraFd, VIDIOC_QUERYCTRL, ctrl);
        if (ret < 0) {
            LOGE("VIDIOC_QUERYCTRL failed(%s)", strerror(errno));
        } else {
            LOGV("getSaturationCtrl OK");
        }
    }
    return ret;
}

int V4L2CameraDevice::setHueValue(int value)
{
    struct v4l2_control ctrl;
    int ret = -1;

    LOGV("setHueValue(%d)", value);
    if(mCameraDev == CAMERA_CSI){
        ctrl.id = V4L2_CID_HUE;
        ctrl.value = value;
        ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
        if (ret < 0) {
            LOGE("VIDIOC_S_CTRL failed(%s)", strerror(errno));
        } else {
            LOGV("setHueValue OK");
        }
    }
    return ret;
}

int V4L2CameraDevice::getHueCtrl(struct v4l2_queryctrl *ctrl)
{
    int ret = -1;

    if(mCameraDev == CAMERA_CSI){
        ctrl->id  = V4L2_CID_HUE;
        ret = ioctl(mCameraFd, VIDIOC_QUERYCTRL, ctrl);
        if (ret < 0) {
            LOGE("VIDIOC_QUERYCTRL failed(%s)", strerror(errno));
        } else {
            LOGV("getHueCtrl OK");
        }
    }
    return ret;
}

int V4L2CameraDevice::setSharpnessValue(int value)
{
    struct v4l2_control ctrl;
    int ret = -1;

    LOGV("setSharpnessValue(%d)", value);
    if(mCameraDev == CAMERA_CSI){
        ctrl.id = V4L2_CID_SHARPNESS;
        ctrl.value = value;
        ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
        if (ret < 0) {
            LOGE("VIDIOC_S_CTRL failed(%s)", strerror(errno));
        } else {
            LOGV("setSharpnessValue OK");
        }
    }
    return ret;
}

int V4L2CameraDevice::setHflip(int value)
{
    struct v4l2_control ctrl;
    int ret = -1;
    
    ctrl.id = V4L2_CID_HFLIP_THUMB;
    ctrl.value = value;
    LOGD ("V4L2CameraDevice::setHflip value=%d\n", value);
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
    {
        //setSharpnessValue failed
        LOGE ("setHflip thumb failed, %s\n", strerror(errno));
    }
    ctrl.id = V4L2_CID_HFLIP;
    ctrl.value = value;
    LOGD ("V4L2CameraDevice::setHflip value=%d\n", value);
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
    {
        //setSharpnessValue failed
        LOGE ("setHflip failed, %s\n", strerror(errno));
    }
    return ret;
}


int V4L2CameraDevice::setVflip(int value)
{
    struct v4l2_control ctrl;
    int ret = -1;
    
    ctrl.id = V4L2_CID_VFLIP_THUMB;
    ctrl.value = value;
    LOGD ("V4L2CameraDevice::setVflip value=%d\n", value);
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
    {
        //setSharpnessValue failed
        LOGE ("setVflip thumb failed111, %s\n", strerror(errno));
    }
    ctrl.id = V4L2_CID_VFLIP;
    ctrl.value = value;
    LOGD ("V4L2CameraDevice::setVflip value=%d\n", value);
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
    {
        //setSharpnessValue failed
        LOGE ("setVflip failed111, %s\n", strerror(errno));
    }

    return ret;
}

status_t V4L2CameraDevice::getSharpnessCtrl(struct v4l2_queryctrl *ctrl)
{
    int ret = -1;

    if(mCameraDev == CAMERA_CSI) {
        ctrl->id  = V4L2_CID_SHARPNESS;
        ret = ioctl(mCameraFd, VIDIOC_QUERYCTRL, ctrl);
        if (ret < 0) {
            LOGE("VIDIOC_QUERYCTRL failed(%s)", strerror(errno));
        } else {
            LOGV("getSharpnessCtrl OK");
        }
    }
    return ret;
}

int V4L2CameraDevice::setTakePictureCtrl()
{
    LOGV();
    struct v4l2_control ctrl;
    int ret = -1;

    if(mCameraDev == CAMERA_CSI) {
        ctrl.id = V4L2_CID_TAKE_PICTURE;
        ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
        if (ret < 0) {
            LOGE("VIDIOC_S_CTRL failed(%s)", strerror(errno));
        } else {
            LOGV("setTakePictureCtrl ok");
        }
    }
    return ret;
}

int V4L2CameraDevice::setPowerLineFrequencyValue(int value)
{
    struct v4l2_control ctrl;
    int ret = -1;

    LOGV("setPowerLineFrequencyValue(%d)", value);
    if(mCameraDev == CAMERA_CSI){
        ctrl.id = V4L2_CID_POWER_LINE_FREQUENCY;
        ctrl.value = value;
        ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
        if (ret < 0) {
            LOGE("VIDIOC_S_CTRL failed(%s)", strerror(errno));
        } else {
            LOGV("setPowerLineFrequencyValue OK");
        }
    }
    return ret;
}
// ae mode
int V4L2CameraDevice::setExposureMode(int mode)
{
    LOGV("mode[%d]", mode);
    int ret = -1;
    struct v4l2_control ctrl;
    
    if(mCameraDev == CAMERA_CSI){
        ctrl.id = V4L2_CID_EXPOSURE_AUTO;
        ctrl.value = mode;
        ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
        if (ret < 0) {
            LOGE("VIDIOC_S_CTRL failed(%s)", strerror(errno));
        } else {
            LOGV("setExposureMode ok");
        }
    }
    return ret;
}

// ae compensation
int V4L2CameraDevice::setExposureCompensation(int val)
{
    LOGV("value[%d]", val);
    int ret = -1;
    struct v4l2_control ctrl;
    
    if(mCameraDev == CAMERA_CSI){
        ctrl.id = V4L2_CID_AUTO_EXPOSURE_BIAS;
        ctrl.value = val;
        ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
        if (ret < 0) {
            LOGE("VIDIOC_S_CTRL failed(%s)", strerror(errno));
        } else {
            LOGV("setExposureCompensation ok");
        }
    }
    return ret;
}

// ae compensation
int V4L2CameraDevice::setExposureWind(int num, void *wind)
{
    LOGV("num[%d], wind[%p]", num, wind);
    int ret = -1;
    struct v4l2_control ctrl;
    
    if(mCameraDev == CAMERA_CSI){
        ctrl.id = V4L2_CID_AUTO_EXPOSURE_WIN_NUM;
        ctrl.value = num;
        ctrl.user_pt = (unsigned int)wind;
        ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
        if (ret < 0) {
            LOGE("VIDIOC_S_CTRL failed(%s)", strerror(errno));
        } else {
            LOGV("setExposureWind ok");
        }
    }
    return ret;
}

// flash mode
int V4L2CameraDevice::setFlashMode(int mode)
{
    LOGV("mode=%d", mode);
    int ret = -1;
    struct v4l2_control ctrl;

#if 0
    ctrl.id = V4L2_CID_CAMERA_FLASH_MODE;
    ctrl.value = mode;
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
        LOGV("setFlashMode failed, %s", strerror(errno));
    else 
        LOGV("setFlashMode ok");
#endif
    return ret;
}

// af init
int V4L2CameraDevice::setAutoFocusInit()
{
    LOGV();
    int ret = -1;
    struct v4l2_control ctrl;
    
    if(mCameraDev == CAMERA_CSI){
        ctrl.id = V4L2_CID_AUTO_FOCUS_INIT;
        ctrl.value = 0;
        ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
        if (ret < 0) {
            LOGE("VIDIOC_S_CTRL failed(%s)", strerror(errno));
        } else {
            LOGV("setAutoFocusInit ok");
        }
    }
    return ret;
}

// af release
int V4L2CameraDevice::setAutoFocusRelease()
{
    LOGV();
    int ret = -1;
    struct v4l2_control ctrl;
    
    if(mCameraDev == CAMERA_CSI){
        ctrl.id = V4L2_CID_AUTO_FOCUS_RELEASE;
        ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
        if (ret < 0) {
            LOGE("VIDIOC_S_CTRL failed(%s)", strerror(errno));
        } else {
            LOGV("setAutoFocusRelease ok");
        }
    }
    return ret;
}

// af range
int V4L2CameraDevice::setAutoFocusRange(int af_range)
{
    LOGV("range=%d", af_range);
    int ret = -1;
    struct v4l2_control ctrl;
#if 0
    ctrl.id = V4L2_CID_FOCUS_AUTO;
    ctrl.value = 1;
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
        LOGV("setAutoFocusRange id V4L2_CID_FOCUS_AUTO failed, %s", strerror(errno));
    else 
        LOGV("setAutoFocusRange id V4L2_CID_FOCUS_AUTO ok");

    ctrl.id = V4L2_CID_AUTO_FOCUS_RANGE;
    ctrl.value = af_range;
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
        LOGV("setAutoFocusRange id V4L2_CID_AUTO_FOCUS_RANGE failed, %s", strerror(errno));
    else 
        LOGV("setAutoFocusRange id V4L2_CID_AUTO_FOCUS_RANGE ok");
#endif
    return ret;
}

// af wind
int V4L2CameraDevice::setAutoFocusWind(int num, void *wind)
{
    LOGV("num=%d, wind=%p", num, wind);
    int ret = -1;
    struct v4l2_control ctrl;
#if 0
    ctrl.id = V4L2_CID_AUTO_FOCUS_WIN_NUM;
    ctrl.value = num;
    ctrl.user_pt = (unsigned int)wind;
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0) {
        LOGE("setAutoFocusCtrl failed, %s", strerror(errno));
        LOGE("AWHLABEL#camera#setAutoFocusWind-FAIL!\n");
    }
    else 
        LOGV("setAutoFocusCtrl ok");
#endif
    return ret;
}

// af start
int V4L2CameraDevice::setAutoFocusStart()
{
    LOGV();
    int ret = -1;
    struct v4l2_control ctrl;
#if 0
    ctrl.id = V4L2_CID_AUTO_FOCUS_START;
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0) {
        LOGE("setAutoFocusStart failed, %s", strerror(errno));
        LOGE("AWHLABEL#camera#setAutoFocusStart-FAIL!\n");
    }
    else 
        LOGV("setAutoFocusStart ok");
#endif
    return ret;
}

// af stop
int V4L2CameraDevice::setAutoFocusStop()
{
    LOGV();
    int ret = -1;
    struct v4l2_control ctrl;

#if 0
    ctrl.id = V4L2_CID_AUTO_FOCUS_STOP;
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0) {
        LOGE("setAutoFocusStart failed, %s", strerror(errno));
        LOGE("AWHLABEL#camera#setAutoFocusStop-FAIL!\n");
    }
    else 
        LOGV("setAutoFocusStart ok");
#endif
    return ret;
}

// get af statue
int V4L2CameraDevice::getAutoFocusStatus()
{
    LOGV();
    int ret = -1;
    struct v4l2_control ctrl;
#if 0
    if (mCameraFd < 0)
    {
        return 0xFF000000;
    }

    ctrl.id = V4L2_CID_AUTO_FOCUS_STATUS;
    ret = ioctl(mCameraFd, VIDIOC_G_CTRL, &ctrl);
    if (ret >= 0)
    {
        //LOGV("getAutoFocusCtrl ok");
    }
#endif
    return ret;
}

int V4L2CameraDevice::set3ALock(int lock)
{
    LOGV("lock[%d]", lock);
    int ret = -1;
    struct v4l2_control ctrl;
    
    ctrl.id = V4L2_CID_3A_LOCK;
    ctrl.value = lock;
    ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0) {
        LOGE("VIDIOC_S_CTRL failed(%s)", strerror(errno));
    } else {
        LOGV("set3ALock ok");
    }

    return ret;
}

int V4L2CameraDevice::v4l2setCaptureParams(int framerate)
{
    LOGV("framerate=%d", framerate);
    int ret = -1;
    
    struct v4l2_streamparm params;
    params.parm.capture.timeperframe.numerator = 1;
    params.parm.capture.timeperframe.denominator = framerate;
    params.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (mTakePictureState == TAKE_PICTURE_NORMAL)
    {
        params.parm.capture.capturemode = V4L2_MODE_IMAGE;
    }
    else
    {
        params.parm.capture.capturemode = V4L2_MODE_VIDEO;
    }

    LOGV("VIDIOC_S_PARM mFrameRate: %d, capture mode: %d", framerate, params.parm.capture.capturemode);

    ret = ioctl(mCameraFd, VIDIOC_S_PARM, &params);
    if (ret < 0) {
        LOGE("fd(%d) VIDIOC_S_PARM failed(%s)", mCameraFd, strerror(errno));
    } else {
        LOGV("v4l2setCaptureParams ok");
    }

    return ret;
}

int V4L2CameraDevice::enumSize(char * pSize, int len)
{
    struct v4l2_frmsizeenum size_enum;
    size_enum.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    size_enum.pixel_format = mCaptureFormat;

    if (pSize == NULL)
    {
        LOGE("error input params");
        return -1;
    }

    char str[16];
    memset(str, 0, 16);
    memset(pSize, 0, len);
    
    for(int i = 0; i < 20; i++)
    {
        size_enum.index = i;
        if (-1 == ioctl (mCameraFd, VIDIOC_ENUM_FRAMESIZES, &size_enum))
        {
            break;
        }
        // LOGV("format index = %d, size_enum: %dx%d", i, size_enum.discrete.width, size_enum.discrete.height);
        sprintf(str, "%dx%d", size_enum.discrete.width, size_enum.discrete.height);
        if (i != 0)
        {
            strcat(pSize, ",");
        }
        strcat(pSize, str);
    }

    return OK;
}

int V4L2CameraDevice::getFullSize(int * full_w, int * full_h)
{
    struct v4l2_frmsizeenum size_enum;
    size_enum.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    size_enum.pixel_format = mCaptureFormat;
    size_enum.index = 0;
    if (-1 == ioctl (mCameraFd, VIDIOC_ENUM_FRAMESIZES, &size_enum))
    {
        LOGE("VIDIOC_ENUM_FRAMESIZES failed(%s)", strerror(errno));
        return -1;
    }
    
    *full_w = size_enum.discrete.width;
    *full_h = size_enum.discrete.height;

    LOGV("getFullSize: %dx%d", *full_w, *full_h);
    
    return OK;
}

int V4L2CameraDevice::getSuitableThumbScale(int full_w, int full_h)
{
    int scale = 1;
    if(mIsThumbUsedForVideo == true)
    {
        scale = 2;
    }
    if ((full_w == 4000)
        && (full_h == 3000))
    {
        return 4;   // 1000x750
    }
    else if ((full_w == 3264)
        && (full_h == 2448))
    {
        return 2;   // 1632x1224
    }
    else if ((full_w == 2592)
        && (full_h == 1936))
    {
        return 2;   // 1296x968
    }
    else if ((full_w == 1280)
        && (full_h == 960))
    {
        return 1 * scale;   // 1280x960
    }
    else if ((full_w == 1920)
        && (full_h == 1080))
    {
        return 2;   // 960x540
    }
    else if ((full_w == 1280)
        && (full_h == 720))
    {
        
        return 1 * scale;   // 1280x720
    }
    else if ((full_w == 640)
        && (full_h == 480))
    {
        return 1;   // 640x480
    }

    LOGW("getSuitableThumbScale unknown size: %dx%d", full_w, full_h);
    return 1;       // failed
}

// get horizontal visual angle
int V4L2CameraDevice::getHorVisualAngle()
{
    LOGV();
    int ret = -1;
    struct v4l2_control ctrl;

    ctrl.id = V4L2_CID_HOR_VISUAL_ANGLE;
    ret = ioctl(mCameraFd, VIDIOC_G_CTRL, &ctrl);
    if (ret < 0) {
        LOGE("VIDIOC_G_CTRL failed(%s)", strerror(errno));
        return ret;
    }
    return ctrl.value;
}

// get vertical visual angle
int V4L2CameraDevice::getVerVisualAngle()
{
    LOGV();
    int ret = -1;
    struct v4l2_control ctrl;

    ctrl.id = V4L2_CID_VER_VISUAL_ANGLE;
    ret = ioctl(mCameraFd, VIDIOC_G_CTRL, &ctrl);
    if (ret < 0) {
        LOGE("VIDIOC_G_CTRL failed(%s)", strerror(errno));
        return ret;
    }
    return ctrl.value;
}

// get pic var
int V4L2CameraDevice::getPicVar()
{
    //LOGV();
    int ret = -1;
    if(mCameraDev == CAMERA_CSI)
    {
        struct v4l2_control ctrl;
        ctrl.id = V4L2_CID_GAIN;
        ret = ioctl(mCameraFd, VIDIOC_G_CTRL, &ctrl);
        if (ret < 0) {
            LOGE("VIDIOC_G_CTRL failed(%s)", strerror(errno));
            return 0;
        }
        return ctrl.value;
    }
    else
    {
        return 0;
    }
}

void V4L2CameraDevice::setVideoBitRate(int32_t bitRate)
{
    LOGV("bitrate=%d", bitRate);
    if (mSonixUsbCameraDevice != NULL) {
        mSonixUsbCameraDevice->setVideoBitRate(bitRate);
    }
}

bool V4L2CameraDevice::isDualStreamDevice()
{
    if (mSonixUsbCameraDevice != NULL)
        return true;
    else
        return false;
}

int V4L2CameraDevice::setVideoSize(int w, int h)
{
    int ret;

    LOGV("%dx%d", w, h);
    if (mVideoWidth != w || mVideoHeight != h) {
        if (mSonixUsbCameraDevice != NULL) {
            ret = mSonixUsbCameraDevice->setVideoSize(w, h);
            if (ret < 0) {
                LOGE("set video size failed.");
                return -1;
            }
        }
        mVideoWidth = w;
        mVideoHeight = h;
    }
    
    return 0;
}

int V4L2CameraDevice::getCaptureFormat()
{
    if (mSonixUsbCameraDevice != NULL) {
        if (mSonixUsbCameraDevice->isUseH264DevRec())
            return V4L2_PIX_FMT_H264;
        else
            return  mCaptureFormat;
    } else {
        return mCaptureFormat;
    }
}

int V4L2CameraDevice::getVideoFormat()
{
    return mVideoFormat;
}

status_t V4L2CameraDevice::startRecording()
{
    LOGE("startRecording");
    if (mSonixUsbCameraDevice != NULL) {
    }
    return NO_ERROR;    
}

status_t V4L2CameraDevice::stopRecording()
{
    LOGE("stopRecording");
    if (mSonixUsbCameraDevice != NULL) {
    }
    return NO_ERROR;
}

#ifdef MOTION_DETECTION_ENABLE
status_t V4L2CameraDevice::AWMDInitialize()
{
    if (mCameraDev != CAMERA_CSI) {
        LOGW("Only csi camera support motion detection!");
        return NO_ERROR;
    }
    if (mMotionDetect != NULL) {
        LOGW("AWMD is already running!");
        return NO_ERROR;
    }

    mMotionDetect = new MotionDetect(mCallbackNotifier);
    if (mMotionDetect == NULL) {
        LOGE("Failed to alloc MotionDetect!");
        return NO_MEMORY;
    }
    if (mMotionDetect->initialize(mFrameWidth, mFrameHeight, mThumbWidth, mThumbHeight, mFrameRate) != NO_ERROR) {
        LOGE("Failed to initialize MotionDetect!");
        delete mMotionDetect;
        mMotionDetect = NULL;
        return UNKNOWN_ERROR;
    }
    AWMDEnable(true);
    return NO_ERROR;
}

status_t V4L2CameraDevice::AWMDDestroy()
{
    if(mMotionDetect == NULL) {
        return NO_ERROR;
    }

    AWMDEnable(false);
    mMotionDetect->destroy();
    delete mMotionDetect;
    mMotionDetect = NULL;
    return NO_ERROR;
}

status_t V4L2CameraDevice::AWMDEnable(bool enable)
{
    if(mMotionDetect != NULL) {
        return mMotionDetect->enable(enable);
    }
    return INVALID_OPERATION;
}

status_t V4L2CameraDevice::AWMDSetSensitivityLevel(int level)
{
    
    if(mMotionDetect != NULL) {
        return mMotionDetect->setSensitivityLevel(level);
    }
    return INVALID_OPERATION;
}
#endif

#ifdef WATERMARK_ENABLE
void V4L2CameraDevice::getTimeString(char *str, int len)
{
    time_t timep;
    struct tm *p;
    int year, month, day, hour, min, sec;
    
    time(&timep);
    p = localtime(&timep);
    year = p->tm_year + 1900;
    month = p->tm_mon + 1;
    day = p->tm_mday;
    hour = p->tm_hour;
    min = p->tm_min;
    sec = p->tm_sec;

    snprintf(str, len, "%d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, min, sec);
}

status_t V4L2CameraDevice::setWaterMark(int enable, const char *str)
{
    LOGV("setWaterMark(%d, \"%s\")", enable, str);
    if (enable) {
        if (!mWaterMarkEnable) {
            LOGD("width=%d, height=%d", mFrameWidth, mFrameHeight);
            int height, sub_h;
            if (mFrameHeight < 1080) {
                height = 540;
                sub_h = 540;
            } else {
                height = 540;
                sub_h = 540;
            }
            pthread_mutex_lock(&mWaterMarkLock);
            mWaterMarkCtrlRec = initialwaterMark(height);
            if (NULL == mWaterMarkCtrlRec) {
                pthread_mutex_unlock(&mWaterMarkLock);
                LOGE("Fail to initialize record water mark!");
                return NO_MEMORY;
            }
#ifndef NO_PREVIEW_WATERMARK
            if (mHalCameraInfo.fast_picture_mode) {
                if (height == sub_h) {
                    mWaterMarkCtrlPrev = mWaterMarkCtrlRec;
                } else {
                    mWaterMarkCtrlPrev = initialwaterMark(sub_h);
                    if (NULL == mWaterMarkCtrlPrev) {
                        LOGE("Fail to initialize preview water mark!");
                    }
                }
            }
#endif
            pthread_mutex_unlock(&mWaterMarkLock);
            mWaterMarkEnable = true;
        }
        if (str != NULL) {
            pthread_mutex_lock(&mWaterMarkLock);
            if (mWaterMarkCtrlRec != NULL) {
                waterMarkSetMultiple(mWaterMarkCtrlRec, str);
            }
            if (mWaterMarkCtrlPrev != NULL) {
                waterMarkSetMultiple(mWaterMarkCtrlPrev, str);
            }
            pthread_mutex_unlock(&mWaterMarkLock);
        }
    } else {
        if (mWaterMarkEnable) {
            mWaterMarkEnable = false;
            pthread_mutex_lock(&mWaterMarkLock);
            if (mWaterMarkCtrlRec != NULL) {
                if (mWaterMarkCtrlRec == mWaterMarkCtrlPrev) {
                    mWaterMarkCtrlPrev = NULL;
                }
                releaseWaterMark(mWaterMarkCtrlRec);
                mWaterMarkCtrlRec = NULL;
            }
            if (mWaterMarkCtrlPrev != NULL) {
                releaseWaterMark(mWaterMarkCtrlPrev);
                mWaterMarkCtrlPrev = NULL;
            }
            pthread_mutex_unlock(&mWaterMarkLock);
        }
    }
    return NO_ERROR;
}
#endif

int V4L2CameraDevice::getInputSource()
{
    return (int)mCameraDev;
}

int V4L2CameraDevice::getTVinSystem()
{
    if (mTVDecoder == NULL) {
        LOGE("Not TV in device!");
        return -1;
    }
    return mTVDecoder->getSysValue();
}

status_t V4L2CameraDevice::TVinresetDevice()
{
    int width;
    int height;

    if (v4l2StopStreaming() != NO_ERROR)
    {
        LOGE("v4l2StopStreaming error");
        return UNKNOWN_ERROR;
    }

    if (v4l2UnmapBuf() != NO_ERROR)
    {
        LOGE("v4l2UnmapBuf error");
        return UNKNOWN_ERROR;
    }

    if (mTVDecoder->v4l2SetVideoParams(&width, &height) != NO_ERROR)
    {
        LOGE("v4l2SetVideoParams error");
        return UNKNOWN_ERROR;
    }
    if (v4l2ReqBufs(&mRealBufferNum) != NO_ERROR)
    {
        LOGE("v4l2ReqBufs error");
        return UNKNOWN_ERROR;
    }
    if (v4l2QueryBuf() != NO_ERROR)
    {
        LOGE("v4l2QueryBuf error");
        return UNKNOWN_ERROR;
    }
    if (v4l2StartStreaming() != NO_ERROR)
    {
        LOGE("v4l2StartStreaming error");
        return UNKNOWN_ERROR;
    }

    if (mFrameWidth != width || mFrameHeight != height) {
        mFrameWidth = width;
        mFrameHeight = height;
        mCameraHardware->resetCaptureSize(mFrameWidth, mFrameHeight);
    }

    return NO_ERROR;
}

status_t V4L2CameraDevice::TVinStartDevice()
{
    if (mTVDecoder->v4l2SetVideoParams(&mFrameWidth, &mFrameHeight) != NO_ERROR) {
        LOGE("v4l2SetVideoParams error!");
        return UNKNOWN_ERROR;
    }
    LOGV("width=%d, height=%d", mFrameWidth, mFrameHeight);
    int buf_cnt = mReqBufferNum;
    if (v4l2ReqBufs(&buf_cnt) != NO_ERROR) {
        LOGE("v4l2ReqBufs error!");
        return UNKNOWN_ERROR;
    }
    mRealBufferNum = buf_cnt;

    // v4l2 query buffers
    if (v4l2QueryBuf() != NO_ERROR) {
        LOGE("v4l2ReqBufs error!");
        return UNKNOWN_ERROR;
    }
    
    // stream on the v4l2 device
    if (v4l2StartStreaming() != NO_ERROR) {
        LOGE("v4l2ReqBufs error!");
        return UNKNOWN_ERROR;
    }

    mCameraHardware->resetCaptureSize(mFrameWidth, mFrameHeight);

    return NO_ERROR;
}

status_t V4L2CameraDevice::TVinStopDevice()
{
    v4l2StopStreaming();
    v4l2UnmapBuf();

    return NO_ERROR;
}

status_t V4L2CameraDevice::getUnusedPicBuffer(int *index)
{
    Mutex::Autolock autoLock(mVideoPicBufLock);
    int firstIdx = -1;
    int nUnusedNum = 0;
    for (int i = 0; i < mRealBufferNum; i++) {
        if (mpVideoPicBuf[i].isUse == 0) {
            nUnusedNum++;
            if(i<=mReleasePicBufIdx) {
                if(-1 == firstIdx)
                {
                    firstIdx = i;
                }
                continue;
            }
            *index = i;
            return NO_ERROR;
        }
    }
    if(firstIdx>=0)
    {
        if(1==nUnusedNum)
        {
            LOGD("use only one unused VideoPicBuf[%d], [%d]!", firstIdx, mReleasePicBufIdx);
        }
        *index = firstIdx;
        return NO_ERROR;
    }
    return UNKNOWN_ERROR;
}

status_t V4L2CameraDevice::getDecodeFrame(int index)
{
    VideoPictureBuffer *pvbuf = mpVideoPicBuf + index;
    pvbuf->picture[0] = libveGetFrame(mDecHandle, 0);
    if (pvbuf->picture[0] == NULL) {
        LOGE("Failed to get main picture!");
        return UNKNOWN_ERROR;
    }
    if (mSubChannelEnable) {
        pvbuf->picture[1] = libveGetFrame(mDecHandle, 1);
        if (pvbuf->picture[1] == NULL) {
            libveReleaseFrame(mDecHandle, pvbuf->picture[0]);
            LOGE("Failed to get sub picture!");
            return UNKNOWN_ERROR;
        }
    }
    //LOGD("index=%d", index);
    Mutex::Autolock autoLock(mVideoPicBufLock);
    pvbuf->isUse = 1;
    return NO_ERROR;
}

status_t V4L2CameraDevice::releaseDecodeFrame(int index)
{
    VideoPictureBuffer *pvbuf = mpVideoPicBuf + index;
    status_t ret = NO_ERROR;
    if (libveReleaseFrame(mDecHandle, pvbuf->picture[0]) != 0) {
        LOGE("libveReleaseFrame main fail!");
        ret = UNKNOWN_ERROR;
    }
    if (mSubChannelEnable) {
        if (libveReleaseFrame(mDecHandle, pvbuf->picture[1]) != 0) {
            LOGE("libveReleaseFrame sub fail!");
            ret = UNKNOWN_ERROR;
        }
    }
    //LOGD("<F:%s, L:%d>index=%d", index);
    Mutex::Autolock autoLock(mVideoPicBufLock);
    pvbuf->isUse = 0;
    mReleasePicBufIdx = index;
    return ret;
}

status_t V4L2CameraDevice::releasePreviewFrameDirectly(int index)
{
    struct v4l2_buffer buf;

    memset(&buf, 0, sizeof(v4l2_buffer));
    buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = index;
    if (ioctl(mCameraFd, VIDIOC_QBUF, &buf) != 0) {
        LOGE("VIDIOC_QBUF failed(%s)", strerror(errno));
        return UNKNOWN_ERROR;
    }
    return NO_ERROR;
}

#ifdef ADAS_ENABLE
status_t V4L2CameraDevice::adasInitialize()
{
    if (mAdas != NULL) {
        LOGW("ADAS is already running!");
        return NO_ERROR;
    }
    memset(mRoiArea, 0, sizeof(Venc_ROI_Config)*ROI_AREA_NUM);
    mAdas = new CameraAdas(mCallbackNotifier, this);
    if (mAdas == NULL) {
        LOGE("Failed to new CameraAdas!");
        return NO_MEMORY;
    }
    mAdas->setLaneLineOffsetSensity(mAdasLaneLineOffsetSensity);
    mAdas->setDistanceDetectLevel(mAdasDistanceDetectLevel);
    mAdas->setCarSpeed(mAdasCarSpeed);
    status_t ret = mAdas->initialize(mFrameWidth, mFrameHeight, mThumbWidth, mThumbHeight, mFrameRate);
    if (ret != NO_ERROR) {
        LOGE("Adas initialize error!");
        delete mAdas;
        return ret;
    }
    return NO_ERROR;
}

status_t V4L2CameraDevice::adasDestroy()
{
    if (mAdas == NULL) {
        return NO_ERROR;
    }
    Mutex::Autolock locker(&mAlgorithmLock);
    mAdas->destroy();
    delete mAdas;
    mAdas = NULL;
    memset(mRoiArea, 0, sizeof(Venc_ROI_Config)*ROI_AREA_NUM);
    return NO_ERROR;
}

status_t V4L2CameraDevice::adasSetLaneLineOffsetSensity(int level)
{
    mAdasLaneLineOffsetSensity = level;
    if (mAdas != NULL) {
        mAdas->setLaneLineOffsetSensity(level);
    }
    return NO_ERROR;
}

status_t V4L2CameraDevice::adasSetDistanceDetectLevel(int level)
{
    mAdasDistanceDetectLevel = level;
    if (mAdas != NULL) {
        mAdas->setDistanceDetectLevel(level);
    }
    return NO_ERROR;
}

status_t V4L2CameraDevice::adasSetMode(int mode)
{
    if (mAdas != NULL) {
        mAdas->adasSetMode(mode);
    }
    return NO_ERROR;
}

status_t V4L2CameraDevice::adasSetTipsMode(int mode)
{
    if (mAdas != NULL) {
        mAdas->adasSetTipsMode(mode);
    }
    return NO_ERROR;
}

status_t V4L2CameraDevice::adasSetCarSpeed(int speed)
{
    float spd = (float)speed / 10000;
    mAdasCarSpeed = spd;
    if (mAdas != NULL) {
        mAdas->setCarSpeed(spd);
    }
    return NO_ERROR;
}

status_t V4L2CameraDevice::adasSetAudioVol(int val)
{
    float vol = (float)val / 10;
    CameraAdas::setAdasAudioVol(vol);
    return NO_ERROR;
}

status_t V4L2CameraDevice::adasSetGsensorData(int val0, float val1, float val2)
{
    if (mAdas != NULL) {
        mAdas->adasSetGsensorData(val0,val1,val2);
    }
    return NO_ERROR;
}

status_t V4L2CameraDevice::adasSetRoiArea(void *adas)
{
    ADASOUT_IF *adasOutIf = (ADASOUT_IF*)adas;
    int platesNum = adasOutIf->adasOut->plates.Num;
    PLATE *plateP = adasOutIf->adasOut->plates.plateP;
    Mutex::Autolock _l(mRoiLock);

    memset(mRoiArea, 0, sizeof(Venc_ROI_Config)*ROI_AREA_NUM);

//    LOGV("adasSetRoiArea: platesNum is %d", platesNum);

    if (platesNum > 0) {
        if (platesNum > ROI_AREA_NUM) platesNum = ROI_AREA_NUM;

        for(int i = 0; i< platesNum; i++) {
            mRoiArea[i].bEnable         = 1;
            mRoiArea[i].index           = i;
            mRoiArea[i].nQPoffset       = 10;
            mRoiArea[i].sRect.left      = plateP->idx.x;
            mRoiArea[i].sRect.top       = plateP->idx.y;
            mRoiArea[i].sRect.width     = plateP->idx.width;
            mRoiArea[i].sRect.height    = plateP->idx.height;
            plateP++;
        }
    }

    return OK;
}
#endif

#ifdef QRDECODE_ENABLE
status_t V4L2CameraDevice::setQrDecode(bool enable)
{
    if (enable) {
        if (mpQrDecode != NULL) {
            return NO_ERROR;
        }
        mpQrDecode = new CameraQrDecode(mCallbackNotifier);
        if (mpQrDecode == NULL) {
            LOGE("Alloc CameraQrDecode error!!");
            return NO_MEMORY;
        }
        status_t ret = mpQrDecode->initialize(mFrameWidth, mFrameHeight, mThumbWidth, mThumbHeight, mFrameRate);
        if (ret != NO_ERROR) {
            LOGE("QrDecode initialize error!");
            delete mpQrDecode;
            return ret;
        }
        mpQrDecode->enable(true);
    } else {
        if (mpQrDecode == NULL) {
            return NO_ERROR;
        }
        mpQrDecode->enable(false);
        mpQrDecode->destroy();
        delete mpQrDecode;
        mpQrDecode = NULL;
        return NO_ERROR;
    }

    return NO_ERROR;
}
#endif

int V4L2CameraDevice::setColorFx(int value)
{
    LOGV("value=%d", value);
    int ret = -1;
    struct v4l2_control ctrl;

    if(mCameraDev == CAMERA_CSI){
        ctrl.id = V4L2_CID_COLORFX;
        ctrl.value = value;
        ret = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
        if (ret < 0) {
            LOGE("VIDIOC_S_CTRL failed(%s)", strerror(errno));
        } else {
            LOGV("setColorFx %d ok", value);
        }
    }

    return ret;
}

status_t V4L2CameraDevice::setUvcGadgetMode(int mode)
{
    LOGV("mode(%d), mUvcDev(%p)", mode, mUvcDev);
    Mutex::Autolock locker(&mObjectLock);
    if (mode) {
        if (mUvcDev == NULL) {
            mUvcDev = uvc_init("/dev/video0");
            if (mUvcDev == NULL) {
                LOGE(" uvc_init error!");
                return UNKNOWN_ERROR;
            }
        } else {
            LOGW("uvc is already running!");
        }
    } else {
        if (mUvcDev != NULL) {
            uvc_exit(mUvcDev);
            mUvcDev = NULL;
        } else {
            LOGW("uvc is already stop!");
        }
    }

    return NO_ERROR;
}

void V4L2CameraDevice::setVideoCaptureBufferNum(int value)
{
    LOGD("value=%d", value);
    if (mCameraDeviceState == STATE_STARTED) {
        LOGW("please set buffer number before start device!");
        return;
    }
    mReqBufferNum = value;
    OSAL_QueueTerminate(&mQueueBufferPreview);
    OSAL_QueueCreate(&mQueueBufferPreview, value);
}

status_t V4L2CameraDevice::initV4l2Buffer()
{
    LOGD("buffer num %d", mReqBufferNum);
    if (mpV4l2Buf != NULL) {
        free(mpV4l2Buf);
    }
    mpV4l2Buf = (V4l2Buffer_tag*)malloc(sizeof(V4l2Buffer_tag) * mReqBufferNum);
    if (mpV4l2Buf == NULL) {
        LOGE("alloc V4l2Buffer_tag error!!");
        return NO_MEMORY;
    }
    memset(mpV4l2Buf, 0, sizeof(V4l2Buffer_tag) * mReqBufferNum);
    return NO_ERROR;
}

status_t V4L2CameraDevice::destroyV4l2Buffer()
{
    if (mpV4l2Buf != NULL) {
        free(mpV4l2Buf);
        mpV4l2Buf = NULL;
    }
    return NO_ERROR;
}

status_t V4L2CameraDevice::initVideoPictureBuffer()
{
    LOGD("buffer num %d", mReqBufferNum);
    if (mpVideoPicBuf != NULL) {
        free(mpVideoPicBuf);
    }
    mpVideoPicBuf = (VideoPictureBuffer*)malloc(sizeof(VideoPictureBuffer) * mReqBufferNum);
    if (mpVideoPicBuf == NULL) {
        LOGE("alloc VideoPictureBuffer error!!");
        return NO_MEMORY;
    }
    memset(mpVideoPicBuf, 0, sizeof(VideoPictureBuffer) * mReqBufferNum);
    return NO_ERROR;
}

status_t V4L2CameraDevice::destroyVideoPictureBuffer()
{
    if (mpVideoPicBuf != NULL) {
        free(mpVideoPicBuf);
        mpVideoPicBuf = NULL;
    }
    return NO_ERROR;
}

}; /* namespace android */
