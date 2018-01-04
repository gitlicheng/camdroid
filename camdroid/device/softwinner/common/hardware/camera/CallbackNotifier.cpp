
#include "CameraDebug.h"
#if DBG_CALLBACK
#define LOG_NDEBUG 0
#endif
#define LOG_TAG "CallbackNotifier"
#include <cutils/log.h>

#include <cutils/properties.h>

#include "V4L2CameraDevice2.h"
#include "CallbackNotifier.h"
#include <vencoder.h>

#include "jpegEncode/CameraJpegEncode.h"

//extern "C" int JpegEnc(void * pBufOut, int * bufSize, JPEG_ENC_t *jpeg_enc);

extern "C" int scaler(unsigned char * psrc, unsigned char * pdst, int src_w, int src_h, int dst_w, int dst_h, int fmt, int align);

namespace android {

/* String representation of camera messages. */
static const char* lCameraMessages[] =
{
    "CAMERA_MSG_ERROR",
    "CAMERA_MSG_SHUTTER",
    "CAMERA_MSG_FOCUS",
    "CAMERA_MSG_ZOOM",
    "CAMERA_MSG_PREVIEW_FRAME",
    "CAMERA_MSG_VIDEO_FRAME",
    "CAMERA_MSG_POSTVIEW_FRAME",
    "CAMERA_MSG_RAW_IMAGE",
    "CAMERA_MSG_COMPRESSED_IMAGE",
    "CAMERA_MSG_RAW_IMAGE_NOTIFY",
    "CAMERA_MSG_PREVIEW_METADATA",
    "CAMERA_MSG_CONTINUOUSSNAP",
    "CAMERA_MSG_SNAP"
    "CAMERA_MSG_SNAP_THUMB"
    "CAMERA_MSG_ADAS_METADATA",  /* for ADAS */
    "CAMERA_MSG_AWMD",	/* MOTION_DETECTION_ENABLE */
    "CAMERA_MSG_TVIN_SYSTEM_CHANGE",
    "CAMERA_MSG_QRDECODE",
};
static const int lCameraMessagesNum = sizeof(lCameraMessages) / sizeof(char*);

/* Builds an array of strings for the given set of messages.
 * Param:
 *  msg - Messages to get strings for,
 *  strings - Array where to save strings
 *  max - Maximum number of entries in the array.
 * Return:
 *  Number of strings saved into the 'strings' array.
 */
static int GetMessageStrings(uint32_t msg, const char** strings, int max)
{
    int index = 0;
    int out = 0;
    while (msg != 0 && out < max && index < lCameraMessagesNum) {
        while ((msg & 0x1) == 0 && index < lCameraMessagesNum) {
            msg >>= 1;
            index++;
        }
        if ((msg & 0x1) != 0 && index < lCameraMessagesNum) {
            strings[out] = lCameraMessages[index];
            out++;
            msg >>= 1;
            index++;
        }
    }

    return out;
}

/* Logs messages, enabled by the mask. */
#if DEBUG_MSG
static void PrintMessages(uint32_t msg)
{
    const char* strs[lCameraMessagesNum];
    const int translated = GetMessageStrings(msg, strs, lCameraMessagesNum);
    for (int n = 0; n < translated; n++) {
        LOGV("    %s", strs[n]);
    }
}
#else
#define PrintMessages(msg)   (void(0))
#endif

static void formatToNV21(void *dst,
		               void *src,
		               int width,
		               int height,
		               size_t stride,
		               uint32_t offset,
		               unsigned int bytesPerPixel,
		               size_t length,
		               int pixelFormat)
{
    unsigned int alignedRow, row;
    unsigned char *bufferDst, *bufferSrc;
    unsigned char *bufferDstEnd, *bufferSrcEnd;
    uint16_t *bufferSrc_UV;

    unsigned int y_uv[2];
    y_uv[0] = (unsigned int)src;
	y_uv[1] = (unsigned int)src + width*height;

	// NV12 -> NV21
    if (pixelFormat == V4L2_PIX_FMT_NV12) {
        bytesPerPixel = 1;
        bufferDst = ( unsigned char * ) dst;
        bufferDstEnd = ( unsigned char * ) dst + width*height*bytesPerPixel;
        bufferSrc = ( unsigned char * ) y_uv[0] + offset;
        bufferSrcEnd = ( unsigned char * ) ( ( size_t ) y_uv[0] + length + offset);
        row = width*bytesPerPixel;
        alignedRow = stride-width;
        int stride_bytes = stride / 8;
        uint32_t xOff = offset % stride;
        uint32_t yOff = offset / stride;

        // going to convert from NV12 here and return
        // Step 1: Y plane: iterate through each row and copy
        for ( int i = 0 ; i < height ; i++) {
            memcpy(bufferDst, bufferSrc, row);
            bufferSrc += stride;
            bufferDst += row;
            if ( ( bufferSrc > bufferSrcEnd ) || ( bufferDst > bufferDstEnd ) ) {
                break;
            }
        }

        bufferSrc_UV = ( uint16_t * ) ((uint8_t*)y_uv[1] + (stride/2)*yOff + xOff);

        uint16_t *bufferDst_UV;

        // Step 2: UV plane: convert NV12 to NV21 by swapping U & V
        bufferDst_UV = (uint16_t *) (((uint8_t*)dst)+row*height);

        for (int i = 0 ; i < height/2 ; i++, bufferSrc_UV += alignedRow/2) {
            int n = width;
            asm volatile (
            "   pld [%[src], %[src_stride], lsl #2]                         \n\t"
            "   cmp %[n], #32                                               \n\t"
            "   blt 1f                                                      \n\t"
            "0: @ 32 byte swap                                              \n\t"
            "   sub %[n], %[n], #32                                         \n\t"
            "   vld2.8  {q0, q1} , [%[src]]!                                \n\t"
            "   vswp q0, q1                                                 \n\t"
            "   cmp %[n], #32                                               \n\t"
            "   vst2.8  {q0,q1},[%[dst]]!                                   \n\t"
            "   bge 0b                                                      \n\t"
            "1: @ Is there enough data?                                     \n\t"
            "   cmp %[n], #16                                               \n\t"
            "   blt 3f                                                      \n\t"
            "2: @ 16 byte swap                                              \n\t"
            "   sub %[n], %[n], #16                                         \n\t"
            "   vld2.8  {d0, d1} , [%[src]]!                                \n\t"
            "   vswp d0, d1                                                 \n\t"
            "   cmp %[n], #16                                               \n\t"
            "   vst2.8  {d0,d1},[%[dst]]!                                   \n\t"
            "   bge 2b                                                      \n\t"
            "3: @ Is there enough data?                                     \n\t"
            "   cmp %[n], #8                                                \n\t"
            "   blt 5f                                                      \n\t"
            "4: @ 8 byte swap                                               \n\t"
            "   sub %[n], %[n], #8                                          \n\t"
            "   vld2.8  {d0, d1} , [%[src]]!                                \n\t"
            "   vswp d0, d1                                                 \n\t"
            "   cmp %[n], #8                                                \n\t"
            "   vst2.8  {d0[0],d1[0]},[%[dst]]!                             \n\t"
            "   bge 4b                                                      \n\t"
            "5: @ end                                                       \n\t"
#ifdef NEEDS_ARM_ERRATA_754319_754320
            "   vmov s0,s0  @ add noop for errata item                      \n\t"
#endif
            : [dst] "+r" (bufferDst_UV), [src] "+r" (bufferSrc_UV), [n] "+r" (n)
            : [src_stride] "r" (stride_bytes)
            : "cc", "memory", "q0", "q1"
            );
        }
    }
}

static void NV12ToYVU420(void* psrc, void* pdst, int width, int height)
{
	uint8_t* psrc_y = (uint8_t*)psrc;
	uint8_t* pdst_y = (uint8_t*)pdst;
	uint8_t* psrc_uv = (uint8_t*)psrc + width * height;
	uint8_t* pdst_uv = (uint8_t*)pdst + width * height;
	
	for(int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			*(uint8_t*)(pdst_y + i * width + j) = *(uint8_t*)(psrc_y + i * width + j);
		}
	}
	
