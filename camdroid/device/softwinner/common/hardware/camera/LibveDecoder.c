
#include "CameraDebug.h"
#if DBG_DECODER
#define LOG_NDEBUG 0
#endif
#define LOG_TAG "LibveDecoder"
#include <cutils/log.h>
#include <videodev2.h>

#include "LibveDecoder.h"
//#include "FormatConvert.h"

#define CAMERA_VEDECODER_VBV_SIZE  (512*1024)

int CameraHal_LRFlipFrame(int nFormat, int nWidth, int nHeight, char* pAddrY, char* pAddrC)
{
    if(nFormat != V4L2_PIX_FMT_NV21 && nFormat != V4L2_PIX_FMT_NV12)
    {
        ALOGW("[%s](f:%s, l:%d) fatal error! nFormat[0x%x] is not support!", strrchr(__FILE__, '/')+1, __FUNCTION__, __LINE__, nFormat);
        return -1;
    }
    if(nWidth%32!=0 || nHeight%2!=0)
    {
        ALOGW("(f:%s, l:%d) fatal error! size[%dx%d] is not valid!", __FUNCTION__, __LINE__, nWidth, nHeight);
        return -1;
    }
    unsigned int i,j;
    unsigned int width_16_align = nWidth;
    unsigned int height_16_align = nHeight;
    unsigned char *src_y = pAddrY;
    unsigned char *src_c = pAddrC;
    unsigned char *p0 = NULL;
    unsigned char *p1 = NULL;
    unsigned char *p2 = NULL;

    unsigned char *p3 = NULL;
    unsigned char *p4 = NULL;
    unsigned char *p5 = NULL;

#define USE_NEON
#ifdef USE_NEON
    p0 = src_y;
    p1 = src_c;
    p2 = src_y + width_16_align;

    p3 = src_y + width_16_align - 16;
    p4 = src_c + width_16_align - 16;
    p5 = src_y + 2*width_16_align - 16;

    for(i=0; i<height_16_align/2; i++)
    {
        for(j=0; j<width_16_align/32; j++)
        {
            asm volatile
            (
                "pld		 	 [%[p0],#32]				\n\t"
                "pld		 	 [%[p1],#32]				\n\t"
                "pld		 	 [%[p2],#32]				\n\t"

                "pld		 	 [%[p3],#-32]				\n\t"
                "pld		 	 [%[p4],#-32]				\n\t"
                "pld		 	 [%[p5],#-32]				\n\t"

                "vld1.u8		 {d0,d1},  [%[p0]]			\n\t"
                "vld1.u8		 {d2,d3},  [%[p2]]			\n\t"
                "vld1.u8		 {d4,d5},  [%[p1]]			\n\t"

                "vld1.u8		 {d6,d7},  [%[p3]]			\n\t"
                "vld1.u8		 {d8,d9},  [%[p5]]			\n\t"
                "vld1.u8		 {d10,d11},[%[p4]]			\n\t"


                "vrev64.8		 q15,  q0					\n\t"
                "vrev64.8		 q14,  q1					\n\t"
                "vrev64.16		 q13,  q2					\n\t"
                "vswp.8		 	 d30,  d31					\n\t"
                "vswp.8		 	 d28,  d29					\n\t"
                "vswp.8		 	 d26,  d27					\n\t"

                "vrev64.8		 q12,  q3					\n\t"
                "vrev64.8		 q11,  q4					\n\t"
                "vrev64.16		 q10,  q5					\n\t"
                "vswp.8		 	 d24,  d25					\n\t"
                "vswp.8		 	 d22,  d23					\n\t"
                "vswp.8		 	 d20,  d21					\n\t"


                "vst1.u8		 {d30,d31},  [%[p3]]		\n\t"
                "vst1.u8		 {d28,d29},  [%[p5]]		\n\t"
                "vst1.u8		 {d26,d27},  [%[p4]]		\n\t"

                "vst1.u8		 {d24,d25},  [%[p0]]!		\n\t"
                "vst1.u8		 {d22,d23},  [%[p2]]!		\n\t"
                "vst1.u8		 {d20,d21},  [%[p1]]!		\n\t"
                : [p0] "+r" (p0), [p1] "+r" (p1), [p2] "+r" (p2), [p3] "+r" (p3), [p4] "+r" (p4), [p5] "+r" (p5)
                :  //[srcY] "r" (srcY)
                : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "d10", "d11", "d20", "d21", "d22", "d23", "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31"
            );
            p3 -= 16;
            p4 -= 16;
            p5 -= 16;
        }
        p0 += width_16_align*3/2;
        p1 += width_16_align/2;
        p2 += width_16_align*3/2;

        p3 += width_16_align*5/2;
        p4 += width_16_align*3/2;
        p5 += width_16_align*5/2;	
    }
#else
    p0 = src_y;
    p1 = src_c;

    p2 = tmp_y + width_16_align -1;
    p3 = tmp_c + width_16_align -1;

    time1 = GetNowUs();

    for(i=0; i<height_16_align; i++)
    {
        if(i%2 == 0)
        {
            for(j=0; j<width_16_align; j++)
            {
                *p2-- = *p0++;
                *p3-- = *p1++;
            }
            p3 += 2*width_16_align;
        }
        else
        {
            for(j=0; j<width_16_align; j++)
            {
                *p2-- = *p0++;
            }
        }

        p2 += 2*width_16_align;	
    }
    memcpy(src_y, tmp_y, width_16_align*height_16_align);
    memcpy(src_c, tmp_c, width_16_align*height_16_align/2);
#endif
    return 0;
}

