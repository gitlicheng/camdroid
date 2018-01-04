/*
********************************************************************************
*                           Android multimedia module
*
*          (c) Copyright 2010-2015, Allwinner Microelectronic Co., Ltd.
*                              All Rights Reserved
*
* File   : VideoResizerDecoder.cpp
* Version: V1.0
* By     : eric_wang
* Date   : 2014-12-14
* Description:
********************************************************************************
*/
//#define LOG_NDEBUG 0
#define LOG_TAG "VideoResizerDecoder"

//#include <cutils/log.h>
#include <utils/Log.h>

#include "VideoResizerDecoder.h"
#include "VideoResizerEncoder.h"
#include <memoryAdapter.h>

namespace android
{
#if 0
cedarv_stream_format_e convertOMX_VIDEO_CODINGTYPE2cedarv_stream_format_e(OMX_VIDEO_CODINGTYPE eCompressionFormat)
{
    switch (eCompressionFormat)
	{
	case OMX_VIDEO_CodingRV:
		return CEDARV_STREAM_FORMAT_REALVIDEO;
	case OMX_VIDEO_CodingAVC:
		return CEDARV_STREAM_FORMAT_H264;
	case OMX_VIDEO_CodingMPEG2:
	case OMX_VIDEO_CodingMPEG1:
		return CEDARV_STREAM_FORMAT_MPEG2;
	case OMX_VIDEO_CodingMPEG4:
		return CEDARV_STREAM_FORMAT_MPEG4;
	case OMX_VIDEO_CodingWMV:
		return CEDARV_STREAM_FORMAT_VC1;
	case OMX_VIDEO_CodingMJPEG:
		return CEDARV_STREAM_FORMAT_MJPEG;
	case OMX_VIDEO_CodingVP8:
	    return CEDARV_STREAM_FORMAT_VP8;
	default:
        ALOGD("(f:%s, l:%d) fatal error! unknown eCompressionFormat[0x%x]", __FUNCTION__, __LINE__, eCompressionFormat);
		return CEDARV_STREAM_FORMAT_UNKNOW;
	}
}

cedarv_container_format_e convertCDX_MEDIA_FILE_FORMAT2cedarv_container_format_e(CDX_MEDIA_FILE_FORMAT demux_type)
{
    cedarv_container_format_e container_format;
    switch (demux_type)
	{
	case CDX_MEDIA_FILE_FMT_RM:
		container_format = CEDARV_CONTAINER_FORMAT_RM;
		break;
	case CDX_MEDIA_FILE_FMT_MKV:
		container_format = CEDARV_CONTAINER_FORMAT_MKV;
		break;
	case CDX_MEDIA_FILE_FMT_TS:
		container_format = CEDARV_CONTAINER_FORMAT_TS;
		break;
	case CDX_MEDIA_FILE_FMT_AVI:
		container_format = CEDARV_CONTAINER_FORMAT_AVI;
		break;
	case CDX_MEDIA_FILE_FMT_MOV:
		container_format = CEDARV_CONTAINER_FORMAT_MOV;
		break;
	case CDX_MEDIA_FILE_FMT_FLV:
		container_format = CEDARV_CONTAINER_FORMAT_FLV;
		break;
	case CDX_MEDIA_FILE_FMT_MPG:
		container_format = CEDARV_CONTAINER_FORMAT_MPG;
		break;
	case CDX_MEDIA_FILE_FMT_ASF:
		container_format = CEDARV_CONTAINER_FORMAT_ASF;
		break;
	case CDX_MEDIA_FILE_FMT_VOB:
		container_format = CEDARV_CONTAINER_FORMAT_VOB;
		break;
	case CDX_MEDIA_FILE_FMT_PMP:
		container_format = CEDARV_CONTAINER_FORMAT_PMP;
		break;
    case CDX_MEDIA_FILE_FMT_WEBM:
        container_format = CEDARV_CONTAINER_FORMAT_WEBM;
        break;
	default:
		container_format = CEDARV_CONTAINER_FORMAT_UNKNOW;
		break;
	}
    return container_format;
}

typedef enum FrameConvertPixelFormat
{
    FRAME_CONVERT_PIXEL_FORMAT_YUV420MB32   = 0x0,
    FRAME_CONVERT_PIXEL_FORMAT_YV12         = 0x1,
    FRAME_CONVERT_PIXEL_FORMAT_NV12         = 0x2,
    FRAME_CONVERT_PIXEL_FORMAT_NV21         = 0x3
}FrameConvertPixelFormat;
typedef struct FrameConvertParameter
{
	FrameConvertPixelFormat mFormatIn;
	FrameConvertPixelFormat mFormatOut;

	int mStoreWidth;    //frame buffer size, base on YBuf.
	int mStoreHeight;

	char *mAddrYIn;
	char *mAddrCIn;
	char *mAddrYOut;
	char *mAddrUOut;
	char *mAddrVOut;
}FrameConvertParameter;

static void map32x32ToYUV_Y(char* srcY, char* tarY,unsigned int nStoreWidth,unsigned int nStoreHeight)
{
	unsigned int i,j,l,m,n;
	unsigned int mb32_line, mb32_width;
	unsigned long offset;
	char *ptr;
	char *dst_asm,*src_asm;
    unsigned int vdecbuf_width, vdecbuf_height;
	ptr = srcY;
    vdecbuf_width = ALIGN32(nStoreWidth);
    vdecbuf_height = ALIGN32(nStoreHeight);
    if(vdecbuf_width!=nStoreWidth || vdecbuf_height!=nStoreHeight)
    {
        ALOGE("(f:%s, l:%d) fatal error! alignBufSize[%dx%d]!= inputBufSize[%dx%d], check code!", __FUNCTION__, __LINE__,
            vdecbuf_width, vdecbuf_height, nStoreWidth, nStoreHeight);
        return;
    }
    mb32_width = nStoreWidth/32;
    mb32_line = nStoreHeight/32;
    
	for(i=0;i<mb32_line;i++)   //mb32 line number
	{
		for(j=0;j<mb32_width;j++)   //macroblock(32*32) number in one line
		{
			for(l=0;l<32;l++)
			{
				//first mb
				m=i*32 + l;     //line num
				n= j*32;        //byte num in one line
				offset = m*nStoreWidth + n;
				//memcpy(tarY+offset,ptr,32);
				dst_asm = tarY+offset;
				src_asm = ptr;
				asm volatile (
				        "vld1.8         {d0 - d3}, [%[src_asm]]              \n\t"
				        "vst1.8         {d0 - d3}, [%[dst_asm]]              \n\t"
				        : [dst_asm] "+r" (dst_asm), [src_asm] "+r" (src_asm)
				        :  //[srcY] "r" (srcY)
				        : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23", "d24", "d28", "d29", "d30", "d31"
				        );

				ptr += 32;  //32 byte in one process.
			}
		}
	}
}

/*******************************************************************************
Function name: android.map32x32ToYUV_UVCombined
Description: 
    we take uv as one point. One point size is 2 bytes.
Parameters: 
    nStoreWidth: picture pixel number in one line, 
    nStoreHeight: picture line number, 
Return: 
    
Time: 2014/12/14
*******************************************************************************/
static void map32x32ToYUV_UVCombined(char* srcC, char* tarC, unsigned int nStoreWidth,unsigned int nStoreHeight)
{
	unsigned int i,j,l,m,n;
	unsigned int mb32_line, mb32_width;
    unsigned int uvPointWidth, uvPointHeight;
    unsigned int nValidLine;
	unsigned long offset;
	char *ptr;
	char *dst_asm,*src_asm;
    unsigned int vdecbuf_width, vdecbuf_height;
	ptr = srcC;
    vdecbuf_width = ALIGN32(nStoreWidth);
    vdecbuf_height = ALIGN32(nStoreHeight);
    if(vdecbuf_width!=nStoreWidth || vdecbuf_height!=nStoreHeight)
    {
        ALOGE("(f:%s, l:%d) fatal error! alignBufSize[%dx%d]!= inputBufSize[%dx%d], check code!", __FUNCTION__, __LINE__,
            vdecbuf_width, vdecbuf_height, nStoreWidth, nStoreHeight);
        return;
    }
    uvPointWidth = nStoreWidth/2;
    uvPointHeight = nStoreHeight/2;
    mb32_width = uvPointWidth/16;
    mb32_line = ALIGN32(uvPointHeight)/32;
	for(i=0;i<mb32_line;i++)   //uv_combined mb32 line number
	{
        if((i+1)*32 > uvPointHeight)
        {
            nValidLine = uvPointHeight - i*32;
        }
        else
        {
            nValidLine = 32;
        }
		for(j=0;j<mb32_width;j++)   //macroblock(32*32) number in one line
		{
			for(l=0;l<nValidLine;l++)
			{
				//first mb
				m=i*32 + l;     //line num
				n= j*32;        //byte num in one line
				offset = m*uvPointWidth*2 + n;
				//memcpy(tarY+offset,ptr,32);
				dst_asm = tarC+offset;
				src_asm = ptr;
				asm volatile (
				        "vld1.8         {d0 - d3}, [%[src_asm]]              \n\t"
				        "vst1.8         {d0 - d3}, [%[dst_asm]]              \n\t"
				        : [dst_asm] "+r" (dst_asm), [src_asm] "+r" (src_asm)
				        :  //[srcY] "r" (srcY)
				        : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23", "d24", "d28", "d29", "d30", "d31"
				        );

				ptr += 32;  //32 byte in one process.
			}
            ptr += (32-nValidLine)*32;
		}
	}
}
int SoftFrameFormatConverter_YUV420MB32ToNV12(FrameConvertParameter *pFCPara)
{
    map32x32ToYUV_Y(pFCPara->mAddrYIn, pFCPara->mAddrYOut, pFCPara->mStoreWidth, pFCPara->mStoreHeight);
    map32x32ToYUV_UVCombined(pFCPara->mAddrCIn, pFCPara->mAddrUOut, pFCPara->mStoreWidth, pFCPara->mStoreHeight);
    return 0;
}

int SoftFrameFormatConvert(FrameConvertParameter *pFCPara)
{
    if(FRAME_CONVERT_PIXEL_FORMAT_YUV420MB32 == pFCPara->mFormatIn
        && FRAME_CONVERT_PIXEL_FORMAT_NV12 == pFCPara->mFormatOut)
    {
        return SoftFrameFormatConverter_YUV420MB32ToNV12(pFCPara);
    }
    return -1;
}

status_t convertMB32ToNV12(cedarv_picture_t *pCedarvPic, VdecOutFrame *pOutFrame)
{
    int vdeclibStoreWidth = ALIGN32(pCedarvPic->display_width);
    int vdeclibStoreHeight = ALIGN32(pCedarvPic->display_height);
    if(vdeclibStoreWidth != pOutFrame->mStoreWidth || vdeclibStoreHeight != pOutFrame->mStoreHeight)
    {
        ALOGE("(f:%s, l:%d) fatal error! why vdeclibStoreSize[%dx%d]!= preSettingStoreSize[%dx%d]? check code!", __FUNCTION__, __LINE__,
            vdeclibStoreWidth, vdeclibStoreHeight, pOutFrame->mStoreWidth, pOutFrame->mStoreHeight);
    }
    FrameConvertParameter fcPara;
    fcPara.mFormatIn = FRAME_CONVERT_PIXEL_FORMAT_YUV420MB32;
    fcPara.mFormatOut = FRAME_CONVERT_PIXEL_FORMAT_NV12;
    fcPara.mStoreWidth = vdeclibStoreWidth;
    fcPara.mStoreHeight = vdeclibStoreHeight;
    fcPara.mAddrYIn = (char*)cedarv_address_phy2vir(pCedarvPic->y);
    fcPara.mAddrCIn = (char*)cedarv_address_phy2vir(pCedarvPic->u);
    fcPara.mAddrYOut = pOutFrame->mBuf;
    fcPara.mAddrUOut = pOutFrame->mBuf + fcPara.mStoreWidth*fcPara.mStoreHeight;
    fcPara.mAddrVOut = NULL;
    //SoftwarePictureScaler(ScalerParameter *cdx_scaler_para);
    SoftFrameFormatConvert(&fcPara);
    return NO_ERROR;
}
#endif
VdecOutFrame::VdecOutFrame(int storeWidth, int storeHeight, enum EPIXELFORMAT format)
    :/*mBuf(NULL), mPhyBuf(NULL), */mStoreWidth(storeWidth), mStoreHeight(storeHeight), mPixelFormat(format)
{
/*
    mBufSize = mStoreWidth*mStoreHeight*3/2;
    //mBuf = (char*)cedar_sys_phymalloc_map((unsigned int)mBufSize, 4096);
    mBuf = (char*)MemAdapterPalloc((unsigned int)mBufSize);
	if(mBuf == NULL)
    {
        ALOGE("(f:%s, l:%d) phymalloc fail", __FUNCTION__, __LINE__);
        return;
	}
    mPhyBuf = (char*)MemAdapterGetPhysicAddressCpu(mBuf);
*/
    mDisplayWidth = 0;
    mDisplayHeight = 0;
    mPts = -1;
    mStatus = OwnedByUs;
}
VdecOutFrame::~VdecOutFrame()
{
/*
    if(mBuf)
    {
        MemAdapterPfree(mBuf);
    }
*/
}

status_t VdecOutFrame::setFrameInfo(VideoPicture *pCedarvPic)   //cedarv_picture_t
{
    mDisplayWidth = pCedarvPic->nWidth;
    mDisplayHeight = pCedarvPic->nHeight;
    mPts = pCedarvPic->nPts;
    mPicture = pCedarvPic;

    return NO_ERROR;
}
/*
status_t VdecOutFrame::copyFrame(VideoPicture *pCedarvPic)
{
    memset(mBuf, 0, mBufSize);
    memcpy(mBuf, pCedarvPic->pData0, mDisplayWidth*mDisplayHeight);
    memcpy(mBuf + mStoreWidth*mStoreHeight, pCedarvPic->pData1, mDisplayWidth*mDisplayHeight/2);

    return NO_ERROR;
}
*/
VideoResizerDecoder::DoDecodeThread::DoDecodeThread(VideoResizerDecoder *pOwner)
    : Thread(false),
      mpOwner(pOwner),
      mThreadId(NULL)
{
}
void VideoResizerDecoder::DoDecodeThread::startThread()
{
    run("ResizeDecodeThread", PRIORITY_DEFAULT);    //PRIORITY_DISPLAY
}
void VideoResizerDecoder::DoDecodeThread::stopThread()
{
    requestExitAndWait();
}
status_t VideoResizerDecoder::DoDecodeThread::readyToRun() 
{
    mThreadId = androidGetThreadId();

    return Thread::readyToRun();
}
bool VideoResizerDecoder::DoDecodeThread::threadLoop()
{
    if(!exitPending())
    {
        return mpOwner->decodeThread();
    }
    else
    {
        return false;
    }
}

VideoResizerDecoder::VideoResizerDecoder()
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    mpCdxDecoder = NULL;
    //mCedarvReqCtxInitFlag = false;
    resetSomeMembers();
    mState = VRComp_StateLoaded;
}