	for(int i = 0; i < height / 2; i++)
	{
		for (int j = 0; j < width / 2; j++)
		{
			*(uint8_t*)(pdst_uv + i * width / 2 + j) = *(uint8_t*)(psrc_uv + i * width + j * 2);
			*(uint8_t*)(pdst_uv + (width / 2) * (height / 2) + i * width / 2 + j) = *(uint8_t*)(psrc_uv + i * width + j * 2 + 1);
		}
	}
	
}

static void NV21ToYVU420(void* psrc, void* pdst, int width, int height)
{
	uint8_t* psrc_y = (uint8_t*)psrc;
	uint8_t* pdst_y = (uint8_t*)pdst;
	uint8_t* psrc_uv = (uint8_t*)psrc + width * height;
	uint8_t* pdst_uv = (uint8_t*)pdst + width * height;
	
	for(int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			*(uint8_t*)(pdst_y + i * width + j) = *(uint8_t*)(psrc_y + i * width + j);
		}
	}
	for(int i = 0; i < height / 2; i++)
	{
		for (int j = 0; j < width / 2; j++)
		{
			*(uint8_t*)(pdst_uv + i * width / 2 + j) = *(uint8_t*)(psrc_uv + i * width + j * 2 + 1);
			*(uint8_t*)(pdst_uv + (width / 2) * (height / 2) + i * width / 2 + j) = *(uint8_t*)(psrc_uv + i * width + j * 2);
		}
	}
}

static void YVU420ToNV21(void* psrc, void* pdst, int width, int height)
{
	uint8_t* psrc_y = (uint8_t*)psrc;
	uint8_t* pdst_y = (uint8_t*)pdst;
	uint8_t* psrc_uv = (uint8_t*)psrc + width * height;
	uint8_t* pdst_uv = (uint8_t*)pdst + width * height;
	
	for(int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			*(uint8_t*)(pdst_y + i * width + j) = *(uint8_t*)(psrc_y + i * width + j);
		}
	}

	for(int i = 0; i < height / 2; i++)
	{
		for (int j = 0; j < width / 2; j++)
		{
			*(uint8_t*)(pdst_uv + i * width + j * 2 + 1) = *(uint8_t*)(psrc_uv + i * width / 2 + j);
			*(uint8_t*)(pdst_uv + i * width + j * 2) = *(uint8_t*)(psrc_uv + (width / 2) * (height / 2) + i * width / 2 + j);
		}
	}
	
}

static bool yuv420spDownScale(void* psrc, void* pdst, int src_w, int src_h, int dst_w, int dst_h)
{
	char * psrc_y = (char *)psrc;
	char * pdst_y = (char *)pdst;
	char * psrc_uv = (char *)psrc + src_w * src_h;
	char * pdst_uv = (char *)pdst + dst_w * dst_h;
	
	int scale = 1;
	int scale_w = src_w / dst_w;
	int scale_h = src_h / dst_h;
	int h, w;
	
	if (dst_w > src_w
		|| dst_h > src_h)
	{
		LOGE("error size, %dx%d -> %dx%d\n", src_w, src_h, dst_w, dst_h);
		return false;
	}
	
	if (scale_w == scale_h)
	{
		scale = scale_w;
	}
	
	LOGV("scale = %d\n", scale);

	if (scale == 1)
	{
		if ((src_w == dst_w)
			&& (src_h == dst_h))
		{
			memcpy((char*)pdst, (char*)psrc, dst_w * dst_h * 3/2);
		}
		else
		{
			// crop
			for (h = 0; h < dst_h; h++)
			{
				memcpy((char*)pdst + h * dst_w, (char*)psrc + h * src_w, dst_w);
			}
			for (h = 0; h < dst_h / 2; h++)
			{
				memcpy((char*)pdst_uv + h * dst_w, (char*)psrc_uv + h * src_w, dst_w);
			}
		}
		
		return true;
	}
	
	for (h = 0; h < dst_h; h++)
	{
		for (w = 0; w < dst_w; w++)
		{
			*(pdst_y + h * dst_w + w) = *(psrc_y + h * scale * src_w + w * scale);
		}
	}
	for (h = 0; h < dst_h / 2; h++)
	{
		for (w = 0; w < dst_w; w+=2)
		{
			*((short*)(pdst_uv + h * dst_w + w)) = *((short*)(psrc_uv + h * scale * src_w + w * scale));
		}
	}
	
	return true;
}


