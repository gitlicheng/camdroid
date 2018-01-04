/*
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

//#define LOG_NDEBUG 0
#define LOG_TAG "MediaPlayer"
#include <utils/Log.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>

//#include <gui/SurfaceTextureClient.h>

#include <media/mediaplayer.h>
#include <media/AudioSystem.h>

#include <binder/MemoryBase.h>

#include <utils/KeyedVector.h>
#include <utils/String8.h>

#include <system/audio.h>
#include <system/window.h>

namespace android {

MediaPlayer::MediaPlayer()
{
    ALOGV("constructor");
    mListener = NULL;
    mCookie = NULL;
    mStreamType = AUDIO_STREAM_MUSIC;
    mCurrentPosition = -1;
    mSeekPosition = -1;
    mCurrentState = MEDIA_PLAYER_IDLE;
    mPrepareSync = false;
    mPrepareStatus = NO_ERROR;
    mLoop = false;
    mLeftVolume = mRightVolume = 1.0;
    mVideoWidth = mVideoHeight = 0;
    mLockThreadId = 0;
    mAudioSessionId = AudioSystem::newAudioSessionId();
    AudioSystem::acquireAudioSessionId(mAudioSessionId);
    mSendLevel = 0;
    mRetransmitEndpointValid = false;
    /* add by Gary. start {{----------------------------------- */
    /* 2011-9-28 16:28:24 */
    /* save properties before creating the real player */
//    mSubGate = true;
//    mSubColor = 0xFFFFFFFF;
//    mSubFrameColor = 0xFF000000;
//    mSubPosition = 0;
    mSubDelay = 0;
//    mSubFontSize = 24;
    strcpy(mSubCharset, CHARSET_GBK);
	//mSubIndex = 0;
//    mTrackIndex = 0;
    mMuteMode = AUDIO_CHANNEL_MUTE_NONE;  // 2012-03-07, set audio channel mute
    /* add by Gary. end   -----------------------------------}} */
    
}

MediaPlayer::~MediaPlayer()
{
    ALOGV("destructor");
    AudioSystem::releaseAudioSessionId(mAudioSessionId);
    disconnect();
    IPCThreadState::self()->flushCommands();
}

void MediaPlayer::disconnect()
{
    ALOGV("disconnect");
    sp<IMediaPlayer> p;
    {
        Mutex::Autolock _l(mLock);
        p = mPlayer;
        mPlayer.clear();
    }

    if (p != 0) {
        p->disconnect();
    }
}

// always call with lock held
void MediaPlayer::clear_l()
{
    mCurrentPosition = -1;
    mSeekPosition = -1;
    mVideoWidth = mVideoHeight = 0;
    mRetransmitEndpointValid = false;
}

status_t MediaPlayer::setListener(const sp<MediaPlayerListener>& listener)
{
    ALOGV("setListener");
    Mutex::Autolock _l(mLock);
    mListener = listener;
    return NO_ERROR;
}


status_t MediaPlayer::attachNewPlayer(const sp<IMediaPlayer>& player)
{
    status_t err = UNKNOWN_ERROR;
    sp<IMediaPlayer> p;
    { // scope for the lock
        Mutex::Autolock _l(mLock);

        if ( !( (mCurrentState & MEDIA_PLAYER_IDLE) ||
                (mCurrentState == MEDIA_PLAYER_STATE_ERROR ) ) ) {
            ALOGE("attachNewPlayer called in state %d", mCurrentState);
            return INVALID_OPERATION;
        }

        clear_l();
        p = mPlayer;
        mPlayer = player;
        if (player != 0) {
            mCurrentState = MEDIA_PLAYER_INITIALIZED;
            err = NO_ERROR;
        } else {
            ALOGE("Unable to to create media player");
        }
    }

    if (p != 0) {
        p->disconnect();
    }

    return err;
}

status_t MediaPlayer::setDataSource(
        const char *url, const KeyedVector<String8, String8> *headers)
{
    ALOGV("setDataSource(%s)", url);
    status_t err = BAD_VALUE;
    if (url != NULL) {
        const sp<IMediaPlayerService>& service(getMediaPlayerService());
        if (service != 0) {
            sp<IMediaPlayer> player(service->create(getpid(), this, mAudioSessionId));
            if ((NO_ERROR != doSetRetransmitEndpoint(player)) ||
                (NO_ERROR != player->setDataSource(url, headers))) {
                player.clear();
            }
            /* add by Gary. start {{----------------------------------- */
            /* 2011-9-28 16:28:24 */
            /* save properties before creating the real player */
            if(player != 0) {
	    	    //player->setSubGate(mSubGate);
//            	player->setSubColor(mSubColor);
//            	player->setSubFrameColor(mSubFrameColor);
//            	player->setSubPosition(mSubPosition);
            	player->setSubDelay(mSubDelay);
//            	player->setSubFontSize(mSubFontSize);
            	player->setSubCharset(mSubCharset);
//            	player->switchSub(mSubIndex);
//            	player->switchTrack(mTrackIndex);
                player->setChannelMuteMode(mMuteMode); // 2012-03-07, set audio channel mute
	        }
            /* add by Gary. end   -----------------------------------}} */
            err = attachNewPlayer(player);
        }
    }
    return err;
}

