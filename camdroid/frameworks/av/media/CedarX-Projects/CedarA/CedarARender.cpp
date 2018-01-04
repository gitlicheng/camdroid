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

//#define LOG_NDEBUG 0
#define LOG_TAG "CedarARender"
#include <utils/Log.h>

#include <binder/IPCThreadState.h>
#include <media/AudioTrack.h>
#if (CEDARX_ANDROID_VERSION < 6)
#include <media/stagefright/MediaDebug.h>
#else
#include <media/stagefright/foundation/ADebug.h>
#endif
#include <CedarAAudioPlayer.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>

#include "CedarAPlayer.h"

namespace android {

CedarAAudioPlayer::CedarAAudioPlayer(
        const sp<MediaPlayerBase::AudioSink> &audioSink,
        CedarAPlayer *observer)
    : mAudioTrack(NULL),
      mInputBuffer(NULL),
      mSampleRate(0),
      mLatencyUs(0),
      mFrameSize(0),
      mSeeking(false),
      mReachedEOS(false),
      mFinalStatus(OK),
      mStarted(false),
      mIsFirstBuffer(false),
      mFirstBufferResult(OK),
      mFirstBuffer(NULL),
      mAudioSink(audioSink),
      mObserver(observer),
      mAudioBufferSize(0){
}

CedarAAudioPlayer::~CedarAAudioPlayer()
{
    if (mStarted) {
        reset();
    }
}

void CedarAAudioPlayer::setSource(const sp<MediaSource> &source)
{

}

void CedarAAudioPlayer::setFormat(int samplerate, int channel)
{
	mSampleRate = samplerate;
	mNumChannels = channel;
}

status_t CedarAAudioPlayer::start(bool sourceAlreadyStarted)
{
	CHECK(!mStarted);

    if (mAudioSink.get() != NULL) {
    	LOGV("AudioPlayer::start 0.1 ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]");
#if (CEDARX_ANDROID_VERSION < 7)
        status_t err = mAudioSink->open(
#if (CEDARX_ANDROID_VERSION == 4)
                mSampleRate, mNumChannels, AudioSystem::PCM_16_BIT,
#else
                mSampleRate, mNumChannels, AUDIO_FORMAT_PCM_16_BIT,
#endif
                DEFAULT_AUDIOSINK_BUFFERCOUNT,
                &CedarAAudioPlayer::AudioSinkCallback, this);
#else
        int channelMask = CHANNEL_MASK_USE_CHANNEL_ORDER;
        status_t err = mAudioSink->open(
                mSampleRate, mNumChannels, channelMask, AUDIO_FORMAT_PCM_16_BIT,
                DEFAULT_AUDIOSINK_BUFFERCOUNT,
                &CedarAAudioPlayer::AudioSinkCallback,
                this, AUDIO_OUTPUT_FLAG_NONE);
#endif
        if (err != OK) {
            return err;
        }

        mLatencyUs = (int64_t)mAudioSink->latency() * 1000;
        mFrameSize = mAudioSink->frameSize();

        mAudioSink->start();
    } else {
    	status_t err;
#if (CEDARX_ANDROID_VERSION < 6)
        mAudioTrack = new AudioTrack(
                AudioSystem::MUSIC, mSampleRate, AudioSystem::PCM_16_BIT,
                (mNumChannels == 2)
                    ? AudioSystem::CHANNEL_OUT_STEREO
                    : AudioSystem::CHANNEL_OUT_MONO,
                0, 0, &AudioCallback, this, 0);
#else
        mAudioTrack = new AudioTrack(
                AUDIO_STREAM_MUSIC, mSampleRate, AUDIO_FORMAT_PCM_16_BIT,
                (mNumChannels == 2)
                    ? AUDIO_CHANNEL_OUT_STEREO
                    : AUDIO_CHANNEL_OUT_MONO,
                0, 0, &AudioCallback, this, 0);
#endif
        if ((err = mAudioTrack->initCheck()) != OK) {
            delete mAudioTrack;
            mAudioTrack = NULL;

            return err;
        }

        mLatencyUs = (int64_t)mAudioTrack->latency() * 1000;
        mFrameSize = mAudioTrack->frameSize();

        mAudioTrack->start();
    }

    mStarted = true;

    return OK;
}

void CedarAAudioPlayer::pause(bool playPendingSamples)
{
    CHECK(mStarted);

    if (playPendingSamples) {
        if (mAudioSink.get() != NULL) {
            mAudioSink->stop();
        } else {
            mAudioTrack->stop();
        }
    } else {
        if (mAudioSink.get() != NULL) {
            mAudioSink->pause();
        } else {
            mAudioTrack->pause();
        }
    }
}

void CedarAAudioPlayer::resume()
{
    CHECK(mStarted);

    if (mAudioSink.get() != NULL) {
        mAudioSink->start();
    } else {
        mAudioTrack->start();
    }
}

void CedarAAudioPlayer::reset()
{
    CHECK(mStarted);

    mReachedEOS = true;

    if (mAudioSink.get() != NULL) {
        mAudioSink->stop();
        mAudioSink->close();
    } else {
        mAudioTrack->stop();

        delete mAudioTrack;
        mAudioTrack = NULL;
    }


    IPCThreadState::self()->flushCommands();

    mSeeking = false;
    mReachedEOS = false;
    mFinalStatus = OK;
    mStarted = false;
    mAudioBufferSize = 0;
}

// static
void CedarAAudioPlayer::AudioCallback(int event, void *user, void *info)
{
    static_cast<CedarAAudioPlayer *>(user)->AudioCallback(event, info);
}

bool CedarAAudioPlayer::isSeeking()
{
    Mutex::Autolock autoLock(mLock);
    return mSeeking;
}

bool CedarAAudioPlayer::reachedEOS(status_t *finalStatus)
{
    *finalStatus = OK;

    Mutex::Autolock autoLock(mLock);
    *finalStatus = mFinalStatus;
    return mReachedEOS;
}

// static
size_t CedarAAudioPlayer::AudioSinkCallback(
        MediaPlayerBase::AudioSink *audioSink,
        void *buffer, size_t size, void *cookie)
{
    CedarAAudioPlayer *me = (CedarAAudioPlayer *)cookie;

    return me->fillBuffer(buffer, size);
}

void CedarAAudioPlayer::AudioCallback(int event, void *info)
{
    if (event != AudioTrack::EVENT_MORE_DATA) {
        return;
    }

    AudioTrack::Buffer *buffer = (AudioTrack::Buffer *)info;
    size_t numBytesWritten = fillBuffer(buffer->raw, buffer->size);

    buffer->size = numBytesWritten;
}

size_t CedarAAudioPlayer::fillBuffer(void *data, size_t size)
{
    if (mReachedEOS) {
        return 0;
    }

    {
		Mutex::Autolock autoLock(mLock);
		mAudioBufferSize = size;
		mAudioBufferPtr = (char *)data;
	}

    LOGV("++++tobe fillBuffer size:%d",mAudioBufferSize);
    while (mAudioBufferSize > 0) {
    	LOGV("tobe fillBuffer size:%d",mAudioBufferSize);

    	if (mReachedEOS)
    	     return 0;

        usleep(10*1000);
    }
    LOGV("----tobe fillBuffer size:%d",mAudioBufferSize);
    return size;
}

int CedarAAudioPlayer::getLatency()
{
    return (int)mLatencyUs;
}

int CedarAAudioPlayer::getSpace()
{
	Mutex::Autolock autoLock(mLock);
    return mAudioBufferSize;
}

int CedarAAudioPlayer::render(void* data, int len)
{
	int tobe_fill_size;
	Mutex::Autolock autoLock(mLock);

	tobe_fill_size = (len > mAudioBufferSize) ? mAudioBufferSize : len;
	memcpy(mAudioBufferPtr, data, tobe_fill_size);
	mAudioBufferSize -= tobe_fill_size;
	mAudioBufferPtr += tobe_fill_size;
	LOGV("++++fillBuffer size:%d",tobe_fill_size);
	return tobe_fill_size;
}

status_t CedarAAudioPlayer::seekTo(int64_t time_us)
{
    Mutex::Autolock autoLock(mLock);

//    mSeeking = true;
//    mReachedEOS = false;

    if (mAudioSink != NULL) {
        mAudioSink->flush();
    } else {
        mAudioTrack->flush();
    }

    return OK;
}

void CedarAAudioPlayer::setPlaybackEos(bool end_of_stream)
{
	mReachedEOS = end_of_stream;
}

}