static int getMeminfo(const char* pattern)
{
  FILE* fp = fopen("/proc/meminfo", "r");
  if (fp == NULL) {
    return -1;
  }

  int result = -1;
  char buf[256];
  while (fgets(buf, sizeof(buf), fp) != NULL) {
    if (sscanf(buf, pattern, &result) == 1) {
      break;
    }
  }
  fclose(fp);
  return result;
}

static int getTotalMemory(void)
{
  return getMeminfo("MemTotal: %d kB");
}

static int getFreeMemory(void)
{
  return getMeminfo("MemFree: %d kB");
}

static int getCachedMemory(void)
{
  return getMeminfo("Cached: %d kB");
}

static int getBuffersMemory(void)
{
  return getMeminfo("Buffers: %d kB");
}

CallbackNotifier::CallbackNotifier()
    : mNotifyCB(NULL),
      mDataCB(NULL),
      mDataCBTimestamp(NULL),
      mGetMemoryCB(NULL),
      mCallbackCookie(NULL),
      mMessageEnabler(0),
      mVideoRecEnabled(false),
      mSavePictureCnt(0),
      mSavePictureMax(0),
      mJpegQuality(90),
      mJpegRotate(0),
      mPictureWidth(640),
      mPictureHeight(480),
	  mThumbWidth(0),
	  mThumbHeight(0),
      mGpsLatitude(0.0),
	  mGpsLongitude(0.0),
	  mGpsAltitude(0),
	  mGpsTimestamp(0),
	  mFocalLength(0.0),
	  mWhiteBalance(0),
	  mCBWidth(0),
	  mCBHeight(0),
	  mBufferList(NULL),
	  mSaveThreadExited(false),
	  mpReleaseBufferLock(NULL)
{
	LOGV("CallbackNotifier construct");
	
	memset(mGpsMethod, 0, sizeof(mGpsMethod));
	memset(mCallingProcessName, 0, sizeof(mCallingProcessName));
	
	strcpy(mExifMake, "MID MAKE");		// default
	strcpy(mExifModel, "MID MODEL");	// default
	memset(mDateTime, 0, sizeof(mDateTime));
	strcpy(mDateTime, "2012:10:24 17:30:00");

	memset(mFolderPath, 0, sizeof(mFolderPath));
	memset(mSnapPath, 0, sizeof(mSnapPath));
}

CallbackNotifier::~CallbackNotifier()
{
	LOGV("CallbackNotifier disconstruct");
}

/****************************************************************************
 * Camera API
 ***************************************************************************/

void CallbackNotifier::setCallbacks(camera_notify_callback notify_cb,
                                    camera_data_callback data_cb,
                                    camera_data_timestamp_callback data_cb_timestamp,
                                    camera_request_memory get_memory,
                                    void* user)
{
    LOGV("%p, %p, %p, %p (%p)", notify_cb, data_cb, data_cb_timestamp, get_memory, user);

    Mutex::Autolock locker(&mObjectLock);
    mNotifyCB = notify_cb;
    mDataCB = data_cb;
    mDataCBTimestamp = data_cb_timestamp;
    mGetMemoryCB = get_memory;
    mCallbackCookie = user;

	mBufferList = new BufferListManager();
	if (mBufferList == NULL)
	{
		LOGE("create BufferListManager failed");
	}

	mSaveThreadExited = false;
	
	// init picture thread
	mSavePictureThread = new DoSavePictureThread(this);
	pthread_mutex_init(&mSavePictureMutex, NULL);
	pthread_cond_init(&mSavePictureCond, NULL);
	mSavePictureThread->startThread();
}

void CallbackNotifier::enableMessage(uint msg_type)
{
    LOGV("msg_type = 0x%x", msg_type);
    PrintMessages(msg_type);

    Mutex::Autolock locker(&mObjectLock);
    mMessageEnabler |= msg_type;
    LOGV("**** Currently enabled messages:");
    PrintMessages(mMessageEnabler);
}

void CallbackNotifier::disableMessage(uint msg_type)
{
    LOGV("msg_type = 0x%x", msg_type);
    PrintMessages(msg_type);

    Mutex::Autolock locker(&mObjectLock);
    mMessageEnabler &= ~msg_type;
    LOGV("**** Currently enabled messages:");
    PrintMessages(mMessageEnabler);
}

status_t CallbackNotifier::enableVideoRecording()
{
    LOGV();

    Mutex::Autolock locker(&mObjectLock);
    mVideoRecEnabled = true;

    return NO_ERROR;
}

void CallbackNotifier::disableVideoRecording()
{
    LOGV();

    Mutex::Autolock locker(&mObjectLock);
    mVideoRecEnabled = false;
}

status_t CallbackNotifier::storeMetaDataInBuffers(bool enable)
{
    /* Return INVALID_OPERATION means HAL does not support metadata. So HAL will
     * return actual frame data with CAMERA_MSG_VIDEO_FRAME. Return
     * INVALID_OPERATION to mean metadata is not supported. */

	return UNKNOWN_ERROR;
}

/****************************************************************************
 * Public API
 ***************************************************************************/