status_t MediaPlayer::setDataSource(int fd, int64_t offset, int64_t length)
{
    ALOGV("setDataSource(%d, %lld, %lld)", fd, offset, length);
    status_t err = UNKNOWN_ERROR;
    const sp<IMediaPlayerService>& service(getMediaPlayerService());
    if (service != 0) {
        sp<IMediaPlayer> player(service->create(getpid(), this, mAudioSessionId));
        if ((NO_ERROR != doSetRetransmitEndpoint(player)) ||
            (NO_ERROR != player->setDataSource(fd, offset, length))) {
            player.clear();
        }
        err = attachNewPlayer(player);
    }
    return err;
}

status_t MediaPlayer::setDataSource(const sp<IStreamSource> &source)
{
    ALOGV("setDataSource");
    status_t err = UNKNOWN_ERROR;
    const sp<IMediaPlayerService>& service(getMediaPlayerService());
    if (service != 0) {
        sp<IMediaPlayer> player(service->create(getpid(), this, mAudioSessionId));
        if ((NO_ERROR != doSetRetransmitEndpoint(player)) ||
            (NO_ERROR != player->setDataSource(source))) {
            player.clear();
        }
        err = attachNewPlayer(player);
    }
    return err;
}

status_t MediaPlayer::invoke(const Parcel& request, Parcel *reply)
{
    Mutex::Autolock _l(mLock);
    const bool hasBeenInitialized =
            (mCurrentState != MEDIA_PLAYER_STATE_ERROR) &&
            ((mCurrentState & MEDIA_PLAYER_IDLE) != MEDIA_PLAYER_IDLE);
    if ((mPlayer != NULL) && hasBeenInitialized) {
        ALOGV("invoke %d", request.dataSize());
        return  mPlayer->invoke(request, reply);
    }
    ALOGE("invoke failed: wrong state %X", mCurrentState);
    return INVALID_OPERATION;
}

status_t MediaPlayer::setMetadataFilter(const Parcel& filter)
{
    ALOGD("setMetadataFilter");
    Mutex::Autolock lock(mLock);
    if (mPlayer == NULL) {
        return NO_INIT;
    }
    return mPlayer->setMetadataFilter(filter);
}

status_t MediaPlayer::getMetadata(bool update_only, bool apply_filter, Parcel *metadata)
{
    ALOGD("getMetadata");
    Mutex::Autolock lock(mLock);
    if (mPlayer == NULL) {
        return NO_INIT;
    }
    return mPlayer->getMetadata(update_only, apply_filter, metadata);
}

//status_t MediaPlayer::setVideoSurfaceTexture(
//        const sp<ISurfaceTexture>& surfaceTexture)
status_t MediaPlayer::setVideoSurfaceTexture(
        const unsigned int hlay)
{
    ALOGV("setVideoSurfaceTexture");
    Mutex::Autolock _l(mLock);
    if (mPlayer == 0) return NO_INIT;
    //return mPlayer->setVideoSurfaceTexture(surfaceTexture);
    return mPlayer->setVideoSurfaceTexture(hlay);
}

// must call with lock held
status_t MediaPlayer::prepareAsync_l()
{
    if ( (mPlayer != 0) && ( mCurrentState & ( MEDIA_PLAYER_INITIALIZED | MEDIA_PLAYER_STOPPED) ) ) {
        mPlayer->setAudioStreamType(mStreamType);
        mCurrentState = MEDIA_PLAYER_PREPARING;
        return mPlayer->prepareAsync();
    }
    ALOGE("prepareAsync called in state %d", mCurrentState);
    return INVALID_OPERATION;
}

// TODO: In case of error, prepareAsync provides the caller with 2 error codes,
// one defined in the Android framework and one provided by the implementation
// that generated the error. The sync version of prepare returns only 1 error
// code.
status_t MediaPlayer::prepare()
{
    ALOGV("prepare");
    Mutex::Autolock _l(mLock);
    mLockThreadId = getThreadId();
    if (mPrepareSync) {
        mLockThreadId = 0;
        return -EALREADY;
    }
    mPrepareSync = true;
    status_t ret = prepareAsync_l();
    if (ret != NO_ERROR) {
        mLockThreadId = 0;
        return ret;
    }

    if (mPrepareSync) {
        mSignal.wait(mLock);  // wait for prepare done
        mPrepareSync = false;
    }
    ALOGV("prepare complete - status=%d", mPrepareStatus);
    mLockThreadId = 0;
    return mPrepareStatus;
}

status_t MediaPlayer::prepareAsync()
{
    ALOGV("prepareAsync");
    Mutex::Autolock _l(mLock);
    return prepareAsync_l();
}

status_t MediaPlayer::start()
{
    ALOGV("start");
    Mutex::Autolock _l(mLock);
    if (mCurrentState & MEDIA_PLAYER_STARTED)
        return NO_ERROR;
    if ( (mPlayer != 0) && ( mCurrentState & ( MEDIA_PLAYER_PREPARED |
                    MEDIA_PLAYER_PLAYBACK_COMPLETE | MEDIA_PLAYER_PAUSED ) ) ) {
        mPlayer->setLooping(mLoop);
        mPlayer->setVolume(mLeftVolume, mRightVolume);
        mPlayer->setAuxEffectSendLevel(mSendLevel);
        mCurrentState = MEDIA_PLAYER_STARTED;
        status_t ret = mPlayer->start();
        if (ret != NO_ERROR) {
            mCurrentState = MEDIA_PLAYER_STATE_ERROR;
        } else {
            if (mCurrentState == MEDIA_PLAYER_PLAYBACK_COMPLETE) {
                ALOGV("playback completed immediately following start()");
            }
        }
        return ret;
    }
    ALOGE("start called in state %d", mCurrentState);
    return INVALID_OPERATION;
}

