//#define LOG_NDEBUG 0
#define LOG_TAG "MediaVideoResizer"
#include <utils/Log.h>
#include <media/mediavideoresizer.h>
#include <binder/IServiceManager.h>
#include <utils/String8.h>
#include <media/IMediaPlayerService.h>
#include <media/IMediaVideoResizer.h>
//#include <media/mediaplayer.h>  // for MEDIA_ERROR_SERVER_DIED


namespace android {

MediaVideoResizer::MediaVideoResizer()
{
    ALOGV("constructor");
    mVideoResizer = NULL;
    mListener = NULL;
    mCurrentState = MEDIA_VIDEO_RESIZER_ERROR;

    const sp<IMediaPlayerService> &service(getMediaPlayerService());
    if (service != NULL) {
        mVideoResizer = service->createMediaVideoResizer(getpid());
    }
    if (mVideoResizer != NULL) {
        mCurrentState = MEDIA_VIDEO_RESIZER_IDLE;
        mVideoResizer->setListener(this);
    }

    mCurrentPosition = -1;
    mSeekPosition = -1;
    mLockThreadId = 0;
}

MediaVideoResizer::~MediaVideoResizer()
{
    ALOGV("destructor");
    if (mVideoResizer != NULL) {
        mVideoResizer.clear();
    }
}

status_t MediaVideoResizer::setDataSource(const char *url)
{
    ALOGV("setDataSource(%s)", url);
    status_t ret;
    Mutex::Autolock lock(mLock);
    if (url == NULL) {
        ALOGE("<F:%s, L:%d>url is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (mVideoResizer == NULL) {
        ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
    if(mCurrentState != MEDIA_VIDEO_RESIZER_IDLE)
    {
        ALOGE("(f:%s, l:%d) setDataSource called in invalid state[0x%x]", __FUNCTION__, __LINE__, mCurrentState);
        return INVALID_OPERATION;
    }
    ret = mVideoResizer->setDataSource(url);
    if(ret!=NO_ERROR)
    {
        ALOGE("(f:%s, l:%d) setDataSource() failed with return code (%d)", __FUNCTION__, __LINE__, ret);
    }
    else
    {
        mCurrentState = MEDIA_VIDEO_RESIZER_INITIALIZED;
    }
    return ret;
}

status_t MediaVideoResizer::setDataSource(int fd, int64_t offset, int64_t length)
{
    ALOGV("setDataSource(%d, %lld, %lld)", fd, offset, length);
    status_t ret;
    Mutex::Autolock lock(mLock);
    if (fd < 0) {
        ALOGE("<F:%s, L:%d>invalid fd!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (mVideoResizer == NULL) {
        ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
    if(mCurrentState != MEDIA_VIDEO_RESIZER_IDLE)
    {
        ALOGE("(f:%s, l:%d) setDataSource called in invalid state[0x%x]", __FUNCTION__, __LINE__, mCurrentState);
        return INVALID_OPERATION;
    }
    ret = mVideoResizer->setDataSource(fd, offset, length);
    if(ret!=NO_ERROR)
    {
        ALOGE("(f:%s, l:%d) setDataSource() failed with return code (%d)", __FUNCTION__, __LINE__, ret);
    }
    else
    {
        mCurrentState = MEDIA_VIDEO_RESIZER_INITIALIZED;
    }
    return ret;
}

status_t MediaVideoResizer::setVideoSize(int32_t width, int32_t height)
{
    ALOGV("setVideoSize(%d, %d)", width, height);
    Mutex::Autolock lock(mLock);
    if (mVideoResizer == NULL) {
        ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
    if (mCurrentState != MEDIA_VIDEO_RESIZER_IDLE && mCurrentState != MEDIA_VIDEO_RESIZER_INITIALIZED && mCurrentState != MEDIA_VIDEO_RESIZER_STOPPED) 
    {
        ALOGE("(f:%s, l:%d) setVideoSize called in invalid state[0x%x]", __FUNCTION__, __LINE__, mCurrentState);
        return INVALID_OPERATION;
    }
    return mVideoResizer->setVideoSize(width, height);
}

status_t MediaVideoResizer::setOutputPath(const char *url)
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
    if (mCurrentState != MEDIA_VIDEO_RESIZER_IDLE && mCurrentState != MEDIA_VIDEO_RESIZER_INITIALIZED && mCurrentState != MEDIA_VIDEO_RESIZER_STOPPED) 
    {
        ALOGE("(f:%s, l:%d) setOutputPath called in invalid state[0x%x]", __FUNCTION__, __LINE__, mCurrentState);
        return INVALID_OPERATION;
    }
    return mVideoResizer->setOutputPath(url);
}

status_t MediaVideoResizer::setFrameRate(int32_t framerate)
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

status_t MediaVideoResizer::setBitRate(int32_t bitrate)
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

status_t MediaVideoResizer::prepare()
{
    ALOGV("prepare");
    status_t ret;
    Mutex::Autolock lock(mLock);
    if (mVideoResizer == NULL) {
        ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
    if(MEDIA_VIDEO_RESIZER_PREPARED == mCurrentState)
    {
        return NO_ERROR;
    }
    if (mCurrentState != MEDIA_VIDEO_RESIZER_INITIALIZED && mCurrentState != MEDIA_VIDEO_RESIZER_STOPPED) 
    {
        ALOGE("(f:%s, l:%d) prepare called in invalid state[0x%x]", __FUNCTION__, __LINE__, mCurrentState);
        return INVALID_OPERATION;
    }
    ret = mVideoResizer->prepare();
    if(ret!=NO_ERROR)
    {
        ALOGE("(f:%s, l:%d) prepare() failed with return code (%d)", __FUNCTION__, __LINE__, ret);
        mCurrentState = MEDIA_VIDEO_RESIZER_ERROR;
    }
    else
    {
        mCurrentState = MEDIA_VIDEO_RESIZER_PREPARED;
    }
    return ret;
}

status_t MediaVideoResizer::start()
{
    ALOGV("start");
    status_t ret;
    Mutex::Autolock lock(mLock);
    if (mVideoResizer == NULL) {
        ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
    if(MEDIA_VIDEO_RESIZER_STARTED == mCurrentState)
    {
        return NO_ERROR;
    }
    if (mCurrentState != MEDIA_VIDEO_RESIZER_PREPARED && mCurrentState != MEDIA_VIDEO_RESIZER_PAUSED && mCurrentState != MEDIA_VIDEO_RESIZER_COMPLETE)
    {
        ALOGE("(f:%s, l:%d) start called in invalid state[0x%x]", __FUNCTION__, __LINE__, mCurrentState);
        return INVALID_OPERATION;
    }
    ret = mVideoResizer->start();
    if(ret!=NO_ERROR)
    {
        ALOGE("(f:%s, l:%d) start() failed with return code (%d)", __FUNCTION__, __LINE__, ret);
        mCurrentState = MEDIA_VIDEO_RESIZER_ERROR;
    }
    else
    {
        mCurrentState = MEDIA_VIDEO_RESIZER_STARTED;
    }
    return ret;
}

status_t MediaVideoResizer::stop()
{
    ALOGV("stop");
    Mutex::Autolock lock(mLock);
    status_t ret;
    if (mVideoResizer == NULL) {
        ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
    if(MEDIA_VIDEO_RESIZER_STOPPED == mCurrentState)
    {
        return NO_ERROR;
    }
    if (mCurrentState != MEDIA_VIDEO_RESIZER_STARTED && mCurrentState != MEDIA_VIDEO_RESIZER_PREPARED 
        && mCurrentState != MEDIA_VIDEO_RESIZER_PAUSED && mCurrentState != MEDIA_VIDEO_RESIZER_COMPLETE)
    {
        ALOGE("(f:%s, l:%d) stop called in invalid state[0x%x]", __FUNCTION__, __LINE__, mCurrentState);
        return INVALID_OPERATION;
    }
    ret = mVideoResizer->stop();
    if(ret != NO_ERROR)
    {
        ALOGE("(f:%s, l:%d) stop() failed with return code (%d)", __FUNCTION__, __LINE__, ret);
        mCurrentState = MEDIA_VIDEO_RESIZER_ERROR;
    }
    else
    {
        mCurrentState = MEDIA_VIDEO_RESIZER_STOPPED;
    }
    return ret;
}

status_t MediaVideoResizer::pause()
{
    ALOGV("pause");
    status_t ret;
    Mutex::Autolock lock(mLock);
    if (mVideoResizer == NULL) {
        ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
    if(MEDIA_VIDEO_RESIZER_PAUSED == mCurrentState || MEDIA_VIDEO_RESIZER_COMPLETE == mCurrentState)
    {
        return NO_ERROR;
    }
    if (mCurrentState != MEDIA_VIDEO_RESIZER_STARTED)
    {
        ALOGE("(f:%s, l:%d) pause called in invalid state[0x%x]", __FUNCTION__, __LINE__, mCurrentState);
        return INVALID_OPERATION;
    }
    ret = mVideoResizer->pause();
    if(ret!=NO_ERROR)
    {
        ALOGE("(f:%s, l:%d) pause() failed with return code (%d)", __FUNCTION__, __LINE__, ret);
        mCurrentState = MEDIA_VIDEO_RESIZER_ERROR;
    }
    else
    {
        mCurrentState = MEDIA_VIDEO_RESIZER_PAUSED;
    }
    return ret;
}

status_t MediaVideoResizer::seekTo_l(int msec)
{
    if (mVideoResizer == NULL) {
        ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
    if ( mCurrentState == MEDIA_VIDEO_RESIZER_STARTED || mCurrentState == MEDIA_VIDEO_RESIZER_PREPARED 
        || mCurrentState == MEDIA_VIDEO_RESIZER_PAUSED ||  mCurrentState == MEDIA_VIDEO_RESIZER_COMPLETE )
    {
        if ( msec < 0 ) 
        {
            ALOGW("Attempt to seek to invalid position: %d", msec);
            msec = 0;
        }

        int durationMs;
        status_t err = mVideoResizer->getDuration(&durationMs);

        if (err != OK) 
        {
            ALOGW("Stream has no duration and is therefore not seekable.");
            return err;
        }

        if (msec > durationMs) 
        {
            ALOGW("Attempt to seek to past end of file: request = %d, "
                  "durationMs = %d",
                  msec,
                  durationMs);

            msec = durationMs;
        }

        // cache duration
        mCurrentPosition = msec;
        if (mSeekPosition < 0) 
        {
            mSeekPosition = msec;
            return mVideoResizer->seekTo(msec);
        }
        else 
        {
            ALOGV("Seek in progress - queue up seekTo[%d]", msec);
            return NO_ERROR;
        }
    }
    ALOGE("(f:%s, l:%d) Attempt to perform seekTo in wrong state: mCurrentState=%u", __FUNCTION__, __LINE__, mCurrentState);
    return INVALID_OPERATION;
}
status_t MediaVideoResizer::seekTo(int msec)
{
    ALOGV("seekTo %d", msec);
    mLockThreadId = getThreadId();
    Mutex::Autolock lock(mLock);
    status_t result = seekTo_l(msec);
    mLockThreadId = 0;
    return result;
}
status_t MediaVideoResizer::reset()
{
    ALOGV("reset");
    status_t ret;
    Mutex::Autolock lock(mLock);
    if (mVideoResizer == NULL) {
        ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
    if(MEDIA_VIDEO_RESIZER_IDLE == mCurrentState)
    {
        return NO_ERROR;
    }
    ret = mVideoResizer->reset();
    if(ret!=NO_ERROR)
    {
        ALOGE("(f:%s, l:%d) reset() failed with return code (%d)", __FUNCTION__, __LINE__, ret);
        mCurrentState = MEDIA_VIDEO_RESIZER_ERROR;
    }
    else
    {
        mCurrentState = MEDIA_VIDEO_RESIZER_IDLE;
    }
    mCurrentPosition = -1;
    mSeekPosition = -1;
    return ret;
}

// Release should be OK in any state
status_t MediaVideoResizer::release()
{
    //when call release(), then you must destroy MediaVideoResizer next!
    // mCurrentState means nothing after call release().
    ALOGV("release");
    Mutex::Autolock lock(mLock);
    if (mVideoResizer == NULL) {
        ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
    return mVideoResizer->release();
}

status_t MediaVideoResizer::getDuration(int *msec)
{
    ALOGV("getDuration");
    Mutex::Autolock lock(mLock);
    if (mVideoResizer == NULL) {
        ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
    bool isValidState = (mCurrentState==MEDIA_VIDEO_RESIZER_PREPARED 
                            || mCurrentState==MEDIA_VIDEO_RESIZER_STARTED 
                            || mCurrentState==MEDIA_VIDEO_RESIZER_PAUSED 
                            || mCurrentState==MEDIA_VIDEO_RESIZER_STOPPED 
                            || mCurrentState==MEDIA_VIDEO_RESIZER_COMPLETE);
    if (isValidState) 
    {
        int durationMs;
        status_t ret = mVideoResizer->getDuration(&durationMs);
        if (msec) 
        {
            *msec = durationMs;
        }
        return ret;
    }
    ALOGE("(f:%s, l:%d) Attempt to call getDuration without a valid state", __FUNCTION__, __LINE__);
    return INVALID_OPERATION;
}


sp<IMemory> MediaVideoResizer::getEncDataHeader()
{
    ALOGV("getEncDataHeader");
    Mutex::Autolock lock(mLock);
    if (mVideoResizer == NULL) {
        ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
        return NULL;
    }
    if (mCurrentState != MEDIA_VIDEO_RESIZER_STARTED && mCurrentState != MEDIA_VIDEO_RESIZER_PAUSED)
    {
        ALOGE("(f:%s, l:%d) called in invalid state[0x%x]", __FUNCTION__, __LINE__, mCurrentState);
        return NULL;
    }
    return mVideoResizer->getEncDataHeader();
}

status_t MediaVideoResizer::getPacket(sp<IMemory> &rawFrame)
{
    ALOGV("getPacket");
    Mutex::Autolock lock(mLock);
	if(mVideoResizer == NULL) {
		ALOGE("<F:%s, L:%d>video resizer not initialized!", __FUNCTION__, __LINE__);
		return NO_INIT;
	}
    if (mCurrentState != MEDIA_VIDEO_RESIZER_STARTED && mCurrentState != MEDIA_VIDEO_RESIZER_PAUSED && mCurrentState != MEDIA_VIDEO_RESIZER_COMPLETE)
    {
        ALOGE("(f:%s, l:%d) called in invalid state[0x%x]", __FUNCTION__, __LINE__, mCurrentState);
        return INVALID_OPERATION;
    }
	return mVideoResizer->getPacket(rawFrame);
}

status_t MediaVideoResizer::setListener(const sp<MediaVideoResizerListener>& listener)
{
    ALOGV("setListener");
    Mutex::Autolock lock(mLock);
    mListener = listener;
    return NO_ERROR;
}

void MediaVideoResizer::notify(int msg, int ext1, int ext2)
{
    ALOGV("message received msg=%d, ext1=%d, ext2=%d", msg, ext1, ext2);
    bool locked = false;
    if (mLockThreadId != getThreadId()) 
    {
        mLock.lock();
        locked = true;
    }
    else
    {
        ALOGD("(f:%s, l:%d) notify in same thread[%p], not lock!", __FUNCTION__, __LINE__, mLockThreadId);
    }
    switch(msg)
    {
        case MEDIA_VIDEO_RESIZER_EVENT_PLAYBACK_COMPLETE:
        {
            ALOGV("video resize complete");
            mCurrentState = MEDIA_VIDEO_RESIZER_COMPLETE;
            break;
        }
        case MEDIA_VIDEO_RESIZER_EVENT_SEEK_COMPLETE:
        {
            ALOGV("Received seek complete");
            if (mSeekPosition != mCurrentPosition)
            {
                ALOGV("Executing queued seekTo(%d)", mSeekPosition);
                mSeekPosition = -1;
                seekTo_l(mCurrentPosition);
            }
            else 
            {
                ALOGV("All seeks complete - return to regularly scheduled program");
                mCurrentPosition = mSeekPosition = -1;
            }
            break;
        }
        case MEDIA_VIDEO_RESIZER_EVENT_ERROR:
        {
            // Always log errors.
            // ext1: Media framework error code.
            // ext2: Implementation dependant error code.
            ALOGE("error (%d, %d)", ext1, ext2);
            mCurrentState = MEDIA_VIDEO_RESIZER_ERROR;
            break;
        }
        case MEDIA_VIDEO_RESIZER_EVENT_INFO:
        {
            // ext1: Media framework error code.
            // ext2: Implementation dependant error code.
            ALOGV("info/warning (%d, %d)", ext1, ext2);
            break;
        }
        default:
        {
            ALOGV("unrecognized message: (%d, %d, %d)", msg, ext1, ext2);
            break;
        }
    }
    sp<MediaVideoResizerListener> listener = mListener;
    if (locked) mLock.unlock();

    if(listener!=0)
    {
        Mutex::Autolock lock(mNotifyLock);
        listener->notify(msg, ext1, ext2);
    }
}

void MediaVideoResizer::died()
{
    ALOGV("died");
    notify(MEDIA_VIDEO_RESIZER_EVENT_ERROR, MEDIA_VIDEO_RESIZER_ERROR_DIED, 0);
}

}; /* namespace android */

