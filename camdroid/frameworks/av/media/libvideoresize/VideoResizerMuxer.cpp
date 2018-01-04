//#define LOG_NDEBUG 0
#define LOG_TAG "VideoResizerMuxer"

//#include <cutils/log.h>
#include <utils/Log.h>
#include <media/mediavideoresizer.h>

#include "VideoResizerMuxer.h"
#include "VideoResizerDemuxer.h"

namespace android
{
    
VideoResizerMuxer::DoMuxThread::DoMuxThread(VideoResizerMuxer *pOwner)
    : Thread(false),
      mpOwner(pOwner),
      mThreadId(NULL)
{
}
void VideoResizerMuxer::DoMuxThread::startThread()
{
    run("ResizeMuxThread", PRIORITY_DEFAULT);    //PRIORITY_DISPLAY
}
void VideoResizerMuxer::DoMuxThread::stopThread()
{
    requestExitAndWait();
}
status_t VideoResizerMuxer::DoMuxThread::readyToRun() 
{
    mThreadId = androidGetThreadId();

    return Thread::readyToRun();
}
bool VideoResizerMuxer::DoMuxThread::threadLoop()
{
    if(!exitPending())
    {
        return mpOwner->MuxThread();
    }
    else
    {
        return false;
    }
}
VideoResizerMuxer::VideoResizerMuxer()
{
    ALOGV("(f:%s, l:%d) construct", __FUNCTION__, __LINE__);
    mFd = -1;
    resetSomeMembers();
    mState = VRComp_StateLoaded;
}

VideoResizerMuxer::~VideoResizerMuxer()
{
    ALOGV("(f:%s, l:%d) destruct", __FUNCTION__, __LINE__);
    if(mState != VRComp_StateLoaded)
    {
        ALOGE("(f:%s, l:%d) fatal error! deconstruct in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
        reset();
    }
}
/*******************************************************************************
Function name: android.VideoResizerEncoder.setMuxerInfo
Description: 
    // Loaded->Idle
Parameters: 
    
Return: 
    
Time: 2014/12/11
*******************************************************************************/
status_t VideoResizerMuxer::setMuxerInfo(char *pUrl, int nFd, int nWidth, int nHeight, MuxerType nType)
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    int i;
    Mutex::Autolock autoLock(mLock);
    if (mState!=VRComp_StateLoaded) 
    {
        ALOGE("(f:%s, l:%d) call in invalid state[0x%x]!", __FUNCTION__, __LINE__, mState);
        return INVALID_OPERATION;
    }
    //init decoder and encoder.
    mOutUrl = pUrl;
    if(strncmp(mOutUrl.string(), "http://", 7) == 0)
    {
        mbCallbackOutFlag = true;
    }
    else
    {
        mbCallbackOutFlag = false;
    }
    if(nFd>=0)
    {
        mFd = dup(nFd);
    }
    else
    {
        mFd = -1;
    }
    mWidth = nWidth;
    mHeight = nHeight;
    mMuxerType = nType;
    mVideoOutPacketList.clear();
    mAudioOutPacketList.clear();
    if(!mVideoInputBsList.empty())
    {
        ALOGE("(f:%s, l:%d) fatal error! videoInputBsList has size[%d], why? clear!", __FUNCTION__, __LINE__, mVideoInputBsList.size());
        mVideoInputBsList.clear();
    }
    if(!mAudioInputBsList.empty())
    {
        ALOGE("(f:%s, l:%d) fatal error! audioInputBsList has size[%d], why? clear!", __FUNCTION__, __LINE__, mAudioInputBsList.size());
        mAudioInputBsList.clear();
    }
    mState = VRComp_StateIdle;
    return NO_ERROR;
}

/*******************************************************************************
Function name: android.VideoResizerMuxer.start
Description: 
    // Idle,Pause->Executing
Parameters: 
    
Return: 
    
Time: 2014/12/7
*******************************************************************************/
status_t VideoResizerMuxer::start()
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
        mMuxEofFlag = false;
        mMuxThread = new DoMuxThread(this);
        mMuxThread->startThread();
        mState = VRComp_StateExecuting;
    }
    else if(VRComp_StatePause == mState)
    {
        //send executing msg
        {
            VideoResizerMessage msg;
            msg.what = VR_MUXER_MSG_EXECUTING;
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
Function name: android.VideoResizerMuxer.stop
Description: 
     // Executing,Pause->Idle
Parameters: 
    
Return: 
    
Time: 2014/12/7
*******************************************************************************/
status_t VideoResizerMuxer::stop_l()
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
    sp<DoMuxThread> thread;
    {
        thread = mMuxThread;
        mMuxThread.clear();
    }
    if (thread == NULL) 
    {
        ALOGE("(f:%s, l:%d) DoMuxThread is null?", __FUNCTION__, __LINE__);
    }
    else
    {
        //send stop msg
        {
            VideoResizerMessage msg;
            msg.what = VR_MUXER_MSG_STOP;
            Mutex::Autolock autoLock(mMessageQueueLock);
            if(mMessageQueue.empty())
            {
                mMessageQueueChanged.signal();
            }
            mMessageQueue.push_back(msg);
        }
        thread->stopThread();
    }
    if(!mVideoOutPacketList.empty())
    {
        ALOGE("(f:%s, l:%d) fatal error! why videoOutBsList size[%d] is not empty?", __FUNCTION__, __LINE__, mVideoOutPacketList.size());
        mVideoOutPacketList.clear();
    }
    if(!mAudioOutPacketList.empty())
    {
        ALOGE("(f:%s, l:%d) fatal error! why audioOutBsList size[%d] is not empty?", __FUNCTION__, __LINE__, mAudioOutPacketList.size());
        mAudioOutPacketList.clear();
    }
    if(!mVideoInputBsList.empty())
    {
        ALOGE("(f:%s, l:%d) fatal error! videoInputBsList has size[%d], why? clear!", __FUNCTION__, __LINE__, mVideoInputBsList.size());
        mVideoInputBsList.clear();
    }
    if(!mAudioInputBsList.empty())
    {
        ALOGE("(f:%s, l:%d) fatal error! audioInputBsList has size[%d], why? clear!", __FUNCTION__, __LINE__, mAudioInputBsList.size());
        mAudioInputBsList.clear();
    }
    mState = VRComp_StateIdle;
    return NO_ERROR;
}
status_t VideoResizerMuxer::stop()
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    Mutex::Autolock locker(mLock);
    return stop_l();
}
/*******************************************************************************
Function name: android.VideoResizerMuxer.pause
Description: 
    // Executing->Pause
Parameters: 
    
Return: 
    
Time: 2014/12/7
*******************************************************************************/
status_t VideoResizerMuxer::pause()
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
        msg.what = VR_MUXER_MSG_PAUSE;
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
status_t VideoResizerMuxer::reset()    // Invalid,Idle,Executing,Pause->Loaded.
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
        if(mFd>=0)
        {
            ALOGE("(f:%s, l:%d) why mFd[%d]>=0? close it", __FUNCTION__, __LINE__, mFd);
            close(mFd);
            mFd = -1;
        }
        resetSomeMembers();
        mState = VRComp_StateLoaded;
    }
    
    return NO_ERROR;
}
/*******************************************************************************
Function name: android.VideoResizerMuxer.seekTo
Description: 
    call in state Idle,Pause. 
    need return all transport frames, let them in vdec idle list.
Parameters: 
    
Return: 
    
Time: 2014/12/7
*******************************************************************************/
status_t VideoResizerMuxer::seekTo(int msec)
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
        mMuxEofFlag = false;
        Mutex::Autolock autoLock(mFrameListLock);
        //clear outVbsList
        if(!mVideoOutPacketList.empty())
        {
            ALOGD("(f:%s, l:%d) videoOutPacketList has [%d]frames not be read, clear!", __FUNCTION__, __LINE__, mVideoOutPacketList.size());
            mVideoOutPacketList.clear();
        }
        if(!mAudioOutPacketList.empty())
        {
            ALOGD("(f:%s, l:%d) audioOutPacketList has [%d]frames not be read, clear!", __FUNCTION__, __LINE__, mAudioOutPacketList.size());
            mAudioOutPacketList.clear();
        }

        if(!mVideoInputBsList.empty())
        {
            ALOGD("(f:%s, l:%d) videoInputBsList has size[%d], need return!", __FUNCTION__, __LINE__, mVideoInputBsList.size());
            OMX_BUFFERHEADERTYPE obh;
            while(!mVideoInputBsList.empty())
            {
                obh.pBuffer = (OMX_U8*)&(*mVideoInputBsList.begin());
                obh.nOutputPortIndex = VR_DemuxVideoOutputPortIndex;
                if(mResizeFlag)
                {
                    mpEncoder->FillThisBuffer(&obh);
                }
                else
                {
                    mpDemuxer->FillThisBuffer(&obh);
                }
                mVideoInputBsList.erase(mVideoInputBsList.begin());
            }
        }
        if(!mAudioInputBsList.empty())
        {
            ALOGD("(f:%s, l:%d) audioInputBsList has size[%d], need return!", __FUNCTION__, __LINE__, mAudioInputBsList.size());
            OMX_BUFFERHEADERTYPE obh;
            while(!mAudioInputBsList.empty())
            {
                obh.pBuffer = (OMX_U8*)&(*mAudioInputBsList.begin());
                obh.nOutputPortIndex = VR_DemuxAudioOutputPortIndex;
                mpDemuxer->FillThisBuffer(&obh);
                mAudioInputBsList.erase(mAudioInputBsList.begin());
            }
        }
        return NO_ERROR;
    }
    else
    {
        //send to demuxThread.
        {
            VideoResizerMessage msg;
            msg.what = VR_MUXER_MSG_SEEK;
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
            ALOGD("(f:%s, l:%d) before wait seek complete[0x%x]", __FUNCTION__, __LINE__, mnSeekDone);
            Mutex::Autolock autoLock(mStateCompleteLock);
            while(0 == mnSeekDone)
            {
                mSeekCompleteCond.wait(mStateCompleteLock);
            }
            mnSeekDone--;
            ALOGD("(f:%s, l:%d) after wait seek complete[0x%x]", __FUNCTION__, __LINE__, mnSeekDone);
        }
        {
            Mutex::Autolock autoLock(mFrameListLock);
            if(!mVideoOutPacketList.empty())
            {
                ALOGE("(f:%s, l:%d) fatal error! videoOutPacketList has size[%d], check code!", __FUNCTION__, __LINE__, mVideoOutPacketList.size());
            }
            if(!mAudioOutPacketList.empty())
            {
                ALOGE("(f:%s, l:%d) fatal error! audioOutPacketList has size[%d], check code!", __FUNCTION__, __LINE__, mAudioOutPacketList.size());
            }
            if(!mVideoInputBsList.empty())
            {
                ALOGE("(f:%s, l:%d) fatal error! videoInputBsList has size[%d], check code!", __FUNCTION__, __LINE__, mVideoInputBsList.size());
            }
            if(!mAudioInputBsList.empty())
            {
                ALOGE("(f:%s, l:%d) fatal error! audioInputBsList has size[%d], check code!", __FUNCTION__, __LINE__, mAudioInputBsList.size());
            }
        }
        return NO_ERROR;
    }
}


