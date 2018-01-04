//#define LOG_NDEBUG 0
#define LOG_TAG "JpegDecoder"
#include <cutils/log.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <binder/IMemory.h>

#include "jpegDecoder.h"


#define JPEG_DECODER_MAX_OUT_WIDTH      600
#define JPEG_DECODER_MAX_OUT_HEIGHT     480
namespace android {

JpegDecoder::JpegDecoder()
{
    ALOGV("constructor");
    mDecoder = NULL;
}

JpegDecoder::~JpegDecoder()
{
    ALOGV("destructor");
}

status_t JpegDecoder::initialize(int jpgWidth, int jpgHeight)
{
    int ret;
    int i;
    int nOutput, nActual;

    ALOGV("initialize(%d, %d)", jpgWidth, jpgHeight);
    mDecoder = CreateVideoDecoder();
    if(mDecoder == NULL){
        ALOGE("<F:%s, L:%d> CreateVideoDecoder error!", __FUNCTION__, __LINE__);
        return UNKNOWN_ERROR;
    }

    memset(&mStreamInfo, 0, sizeof(VideoStreamInfo));
    memset(&mConfig, 0, sizeof(VConfig));
    memset(&mDataInfo, 0, sizeof(VideoStreamDataInfo));

    mStreamInfo.eCodecFormat            = VIDEO_CODEC_FORMAT_MJPEG;
    mStreamInfo.nWidth                  = jpgWidth;
    mStreamInfo.nHeight                 = jpgHeight;
    mStreamInfo.nFrameRate              = 25*1000;
    mStreamInfo.nFrameDuration          = 0;
    mStreamInfo.nCodecSpecificDataLen   = 0;
    mStreamInfo.pCodecSpecificData      = NULL;
    mStreamInfo.nAspectRatio            = 0;
    mStreamInfo.bIs3DStream             = 0;

    //mConfig.eOutputPixelFormat          = PIXEL_FORMAT_YV12;
    mConfig.eOutputPixelFormat          = PIXEL_FORMAT_YUV_PLANER_420;
    //mConfig.eOutputPixelFormat          = PIXEL_FORMAT_YUV_MB32_420;
    mConfig.bDisable3D                  = 1;
    mConfig.nVbvBufferSize              = 4*1024*1024;
    mConfig.bScaleDownEn = 0;
    mConfig.nHorizonScaleDownRatio = 0;
    mConfig.nVerticalScaleDownRatio = 0;
    mConfig.nSecHorizonScaleDownRatio = 0;
    mConfig.nSecVerticalScaleDownRatio = 0;
    mConfig.nFrameBufferNum = 1;

#if 1
    if (mStreamInfo.nWidth > JPEG_DECODER_MAX_OUT_WIDTH)
    {
        mConfig.bScaleDownEn = 1;
        nOutput = JPEG_DECODER_MAX_OUT_WIDTH;
        nActual = mStreamInfo.nWidth;
        for (i=0; nOutput<nActual; i++)
        {
            nOutput*=2;
        }
        mConfig.nHorizonScaleDownRatio = i;
    }
    if (mStreamInfo.nHeight > JPEG_DECODER_MAX_OUT_HEIGHT)
    {
        mConfig.bScaleDownEn = 1;
        nOutput = JPEG_DECODER_MAX_OUT_HEIGHT;
        nActual = mStreamInfo.nHeight;
        for (i=0; nOutput<nActual; i++)
        {
            nOutput*=2;
        }
        mConfig.nVerticalScaleDownRatio = i;
    }
    if (mConfig.bScaleDownEn == 1) {
        if (mConfig.nHorizonScaleDownRatio > mConfig.nVerticalScaleDownRatio) {
            mConfig.nVerticalScaleDownRatio = mConfig.nHorizonScaleDownRatio;
        } else {
            mConfig.nHorizonScaleDownRatio = mConfig.nVerticalScaleDownRatio;
        }
    }
    if (mConfig.nVerticalScaleDownRatio > 3) {
        mConfig.nVerticalScaleDownRatio = 3;
        mConfig.nHorizonScaleDownRatio = 3;
    }
#endif

	ret = InitializeVideoDecoder(mDecoder, &mStreamInfo, &mConfig);
	if(ret < 0){
		ALOGE("<F:%s, L:%d> InitializeVideoDecoder error!", __FUNCTION__, __LINE__);
		DestroyVideoDecoder(mDecoder);
        mDecoder = NULL;
        return UNKNOWN_ERROR;
	}
    return NO_ERROR;
}

status_t JpegDecoder::destroy()
{
	ALOGV("destroy");
    if (mDecoder == NULL) {
        ALOGW("<F:%s, L:%d> JpegDecoder not initialize yet!", __FUNCTION__, __LINE__);
        return NO_ERROR;
    }
    DestroyVideoDecoder(mDecoder);
    mDecoder = NULL;
    return NO_ERROR;
}

status_t JpegDecoder::requestBuffer(int size, char **buf0, int *size0, char **buf1, int *size1)
{
    int ret;

    ALOGV("requestBuffer");
    if (mDecoder == NULL) {
        ALOGE("<F:%s, L:%d> JpegDecoder not initialize!", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
    ret = RequestVideoStreamBuffer(mDecoder, size, buf0, size0, buf1, size1, 0);
    if (ret < 0) {
        ALOGE("<F:%s, L:%d> RequestVideoStreamBuffer error!", __FUNCTION__, __LINE__);
        return UNKNOWN_ERROR;
    }
    return NO_ERROR;
}

status_t JpegDecoder::updateBuffer(int size, int64_t pts, char *data)
{
    ALOGV("updateBuffer");

    if (mDecoder == NULL) {
        ALOGE("JpegDecoder not initialize!");
        return NO_INIT;
    }

    mDataInfo.nLength = size;
    mDataInfo.nPts = pts;
    mDataInfo.bIsFirstPart = 1;
    mDataInfo.bIsLastPart  = 1;
    mDataInfo.pData = data;
    mDataInfo.nStreamIndex = 0;
    SubmitVideoStreamData(mDecoder, &mDataInfo, mDataInfo.nStreamIndex);
    return NO_ERROR;
}

status_t JpegDecoder::decode()
{
    int ret;

    ALOGV("decode");

    if (mDecoder == NULL) {
        ALOGE("JpegDecoder not initialize!");
        return NO_INIT;
    }

    ret = DecodeVideoStream(mDecoder, 0, 0, 0, 0);

    if(ret <= VDECODE_RESULT_UNSUPPORTED) {
        ALOGE("bit stream is unsupported, ret = %d.\n", ret);
        return INVALID_OPERATION;
    } else if(ret==VDECODE_RESULT_OK || ret==VDECODE_RESULT_CONTINUE) {
        ALOGV("Successfully return,decording!!");
        return NO_ERROR;
    } else if(ret==VDECODE_RESULT_FRAME_DECODED) {
        ALOGV("One frame decorded!!");
        return NO_ERROR;
    } else if(ret == VDECODE_RESULT_KEYFRAME_DECODED) {
        ALOGV("One key frame decorded!!");
        return NO_ERROR;
    } else {
        ALOGE("decode fail. ret = %d !!", ret);
        return UNKNOWN_ERROR;
    }

    return NO_ERROR;
}

VideoPicture* JpegDecoder::getFrame(void)
{
    if (mDecoder == NULL) {
        ALOGE("JpegDecoder not initialize!");
        return NULL;
    }
    return RequestPicture(mDecoder, 0);
}

status_t JpegDecoder::releaseFrame(VideoPicture *picture)
{
    if (mDecoder == NULL) {
        ALOGE("JpegDecoder not initialize!");
        return NO_INIT;
    }
    return ReturnPicture(mDecoder, picture);
}

void JpegDecoder::YV12_to_RGB24(unsigned char *yv12, unsigned char *rgb24, int width, int height)
{
    if(yv12 == NULL || rgb24 == NULL) {
        return;
    }
    const long nYLen = long(height * width);
    const int nHfWidth = (width >> 1);
    if(nYLen < 1 || nHfWidth < 1)  {
        return;
    }

    // yv12 format £¬y data length is width*height, U data and V data length is width*height/4
    unsigned char *yData = yv12; //y address
    unsigned char *vData = yData + nYLen; //u address
    unsigned char *uData = vData + nYLen / 4;  //v address
    // Convert YV12 to RGB24
    int rgb[3];
    int i, j, m, n, x, y;
    m = -width;
    n = -nHfWidth;
    for(y = 0; y < height; ++y)
    {
        m += width;
        if(!(y % 2)) {
            n += nHfWidth;
        }
        for(x = 0; x < width; ++x)
        {
            i = m + x;
            j = n + (x >> 1);
#if 0
            rgb[2] = int((float)yData[i] + 1.370705 * (float)(vData[j] - 128));
            rgb[1] = int((float)yData[i] - 0.698001 * (float)(uData[j] - 128)  - 0.703125 * (float)(vData[j] - 128));
            rgb[0] = int((float)yData[i] + 1.732446 * (float)(uData[j] - 128));
#else
            rgb[2] = int((float)yData[i] + 1.13983 * (float)(vData[j] - 128));
            rgb[1] = int((float)yData[i] - 0.39465 * (float)(uData[j] - 128)  - 0.58060 * (float)(vData[j] - 128));
            rgb[0] = int((float)yData[i] + 2.03211 * (float)(uData[j] - 128));
#endif
            j = nYLen - width - m + x;
            i = (j << 1) + j;
            for(int j = 0; j < 3; ++j)
            {
                if (rgb[j] < 0) {
                    rgb24[i+j] = 0;
                } else if (rgb[j] > 255) {
                    rgb24[i+j] = 255;
                } else {
                    rgb24[i+j] = rgb[j];
                }
            }
        }
    }
}

void JpegDecoder::RGB24_to_ARGB(unsigned char *rgb24, unsigned char *argb, int width, int height)
{
    int len = width * height;
    int i = 0;

    while (i < len) {
        *argb = *rgb24;
        ++argb;
        ++rgb24;
        *argb = *rgb24;
        ++argb;
        ++rgb24;
        *argb = *rgb24;
        ++argb;
        ++rgb24;
        *argb = 0xff;
        ++argb;
        ++i;
    }
}

extern "C" int getJpegInfoSize(uint8_t *data, uint32_t size, int *width, int *height);
status_t JpegDecoder::getJpgInfo(JpegInfo *jpg)
{
    return getJpegInfoSize(jpg->data, jpg->datasize, &jpg->width, &jpg->height);
}

}; /* namespace android */