void CallbackNotifier::cleanupCBNotifier()
{
	if (mSavePictureThread != NULL)
	{
		mSavePictureThread->stopThread();
		pthread_cond_signal(&mSavePictureCond);
		
		pthread_mutex_lock(&mSavePictureMutex);
		if (!mSaveThreadExited)
		{
			struct timespec timeout;
			timeout.tv_sec=time(0) + 1;		// 1s timeout
			timeout.tv_nsec = 0;
			pthread_cond_timedwait(&mSavePictureCond, &mSavePictureMutex, &timeout);
			//pthread_cond_wait(&mSavePictureCond, &mSavePictureMutex);
		}
		pthread_mutex_unlock(&mSavePictureMutex);
		
		mSavePictureThread.clear();
		mSavePictureThread = 0;
		
		pthread_mutex_destroy(&mSavePictureMutex);
		pthread_cond_destroy(&mSavePictureCond);
	}
	
    Mutex::Autolock locker(&mObjectLock);
    mMessageEnabler = 0;
    mNotifyCB = NULL;
    mDataCB = NULL;
    mDataCBTimestamp = NULL;
    mGetMemoryCB = NULL;
    mCallbackCookie = NULL;
    mVideoRecEnabled = false;
    mJpegQuality = 90;
	mJpegRotate = 0;
	mPictureWidth = 640;
	mPictureHeight = 480;
	mThumbWidth = 320;
	mThumbHeight = 240;
	mGpsLatitude = 0.0;
	mGpsLongitude = 0.0;
	mGpsAltitude = 0;
	mGpsTimestamp = 0;
	mFocalLength = 0.0;
	mWhiteBalance = 0;

	if (mBufferList != NULL)
	{
		delete mBufferList;
		mBufferList = NULL;
	}
}

void CallbackNotifier::onNextFrameAvailable(const void* frame, bool hw, int bufIdx)
{
    if (hw)
    {
    	onNextFrameHW(frame, bufIdx);
    }
	else
	{
    	onNextFrameSW(frame, bufIdx);
	}
}

void CallbackNotifier::onNextFrameHW(const void* frame, int bufIdx)
{
	V4L2BUF_t * pbuf = (V4L2BUF_t*)frame;
    status_t ret;
	
	if (isMessageEnabled(CAMERA_MSG_VIDEO_FRAME) && isVideoRecordingEnabled()) 
	{
        camera_memory_t* cam_buff = mGetMemoryCB(-1, sizeof(V4L2BUF_t), 1, NULL);
        if (NULL != cam_buff && NULL != cam_buff->data) 
		{
            if(mpReleaseBufferLock)
            {
                Mutex::Autolock locker(mpReleaseBufferLock);
                pbuf->refCnt++;
            }
            else
            {
                LOGE("mpReleaseBufferLock is NULL. Be careful!");
                pbuf->refCnt++;
            }
            memcpy(cam_buff->data, frame, sizeof(V4L2BUF_t));
            ret = mDataCBTimestamp(pbuf->timeStamp, CAMERA_MSG_VIDEO_FRAME,
                               cam_buff, 0, bufIdx, mCallbackCookie);
            if(ret != NO_ERROR)
            {
                LOGD("call mDataCBTimestamp fail[%d], maybe stop Recording now.", ret);
                if(mpReleaseBufferLock)
                {
                    Mutex::Autolock locker(mpReleaseBufferLock);
                    pbuf->refCnt--;
                }
                else
                {
                    LOGE("mpReleaseBufferLock is NULL. Be careful!");
                    pbuf->refCnt--;
                }
            }
			cam_buff->release(cam_buff);
        } 
		else 
		{
            LOGE("Memory failure in CAMERA_MSG_VIDEO_FRAME");
        }
    }

    if (isMessageEnabled(CAMERA_MSG_PREVIEW_FRAME)) 
	{
        camera_memory_t* cam_buff = mGetMemoryCB(-1, sizeof(V4L2BUF_t), 1, NULL);
        if (NULL != cam_buff && NULL != cam_buff->data) 
		{
            memcpy(cam_buff->data, frame, sizeof(V4L2BUF_t));
			mDataCB(CAMERA_MSG_PREVIEW_FRAME, cam_buff, 0, NULL, mCallbackCookie);
			cam_buff->release(cam_buff);
        } 
		else 
		{
            LOGE("Memory failure in CAMERA_MSG_PREVIEW_FRAME");
        }
    }
}