status_t MediaPlayer::stop()
{
    ALOGV("stop");
    Mutex::Autolock _l(mLock);
    if (mCurrentState & MEDIA_PLAYER_STOPPED) return NO_ERROR;
    if ( (mPlayer != 0) && ( mCurrentState & ( MEDIA_PLAYER_STARTED | MEDIA_PLAYER_PREPARED |
                    MEDIA_PLAYER_PAUSED | MEDIA_PLAYER_PLAYBACK_COMPLETE ) ) ) {
        status_t ret = mPlayer->stop();
        if (ret != NO_ERROR) {
            mCurrentState = MEDIA_PLAYER_STATE_ERROR;
        } else {
            mCurrentState = MEDIA_PLAYER_STOPPED;
        }
        return ret;
    }
    ALOGE("stop called in state %d", mCurrentState);
    return INVALID_OPERATION;
}

status_t MediaPlayer::pause()
{
    ALOGV("pause");
    Mutex::Autolock _l(mLock);
    if (mCurrentState & (MEDIA_PLAYER_PAUSED|MEDIA_PLAYER_PLAYBACK_COMPLETE))
        return NO_ERROR;
    if ((mPlayer != 0) && (mCurrentState & MEDIA_PLAYER_STARTED)) {
        status_t ret = mPlayer->pause();
        if (ret != NO_ERROR) {
            mCurrentState = MEDIA_PLAYER_STATE_ERROR;
        } else {
            mCurrentState = MEDIA_PLAYER_PAUSED;
        }
        return ret;
    }
    ALOGE("pause called in state %d", mCurrentState);
    return INVALID_OPERATION;
}

bool MediaPlayer::isPlaying()
{
    Mutex::Autolock _l(mLock);
    if (mPlayer != 0) {
        bool temp = false;
        mPlayer->isPlaying(&temp);
        ALOGV("isPlaying: %d", temp);
        if ((mCurrentState & MEDIA_PLAYER_STARTED) && ! temp) {
            ALOGE("internal/external state mismatch corrected");
            mCurrentState = MEDIA_PLAYER_PAUSED;
        }
        return temp;
    }
    ALOGV("isPlaying: no active player");
    return false;
}

status_t MediaPlayer::getVideoWidth(int *w)
{
    ALOGV("getVideoWidth");
    Mutex::Autolock _l(mLock);
    if (mPlayer == 0) return INVALID_OPERATION;
    *w = mVideoWidth;
    return NO_ERROR;
}

status_t MediaPlayer::getVideoHeight(int *h)
{
    ALOGV("getVideoHeight");
    Mutex::Autolock _l(mLock);
    if (mPlayer == 0) return INVALID_OPERATION;
    *h = mVideoHeight;
    return NO_ERROR;
}

status_t MediaPlayer::getCurrentPosition(int *msec)
{
    ALOGV("getCurrentPosition");
    Mutex::Autolock _l(mLock);
    if (mPlayer != 0) {
        if (mCurrentPosition >= 0) {
            ALOGV("Using cached seek position: %d", mCurrentPosition);
            *msec = mCurrentPosition;
            return NO_ERROR;
        }
        return mPlayer->getCurrentPosition(msec);
    }
    ALOGE("Player is not init!!");
    return INVALID_OPERATION;
}

status_t MediaPlayer::getDuration_l(int *msec)
{
    ALOGV("getDuration_l");
    bool isValidState = (mCurrentState & (MEDIA_PLAYER_PREPARED | MEDIA_PLAYER_STARTED | MEDIA_PLAYER_PAUSED | MEDIA_PLAYER_STOPPED | MEDIA_PLAYER_PLAYBACK_COMPLETE));
    if (mPlayer != 0 && isValidState) {
        int durationMs;
        status_t ret = mPlayer->getDuration(&durationMs);
        if (msec) {
            *msec = durationMs;
        }
        return ret;
    }
    ALOGE("Attempt to call getDuration without a valid mediaplayer, mCurrentState=%d, mPlayer is %s", mCurrentState, mPlayer==NULL?"NULL":"not NULL");
    return INVALID_OPERATION;
}

status_t MediaPlayer::getDuration(int *msec)
{
    Mutex::Autolock _l(mLock);
    return getDuration_l(msec);
}

status_t MediaPlayer::seekTo_l(int msec)
{
    ALOGV("seekTo %d", msec);
    if ((mPlayer != 0) && ( mCurrentState & ( MEDIA_PLAYER_STARTED | MEDIA_PLAYER_PREPARED | MEDIA_PLAYER_PAUSED |  MEDIA_PLAYER_PLAYBACK_COMPLETE) ) ) {
        if ( msec < 0 ) {
            ALOGW("Attempt to seek to invalid position: %d", msec);
            msec = 0;
        }

        int durationMs;
        status_t err = mPlayer->getDuration(&durationMs);

        if (err != OK) {
            ALOGW("Stream has no duration and is therefore not seekable.");
            return err;
        }

        if (msec > durationMs) {
            ALOGW("Attempt to seek to past end of file: request = %d, "
                  "durationMs = %d",
                  msec,
                  durationMs);

            msec = durationMs;
        }

        // cache duration
        mCurrentPosition = msec;
        if (mSeekPosition < 0) {
            mSeekPosition = msec;
            return mPlayer->seekTo(msec);
        }
        else {
            ALOGV("Seek in progress - queue up seekTo[%d]", msec);
            return NO_ERROR;
        }
    }
    ALOGE("Attempt to perform seekTo in wrong state: mPlayer=%p, mCurrentState=%u", mPlayer.get(), mCurrentState);
    return INVALID_OPERATION;
}