VideoResizerDecoder::~VideoResizerDecoder()
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    if(mState != VRComp_StateLoaded)
    {
        ALOGE("(f:%s, l:%d) fatal error! deconstruct in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
        reset();
    }
}

status_t VideoResizerDecoder::setVideoFormat(OMX_VIDEO_PORTDEFINITIONTYPE *pVideoFormat, CDX_MEDIA_FILE_FORMAT nFileFormat, int nOutWidth, int nOutHeight)
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    size_t i;
    int nAlignOutWidth;
    int nAlignOutHeight;
    int nAlignSrcWidth;
    int nAlignSrcHeight;
    Mutex::Autolock autoLock(mLock);
    if (mState!=VRComp_StateLoaded) 
    {
        ALOGE("(f:%s, l:%d) call setDataSource in invalid state[0x%x]!", __FUNCTION__, __LINE__, mState);
        return INVALID_OPERATION;
    }
    //init decoder and encoder.
    /*if(false == mCedarvReqCtxInitFlag)
    {
        if(ve_mutex_init(&mCedarvReqCtx, CEDARV_DECODE) < 0)
        {
            ALOGE("(f:%s, l:%d) fatal error! ve_mutex_init fail!", __FUNCTION__, __LINE__);
            return UNKNOWN_ERROR;
        }
        mCedarvReqCtxInitFlag = true;
    }
	if(ve_mutex_lock(&mCedarvReqCtx) < 0)
	{
		ALOGE("(f:%s, l:%d) can not request ve resource, why?", __FUNCTION__, __LINE__);
		return UNKNOWN_ERROR;
	}*/
    int ret;
    mpCdxDecoder = CreateVideoDecoder();
	if (mpCdxDecoder == NULL)
	{
		//TODO: WHAT IT RETURN
		ALOGE("(f:%s, l:%d) libcedarv_init error!", __FUNCTION__, __LINE__);
        goto _err_hw_resize0;
	}
	//mpCdxDecoder->cedarx_cookie = NULL;
	//memset(&mCedarvStreamInfo, 0, sizeof(cedarv_stream_info_t));

	//mCedarvStreamInfo.video_width   = pVideoFormat->nFrameWidth;
	//mCedarvStreamInfo.video_height  = pVideoFormat->nFrameHeight;
	//mCedarvStreamInfo.init_data     = (OMX_U8*) pVideoFormat->pCodecExtraData;
	//mCedarvStreamInfo.init_data_len = pVideoFormat->nCodecExtraDataLen;
	//mCedarvStreamInfo.sub_format    = (cedarv_sub_format_e)pVideoFormat->eCompressionSubFormat;
    //mCedarvStreamInfo.format = convertOMX_VIDEO_CODINGTYPE2cedarv_stream_format_e(pVideoFormat->eCompressionFormat);
    //mCedarvStreamInfo.container_format = convertCDX_MEDIA_FILE_FORMAT2cedarv_container_format_e(nFileFormat);
	memset(&mCedarvStreamInfo, 0, sizeof(VideoStreamInfo));

	mCedarvStreamInfo.nWidth   = pVideoFormat->nFrameWidth;
	mCedarvStreamInfo.nHeight  = pVideoFormat->nFrameHeight;
	mCedarvStreamInfo.pCodecSpecificData     = (char *) pVideoFormat->pCodecExtraData;
	mCedarvStreamInfo.nCodecSpecificDataLen = pVideoFormat->nCodecExtraDataLen;
    ALOGD("eCompressionFormat(%d), eCompressionSubFormat(%d)", pVideoFormat->eCompressionFormat, pVideoFormat->eCompressionSubFormat);
    mCedarvStreamInfo.eCodecFormat    = pVideoFormat->eCompressionSubFormat;
    mCedarvStreamInfo.nFrameRate = pVideoFormat->xFramerate;
    mCedarvStreamInfo.nFrameDuration = pVideoFormat->nMicSecPerFrame;
	//mCedarvStreamInfo.nFrameDuration = 1000000000 / pVideoFormat->xFramerate;
	mCedarvStreamInfo.bIs3DStream = 0;

    mDesOutWidth = nOutWidth;
    mDesOutHeight = nOutHeight;
    mRotation = pVideoFormat->nRotation;
	//mpCdxDecoder->set_vstream_info(mpCdxDecoder, &mCedarvStreamInfo);

    memset(&mDecoderConfig, 0, sizeof(VConfig));

    mDecoderConfig.bThumbnailMode      = 0;
    mDecoderConfig.eOutputPixelFormat  = PIXEL_FORMAT_NV21;
    //* no need to decode two picture when decoding a thumbnail picture.
    mDecoderConfig.bDisable3D          = 1;
    mDecoderConfig.bSupportMaf         = 0;
    mDecoderConfig.bDispErrorFrame     = 0;
    mDecoderConfig.nVbvBufferSize      = 1*1024*1024;
	if(!pVideoFormat->nRotation) {
        mDecoderConfig.bRotationEn     = 0;
    } else {
        mDecoderConfig.bRotationEn     = 1;
        mDecoderConfig.nRotateDegree   = pVideoFormat->nRotation;
    }
 
    //nAlignOutWidth = ALIGN32(nOutWidth);
    //nAlignOutHeight = ALIGN32(nOutHeight);
    //nAlignSrcWidth = ALIGN32(mCedarvStreamInfo.nWidth);
    //nAlignSrcHeight = ALIGN32(mCedarvStreamInfo.nHeight);
    nAlignOutWidth = ALIGN16(nOutWidth);
    nAlignOutHeight = ALIGN16(nOutHeight);
    nAlignSrcWidth = ALIGN16(mCedarvStreamInfo.nWidth);
    nAlignSrcHeight = ALIGN16(mCedarvStreamInfo.nHeight);

    if(nAlignSrcWidth > nAlignOutWidth || nAlignSrcHeight > nAlignOutHeight)
    {
        mbScaleEnableFlag = true;
        mDecoderConfig.bScaleDownEn = 1;
        //mpCdxDecoder->ioctrl(mpCdxDecoder, CEDARV_COMMAND_SET_MAX_OUTPUT_WIDTH, nAlignOutWidth);
    	//mpCdxDecoder->ioctrl(mpCdxDecoder, CEDARV_COMMAND_SET_MAX_OUTPUT_HEIGHT, nAlignOutHeight);
        //for(i=1;nAlignOutWidth<nAlignSrcWidth;i*=2)
        //{
        //    nAlignOutWidth*=2;
        //}

        for(i=0;nAlignOutWidth<nAlignSrcWidth;i++)
        {
            nAlignOutWidth*=2;
        }
        mDecoderConfig.nHorizonScaleDownRatio = i;
        //mVdecStoreWidth = ALIGN32(nAlignSrcWidth/i);
        mVdecStoreWidth = ALIGN16(nAlignSrcWidth/(i*2));

        //for(i=1;nAlignOutHeight<nAlignSrcHeight;i*=2)
        //{
        //    nAlignOutHeight*=2;
        //}
        for(i=0;nAlignOutHeight<nAlignSrcHeight;i++)
        {
            nAlignOutHeight*=2;
        }
        mDecoderConfig.nVerticalScaleDownRatio = i;
        //mVdecStoreHeight = ALIGN32(nAlignSrcHeight/i);
        mVdecStoreHeight = ALIGN16(nAlignSrcHeight/(i*2));
    }
    else
    {
        mbScaleEnableFlag = false;
        //mVdecStoreWidth = ALIGN32(mCedarvStreamInfo.nWidth);
        //mVdecStoreHeight = ALIGN32(mCedarvStreamInfo.nHeight);
        mVdecStoreWidth = ALIGN16(mCedarvStreamInfo.nWidth);
        mVdecStoreHeight = ALIGN16(mCedarvStreamInfo.nHeight);
    }
    ALOGD("(f:%s, l:%d) scale[%d], srcSize[%dx%d], outSize[%dx%d], storeSize[%dx%d]", __FUNCTION__, __LINE__, mbScaleEnableFlag,
            mCedarvStreamInfo.nWidth, mCedarvStreamInfo.nHeight, mDesOutWidth, mDesOutHeight, mVdecStoreWidth, mVdecStoreHeight);
