//#define LOG_NDEBUG 0
#define LOG_TAG "CedarMediaVideoResizer"
#include <unistd.h>
#include <fcntl.h>
#include <utils/Log.h>
#include <binder/IMemory.h>
#include <binder/IServiceManager.h>

#include <CedarMediaVideoResizer.h>
#include <MediaCallbackDispatcher.h>

namespace android
{

void CedarMediaVideoResizer::EventHandler::handleMessage(const MediaCallbackMessage &msg)
{
    if (mCedarMR->mVideoResizer == NULL) {
        ALOGE("mediavideoresizer went away with unhandled events");
        return;
    }

    switch (msg.what) {
        case MEDIA_VIDEO_RESIZER_EVENT_PLAYBACK_COMPLETE:
            ALOGV("MEDIA_VIDEO_RESIZER_EVENT_PLAYBACK_COMPLETE");
            if (mCedarMR->mOnCompletionListener != NULL)
				mCedarMR->mOnCompletionListener->onCompletion(mCedarMR);
			return;
        case MEDIA_VIDEO_RESIZER_EVENT_SEEK_COMPLETE:
            ALOGV("MEDIA_VIDEO_RESIZER_EVENT_SEEK_COMPLETE");
            if (mCedarMR->mOnSeekCompleteListener != NULL)
                mCedarMR->mOnSeekCompleteListener->onSeekComplete(mCedarMR);
            return;
        case MEDIA_VIDEO_RESIZER_EVENT_INFO:
            ALOGV("MEDIA_VIDEO_RESIZER_EVENT_INFO");
            if (mCedarMR->mOnInfoListener != NULL) {
                mCedarMR->mOnInfoListener->onInfo(mCedarMR, msg.arg1, msg.arg2);
            }
            return;
        case MEDIA_VIDEO_RESIZER_EVENT_ERROR:
            ALOGV("MEDIA_RESIZER_ERROR");
            if (mCedarMR->mOnErrorListener != NULL) {
                mCedarMR->mOnErrorListener->onError(mCedarMR, msg.arg1, msg.arg2);
            }
            return;
        default:
            ALOGE("Unknown message type %d", msg.what);
            return;
    }
}

void CedarMediaVideoResizer::CedarMediaVideoResizerListener::notify(int msg, int ext1, int ext2)
{
	if ( mEventHandler != NULL) {
		MediaCallbackMessage message;
		message.what = msg;
		message.arg1 = ext1;
		message.arg2 = ext2;
		mEventHandler->post(message);
	}
}

CedarMediaVideoResizer::PacketBuffer::PacketBuffer(const sp<IMemory>& rawFrame)
    :mRawFrame(rawFrame)
{
    char *ptr_dst = (char *)mRawFrame->pointer();
    //VRPacketHeader pktHdr;
    //memcpy(&pktHdr, ptr_dst, sizeof(VRPacketHeader));
	memcpy(&mStreamType, ptr_dst, sizeof(MediaVideoResizerStreamType));
	ptr_dst += sizeof(MediaVideoResizerStreamType);
	memcpy(&mDataSize, ptr_dst, sizeof(int));
	ptr_dst += sizeof(int);
    memcpy(&mPts, ptr_dst, sizeof(long long));
    ptr_dst += sizeof(long long);
	mpData = ptr_dst;
}

CedarMediaVideoResizer::CedarMediaVideoResizer()
{
	ALOGV("constructor");
	mOnErrorListener = NULL;
	mOnInfoListener = NULL;

	mVideoResizer = new MediaVideoResizer();
	if (mVideoResizer == NULL) {
		ALOGE("Failed to new mVideoResizer");
		return;
	}
	mEventHandler = new EventHandler(this);
    if (mEventHandler == NULL) {
        ALOGE("Failed to new mEventHandler");
    }
	sp<CedarMediaVideoResizerListener> listener = new CedarMediaVideoResizerListener(mEventHandler);
    if (listener == NULL) {
        ALOGE("Failed to new listener");
    }
	mVideoResizer->setListener(listener);
}

CedarMediaVideoResizer::~CedarMediaVideoResizer()
{
	ALOGV("destructor");
}

status_t CedarMediaVideoResizer::setDataSource(const char *url)
{
    ALOGV("setDataSource(%s)", url);
    status_t opStatus;

    if (access(url, F_OK) == 0) {
		int fd = open(url, O_RDWR);
		if (fd < 0) {
			ALOGE("Failed to open file %s(%s)", url, strerror(errno));
			return UNKNOWN_ERROR;
		}
		opStatus = setDataSource(fd, 0, 0);
		close(fd);
	} else {
        Mutex::Autolock lock(mLock);
        if (url == NULL) {
            ALOGE("<F:%s, L:%d>url is NULL!!", __FUNCTION__, __LINE__);
            return BAD_VALUE;
        }
        if (mVideoResizer == NULL) {
            ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
            return NO_INIT;
        }
        opStatus = mVideoResizer->setDataSource(url);
	}
	return opStatus;
}

status_t CedarMediaVideoResizer::setDataSource(int fd, int64_t offset, int64_t length)
{
    ALOGV("setDataSource(%d, %lld, %lld)", fd, offset, length);
    Mutex::Autolock lock(mLock);
    if (fd < 0) {
        ALOGE("<F:%s, L:%d>invalid fd!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (mVideoResizer == NULL) {
        ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
    return mVideoResizer->setDataSource(fd, offset, length);
}

status_t CedarMediaVideoResizer::setVideoSize(int32_t width, int32_t height)
{
    ALOGV("setVideoSize(%d, %d)", width, height);
    Mutex::Autolock lock(mLock);
    if (mVideoResizer == NULL) {
        ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
    return mVideoResizer->setVideoSize(width, height);
}

status_t CedarMediaVideoResizer::setOutputPath(const char *url)
{
    ALOGV("setOutputPath(%s)", url);
    Mutex::Autolock lock(mLock);
    if (url == NULL) {
        ALOGE("<F:%s, L:%d>url is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (mVideoResizer == NULL) {
        ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
    return mVideoResizer->setOutputPath(url);
}

status_t CedarMediaVideoResizer::setFrameRate(int32_t framerate)
{
    ALOGV("setFrameRate(%d)", framerate);
    Mutex::Autolock lock(mLock);
    if (framerate <= 0) {
        ALOGE("<F:%s, L:%d>framerate<=0!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (mVideoResizer == NULL) {
        ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
    return mVideoResizer->setFrameRate(framerate);
}

status_t CedarMediaVideoResizer::setBitRate(int32_t bitrate)
{
    ALOGV("setBitRate(%d)", bitrate);
    Mutex::Autolock lock(mLock);
    if (bitrate <= 0) {
        ALOGE("<F:%s, L:%d>bitrate<=0!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (mVideoResizer == NULL) {
        ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
    return mVideoResizer->setBitRate(bitrate);
}

status_t CedarMediaVideoResizer::prepare()
{
    ALOGV("prepare");
    Mutex::Autolock lock(mLock);
    if (mVideoResizer == NULL) {
        ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
    return mVideoResizer->prepare();
}

status_t CedarMediaVideoResizer::start()
{
    ALOGV("start");
    Mutex::Autolock lock(mLock);
    if (mVideoResizer == NULL) {
        ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
    return mVideoResizer->start();
}

status_t CedarMediaVideoResizer::stop()
{
    ALOGV("stop");
    Mutex::Autolock lock(mLock);
    if (mVideoResizer == NULL) {
        ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
    return mVideoResizer->stop();
}

//seekTo() use aysnc mode. unit:ms
status_t CedarMediaVideoResizer::seekTo(int msec)
{
    ALOGV("seekTo");
    Mutex::Autolock lock(mLock);
    if (mVideoResizer == NULL) {
        ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
    return mVideoResizer->seekTo(msec);
}

status_t CedarMediaVideoResizer::reset()
{
    ALOGV("reset");
    Mutex::Autolock lock(mLock);
    if (mVideoResizer == NULL) {
        ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
    return mVideoResizer->reset();
}

status_t CedarMediaVideoResizer::release()
{
    ALOGV("release");
    Mutex::Autolock lock(mLock);
    if (mVideoResizer == NULL) {
        ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
    return mVideoResizer->release();
}

int CedarMediaVideoResizer::getDuration()
{
    if (mVideoResizer == NULL) {
        ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
    int msec;
    status_t ret = mVideoResizer->getDuration(&msec);
    if(ret!=OK)
    {
        ALOGE("(f:%s, l:%d) getDuration failed with return code (%d).", __FUNCTION__, __LINE__, ret);
        return -1;
    }
    ALOGV("getDuration: %d (msec)", msec);
    return msec;
}

sp<IMemory> CedarMediaVideoResizer::getEncDataHeader()
{
	if (mVideoResizer == NULL) {
        ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
        return NULL;
    }
	return mVideoResizer->getEncDataHeader();
}

void CedarMediaVideoResizer::setOnInfoListener(OnInfoListener *pListener)
{
    mOnInfoListener = pListener;
}

void CedarMediaVideoResizer::setOnErrorListener(OnErrorListener *pl)
{
    mOnErrorListener = pl;
}

void CedarMediaVideoResizer::setOnCompletionListener(OnCompletionListener *pListener)
{
    mOnCompletionListener = pListener;
}

void CedarMediaVideoResizer::setOnSeekCompleteListener(OnSeekCompleteListener *pListener)
{
    mOnSeekCompleteListener = pListener;
}

/*******************************************************************************
Function name: android.CedarMediaVideoResizer.getPacket
Description: 
    
Parameters: 
    
Return: 
    NO_ERROR: operation success. maybe get packet null.
    INVALID_OPERATION: call in wrong status.
    NOT_ENOUGH_DATA: data end.
Time: 2014/12/13
*******************************************************************************/
status_t CedarMediaVideoResizer::getPacket(sp<PacketBuffer> &pktBuf)
{
    ALOGV("getPacket");
    Mutex::Autolock lock(mLock);
    if (mVideoResizer == NULL) {
        ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
        return NULL;
    }
    status_t    ret;
    sp<IMemory> mem;
    pktBuf = NULL;
    ret = mVideoResizer->getPacket(mem);
    if(NO_ERROR == ret)
    {
        if(mem!=NULL)
        {
            pktBuf = new PacketBuffer(mem);
            //ALOGV("getOneBsFrame: stream_type=%d, data_size=%d, pts=%lld, data=%p\n",
	        //	pktBuf->mStreamType, pktBuf->mDataSize, pktBuf->mPts, pktBuf->mpData);
        }
    }
    else if(NOT_ENOUGH_DATA == ret)
    {
        ALOGE("<F:%s, L:%d> data end!", __FUNCTION__, __LINE__);
    }
	return ret;
}

} /* namespace android */