status_t MediaPlayer::seekTo(int msec)
{
    mLockThreadId = getThreadId();
    Mutex::Autolock _l(mLock);
    status_t result = seekTo_l(msec);
    mLockThreadId = 0;

    return result;
}

status_t MediaPlayer::reset_l()
{
    mLoop = false;
    if (mCurrentState == MEDIA_PLAYER_IDLE) return NO_ERROR;
    mPrepareSync = false;
    if (mPlayer != 0) {
        status_t ret = mPlayer->reset();
        if (ret != NO_ERROR) {
            ALOGE("reset() failed with return code (%d)", ret);
            mCurrentState = MEDIA_PLAYER_STATE_ERROR;
        } else {
            mCurrentState = MEDIA_PLAYER_IDLE;
        }
        // setDataSource has to be called again to create a
        // new mediaplayer.
        mPlayer = 0;
        return ret;
    }
    clear_l();
    
    return NO_ERROR;
}

status_t MediaPlayer::doSetRetransmitEndpoint(const sp<IMediaPlayer>& player) {
    Mutex::Autolock _l(mLock);

    if (player == NULL) {
        return UNKNOWN_ERROR;
    }

    if (mRetransmitEndpointValid) {
        return player->setRetransmitEndpoint(&mRetransmitEndpoint);
    }

    return OK;
}

status_t MediaPlayer::reset()
{
    ALOGV("reset");
    Mutex::Autolock _l(mLock);
    return reset_l();
}

status_t MediaPlayer::setAudioStreamType(audio_stream_type_t type)
{
    ALOGV("MediaPlayer::setAudioStreamType");
    Mutex::Autolock _l(mLock);
    if (mStreamType == type) return NO_ERROR;
    if (mCurrentState & ( MEDIA_PLAYER_PREPARED | MEDIA_PLAYER_STARTED |
                MEDIA_PLAYER_PAUSED | MEDIA_PLAYER_PLAYBACK_COMPLETE ) ) {
        // Can't change the stream type after prepare
        ALOGE("setAudioStream called in state %d", mCurrentState);
        return INVALID_OPERATION;
    }
    // cache
    mStreamType = type;
    return OK;
}

status_t MediaPlayer::setLooping(int loop)
{
    ALOGV("MediaPlayer::setLooping");
    Mutex::Autolock _l(mLock);
    mLoop = (loop != 0);
    if (mPlayer != 0) {
        return mPlayer->setLooping(loop);
    }
    return OK;
}

bool MediaPlayer::isLooping() {
    ALOGV("isLooping");
    Mutex::Autolock _l(mLock);
    if (mPlayer != 0) {
        return mLoop;
    }
    ALOGV("isLooping: no active player");
    return false;
}

status_t MediaPlayer::setVolume(float leftVolume, float rightVolume)
{
    ALOGV("MediaPlayer::setVolume(%f, %f)", leftVolume, rightVolume);
    Mutex::Autolock _l(mLock);
    mLeftVolume = leftVolume;
    mRightVolume = rightVolume;
    if (mPlayer != 0) {
        return mPlayer->setVolume(leftVolume, rightVolume);
    }
    return OK;
}

status_t MediaPlayer::setAudioSessionId(int sessionId)
{
    ALOGV("MediaPlayer::setAudioSessionId(%d)", sessionId);
    Mutex::Autolock _l(mLock);
    if (!(mCurrentState & MEDIA_PLAYER_IDLE)) {
        ALOGE("setAudioSessionId called in state %d", mCurrentState);
        return INVALID_OPERATION;
    }
    if (sessionId < 0) {
        return BAD_VALUE;
    }
    if (sessionId != mAudioSessionId) {
        AudioSystem::releaseAudioSessionId(mAudioSessionId);
        AudioSystem::acquireAudioSessionId(sessionId);
        mAudioSessionId = sessionId;
    }
    return NO_ERROR;
}

int MediaPlayer::getAudioSessionId()
{
    Mutex::Autolock _l(mLock);
    return mAudioSessionId;
}

status_t MediaPlayer::setAuxEffectSendLevel(float level)
{
    ALOGV("MediaPlayer::setAuxEffectSendLevel(%f)", level);
    Mutex::Autolock _l(mLock);
    mSendLevel = level;
    if (mPlayer != 0) {
        return mPlayer->setAuxEffectSendLevel(level);
    }
    return OK;
}

status_t MediaPlayer::attachAuxEffect(int effectId)
{
    ALOGV("MediaPlayer::attachAuxEffect(%d)", effectId);
    Mutex::Autolock _l(mLock);
    if (mPlayer == 0 ||
        (mCurrentState & MEDIA_PLAYER_IDLE) ||
        (mCurrentState == MEDIA_PLAYER_STATE_ERROR )) {
        ALOGE("attachAuxEffect called in state %d", mCurrentState);
        return INVALID_OPERATION;
    }

    return mPlayer->attachAuxEffect(effectId);
}

status_t MediaPlayer::setParameter(int key, const Parcel& request)
{
    ALOGV("MediaPlayer::setParameter(%d)", key);
    Mutex::Autolock _l(mLock);
    if (mPlayer != NULL) {
        return  mPlayer->setParameter(key, request);
    }
    ALOGV("setParameter: no active player");
    return INVALID_OPERATION;
}