/*
	mpCdxDecoder->ioctrl(mpCdxDecoder, CEDARV_COMMAND_SET_VBV_SIZE, 1*1024*1024);
	mpCdxDecoder->ioctrl(mpCdxDecoder, CEDARV_COMMAND_DISABLE_3D, 1);
	if(pVideoFormat->nRotation)
	{
        ALOGE("(f:%s, l:%d) file has nRotation[%d]", __FUNCTION__, __LINE__, pVideoFormat->nRotation);
		mpCdxDecoder->ioctrl(mpCdxDecoder, CEDARV_COMMAND_ROTATE, (pVideoFormat->nRotation/90) & 3);
	}
*/
	//ret = mpCdxDecoder->open(mpCdxDecoder);
	ret = InitializeVideoDecoder(mpCdxDecoder, &mCedarvStreamInfo, &mDecoderConfig);
	if (ret < 0)
	{
		ALOGE("(f:%s, l:%d) CedarV open error! ret=%d", __FUNCTION__, __LINE__, ret);
        goto _err_hw_resize1;
	}

    //mpCdxDecoder->ioctrl(mpCdxDecoder, CEDARV_COMMAND_PLAY, 0);
    //ve_mutex_unlock(&mCedarvReqCtx);

    //phyMalloc output NV12 frame
    if(NO_ERROR!=createFrameArray())
    {
        goto _err_hw_resize1;
    }
    //queue to idleframelist
    {
        Mutex::Autolock autoLock(mIdleFrameListLock);
        for(i=0;i<mFrameArray.size();i++)
        {
            mIdleFrameList.push_back(mFrameArray[i]);
        }
    }
    mState = VRComp_StateIdle;
    return NO_ERROR;
    