static int GetStreamData(const void* in, char* buf0, int buf_size0, char* buf1, int buf_size1, VideoStreamDataInfo* data_info)  //cedarv_stream_data_info_t
{
	ALOGV("Starting get stream data!!");
	if(data_info->nLength <= buf_size0) {
			ALOGV("The stream lengh is %d, the buf_size0 is %d",data_info->nLength,buf_size0);
			memcpy(buf0, in, data_info->nLength);
	}
	else {
		if(data_info->nLength <= (buf_size0+buf_size1)){
			ALOGV("The stream lengh is %d, the buf_size0 is %d,the buf_size1 is %d",data_info->nLength,buf_size0,buf_size1);
			memcpy(buf0, in, buf_size0);
			memcpy(buf1,(void*)((int)in + buf_size0),(data_info->nLength - buf_size0));
		}
		else
		{
            ALOGW("[%s](f:%s, l:%d) fatal error! nLength[%d]>size0[%d]+size1[%d]", strrchr(__FILE__, '/')+1, __FUNCTION__, __LINE__, data_info->nLength, buf_size0, buf_size1);
			return -1;
		}
	}
    data_info->bIsFirstPart = 1;
    data_info->bIsLastPart  = 1;
    data_info->pData = (char*)buf0;
    data_info->nStreamIndex = 0;

	return 0;
}

DecodeHandle* libveInit(int capfmt, int outfmt, int w, int h, int subw, int subh, int fps, int *scl)
{
	int ret;
    int format;
    int out_fmt;
    int scale = 0, scale_en = 0;
	DecodeHandle *dec_handle;

    if (capfmt == V4L2_PIX_FMT_H264) {
        format = VIDEO_CODEC_FORMAT_H264;
    } else if (capfmt == V4L2_PIX_FMT_MJPEG) {
        format = VIDEO_CODEC_FORMAT_MJPEG;
    } else {
        ALOGE("<F:%s, L:%d>Invalid capfmt(%d)", __FUNCTION__, __LINE__, capfmt);
        return NULL;
    }

    if (outfmt == V4L2_PIX_FMT_NV21) {
        out_fmt = PIXEL_FORMAT_NV21;
    } else if (outfmt == V4L2_PIX_FMT_NV12) {
        out_fmt = PIXEL_FORMAT_NV12;
    } else {
        out_fmt = PIXEL_FORMAT_DEFAULT;
    }

    ALOGD("video codec format=%d, output pixel format=%d, ", format, out_fmt);
	dec_handle = malloc(sizeof(DecodeHandle));
	if(dec_handle == NULL) {
		LOGW("malloc for DecodeHandle fail.\n");
		return NULL;
	}
	memset(dec_handle, 0, sizeof(DecodeHandle));

    dec_handle->mDecoder = CreateVideoDecoder();
	ALOGV("libcedarv_init!!");
	if(NULL == dec_handle->mDecoder){
		ALOGE("can not initialize the mDecoder library.");
		goto DEC_ERR2;
	}

    dec_handle->mStream_info.eCodecFormat = format;  //VIDEO_CODEC_FORMAT_H264
    dec_handle->mStream_info.nWidth   = w;
	dec_handle->mStream_info.nHeight  = h;
    dec_handle->mStream_info.nFrameRate = fps * 1000;
    dec_handle->mStream_info.nFrameDuration = (1000*1000*1000)/(dec_handle->mStream_info.nFrameRate);
    dec_handle->mStream_info.nCodecSpecificDataLen = 0;
	dec_handle->mStream_info.pCodecSpecificData = NULL;

    dec_handle->mDecoderConfig.eOutputPixelFormat  = out_fmt;
    dec_handle->mDecoderConfig.bDisable3D          = 1;
    dec_handle->mDecoderConfig.nVbvBufferSize      = CAMERA_VEDECODER_VBV_SIZE;

    if (subw > 0 && subh > 0) {
        float ratio = (float)w / subw;
        if (ratio <= 1) {
            scale = 0;
            scale_en = 0;
        } else if (ratio <= 2) {
            scale = 1;
            scale_en = 1;
        } else {
            scale = 2;
            scale_en = 1;
        }
    } else {
        scale = 0;
        scale_en = 0;
    }
    *scl = scale_en;

    ALOGD("(f:%s, l:%d) ScaleDownEn=%d, scale=%d, [%dx%d][%dx%d]", __FUNCTION__, __LINE__, scale_en, scale, w, h, subw, subh);
    dec_handle->mDecoderConfig.bScaleDownEn = scale_en;
    dec_handle->mDecoderConfig.nHorizonScaleDownRatio = 0;
    dec_handle->mDecoderConfig.nVerticalScaleDownRatio = 0;
    dec_handle->mDecoderConfig.nSecHorizonScaleDownRatio = scale;
    dec_handle->mDecoderConfig.nSecVerticalScaleDownRatio = scale;

	ALOGD("mDecoder->open!!");
	ret = InitializeVideoDecoder(dec_handle->mDecoder, &dec_handle->mStream_info, &dec_handle->mDecoderConfig);
	if(ret < 0){
		ALOGE("can not open mDecoder.");
		goto DEC_ERR3;
	}

	ALOGD("libveInit OK !");
	return dec_handle;

DEC_ERR3:
    DestroyVideoDecoder(dec_handle->mDecoder);
DEC_ERR2:
DEC_ERR1:	
	free(dec_handle);
	ALOGD("libveInit FAIL !");
	return NULL;
}