status_t MediaPlayer::getParameter(int key, Parcel *reply)
{
    ALOGV("MediaPlayer::getParameter(%d)", key);
    Mutex::Autolock _l(mLock);
    if (mPlayer != NULL) {
        return  mPlayer->getParameter(key, reply);
    }
    ALOGV("getParameter: no active player");
    return INVALID_OPERATION;
}

status_t MediaPlayer::setRetransmitEndpoint(const char* addrString,
                                            uint16_t port) {
    ALOGV("MediaPlayer::setRetransmitEndpoint(%s:%hu)",
            addrString ? addrString : "(null)", port);

    Mutex::Autolock _l(mLock);
    if ((mPlayer != NULL) || (mCurrentState != MEDIA_PLAYER_IDLE))
        return INVALID_OPERATION;

    if (NULL == addrString) {
        mRetransmitEndpointValid = false;
        return OK;
    }

    struct in_addr saddr;
    if(!inet_aton(addrString, &saddr)) {
        return BAD_VALUE;
    }

    memset(&mRetransmitEndpoint, 0, sizeof(&mRetransmitEndpoint));
    mRetransmitEndpoint.sin_family = AF_INET;
    mRetransmitEndpoint.sin_addr   = saddr;
    mRetransmitEndpoint.sin_port   = htons(port);
    mRetransmitEndpointValid       = true;

    return OK;
}

void MediaPlayer::notify(int msg, int ext1, int ext2, const Parcel *obj)
{
    ALOGV("message received msg=%d, ext1=%d, ext2=%d", msg, ext1, ext2);
    bool send = true;
    bool locked = false;

    // TODO: In the future, we might be on the same thread if the app is
    // running in the same process as the media server. In that case,
    // this will deadlock.
    //
    // The threadId hack below works around this for the care of prepare
    // and seekTo within the same process.
    // FIXME: Remember, this is a hack, it's not even a hack that is applied
    // consistently for all use-cases, this needs to be revisited.
    if (mLockThreadId != getThreadId()) {
        mLock.lock();
        locked = true;
    }

    // Allows calls from JNI in idle state to notify errors
    if (!(msg == MEDIA_ERROR && mCurrentState == MEDIA_PLAYER_IDLE) && mPlayer == 0) {
        ALOGV("notify(%d, %d, %d) callback on disconnected mediaplayer", msg, ext1, ext2);
        if (locked) mLock.unlock();   // release the lock when done.
        return;
    }

    switch (msg) {
    case MEDIA_NOP: // interface test message
        break;
    case MEDIA_PREPARED:
        ALOGV("prepared");
        mCurrentState = MEDIA_PLAYER_PREPARED;
        if (mPrepareSync) {
            ALOGV("signal application thread");
            mPrepareSync = false;
            mPrepareStatus = NO_ERROR;
            mSignal.signal();
        }
        break;
    case MEDIA_PLAYBACK_COMPLETE:
        ALOGV("playback complete");
        if (mCurrentState == MEDIA_PLAYER_IDLE) {
            ALOGE("playback complete in idle state");
        }
        if (!mLoop) {
            mCurrentState = MEDIA_PLAYER_PLAYBACK_COMPLETE;
        }
        break;
    case MEDIA_ERROR:
        // Always log errors.
        // ext1: Media framework error code.
        // ext2: Implementation dependant error code.
        ALOGE("error (%d, %d)", ext1, ext2);
        mCurrentState = MEDIA_PLAYER_STATE_ERROR;
        if (mPrepareSync)
        {
            ALOGV("signal application thread");
            mPrepareSync = false;
            mPrepareStatus = ext1;
            mSignal.signal();
            send = false;
        }
        break;
    case MEDIA_INFO:
        // ext1: Media framework error code.
        // ext2: Implementation dependant error code.
        if (ext1 != MEDIA_INFO_VIDEO_TRACK_LAGGING) {
            ALOGW("info/warning (%d, %d)", ext1, ext2);
        }
        break;
    case MEDIA_SEEK_COMPLETE:
        ALOGV("Received seek complete");
        if (mSeekPosition != mCurrentPosition) {
            ALOGV("Executing queued seekTo(%d)", mSeekPosition);
            mSeekPosition = -1;
            seekTo_l(mCurrentPosition);
        }
        else {
            ALOGV("All seeks complete - return to regularly scheduled program");
            mCurrentPosition = mSeekPosition = -1;
        }
        break;
    case MEDIA_BUFFERING_UPDATE:
        ALOGV("buffering %d", ext1);
        break;
    case MEDIA_SET_VIDEO_SIZE:
        ALOGV("New video size %d x %d", ext1, ext2);
        mVideoWidth = ext1;
        mVideoHeight = ext2;
        break;
    case MEDIA_TIMED_TEXT:
        ALOGV("Received timed text message");
        break;
    default:
        ALOGV("unrecognized message: (%d, %d, %d)", msg, ext1, ext2);
        break;
    }

    sp<MediaPlayerListener> listener = mListener;
    if (locked) mLock.unlock();

    // this prevents re-entrant calls into client code
    if ((listener != 0) && send) {
        Mutex::Autolock _l(mNotifyLock);
        ALOGV("callback application");
        listener->notify(msg, ext1, ext2, obj);
        ALOGV("back from callback");
    }
}

