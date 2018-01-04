//#define LOG_NDEBUG 0
#define LOG_TAG "VideoResizerEncoder"

//#include <cutils/log.h>
#include <utils/Log.h>

#include "VideoResizerEncoder.h"
#include "VideoResizerMuxer.h"
#include <memoryAdapter.h>

namespace android
{
/*
__pixel_yuvfmt_t convertPixelFormat_Vdec2VEnc(cedarv_pixel_format_e nVdecPixelFormat)
{
    switch(nVdecPixelFormat)
    {
        case CEDARV_PIXEL_FORMAT_PLANNER_YUV420:
        {
            return PIXEL_YUV420;
        }
        case CEDARV_PIXEL_FORMAT_PLANNER_YVU420:
        {
            return PIXEL_YVU420;
        }
        default:
        {
            ALOGE("(f:%s, l:%d) fatal error! unknown vdec pixel format[0x%x], default return PIXEL_YVU420", __FUNCTION__, __LINE__, nVdecPixelFormat);
            return PIXEL_YVU420;
        }
    }
}
*/
VencOutVbs::VencOutVbs()
{
    mStatus = OwnedByUs;
    mBuf0 = NULL;
    mBufSize0 = 0;
    mBuf1 = NULL;
    mBufSize1 = 0;
    mPts = -1;
    mIndex = -1;
}
VencOutVbs::~VencOutVbs()
{
    if(mBuf0!=NULL)
    {
        ALOGE("(f:%s, l:%d) fatal error! must release buf to venclib! [%p][%d][%d]", __FUNCTION__, __LINE__, mBuf0, mBufSize0, mIndex);
    }
}
void VencOutVbs::setVbsInfo(void *pVbs0, int nVbsSize0, void *pVbs1, int nVbsSize1, int64_t nPts, int nVbsIndex)
{
    mBuf0 = pVbs0;
    mBufSize0 = nVbsSize0;
    mBuf1 = pVbs1;
    mBufSize1 = nVbsSize1;
    mPts = nPts;
    mIndex = nVbsIndex;
}
void VencOutVbs::clearVbsInfo()
{
    mBuf0 = NULL;
    mBufSize0 = 0;
    mBuf1 = NULL;
    mBufSize1 = 0;
    mPts = -1;
    mIndex = -1;
}

status_t VencOutVbs::getVbsBufInfo(BufInfo *pBufInfo) const
{
    pBufInfo->mpData0 = mBuf0;
    pBufInfo->mDataSize0 = mBufSize0;
    pBufInfo->mpData1 = mBuf1;
    pBufInfo->mDataSize1 = mBufSize1;
    pBufInfo->mDataPts = mPts;
    return NO_ERROR;
}

int VencOutVbs::getVbsIndex() const
{
    return mIndex;
}

VideoResizerEncoder::DoEncodeThread::DoEncodeThread(VideoResizerEncoder *pOwner)
    : Thread(false),
      mpOwner(pOwner),
      mThreadId(NULL)
{
}
void VideoResizerEncoder::DoEncodeThread::startThread()
{
    run("ResizeEncodeThread", PRIORITY_DEFAULT);    //PRIORITY_DISPLAY
}
void VideoResizerEncoder::DoEncodeThread::stopThread()
{
    requestExitAndWait();
}
status_t VideoResizerEncoder::DoEncodeThread::readyToRun() 
{
    mThreadId = androidGetThreadId();

    return Thread::readyToRun();
}
bool VideoResizerEncoder::DoEncodeThread::threadLoop()
{
    if(!exitPending())
    {
        return mpOwner->encodeThread();
    }
    else
    {
        return false;
    }
}
VideoResizerEncoder::VideoResizerEncoder()
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    mpCdxEncoder = NULL;
    mEncoderOpenedFlag = false;
    //mCedarvReqCtxInitFlag = false;
    resetSomeMembers();
    mState = VRComp_StateLoaded;
}

VideoResizerEncoder::~VideoResizerEncoder()
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    if(mState != VRComp_StateLoaded)
    {
        ALOGE("(f:%s, l:%d) fatal error! deconstruct in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
        reset();
    }
}
/*******************************************************************************
Function name: android.VideoResizerEncoder.setEncodeType
Description: 
    // Loaded->Idle
Parameters: 
    
Return: 
    
Time: 2014/12/7
*******************************************************************************/
status_t VideoResizerEncoder::setEncodeType(OMX_VIDEO_CODINGTYPE nType, int nFrameRate, int nBitRate, int nOutWidth, int nOutHeight, int nSrcFrameRate)
{
    ALOGV("(f:%s, l:%d) type=%d, reqFrameRate=%d, bitRate=%d, width=%d, height=%d, srcFrameRate=%d",
        __FUNCTION__, __LINE__, nType, nFrameRate, nBitRate, nOutWidth, nOutHeight, nSrcFrameRate);
    size_t i;
    Mutex::Autolock autoLock(mLock);
    if (mState!=VRComp_StateLoaded) 
    {
        ALOGE("(f:%s, l:%d) call in invalid state[0x%x]!", __FUNCTION__, __LINE__, mState);
        return INVALID_OPERATION;
    }
    //init decoder and encoder.
    mEncodeType = nType;
    mFrameRate = nFrameRate;
    mBitRate = nBitRate;
    mDesOutWidth = nOutWidth;
    mDesOutHeight = nOutHeight;
    mSrcFrameRate = nSrcFrameRate;
    if(mEncodeType!=OMX_VIDEO_CodingAVC)
    {
        ALOGE("(f:%s, l:%d) sorry, unsupport encode format[0x%x]", __FUNCTION__, __LINE__, mEncodeType);
        return INVALID_OPERATION;
    }
/*
    if(false == mCedarvReqCtxInitFlag)
    {
        if(ve_mutex_init(&mCedarvReqCtx, CEDARV_ENCODE) < 0)
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
	}
*/
    int ret;
    //* create a h264 encoder.
	//mpCdxEncoder = H264EncInit(&ret);
    mpCdxEncoder = VideoEncCreate(VENC_CODEC_H264);
	if(mpCdxEncoder == NULL)
	{
        goto _err_hw_resize0;
	}
    //ve_mutex_unlock(&mCedarvReqCtx);

    //phyMalloc output NV12 frame
    if(NO_ERROR!=createVbsArray())
    {
        goto _err_hw_resize1;
    }
    //queue to mOutIdleVbsList
    {
        Mutex::Autolock autoLock(mOutIdleVbsListLock);
        for(i=0;i<mOutVbsArray.size();i++)
        {
            mOutVbsArray[i]->mStatus = VencOutVbs::OwnedByUs;
            mOutIdleVbsList.push_back(mOutVbsArray[i]);
        }
    }
    mState = VRComp_StateIdle;

	if (setFrameRateControl(mFrameRate/1000, mSrcFrameRate/1000, &mFrameRateCtrl.capture, &mFrameRateCtrl.total) >= 0) {
        ALOGD("<F;%s, L:%d>Low framerate mode, capture %d in %d", __FUNCTION__, __LINE__, mFrameRateCtrl.capture, mFrameRateCtrl.total);
		mFrameRateCtrl.count = 0;
		mFrameRateCtrl.enable = true;
	} else {
		mFrameRateCtrl.enable = false;
	}
    return NO_ERROR;

_err_hw_resize1:
    //if(ve_mutex_lock(&mCedarvReqCtx) < 0)
	//{
	//	ALOGE("(f:%s, l:%d) can not request ve resource, why?", __FUNCTION__, __LINE__);
	//	return UNKNOWN_ERROR;
	//}
    //H264EncExit(mpCdxEncoder);
    VideoEncDestroy(mpCdxEncoder);
    mpCdxEncoder = NULL;
_err_hw_resize0:
    //ve_mutex_unlock(&mCedarvReqCtx);
    mpCdxEncoder = NULL;
    return UNKNOWN_ERROR;
}

