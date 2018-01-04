//#define LOG_NDEBUG 0
#define LOG_TAG "VideoResizerDemuxer"

#include <utils/Log.h>
//#include <cutils/log.h>
//#include <sys/types.h>
//#include <strings.h>
//#include <string.h>
//#include <stdlib.h>
//#include <unistd.h>
//#include <sys/stat.h>
//#include <fcntl.h>

//#include <CDX_MediaDefs.h>
#include <sys/sysinfo.h>

#include "VideoResizerDemuxer.h"
#include "VideoResizerMuxer.h"


namespace android
{
static CDX_MEDIA_FILE_FORMAT mediafile_format_detect(int fd)
{
    #if 0
    media_probe_data_t prob;
    //file format probe from here
	prob.buf = malloc(PROBE_BUFFER_MAX_SIZE);
	if (!prob.buf)
	{
		LOGE("malloc fail");
		return CDX_ERROR;
	}
	prob.buf_size = PROBE_BUFFER_MAX_SIZE;
	read(fd_probe, prob.buf, PROBE_BUFFER_MAX_SIZE);
	LOGV("prob buffer first N bytes: %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x",
			prob.buf[0],prob.buf[1],prob.buf[2],prob.buf[3],
			prob.buf[4],prob.buf[5],prob.buf[6],prob.buf[7]);

    
	file_fmt_type = mediafile_format_detect(mInSource.fd);
	free(prob.buf);
    #endif
    return CDX_MEDIA_FILE_FMT_MOV;
}

static CDX_MEDIA_FILE_FORMAT mediafile_format_detect(char *url)
{
    ALOGW("(f:%s, l:%d) not detect file[%s] now, guess mp4!", __FUNCTION__, __LINE__, url);
    return CDX_MEDIA_FILE_FMT_MOV;
}
    
VideoResizerDemuxer::DoDemuxThread::DoDemuxThread(VideoResizerDemuxer *pDemuxer)
    : Thread(false),
      mpDemuxer(pDemuxer),
      mThreadId(NULL)
{
}
void VideoResizerDemuxer::DoDemuxThread::startThread()
{
    run("ResizerDemuxThread", PRIORITY_DEFAULT);    //PRIORITY_DISPLAY
}
void VideoResizerDemuxer::DoDemuxThread::stopThread()
{
    requestExitAndWait();
}
status_t VideoResizerDemuxer::DoDemuxThread::readyToRun() 
{
    mThreadId = androidGetThreadId();

    return Thread::readyToRun();
}
bool VideoResizerDemuxer::DoDemuxThread::threadLoop()
{
    if(!exitPending())
    {
        return mpDemuxer->demuxThread();
    }
    else
    {
        return false;
    }
}
VideoResizerDemuxer::VideoResizerDemuxer()
{
    ALOGV("(f:%s, l:%d) construct", __FUNCTION__, __LINE__);
    mDemuxThread = NULL;
    memset(&mInSource, 0, sizeof(ResizerInputSource));
    mInSource.fd = -1;
    memset(&mDataSrcDesc, 0, sizeof(CedarXDataSourceDesc));
    mDataSrcDesc.ext_fd_desc.fd = -1;
    memset(&mCdxMediainfo, 0, sizeof(CedarXMediainfo));
    memset(&mCdxPkt, 0, sizeof(CedarXPacket));
    memset(&mSeekPara, 0, sizeof(CedarXSeekPara));
    mpCdxEpdkDmx = NULL;
    mDemuxMode = CDX_DEMUX_MODE_PULL;
    mEofFlag = false;
    mpOutChunk = NULL;
    mOutChunkSize = 0;
    mpOutVbs = new VencOutVbs();
    resetSomeMembers();
    mState = VRComp_StateLoaded;
}

VideoResizerDemuxer::~VideoResizerDemuxer()
{
    ALOGV("desconstruct");
    if(mState != VRComp_StateLoaded)
    {
        ALOGE("(f:%s, l:%d) fatal error! deconstruct in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
        reset();
    }
    if(mpOutChunk)
    {
        free(mpOutChunk);
        mpOutChunk = NULL;
    }
}

status_t VideoResizerDemuxer::setDataSource(const char *url)
{
    ALOGV("setDataSource, url[%s]", url);
    Mutex::Autolock autoLock(mLock);
    if (mState!=VRComp_StateLoaded) 
    {
        ALOGE("(f:%s, l:%d) call setDataSource in invalid state[0x%x]!", __FUNCTION__, __LINE__, mState);
        return INVALID_OPERATION;
    }
    if (mInSource.fd >= 0)
    {
        ALOGE("(f:%s, l:%d) fatal error! close fd[%d]", __FUNCTION__, __LINE__, mInSource.fd);
        ::close(mInSource.fd);
        mInSource.fd = -1;
    }
    if(mInSource.url)
    {
        ALOGE("(f:%s, l:%d) fatal error! free url[%s]", __FUNCTION__, __LINE__, mInSource.url);
        free(mInSource.url);
        mInSource.url = NULL;
    }
    mInSource.url = strdup(url);

    memset(&mDataSrcDesc, 0, sizeof(CedarXDataSourceDesc));
    memset(&mCdxMediainfo, 0, sizeof(CedarXMediainfo));
    memset(&mCdxPkt, 0, sizeof(CedarXPacket));
    memset(&mSeekPara, 0, sizeof(CedarXSeekPara));
    mDataSrcDesc.source_url = mInSource.url;
    mDataSrcDesc.stream_type = CEDARX_STREAM_LOCALFILE;
	mDataSrcDesc.source_type = CEDARX_SOURCE_FILEPATH;
    mDataSrcDesc.media_type = CEDARX_MEDIATYPE_NORMAL;
    mDataSrcDesc.demux_type = mediafile_format_detect(mDataSrcDesc.source_url);
    //open demuxer, get video and audio information.
    mpCdxEpdkDmx = cedarx_demux_create(mDataSrcDesc.demux_type);
    int ret = mpCdxEpdkDmx->open(mpCdxEpdkDmx, &mCdxMediainfo, &mDataSrcDesc);
	if(ret < 0)
	{
        ALOGE("(f:%s, l:%d) cedarX demuxer open error,ret[%d]", __FUNCTION__, __LINE__, ret);
		cedarx_demux_destory(mpCdxEpdkDmx);
        mpCdxEpdkDmx = NULL;
        free(mDataSrcDesc.source_url);
        mDataSrcDesc.source_url = NULL;
        return UNKNOWN_ERROR;
	}
    mDemuxMode = (CDX_DEMUX_MODE_E)mpCdxEpdkDmx->control(mpCdxEpdkDmx, CDX_DMX_CMD_GET_DEMUX_MODE, 0, NULL);
	if (mDemuxMode == CDX_DEMUX_MODE_PUSH)
	{
        ALOGE("(f:%s, l:%d) not support push mode of ts!", __FUNCTION__, __LINE__);
	}
    mEofFlag = false;
    if(mCdxMediainfo.nHasVideo)
    {
        ALOGV("demuxVideoInfo:\n"
            "cMIMEType[%p]\n"
            "nFrameWidth[%d]\n"
            "nFrameHeight[%d]\n"
            "nBitrate[%d]\n"
            "xFramerate[%d]\n"
            "nRotation[%d]\n"
            "eCompressionFormat[%d]\n"
            "eCompressionSubFormat[%d]\n"
            "eColorFormat[%d]\n"
            "nMicSecPerFrame[%d]\n"
            "nDemuxType[%d]\n",
            mCdxMediainfo.VidStrmList[0].cMIMEType,
            mCdxMediainfo.VidStrmList[0].nFrameWidth,
            mCdxMediainfo.VidStrmList[0].nFrameHeight,
            mCdxMediainfo.VidStrmList[0].nBitrate,
            mCdxMediainfo.VidStrmList[0].xFramerate,
            mCdxMediainfo.VidStrmList[0].nRotation,
            mCdxMediainfo.VidStrmList[0].eCompressionFormat,
            mCdxMediainfo.VidStrmList[0].eCompressionSubFormat,
            mCdxMediainfo.VidStrmList[0].eColorFormat,
            mCdxMediainfo.VidStrmList[0].nMicSecPerFrame,
            mCdxMediainfo.VidStrmList[0].nDemuxType
            );
    }
    mState = VRComp_StateIdle;
    return NO_ERROR;
}

status_t VideoResizerDemuxer::setDataSource(int fd, int64_t offset, int64_t length)
{
    ALOGV("setDataSource, fd(%d), offset(%lld), length(%lld)", fd, offset, length);
    Mutex::Autolock autoLock(mLock);
    if (mState!=VRComp_StateLoaded) 
    {
        ALOGE("(f:%s, l:%d) call setDataSource in invalid state[0x%x]!", __FUNCTION__, __LINE__, mState);
        return INVALID_OPERATION;
    }
    if (mInSource.fd >= 0)
    {
        ALOGD("<F:%s, L:%d>close fd[%d]", __FUNCTION__, __LINE__, mInSource.fd);
        ::close(mInSource.fd);
    }
    mInSource.fd = dup(fd);
    mInSource.offset = offset;
    mInSource.length = length;

    memset(&mDataSrcDesc, 0, sizeof(CedarXDataSourceDesc));
    memset(&mCdxMediainfo, 0, sizeof(CedarXMediainfo));
    memset(&mCdxPkt, 0, sizeof(CedarXPacket));
    memset(&mSeekPara, 0, sizeof(CedarXSeekPara));
    mDataSrcDesc.ext_fd_desc.fd = mInSource.fd;
    mDataSrcDesc.ext_fd_desc.offset = offset;
    mDataSrcDesc.ext_fd_desc.length = length;
    mDataSrcDesc.stream_type = CEDARX_STREAM_LOCALFILE;
	mDataSrcDesc.source_type = CEDARX_SOURCE_FD;
    mDataSrcDesc.media_type = CEDARX_MEDIATYPE_NORMAL;
    mDataSrcDesc.demux_type = mediafile_format_detect(mInSource.fd);
    //open demuxer, get video and audio information.
    mpCdxEpdkDmx = cedarx_demux_create(mDataSrcDesc.demux_type);
    int ret = mpCdxEpdkDmx->open(mpCdxEpdkDmx, &mCdxMediainfo, &mDataSrcDesc);
	if(ret < 0)
	{
        ALOGE("(f:%s, l:%d) cedarX demuxer open error,ret[%d]", __FUNCTION__, __LINE__, ret);
		cedarx_demux_destory(mpCdxEpdkDmx);
        mpCdxEpdkDmx = NULL;
        ::close(mInSource.fd);
        mInSource.fd = -1;
        return UNKNOWN_ERROR;
	}
    mDemuxMode = (CDX_DEMUX_MODE_E)mpCdxEpdkDmx->control(mpCdxEpdkDmx, CDX_DMX_CMD_GET_DEMUX_MODE, 0, NULL);
	if (mDemuxMode == CDX_DEMUX_MODE_PUSH)
	{
        ALOGE("(f:%s, l:%d) not support push mode of ts!", __FUNCTION__, __LINE__);
	}
    mEofFlag = false;
    if(mCdxMediainfo.nHasVideo)
    {
        ALOGV("demuxVideoInfo:\n"
            "cMIMEType[%p]\n"
            "nFrameWidth[%d]\n"
            "nFrameHeight[%d]\n"
            "nBitrate[%d]\n"
            "xFramerate[%d]\n"
            "nRotation[%d]\n"
            "eCompressionFormat[%d]\n"
            "eCompressionSubFormat[%d]\n"
            "eColorFormat[%d]\n"
            "nMicSecPerFrame[%d]\n"
            "nDemuxType[%d]\n",
            mCdxMediainfo.VidStrmList[0].cMIMEType,
            mCdxMediainfo.VidStrmList[0].nFrameWidth,
            mCdxMediainfo.VidStrmList[0].nFrameHeight,
            mCdxMediainfo.VidStrmList[0].nBitrate,
            mCdxMediainfo.VidStrmList[0].xFramerate,
            mCdxMediainfo.VidStrmList[0].nRotation,
            mCdxMediainfo.VidStrmList[0].eCompressionFormat,
            mCdxMediainfo.VidStrmList[0].eCompressionSubFormat,
            mCdxMediainfo.VidStrmList[0].eColorFormat,
            mCdxMediainfo.VidStrmList[0].nMicSecPerFrame,
            mCdxMediainfo.VidStrmList[0].nDemuxType
            );
    }
    mState = VRComp_StateIdle;
    return NO_ERROR;
}

/*******************************************************************************
Function name: android.VideoResizerDemuxer.start
Description: 
    // Idle,Pause->Executing
Parameters: 
    
Return: 
    
Time: 2014/12/2
*******************************************************************************/
status_t VideoResizerDemuxer::start()
{
    Mutex::Autolock locker(&mLock);
    if(VRComp_StateExecuting == mState)
    {
        return NO_ERROR;
    }
    if (mState != VRComp_StateIdle && mState != VRComp_StatePause)
    {
        ALOGE("(f:%s, l:%d) start called in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
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
        mDemuxThread = new DoDemuxThread(this);
        mDemuxThread->startThread();
        mState = VRComp_StateExecuting;
    }
    else if(VRComp_StatePause == mState)
    {
        //send executing msg
        {
            VideoResizerMessage msg;
            msg.what = VR_DEMUXER_MSG_EXECUTING;
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
Function name: android.VideoResizerDemuxer.stop
Description: 
    // Executing,Pause->Idle
Parameters: 
    
Return: 
    
Time: 2014/12/3
*******************************************************************************/
status_t VideoResizerDemuxer::stop_l()
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
    sp<DoDemuxThread> thread;
    {
        thread = mDemuxThread;
        mDemuxThread.clear();
    }
    if (thread == NULL) 
    {
        ALOGE("(f:%s, l:%d) DoDemuxThread is null?", __FUNCTION__, __LINE__);
    }
    else
    {
        //send stop msg
        {
            VideoResizerMessage msg;
            msg.what = VR_DEMUXER_MSG_STOP;
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
status_t VideoResizerDemuxer::stop()
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    Mutex::Autolock locker(&mLock);
    return stop_l();
}
/*******************************************************************************
Function name: android.VideoResizerDemuxer.pause
Description: 
    Executing->Pause
Parameters: 
    
Return: 
    
Time: 2014/12/3
*******************************************************************************/
status_t VideoResizerDemuxer::pause()
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
        msg.what = VR_DEMUXER_MSG_PAUSE;
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
Function name: android.VideoResizerDemuxer.reset
Description: 
    // Invalid,Idle,Executing,Pause->Loaded.
Parameters: 
    
Return: 
    
Time: 2014/12/3
*******************************************************************************/
status_t VideoResizerDemuxer::reset()
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
        if(mpCdxEpdkDmx)
        {
            for(int i = 0; i < MAX_AUDIO_STREAM_NUM; i++) {
                if(mCdxMediainfo.AudStrmList[i].extra_data && mCdxMediainfo.AudStrmList[i].extra_data_len != 0) {
                    free(mCdxMediainfo.AudStrmList[i].extra_data);
                    mCdxMediainfo.AudStrmList[i].extra_data = NULL;
                }
            }

            for(int i = 0; i < MAX_VIDEO_STREAM_NUM; i++) {
                if(mCdxMediainfo.VidStrmList[i].pCodecExtraData) {
                    free(mCdxMediainfo.VidStrmList[i].pCodecExtraData);
                    mCdxMediainfo.VidStrmList[i].pCodecExtraData = NULL;
                }
            }

            mpCdxEpdkDmx->close(mpCdxEpdkDmx);
    		cedarx_demux_destory(mpCdxEpdkDmx);
            mpCdxEpdkDmx = NULL;
        }
        if(mInSource.fd>=0)
        {
            close(mInSource.fd);
            mInSource.fd = -1;
        }
        if(mInSource.url)
        {
            free(mInSource.url);
            mInSource.url = NULL;
        }
        memset(&mInSource, 0, sizeof(ResizerInputSource));
        mInSource.fd = -1;
        resetSomeMembers();
        mState = VRComp_StateLoaded;
    }
    
    return NO_ERROR;
}
/*******************************************************************************
Function name: android.VideoResizerDemuxer.seekTo
Description: 
    //call in state Idle,Pause
Parameters: 
    
Return: 
    
Time: 2014/12/3
*******************************************************************************/
status_t VideoResizerDemuxer::seekTo(int msec)
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
        int seekRet;
        CedarXSeekPara seek_para;
    	seek_para.seek_time = msec;
    	seek_para.seek_relative_time = 0;
    	seek_para.seek_dir = 0;
        seekRet = mpCdxEpdkDmx->control(mpCdxEpdkDmx, CDX_DMX_CMD_MEDIAMODE_CONTRL, CDX_MEDIA_STATUS_JUMP, &seek_para);
        if(seekRet != CDX_OK)
        {
            ALOGE("(f:%s, l:%d) seek to [%d]ms fail[%d]!", __FUNCTION__, __LINE__, msec, seekRet);
            return UNKNOWN_ERROR;
        }
        else
        {
            mEofFlag = false;
            return NO_ERROR;
        }
    }
    else
    {
        //send to demuxThread.
        {
            VideoResizerMessage msg;
            msg.what = VR_DEMUXER_MSG_SEEK;
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
CDX_MEDIA_FILE_FORMAT VideoResizerDemuxer::getMediaFileFormat()
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    Mutex::Autolock lock(mLock);
    if (mState != VRComp_StateIdle && mState != VRComp_StateExecuting && mState != VRComp_StatePause)
    {
        ALOGE("(f:%s, l:%d) called in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
        return CDX_MEDIA_FILE_FMT_UNKOWN;
    }
    return (CDX_MEDIA_FILE_FORMAT)mDataSrcDesc.demux_type;
}
CedarXMediainfo* VideoResizerDemuxer::getMediainfo()
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    Mutex::Autolock lock(mLock);
    if (mState != VRComp_StateIdle && mState != VRComp_StateExecuting && mState != VRComp_StatePause)
    {
        ALOGE("(f:%s, l:%d) called in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
        return NULL;
    }
    return &mCdxMediainfo;
}
status_t VideoResizerDemuxer::setResizeFlag(bool flag)
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
status_t VideoResizerDemuxer::setDecoderComponent(VideoResizerDecoder *pDecoderComp)
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    Mutex::Autolock lock(mLock);
    if (mState != VRComp_StateIdle)
    {
        ALOGE("(f:%s, l:%d) called in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
        return INVALID_OPERATION;
    }
    mpDecoder = pDecoderComp;
    return NO_ERROR;
}
status_t VideoResizerDemuxer::setMuxerComponent(VideoResizerMuxer *pMuxerComp)
{
    ALOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    Mutex::Autolock lock(mLock);
    if (mState != VRComp_StateIdle)
    {
        ALOGE("(f:%s, l:%d) called in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
        return INVALID_OPERATION;
    }
    mpMuxer = pMuxerComp;
    return NO_ERROR;
}

status_t VideoResizerDemuxer::FillThisBuffer(OMX_BUFFERHEADERTYPE* pBuffer)
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
    if(VR_DemuxVideoOutputPortIndex == pBuffer->nOutputPortIndex)
    {
        Mutex::Autolock autoLock(mOutChunkLock);
        if(outVbs->mStatus!=VencOutVbs::OwnedByDownStream)
        {
            ALOGE("(f:%s, l:%d) fatal error! outVbs->mStatus[0x%x]!=VencOutVbs::OwnedByDownStream, check code!", __FUNCTION__, __LINE__, outVbs->mStatus);
        }
        VencOutVbs::BufInfo bufInfo;
        outVbs->getVbsBufInfo(&bufInfo);
        ALOGV("(f:%s, l:%d) returnVbs len0[%d]len1[%d], data0[%p]data1[%p], pts[%lld]", __FUNCTION__, __LINE__, 
            bufInfo.mDataSize0, bufInfo.mDataSize1, bufInfo.mpData0, bufInfo.mpData1, bufInfo.mDataPts);
        outVbs->mStatus = VencOutVbs::OwnedByUs;
        outVbs->clearVbsInfo();
        if(mpOutVbs!=NULL)
        {
            ALOGE("(f:%s, l:%d) fatal error! why mpOutVbs != NULL?", __FUNCTION__, __LINE__);
        }
        mpOutVbs = outVbs;
        if(mbOutChunkUnderflow)
        {
            ALOGV("(f:%s, l:%d) signal outVbs available!", __FUNCTION__, __LINE__);
            VideoResizerMessage msg;
            msg.what = VR_DEMUXER_MSG_OUTVBS_AVAILABLE;
            Mutex::Autolock autoLock(mMessageQueueLock);
            if(mMessageQueue.empty())
            {
                mMessageQueueChanged.signal();
            }
            mMessageQueue.push_back(msg);
        }
    }
    else
    {
        ALOGE("(f:%s, l:%d) fatal error! pBuffer->nOutputPortIndex[%d], demuxer not support audio now!", __FUNCTION__, __LINE__, pBuffer->nOutputPortIndex);
    }
    return NO_ERROR;
}
/*******************************************************************************
Function name: android.VideoResizerDemuxer.demuxThread
Description: 
    demuxThread only work in two state: VRComp_StatePause and VRComp_StateExecuting.
    when demuxThread work, state change must be processed in thread, except stop.
    
Parameters: 
    
Return: 
    
Time: 2014/12/3
*******************************************************************************/
bool VideoResizerDemuxer::demuxThread()
{
    bool bHasMessage;
    VideoResizerMessage msg;
    int nNeedReadFlag = 0;   //1:indicate prefetch done, but chunk data still not read to buffer.
    OMX_BUFFERHEADERTYPE omxBufferHeader;
    int prefetchRet;
    int readRet;
    int vdecRequestBufferRet;
    int muxerRequestBufferRet;
    int waitRet;
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
                case VR_DEMUXER_MSG_EXECUTING:
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
                case VR_DEMUXER_MSG_PAUSE:
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
                case VR_DEMUXER_MSG_SEEK:
                {
                    if(mState!=VRComp_StatePause)
                    {
                        ALOGE("(f:%s, l:%d) fatal error! call seek in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
                    }
                    //parser seek 
                    int seekRet;
                    CedarXSeekPara seek_para;
                	seek_para.seek_time = msg.arg1;
                	seek_para.seek_relative_time = 0;
                	seek_para.seek_dir = 0;
                    seekRet = mpCdxEpdkDmx->control(mpCdxEpdkDmx, CDX_DMX_CMD_MEDIAMODE_CONTRL, CDX_MEDIA_STATUS_JUMP, &seek_para);
                    if(seekRet != CDX_OK)
                    {
                        ALOGE("(f:%s, l:%d) seek to [%d]ms fail[%d]!", __FUNCTION__, __LINE__, seek_para.seek_time, seekRet);
                    }
                    mEofFlag =false;
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
                case VR_DEMUXER_MSG_STOP:
                {
                    ALOGD("(f:%s, l:%d) demuxThread() will exit!", __FUNCTION__, __LINE__);
                    if(mState!=VRComp_StatePause && mState!=VRComp_StateExecuting)
                    {
                        ALOGE("(f:%s, l:%d) fatal error! demuxThread is in invalid state[0x%x]", __FUNCTION__, __LINE__, mState);
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
                case VR_DEMUXER_MSG_OUTVBS_AVAILABLE:
                {
                    Mutex::Autolock autoLock(mOutChunkLock);
                    if(mbOutChunkUnderflow)
                    {
                        mbOutChunkUnderflow = false;
                    }
                    else
                    {
                        ALOGD("(f:%s, l:%d) state[0x%x], outChunkUnderflow[%d] already false", __FUNCTION__, __LINE__, mState, mbOutChunkUnderflow);
                    }
                    break;
                }
                default:
                {
                    ALOGE("(f:%s, l:%d) unknown msg[%d]!", __FUNCTION__, __LINE__, msg.what);
                    break;
                }
            }
        }
        if(true==mEofFlag || VRComp_StatePause==mState)
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

        _prefech_demuxer:
        if(0 == nNeedReadFlag)
        {
            ALOGV("(f:%s, l:%d) prefetch", __FUNCTION__, __LINE__);
            prefetchRet = mpCdxEpdkDmx->prefetch(mpCdxEpdkDmx, &mCdxPkt);
            ALOGV("(f:%s, l:%d) prefetch[%d]", __FUNCTION__, __LINE__, prefetchRet);
            if(prefetchRet==CDX_OK)
            {
                nNeedReadFlag = 1;
            }
            else
            {
                ALOGD("(f:%s, l:%d) prefetch chunk fail. Demuxer EOF found!", __FUNCTION__, __LINE__);
                mEofFlag = true;
                ALOGD("(f:%s, l:%d) notify EOF to next component!", __FUNCTION__, __LINE__);
                if(mResizeFlag)
                {
                    mpDecoder->SetConfig(VR_IndexConfigSetStreamEof, NULL);
                }
                else
                {
                    mpMuxer->SetConfig(VR_IndexConfigSetStreamEof, NULL);
                }
                goto _process_message;
            }
        }
        if(0 == nNeedReadFlag)
        {
            ALOGE("(f:%s, l:%d) fatal error! nNeedReadFlag must be 1 here!", __FUNCTION__, __LINE__);
        }
        //get video bsBuffer, skip audio and others.
        if(mCdxPkt.pkt_type == CDX_PacketVideo)
        {
            if(mCdxPkt.pkt_length<=0)
            {
                ALOGW("(f:%s, l:%d) pkt_length[%d]<=0,ignore!", __FUNCTION__, __LINE__, mCdxPkt.pkt_length);
                nNeedReadFlag = 0;
                goto _process_message;
            }
            if(true == mResizeFlag) //need decode and encode.
            {
                //omxBufferHeader.nOutputPortIndex = VR_DemuxVideoOutputPortIndex;
                omxBufferHeader.nTobeFillLen = mCdxPkt.pkt_length;
                _requestBufferAgain:
                vdecRequestBufferRet = mpDecoder->requstBuffer(&omxBufferHeader);
        		if (vdecRequestBufferRet != NO_ERROR)
        		{
                    ALOGV("(f:%s, l:%d) request buffer fail[%d], vbv buffer may full!!", __FUNCTION__, __LINE__, vdecRequestBufferRet);
                    Mutex::Autolock autoLock(mMessageQueueLock);
                    while(mMessageQueue.empty())
                    {
                        waitRet = mMessageQueueChanged.waitRelative(mMessageQueueLock, WaitRequestBufferTime);
                        if(TIMED_OUT == waitRet)
                        {
                            ALOGV("(f:%s, l:%d) wait msg timeout[0x%x]", __FUNCTION__, __LINE__, waitRet);
                            goto _requestBufferAgain;
                        }
                    }
                    goto _process_message;
        		}
                mCdxPkt.is_dummy_packet = 0;
                mCdxPkt.pkt_info.epdk_read_pkt_info.pkt_buf0  = omxBufferHeader.pBuffer;
                mCdxPkt.pkt_info.epdk_read_pkt_info.pkt_buf1  = omxBufferHeader.pBufferExtra;
                mCdxPkt.pkt_info.epdk_read_pkt_info.pkt_size0 = omxBufferHeader.nBufferLen;
                mCdxPkt.pkt_info.epdk_read_pkt_info.pkt_size1 = omxBufferHeader.nBufferExtraLen;
                //read chunk to decoder buffer
                if((readRet=mpCdxEpdkDmx->read(mpCdxEpdkDmx, &mCdxPkt)) != CDX_OK)
        		{
                    nNeedReadFlag = 0;  //read done.
        			ALOGD("(f:%s, l:%d) readChunk fail[%d]. Demuxer EOF found! perhaps file is not complete", __FUNCTION__, __LINE__, readRet);
                    mEofFlag = true;
                    ALOGD("(f:%s, l:%d) notify EOF to next component!", __FUNCTION__, __LINE__);
                    if(mpDecoder)
                    {
                        mpDecoder->SetConfig(VR_IndexConfigSetStreamEof, NULL);
                    }
                    if(mpMuxer)
                    {
                        mpMuxer->SetConfig(VR_IndexConfigSetStreamEof, NULL);
                    }
                    goto _process_message;
        		}
                //update decoder buffer.
                omxBufferHeader.video_stream_type = CDX_VIDEO_STREAM_MAJOR;
                omxBufferHeader.nCtrlBits     = mCdxPkt.ctrl_bits;
                omxBufferHeader.nFilledLen    = mCdxPkt.pkt_length;
                omxBufferHeader.nTimeStamp    = mCdxPkt.pkt_pts;
                omxBufferHeader.duration      = mCdxPkt.pkt_duration;
                mpDecoder->updateBuffer(&omxBufferHeader);
                nNeedReadFlag = 0;  //read done.
            }
            else    //directly copy chunk to videoFrameList.
            {
                //get outVbs.
                {
                    Mutex::Autolock autoLock(mOutChunkLock);
                    if(mpOutVbs==NULL)
                    {
                        if(true == mbOutChunkUnderflow)
                        {
                            ALOGD("(f:%s, l:%d) outChunkUnderflow already true", __FUNCTION__, __LINE__);
                        }
                        mbOutChunkUnderflow = true;
                        goto _wait_outVbs;
                    }
                    else
                    {
                        if(mbOutChunkUnderflow!=false)
                        {
                            ALOGD("(f:%s, l:%d) outChunkUnderflow is true, turn to false", __FUNCTION__, __LINE__);
                            mbOutChunkUnderflow = false;
                        }
                    }
                    //omxBufferHeader.nOutputPortIndex = VR_DemuxVideoOutputPortIndex;
                    omxBufferHeader.nTobeFillLen = mCdxPkt.pkt_length;
                    if(mpOutChunk)
                    {
                        if(mOutChunkSize < omxBufferHeader.nTobeFillLen)
                        {
                            free(mpOutChunk);
                            mpOutChunk = NULL;
                            mOutChunkSize = 0;
                        }
                    }
                    if(NULL == mpOutChunk)
                    {
                        mpOutChunk = malloc(omxBufferHeader.nTobeFillLen);
                        if(NULL == mpOutChunk)
                        {
                            ALOGE("(f:%s, l:%d) fatal error! malloc fail!", __FUNCTION__, __LINE__);
                            return false;
                        }
                        ALOGD("(f:%s, l:%d) remalloc [%d]bytes", __FUNCTION__, __LINE__, omxBufferHeader.nTobeFillLen);
                        mOutChunkSize = omxBufferHeader.nTobeFillLen;
                    }
                    
                    mCdxPkt.is_dummy_packet = 0;
                    mCdxPkt.pkt_info.epdk_read_pkt_info.pkt_buf0  = (unsigned char*)mpOutChunk;
                    mCdxPkt.pkt_info.epdk_read_pkt_info.pkt_buf1  = NULL;
                    mCdxPkt.pkt_info.epdk_read_pkt_info.pkt_size0 = omxBufferHeader.nTobeFillLen;
                    mCdxPkt.pkt_info.epdk_read_pkt_info.pkt_size1 = 0;
                    ALOGV("(f:%s, l:%d) pkt len[%d],pts[%lld]", __FUNCTION__, __LINE__, mCdxPkt.pkt_length, mCdxPkt.pkt_pts);
                    //read chunk to outVbsBuffer
                    if((readRet=mpCdxEpdkDmx->read(mpCdxEpdkDmx, &mCdxPkt)) != CDX_OK)
            		{
                        nNeedReadFlag = 0;  //read done.
            			ALOGD("(f:%s, l:%d) readChunk fail[%d]. Demuxer EOF found! perhaps file is not complete", __FUNCTION__, __LINE__, readRet);
                        mEofFlag = true;
                        ALOGD("(f:%s, l:%d) notify EOF to next component!", __FUNCTION__, __LINE__);
                        if(mpMuxer)
                        {
                            mpMuxer->SetConfig(VR_IndexConfigSetStreamEof, NULL);
                        }
                        goto _process_message;
            		}
                    //send outVbs
    //              omxBufferHeader.nCtrlBits 	= mCdxPkt.ctrl_bits;
    //				omxBufferHeader.nFilledLen 	= mCdxPkt.pkt_length;
    //				omxBufferHeader.nTimeStamp 	= mCdxPkt.pkt_pts;
    //				omxBufferHeader.duration    = mCdxPkt.pkt_duration;
                    mpOutVbs->setVbsInfo(mpOutChunk, mCdxPkt.pkt_length, NULL, 0, mCdxPkt.pkt_pts, 0);
                    mpOutVbs->mStatus = VencOutVbs::OwnedByDownStream;
                    omxBufferHeader.pBuffer = (OMX_U8*)&mpOutVbs;
                    omxBufferHeader.nOutputPortIndex = VR_DemuxVideoOutputPortIndex;
                    mpMuxer->EmptyThisBuffer(&omxBufferHeader);
                    mpOutVbs = NULL;
                    nNeedReadFlag = 0;  //read done.
                }
                _wait_outVbs:
                if(mbOutChunkUnderflow)
                {
                    Mutex::Autolock autoLock(mMessageQueueLock);
                    while(mMessageQueue.empty())
                    {
                        ALOGV("(f:%s, l:%d) outVbs==NULL, need wait", __FUNCTION__, __LINE__);
                        //waitRet = mMessageQueueChanged.waitRelative(mMessageQueueLock, WaitRequestBufferTime);
                        waitRet = mMessageQueueChanged.wait(mMessageQueueLock);
//                        if(TIMED_OUT == waitRet)
//                        {
//                            ALOGV("(f:%s, l:%d) wait msg timeout[0x%x]", __FUNCTION__, __LINE__, waitRet);
//                        }
                    }
                    goto _process_message;
                }
            }
        }
        else    //audio, subtitle, and others, skip now.
        {
            int skipRet;
            mCdxPkt.is_dummy_packet = 1;
            if ((skipRet = mpCdxEpdkDmx->read(mpCdxEpdkDmx, &mCdxPkt)) != CDX_OK)
        	{
                nNeedReadFlag = 0;  //read done.
                ALOGD("(f:%s, l:%d) readChunk fail[%d]. Demuxer EOF found! perhaps file is not complete", __FUNCTION__, __LINE__, skipRet);
                mEofFlag = true;
                ALOGD("(f:%s, l:%d) notify EOF to next component!", __FUNCTION__, __LINE__);
                if(mpDecoder)
                {
                    mpDecoder->SetConfig(VR_IndexConfigSetStreamEof, NULL);
                }
                if(mpMuxer)
                {
                    mpMuxer->SetConfig(VR_IndexConfigSetStreamEof, NULL);
                }
                goto _process_message;
            }
            nNeedReadFlag = 0;  //read done.
        }
    }
    ALOGE("(f:%s, l:%d) fatal error! impossible come here! exit demuxThread()", __FUNCTION__, __LINE__);
    return false;
}

void VideoResizerDemuxer::resetSomeMembers()
{
    mResizeFlag = 0;
    mpDecoder = NULL;
    mpMuxer = NULL;
    mEofFlag = false;
    mnExecutingDone = 0;
    mnPauseDone = 0;
    mnSeekDone = 0;
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