/*static*/ sp<IMemory> MediaPlayer::decode(const char* url, uint32_t *pSampleRate, int* pNumChannels, audio_format_t* pFormat)
{
    ALOGV("decode(%s)", url);
    sp<IMemory> p;
    const sp<IMediaPlayerService>& service = getMediaPlayerService();
    if (service != 0) {
        p = service->decode(url, pSampleRate, pNumChannels, pFormat);
    } else {
        ALOGE("Unable to locate media service");
    }
    return p;

}

void MediaPlayer::died()
{
    ALOGV("died");
    notify(MEDIA_ERROR, MEDIA_ERROR_SERVER_DIED, 0);
}

/*static*/ sp<IMemory> MediaPlayer::decode(int fd, int64_t offset, int64_t length, uint32_t *pSampleRate, int* pNumChannels, audio_format_t* pFormat)
{
    ALOGV("decode(%d, %lld, %lld)", fd, offset, length);
    sp<IMemory> p;
    const sp<IMediaPlayerService>& service = getMediaPlayerService();
    if (service != 0) {
        p = service->decode(fd, offset, length, pSampleRate, pNumChannels, pFormat);
    } else {
        ALOGE("Unable to locate media service");
    }
    return p;

}

status_t MediaPlayer::setNextMediaPlayer(const sp<MediaPlayer>& next) {
    if (mPlayer == NULL) {
        return NO_INIT;
    }
    return mPlayer->setNextPlayer(next == NULL ? NULL : next->mPlayer);
}

/* add by Gary.  {{----------------------------------- */
//status_t MediaPlayer::setScreen(int screen)
//{
//    ALOGV("setScreen");
//    const sp<IMediaPlayerService>& service(getMediaPlayerService());
//    if (service != 0) {
//        return service->setScreen(screen);
//    }else {
//        return BAD_VALUE;
//    }
//}
//
//status_t MediaPlayer::getScreen(int *screen)
//{
//    ALOGV("getScreen");
//    const sp<IMediaPlayerService>& service(getMediaPlayerService());
//    if (service != 0) {
//        return service->getScreen(screen);
//    }else {
//        return BAD_VALUE;
//    }
//}

status_t MediaPlayer::isPlayingVideo(bool *playing)
{
    ALOGV("isPlayingVideo");
    int b;
    int r;
    
    const sp<IMediaPlayerService>& service(getMediaPlayerService());
    if (service != 0) {
        r = service->isPlayingVideo(&b);
        if(b)
            *playing = true;
        else
            *playing = false;
        
        return r;
    }else {
        return BAD_VALUE;
    }
}
/* add by Gary. end   -----------------------------------}} */

/* add by Gary. start {{----------------------------------- */
/* 2011-9-14 14:27:12 */
/* expend interfaces about subtitle, track and so on */
//int MediaPlayer::getSubCount()
//{
//    Mutex::Autolock lock(mLock);
//    if (mPlayer == NULL) {
//        return -1;
//    }
//    return mPlayer->getSubCount();
//}
//
//int MediaPlayer::getSubList(MediaPlayer_SubInfo *infoList, int count)
//{
//    Mutex::Autolock lock(mLock);
//    if (mPlayer == NULL) {
//        return -1;
//    }
//    return mPlayer->getSubList(infoList, count);
//}
//
//int MediaPlayer::getCurSub()
//{
//    Mutex::Autolock lock(mLock);
//    if (mPlayer == NULL) {
//        return -1;
//    }
//    return mPlayer->getCurSub();
//}
//
//
//status_t MediaPlayer::switchSub(int index)
//{
//    Mutex::Autolock lock(mLock);
//    mSubIndex = index;
//    if (mPlayer == NULL) {
//        return NO_INIT;
//    }
//    return mPlayer->switchSub(index);
//}
//
//
//status_t MediaPlayer::setSubGate(bool showSub)
//{
//    Mutex::Autolock lock(mLock);
//    mSubGate = showSub;
//    if (mPlayer == NULL) {
//        return OK;
//    }
//    return mPlayer->setSubGate(showSub);
//}
//
//
//bool MediaPlayer::getSubGate()
//{
//    Mutex::Autolock lock(mLock);
//    if (mPlayer == NULL) {
//        return false;
//    }
//    return mPlayer->getSubGate();
//}
//
//
//status_t MediaPlayer::setSubColor(int color)
//{
//    Mutex::Autolock lock(mLock);
//    mSubColor = color;
//    if (mPlayer == NULL) {
//        return OK;
//    }
//    return mPlayer->setSubColor(color);
//}
//
//
//int MediaPlayer::getSubColor()
//{
//    Mutex::Autolock lock(mLock);
//    if (mPlayer == NULL) {
//        return 0xFFFFFFFF;
//    }
//    return mPlayer->getSubColor();
//}
//
//
//status_t MediaPlayer::setSubFrameColor(int color)
//{
//    Mutex::Autolock lock(mLock);
//    mSubFrameColor = color;
//    if (mPlayer == NULL) {
//        return OK;
//    }
//    return mPlayer->setSubFrameColor(color);
//}
//
//
//int MediaPlayer::getSubFrameColor()
//{
//    Mutex::Autolock lock(mLock);
//    if (mPlayer == NULL) {
//        return 0xFFFFFFFF;
//    }
//    return mPlayer->getSubFrameColor();
//}
//
//
//status_t MediaPlayer::setSubFontSize(int size)
//{
//    Mutex::Autolock lock(mLock);
//    mSubFontSize = size;
//    if (mPlayer == NULL) {
//        return OK;
//    }
//    return mPlayer->setSubFontSize(size);
//}
//
//
//int MediaPlayer::getSubFontSize()
//{
//    Mutex::Autolock lock(mLock);
//    if (mPlayer == NULL) {
//        return -1;
//    }
//    return mPlayer->getSubFontSize();
//}