void CallbackNotifier::onNextFrameSW(const void* frame, int bufIdx)
{
	V4L2BUF_t * pbuf = (V4L2BUF_t*)frame;
	int framesize =0;
	int src_format = 0;
	int src_addr_phy = 0;
	int src_addr_vir = 0;
	int src_width = 0;
	int src_height = 0;
	RECT_t src_crop;

	if ((pbuf->isThumbAvailable == 1)
		&& (pbuf->thumbUsedForPreview == 1))
	{
		src_format			= pbuf->thumbFormat;
		src_addr_phy		= pbuf->thumbAddrPhyY;
		src_addr_vir		= pbuf->thumbAddrVirY;
		src_width			= pbuf->thumbWidth;
		src_height			= pbuf->thumbHeight;
		memcpy((void*)&src_crop, (void*)&pbuf->thumb_crop_rect, sizeof(RECT_t));
	}
	else
	{
		src_format			= pbuf->format;
		src_addr_phy		= pbuf->addrPhyY;
		src_addr_vir		= pbuf->addrVirY;
		src_width			= pbuf->width;
		src_height			= pbuf->height;
		memcpy((void*)&src_crop, (void*)&pbuf->crop_rect, sizeof(RECT_t));
	}
	
	framesize = ALIGN_16B(src_width) * src_height * 3/2;
	
	if (isMessageEnabled(CAMERA_MSG_VIDEO_FRAME) && isVideoRecordingEnabled()) 
	{
        camera_memory_t* cam_buff = mGetMemoryCB(-1, framesize, 1, NULL);
        if (NULL != cam_buff && NULL != cam_buff->data) 
		{
            memcpy(cam_buff->data, (void *)src_addr_vir, framesize);
            mDataCBTimestamp(pbuf->timeStamp, CAMERA_MSG_VIDEO_FRAME,
                               cam_buff, 0, bufIdx, mCallbackCookie);
			cam_buff->release(cam_buff);		// star add
        } else {
            LOGE("Memory failure in CAMERA_MSG_VIDEO_FRAME");
        }
    }

    if (isMessageEnabled(CAMERA_MSG_PREVIEW_FRAME)) 
	{
		if (strcmp(mCallingProcessName, "com.android.facelock") == 0)
		{
 			camera_memory_t* cam_buff = mGetMemoryCB(-1, 160 * 120 * 3 / 2, 1, NULL);
	        if (NULL != cam_buff && NULL != cam_buff->data) 
			{
				yuv420spDownScale((void*)src_addr_vir, cam_buff->data, 
								ALIGN_16B(src_width), src_height,
								160, 120);
	            mDataCB(CAMERA_MSG_PREVIEW_FRAME, cam_buff, 0, NULL, mCallbackCookie);
	            cam_buff->release(cam_buff);
	        }
			else 
			{
	            LOGE("Memory failure in CAMERA_MSG_PREVIEW_FRAME");
	        }
		}
		else
		{
			//LOGD("src size: %dx%d, cb size: %dx%d,", src_width, src_height, mCBWidth, mCBHeight);
			framesize = mCBWidth * mCBHeight * 3/2;
	        camera_memory_t* cam_buff = mGetMemoryCB(-1, framesize, 1, NULL);

			if (NULL == cam_buff 
				|| NULL == cam_buff->data) 
			{
				LOGE("Memory failure in CAMERA_MSG_PREVIEW_FRAME");
				return;
			}
			if (src_format == V4L2_PIX_FMT_YVU420)
			{
				// it will be used in cts
				scaler((unsigned char*)src_addr_vir, (unsigned char*)cam_buff->data, 
									ALIGN_16B(src_width), src_height,
									mCBWidth, mCBHeight, /*src_format*/0, 16);
			}
			else
			{
		    	framesize = ALIGN_16B(src_width) * src_height * 3/2;
				camera_memory_t* src_addr_vir_copy = mGetMemoryCB(-1, framesize, 1, NULL);
				if (NULL == src_addr_vir_copy 
				|| NULL == src_addr_vir_copy->data) 
				{
					LOGE("Memory failure in CAMERA_MSG_PREVIEW_FRAME");
					return;
				}

				framesize = mCBWidth * mCBHeight * 3/2;
	        	camera_memory_t* cam_buff_copy = mGetMemoryCB(-1, framesize, 1, NULL);
				if (NULL == cam_buff_copy 
				|| NULL == cam_buff_copy->data) 
				{
					LOGE("Memory failure in CAMERA_MSG_PREVIEW_FRAME");
					return;
				}
			
				if (src_format == V4L2_PIX_FMT_NV12)
				{
					NV12ToYVU420((void*)src_addr_vir, (void*)src_addr_vir_copy->data, ALIGN_16B(src_width), src_height);
				}
				else if(src_format == V4L2_PIX_FMT_NV21)
				{
					NV21ToYVU420((void*)src_addr_vir, (void*)src_addr_vir_copy->data, ALIGN_16B(src_width), src_height);
				}
				
				scaler((unsigned char*)src_addr_vir_copy->data, (unsigned char*)cam_buff_copy->data, 
									ALIGN_16B(src_width), src_height,
									mCBWidth, mCBHeight, 0, 16);
				YVU420ToNV21(cam_buff_copy->data, cam_buff->data, mCBWidth, mCBHeight);

				cam_buff_copy->release(cam_buff_copy);
				src_addr_vir_copy->release(src_addr_vir_copy);
			}
			
            mDataCB(CAMERA_MSG_PREVIEW_FRAME, cam_buff, 0, NULL, mCallbackCookie);
            cam_buff->release(cam_buff);
		}
    }
}

status_t CallbackNotifier::autoFocusMsg(bool success)
{
	if (isMessageEnabled(CAMERA_MSG_FOCUS))
	{
        mNotifyCB(CAMERA_MSG_FOCUS, success, 0, mCallbackCookie);
    }
	return NO_ERROR;
}

status_t CallbackNotifier::autoFocusContinuousMsg(bool success)
{
	if (isMessageEnabled(CAMERA_MSG_FOCUS_MOVE))
	{
		// in continuous focus mode
		// true for starting focus, false for focus ok
		mNotifyCB(CAMERA_MSG_FOCUS_MOVE, success, 0, mCallbackCookie);
	}
    return NO_ERROR;
}

#if 0  // face
status_t CallbackNotifier::faceDetectionMsg(camera_frame_metadata_t *face)
{
	if (isMessageEnabled(CAMERA_MSG_PREVIEW_METADATA))
	{
		camera_memory_t *cam_buff = mGetMemoryCB(-1, 1, 1, NULL);
		mDataCB(CAMERA_MSG_PREVIEW_METADATA, cam_buff, 0, face, mCallbackCookie);
		cam_buff->release(cam_buff); 
	}
    return NO_ERROR;
}
#endif

void CallbackNotifier::notifyPictureMsg(const void* frame)
{
	LOGV();

	// shutter msg
    if (isMessageEnabled(CAMERA_MSG_SHUTTER)) 
	{
		LOGV();
        mNotifyCB(CAMERA_MSG_SHUTTER, 0, 0, mCallbackCookie);
    }

	// raw image msg
	if (isMessageEnabled(CAMERA_MSG_RAW_IMAGE)) 
	{
		camera_memory_t *dummyRaw = mGetMemoryCB(-1, 1, 1, NULL);
		if ( NULL == dummyRaw ) 
		{
			LOGE("Memory failure in CAMERA_MSG_PREVIEW_FRAME");
			return;
		}
		mDataCB(CAMERA_MSG_RAW_IMAGE, dummyRaw, 0, NULL, mCallbackCookie);
		dummyRaw->release(dummyRaw);
	}
	else if (isMessageEnabled(CAMERA_MSG_RAW_IMAGE_NOTIFY)) 
	{
		mNotifyCB(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0, mCallbackCookie);
	}
}

void CallbackNotifier::startContinuousPicture()
{
	LOGV();
	
	// 
	mSavePictureCnt = 0;
}

void CallbackNotifier::stopContinuousPicture()
{
	// do nothing
}