status_t VideoResizerEncoder::setFrameRate(int32_t framerate)
{
    ALOGV("<F:%s, L:%d> framerate=%d", __FUNCTION__, __LINE__, framerate);
    Mutex::Autolock autoLock(mLock);
    mFrameRate = framerate;
	if (setFrameRateControl(mFrameRate/1000, mSrcFrameRate/1000, &mFrameRateCtrl.capture, &mFrameRateCtrl.total) >= 0) {
        ALOGD("<F;%s, L:%d>Low framerate mode, capture %d in %d", __FUNCTION__, __LINE__, mFrameRateCtrl.capture, mFrameRateCtrl.total);
		mFrameRateCtrl.count = 0;
		mFrameRateCtrl.enable = true;
	} else {
		mFrameRateCtrl.enable = false;
	}
    return NO_ERROR;
}

status_t VideoResizerEncoder::setBitRate(int32_t bitrate)
{
    ALOGV("<F:%s, L:%d> bitrate=%d", __FUNCTION__, __LINE__, bitrate);
    Mutex::Autolock autoLock(mLock);
    mBitRate = bitrate;
    if (mpCdxEncoder != NULL) {
        int ret = VideoEncSetParameter(mpCdxEncoder, VENC_IndexParamBitrate, (void*)&bitrate);
        if (ret != 0) {
            ALOGE("<F:%s, L:%d> fatal error! VideoEncSetParameter(VENC_IndexParamBitrate) fail[%d]", __FUNCTION__, __LINE__, ret);
        }
        int threshold = mDesOutWidth * mDesOutHeight;
        ALOGD("bit rate is %d, frame length threshold %d", mBitRate, threshold);
        ret = VideoEncSetParameter(mpCdxEncoder, VENC_IndexParamSetFrameLenThreshold, &threshold);
        if (ret != 0) {
            ALOGE("<F:%s, L:%d> fatal error! VideoEncSetParameter(VENC_IndexParamSetFrameLenThreshold) fail[%d]", __FUNCTION__, __LINE__, ret);
        }
    }
    return NO_ERROR;
}