int libveExit(DecodeHandle *handle) {
	DecodeHandle *dec_handle = handle;

	if(dec_handle == NULL) {
		return -1;
	}

    DestroyVideoDecoder(dec_handle->mDecoder);
	free(dec_handle);
	ALOGD("libveExit OK !");
	
	return 0;
}

int libveDecode(DecodeHandle *handle, const void *data_in, int data_size, int64_t pts) {
	int     ret;
	char*		buf0;
	char*		buf1;
	int		buf0size;	
	int   	buf1size;
	DecodeHandle *dec_handle = handle;

	if(dec_handle == NULL) {
		return -1;
	}
	ALOGV("libveDecode!");
    ret = RequestVideoStreamBuffer(dec_handle->mDecoder, 
                                   data_size, 
                                   (char**)&buf0, 
                                   &buf0size,
                                   (char**)&buf1,
                                   &buf1size,
                                   0);
	if(ret < 0){
		ALOGE("request bit stream buffer fail.");
		//return  ret;
	} else {
    	ALOGV("GetStreamData!!");
    	dec_handle->mData_info.nLength = data_size;
    	dec_handle->mData_info.nPts = (int64_t)pts;
    	GetStreamData(data_in, buf0, buf0size, buf1, buf1size, &dec_handle->mData_info);
        SubmitVideoStreamData(dec_handle->mDecoder, &dec_handle->mData_info, dec_handle->mData_info.nStreamIndex);
	}
DECODE_AGAIN:
    ret = DecodeVideoStream(dec_handle->mDecoder, 0, 0, 0, 0);
	if(ret <= VDECODE_RESULT_UNSUPPORTED)
    {
		ALOGE("bit stream is unsupported, ret = %d.", ret);
	} 
    else if(ret==VDECODE_RESULT_OK || ret==VDECODE_RESULT_CONTINUE) 
    {
		ALOGV("Successfully return,decording!!");
	} 
    else if(ret==VDECODE_RESULT_FRAME_DECODED) 
    {
		ALOGV("One frame decorded!!");	
	} 
    else if(ret == VDECODE_RESULT_KEYFRAME_DECODED) 
    {
		ALOGV("One key frame decorded!!");
	} 
    else if(ret == VDECODE_RESULT_NO_FRAME_BUFFER) 
    {
        ALOGD("(f:%s, l:%d) No frame buffer, wait 5ms!", __FUNCTION__, __LINE__);
        usleep(5*1000);
        goto DECODE_AGAIN;
    }
    else 
    {
		ALOGE("(f:%s, l:%d) decode fail. ret = %d !!", __FUNCTION__, __LINE__, ret);
	}
	
	return  ret;
}

VideoPicture* libveGetFrame(DecodeHandle *handle, int streamIdx)
{
	if(handle == NULL) {
        ALOGE("<F:%s, L:%d>DecodeHandle is NULL!", __FUNCTION__, __LINE__);
		return NULL;
	}

    return RequestPicture(handle->mDecoder, streamIdx);
}

int libveReleaseFrame(DecodeHandle *handle, VideoPicture *picture)
{
	if(handle == NULL || picture == NULL) {
        ALOGE("<F:%s, L:%d>DecodeHandle or picture is NULL!", __FUNCTION__, __LINE__);
		return -1;
	}

    return ReturnPicture(handle->mDecoder, picture);
}