_err_hw_resize1:
    //libcedarv_exit(mpCdxDecoder);
    DestroyVideoDecoder(mpCdxDecoder);
_err_hw_resize0:
    //ve_mutex_unlock(&mCedarvReqCtx);
    mpCdxDecoder = NULL;
    return UNKNOWN_ERROR;
}

/*******************************************************************************
Function name: android.VideoResizerDecoder.start
Description: 
    Idle,Pause->Executing
Parameters: 
    
Return: 
    
Time: 2014/12/4
*******************************************************************************/
status_t VideoResizerDecoder::start()
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    Mutex::Autolock autoLock(mLock);
    if(VRComp_StateExecuting == mState)
    {
        return NO_ERROR;
    }
    if (mState != VRComp_StateIdle && mState != VRComp_StatePause)
    {
        ALOGE("(f:%s, l:%d) called in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
        return INVALID_OPERATION;
    }

    if(VRComp_StateIdle == mState)
    {
        {
            Mutex::Autolock autoLock(mMessageQueueLock);
            if(false == mMessageQueue.empty())
            {
                ALOGE("(f:%s, l:%d) Be careful! mMessageQueue has [%d]message not process in state[%d], clear!", __FUNCTION__, __LINE__, mMessageQueue.size(), mState);
                mMessageQueue.clear();
            }
        }
        mDecodeThread = new DoDecodeThread(this);
        mDecodeThread->startThread();
        mState = VRComp_StateExecuting;
    }
    else if(VRComp_StatePause == mState)
    {
        //send executing msg
        {
            VideoResizerMessage msg;
            msg.what = VR_DECODER_MSG_EXECUTING;
            Mutex::Autolock autoLock(mMessageQueueLock);
            if(mMessageQueue.empty())
            {
                mMessageQueueChanged.signal();
            }
            mMessageQueue.push_back(msg);
        }
        //wait state pause->executing done!
        {
            ALOGD("(f:%s, l:%d) before wait executing complete", __FUNCTION__, __LINE__);
            Mutex::Autolock autoLock(mStateCompleteLock);
            while(0 == mnExecutingDone)
            {
            	mExecutingCompleteCond.wait(mStateCompleteLock);
            }
            mnExecutingDone--;
            ALOGD("(f:%s, l:%d) after wait executing complete", __FUNCTION__, __LINE__);
        }
        if(VRComp_StateExecuting == mState)
        {
            ALOGD("(f:%s, l:%d) Pause->Executing done!", __FUNCTION__, __LINE__);
        }
        else
        {
            ALOGE("(f:%s, l:%d) fatal error! Pause->Executing fail! current state[0x%x]", __FUNCTION__, __LINE__, mState);
        }
    }
    return NO_ERROR;
}
/*******************************************************************************
Function name: android.VideoResizerDecoder.stop
Description: 
    Executing,Pause->Idle
Parameters: 
    
Return: 
    
Time: 2014/12/4
*******************************************************************************/
status_t VideoResizerDecoder::stop_l()
{
    if(VRComp_StateIdle == mState)
    {
        return NO_ERROR;
    }
    if (mState != VRComp_StateExecuting && mState != VRComp_StatePause)
    {
        ALOGE("(f:%s, l:%d) called in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
        return INVALID_OPERATION;
    }
    //stop demuxerThread!
    sp<DoDecodeThread> thread;
    {
        thread = mDecodeThread;
        mDecodeThread.clear();
    }
    if (thread == NULL) 
    {
        ALOGE("(f:%s, l:%d) DoDecodeThread is null?", __FUNCTION__, __LINE__);
    }
    else
    {
        //send stop msg
        {
            VideoResizerMessage msg;
            msg.what = VR_DECODER_MSG_STOP;
            Mutex::Autolock autoLock(mMessageQueueLock);
            if(mMessageQueue.empty())
            {
                mMessageQueueChanged.signal();
            }
            mMessageQueue.push_back(msg);
        }
        thread->stopThread();
    }
    mState = VRComp_StateIdle;
    return NO_ERROR;
}
status_t VideoResizerDecoder::stop()
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    Mutex::Autolock locker(mLock);
    return stop_l();
}

