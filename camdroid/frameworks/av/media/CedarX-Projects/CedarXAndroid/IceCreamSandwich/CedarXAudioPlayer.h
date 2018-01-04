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

#ifndef CEDARX_AUDIO_PLAYER_H_

#define CEDARX_AUDIO_PLAYER_H_

#include <media/MediaPlayerInterface.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/TimeSource.h>
#include <utils/threads.h>
//#include <media/stagefright/MediaClock.h>

namespace android {

class MediaSource;
class AudioTrack;
class CedarXPlayer;

class CedarXAudioPlayer {
public:
    enum {
        REACHED_EOS,
        SEEK_COMPLETE
    };

    enum {
    	FlagsFillBuffer = 1,
    };

    CedarXAudioPlayer(const sp<MediaPlayerBase::AudioSink> &audioSink,
                CedarXPlayer *audioObserver = NULL);

    virtual ~CedarXAudioPlayer();

    // Caller retains ownership of "source".
    void setSource(const sp<MediaSource> &source);

    void setFormat(int samplerate, int channel);


    status_t start(bool sourceAlreadyStarted = false);

    void pause(bool playPendingSamples = false);
    void resume();

    int getLatency();
    int getSpace();
    int render(void* data, int len);

    status_t seekTo(int64_t time_us);

    bool isSeeking();
    bool reachedEOS(status_t *finalStatus);
    void setSuppressData(bool is);
    void setPlaybackEos(bool end_of_stream);
    //void setEventMark(uint32_t event);

private:
    sp<MediaSource> mSource;
    AudioTrack *mAudioTrack;

    //bool mInitMediaClock;
    //MediaClock *mMediaClock;

    MediaBuffer *mInputBuffer;

    int mSampleRate;
    int numChannels;
    int64_t mLatencyUs;
    size_t mFrameSize;

    Mutex mLock, mLock2;
    int64_t mNumFramesPlayed;

    int64_t mPositionTimeMediaUs;
    int64_t mPositionTimeRealUs;

    bool mSeeking;
    bool mReachedEOS;
    status_t mFinalStatus;
    int64_t mSeekTimeUs;

    bool mStarted;

    bool mIsFirstBuffer;
    status_t mFirstBufferResult;
    MediaBuffer *mFirstBuffer;

    sp<MediaPlayerBase::AudioSink> mAudioSink;
    CedarXPlayer *mObserver;
    char *mAudioBufferPtr;
    int mAudioBufferSize;
    char *mAudioBufferPtrBak;
    int mAudioBufferSizeBak;
    int mFlags;
    /*add by weihongqiang, to protect audio data that is drm protected.*/
    bool mSuppressData;

    static void AudioCallback(int event, void *user, void *info);
    void AudioCallback(int event, void *info);

    static size_t AudioSinkCallback(
            MediaPlayerBase::AudioSink *audioSink,
            void *data, size_t size, void *me);

    size_t fillBuffer(void *data, size_t size);
    Condition mFillBufferCondition;

    void reset();

    CedarXAudioPlayer(const CedarXAudioPlayer &);
    CedarXAudioPlayer &operator=(const CedarXAudioPlayer &);
};

}  // namespace android

#endif  // AUDIO_PLAYER_H_