void CallbackNotifier::setContinuousPictureNumber(int cnt)
{
	mSavePictureMax = cnt;
}

status_t CallbackNotifier::cameraDataCallBack(int msgType, void *buffer, size_t size)
{
    if (!isMessageEnabled(msgType) || mDataCB == NULL) {
        return NO_ERROR;
    }
	camera_memory_t* mem = mGetMemoryCB(-1, size, 1, NULL);
	if (NULL != mem && NULL != mem->data)
	{
		memcpy(mem->data, (uint8_t*)buffer, size);
		mDataCB(msgType, mem, 0, NULL, mCallbackCookie);
		mem->release(mem);
	}
	else
	{
		LOGE("Memory(size=%u) failure in msgType[%d]", size, msgType);
        return NO_MEMORY;
	}
    return NO_ERROR;
}

bool CallbackNotifier::takePicture(const void* frame, bool isContinuous, bool isThumbnailPic)
{
    V4L2BUF_t * pbuf = (V4L2BUF_t *)frame;
    int src_format = 0;
    unsigned int src_addr_phy = 0;
    unsigned int src_addr_vir = 0;
    int src_width = 0;
    int src_height = 0;
    int picWidth = 0, picHeight = 0;
    int msgType = CAMERA_MSG_COMPRESSED_IMAGE;
    status_t ret;
    unsigned int jpegsize;
    PictureBuffer buf;

    JpegEncInfo sjpegInfo;
    EXIFInfo   exifInfo;

    memset(&sjpegInfo, 0, sizeof(JpegEncInfo));
    memset(&exifInfo, 0, sizeof(EXIFInfo));

    if (isThumbnailPic)
    {
        if (!isMessageEnabled(CAMERA_MSG_POSTVIEW_FRAME)) {
            return false;
        }
        picWidth = mThumbWidth;
        picHeight = mThumbHeight;
        msgType = CAMERA_MSG_POSTVIEW_FRAME;
    }
    else
    {
        picWidth = mPictureWidth;
        picHeight = mPictureHeight;
        msgType = CAMERA_MSG_COMPRESSED_IMAGE;
    }

    if ((pbuf->isThumbAvailable == 1) && (pbuf->thumbUsedForPhoto == 1))
    {
        src_format			= pbuf->thumbFormat;
        src_addr_phy		= pbuf->thumbAddrPhyY;
        src_addr_vir		= pbuf->thumbAddrVirY;
        src_width			= pbuf->thumbWidth;
        src_height			= pbuf->thumbHeight;

        sjpegInfo.sCropInfo.nLeft = pbuf->thumb_crop_rect.left;
        sjpegInfo.sCropInfo.nTop = pbuf->thumb_crop_rect.top;
        sjpegInfo.sCropInfo.nWidth = pbuf->thumb_crop_rect.width;
        sjpegInfo.sCropInfo.nHeight = pbuf->thumb_crop_rect.height;
    }
    else
    {
        src_format			= pbuf->format;
        src_addr_phy		= pbuf->addrPhyY;
        src_addr_vir		= pbuf->addrVirY;
        src_width			= pbuf->width;
        src_height			= pbuf->height;

        sjpegInfo.sCropInfo.nLeft = pbuf->crop_rect.left;
        sjpegInfo.sCropInfo.nTop = pbuf->crop_rect.top;
        sjpegInfo.sCropInfo.nWidth = pbuf->crop_rect.width;
        sjpegInfo.sCropInfo.nHeight = pbuf->crop_rect.height;
    }

    getCurrentDateTime();

    sjpegInfo.sBaseInfo.nInputWidth = src_width;
    sjpegInfo.sBaseInfo.nInputHeight = src_height;
    sjpegInfo.sBaseInfo.nDstWidth = picWidth;
    sjpegInfo.sBaseInfo.nDstHeight = picHeight;
    sjpegInfo.pAddrPhyY = (unsigned char*)src_addr_phy;
    sjpegInfo.pAddrPhyC = (unsigned char*)src_addr_phy + ALIGN_16B(src_width) * ALIGN_16B(src_height);
    sjpegInfo.sBaseInfo.eInputFormat = (src_format == V4L2_PIX_FMT_NV21) ? VENC_PIXEL_YVU420SP: VENC_PIXEL_YUV420SP;
    sjpegInfo.quality		= mJpegQuality;
    exifInfo.Orientation    = mJpegRotate;
    sjpegInfo.bEnableCorp  = 1;

    exifInfo.ThumbWidth = 0;
    exifInfo.ThumbHeight = 0;

    strcpy((char*)exifInfo.CameraMake,	mExifMake);
    strcpy((char*)exifInfo.CameraModel,	mExifModel);
    strcpy((char*)exifInfo.DateTime, mDateTime);

    if (0 != strlen(mGpsMethod)) {
        strcpy((char*)exifInfo.gpsProcessingMethod,mGpsMethod);
        exifInfo.enableGpsInfo = 1;
        exifInfo.gps_latitude = mGpsLatitude;
        exifInfo.gps_longitude = mGpsLongitude;
        exifInfo.gps_altitude = mGpsAltitude;
        exifInfo.gps_timestamp = mGpsTimestamp;		
    } else {
        exifInfo.enableGpsInfo = 0;
    }

    exifInfo.WhiteBalance = mWhiteBalance;

    CameraJpegEncConfig VJpegEncConfig;
    memset(&VJpegEncConfig, 0, sizeof(CameraJpegEncConfig));
    VJpegEncConfig.nVbvBufferSize = ((((picWidth * picHeight * 3) >> 2) + 1023) >> 10) << 10;
    LOGD("takePicture: Picture size %dx%d, vbv buffer size %u", picWidth, picHeight, VJpegEncConfig.nVbvBufferSize);

    CameraJpegEncode *pJpegenc = new CameraJpegEncode();
    ret = pJpegenc->initialize(&VJpegEncConfig, &sjpegInfo, &exifInfo);
    if (ret != NO_ERROR) {
        LOGE("CameraJpegEncode initialize error!");
		goto JPEG_INIT_ERR;
    }
    ret = pJpegenc->encode(&sjpegInfo);
    if (ret != NO_ERROR) {
        LOGE("CameraJpegEncode encode error!");
        goto JPEG_ENCODE_ERR;
    }
    if (pJpegenc->getFrame() != 0) {
        LOGE("CameraJpegEncode getFrame error!");
        goto JPEG_GET_FRAME_ERR;
    }
    jpegsize = pJpegenc->getDataSize();

    {
        int freeMemKb = getFreeMemory();
        int buffersMemKb = getBuffersMemory();
        int cachedMemKb = getCachedMemory();
        if ((freeMemKb+buffersMemKb+cachedMemKb) < (jpegsize+2*1024*1024)>>10) {
            LOGW("Free memory too small! jpegsize=%dKB, MemTotal=%dKB, MemFree=%dKB, Buffers=%dKB, Cached=%dKB",
                jpegsize, getTotalMemory(), freeMemKb, buffersMemKb, cachedMemKb);
            goto ALLOC_MEM_ERR;
        }
    }

    buf.msgType = msgType;
    buf.isContinuous = isContinuous;
    buf.isEnd = false;
    buf.memory = mGetMemoryCB(-1, jpegsize, 1, NULL);
    if (buf.memory == NULL || buf.memory->data == NULL) {
        LOGE("Memory(size=%u) failure in msgType[%d]", jpegsize, msgType);
        goto ALLOC_MEM_ERR;
    }
    pJpegenc->getDataToBuffer(buf.memory->data);
    mPicBufVector.push_back(buf);
    pthread_cond_signal(&mSavePictureCond);

    pJpegenc->returnFrame();
    pJpegenc->destroy();
    delete pJpegenc;

	LOGV("taking photo end");
	return true;

ALLOC_MEM_ERR:
    pJpegenc->returnFrame();
JPEG_GET_FRAME_ERR:
JPEG_ENCODE_ERR:
    pJpegenc->destroy();
JPEG_INIT_ERR:
    delete pJpegenc;
    if (!isContinuous) {
        buf.msgType = msgType;
        buf.isContinuous = isContinuous;
        buf.isEnd = false;
        buf.memory = mGetMemoryCB(-1, sizeof(int), 1, NULL);
        if (buf.memory == NULL || buf.memory->data == NULL) {
            LOGE("Memory(size=%u) failure in msgType[%d]", sizeof(int), msgType);
            return false;
        }
        memset(buf.memory->data, 0, sizeof(int));
        mPicBufVector.push_back(buf);
        pthread_cond_signal(&mSavePictureCond);
    }
    return false;
}