status_t MediaPlayer::setSubCharset(const char *charset)
{
    Mutex::Autolock lock(mLock);
    strcpy(mSubCharset, charset);
    if (mPlayer == NULL) {
        return OK;
    }
    return mPlayer->setSubCharset(charset);
}


status_t MediaPlayer::getSubCharset(char *charset)
{
    Mutex::Autolock lock(mLock);
    if (mPlayer == NULL) {
        return NO_INIT;
    }
    return mPlayer->getSubCharset(charset);
}


//status_t MediaPlayer::setSubPosition(int percent)
//{
//    Mutex::Autolock lock(mLock);
//    mSubPosition = percent;
//    if (mPlayer == NULL) {
//        return OK;
//    }
//    return mPlayer->setSubPosition(percent);
//}
//
//
//int MediaPlayer::getSubPosition()
//{
//    Mutex::Autolock lock(mLock);
//    if (mPlayer == NULL) {
//        return NO_INIT;
//    }
//    return mPlayer->getSubPosition();
//}


status_t MediaPlayer::setSubDelay(int time)
{
    Mutex::Autolock lock(mLock);
    mSubDelay = time;
    if (mPlayer == NULL) {
        return OK;
    }
    return mPlayer->setSubDelay(time);
}


int MediaPlayer::getSubDelay()
{
    Mutex::Autolock lock(mLock);
    if (mPlayer == NULL) {
        return -1;
    }
    return mPlayer->getSubDelay();
}


//int MediaPlayer::getTrackCount()
//{
//    Mutex::Autolock lock(mLock);
//    if (mPlayer == NULL) {
//        return -1;
//    }
//    return mPlayer->getTrackCount();
//}
//
//
//int MediaPlayer::getTrackList(MediaPlayer_TrackInfo *infoList, int count)
//{
//    Mutex::Autolock lock(mLock);
//    if (mPlayer == NULL) {
//        return -1;
//    }
//    return mPlayer->getTrackList((MediaPlayer_TrackInfo *)infoList, count);
//}
//
//
//int MediaPlayer::getCurTrack()
//{
//    Mutex::Autolock lock(mLock);
//    if (mPlayer == NULL) {
//        return -1;
//    }
//    return mPlayer->getCurTrack();
//}
//
//
//status_t MediaPlayer::switchTrack(int index)
//{
//    Mutex::Autolock lock(mLock);
//    mTrackIndex = index;
//    if (mPlayer == NULL) {
//        return NO_INIT;
//    }
//    return mPlayer->switchTrack(index);
//}
//
//
//status_t MediaPlayer::setInputDimensionType(int type)
//{
//    Mutex::Autolock lock(mLock);
//    if (mPlayer == NULL) {
//        return NO_INIT;
//    }
//    return mPlayer->setInputDimensionType(type);
//}
//
//
//int MediaPlayer::getInputDimensionType()
//{
//    Mutex::Autolock lock(mLock);
//    if (mPlayer == NULL) {
//        return -1;
//    }
//    return mPlayer->getInputDimensionType();
//}
//
//
//status_t MediaPlayer::setOutputDimensionType(int type)
//{
//    Mutex::Autolock lock(mLock);
//    if (mPlayer == NULL) {
//        return NO_INIT;
//    }
//    return mPlayer->setOutputDimensionType(type);
//}
//
//
//int MediaPlayer::getOutputDimensionType()
//{
//    Mutex::Autolock lock(mLock);
//    if (mPlayer == NULL) {
//        return -1;
//    }
//    return mPlayer->getOutputDimensionType();
//}
//
//
//status_t MediaPlayer::setAnaglaghType(int type)
//{
//    Mutex::Autolock lock(mLock);
//    if (mPlayer == NULL) {
//        return NO_INIT;
//    }
//    return mPlayer->setAnaglaghType(type);
//}
//
//
//int MediaPlayer::getAnaglaghType()
//{
//    Mutex::Autolock lock(mLock);
//    if (mPlayer == NULL) {
//        return -1;
//    }
//    return mPlayer->getAnaglaghType();
//}
//
//
//status_t MediaPlayer::getVideoEncode(char *encode)
//{
//    Mutex::Autolock lock(mLock);
//    if (mPlayer == NULL) {
//        return -1;
//    }
//    return mPlayer->getVideoEncode(encode);
//}
//
//
//int MediaPlayer::getVideoFrameRate()
//{
//    Mutex::Autolock lock(mLock);
//    if (mPlayer == NULL) {
//        return -1;
//    }
//    return mPlayer->getVideoFrameRate();
//}
//
//
//status_t MediaPlayer::getAudioEncode(char *encode)
//{
//    Mutex::Autolock lock(mLock);
//    if (mPlayer == NULL) {
//        return -1;
//    }
//    return mPlayer->getAudioEncode(encode);
//}
//
//
//int MediaPlayer::getAudioBitRate()
//{
//    Mutex::Autolock lock(mLock);
//    if (mPlayer == NULL) {
//        return -1;
//    }
//    return mPlayer->getAudioBitRate();
//}
//
//
//int MediaPlayer::getAudioSampleRate()
//{
//    Mutex::Autolock lock(mLock);
//    if (mPlayer == NULL) {
//        return -1;
//    }
//    return mPlayer->getAudioSampleRate();
//}


/* add by Gary. end   -----------------------------------}} */

