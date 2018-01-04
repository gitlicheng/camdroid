//#define LOG_NDEBUG 0
#define LOG_TAG "VideoResizer"

//#include <cutils/log.h>
#include <utils/Log.h>
#include <media/mediavideoresizer.h>
#include <VideoResizer.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>

#include "VideoResizerComponentCommon.h"

namespace android {

VideoResizer::DoResizeThread::DoResizeThread(VideoResizer *pOwner)
    : Thread(false),
      mpOwner(pOwner),
      mThreadId(NULL)
{
}
void VideoResizer::DoResizeThread::startThread()
{
    run("ResizeThread", PRIORITY_DEFAULT);
}
void VideoResizer::DoResizeThread::stopThread()
{
    requestExitAndWait();
}
status_t VideoResizer::DoResizeThread::readyToRun() 
{
    mThreadId = androidGetThreadId();

    return Thread::readyToRun();
}
bool VideoResizer::DoResizeThread::threadLoop()
{
    if(!exitPending())
    {
        return mpOwner->resizeThread();
    }
    else
    {
        return false;
    }
}

VideoResizer::MuxerComponentListener::MuxerComponentListener(VideoResizer *pVR)
    :mpVideoResizer(pVR) 
{
}
void VideoResizer::MuxerComponentListener::notify(int msg, int ext1, int ext2, void *obj)
{
    switch(msg)
    {
        case VR_EventBufferFlag:
        {
            ALOGD("(f:%s, l:%d) muxer msg[0x%x]", __FUNCTION__, __LINE__, msg);
            VideoResizerMessage VRMsg;
            VRMsg.what = VR_EventBufferFlag;
            VRMsg.arg1 = VRComp_TypeMuxer;    //indicate which component. VideoResizerComponentType
            VRMsg.arg2 = 0;
            Mutex::Autolock autoLock(mpVideoResizer->mMessageQueueLock);
            if(mpVideoResizer->mMessageQueue.empty())
            {
                mpVideoResizer->mMessageQueueChanged.signal();
            }
            mpVideoResizer->mMessageQueue.push_back(VRMsg);
            break;
        }
        case VR_EventBsAvailable:
        {
            ALOGV("(f:%s, l:%d) receive muxer BsAvailable[%s]", __FUNCTION__, __LINE__, ext1==0?"video":"audio");
            VideoResizerMessage VRMsg;
            VRMsg.what = VR_EventBsAvailable;
            VRMsg.arg1 = ext1;    //MediaVideoResizerStreamType_Video
            VRMsg.arg2 = 0;
            Mutex::Autolock autoLock(mpVideoResizer->mMessageQueueLock);
            if(mpVideoResizer->mMessageQueue.empty())
            {
                mpVideoResizer->mMessageQueueChanged.signal();
            }
            mpVideoResizer->mMessageQueue.push_back(VRMsg);
            break;
        }
        default:
        {
            ALOGE("(f:%s, l:%d) unknown msg[0x%x]", __FUNCTION__, __LINE__, msg);
            break;
        }
    }
}

VideoResizer::VideoResizer()
    : mStatus(VIDEO_RESIZER_IDLE)
{
    ALOGV("VideoResizer construct");
    //memset(&mInSource, 0, sizeof(ResizerInputSource));
    memset(&mOutDest, 0, sizeof(ResizerOutputDest));
    mOutDest.fd = -1;
    //mInSource.fd = -1;
    mListener = NULL;

    mpDemuxer = NULL;
    mpDecoder = NULL;
    mpEncoder = NULL;
    mpMuxer = NULL;

    mResizeThread = NULL;
    resetSomeMembers();

    mResizeThread = new DoResizeThread(this);
    mResizeThread->startThread();
}

VideoResizer::~VideoResizer()
{
    ALOGV("VideoResizer desconstruct");

    if (mOutDest.url != NULL) 
    {
        free(mOutDest.url);
        mOutDest.url = NULL;
    }

    if (mResizeThread == NULL) 
    {
        ALOGE("(f:%s, l:%d) ResizeThread is null?", __FUNCTION__, __LINE__);
    }
    else
    {
        {
            Mutex::Autolock autoLock(mMessageQueueLock);
            if(!mMessageQueue.empty())
            {
                ALOGE("(f:%s, l:%d) Be careful! mMessageQueue has [%d]message not process in state[%d], clear!", __FUNCTION__, __LINE__, mMessageQueue.size(), mStatus);
                mMessageQueue.clear();
            }
        }
        {
            VideoResizerMessage msg;
            msg.what = VIDEO_RESIZER_MESSAGE_STOP;
            Mutex::Autolock autoLock(mMessageQueueLock);
            if(mMessageQueue.empty())
            {
                mMessageQueueChanged.signal();
            }
            mMessageQueue.push_back(msg);
        }
        mResizeThread->stopThread();
    }
}

status_t VideoResizer::setDataSource(const char * url)
{
    ALOGV("setDataSource url(%s)", url);
    status_t ret;
    Mutex::Autolock autoLock(mLock);
    if (mStatus!=VIDEO_RESIZER_IDLE) 
    {
        ALOGE("<F:%s, L:%d>Must call setDataSource before prepare!", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }
    mpDemuxer = new VideoResizerDemuxer();
    ret = mpDemuxer->setDataSource(url);
    if(ret!=NO_ERROR)
    {
        return UNKNOWN_ERROR;
    }
    mStatus = VIDEO_RESIZER_INITIALIZED;
    return NO_ERROR;}

status_t VideoResizer::setDataSource(int fd, int64_t offset, int64_t length)
{
    ALOGV("setDataSource, fd(%d), offset(%lld), length(%lld)", fd, offset, length);
    status_t ret;
    Mutex::Autolock autoLock(mLock);
    if (mStatus!=VIDEO_RESIZER_IDLE) 
    {
        ALOGE("<F:%s, L:%d>Must call setDataSource before prepare!", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }
    mpDemuxer = new VideoResizerDemuxer();
    ret = mpDemuxer->setDataSource(fd, offset, length);
    if(ret!=NO_ERROR)
    {
        return UNKNOWN_ERROR;
    }
    mStatus = VIDEO_RESIZER_INITIALIZED;
    return NO_ERROR;
}

status_t VideoResizer::setVideoSize(int32_t width, int32_t height)
{
    ALOGV("setVideoSize width(%d), height(%d)", width, height);
    Mutex::Autolock autoLock(mLock);
    if (mStatus != VIDEO_RESIZER_IDLE && mStatus != VIDEO_RESIZER_INITIALIZED && mStatus != VIDEO_RESIZER_STOPPED) 
    {
        ALOGE("<F:%s, L:%d>Must call setVideoSize before prepare! mStatus[0x%x]", __FUNCTION__, __LINE__, mStatus);
        return INVALID_OPERATION;
    }
    mOutDest.width = width;
    mOutDest.height = height;
    return NO_ERROR;
}

status_t VideoResizer::setOutputPath(const char * url)
{
    ALOGV("setOutputPath url(%s)", url);
    Mutex::Autolock autoLock(mLock);
    if (mStatus != VIDEO_RESIZER_IDLE && mStatus != VIDEO_RESIZER_INITIALIZED && mStatus != VIDEO_RESIZER_STOPPED) 
    {
        ALOGE("<F:%s, L:%d>Must call setOutputPath before prepare! mStatus[0x%x]", __FUNCTION__, __LINE__, mStatus);
        return INVALID_OPERATION;
    }
    if (mOutDest.url != NULL) 
    {
        ALOGD("<F:%s, L:%d>url[%s] is not null, free it", __FUNCTION__, __LINE__, mOutDest.url);
        free(mOutDest.url);
    }
    mOutDest.url = strdup(url);
    return NO_ERROR;
}

status_t VideoResizer::prepare()
{
    ALOGV("prepare");
    status_t ret;
    Mutex::Autolock autoLock(mLock);
    if (VIDEO_RESIZER_PREPARED == mStatus)
    {
        return NO_ERROR;
    }
    if (mStatus != VIDEO_RESIZER_INITIALIZED && mStatus != VIDEO_RESIZER_STOPPED) 
    {
        ALOGE("<F:%s, L:%d>prepare called in invalid state[0x%x]", __FUNCTION__, __LINE__, mStatus);
        return INVALID_OPERATION;
    }
    //prepare vdeclib and venclib.
    OMX_VIDEO_PORTDEFINITIONTYPE *pVideoFormat = NULL;
    CedarXMediainfo *pCdxMediainfo = mpDemuxer->getMediainfo();
    if (pCdxMediainfo->nHasVideo && pCdxMediainfo->VidStrmList[0].eCompressionFormat != OMX_VIDEO_CodingUnused)
	{
        ALOGD("(f:%s, l:%d) video format[0x%x], wxh[%dx%d]", __FUNCTION__, __LINE__, pCdxMediainfo->VidStrmList[0].eCompressionFormat,
            pCdxMediainfo->VidStrmList[0].nFrameWidth, pCdxMediainfo->VidStrmList[0].nFrameHeight);
	}
	else
	{
        ALOGD("(f:%s, l:%d) media file has no video!", __FUNCTION__, __LINE__);
	}
    
    //decide if need decode and encode. If mOutDest.width and mOutDest.height is equal to source file,
    //then we can transport it directly.
    if(pCdxMediainfo->nHasVideo)
    {
        pVideoFormat = &pCdxMediainfo->VidStrmList[0];
        if(pVideoFormat->nFrameWidth*pVideoFormat->nFrameHeight > (uint32_t)mOutDest.width*mOutDest.height)
        {
            ALOGD("(f:%s, l:%d) file video size[%dx%d]>output size[%dx%d], need resize!", __FUNCTION__, __LINE__, 
                pVideoFormat->nFrameWidth, pVideoFormat->nFrameHeight, mOutDest.width, mOutDest.height);
            mResizeFlag = true;
        }
        else
        {
            ALOGD("(f:%s, l:%d) file video size[%dx%d]<=output size[%dx%d], transport directly!", __FUNCTION__, __LINE__, 
                pVideoFormat->nFrameWidth, pVideoFormat->nFrameHeight, mOutDest.width, mOutDest.height);
            mResizeFlag = false;
        }
    }

    if(true == mResizeFlag)
    {
        if(mpDecoder || mpEncoder)
        {
            ALOGE("(f:%s, l:%d) fatal error, mpDecoder and mpEncoder must be null, check code!", __FUNCTION__, __LINE__);
        }
        mpDecoder = new VideoResizerDecoder();
        ret = mpDecoder->setVideoFormat(pVideoFormat, mpDemuxer->getMediaFileFormat(), mOutDest.width, mOutDest.height);
        if(ret != NO_ERROR)
        {
            ALOGE("(f:%s, l:%d) mpDecoder setVideoFormat fail[0x%x]!", __FUNCTION__, __LINE__, ret);
            goto _err_prepare1;
        }
        mpEncoder = new VideoResizerEncoder();
        if (mOutDest.frameRate <= 0) {
            mOutDest.frameRate = pVideoFormat->xFramerate;
        }
        ret = mpEncoder->setEncodeType(pVideoFormat->eCompressionFormat, mOutDest.frameRate, mOutDest.bitRate, mOutDest.width, mOutDest.height, pVideoFormat->xFramerate);
        if(ret != NO_ERROR)
        {
            ALOGE("(f:%s, l:%d) mpEncoder setEncodeType fail[0x%x]!", __FUNCTION__, __LINE__, ret);
            goto _err_prepare1;
        }
    }
    mpMuxer = new VideoResizerMuxer();
    ret = mpMuxer->setMuxerInfo(mOutDest.url, mOutDest.fd, mOutDest.width, mOutDest.height, MUXER_TYPE_RAW);
    if(ret != NO_ERROR)
    {
        ALOGE("(f:%s, l:%d) mpMuxer setMuxerInfo fail[0x%x]!", __FUNCTION__, __LINE__, ret);
        goto _err_prepare1;
    }
    mMuxerCompListener = new MuxerComponentListener(this);
    //create link between different component.
    mpDemuxer->setResizeFlag(mResizeFlag);
    mpMuxer->setResizeFlag(mResizeFlag);
    if(mResizeFlag)
    {
        mpDemuxer->setDecoderComponent(mpDecoder);
        mpDecoder->setEncoderComponent(mpEncoder);
        mpEncoder->setDecoderComponent(mpDecoder);
        mpEncoder->setMuxerComponent(mpMuxer);
        mpMuxer->setEncoderComponent(mpEncoder);
        mpMuxer->setListener(mMuxerCompListener);
    }
    else
    {
        mpDemuxer->setMuxerComponent(mpMuxer);
        mpMuxer->setDemuxerComponent(mpDemuxer);
        mpMuxer->setListener(mMuxerCompListener);
    }
    
    mStatus = VIDEO_RESIZER_PREPARED;
    return NO_ERROR;

_err_prepare1:
    if(mpEncoder)
    {
        mpEncoder->reset();
        delete mpEncoder;
        mpEncoder = NULL;
    }
    if(mpDecoder)
    {
        mpDecoder->reset();
        delete mpDecoder;
        mpDecoder = NULL;
    }
    if(mpMuxer)
    {
        mpMuxer->reset();
        delete mpMuxer;
        mpMuxer = NULL;
    }
    mStatus = VIDEO_RESIZER_STATE_ERROR;
	return UNKNOWN_ERROR;
}

status_t VideoResizer::start()
{
    ALOGV("start");
    Mutex::Autolock autoLock(mLock);
    if (VIDEO_RESIZER_STARTED == mStatus)
    {
        return NO_ERROR;
    }
    if (mStatus != VIDEO_RESIZER_PREPARED && mStatus != VIDEO_RESIZER_PAUSED && mStatus != VIDEO_RESIZER_PLAYBACK_COMPLETE)
    {
        ALOGE("<F:%s, L:%d>start called in invalid state[0x%x]", __FUNCTION__, __LINE__, mStatus);
        return INVALID_OPERATION;
    }
    //start thread to demux file and sink it!
    if(mStatus == VIDEO_RESIZER_PREPARED)
    {
        mpDemuxer->start();
        if(mResizeFlag)
        {
            mpDecoder->start();
            mpEncoder->start();
        }
        mpMuxer->start();

        if(mResizeFlag)
        {
            while(1)
            {
                if(NO_ERROR == mpEncoder->GetExtraData(&mPpsInfo))
                {
                    ALOGV("(f:%s, l:%d) state[0x%x], get extraData from Encoder.", __FUNCTION__, __LINE__, mStatus);
                    break;
                }
                else
                {
                    ALOGD("(f:%s, l:%d) state[0x%x], wait 50ms to get extraData from Encoder.", __FUNCTION__, __LINE__, mStatus);
                    usleep(50*1000);
                }
            }
        }
        else
        {
            ALOGV("(f:%s, l:%d) state[0x%x], demuxer has ready for extraData!", __FUNCTION__, __LINE__, mStatus);
        }
    }
    else if(mStatus == VIDEO_RESIZER_PLAYBACK_COMPLETE)
    {
        mpMuxer->stop();
        if(mResizeFlag)
        {
            mpEncoder->stop();
            mpDecoder->stop();
        }
        mpDemuxer->stop();
        
        mpDemuxer->start();
        if(mResizeFlag)
        {
            mpDecoder->start();
            mpEncoder->start();
        }
        mpMuxer->start();
    }
    else
    {
        ALOGD("(f:%s, l:%d) not support now. start() in state VIDEO_RESIZER_PAUSED.", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }
    
    mStatus = VIDEO_RESIZER_STARTED;
    return NO_ERROR;
}

status_t VideoResizer::stop_l()
{
    if (VIDEO_RESIZER_STOPPED == mStatus)
    {
        return NO_ERROR;
    }
    if (mStatus != VIDEO_RESIZER_STARTED && mStatus != VIDEO_RESIZER_PREPARED 
        && mStatus != VIDEO_RESIZER_PAUSED && mStatus != VIDEO_RESIZER_PLAYBACK_COMPLETE)
    {
        ALOGE("<F:%s, L:%d>stop called in invalid state[0x%x]", __FUNCTION__, __LINE__, mStatus);
        return INVALID_OPERATION;
    }
    //pause
    {
        ALOGD("(f:%s, l:%d) status[0x%x], pause component before stop!", __FUNCTION__, __LINE__, mStatus);
        mpDemuxer->pause();
        if(mResizeFlag)
        {
            mpDecoder->pause();
            mpEncoder->pause();
        }
        mpMuxer->pause();
    }
    mpMuxer->stop();
    //close vdeclib and venclib
    if(mResizeFlag)
    {
        mpEncoder->stop();
        mpDecoder->stop();
    }
    mpDemuxer->stop();

    if(mpMuxer)
    {
        mpMuxer->reset();
        delete mpMuxer;
        mpMuxer = NULL;
    }
    if(mpEncoder)
    {
        mpEncoder->reset();
        delete mpEncoder;
        mpEncoder = NULL;
    }
    if(mpDecoder)
    {
        mpDecoder->reset();
        delete mpDecoder;
        mpDecoder = NULL;
    }
    mMuxerCompListener = NULL;
    mStatus = VIDEO_RESIZER_STOPPED;
    return NO_ERROR;
}

status_t VideoResizer::stop()
{
    ALOGV("stop");
    Mutex::Autolock autoLock(mLock);
    return stop_l();
}

status_t VideoResizer::pause()
{
    ALOGE("(f:%s, l:%d) pause not implement", __FUNCTION__, __LINE__);
    Mutex::Autolock autoLock(mLock);
    return INVALID_OPERATION;
}

status_t VideoResizer::seekTo(int msec)
{
    ALOGV("seekTo(%d)ms", msec);
    Mutex::Autolock autoLock(mLock);
    if ( mStatus != VIDEO_RESIZER_STARTED && mStatus != VIDEO_RESIZER_PREPARED 
        && mStatus != VIDEO_RESIZER_PAUSED &&  mStatus != VIDEO_RESIZER_PLAYBACK_COMPLETE )
    {
        ALOGE("<F:%s, L:%d>start called in invalid state[0x%x]", __FUNCTION__, __LINE__, mStatus);
        return INVALID_OPERATION;
    }
    if(mStatus == VIDEO_RESIZER_PREPARED || mStatus == VIDEO_RESIZER_PLAYBACK_COMPLETE)
    {
        int seekRet;
        int nFlag = true;
        seekRet = mpMuxer->seekTo(msec);
        if(seekRet!=NO_ERROR)
        {
            ALOGE("(f:%s, l:%d) muxer seek to [%d]ms fail!", __FUNCTION__, __LINE__, msec);
            nFlag = false;
        }
        if(mResizeFlag)
        {
            seekRet = mpEncoder->seekTo(msec);
            if(seekRet!=NO_ERROR)
            {
                ALOGE("(f:%s, l:%d) encoder seek to [%d]ms fail!", __FUNCTION__, __LINE__, msec);
                nFlag = false;
            }
            seekRet = mpDecoder->seekTo(msec);
            if(seekRet!=NO_ERROR)
            {
                ALOGE("(f:%s, l:%d) decoder seek to [%d]ms fail!", __FUNCTION__, __LINE__, msec);
                nFlag = false;
            }
        }
        seekRet = mpDemuxer->seekTo(msec);
        if(seekRet!=NO_ERROR)
        {
            ALOGE("(f:%s, l:%d) demuxer seek to [%d]ms fail!", __FUNCTION__, __LINE__, msec);
            nFlag = false;
        }
        if(nFlag)
        {
            mListener->notify(MEDIA_VIDEO_RESIZER_EVENT_SEEK_COMPLETE, 0, 0);
            return NO_ERROR;
        }
        else
        {
            mListener->notify(MEDIA_VIDEO_RESIZER_EVENT_ERROR, MEDIA_VIDEO_RESIZER_ERROR_SEEK_FAIL, 0);
            //return UNKNOWN_ERROR;
            return NO_ERROR;
        }
    }
    else    //send to resizeThread.
    {
        {
            VideoResizerMessage msg;
            msg.what = VIDEO_RESIZER_MESSAGE_SEEK;
            msg.arg1 = msec;
            Mutex::Autolock autoLock(mMessageQueueLock);
            if(mMessageQueue.empty())
            {
                mMessageQueueChanged.signal();
            }
            mMessageQueue.push_back(msg);
        }
        //wait_seek_done! 
        //if use sync mode, then wait here.
        //if use async mode, don't wait, thread will notify if complete.
        //we use sync mode here, because we want to simulate demuxLib behaviour. 
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
        //notify app
        if(NO_ERROR==mSeekResult)
        {
            mListener->notify(MEDIA_VIDEO_RESIZER_EVENT_SEEK_COMPLETE, 0, 0);
        }
        else
        {
            mListener->notify(MEDIA_VIDEO_RESIZER_EVENT_ERROR, MEDIA_VIDEO_RESIZER_ERROR_SEEK_FAIL, 0);
        }
        return NO_ERROR;
    }
}

status_t VideoResizer::reset()
{
    status_t ret;
    Mutex::Autolock autoLock(mLock);
    if (mStatus == VIDEO_RESIZER_IDLE)
    {
        return NO_ERROR;
    }
    
    if(mStatus == VIDEO_RESIZER_PREPARED || mStatus == VIDEO_RESIZER_STARTED
        || mStatus == VIDEO_RESIZER_PAUSED || mStatus == VIDEO_RESIZER_PLAYBACK_COMPLETE)
    {
        ret = stop_l();
        if(ret!=NO_ERROR)
        {
            ALOGE("(F:%s, L:%d) fatal error! stop() called fail in state[0x%x]", __FUNCTION__, __LINE__, mStatus);
            mStatus = VIDEO_RESIZER_STATE_ERROR;
        }
    }
    if(mStatus == VIDEO_RESIZER_STATE_ERROR)
    {
        if(mpEncoder)
        {
            mpEncoder->reset();
            delete mpEncoder;
            mpEncoder = NULL;
        }
        if(mpDecoder)
        {
            mpDecoder->reset();
            delete mpDecoder;
            mpDecoder = NULL;
        }
        if(mpMuxer)
        {
            mpMuxer->reset();
            delete mpMuxer;
            mpMuxer = NULL;
        }
    }
    if(mStatus == VIDEO_RESIZER_STOPPED || mStatus == VIDEO_RESIZER_INITIALIZED || mStatus == VIDEO_RESIZER_STATE_ERROR)
    {
        if(mpDemuxer)
        {
            mpDemuxer->reset();
            delete mpDemuxer;
            mpDemuxer = NULL;
        }
        if(mOutDest.url)
        {
            free(mOutDest.url);
            mOutDest.url = NULL;
        }
        if(mOutDest.fd>=0)
        {
            ALOGD("(f:%s, l:%d) mOutDest.fd[%d] will be closed", __FUNCTION__, __LINE__, mOutDest.fd);
            close(mOutDest.fd);
            mOutDest.fd = -1;
        }
        //memset(&mInSource, 0, sizeof(ResizerInputSource));
        memset(&mOutDest, 0, sizeof(ResizerOutputDest));
        //mInSource.fd = -1;
        resetSomeMembers();
        mStatus = VIDEO_RESIZER_IDLE;
    }

    if(mStatus != VIDEO_RESIZER_IDLE)
    {
        ALOGE("(f:%s, l:%d) mStatus[0x%x]!=VIDEO_RESIZER_IDLE, check code!", __FUNCTION__, __LINE__, mStatus);
    }

    return NO_ERROR;
}

status_t VideoResizer::getDuration(int *msec)
{
    ALOGV("getDuration");
    Mutex::Autolock autoLock(mLock);
    if (mStatus != VIDEO_RESIZER_PREPARED && mStatus != VIDEO_RESIZER_STARTED && mStatus != VIDEO_RESIZER_PAUSED
        && mStatus != VIDEO_RESIZER_STOPPED && mStatus != VIDEO_RESIZER_PLAYBACK_COMPLETE)
    {
        ALOGE("<F:%s, L:%d>call in invalid status[0x%x]", __FUNCTION__, __LINE__, mStatus);
        return INVALID_OPERATION;
    }
    if(msec)
    {
        CedarXMediainfo *pCdxMediainfo = mpDemuxer->getMediainfo();
        *msec = pCdxMediainfo->nDuration;
        return NO_ERROR;
    }
    else
    {
        return UNKNOWN_ERROR;
    }
    
}

status_t VideoResizer::setListener(const sp<IMediaVideoResizerClient> &listener)
{
    ALOGV("setListener");
    Mutex::Autolock autoLock(mLock);
    mListener = listener;
    return NO_ERROR;
}

/*******************************************************************************
Function name: android.VideoResizer.getEncDataHeader
Description: 
    hw encoder output spspps struct:
        0x00,00,00,01,sps_data,00,00,00,01,pps_data;
    mp4 demuxer output spspps struct:
        0xxx,xx,xx,xx,xx,xx, spslen(2 byte), sps_data, xx, ppslen(2 byte), pps_data.
Parameters: 
    
Return: 
    
Time: 2014/12/18
*******************************************************************************/
sp<IMemory> VideoResizer::getEncDataHeader()
{
    Mutex::Autolock autoLock(mLock);
    if (mStatus != VIDEO_RESIZER_STARTED)
    {
        ALOGE("<F:%s, L:%d>call in invalid status[0x%x]", __FUNCTION__, __LINE__, mStatus);
        return NULL;
    }
    int i;
    size_t nEncDataSize;
    char *spsPtr;
    char *ppsPtr;
    int spsSize = 0;
    int ppsSize = 0;
    if(mResizeFlag)
    {
        if(0 == mPpsInfo.nLength)
        {
            ALOGE("(f:%s, l:%d) hw encoder PpsSps length=0!", __FUNCTION__, __LINE__);
            return NULL;
        }
        nEncDataSize = mPpsInfo.nLength + sizeof(int);
        ALOGV("(f:%s, l:%d) hwEncoder SpsPps len:%d", __FUNCTION__, __LINE__, mPpsInfo.length);
        for(i=0;i<(int)mPpsInfo.nLength;i++)
        {
            ALOGV("0x%x", mPpsInfo.pBuffer[i]);
        }
    }
    else
    {
        CedarXMediainfo *pCdxMediainfo = mpDemuxer->getMediainfo();
        if(pCdxMediainfo->nHasVideo)
        {
            char *pExtraData = (char*)pCdxMediainfo->VidStrmList[0].pCodecExtraData;
            int nExtraDataLen = pCdxMediainfo->VidStrmList[0].nCodecExtraDataLen;
            char *cursor = pExtraData + 6;
        	spsSize = (*(cursor++) << 8);
        	spsSize += *(cursor++);
            spsPtr = cursor;
            cursor += spsSize + 1;
            ppsSize = (*(cursor++) << 8);
        	ppsSize += *(cursor++);
            ppsPtr = cursor;
            nEncDataSize = 4+spsSize+4+ppsSize+sizeof(int);

            ALOGV("(f:%s, l:%d) mp4 demuxer SpsPps len:%d", __FUNCTION__, __LINE__, nExtraDataLen);
            for(i=0;i<nExtraDataLen;i++)
            {
                ALOGV("0x%x", pExtraData[i]);
            }
        }
        else
        {
            ALOGE("(f:%s, l:%d) demuxer has no video!", __FUNCTION__, __LINE__);
            return NULL;
        }
    }
    
    sp<MemoryHeapBase> heap = new MemoryHeapBase(nEncDataSize);
    if (heap == NULL) {
        ALOGE("failed to create MemoryDealer");
        return NULL;
    }
    sp<IMemory> ppsInfoMemory = new MemoryBase(heap, 0, nEncDataSize);
    if (ppsInfoMemory == NULL) {
        ALOGE("not enough memory for ppsInfo size=%u", nEncDataSize);
        return NULL;
    }
	uint8_t *ptr_dst = (uint8_t *)ppsInfoMemory->pointer();

    if(mResizeFlag)
    {
        memcpy(ptr_dst, mPpsInfo.pBuffer, mPpsInfo.nLength);
        ptr_dst += mPpsInfo.nLength;
    	memcpy(ptr_dst, &mPpsInfo.nLength, sizeof(int));
    }
    else
    {
        *ptr_dst++ = 0x00;
        *ptr_dst++ = 0x00;
        *ptr_dst++ = 0x00;
        *ptr_dst++ = 0x01;
        memcpy(ptr_dst, spsPtr, spsSize);
    	ptr_dst += spsSize;

        *ptr_dst++ = 0x00;
        *ptr_dst++ = 0x00;
        *ptr_dst++ = 0x00;
        *ptr_dst++ = 0x01;
        memcpy(ptr_dst, ppsPtr, ppsSize);
        ptr_dst += ppsSize;

        *(int*)ptr_dst = nEncDataSize - sizeof(int);
    }
	return ppsInfoMemory;
}

//sp<IMemory> VideoResizer::getOneBsFrame()
status_t VideoResizer::getPacket(sp<IMemory> &rawFrame)
{
    ALOGV("getOneBsFrame");
    int ret;
    Mutex::Autolock autoLock(mLock);
    if (mStatus != VIDEO_RESIZER_STARTED && mStatus != VIDEO_RESIZER_PAUSED)
    {
        ALOGE("<F:%s, L:%d>call in invalid status[0x%x]", __FUNCTION__, __LINE__, mStatus);
        return INVALID_OPERATION;
    }
    return mpMuxer->getPacket(rawFrame);
}

bool VideoResizer::resizeThread()
{
    VideoResizerMessage msg;
    bool bHasMessage;
    while(1)
    {
        _process_message:
        {
            Mutex::Autolock autoLock(mMessageQueueLock);
            while(mMessageQueue.empty())
            {
                mMessageQueueChanged.wait(mMessageQueueLock);
            }
            bHasMessage = true;
            msg = *mMessageQueue.begin();
            mMessageQueue.erase(mMessageQueue.begin());
        }
        if(bHasMessage)
        {
            switch(msg.what)
            {
                case VIDEO_RESIZER_MESSAGE_SEEK:
                {
                    int seekRet;
                    int msec = msg.arg1;
                    mSeekResult = NO_ERROR;
                    VideoResizerStates  curState = mStatus;
                    if(VIDEO_RESIZER_PAUSED != curState && VIDEO_RESIZER_STARTED != curState)
                    {
                        ALOGE("(f:%s, l:%d) fatal error! receive seek cmd in invalid status[0x%x]", __FUNCTION__, __LINE__, curState);
                        mListener->notify(MEDIA_VIDEO_RESIZER_EVENT_ERROR, MEDIA_VIDEO_RESIZER_ERROR_SEEK_FAIL, 0);
                        break;
                    }
                    //pause
                    if(curState != VIDEO_RESIZER_PAUSED)
                    {
                        ALOGD("(f:%s, l:%d) status[0x%x], pause component before seek!", __FUNCTION__, __LINE__, curState);
                        mpDemuxer->pause();
                        if(mResizeFlag)
                        {
                            mpDecoder->pause();
                            mpEncoder->pause();
                        }
                        mpMuxer->pause();
                    }
                    //seek
                    seekRet = mpMuxer->seekTo(msec);
                    if(seekRet!=NO_ERROR)
                    {
                        ALOGE("(f:%s, l:%d) muxer seek to [%d]ms fail!", __FUNCTION__, __LINE__, msec);
                        mSeekResult = UNKNOWN_ERROR;
                    }
                    if(mResizeFlag)
                    {
                        seekRet = mpEncoder->seekTo(msec);
                        if(seekRet!=NO_ERROR)
                        {
                            ALOGE("(f:%s, l:%d) encoder seek to [%d]ms fail!", __FUNCTION__, __LINE__, msec);
                            mSeekResult = UNKNOWN_ERROR;
                        }
                        seekRet = mpDecoder->seekTo(msec);
                        if(seekRet!=NO_ERROR)
                        {
                            ALOGE("(f:%s, l:%d) decoder seek to [%d]ms fail!", __FUNCTION__, __LINE__, msec);
                            mSeekResult = UNKNOWN_ERROR;
                        }
                    }
                    seekRet = mpDemuxer->seekTo(msec);
                    if(seekRet!=NO_ERROR)
                    {
                        ALOGE("(f:%s, l:%d) demuxer seek to [%d]ms fail!", __FUNCTION__, __LINE__, msec);
                        mSeekResult = UNKNOWN_ERROR;
                    }
                    //resume.
                    if(curState != VIDEO_RESIZER_PAUSED)
                    {
                        ALOGD("(f:%s, l:%d) status[0x%x], start component after seek!", __FUNCTION__, __LINE__, curState);
                        mpDemuxer->start();
                        if(mResizeFlag)
                        {
                            mpDecoder->start();
                            mpEncoder->start();
                        }
                        mpMuxer->start();
                    }
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
                case VIDEO_RESIZER_MESSAGE_STOP:
                {
                    ALOGD("(f:%s, l:%d) VideoResizer::resizeThread() will exit!", __FUNCTION__, __LINE__);
                    {
                        Mutex::Autolock autoLock(mMessageQueueLock);
                        if(!mMessageQueue.empty())
                        {
                            ALOGE("(f:%s, l:%d) Be careful! mMessageQueue has [%d]messages not process when exit resizeThread, ignore and clear them.", __FUNCTION__, __LINE__, mMessageQueue.size());
                            mMessageQueue.clear();
                        }
                    }
                    return false;
                }
                case VR_EventBufferFlag:
                {
                    ALOGD("(f:%s, l:%d) receive eof? [0x%x]", __FUNCTION__, __LINE__, msg.arg1);
                    if(msg.arg1 == VRComp_TypeMuxer)
                    {
                        ALOGD("(f:%s, l:%d) status[0x%x], receive Muxer msg VR_EventBufferFlag, but videoResizer should not turn state, and not need to notify app!", __FUNCTION__, __LINE__, mStatus);
                        //Mutex::Autolock autoLock(mLock);
                        //mStatus = VIDEO_RESIZER_PLAYBACK_COMPLETE;
                        //mListener->notify(MEDIA_VIDEO_RESIZER_EVENT_PLAYBACK_COMPLETE, 0, 0);
                    }
                    else
                    {
                        ALOGD("(f:%s, l:%d) other component[0x%x] msg VR_EventBufferFlag, ignore!", __FUNCTION__, __LINE__, msg.arg1);
                    }
                    break;
                }
                case VR_EventBsAvailable:
                {
                    ALOGV("(f:%s, l:%d) send bsType[%d] Available", __FUNCTION__, __LINE__, msg.arg1);
                    mListener->notify(MEDIA_VIDEO_RESIZER_EVENT_INFO, MEDIA_VIDEO_RESIZER_INFO_BSFRAME_AVAILABLE, (MediaVideoResizerStreamType)msg.arg1);
                    break;
                }
                default:
                {
                    ALOGE("(f:%s, l:%d) unknown msg[%d]!", __FUNCTION__, __LINE__, msg.what);
                    break;
                }
            }
        }
    }
    
    ALOGE("(f:%s, l:%d) fatal error! impossible come here!", __FUNCTION__, __LINE__);
    return false;
}

void VideoResizer::resetSomeMembers()
{
    mResizeFlag = 0;
    mnSeekDone = 0;
    memset(&mPpsInfo, 0, sizeof(VencHeaderData));
    {
        Mutex::Autolock autoLock(mMessageQueueLock);
        if(!mMessageQueue.empty())
        {
            ALOGD("(f:%s, l:%d) mMessageQueueLock has [%d] messages not process!", __FUNCTION__, __LINE__, mMessageQueue.size());
            mMessageQueue.clear();
        }
    }
}

status_t VideoResizer::setFrameRate(int32_t framerate)
{
    if (framerate <= 0) {
        ALOGE("<F:%s, L:%d> Invalid framerate[%d]", __FUNCTION__, __LINE__, framerate);
        return BAD_VALUE;
    }
    Mutex::Autolock autoLock(mLock);
    mOutDest.frameRate = framerate * 1000;
    if (mpEncoder != NULL) {
        mpEncoder->setFrameRate(mOutDest.frameRate);
    }
    return NO_ERROR;
}

status_t VideoResizer::setBitRate(int32_t bitrate)
{
    if (bitrate <= 0) {
        ALOGE("<F:%s, L:%d> Invalid framerate[%d]", __FUNCTION__, __LINE__, bitrate);
        return BAD_VALUE;
    }
    Mutex::Autolock autoLock(mLock);
    mOutDest.bitRate = bitrate;
    if (mpEncoder != NULL) {
        mpEncoder->setBitRate(mOutDest.bitRate);
    }
    return NO_ERROR;
}
}; /* namespace android */
