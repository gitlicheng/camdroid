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
#define LOG_TAG "CedarXAudioPlayer"
#include <CDX_Debug.h>

#include <binder/IPCThreadState.h>
#include <media/AudioTrack.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>

#include "CedarXPlayer.h"
#include "CedarXAudioPlayer.h"

namespace android {

CedarXAudioPlayer::CedarXAudioPlayer(
        const sp<MediaPlayerBase::AudioSink> &audioSink,
        CedarXPlayer *observer)
    : mAudioTrack(NULL),
      mInputBuffer(NULL),
      mSampleRate(0),
      mLatencyUs(0),
      mFrameSize(0),
      mNumFramesPlayed(0),
      mSeeking(false),
      mReachedEOS(false),
      mFinalStatus(OK),
      mStarted(false),
      mIsFirstBuffer(false),
      mFirstBufferResult(OK),
      mFirstBuffer(NULL),
      mAudioSink(audioSink),
      mObserver(observer),
      mAudioBufferSize(0),
      mSuppressData(false) {
}

CedarXAudioPlayer::~CedarXAudioPlayer()
{
    if (mStarted) {
        reset();
    }
}

void CedarXAudioPlayer::setSource(const sp<MediaSource> &source)
{

}

void CedarXAudioPlayer::setFormat(int samplerate, int channel)
{
	mSampleRate = samplerate;
	numChannels = channel;
}

status_t CedarXAudioPlayer::start(bool sourceAlreadyStarted)
{
	CHECK(!mStarted);

    if (mAudioSink.get() != NULL) {
    	LOGV("AudioPlayer::start 0.1 ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]");
#if (CEDARX_ANDROID_VERSION < 7)
        status_t err = mAudioSink->open(
#if (CEDARX_ANDROID_VERSION == 4)
                mSampleRate, numChannels, AudioSystem::PCM_16_BIT,
#else
                mSampleRate, numChannels, AUDIO_FORMAT_PCM_16_BIT,
#endif
                DEFAULT_AUDIOSINK_BUFFERCOUNT,
                &CedarXAudioPlayer::AudioSinkCallback, this);
#else
        int channelMask = CHANNEL_MASK_USE_CHANNEL_ORDER;
        status_t err = mAudioSink->open(
                mSampleRate, numChannels, channelMask, AUDIO_FORMAT_PCM_16_BIT,
                DEFAULT_AUDIOSINK_BUFFERCOUNT,
                &CedarXAudioPlayer::AudioSinkCallback,
                this, AUDIO_OUTPUT_FLAG_NONE);
#endif

        if (err != OK) {

            return err;
        }

        mLatencyUs = (int64_t)mAudioSink->latency() * 1000;
        mFrameSize = mAudioSink->frameSize();

        mAudioSink->start();
    } else {
    	LOGV("AudioPlayer::start 0.2 ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]");
    	status_t err;
        mAudioTrack = new AudioTrack(
                AUDIO_STREAM_MUSIC, mSampleRate, AUDIO_FORMAT_PCM_16_BIT,
                (numChannels == 2)
                    ? AUDIO_CHANNEL_OUT_STEREO
                    : AUDIO_CHANNEL_OUT_MONO,
                0, 0, &AudioCallback, this, 0);

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
    mNumFramesPlayed = 0;
    LOGV("AudioPlayer::start 0.8 ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]");

    return OK;
}

void CedarXAudioPlayer::pause(bool playPendingSamples)
{
	if(!mStarted) {
		return ;
	}

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

void CedarXAudioPlayer::resume()
{
	if(!mStarted) {
		return ;
	}

    if (mAudioSink.get() != NULL) {
        mAudioSink->start();
    } else {
        mAudioTrack->start();
    }
}

void CedarXAudioPlayer::reset()
{
	if(!mStarted) {
		return ;
	}

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
void CedarXAudioPlayer::AudioCallback(int event, void *user, void *info)
{
    static_cast<CedarXAudioPlayer *>(user)->AudioCallback(event, info);
}

bool CedarXAudioPlayer::isSeeking()
{
    Mutex::Autolock autoLock(mLock);
    return mSeeking;
}

bool CedarXAudioPlayer::reachedEOS(status_t *finalStatus)
{
    *finalStatus = OK;

    Mutex::Autolock autoLock(mLock);
    *finalStatus = mFinalStatus;
    return mReachedEOS;
}

// static
size_t CedarXAudioPlayer::AudioSinkCallback(
        MediaPlayerBase::AudioSink *audioSink,
        void *buffer, size_t size, void *cookie)
{
    CedarXAudioPlayer *me = (CedarXAudioPlayer *)cookie;

    return me->fillBuffer(buffer, size);
}

void CedarXAudioPlayer::AudioCallback(int event, void *info)
{
    if (event != AudioTrack::EVENT_MORE_DATA) {
        return;
    }

    AudioTrack::Buffer *buffer = (AudioTrack::Buffer *)info;
    LOGV("==== AudioSinkCallback, buffersize = %d", buffer->size);
    size_t numBytesWritten = fillBuffer(buffer->raw, buffer->size);

    buffer->size = numBytesWritten;
}

size_t CedarXAudioPlayer::fillBuffer(void *data, size_t size)
{
    if (mReachedEOS) {
        LOGV("(f:%s, l:%d) mReachedEOS=%d", __FUNCTION__, __LINE__, mReachedEOS);
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

    	if (mReachedEOS) {
            LOGD("(f:%s, l:%d) mReachedEOS=%d, mAudioBufferSize=%d", __FUNCTION__, __LINE__, mReachedEOS, mAudioBufferSize);
    	    return size - mAudioBufferSize;
    	}

        usleep(5*1000);
    }
    LOGV("----tobe fillBuffer size:%d",mAudioBufferSize);
    return size;
}

int CedarXAudioPlayer::getLatency()
{
	unsigned int cache;
	int cache_time;

	if (mAudioSink.get() != NULL) {
		mAudioSink->getPosition(&cache);
		if(mFrameSize != 0) {
			cache_time = (mNumFramesPlayed / mFrameSize - cache) * 1000000 / mSampleRate;
		}
		else {
			cache_time = 0;
		}
		LOGV("mLatencyUs = %d", cache_time);
		if(cache_time < 0) {
			cache_time = 0;
		}
	}
	else {
		mAudioTrack->getPosition(&cache);
		if(mFrameSize != 0) {
			cache_time = (mNumFramesPlayed / mFrameSize - cache) * 1000000 / mSampleRate;
		}
		else {
			cache_time = 0;
		}
		LOGV("mLatencyUs = %d", cache_time);
		if(cache_time < 0) {
			cache_time = 0;
		}
	}
	return cache_time;
}

int CedarXAudioPlayer::getSpace()
{
	Mutex::Autolock autoLock(mLock);
    return mAudioBufferSize;
}

int CedarXAudioPlayer::render(void* data, int len)
{
	int tobe_fill_size;
	Mutex::Autolock autoLock(mLock);

	tobe_fill_size = (len > mAudioBufferSize) ? mAudioBufferSize : len;
	if(mSuppressData) {
		memset(mAudioBufferPtr, 0, tobe_fill_size);
	} else {
		memcpy(mAudioBufferPtr, data, tobe_fill_size);
	}
	mAudioBufferSize -= tobe_fill_size;
	mAudioBufferPtr += tobe_fill_size;
//	LOGV("++++fillBuffer size:%d",tobe_fill_size);
	mNumFramesPlayed += tobe_fill_size;
	return tobe_fill_size;
}

status_t CedarXAudioPlayer::seekTo(int64_t time_us)
{
	unsigned int cache;
    Mutex::Autolock autoLock(mLock);

    mNumFramesPlayed = 0;

    if (mAudioSink != NULL) {
        mAudioSink->flush();

    	mAudioSink->getPosition(&cache);
    	mNumFramesPlayed = cache*mFrameSize;
    } else {
        mAudioTrack->flush();

        mAudioTrack->getPosition(&cache);
    	mNumFramesPlayed = cache*mFrameSize;
    }

    return OK;
}

void CedarXAudioPlayer::setSuppressData(bool is)
{
	 LOGV("setSuppressData %d", is);
	 Mutex::Autolock autoLock(mLock);
	 mSuppressData = is;
}

void CedarXAudioPlayer::setPlaybackEos(bool end_of_stream)
{
	LOGV("setPlaybackEnd");
	mReachedEOS = end_of_stream;
}

}