status_t VideoResizerMuxer::setResizeFlag(bool flag) //call in state Idle
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    Mutex::Autolock lock(mLock);
    if (mState != VRComp_StateIdle)
    {
        ALOGE("(f:%s, l:%d) called in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
        return INVALID_OPERATION;
    }
    mResizeFlag = flag;
    return NO_ERROR;
}

status_t VideoResizerMuxer::setEncoderComponent(VideoResizerEncoder *pEncoder) //call in state Idle
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    Mutex::Autolock lock(mLock);
    if (mState != VRComp_StateIdle)
    {
        ALOGE("(f:%s, l:%d) called in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
        return INVALID_OPERATION;
    }
    mpEncoder = pEncoder;
    return NO_ERROR;
}

status_t VideoResizerMuxer::setDemuxerComponent(VideoResizerDemuxer *pDemuxer) //call in state Idle
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    Mutex::Autolock lock(mLock);
    if (mState != VRComp_StateIdle)
    {
        ALOGE("(f:%s, l:%d) called in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
        return INVALID_OPERATION;
    }
    mpDemuxer = pDemuxer;
    return NO_ERROR;
}

status_t VideoResizerMuxer::setListener(sp<VideoResizerComponentListener> pListener) //call in state Idle
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    Mutex::Autolock lock(mLock);
    if (mState != VRComp_StateIdle)
    {
        ALOGE("(f:%s, l:%d) called in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
        return INVALID_OPERATION;
    }
    mpListener = pListener;
    return NO_ERROR;
}
/*******************************************************************************
Function name: android.VideoResizerEncoder.EmptyThisBuffer
Description: 
    call in state Idle,Executing,Pause; called by previous component.
Parameters: 
    
Return: 
    
Time: 2014/12/7
*******************************************************************************/
status_t VideoResizerMuxer::EmptyThisBuffer(OMX_BUFFERHEADERTYPE* pBuffer)
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    Mutex::Autolock lock(mLock);
    if (mState != VRComp_StateIdle && mState != VRComp_StateExecuting && mState != VRComp_StatePause)
    {
        ALOGE("(f:%s, l:%d) called in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
        return INVALID_OPERATION;
    }
    if(VR_DemuxVideoOutputPortIndex == pBuffer->nOutputPortIndex)
    {
        sp<VencOutVbs> inputVbs = *(sp<VencOutVbs>*)pBuffer->pBuffer;
        if(inputVbs->mStatus!=VencOutVbs::OwnedByDownStream)
        {
            ALOGE("(f:%s, l:%d) fatal error! inputVbs->mStatus[0x%x] is not OwnedByDownStream!", __FUNCTION__, __LINE__, inputVbs->mStatus);
        }
        VencOutVbs::BufInfo bufInfo;
        inputVbs->getVbsBufInfo(&bufInfo);
        ALOGV("(f:%s, l:%d) inputVbs len0[%d]len1[%d], data0[%p]data1[%p], pts[%lld]", __FUNCTION__, __LINE__, 
            bufInfo.mDataSize0, bufInfo.mDataSize1, bufInfo.mpData0, bufInfo.mpData1, bufInfo.mDataPts);
        Mutex::Autolock autoLock(mFrameListLock);
        mVideoInputBsList.push_back(inputVbs);
        if(mbWaitBsAvailable)
        {
            VideoResizerMessage msg;
            msg.what = VR_MUXER_MSG_BS_AVAILABLE;
            Mutex::Autolock autoLock(mMessageQueueLock);
            if(mMessageQueue.empty())
            {
                mMessageQueueChanged.signal();
            }
            mMessageQueue.push_back(msg);
        }
        return NO_ERROR;
    }
    else
    {
        ALOGE("(f:%s, l:%d) fatal error! not support audio temporary! pBuffer->nOutputPortIndex[%d]", __FUNCTION__, __LINE__, pBuffer->nOutputPortIndex);
        return UNKNOWN_ERROR;
    }
}