/* add by Gary. start {{----------------------------------- */
/* 2011-11-14 */
/* support scale mode */
status_t MediaPlayer::enableScaleMode(bool enable, int width, int height)
{
    Mutex::Autolock lock(mLock);
    if (mPlayer == NULL) {
        return NO_INIT;
    }
    return mPlayer->enableScaleMode(enable, width, height);
}
/* add by Gary. end   -----------------------------------}} */

/* add by Gary. start {{----------------------------------- */
/* 2011-11-14 */
/* support adjusting colors while playing video */
//status_t MediaPlayer::setVppGate(bool enableVpp)
//{
//    const sp<IMediaPlayerService>& service(getMediaPlayerService());
//    if (service != 0) {
//        return service->setVppGate(enableVpp);
//    }else {
//        return BAD_VALUE;
//    }
//}
//
//bool MediaPlayer::getVppGate()
//{
//    const sp<IMediaPlayerService>& service(getMediaPlayerService());
//    if (service != 0) {
//        return service->getVppGate();
//    }else {
//        return false;
//    }
//}
//
//status_t MediaPlayer::setLumaSharp(int value)
//{
//    const sp<IMediaPlayerService>& service(getMediaPlayerService());
//    if (service != 0) {
//        return service->setLumaSharp(value);
//    }else {
//        return BAD_VALUE;
//    }
//}
//
//int MediaPlayer::getLumaSharp()
//{
//    const sp<IMediaPlayerService>& service(getMediaPlayerService());
//    if (service != 0) {
//        return service->getLumaSharp();
//    }else {
//        return -1;
//    }
//}
//
//int MediaPlayer::getChromaSharp()
//{
//    const sp<IMediaPlayerService>& service(getMediaPlayerService());
//    if (service != 0) {
//        return service->getChromaSharp();
//    }else {
//        return -1;
//    }
//}
//
//status_t MediaPlayer::setChromaSharp(int value)
//{
//    const sp<IMediaPlayerService>& service(getMediaPlayerService());
//    if (service != 0) {
//        return service->setChromaSharp(value);
//    }else {
//        return BAD_VALUE;
//    }
//}
//
//int MediaPlayer::getWhiteExtend()
//{
//    const sp<IMediaPlayerService>& service(getMediaPlayerService());
//    if (service != 0) {
//        return service->getWhiteExtend();
//    }else {
//        return -1;
//    }
//}
//
//status_t MediaPlayer::setWhiteExtend(int value)
//{
//    const sp<IMediaPlayerService>& service(getMediaPlayerService());
//    if (service != 0) {
//        return service->setWhiteExtend(value);
//    }else {
//        return BAD_VALUE;
//    }
//}
//
//int MediaPlayer::getBlackExtend()
//{
//    const sp<IMediaPlayerService>& service(getMediaPlayerService());
//    if (service != 0) {
//        return service->getBlackExtend();
//    }else {
//        return -1;
//    }
//}
//
//status_t MediaPlayer::setBlackExtend(int value)
//{
//    const sp<IMediaPlayerService>& service(getMediaPlayerService());
//    if (service != 0) {
//        return service->setBlackExtend(value);
//    }else {
//        return BAD_VALUE;
//    }
//}

/* add by Gary. end   -----------------------------------}} */

/* add by Gary. start {{----------------------------------- */
/* 2012-03-07 */
/* set audio channel mute */
status_t MediaPlayer::setChannelMuteMode(int muteMode)
{
    Mutex::Autolock lock(mLock);
    mMuteMode = muteMode;
    if (mPlayer == NULL) {
        return OK;
    }
    return mPlayer->setChannelMuteMode(muteMode);
}


int MediaPlayer::getChannelMuteMode()
{
    Mutex::Autolock lock(mLock);
    if (mPlayer == NULL) {
        return 0xFFFFFFFF;
    }
    return mPlayer->getChannelMuteMode();
}
/* add by Gary. end   -----------------------------------}} */

/* add by Gary. start {{----------------------------------- */
/* 2012-03-12 */
/* add the global interfaces to control the subtitle gate  */

//bool MediaPlayer::getGlobalSubGate()
//{
//    const sp<IMediaPlayerService>& service(getMediaPlayerService());
//    if (service != 0) {
//        return service->getGlobalSubGate();
//    }else {
//        return -1;
//    }
//}
//
//status_t MediaPlayer::setGlobalSubGate(bool showSub)
//{
//    const sp<IMediaPlayerService>& service(getMediaPlayerService());
//    if (service != 0) {
//        return service->setGlobalSubGate(showSub);
//    }else {
//        return BAD_VALUE;
//    }
//}
/* add by Gary. end   -----------------------------------}} */

/* add by Gary. start {{----------------------------------- */
/* 2012-4-24 */
/* add two general interfaces for expansibility */
status_t MediaPlayer::generalInterface(int cmd, int int1, int int2, int int3, void *p)
{
    Mutex::Autolock lock(mLock);
    if (mPlayer == NULL) {
        return NO_INIT;
    }
    return mPlayer->generalInterface(cmd, int1, int2, int3, p);
}
//
//status_t MediaPlayer::generalGlobalInterface(int cmd, int int1, int int2, int int3, void *p)
//{
//    const sp<IMediaPlayerService>& service(getMediaPlayerService());
//    if (service != 0) {
//        return service->generalGlobalInterface(cmd, int1, int2, int3, p);
//    }else {
//        return NO_INIT;
//    }
//}

/* add by Gary. end   -----------------------------------}} */

}; // namespace android