/*******************************************************************************
Function name: android.VideoResizerEncoder.start
Description: 
    // Idle,Pause->Executing
Parameters: 
    
Return: 
    
Time: 2014/12/7
*******************************************************************************/
status_t VideoResizerEncoder::start()
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
        mEofFlag = false;
        mEncodeEofFlag = false;
        mEncodeThread = new DoEncodeThread(this);
        mEncodeThread->startThread();
        mState = VRComp_StateExecuting;
    }
    else if(VRComp_StatePause == mState)
    {
        //send executing msg
        {
            VideoResizerMessage msg;
            msg.what = VR_ENCODER_MSG_EXECUTING;
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
Function name: android.VideoResizerEncoder.stop
Description: 
     // Executing,Pause->Idle
Parameters: 
    
Return: 
    
Time: 2014/12/7
*******************************************************************************/
status_t VideoResizerEncoder::stop_l()
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
    sp<DoEncodeThread> thread;
    {
        thread = mEncodeThread;
        mEncodeThread.clear();
    }
    if (thread == NULL) 
    {
        ALOGE("(f:%s, l:%d) DoEncodeThread is null?", __FUNCTION__, __LINE__);
    }
    else
    {
        //send stop msg
        {
            VideoResizerMessage msg;
            msg.what = VR_ENCODER_MSG_STOP;
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
status_t VideoResizerEncoder::stop()
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    Mutex::Autolock locker(mLock);
    return stop_l();
}
/*******************************************************************************
Function name: android.VideoResizerEncoder.pause
Description: 
    // Executing->Pause
Parameters: 
    
Return: 
    
Time: 2014/12/7
*******************************************************************************/
status_t VideoResizerEncoder::pause()
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
        msg.what = VR_ENCODER_MSG_PAUSE;
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
status_t VideoResizerEncoder::reset()    // Invalid,Idle,Executing,Pause->Loaded.
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    int i;
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
        if(mpCdxEncoder)
        {
            //if(ve_mutex_lock(&mCedarvReqCtx) < 0)
            //{
            //    ALOGE("(f:%s, l:%d) can not request ve resource, why?", __FUNCTION__, __LINE__);
            //    //return UNKNOWN_ERROR;
            //}
            //H264EncExit(mpCdxEncoder);
            VideoEncDestroy(mpCdxEncoder);
            mpCdxEncoder = NULL;
            //ve_mutex_unlock(&mCedarvReqCtx);
        }
        //if(mCedarvReqCtxInitFlag)
        //{
        //    ve_mutex_destroy(&mCedarvReqCtx);
        //    mCedarvReqCtxInitFlag = false;
        //}
        {
            Mutex::Autolock autoLock(mInputFrameListLock);
            if(false == mInputFrameList.empty())
            {
                ALOGE("(f:%s, l:%d) fatal error! input frame list has [%d]frames ,return to previous component now", __FUNCTION__, __LINE__, mInputFrameList.size());
                OMX_BUFFERHEADERTYPE obh;
                while(!mInputFrameList.empty())
                {
                    obh.pBuffer = (OMX_U8*)&(*mInputFrameList.begin());
                    mpDecoder->FillThisBuffer(&obh);
                    mInputFrameList.erase(mInputFrameList.begin());
                }
            }
        }
        {
            Mutex::Autolock autoLock(mOutIdleVbsListLock);
            if(mOutIdleVbsList.size()!=mOutVbsArray.size())
            {
                ALOGE("(f:%s, l:%d) fatal error! OutIdleVbsList is not full[%d]!=[%d]", __FUNCTION__, __LINE__, mOutIdleVbsList.size(), mOutVbsArray.size());
            }
            mOutIdleVbsList.clear();
        }
        destroyVbsArray();
        resetSomeMembers();
        mState = VRComp_StateLoaded;
    }
    
    return NO_ERROR;
}
/*******************************************************************************
Function name: android.VideoResizerEncoder
Description: 
    call in state Idle,Pause. 
    need return all transport frames, let them in vdec idle list.
Parameters: 
    
Return: 
    
Time: 2014/12/7
*******************************************************************************/
status_t VideoResizerEncoder::seekTo(int msec)
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    int i;
    Mutex::Autolock autoLock(mLock);
    if(mState != VRComp_StateIdle && mState != VRComp_StatePause)
    {
        ALOGE("(f:%s, l:%d) called in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
        return INVALID_OPERATION;
    }
    if(mState == VRComp_StateIdle)
    {
        mEofFlag = false;
        mEncodeEofFlag = false;
        {
            Mutex::Autolock autoLock(mInputFrameListLock);
            if(!mInputFrameList.empty())
            {
                ALOGE("(f:%s, l:%d) input frame list has [%d]frames when seek in Idle, return them!", __FUNCTION__, __LINE__, mInputFrameList.size());
                OMX_BUFFERHEADERTYPE obh;
                while(!mInputFrameList.empty())
                {
                    obh.pBuffer = (OMX_U8*)&(*mInputFrameList.begin());
                    mpDecoder->FillThisBuffer(&obh);
                    mInputFrameList.erase(mInputFrameList.begin());
                }
            }
        }
        return NO_ERROR;
    }
    else
    {
        //send to demuxThread.
        {
            VideoResizerMessage msg;
            msg.what = VR_ENCODER_MSG_SEEK;
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
status_t VideoResizerEncoder::setDecoderComponent(VideoResizerDecoder *pDecoder) //call in state Idle
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    Mutex::Autolock lock(mLock);
    if (mState != VRComp_StateIdle)
    {
        ALOGE("(f:%s, l:%d) called in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
        return INVALID_OPERATION;
    }
    mpDecoder = pDecoder;
    return NO_ERROR;
}
status_t VideoResizerEncoder::setMuxerComponent(VideoResizerMuxer *pMuxer) //call in state Idle
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    Mutex::Autolock lock(mLock);
    if (mState != VRComp_StateIdle)
    {
        ALOGE("(f:%s, l:%d) called in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
        return INVALID_OPERATION;
    }
    mpMuxer = pMuxer;
    return NO_ERROR;
}

status_t VideoResizerEncoder::GetExtraData(VencHeaderData *pExtraData)  //ENCEXTRADATAINFO_t
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    Mutex::Autolock lock(mLock);
    if(mEncoderOpenedFlag)
    {
        pExtraData->pBuffer = mPpsInfo.pBuffer;
        pExtraData->nLength = mPpsInfo.nLength;
        return NO_ERROR;
    }
    else
    {
        return NO_INIT;
    }
}
/*******************************************************************************
Function name: android.VideoResizerEncoder
Description: 
    call in state Idle,Executing,Pause; called by previous component.
Parameters: 
    
Return: 
    
Time: 2014/12/7
*******************************************************************************/
status_t VideoResizerEncoder::EmptyThisBuffer(OMX_BUFFERHEADERTYPE* pBuffer)
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    Mutex::Autolock lock(mLock);
    if (mState != VRComp_StateIdle && mState != VRComp_StateExecuting && mState != VRComp_StatePause)
    {
        ALOGE("(f:%s, l:%d) called in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
        return INVALID_OPERATION;
    }
    sp<VdecOutFrame> inputFrame = *(sp<VdecOutFrame>*)pBuffer->pBuffer;
    if(inputFrame->mStatus!=VdecOutFrame::OwnedByDownStream)
    {
        ALOGE("(f:%s, l:%d) fatal error! inputFrame->mStatus[0x%x] is not OwnedByDownStream!", __FUNCTION__, __LINE__, inputFrame->mStatus);
    }
    {
        Mutex::Autolock autoLock(mInputFrameListLock);
        mInputFrameList.push_back(inputFrame);
        if(mInputFrameUnderflow)
        {
            VideoResizerMessage msg;
            msg.what = VR_ENCODER_MSG_INPUTFRAME_AVAILABLE;
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
Function name: android.VideoResizerEncoder.FillThisBuffer
Description: 
    call in state Idle,Executing,Pause; called by next component.
Parameters: 
    
Return: 
    
Time: 2014/12/7
*******************************************************************************/
status_t VideoResizerEncoder::FillThisBuffer(OMX_BUFFERHEADERTYPE* pBuffer)
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    int releaseRet;
    Mutex::Autolock lock(mLock);
    if (mState != VRComp_StateIdle && mState != VRComp_StateExecuting && mState != VRComp_StatePause)
    {
        ALOGE("(f:%s, l:%d) called in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
        return INVALID_OPERATION;
    }
    sp<VencOutVbs> outVbs = *(sp<VencOutVbs>*)pBuffer->pBuffer;
    {
        Mutex::Autolock autoLock(mOutIdleVbsListLock);
        if(outVbs->mStatus!=VencOutVbs::OwnedByDownStream)
        {
            ALOGE("(f:%s, l:%d) fatal error! outVbs->mStatus[0x%x]!=VencOutVbs::OwnedByDownStream, check code!", __FUNCTION__, __LINE__, outVbs->mStatus);
        }
        outVbs->mStatus = VencOutVbs::OwnedByUs;
        //release vbs to Venclib!
        //releaseRet = mpCdxEncoder->ReleaseBitStreamInfo(mpCdxEncoder, outVbs->getVbsIndex());
        //if(releaseRet != PIC_ENC_ERR_NONE)
        //{
        //    ALOGE("(f:%s, l:%d) fatal error! releaseRet[%d], idx[%d], why?", __FUNCTION__, __LINE__,releaseRet, outVbs->getVbsIndex());
        //}
        VencOutputBuffer outStreamFrame;
        outStreamFrame.nID = outVbs->getVbsIndex();
        releaseRet = FreeOneBitStreamFrame(mpCdxEncoder, &outStreamFrame);
		if(releaseRet != 0)
        {
            ALOGE("(f:%s, l:%d) fatal error! FreeOneBitStreamFrame fail[%d]", __FUNCTION__, __LINE__, releaseRet);
        }
        outVbs->clearVbsInfo();
        mOutIdleVbsList.push_back(outVbs);
        if(mOutVbsUnderflow)
        {
            ALOGV("(f:%s, l:%d) signal outVbs available!", __FUNCTION__, __LINE__);
            VideoResizerMessage msg;
            msg.what = VR_ENCODER_MSG_OUTVBS_AVAILABLE;
            Mutex::Autolock autoLock(mMessageQueueLock);
            if(mMessageQueue.empty())
            {
                mMessageQueueChanged.signal();
            }
            mMessageQueue.push_back(msg);
        }
        if(mbWaitingOutIdleVbsListFull)
        {
            if(mOutIdleVbsList.size()==MaxVideoEncodedFrameNum)
            {
                ALOGD("(f:%s, l:%d) signal OutIdleVbsListFull!", __FUNCTION__, __LINE__);
                mOutIdleVbsListFullCond.signal();
            }
        }
    }
    return NO_ERROR;
}

status_t VideoResizerEncoder::SetConfig(VideoResizerIndexType nIndexType, void *pParam)
{
    switch(nIndexType)
    {
        case VR_IndexConfigSetStreamEof:
        {
            ALOGD("(f:%s, l:%d) notify Encoder eof in state[0x%x]!", __FUNCTION__, __LINE__, mState);
            mEofFlag = true;
            //send eof msg
            {
                VideoResizerMessage msg;
                msg.what = VR_ENCODER_MSG_EOF;
                Mutex::Autolock autoLock(mMessageQueueLock);
                if(mMessageQueue.empty())
                {
                    mMessageQueueChanged.signal();
                }
                mMessageQueue.push_back(msg);
            }
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

bool VideoResizerEncoder::encodeThread()
{
    bool bHasMessage;
    VideoResizerMessage msg;
    int i;
    int encodeRet;
    int getRet;
    int releaseRet;
    int nOutIdleVbsNum = 0;
    sp<VdecOutFrame> inputFrame = NULL;
    sp<VencOutVbs> transportVbs;
    //__vbv_data_ctrl_info_t  outVbvData;
    VencOutputBuffer  outVbvData;
    mDebugFrameCnt = 0;
    //int tm1,tm2,tm3,tm4;
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
                case VR_ENCODER_MSG_EXECUTING:
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
                case VR_ENCODER_MSG_PAUSE:
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
                case VR_ENCODER_MSG_SEEK:
                {
                    if(mState!=VRComp_StatePause)
                    {
                        ALOGE("(f:%s, l:%d) fatal error! call seek in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
                    }
                    //clear Encoder's outVbsList and venclib's vbsBuffer.
                    {
                        //must release OutIdleVbsList's vbs first.
                        Mutex::Autolock autoLock(mOutIdleVbsListLock);
                        while(mOutIdleVbsList.size() < MaxVideoEncodedFrameNum)
                        {
                            if(mbWaitingOutIdleVbsListFull!=false)
                            {
                                ALOGE("(f:%s, l:%d) fatal error! in seek duration, mbWaitingOutIdleVbsListFull[%d] must be false", __FUNCTION__, __LINE__, mbWaitingOutIdleVbsListFull);
                            }
                            mbWaitingOutIdleVbsListFull = true;
                            mOutIdleVbsListFullCond.wait(mOutIdleVbsListLock);
                            mbWaitingOutIdleVbsListFull = false;
                        }
                    }
                    clearVenclibVbsBuffer(mpCdxEncoder);
                    //need return all transport frames, let them in vdec idle list.
                    {
                        Mutex::Autolock autoLock(mInputFrameListLock);
                        if(false == mInputFrameList.empty())
                        {
                            ALOGE("(f:%s, l:%d) input frame list has [%d]frames when seek in Idle, return them!", __FUNCTION__, __LINE__, mInputFrameList.size());
                            OMX_BUFFERHEADERTYPE obh;
                            while(!mInputFrameList.empty())
                            {
                                obh.pBuffer = (OMX_U8*)&(*mInputFrameList.begin());
                                mpDecoder->FillThisBuffer(&obh);
                                mInputFrameList.erase(mInputFrameList.begin());
                            }
                        }
                    }
                    mEofFlag = false;
                    mEncodeEofFlag = false;
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
                case VR_ENCODER_MSG_INPUTFRAME_AVAILABLE:
                {
                    ALOGV("(f:%s, l:%d) state[0x%x], receive inputFrame_available", __FUNCTION__, __LINE__, mState);
                    Mutex::Autolock autoLock(mInputFrameListLock);
                    if(mInputFrameUnderflow!=true)
                    {
                        ALOGD("(f:%s, l:%d) state[0x%x], inputFrameUnderflow[%d]!=true", __FUNCTION__, __LINE__, mState, mInputFrameUnderflow);
                    }
                    mInputFrameUnderflow = false;
                    break;
                }
                case VR_ENCODER_MSG_OUTVBS_AVAILABLE:
                {
                    ALOGV("(f:%s, l:%d) state[0x%x], receive outVbs_available", __FUNCTION__, __LINE__, mState);
                    Mutex::Autolock autoLock(mOutIdleVbsListLock);
                    if(mOutVbsUnderflow!=false)
                    {
                        mOutVbsUnderflow = false;
                    }
                    else
                    {
                        ALOGV("(f:%s, l:%d) state[0x%x], outVbsUnderflow[%d] already false", __FUNCTION__, __LINE__, mState, mOutVbsUnderflow);
                    }
                    break;
                }
                case VR_ENCODER_MSG_EOF:
                {
                    ALOGD("(f:%s, l:%d) receive eof message!", __FUNCTION__, __LINE__);
                    //mEofFlag = true;
                    break;
                }
                case VR_ENCODER_MSG_STOP:
                {
                    ALOGD("(f:%s, l:%d) encodeThread() will exit!", __FUNCTION__, __LINE__);
                    if(mState!=VRComp_StatePause && mState!=VRComp_StateExecuting)
                    {
                        ALOGE("(f:%s, l:%d) fatal error! encodeThread is in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
                    }
                    //need return all transport frames, let them in vdec idle list.
                    {
                        Mutex::Autolock autoLock(mInputFrameListLock);
                        if(false == mInputFrameList.empty())
                        {
                            ALOGE("(f:%s, l:%d) input frame list has [%d]frames when seek in Idle, return them!", __FUNCTION__, __LINE__, mInputFrameList.size());
                            OMX_BUFFERHEADERTYPE obh;
                            while(!mInputFrameList.empty())
                            {
                                obh.pBuffer = (OMX_U8*)&(*mInputFrameList.begin());
                                mpDecoder->FillThisBuffer(&obh);
                                mInputFrameList.erase(mInputFrameList.begin());
                            }
                        }
                    }
                    //wait Encoder's outVbsList to release vbvData to Venclib
                    {
                        Mutex::Autolock autoLock(mOutIdleVbsListLock);
                        while(mOutIdleVbsList.size() < MaxVideoEncodedFrameNum)
                        {
                            ALOGD("(f:%s, l:%d) mOutIdleVbsList[%d], need wait all outVbs return!", __FUNCTION__, __LINE__, mOutIdleVbsList.size());
                            if(mbWaitingOutIdleVbsListFull!=false)
                            {
                                ALOGE("(f:%s, l:%d) fatal error! in seek duration, mbWaitingOutIdleVbsListFull[%d] must be false", __FUNCTION__, __LINE__, mbWaitingOutIdleVbsListFull);
                            }
                            mbWaitingOutIdleVbsListFull = true;
                            mOutIdleVbsListFullCond.wait(mOutIdleVbsListLock);
                            mbWaitingOutIdleVbsListFull = false;
                        }
                    }
                    {
                        Mutex::Autolock autoLock(mMessageQueueLock);
                        if(false == mMessageQueue.empty())
                        {
                            ALOGE("(f:%s, l:%d) Be careful! mMessageQueue has [%d]messages not process when exit demuxThread, ignore and clear them.", __FUNCTION__, __LINE__, mMessageQueue.size());
                            mMessageQueue.clear();
                        }
                    }
                    if(mEncoderOpenedFlag)
                    {
                        //mpCdxEncoder->close(mpCdxEncoder);
            			VideoEncUnInit(mpCdxEncoder);
                        mEncoderOpenedFlag = false;
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
        if(true==mEncodeEofFlag || VRComp_StatePause==mState)
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
        _venc_encode:
        //try to get one input frame
        {
            Mutex::Autolock autoLock(mInputFrameListLock);
            if(mInputFrameList.empty())
            {
                if(mEofFlag)
                {
                    ALOGD("(f:%s, l:%d) Encoder set eof flag", __FUNCTION__, __LINE__);
                    if(mEncodeEofFlag!=false)
                    {
                        ALOGE("(f:%s, l:%d) fatal error! state[0x%x], encode eof flag[0x%x] should be false before set it true!", __FUNCTION__, __LINE__, mState, mEncodeEofFlag);
                    }
                    mEncodeEofFlag = true;
                    ALOGD("(f:%s, l:%d) state[0x%x], notify next component eof!", __FUNCTION__, __LINE__, mState);
                    mpMuxer->SetConfig(VR_IndexConfigSetStreamEof, NULL);
                    goto _process_message;
                }
                else
                {
                    if(mInputFrameUnderflow!=false)
                    {
                        ALOGE("(f:%s, l:%d) state[0x%x], inputFrameUnderflow[%d] already true.", __FUNCTION__, __LINE__, mState, mInputFrameUnderflow);
                    }
                    mInputFrameUnderflow = true;
                }
            }
            else
            {
                inputFrame = *mInputFrameList.begin();
                mInputFrameList.erase(mInputFrameList.begin());

                {
            		if (mFrameRateCtrl.enable) {
                        if (mFrameRateCtrl.count >= mFrameRateCtrl.total) {
                            mFrameRateCtrl.count = 0;
                        }
                        if (mFrameRateCtrl.count++ >= mFrameRateCtrl.capture) {
                            //release VdecOutFrame.
                            OMX_BUFFERHEADERTYPE    obhFrame;
                            obhFrame.pBuffer = (OMX_U8*)&inputFrame;
                            mpDecoder->FillThisBuffer(&obhFrame);
                            inputFrame = NULL;
                            goto _venc_encode;
                        }
            		}
                }

                if(mInputFrameUnderflow!=false)
                {
                    ALOGD("(f:%s, l:%d) state[0x%x], inputFrameUnderflow[%d] not false, set it to false.", __FUNCTION__, __LINE__, mState, mInputFrameUnderflow);
                    mInputFrameUnderflow = false;
                }
            }
        }
        if(mInputFrameUnderflow)
        {
            Mutex::Autolock autoLock(mMessageQueueLock);
            while(mMessageQueue.empty())
            {
                ALOGV("(f:%s, l:%d) no inputFrame, wait", __FUNCTION__, __LINE__);
                mMessageQueueChanged.wait(mMessageQueueLock);
            }
            goto _process_message;
        }
        if(mOutVbsUnderflow)
        {
            ALOGV("(f:%s, l:%d) outVbsUnderflow, goto _transportVbs", __FUNCTION__, __LINE__);
            goto _transportVbs;
        }
        if(mEncoderOpenedFlag == false)
        {
#if 0
            //* open the encoder.
            memset(&mEncFormat, 0, sizeof(__video_encode_format_t));
            
            mEncFormat.src_width		 = inputFrame->mStoreWidth;     //(mCedarvPic.display_width + 31) & ~31;
            mEncFormat.src_height	 = inputFrame->mStoreHeight;    //(mCedarvPic.display_height + 31) & ~31;
            mEncFormat.width			 = mDesOutWidth;    //inputFrame->mDisplayHeight;
            mEncFormat.height		 = mDesOutHeight;   //inputFrame->mDisplayHeight
            mEncFormat.avg_bit_rate	 = 150000;	//* 400 kbits.
            mEncFormat.profileIdc	 = 66;
            mEncFormat.levelIdc		 = 31;
            mEncFormat.color_format	 = convertPixelFormat_Vdec2VEnc(inputFrame->mPixelFormat);
            mEncFormat.color_space	 = BT601;
            mEncFormat.qp_max		 = 40;
            mEncFormat.qp_min		 = 10;
            if(mFrameRate > 0)
            {
                mEncFormat.frame_rate    = mFrameRate;
            }
            else
            {
                mEncFormat.frame_rate = 30000;
            }
            mEncFormat.frame_rate/=mInputFrameRatio;
            mEncFormat.maxKeyInterval = 30; //mFrameRate / 1000;
            //encodeAllFrames = 1;
            ALOGD("(f:%s, l:%d) encFormat srcSize[%dx%d], desSize[%dx%d], format[0x%x], frameRate[%d]", __FUNCTION__, __LINE__,
                mEncFormat.src_width, mEncFormat.src_height, mEncFormat.width, mEncFormat.height, mEncFormat.color_format, mEncFormat.frame_rate);

            ve_mutex_lock(&mCedarvReqCtx);
            mpCdxEncoder->IoCtrl(mpCdxEncoder, VENC_SET_ENC_INFO_CMD, (u32)&mEncFormat);
            mpCdxEncoder->open(mpCdxEncoder);
            //* get SPS, PPS.
            mpCdxEncoder->IoCtrl(mpCdxEncoder, VENC_GET_SPS_PPS_DATA, (u32)&mPpsInfo);
            ve_mutex_unlock(&mCedarvReqCtx);
#endif
            VencH264Param h264Param;
            memset(&h264Param, 0, sizeof(VencH264Param));
            h264Param.bEntropyCodingCABAC = 1;
            if (mBitRate > 0)
            {
                h264Param.nBitrate = mBitRate;
            }
            else
            {
                h264Param.nBitrate = 150000;	//* 400 kbits.
            }
            h264Param.nCodingMode = VENC_FRAME_CODING;
            h264Param.nMaxKeyInterval = 30; //mFrameRate / 1000;
            h264Param.sProfileLevel.nProfile = VENC_H264ProfileMain;
            h264Param.sProfileLevel.nLevel = VENC_H264Level31;
            h264Param.sQPRange.nMinqp = 10;
            h264Param.sQPRange.nMaxqp = 40;
            if(mFrameRate > 0)
            {
                h264Param.nFramerate = mFrameRate / 1000;
            }
            else
            {
                h264Param.nFramerate = 30;
            }

            VideoEncSetParameter(mpCdxEncoder, VENC_IndexParamH264Param, &h264Param);
            int nIFilterEnable = 1;
            VideoEncSetParameter(mpCdxEncoder, VENC_IndexParamIfilter, &nIFilterEnable);
            //VideoEncSetParameter(pCedarV, VENC_IndexParamROIConfig, &sRoiConfig[0]);
            VencBaseConfig baseConfig;
            memset(&baseConfig, 0 ,sizeof(VencBaseConfig));
            baseConfig.nInputWidth= inputFrame->mStoreWidth;     //(mCedarvPic.display_width + 31) & ~31;
            baseConfig.nInputHeight = inputFrame->mStoreHeight;    //(mCedarvPic.display_height + 31) & ~31;
            baseConfig.nStride = inputFrame->mStoreWidth;
            baseConfig.nDstWidth = mDesOutWidth;
            baseConfig.nDstHeight = mDesOutHeight;
/*
            if(V4L2_PIX_FMT_NV21 == inputFrame->mPixelFormat)
            {
                baseConfig.eInputFormat = VENC_PIXEL_YVU420SP;
            }
            else if(V4L2_PIX_FMT_NV12 == inputFrame->mPixelFormat)
            {
                baseConfig.eInputFormat = VENC_PIXEL_YUV420SP;
            }
            else
            {
                ALOGE("(f:%s, l:%d) fatal error! input pixel format =[0x%x], not NV21 or NV12", __FUNCTION__, __LINE__, pVideoEncData->mSourceVideoInfo.mPixelFormat);
            }
*/
            baseConfig.eInputFormat = VENC_PIXEL_YVU420SP;
            VideoEncInit(mpCdxEncoder, &baseConfig);

            VideoEncGetParameter(mpCdxEncoder, VENC_IndexParamH264SPSPPS, &mPpsInfo);    //VencHeaderData    spsppsInfo;

            mEncoderOpenedFlag = true;
        }
        //fill encoder and encode.
		//ve_mutex_lock(&mCedarvReqCtx);
		ALOGV("encode frame %d", mDebugFrameCnt++);
		//VEnc_FrmBuf_Info inFrame;
        //__video_crop_crop_para_t crop_para;
		VencInputBuffer inFrame;
/*
		memset((void*)&inFrame, 0, sizeof(VEnc_FrmBuf_Info));
        inFrame.addrY       = (unsigned char*)inputFrame->mPhyBuf;
        inFrame.addrCb      = (unsigned char*)inputFrame->mPhyBuf+inputFrame->mStoreWidth*inputFrame->mStoreHeight;
        inFrame.addrCr      = NULL;
        inFrame.color_fmt   = mEncFormat.color_format;
        inFrame.color_space = BT601;
        inFrame.pts_valid   = 1;
        inFrame.pts         = inputFrame->mPts;
        ALOGV("(f:%s, l:%d) encode frame para: addrY[%p],addrCb[%p],color_fmt[0x%x],pts[%lld], displaySize[%dx%d]", __FUNCTION__, __LINE__,
            inFrame.addrY, inFrame.addrCb, inFrame.color_fmt, inFrame.pts, inputFrame->mDisplayWidth, inputFrame->mDisplayHeight);
*/
		memset((void*)&inFrame, 0, sizeof(VencInputBuffer));
        //inFrame.nID = inputFrame->mBufferID;  //mBufferID value
        inFrame.nPts = inputFrame->mPts;
        inFrame.nFlag = 0;
        //inFrame.pAddrPhyY = (unsigned char*)inputFrame->mPhyBuf;
        //inFrame.pAddrPhyC = (unsigned char*)inputFrame->mPhyBuf+inputFrame->mStoreWidth*inputFrame->mStoreHeight;
        inFrame.pAddrPhyY = (unsigned char*)MemAdapterGetPhysicAddressCpu(inputFrame->mPicture->pData0);
        inFrame.pAddrPhyC = (unsigned char*)MemAdapterGetPhysicAddressCpu(inputFrame->mPicture->pData1);
        inFrame.pAddrVirY = NULL;
        inFrame.pAddrVirC = NULL;
#if 0
		crop_para.crop_img_enable = 1;
		crop_para.start_x         = 0;
		crop_para.start_y         = 0;
		crop_para.crop_pic_width  = inputFrame->mDisplayWidth;
		crop_para.crop_pic_height = inputFrame->mDisplayHeight;

		mpCdxEncoder->IoCtrl(mpCdxEncoder, VENC_SET_ENC_INFO_CMD, (unsigned int)(&mEncFormat));
		mpCdxEncoder->IoCtrl(mpCdxEncoder, VENC_SET_ENCODE_MODE_CMD, M_ENCODE);
		mpCdxEncoder->IoCtrl(mpCdxEncoder, VENC_CROP_IMAGE_CMD, (unsigned int)(&crop_para));
#endif
        inFrame.bEnableCorp = 1;
        inFrame.sCropInfo.nLeft = 0;
        inFrame.sCropInfo.nTop = 0;
        inFrame.sCropInfo.nWidth = inputFrame->mDisplayWidth;
        inFrame.sCropInfo.nHeight= inputFrame->mDisplayHeight;
		//set var of encoder
		inFrame.ispPicVar = 0;
/*
		//encodeRet = mpCdxEncoder->encode(mpCdxEncoder, &inFrame);
		//ve_mutex_unlock(&mCedarvReqCtx);
        //release VdecOutFrame.        
        OMX_BUFFERHEADERTYPE    obhFrame;
        obhFrame.pBuffer = (OMX_U8*)&inputFrame;
        mpDecoder->FillThisBuffer(&obhFrame);
        inputFrame = NULL;
        
        if(encodeRet == PIC_ENC_ERR_NONE)
        {
            ALOGV("encode frame %d ok", mDebugFrameCnt);
        }
        else
        {
            ALOGE("(f:%s, l:%d) encode fail with return code (%d)!", __FUNCTION__, __LINE__, encodeRet);
        }
*/
        if((encodeRet = AddOneInputBuffer(mpCdxEncoder, &inFrame)) != 0)
        {
            ALOGE("(f:%s, l:%d) fatal error! AddOneInputBuffer fail[%d]", __FUNCTION__, __LINE__, encodeRet);
        }
        if((encodeRet = VideoEncodeOneFrame(mpCdxEncoder)) != 0)
        {
            ALOGE("(f:%s, l:%d) fatal error! VideoEncodeOneFrame fail[%d]", __FUNCTION__, __LINE__, encodeRet);
        }
        if((encodeRet = AlreadyUsedInputBuffer(mpCdxEncoder, &inFrame)) != 0)
        {
            ALOGE("(f:%s, l:%d) fatal error! AlreadyUsedInputBuffer fail[%d]", __FUNCTION__, __LINE__, encodeRet);
        }

        //release VdecOutFrame.
        OMX_BUFFERHEADERTYPE    obhFrame;
        obhFrame.pBuffer = (OMX_U8*)&inputFrame;
        mpDecoder->FillThisBuffer(&obhFrame);
        inputFrame = NULL;

        _transportVbs:
        //get elem from mOutIdleVbsList, to transport vbvData to next component
        {
            Mutex::Autolock autoLock(mOutIdleVbsListLock);
            nOutIdleVbsNum = mOutIdleVbsList.size();
            if(nOutIdleVbsNum>0)
            {
                if(mOutVbsUnderflow!=false)
                {
                    ALOGD("(f:%s, l:%d) state[0x%x], set outVbsUnderflow[%d] to false.", __FUNCTION__, __LINE__, mState, mOutVbsUnderflow);
                    mOutVbsUnderflow = false;
                }
/*
                if((getRet=mpCdxEncoder->GetBitStreamInfo(mpCdxEncoder, &outVbvData)) == PIC_ENC_ERR_NONE)
                {
                    if(outVbvData.uSize0<0 || outVbvData.uSize1<0)
                    {
                        ALOGE("(f:%s, l:%d) fatal error! vbsSize[%d][%d] wrong!", __FUNCTION__, __LINE__, outVbvData.uSize0, outVbvData.uSize1);
                    }
                    //dequeue one idle frame
                    transportVbs = *mOutIdleVbsList.begin();
                    mOutIdleVbsList.erase(mOutIdleVbsList.begin());
                    if(transportVbs->mStatus!=VencOutVbs::OwnedByUs)
                    {
                        ALOGE("(f:%s, l:%d) fatal error! transportVbsStatus[%d]!=OwnedByUs, check code!", __FUNCTION__, __LINE__, transportVbs->mStatus);
                    }
                    transportVbs->mStatus = VencOutVbs::OwnedByVenc;
                }
                else
                {
                    transportVbs = NULL;
                    goto _process_message;
                }
*/
				memset(&outVbvData, 0, sizeof(VencOutputBuffer));
				if(GetOneBitstreamFrame(mpCdxEncoder, &outVbvData) == 0)
				{
					ALOGV("GetOneBitstreamFrame return ok.");
					//* write stream to internal buffer.
					if(outVbvData.pData0 != NULL && outVbvData.nSize0 > 0)
					{
                        //dequeue one idle frame
                        transportVbs = *mOutIdleVbsList.begin();
                        mOutIdleVbsList.erase(mOutIdleVbsList.begin());
                        if(transportVbs->mStatus!=VencOutVbs::OwnedByUs)
                        {
                            ALOGE("(f:%s, l:%d) fatal error! transportVbsStatus[%d]!=OwnedByUs, check code!", __FUNCTION__, __LINE__, transportVbs->mStatus);
                        }
                        transportVbs->mStatus = VencOutVbs::OwnedByVenc;
                    }
				}
                else
                {
                    transportVbs = NULL;
                    goto _process_message;
                }
            }
            else
            {
                transportVbs = NULL;
                Mutex::Autolock autoLock(mMessageQueueLock);
                if(true == mOutVbsUnderflow)
                {
                    ALOGD("(f:%s, l:%d) state[0x%x], outVbsUnderflow[%d] already true!", __FUNCTION__, __LINE__, mState, mOutVbsUnderflow);
                }
                mOutVbsUnderflow = true;
            }
        }
        if(mOutVbsUnderflow)
        {
            Mutex::Autolock autoLock(mMessageQueueLock);
            while(mMessageQueue.empty())
            {
                ALOGV("(f:%s, l:%d) vencComponent has no OutVbvBuf, wait", __FUNCTION__, __LINE__);
                mMessageQueueChanged.wait(mMessageQueueLock);
            }
            goto _process_message;
        }
        if(transportVbs!=NULL)
        {
            transportVbs->setVbsInfo(outVbvData.pData0, outVbvData.nSize0, outVbvData.pData1, outVbvData.nSize1, outVbvData.nPts, outVbvData.nID);
            //call EmptyThisBuffer() to send to next component
            transportVbs->mStatus = VencOutVbs::OwnedByDownStream;
            OMX_BUFFERHEADERTYPE    obhFrame;
            obhFrame.pBuffer = (OMX_U8*)&transportVbs;
            obhFrame.nOutputPortIndex = VR_DemuxVideoOutputPortIndex;
            if(NO_ERROR != mpMuxer->EmptyThisBuffer(&obhFrame))
            {
                ALOGE("(f:%s, l:%d) fatal error! EmptyThisBuffer() fail, framework design can avoid this happen, why it happen? check code!", __FUNCTION__, __LINE__);
                //releaseRet = mpCdxEncoder->ReleaseBitStreamInfo(mpCdxEncoder, transportVbs->getVbsIndex()); 
                //if(releaseRet != PIC_ENC_ERR_NONE)
                //{
                //    ALOGE("(f:%s, l:%d) fatal error! releaseRet[%d], why?", __FUNCTION__, __LINE__,releaseRet);
                //}
                VencOutputBuffer outStreamFrame;
                outStreamFrame.nID = transportVbs->getVbsIndex();
                releaseRet = FreeOneBitStreamFrame(mpCdxEncoder, &outStreamFrame);
        		if(releaseRet != 0)
                {
                    ALOGE("(f:%s, l:%d) fatal error! FreeOneBitStreamFrame fail[%d]", __FUNCTION__, __LINE__, releaseRet);
                }
                transportVbs->clearVbsInfo();
                mOutIdleVbsList.push_back(transportVbs);
            }
            transportVbs = NULL;
            goto _transportVbs;
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


status_t VideoResizerEncoder::createVbsArray()
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    int i;
    sp<VencOutVbs> pVbs;
    for(i=0;i<MaxVideoEncodedFrameNum;i++)
    {
        pVbs = new VencOutVbs();
        mOutVbsArray.push_back(pVbs);
    }
    return NO_ERROR;
}
status_t VideoResizerEncoder::destroyVbsArray()
{
    mOutVbsArray.clear();
    return NO_ERROR;
}

status_t VideoResizerEncoder::clearVenclibVbsBuffer(VideoEncoder *pVenc)
{
    int ret;
    int getRet;
    int releaseRet;
    //__vbv_data_ctrl_info_t dataInfo;
	VencOutputBuffer data_info;

    while(1)
    {
/*
        getRet = pVenc->GetBitStreamInfo(pVenc, &dataInfo);
        if(PIC_ENC_ERR_NONE == getRet)
        {
            releaseRet = pVenc->ReleaseBitStreamInfo(pVenc, dataInfo.idx);
            if(releaseRet != PIC_ENC_ERR_NONE)
            {
                ALOGE("(f:%s, l:%d) fatal error! releaseRet[%d], why?", __FUNCTION__, __LINE__,releaseRet);
            }
        }
        else
        {
            if(PIC_ENC_ERR_NO_CODED_FRAME == getRet)
            {
                ALOGD("(f:%s, l:%d) clear Venclib vbs buffer.", __FUNCTION__, __LINE__);
            }
            else
            {
                ALOGE("(f:%s, l:%d) fatal error! ret[%d], why?", __FUNCTION__, __LINE__,getRet);
            }
            ret = NO_ERROR;
            break;
        }
*/
		memset(&data_info, 0 , sizeof(VencOutputBuffer));
        getRet = GetOneBitstreamFrame(pVenc, &data_info);
		if (getRet == 0) {
			if(FreeOneBitStreamFrame(pVenc, &data_info) != 0) {
                ALOGE("(f:%s, l:%d) fatal error! FreeOneBitStreamFrame fail[%d]", __FUNCTION__, __LINE__, getRet);
            }
		} else {
            ret = NO_ERROR;
            break;
		}
    }
    return ret;
}

void VideoResizerEncoder::resetSomeMembers()
{
    mEofFlag = false;
    mEncodeEofFlag = false;
    mOutVbsUnderflow = false;
    mInputFrameUnderflow = false;
    mEncodeType = OMX_VIDEO_CodingUnused;
    mpDecoder = NULL;
    mpMuxer = NULL;
    mnExecutingDone = 0;
    mnPauseDone = 0;
    mnSeekDone = 0;
    mbWaitingOutIdleVbsListFull = false;
    mDesOutWidth = 0;
    mDesOutHeight = 0;
    mInputFrameRatio = 2;
    mInputFrameLoopNum = 0;
    mDebugFrameCnt = 0;
    mEncodeTotalDuration = 0;
    mLockEncodeTotalDuration = 0;

    mFrameRateCtrl.enable = false;
    mFrameRateCtrl.count = 0;
    mFrameRateCtrl.total = 0;
    mFrameRateCtrl.capture = 0;

    {
        Mutex::Autolock autoLock(mMessageQueueLock);
        if(false == mMessageQueue.empty())
        {
            ALOGD("(f:%s, l:%d) mMessageQueueLock has [%d] messages not process!", __FUNCTION__, __LINE__, mMessageQueue.size());
            mMessageQueue.clear();
        }
    }
}

int VideoResizerEncoder::setFrameRateControl(int reqFrameRate, int srcFrameRate, int *numerator, int *denominator)
{
    int i;
    int ratio;

    if (reqFrameRate >= srcFrameRate) {
        ALOGV("reqFrameRate[%d] > srcFrameRate[%d]", reqFrameRate, srcFrameRate);
        *numerator = 0;
        *denominator = 0;
        return -1;
    }

    ratio = (int)((float)reqFrameRate / srcFrameRate * 1000);

    for (i = srcFrameRate; i > 1; --i) {
        if (ratio < (int)((float)1.0 / i * 1000)) {
            break;
        }
    }

    if (i == srcFrameRate) {
        *numerator = 1;
        *denominator = i;
    } else if (i > 1){
        int rt1 = (int)((float)1.0 / i * 1000);
        int rt2 = (int)((float)1.0 / (i+1) * 1000);
        *numerator = 1;
        if (ratio - rt2 > rt1 - ratio) {
            *denominator = i;
        } else {
            *denominator = i + 1;
        }
    } else {
        for (i = 2; i < srcFrameRate - 1; ++i) {
            if (ratio < (int)((float)i / (i+1) * 1000)) {
                break;
            }
        }
        int rt1 = (int)((float)(i-1) / i * 1000);
        int rt2 = (int)((float)i / (i+1) * 1000);
        if (ratio - rt1 > rt2 - ratio) {
            *numerator = i;
            *denominator = i+1;
        } else {
            *numerator = i-1;
            *denominator = i;
        }
    }
    return 0;
}

}; /* namespace android */

