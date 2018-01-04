/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CEDARA_PLAYER_H_

#define CEDARA_PLAYER_H_

#include <media/MediaPlayerInterface.h>
#include <media/stagefright/DataSource.h>
//#include <media/stagefright/OMXClient.h>
#include <media/stagefright/TimeSource.h>
#include <utils/threads.h>

#include <CedarAAudioPlayer.h>
#include <media/stagefright/MediaBuffer.h>

#include "CedarADecoder.h"

#if (CEDARX_ANDROID_VERSION > 6)
#define LOGV ALOGV
#define LOGE ALOGE
#endif

namespace android {

struct CedarAAudioPlayer;
struct DataSource;
struct MediaBuffer;
struct MediaExtractor;
struct MediaSource;
struct NuCachedSource2;
struct ALooper;

struct CedarAPlayer {
    CedarAPlayer();
    ~CedarAPlayer();

    void setListener(const wp<MediaPlayerBase> &listener);
    void setUID(uid_t uid);

    status_t setDataSource(
            const char *uri,
            const KeyedVector<String8, String8> *headers = NULL);

    status_t setDataSource(int fd, int64_t offset, int64_t length);
#if (CEDARX_ANDROID_VERSION >= 6)
    status_t setDataSource(const sp<IStreamSource> &source);
#endif
    void reset();

    status_t prepare();
    status_t prepare_l();
    status_t prepareAsync();
    status_t prepareAsync_l();

    status_t play();
    status_t pause();
    status_t stop_l();
    status_t stop();

    bool isPlaying() const;
#if (CEDARX_ANDROID_VERSION < 6)
    void setISurface(const sp<ISurface> &isurface);
#else
#if (CEDARX_ANDROID_VERSION < 7)
    status_t setSurface(const sp<Surface> &surface);
#endif
    status_t setSurfaceTexture(const unsigned int hlay);
    status_t setParameter(int key, const Parcel &request);
    status_t getParameter(int key, Parcel *reply);
    status_t setCacheStatCollectFreq(const Parcel &request);
#endif
    void setAudioSink(const sp<MediaPlayerBase::AudioSink> &audioSink);
    status_t setLooping(bool shouldLoop);

    status_t getDuration(int64_t *durationUs);
    status_t getPosition(int64_t *positionUs);

    status_t seekTo(int64_t timeUs);

    status_t getVideoDimensions(int32_t *width, int32_t *height) const;

    status_t suspend();
    status_t resume();

//    status_t setScreen(int screen);

    // This is a mask of MediaExtractor::Flags.
    uint32_t flags() const;

    void postAudioEOS();
    void postAudioSeekComplete();

    int CedarAPlayerCallback(int event, void *info);
private:
    //friend struct CedarXEvent;

    enum {
        PLAYING             = 1,
        LOOPING             = 2,
        FIRST_FRAME         = 4,
        PREPARING           = 8,
        PREPARED            = 16,
        AT_EOS              = 32,
        PREPARE_CANCELLED   = 64,
        CACHE_UNDERRUN      = 128,
        AUDIO_AT_EOS        = 256,
        VIDEO_AT_EOS        = 512,
        AUTO_LOOPING        = 1024,
        WAIT_TO_PLAY        = 2048,
        WAIT_VO_EXIT        = 4096,
        CEDARX_LIB_INIT     = 8192,
        SUSPENDING          = 16384,
        PAUSING				= 32768,
    };

    mutable Mutex mLock;
    Mutex mMiscStateLock;

    CDADecoder *mPlayer;
    int mStreamType;

    bool mQueueStarted;
    wp<MediaPlayerBase> mListener;
#if (CEDARX_ANDROID_VERSION < 6)
    sp<ISurface> mISurface;
#else
    bool mUIDValid;
    uid_t mUID;
#if (CEDARX_ANDROID_VERSION < 7)
    sp<Surface> mSurface;
#endif
    //sp<ANativeWindow> mNativeWindow;
#endif

    sp<MediaPlayerBase::AudioSink> mAudioSink;

    String8 mUri;
    bool mIsUri;
    KeyedVector<String8, String8> mUriHeaders;

    CedarAAudioPlayer *mAudioPlayer;
    int64_t mDurationUs;

    uint32_t mFlags;
    uint32_t mExtractorFlags;
    bool isCedarAInitialized;

    bool mSeeking;
    bool mSeekNotificationSent;
    int64_t mSeekTimeUs;

    int64_t mBitrate;  // total bitrate of the file (in bps) or -1 if unknown.

    int64_t mLastValidPosition;

    bool mIsAsyncPrepare;
    status_t mPrepareResult;
    status_t mStreamDoneStatus;

    int32_t mScreenID;

    void postVideoEvent_l(int64_t delayUs = -1);
    void postBufferingEvent_l();
    void postStreamDoneEvent_l(status_t status);
    void postCheckAudioStatusEvent_l();
    status_t play_l(int command);

    bool mWaitAudioPlayback;

    sp<ALooper> mLooper;

    int64_t mSuspensionPositionUs;

    struct SuspensionState {
        String8 mUri;
        KeyedVector<String8, String8> mUriHeaders;
        sp<DataSource> mFileSource;

        uint32_t mFlags;
        int64_t mPositionUs;
    } mSuspensionState;

//    status_t setDataSource_l(
//            const char *uri,
//            const KeyedVector<String8, String8> *headers = NULL);
//
//    status_t setDataSource_l(const sp<DataSource> &dataSource);
//    status_t setDataSource_l(const sp<MediaExtractor> &extractor);
    void reset_l();
    void partial_reset_l();
    status_t seekTo_l(int64_t timeUs);
    status_t pause_l(bool at_eos = false);
    void initRenderer_l();
    void seekAudioIfNecessary_l();

    void cancelPlayerEvents(bool keepBufferingGoing = false);

    void setAudioSource(sp<MediaSource> source);
    status_t initAudioDecoder();

    void setVideoSource(sp<MediaSource> source);
    status_t initVideoDecoder(uint32_t flags = 0);

    void onStreamDone();

    void notifyListener_l(int msg, int ext1 = 0, int ext2 = 0);

    void onBufferingUpdate();
    void onCheckAudioStatus();
    void onPrepareAsyncEvent();
    void abortPrepare(status_t err);
    void finishAsyncPrepare_l(int err);
    void finishSeek_l(int err);
    bool getCachedDuration_l(int64_t *durationUs, bool *eos);

    status_t finishSetDataSource_l();

    static bool ContinuePreparation(void *cookie);

    bool getBitrate(int64_t *bitrate);

    int StagefrightVideoRenderInit(int width, int height, int format);
    int StagefrightAudioRenderInit(int samplerate, int channels, int format);
    void StagefrightVideoRenderData(void *frame_info, int frame_id);
    int StagefrightVideoRenderGetFrameID();

    void StagefrightAudioRenderExit(int immed);
    int StagefrightAudioRenderData(void* data, int len);
    int StagefrightAudioRenderGetSpace(void);
    int StagefrightAudioRenderGetDelay(void);

    CedarAPlayer(const CedarAPlayer &);
    CedarAPlayer &operator=(const CedarAPlayer &);
};

}  // namespace android

#endif  // AWESOME_PLAYER_H_