/*******************************************************************************
Function name: android.VideoResizerMuxer.getPacket
Description: 
    
Parameters: 
    
Return: 
    NO_ERROR: operation success. maybe get packet null.
    INVALID_OPERATION: call in wrong status.
    NOT_ENOUGH_DATA: data end.
Time: 2014/12/13
*******************************************************************************/
status_t VideoResizerMuxer::getPacket(sp<IMemory> &rawFrame)
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    Mutex::Autolock lock(mLock);
    if (mState != VRComp_StateExecuting && mState != VRComp_StatePause)
    {
        ALOGE("(f:%s, l:%d) called in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
        rawFrame = NULL;
        return INVALID_OPERATION;
    }
    Mutex::Autolock autoLock(mFrameListLock);
    if(!mVideoOutPacketList.empty())
    {
        rawFrame = *mVideoOutPacketList.begin();
        mVideoOutPacketList.erase(mVideoOutPacketList.begin());
        return NO_ERROR;
    }
    else if(!mAudioOutPacketList.empty())
    {
        ALOGE("(f:%s, l:%d) fatal error! state[0x%x], not support audio temporary!", __FUNCTION__, __LINE__, mState);
        rawFrame = NULL;
        return NO_ERROR;
    }
    else
    {
        rawFrame = NULL;
        if(mEofFlag)
        {
            ALOGD("(f:%s, l:%d) state[0x%x], set mux eof flag", __FUNCTION__, __LINE__, mState);
            mMuxEofFlag = true;
            return NOT_ENOUGH_DATA;
        }
        else
        {
            return NO_ERROR;
        }
    }
}