bool CallbackNotifier::savePictureThread()
{
    if (mPicBufVector.isEmpty()) {
        pthread_mutex_lock(&mSavePictureMutex);
        if (mSavePictureThread->getThreadStatus() == THREAD_STATE_EXIT) {
            mSaveThreadExited = true;
            pthread_mutex_unlock(&mSavePictureMutex);
            pthread_cond_signal(&mSavePictureCond);
            LOGD("savePictureThread exit");
            return false;
        }
        LOGV("wait for picture to save");
        pthread_cond_wait(&mSavePictureCond, &mSavePictureMutex);
        pthread_mutex_unlock(&mSavePictureMutex);
        return true;
    }

    PictureBuffer &buf = mPicBufVector.editItemAt(0);

    if (buf.isEnd) {
        mDataCB(buf.msgType, buf.memory, 0, (camera_frame_metadata_t*)1, mCallbackCookie);
        goto SAVE_PICTURE_END;
    }

    if (buf.isContinuous) {
        if (mFolderPath[0] != 0) {
            if (isMessageEnabled(CAMERA_MSG_CONTINUOUSSNAP)) {
                mNotifyCB(CAMERA_MSG_CONTINUOUSSNAP, mSavePictureCnt, 0, mCallbackCookie);
            }
            char fname[128];
            FILE *fp = NULL;
            sprintf(fname, "%s%03d.jpg", mFolderPath, mSavePictureCnt);
            fp = fopen(fname, "wb+");
            if (fp == NULL) {
                LOGE("open %s failed, %s", fname, strerror(errno));
                goto SAVE_PICTURE_END;
            }
            fwrite(buf.memory->data, buf.memory->size, 1, fp);
            fflush(fp);
            fclose(fp);
        } else {
            mDataCB(buf.msgType, buf.memory, 0, NULL, mCallbackCookie);
        }
    } else {
        if (mSnapPath[0] != 0) {
            mNotifyCB(CAMERA_MSG_SNAP, 0, 0, mCallbackCookie);
            FILE *fp = fopen(mSnapPath, "wb+");
            if (fp == NULL) {
                LOGE("open %s failed, %s", mSnapPath, strerror(errno));
                goto SAVE_PICTURE_END;
            }
            fwrite(buf.memory->data, buf.memory->size, 1, fp);
            fflush(fp);
            fclose(fp);

    		camera_memory_t* cb_buff;
    		cb_buff = mGetMemoryCB(-1, strlen(mSnapPath), 1, NULL);
    		if (NULL == cb_buff || NULL == cb_buff->data) {
    			LOGE("Memory failure in CAMERA_MSG_SNAP_THUMB");
                goto SAVE_PICTURE_END;
    		}
    		memcpy(cb_buff->data, mSnapPath, strlen(mSnapPath));
    		mDataCB(CAMERA_MSG_SNAP_THUMB, cb_buff, 0, NULL, mCallbackCookie);
    		cb_buff->release(cb_buff);
        } else {
            mDataCB(buf.msgType, buf.memory, 0, NULL, mCallbackCookie);
        }
    }

SAVE_PICTURE_END:
	buf.memory->release(buf.memory);
    mPicBufVector.removeAt(0);
	return true;
}