status_t VideoResizerDecoder::pause()
{
    Mutex::Autolock locker(&mLock);
    if(VRComp_StatePause == mState)
    {
        return NO_ERROR;
    }
    if (mState != VRComp_StateExecuting)
    {
        ALOGE("(f:%s, l:%d) called in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
        return INVALID_OPERATION;
    }
    //send pause msg
    {
        VideoResizerMessage msg;
        msg.what = VR_DECODER_MSG_PAUSE;
        Mutex::Autolock autoLock(mMessageQueueLock);
        if(mMessageQueue.empty())
        {
            mMessageQueueChanged.signal();
        }
        mMessageQueue.push_back(msg);
    }
    //wait state executing->pause done!
    {
        ALOGD("(f:%s, l:%d) before wait pause complete", __FUNCTION__, __LINE__);
        Mutex::Autolock autoLock(mStateCompleteLock);
        while(0 == mnPauseDone)
        {
        	mPauseCompleteCond.wait(mStateCompleteLock);
        }
        mnPauseDone--;
        ALOGD("(f:%s, l:%d) after wait pause complete", __FUNCTION__, __LINE__);
    }
    if(VRComp_StatePause == mState)
    {
        ALOGD("(f:%s, l:%d) Executing->Pause done!", __FUNCTION__, __LINE__);
    }
    else
    {
        ALOGE("(f:%s, l:%d) fatal error! Executing->Pause fail! current state[0x%x]", __FUNCTION__, __LINE__, mState);
    }
    return NO_ERROR;
}
/*******************************************************************************
Function name: android.VideoResizerDecoder.reset
Description: 
    Invalid,Idle,Executing,Pause->Loaded.
Parameters: 
    
Return: 
    
Time: 2014/12/4
*******************************************************************************/
status_t VideoResizerDecoder::reset()
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    status_t ret;
    Mutex::Autolock locker(&mLock);
    if(VRComp_StateLoaded == mState)
    {
        return NO_ERROR;
    }
    if (VRComp_StateExecuting == mState || VRComp_StatePause == mState)
    {
        ALOGD("(f:%s, l:%d) state[0x%x]->Idle", __FUNCTION__, __LINE__, mState);
        ret = stop_l();
        if(ret != NO_ERROR)
        {
            ALOGE("(f:%s, l:%d) fatal error! state[0x%x]->Idle fail!", __FUNCTION__, __LINE__, mState);
        }
    }
    if(VRComp_StateIdle != mState)
    {
        ALOGE("(f:%s, l:%d) fatal error! not support state[0x%x]", __FUNCTION__, __LINE__, mState);
    }
    if(VRComp_StateIdle == mState || VRComp_StateInvalid == mState)
    {
        {
            Mutex::Autolock autoLock(mIdleFrameListLock);
            if(mIdleFrameList.size()!=mFrameArray.size())
            {
                ALOGE("(f:%s, l:%d) fatal error! IdleFrameList is not full[%d]!=[%d]", __FUNCTION__, __LINE__, mIdleFrameList.size(), mFrameArray.size());
            }
            mIdleFrameList.clear();
        }
        destroyFrameArray();
        /*if(mpCdxDecoder)
        {
            if(ve_mutex_lock(&mCedarvReqCtx) < 0)
            {
                ALOGE("(f:%s, l:%d) can not request ve resource, why?", __FUNCTION__, __LINE__);
                //return UNKNOWN_ERROR;
            }
            libcedarv_exit(mpCdxDecoder);
            mpCdxDecoder = NULL;
            ve_mutex_unlock(&mCedarvReqCtx);
        }
        if(mCedarvReqCtxInitFlag)
        {
            ve_mutex_destroy(&mCedarvReqCtx);
            mCedarvReqCtxInitFlag = false;
        }*/
        DestroyVideoDecoder(mpCdxDecoder);
        mpCdxDecoder = NULL;
        resetSomeMembers();
        mState = VRComp_StateLoaded;
    }
    
    return NO_ERROR;
}
status_t VideoResizerDecoder::seekTo(int msec)
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    Mutex::Autolock autoLock(mLock);
    if(mState != VRComp_StateIdle && mState != VRComp_StatePause)
    {
        ALOGE("(f:%s, l:%d) called in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
        return INVALID_OPERATION;
    }
    if(mState == VRComp_StateIdle)
    {
        mEofFlag = false;
        mDecodeEndFlag = false;
        mDecodeEofFlag = false;
        return NO_ERROR;
    }
    else
    {
        //send to demuxThread.
        {
            VideoResizerMessage msg;
            msg.what = VR_DECODER_MSG_SEEK;
            msg.arg1 = msec;
            Mutex::Autolock autoLock(mMessageQueueLock);
            if(mMessageQueue.empty())
            {
                mMessageQueueChanged.signal();
            }
            mMessageQueue.push_back(msg);
        }
        //wait_seek_done!
        {
            ALOGD("(f:%s, l:%d) before wait seek complete", __FUNCTION__, __LINE__);
            Mutex::Autolock autoLock(mStateCompleteLock);
            while(0 == mnSeekDone)
            {
            	mSeekCompleteCond.wait(mStateCompleteLock);
            }
            mnSeekDone--;
            ALOGD("(f:%s, l:%d) after wait seek complete", __FUNCTION__, __LINE__);
        }
        return NO_ERROR;
    }
}
status_t VideoResizerDecoder::setEncoderComponent(VideoResizerEncoder *pEncoderComp)
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    Mutex::Autolock lock(mLock);
    if (mState != VRComp_StateIdle)
    {
        ALOGE("(f:%s, l:%d) called in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
        return INVALID_OPERATION;
    }
    mpEncoder = pEncoderComp;
    return NO_ERROR;
}
status_t VideoResizerDecoder::requstBuffer(OMX_BUFFERHEADERTYPE* pBuffer)
{
    int vdecRequestWriteRet;
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    Mutex::Autolock lock(mLock);
    if (mState != VRComp_StateExecuting && mState != VRComp_StatePause)
    {
        ALOGE("(f:%s, l:%d) called in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
        return INVALID_OPERATION;
    }
/*    vdecRequestWriteRet = mpCdxDecoder->request_write(mpCdxDecoder,
                                                pBuffer->nTobeFillLen,
                                                &pBuffer->pBuffer,
                                                &pBuffer->nBufferLen,
                                                &pBuffer->pBufferExtra,
                                                &pBuffer->nBufferExtraLen);
*/
    int nStreamBufIndex;
    if(CDX_VIDEO_STREAM_MINOR == pBuffer->video_stream_type)
    {
        nStreamBufIndex = 1;
    }
    else
    {
        nStreamBufIndex = 0;
    }
    vdecRequestWriteRet = RequestVideoStreamBuffer(mpCdxDecoder, 
                                   pBuffer->nTobeFillLen, 
                                   (char**)&pBuffer->pBuffer, 
                                   (int *)&pBuffer->nBufferLen,
                                   (char**)&pBuffer->pBufferExtra,
                                   (int *)&pBuffer->nBufferExtraLen,
                                   nStreamBufIndex);
	if (vdecRequestWriteRet < 0)
	{
		ALOGV("request buffer fail, vbv buffer may full!");
        return NO_MEMORY;
	}
    else
    {
        return NO_ERROR;
    }
    
}
status_t VideoResizerDecoder::updateBuffer(OMX_BUFFERHEADERTYPE* pBuffer)
{
    int ret;
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    Mutex::Autolock lock(mLock);
    if (mState != VRComp_StateExecuting && mState != VRComp_StatePause)
    {
        ALOGE("(f:%s, l:%d) called in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
        return INVALID_OPERATION;
    }
    //cedarv_stream_data_info_t stream_info;
	//stream_info.type  = pBuffer->video_stream_type;
	//stream_info.lengh = pBuffer->nFilledLen;
	//stream_info.flags = pBuffer->nCtrlBits;
	//if (pBuffer->nTimeStamp != -1)
	//{
	//	stream_info.flags |= CEDARV_FLAG_PTS_VALID;
	//	stream_info.pts = pBuffer->nTimeStamp;
	//}
	//mpCdxDecoder->update_data(mpCdxDecoder, &stream_info);
    VideoStreamDataInfo dataInfo;
    memset(&dataInfo, 0, sizeof(VideoStreamDataInfo));
    dataInfo.pData        = (char*)pBuffer->pBuffer;
    dataInfo.nLength      = pBuffer->nFilledLen;
    if(pBuffer->nCtrlBits & CEDARV_FLAG_PTS_VALID)
    {
        dataInfo.nPts         = pBuffer->nTimeStamp;
    }
    else
    {
        dataInfo.nPts         = -1;
    }
    dataInfo.nPcr         = -1;
    if(pBuffer->nCtrlBits & CEDARV_FLAG_FIRST_PART)
    {
        dataInfo.bIsFirstPart = 1;
    }
    if(pBuffer->nCtrlBits & CEDARV_FLAG_LAST_PART)
    {
        dataInfo.bIsLastPart = 1;
        ALOGV("(f:%s, l:%d) last part found!", __FUNCTION__, __LINE__);
    }
    if(pBuffer->video_stream_type == CDX_VIDEO_STREAM_MINOR)
    {
        dataInfo.nStreamIndex = 1;
    }
    else
    {
        dataInfo.nStreamIndex = 0;
    }
    ALOGV("(f:%s, l:%d) update buffer size:%d", __FUNCTION__, __LINE__, dataInfo.nLength);
    SubmitVideoStreamData(mpCdxDecoder, &dataInfo, dataInfo.nStreamIndex);

    {
        Mutex::Autolock autoLock(mVbsLock);
        if(0 == mVbsSemValue)
        {
            ALOGV("(f:%s, l:%d) signal vbs available!", __FUNCTION__, __LINE__);
            mVbsSemValue++;
            VideoResizerMessage msg;
            msg.what = VR_DECODER_MSG_VBS_AVAILABLE;
            Mutex::Autolock autoLock(mMessageQueueLock);
            if(mMessageQueue.empty())
            {
                mMessageQueueChanged.signal();
            }
            mMessageQueue.push_back(msg);
        }
	}
    return NO_ERROR;
}
/*******************************************************************************
Function name: android.VideoResizerDecoder.FillThisBuffer
Description: 
    return outFrame through this function.
    can call in state Idle, Executing, Pause.
Parameters: 
    
Return: 
    
Time: 2014/12/5
*******************************************************************************/
status_t VideoResizerDecoder::FillThisBuffer(OMX_BUFFERHEADERTYPE* pBuffer)
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    Mutex::Autolock autoLock(mLock);
    if (mState != VRComp_StateIdle && mState != VRComp_StateExecuting && mState != VRComp_StatePause)
    {
        ALOGE("(f:%s, l:%d) called in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
        return INVALID_OPERATION;
    }
    sp<VdecOutFrame> outFrame = *(sp<VdecOutFrame>*)pBuffer->pBuffer;
    
    if(ReturnPicture(mpCdxDecoder, outFrame->mPicture) != 0) {
        ALOGE("(f:%s, l:%d) fatal error! ReturnPicture() fail", __FUNCTION__, __LINE__);
    }
    //queue buffer to IdleFrameList
    {
        Mutex::Autolock autoLock(mIdleFrameListLock);
        if(outFrame->mStatus!=VdecOutFrame::OwnedByDownStream)
        {
            ALOGE("(f:%s, l:%d) fatal error! outFrameStatus[%d]!=OwnedByDownStream, check code!", __FUNCTION__, __LINE__, outFrame->mStatus);
        }
        outFrame->mStatus = VdecOutFrame::OwnedByUs;
        mIdleFrameList.push_back(outFrame);
        if(mOutFrmUnderFlow)
        {
            ALOGV("(f:%s, l:%d) signal outFrame available!", __FUNCTION__, __LINE__);
            VideoResizerMessage msg;
            msg.what = VR_DECODER_MSG_OUTFRAME_AVAILABLE;
            Mutex::Autolock autoLock(mMessageQueueLock);
            if(mMessageQueue.empty())
            {
                mMessageQueueChanged.signal();
            }
            mMessageQueue.push_back(msg);
        }
    }
    return NO_ERROR;
}
status_t VideoResizerDecoder::SetConfig(VideoResizerIndexType nIndexType, void *pParam)
{
    switch(nIndexType)
    {
        case VR_IndexConfigSetStreamEof:
        {
            ALOGD("(f:%s, l:%d) notify Decoder eof in state[0x%x]!", __FUNCTION__, __LINE__, mState);
            mEofFlag = true;
            break;
        }
        default:
        {
            ALOGE("(f:%s, l:%d) unknown indexType[0x%x]", __FUNCTION__, __LINE__, nIndexType);
            return INVALID_OPERATION;
        }
    }
    return NO_ERROR;
}
bool VideoResizerDecoder::decodeThread()
{
    bool bHasMessage;
    VideoResizerMessage msg;
    int decodeRet;
    int nIdleFrameNum = 0;
    sp<VdecOutFrame> transportFrame;
    bool bVdecNoFrameBuffer = false;
    mDebugFrameCnt = 0;
    while(1)
    {
        _process_message:
        {
            Mutex::Autolock autoLock(mMessageQueueLock);
            if(false == mMessageQueue.empty())
            {
                bHasMessage = true;
                msg = *mMessageQueue.begin();
                mMessageQueue.erase(mMessageQueue.begin());
            }
            else
            {
                bHasMessage = false;
            }
        }
        if(bHasMessage)
        {
            switch(msg.what)
            {
                case VR_DECODER_MSG_EXECUTING:
                {
                    if(mState!=VRComp_StatePause)
                    {
                        ALOGE("(f:%s, l:%d) fatal error! call executing in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
                        break;
                    }
                    //change state to Executing.
                    mState = VRComp_StateExecuting;
                    //notify caller.
                    {
                        Mutex::Autolock autoLock(mStateCompleteLock);
                        if(0 == mnExecutingDone)
                        {
                            mnExecutingDone++;
                        }
                        else
                        {
                            ALOGE("(f:%s, l:%d) fatal error! what happened? mnExecutingDone[0x%x]", __FUNCTION__, __LINE__, mnExecutingDone);
                        }
                        mExecutingCompleteCond.signal();
                    }
                    break;
                }
                case VR_DECODER_MSG_PAUSE:
                {
                    if(mState!=VRComp_StateExecuting)
                    {
                        ALOGE("(f:%s, l:%d) fatal error! call pause in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
                        break;
                    }
                    //change state to Pause.
                    mState = VRComp_StatePause;
                    //notify caller.
                    {
                        Mutex::Autolock autoLock(mStateCompleteLock);
                        if(0 == mnPauseDone)
                        {
                            mnPauseDone++;
                        }
                        else
                        {
                            ALOGE("(f:%s, l:%d) fatal error! what happened? mnPauseDone[0x%x]", __FUNCTION__, __LINE__, mnPauseDone);
                        }
                        mPauseCompleteCond.signal();
                    }
                    break;
                }
                case VR_DECODER_MSG_SEEK:
                {
                    if(mState!=VRComp_StatePause)
                    {
                        ALOGE("(f:%s, l:%d) fatal error! call seek in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
                    }
                    //reset vdeclib.
                    //ve_mutex_lock(&mCedarvReqCtx);
                    //mpCdxDecoder->ioctrl(mpCdxDecoder, CEDARV_COMMAND_JUMP, 0);
                    //ve_mutex_unlock(&mCedarvReqCtx);
            		ResetVideoDecoder(mpCdxDecoder);

                    bVdecNoFrameBuffer = false;
                    mEofFlag = false;
                    mDecodeEndFlag = false;
                    mDecodeEofFlag = false;
                    //notify caller.
                    {
                        Mutex::Autolock autoLock(mStateCompleteLock);
                        if(0 == mnSeekDone)
                        {
                            mnSeekDone++;
                        }
                        else
                        {
                            ALOGE("(f:%s, l:%d) fatal error! what happened? mnSeekDone[0x%x]", __FUNCTION__, __LINE__, mnSeekDone);
                        }
                        mSeekCompleteCond.signal();
                    }
                    break;
                }
                case VR_DECODER_MSG_VBS_AVAILABLE:
                {
                    ALOGV("(f:%s, l:%d) state[0x%x], receive vbs_available", __FUNCTION__, __LINE__, mState);
                    break;
                }
                case VR_DECODER_MSG_OUTFRAME_AVAILABLE:
                {
                    ALOGV("(f:%s, l:%d) state[0x%x], receive outframe_available", __FUNCTION__, __LINE__, mState);
                    Mutex::Autolock autoLock(mIdleFrameListLock);
                    if(mOutFrmUnderFlow!=true)
                    {
                        ALOGD("(f:%s, l:%d) state[0x%x], outFrmUnderFlow[%d]!=true", __FUNCTION__, __LINE__, mState, mOutFrmUnderFlow);
                    }
                    mOutFrmUnderFlow = false;
                    break;
                }
                case VR_DECODER_MSG_STOP:
                {
                    ALOGD("(f:%s, l:%d) decodeThread() will exit!", __FUNCTION__, __LINE__);
                    if(mState!=VRComp_StatePause && mState!=VRComp_StateExecuting)
                    {
                        ALOGE("(f:%s, l:%d) fatal error! decodeThread is in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
                    }
                    {
                        Mutex::Autolock autoLock(mMessageQueueLock);
                        if(false == mMessageQueue.empty())
                        {
                            ALOGE("(f:%s, l:%d) Be careful! mMessageQueue has [%d]messages not process when exit demuxThread, ignore and clear them.", __FUNCTION__, __LINE__, mMessageQueue.size());
                            mMessageQueue.clear();
                        }
                    }
                    return false;
                }
                default:
                {
                    ALOGE("(f:%s, l:%d) unknown msg[%d]!", __FUNCTION__, __LINE__, msg.what);
                    break;
                }
            }
        }
        if(true==mDecodeEofFlag || VRComp_StatePause==mState)
        {
            Mutex::Autolock autoLock(mMessageQueueLock);
            while(mMessageQueue.empty())
            {
                mMessageQueueChanged.wait(mMessageQueueLock);
            }
            goto _process_message;
            
        }
        if(VRComp_StateExecuting != mState)
        {
            ALOGE("(f:%s, l:%d) fatal error! wrong state[0x%x]!", __FUNCTION__, __LINE__, mState);
            mState = VRComp_StateInvalid;
            return false;
        }
        if(bVdecNoFrameBuffer)
        {
            ALOGV("(f:%s, l:%d) VdecFrameUnderflow, directly goto _convert_frame", __FUNCTION__, __LINE__);
            goto _convert_frame;
        }
        _vdec_decode:
        if(mDecodeEndFlag)
        {
            ALOGV("(f:%s, l:%d) decode end, directly goto _convert_frame", __FUNCTION__, __LINE__);
            goto _convert_frame;
        }
        //ve_mutex_lock(&mCedarvReqCtx);
		//decodeRet = mpCdxDecoder->decode(mpCdxDecoder);
        //ve_mutex_unlock(&mCedarvReqCtx);
        decodeRet = DecodeVideoStream(mpCdxDecoder, 0/*eos*/, 0/*key frame only*/, 0/*drop b frame*/, 0/*current time*/);
        //if(decodeRet == CEDARV_RESULT_KEYFRAME_DECODED || decodeRet == CEDARV_RESULT_FRAME_DECODED)
    	if(decodeRet == VDECODE_RESULT_KEYFRAME_DECODED || decodeRet == VDECODE_RESULT_FRAME_DECODED)
        {
            mDebugFrameCnt++;
            ALOGV("(f:%s, l:%d) decode ret[%d], frameCnt[%d]!", __FUNCTION__, __LINE__, decodeRet, mDebugFrameCnt);
//            mDecodeTotalDuration += (tm2-tm1);
//            mLockDecodeTotalDuration += (tm4-tm3);
//            if(mDebugFrameCnt%30 == 0)
//            {
//                ALOGD("(f:%s, l:%d) avgDecode[%lld]ms, [%lld]ms", __FUNCTION__, __LINE__, mDecodeTotalDuration/(mDebugFrameCnt*1000), mLockDecodeTotalDuration/(mDebugFrameCnt*1000));
//                mDebugFrameCnt = 0;
//                mDecodeTotalDuration = 0;
//                mLockDecodeTotalDuration = 0;
//            }
		}
        else if(decodeRet == VDECODE_RESULT_OK)
        {
            ALOGV("(f:%s, l:%d) decode ret[%d]", __FUNCTION__, __LINE__, decodeRet);
            goto _process_message;
        }
        else if(decodeRet == VDECODE_RESULT_NO_FRAME_BUFFER)
        {
            ALOGV("(f:%s, l:%d) no frame buffer, impossible, because release immediately", __FUNCTION__, __LINE__);
            bVdecNoFrameBuffer = true;
//            Mutex::Autolock autoLock(mMessageQueueLock);
//            if(mMessageQueue.empty())
//            {
//                mMessageQueueChanged.wait(mMessageQueueLock);
//            }
//            goto _process_message;
            
        }
        else if(decodeRet == VDECODE_RESULT_NO_BITSTREAM)
        {
            ALOGV("(f:%s, l:%d) no vbs, wait", __FUNCTION__, __LINE__);
            if(true == mEofFlag)
            {
                ALOGD("(f:%s, l:%d) Decoder set decodeEndFlag", __FUNCTION__, __LINE__);
                if(mDecodeEndFlag!=false)
                {
                    ALOGE("(f:%s, l:%d) fatal error! demuxer eof, decodeEndFlag should be false, check code!", __FUNCTION__, __LINE__);
                }
                mDecodeEndFlag = true;
                goto _convert_frame;
            }
            Mutex::Autolock autoLock(mMessageQueueLock);
            while(mMessageQueue.empty())
            {
                {
                    Mutex::Autolock autoLock(mVbsLock);
                    if(mVbsSemValue>0)
                    {
                        mVbsSemValue--;
                        goto _process_message;
                    }
                }
                mMessageQueueChanged.wait(mMessageQueueLock);
            }
            goto _process_message;
        }
        //else if(decodeRet == CEDARV_RESULT_ERR_INVALID_PARAM || decodeRet == CEDARV_RESULT_ERR_NO_MEMORY || decodeRet == CEDARV_RESULT_ERR_UNSUPPORTED)
        else if(decodeRet <= VDECODE_RESULT_UNSUPPORTED)
        {
            ALOGE("(f:%s, l:%d) fatal error, decoder return impossible[%d]!", __FUNCTION__, __LINE__, decodeRet);
            goto _process_message;
        }
        else
        {
            ALOGE("(f:%s, l:%d) fatal error, decoder return unexcepted result[%d], check code!", __FUNCTION__, __LINE__, decodeRet);
            goto _process_message;
        }
        
        _convert_frame:
        //display_request(), convert to NV12, display_release().
        {
            Mutex::Autolock autoLock(mIdleFrameListLock);
            nIdleFrameNum = mIdleFrameList.size();
            if(nIdleFrameNum>0)
            {
                if(mOutFrmUnderFlow!=false)
                {
                    ALOGD("(f:%s, l:%d) state[0x%x], outFrmUnderFlow[%d] not false, set it to false.", __FUNCTION__, __LINE__, mState, mOutFrmUnderFlow);
                    mOutFrmUnderFlow = false;
                }

                mCedarvPic = RequestPicture(mpCdxDecoder, 0/*the major stream*/);
                if(mCedarvPic != NULL)
                {
                    bVdecNoFrameBuffer = false;
                    //dequeue one idle frame
                    transportFrame = *mIdleFrameList.begin();
                    mIdleFrameList.erase(mIdleFrameList.begin());
                    if(transportFrame->mStatus!=VdecOutFrame::OwnedByUs)
                    {
                        ALOGE("(f:%s, l:%d) fatal error! transportFrameStatus[%d]!=OwnedByUs, check code!", __FUNCTION__, __LINE__, transportFrame->mStatus);
                    }
                    transportFrame->mStatus = VdecOutFrame::OwnedByVdec;
                }
                else
                {
                    if(mDecodeEndFlag)
                    {
                        ALOGD("(f:%s, l:%d) Decoder set eof flag", __FUNCTION__, __LINE__);
                        if(mDecodeEofFlag!=false)
                        {
                            ALOGE("(f:%s, l:%d) fatal error! decode end, but decodeEofFlag should be false, check code!", __FUNCTION__, __LINE__);
                        }
                        mDecodeEofFlag = true;
                        ALOGD("(f:%s, l:%d) state[0x%x], notify next component eof!", __FUNCTION__, __LINE__, mState);
                        if(NO_ERROR != mpEncoder->SetConfig(VR_IndexConfigSetStreamEof, NULL))
                        {
                            ALOGE("(f:%s, l:%d) fatal error! notify next component eof fail!", __FUNCTION__, __LINE__);
                        }
                    }
                    else
                    {
                        ALOGD("(f:%s, l:%d) vdec should has ready frame, but display_request fail, continue decode.", __FUNCTION__, __LINE__);
                    }
                    if(bVdecNoFrameBuffer)
                    {
                        ALOGE("(f:%s, l:%d) fatal error! why display_request fail when vdeclib has all ready frame? check code!", __FUNCTION__, __LINE__);
                    }
                    transportFrame = NULL;
                    goto _process_message;
                }
            }
            else
            {
                transportFrame = NULL;
                if(bVdecNoFrameBuffer)
                {
                    if(mOutFrmUnderFlow!=false)
                    {
                        ALOGD("(f:%s, l:%d) state[0x%x], outFrmUnderFlow[%d] already true", __FUNCTION__, __LINE__, mState, mOutFrmUnderFlow);
                    }
                    mOutFrmUnderFlow = true;
                }
                else
                {
                    goto _process_message;
                }
            }
        }
        if(mOutFrmUnderFlow)
        {
            Mutex::Autolock autoLock(mMessageQueueLock);
            while(mMessageQueue.empty())
            {
                ALOGV("(f:%s, l:%d) vdecNoFrameBuf, wait", __FUNCTION__, __LINE__);
                mMessageQueueChanged.wait(mMessageQueueLock);
            }
            goto _process_message;
        }
        if(transportFrame!=NULL)  //dequeue idle frame success.
        {
            //set VdecOutFrame.
            transportFrame->setFrameInfo(mCedarvPic);
/*
            //convert frame from MB32 to NV12.
            //convertMB32ToNV12(&mCedarvPic, transportFrame.get());
            //transportFrame->copyFrame(mCedarvPic);
            //mpCdxDecoder->display_release(mpCdxDecoder, mCedarvPic.id);
            int ret;
            if((ret = ReturnPicture(mpCdxDecoder, (VideoPicture*)mCedarvPic)) != 0) {
                ALOGE("(f:%s, l:%d) fatal error! ReturnPicture() fail ret[%d]", __FUNCTION__, __LINE__, ret);
            }
*/
            //call EmptyThisBuffer() to send to next component
            transportFrame->mStatus = VdecOutFrame::OwnedByDownStream;
            OMX_BUFFERHEADERTYPE    obhFrame;
            obhFrame.pBuffer = (OMX_U8*)&transportFrame;
            if(NO_ERROR != mpEncoder->EmptyThisBuffer(&obhFrame))
            {
                ALOGE("(f:%s, l:%d) fatal error! EmptyThisBuffer() fail, framework design can avoid this happen, why it happen? check code!", __FUNCTION__, __LINE__);
                mIdleFrameList.push_back(transportFrame);
            }
            transportFrame = NULL;
        }
        else
        {
            ALOGE("(f:%s, l:%d) fatal error! impossible here, check code!", __FUNCTION__, __LINE__);
            goto _process_message;
        }
    }
    ALOGE("(f:%s, l:%d) fatal error! impossible come here! exit demuxThread()", __FUNCTION__, __LINE__);
    return false;
}
/*
status_t VideoResizerDecoder::releaseFrame(sp<VdecOutFrame>& outFrame)
{
    int ret;
    if((ret = ReturnPicture(mpCdxDecoder, outFrame->mPicture)) != 0) {
        ALOGE("(f:%s, l:%d) fatal error! ReturnPicture() fail ret[%d]", __FUNCTION__, __LINE__, ret);
    }
    return NO_ERROR;
}
*/
status_t VideoResizerDecoder::createFrameArray()
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    int i;
    sp<VdecOutFrame> pFrame;
    for(i=0;i<FRAME_ARRAY_SIZE;i++)
    {
        pFrame = new VdecOutFrame(mVdecStoreWidth, mVdecStoreHeight, PIXEL_FORMAT_NV21);
        //if(NULL == pFrame->mBuf)
        if(pFrame == NULL)
        {
            ALOGE("(f:%s, l:%d) new VdecOutFrame fail", __FUNCTION__, __LINE__);
            goto _err0;
        }
        mFrameArray.push_back(pFrame);
    }
    return NO_ERROR;
_err0:
    mFrameArray.clear();
    return NO_MEMORY;
}

status_t VideoResizerDecoder::destroyFrameArray()
{
    //because use sp<>, so clear mFrameArray don't means frame is free. 
    //But we should known memory leak is impossible happen.
    mFrameArray.clear();
    return NO_ERROR;
}
void VideoResizerDecoder::resetSomeMembers()
{
    mEofFlag = false;
    mDecodeEndFlag = false;
    mDecodeEofFlag = false;
    mVbsSemValue = 0;
    mOutFrmUnderFlow = false;
    mbScaleEnableFlag = false;
    mDesOutWidth = 0;
    mDesOutHeight = 0;
    mVdecStoreWidth = 0;
    mVdecStoreHeight = 0;
    mRotation = 0;
    mpEncoder = NULL;
    mnExecutingDone = 0;
    mnPauseDone = 0;
    mnSeekDone = 0;
    mDebugFrameCnt = 0;
    mDecodeTotalDuration = 0;
    mLockDecodeTotalDuration = 0;
    {
        Mutex::Autolock autoLock(mMessageQueueLock);
        if(false == mMessageQueue.empty())
        {
            ALOGD("(f:%s, l:%d) mMessageQueueLock has [%d] messages not process!", __FUNCTION__, __LINE__, mMessageQueue.size());
            mMessageQueue.clear();
        }
    }
}

}; /* namespace android */