status_t VideoResizerMuxer::SetConfig(VideoResizerIndexType nIndexType, void *pParam)
{
    switch(nIndexType)
    {
        case VR_IndexConfigSetStreamEof:
        {
            ALOGD("(f:%s, l:%d) notify Muxer eof in state[0x%x]!", __FUNCTION__, __LINE__, mState);
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

bool VideoResizerMuxer::MuxThread()
{
    bool bHasMessage;
    VideoResizerMessage msg;
    int i;
    int encodeRet;
    int getRet;
    int releaseRet;
    int nOutIdleVbsNum = 0;
    sp<VencOutVbs> transportVbs;
    //__vbv_data_ctrl_info_t  outVbvData;
    VencOutputBuffer  outVbvData;
    bool    bCanGetVideo;
    bool    bCanGetAudio;
    bool    bVideoOutPacketListFullFlag;
    bool    bAudioOutPacketListFullFlag;
    sp<VencOutVbs>  videoInputBs;
    sp<VencOutVbs>  audioInputBs;
    sp<IMemory>     videoOutPacket;
    sp<IMemory>     audioOutBs;
    OMX_BUFFERHEADERTYPE    obh;
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
                case VR_MUXER_MSG_EXECUTING:
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
                case VR_MUXER_MSG_PAUSE:
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
                case VR_MUXER_MSG_SEEK:
                {
                    ALOGD("(f:%s, l:%d) thread receive seek msg in state[0x%x]", __FUNCTION__, __LINE__, mState);
                    if(mState!=VRComp_StatePause)
                    {
                        ALOGE("(f:%s, l:%d) fatal error! call seek in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
                    }
                    {
                        Mutex::Autolock autoLock(mFrameListLock);
                        //clear outVbsList
                        if(!mVideoOutPacketList.empty())
                        {
                            ALOGD("(f:%s, l:%d) videoOutBsList has [%d]frames not be read, clear!", __FUNCTION__, __LINE__, mVideoOutPacketList.size());
                            mVideoOutPacketList.clear();
                        }
                        if(!mAudioOutPacketList.empty())
                        {
                            ALOGD("(f:%s, l:%d) audioOutBsList has [%d]frames not be read, clear!", __FUNCTION__, __LINE__, mAudioOutPacketList.size());
                            mAudioOutPacketList.clear();
                        }
                        //return inputBsList
                        if(!mVideoInputBsList.empty())
                        {
                            ALOGD("(f:%s, l:%d) videoInputBsList has size[%d], need return!", __FUNCTION__, __LINE__, mVideoInputBsList.size());
                            OMX_BUFFERHEADERTYPE obh;
                            while(!mVideoInputBsList.empty())
                            {
                                obh.pBuffer = (OMX_U8*)&(*mVideoInputBsList.begin());
                                obh.nOutputPortIndex = VR_DemuxVideoOutputPortIndex;
                                if(mResizeFlag)
                                {
                                    mpEncoder->FillThisBuffer(&obh);
                                }
                                else
                                {
                                    mpDemuxer->FillThisBuffer(&obh);
                                }
                                mVideoInputBsList.erase(mVideoInputBsList.begin());
                            }
                        }
                        if(!mAudioInputBsList.empty())
                        {
                            ALOGD("(f:%s, l:%d) audioInputBsList has size[%d], need return!", __FUNCTION__, __LINE__, mAudioInputBsList.size());
                            OMX_BUFFERHEADERTYPE obh;
                            while(!mAudioInputBsList.empty())
                            {
                                obh.pBuffer = (OMX_U8*)&(*mAudioInputBsList.begin());
                                obh.nOutputPortIndex = VR_DemuxAudioOutputPortIndex;
                                mpDemuxer->FillThisBuffer(&obh);
                                mAudioInputBsList.erase(mAudioInputBsList.begin());
                            }
                        }
                    }
                    mEofFlag = false;
                    mMuxEofFlag = false;
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
                case VR_MUXER_MSG_BS_AVAILABLE:
                {
                    ALOGV("(f:%s, l:%d) state[0x%x], receive Bs_available", __FUNCTION__, __LINE__, mState);
                    Mutex::Autolock autoLock(mFrameListLock);
                    if(false != mbWaitBsAvailable)
                    {
                        mbWaitBsAvailable = false;
                    }
                    else
                    {
                        ALOGD("(f:%s, l:%d) state[0x%x], bWaitBsAvailable already false", __FUNCTION__, __LINE__, mState);
                    }
                    break;
                }
                case VR_MUXER_MSG_STOP:
                {
                    ALOGD("(f:%s, l:%d) muxThread() will exit!", __FUNCTION__, __LINE__);
                    if(mState!=VRComp_StatePause && mState!=VRComp_StateExecuting)
                    {
                        ALOGE("(f:%s, l:%d) fatal error! muxThread is in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
                    }
                    //need return all transport frames, let them in vdec idle list.
                    {
                        Mutex::Autolock autoLock(mFrameListLock);
                        //clear outVbsList
                        if(!mVideoOutPacketList.empty())
                        {
                            ALOGD("(f:%s, l:%d) videoOutBsList has [%d]frames not be read, clear!", __FUNCTION__, __LINE__, mVideoOutPacketList.size());
                            mVideoOutPacketList.clear();
                        }
                        if(!mAudioOutPacketList.empty())
                        {
                            ALOGD("(f:%s, l:%d) audioOutBsList has [%d]frames not be read, clear!", __FUNCTION__, __LINE__, mAudioOutPacketList.size());
                            mAudioOutPacketList.clear();
                        }
                        //return inputBsList
                        if(!mVideoInputBsList.empty())
                        {
                            ALOGD("(f:%s, l:%d) videoInputBsList has size[%d], need return!", __FUNCTION__, __LINE__, mVideoInputBsList.size());
                            OMX_BUFFERHEADERTYPE obh;
                            while(!mVideoInputBsList.empty())
                            {
                                obh.pBuffer = (OMX_U8*)&(*mVideoInputBsList.begin());
                                obh.nOutputPortIndex = VR_DemuxVideoOutputPortIndex;
                                if(mResizeFlag)
                                {
                                    mpEncoder->FillThisBuffer(&obh);
                                }
                                else
                                {
                                    mpDemuxer->FillThisBuffer(&obh);
                                }
                                mVideoInputBsList.erase(mVideoInputBsList.begin());
                            }
                        }
                        if(!mAudioInputBsList.empty())
                        {
                            ALOGD("(f:%s, l:%d) audioInputBsList has size[%d], need return!", __FUNCTION__, __LINE__, mAudioInputBsList.size());
                            OMX_BUFFERHEADERTYPE obh;
                            while(!mAudioInputBsList.empty())
                            {
                                obh.pBuffer = (OMX_U8*)&(*mAudioInputBsList.begin());
                                obh.nOutputPortIndex = VR_DemuxAudioOutputPortIndex;
                                mpDemuxer->FillThisBuffer(&obh);
                                mAudioInputBsList.erase(mAudioInputBsList.begin());
                            }
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
                    return false;
                }
                default:
                {
                    ALOGE("(f:%s, l:%d) unknown msg[%d]!", __FUNCTION__, __LINE__, msg.what);
                    break;
                }
            }
        }
        if(true==mMuxEofFlag || VRComp_StatePause==mState)
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
        //try to get an Bs from previous component
        bCanGetVideo = false;
        bCanGetAudio = false;
        bVideoOutPacketListFullFlag = false;
        bAudioOutPacketListFullFlag = false;
        {
            int nBsSize;
            int nTotalSize;
            int nMemoryHeapSize;
            char *ptrDst;
            VRPacketHeader pktHdr;
            VencOutVbs::BufInfo bufInfo;
            Mutex::Autolock autoLock(mFrameListLock);
            while(!mVideoInputBsList.empty() && mVideoOutPacketList.size()<MaxVideoEncodedFrameNum)
            {
                bCanGetVideo = true;
                videoInputBs = *mVideoInputBsList.begin();
                mVideoInputBsList.erase(mVideoInputBsList.begin());
                videoInputBs->getVbsBufInfo(&bufInfo);
                nBsSize = bufInfo.mDataSize0 + bufInfo.mDataSize1;
                if(nBsSize>0)
                {
                    nTotalSize = sizeof(VRPacketHeader) + nBsSize;
                    nMemoryHeapSize = nTotalSize;
                    sp<MemoryHeapBase> heap = new MemoryHeapBase(nMemoryHeapSize);
                    if (heap == NULL) 
                    {
                        ALOGE("failed to create MemoryDealer");
                        return false;
                    }
                    videoOutPacket = new MemoryBase(heap, 0, nMemoryHeapSize);
                    if (videoOutPacket == NULL) 
                    {
                        ALOGE("not enough memory for VideoFrame size=%u", nBsSize);
                        return false;
                    }
                    pktHdr.mStreamType = MediaVideoResizerStreamType_Video;
                    pktHdr.mSize = nBsSize;
                    pktHdr.mPts = bufInfo.mDataPts/1000;
                    ptrDst = (char*)videoOutPacket->pointer();
                    memcpy(ptrDst, &pktHdr, sizeof(VRPacketHeader));
                    ptrDst += sizeof(VRPacketHeader);
                    if(bufInfo.mDataSize0>0)
                    {
                        memcpy(ptrDst, bufInfo.mpData0, bufInfo.mDataSize0);
                        if(false == mResizeFlag)
                        {
                            if(bufInfo.mDataSize0>=4)
                            {
                                ptrDst[0] = 0x00;
                                ptrDst[1] = 0x00;
                                ptrDst[2] = 0x00;
                                ptrDst[3] = 0x01;
                            }
                            else
                            {
                                ALOGE("(f:%s, l:%d) fatal error! why demuxLib transport [%d]bytes here", __FUNCTION__, __LINE__, bufInfo.mDataSize0);
                            }
                        }
                        ptrDst+=bufInfo.mDataSize0;
                    }
                    if(bufInfo.mDataSize1>0)
                    {
                        memcpy(ptrDst, bufInfo.mpData1, bufInfo.mDataSize1);
                        ptrDst+=bufInfo.mDataSize1;
                    }
                    mVideoOutPacketList.push_back(videoOutPacket);
                    ALOGV("(f:%s, l:%d) state[0x%x], videoOutList size[%d], packet size[%d], pts[%lld]ms", 
                        __FUNCTION__, __LINE__, mState, mVideoOutPacketList.size(), pktHdr.mSize, pktHdr.mPts);
                }
                else
                {
                    ALOGE("(f:%s, l:%d) state[0x%x], videoInputBs.size[%d][%d]<=0, return now!", __FUNCTION__, __LINE__, mState, bufInfo.mDataSize0, bufInfo.mDataSize1);
                }
                obh.pBuffer = (OMX_U8*)&videoInputBs;
                obh.nOutputPortIndex = VR_DemuxVideoOutputPortIndex;
                if(mResizeFlag)
                {
                    mpEncoder->FillThisBuffer(&obh);
                }
                else
                {
                    mpDemuxer->FillThisBuffer(&obh);
                }
                videoInputBs = NULL;
            }
            if(mVideoInputBsList.empty())
            {
                if(true == mbWaitBsAvailable)
                {
                    ALOGD("(f:%s, l:%d) state[0x%x], bWaitBsAvailable[%d] already true, continue wait!", __FUNCTION__, __LINE__, mState, mbWaitBsAvailable);
                }
                mbWaitBsAvailable = true;
            }
            if(mVideoOutPacketList.size()>=MaxVideoEncodedFrameNum)
            {
                bVideoOutPacketListFullFlag = true;
            }
            while(!mAudioInputBsList.empty() && mAudioOutPacketList.size()<MaxVideoEncodedFrameNum)
            {
                //bCanGetAudio = true;
                ALOGE("(f:%s, l:%d) fatal error! state[0x%x], now support audio temporary!", __FUNCTION__, __LINE__, mState);
                obh.pBuffer = (OMX_U8*)&(*mAudioInputBsList.begin());
                obh.nOutputPortIndex = VR_DemuxAudioOutputPortIndex;
                mpDemuxer->FillThisBuffer(&obh);
                mAudioInputBsList.erase(mAudioInputBsList.begin());
            }
        }
        if(bCanGetVideo || bCanGetAudio)
        {
            mpListener->notify(VR_EventBsAvailable, bCanGetVideo?MediaVideoResizerStreamType_Video:MediaVideoResizerStreamType_Audio, 0, NULL);
        }
        if(bVideoOutPacketListFullFlag || bAudioOutPacketListFullFlag)
        {
            ALOGV("(f:%s, l:%d) state[0x%x], video[%d] audio[%d] out packet list is full! sleep...", __FUNCTION__, __LINE__, mState, bVideoOutPacketListFullFlag, bAudioOutPacketListFullFlag);
            usleep(100*1000);
        }
        if(mbWaitBsAvailable)
        {
            Mutex::Autolock autoLock(mMessageQueueLock);
            while(mMessageQueue.empty())
            {
                ALOGV("(f:%s, l:%d) wait BsAvailable", __FUNCTION__, __LINE__);
                mMessageQueueChanged.wait(mMessageQueueLock);
            }
            goto _process_message;
        }
    }
    ALOGE("(f:%s, l:%d) fatal error! impossible come here! exit demuxThread()", __FUNCTION__, __LINE__);
    return false;
}

void VideoResizerMuxer::resetSomeMembers()
{
    mOutUrl = "";
    mWidth = 0;
    mHeight = 0;
    mMuxerType = MUXER_TYPE_RAW;
    mResizeFlag = true;
    mpEncoder = NULL;
    mpDemuxer = NULL;
    mnExecutingDone = 0;
    mnPauseDone = 0;
    mnSeekDone = 0;
    mpListener = NULL;
    mbWaitBsAvailable= false;
    mbCallbackOutFlag = false;

    if(!mVideoOutPacketList.empty())
    {
        ALOGE("(f:%s, l:%d) fatal error! videoOutBsList has size[%d], why? clear!", __FUNCTION__, __LINE__, mVideoOutPacketList.size());
        mVideoOutPacketList.clear();
    }
    if(!mAudioOutPacketList.empty())
    {
        ALOGE("(f:%s, l:%d) fatal error! audioOutBsList has size[%d], why? clear!", __FUNCTION__, __LINE__, mAudioOutPacketList.size());
        mAudioOutPacketList.clear();
    }

    if(!mVideoInputBsList.empty())
    {
        ALOGE("(f:%s, l:%d) fatal error! videoInputBsList has size[%d], why? clear!", __FUNCTION__, __LINE__, mVideoInputBsList.size());
        mVideoInputBsList.clear();
    }
    if(!mAudioInputBsList.empty())
    {
        ALOGE("(f:%s, l:%d) fatal error! audioInputBsList has size[%d], why? clear!", __FUNCTION__, __LINE__, mAudioInputBsList.size());
        mAudioInputBsList.clear();
    }
}

}; /* namespace android */