status_t CallbackNotifier::takePictureEnd()
{
    LOGV();
    PictureBuffer buf;
    buf.msgType = CAMERA_MSG_COMPRESSED_IMAGE;
    buf.isContinuous = false;
    buf.isEnd = true;
    buf.memory = mGetMemoryCB(-1, 1, 1, NULL);
    if (NULL == buf.memory || NULL == buf.memory->data)
    {
        LOGE("mGetMemoryCB failed!");
        return NO_MEMORY;
    }
    mPicBufVector.push_back(buf);
    pthread_cond_signal(&mSavePictureCond);
    return NO_ERROR;
}


void CallbackNotifier::getCurrentDateTime()
{
	time_t t;
	struct tm *tm_t;
	time(&t);
	tm_t = localtime(&t);
	sprintf(mDateTime, "%4d:%02d:%02d %02d:%02d:%02d", 
		tm_t->tm_year+1900, tm_t->tm_mon+1, tm_t->tm_mday,
		tm_t->tm_hour, tm_t->tm_min, tm_t->tm_sec);
}

void CallbackNotifier::onCameraDeviceError(int err)
{
    if (isMessageEnabled(CAMERA_MSG_ERROR) && mNotifyCB != NULL) {
        mNotifyCB(CAMERA_MSG_ERROR, err, 0, mCallbackCookie);
    }
}

void CallbackNotifier::setMutexForReleaseBufferLock(Mutex *pMutex)
{
    mpReleaseBufferLock = pMutex;
}

/* MOTION_DETECTION_ENABLE start */
status_t CallbackNotifier::AWMDDetectionMsg(int value)
{
	if (isMessageEnabled(CAMERA_MSG_AWMD) && mNotifyCB != NULL)
	{
		mNotifyCB(CAMERA_MSG_AWMD, value, 0, mCallbackCookie);
	}
    return NO_ERROR;
}
/* MOTION_DETECTION_ENABLE end */

status_t CallbackNotifier::qrdecodeMsg(char *result, int size)
{
	cameraDataCallBack(CAMERA_MSG_QRDECODE, result, size);
    return NO_ERROR;
}

status_t CallbackNotifier::TvinSystemChangeMsg(int system)
{
	if (isMessageEnabled(CAMERA_MSG_TVIN_SYSTEM_CHANGE) && mNotifyCB != NULL)
	{
		mNotifyCB(CAMERA_MSG_TVIN_SYSTEM_CHANGE, system, 0, mCallbackCookie);
	}
    return NO_ERROR;
}

/* for ADAS start */
void CallbackNotifier::copyAdasOutData(void *data, ADASOUT_IF *adas)
{
#if 0
    uint8_t *p = (uint8_t *)data;
    *p++ = adas->adasOut->lane.isDisp;
    *p++ = adas->adasOut->lane.ltWarn;
    *p++ = adas->adasOut->lane.rtWarn;
    memcpy(p, &adas->adasOut->lane.ltIdxs, sizeof(PointC) * 2);
    p += sizeof(PointC) * 2;
    memcpy(p, &adas->adasOut->lane.rtIdxs, sizeof(PointC) * 2);
    p += sizeof(PointC) * 2;
    memcpy(p, &adas->adasOut->lane.colorPointsNum, sizeof(int));
    p += sizeof(int);
    *p++ = adas->adasOut->lane.dnColor;
    memcpy(p, adas->adasOut->lane.rows, sizeof(unsigned short) * adas->adasOut->lane.colorPointsNum);
    p += sizeof(unsigned short) * adas->adasOut->lane.colorPointsNum;
    memcpy(p, adas->adasOut->lane.rtCols, sizeof(unsigned short) * adas->adasOut->lane.colorPointsNum);
    p += sizeof(unsigned short) * adas->adasOut->lane.colorPointsNum;
    memcpy(p, adas->adasOut->lane.mdCols, sizeof(unsigned short) * adas->adasOut->lane.colorPointsNum);
    p += sizeof(unsigned short) * adas->adasOut->lane.colorPointsNum;
    memcpy(p, adas->adasOut->lane.ltCols, sizeof(unsigned short) * adas->adasOut->lane.colorPointsNum);
    p += sizeof(unsigned short) * adas->adasOut->lane.colorPointsNum;

    memcpy(p, &adas->adasOut->cars.Num, sizeof(int));
    p += sizeof(int);
    memcpy(p, adas->adasOut->cars.carP, sizeof(CAR) * adas->adasOut->cars.Num);
    p += sizeof(CAR) * adas->adasOut->cars.Num;

    memcpy(p, &adas->adasOut->plates.Num, sizeof(int));
    p += sizeof(int);
    memcpy(p, adas->adasOut->plates.plateP, sizeof(PLATE) * adas->adasOut->plates.Num);
    p += sizeof(PLATE) * adas->adasOut->plates.Num;

    memcpy(p, &adas->adasOut->score, sizeof(int));
    p += sizeof(int);
    if (adas->adasOut->version != NULL) {
        memcpy(p, adas->adasOut->version, strlen(adas->adasOut->version));
        p += strlen(adas->adasOut->version);
    }
    *p++ = '\0';

    memcpy(p, &adas->subWidth, sizeof(int));
    p += sizeof(int);
    memcpy(p, &adas->subHeight, sizeof(int));
#endif
}

status_t CallbackNotifier::adasDetectionMsg(void *adas)
{
    if (isMessageEnabled(CAMERA_MSG_ADAS_METADATA))
    {
        ADASOUT_IF *adasOut_if = (ADASOUT_IF*)adas;
        size_t size = getAdasOutDataLength(adasOut_if);
        camera_memory_t *adas_buff = mGetMemoryCB(-1, size, 1, NULL);
        if (NULL != adas_buff || NULL != adas_buff->data) {
            copyAdasOutData(adas_buff->data, adasOut_if);
            mDataCB(CAMERA_MSG_ADAS_METADATA, adas_buff, 0, NULL, mCallbackCookie);
            adas_buff->release(adas_buff);
        } else {
            LOGE("Memory failure in CAMERA_MSG_ADAS_METADATA");
            return NO_MEMORY;
        }
    }
    return NO_ERROR;
}
/* for ADAS end */

}; /* namespace android */
