//#define LOG_NDEBUG 0
#define LOG_TAG "SoundRecorder"
#include <utils/Log.h>

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <linux/delay.h>

#include "SoundRecorder.h"

#include <media/AudioRecord.h>
#include <media/AudioSystem.h>

static const nsecs_t kWarningIntervalNs = milliseconds(60);

SoundRecorder::SoundRecorder()
{
    mStarted = false;
    mInitCheck = false;
    mCbf = NULL;
    mRecordThread = new RecordThread(this);
    mBufferManager = new BufferManager();
    mAecState = AEC_ENABLED;
}

SoundRecorder::~SoundRecorder()
{
    exit();
    delete mBufferManager;
}

status_t SoundRecorder::createAudioRecorder(int sampleRate, int channels)
{
    if(!(channels == 1 || channels == 2)) {
        return -1;
    }

    int minFrameCount;

    status_t status = AudioRecord::getMinFrameCount(&minFrameCount,
                                    sampleRate,
                                    AUDIO_FORMAT_PCM_16_BIT,
                                    audio_channel_in_mask_from_count(channels));

    ALOGI("getMinFrameCount is %d",  minFrameCount);
    if (status != OK) {
        ALOGE("getMinFrameCount failed.");
        return UNKNOWN_ERROR;
    }

    int bufCount = 3;
    int frameCount = mSampleCount * mAecFrameCnt;
    while ((bufCount * frameCount) < minFrameCount) {
        bufCount++;
    }

    mAudioRecord = new AudioRecord(
                AUDIO_SOURCE_MIC, sampleRate, AUDIO_FORMAT_PCM_16_BIT,
                audio_channel_in_mask_from_count(channels),
                bufCount * frameCount,
                NULL,
                NULL,
                frameCount);

    if (mAudioRecord == NULL)
    {
        ALOGE("create AudioRecord failed");
        return UNKNOWN_ERROR;
    }

    status_t err = mAudioRecord->initCheck();
    if (err != OK)
    {
        mAudioRecord.clear();
        ALOGE("AudioRecord is not initialized");
        return UNKNOWN_ERROR;
    }

    ALOGI("recorder latency is %d", mAudioRecord->latency());
    ALOGI("CreateAudioRecorder framesize: %d, frameCount: %d, channelCount: %d, sampleRate: %d",
        mAudioRecord->frameSize(), mAudioRecord->frameCount(), mAudioRecord->channelCount(),
        mAudioRecord->getSampleRate());

    return OK;
}

status_t SoundRecorder::audioEncoderInit(CodecParams *params)
{
    mEncoder = newAudioEncoder(params->name);
    if (mEncoder == NULL) {
        return UNKNOWN_ERROR;
    }

    mSampleCount = mEncoder->set(params->sampleRate, params->channels, params->mode);

    ALOGV("sampleCount is %d", mSampleCount);

    int i = 1;

    int sampleCnt = 0;//mSampleCount;
    int sliceSize = EC_FRAME_SIZE * params->sampleRate / 8000;
    for(i = 1; i < 5; i++) {
        sampleCnt += mSampleCount;
        if (sampleCnt % sliceSize == 0) {
            break;
        }
    }

    if (i == 5) {
        delete mEncoder;
        mEncoder = NULL;
        ALOGE("get aec frame count failed.");
        return UNKNOWN_ERROR;
    }

    mAecFrameCnt = i;
    ALOGD("mAecFrameCnt is %d", mAecFrameCnt);

    uint32_t outputSize = mEncoder->getEncodedFrameMaxSize();
    if (mBufferManager->bufferAlloc(REC_BUF_COUNT, outputSize) != OK) {
        delete mEncoder;
        mEncoder = NULL;
        return -ENOMEM;
    }

    int frameSize = sizeof(int16_t) * params->channels;
    mRecBufSize = mSampleCount * frameSize * mAecFrameCnt;
    mRecBuffer = (char *)malloc(mRecBufSize);
    if (!mRecBuffer) {
        ALOGE("malloc mRecBuffer error.\n");
        goto fail;
    }

    mRecEcBuffer = (char *)malloc(mRecBufSize);
    if (!mRecEcBuffer) {
        ALOGE("malloc mRecEcBuffer error.\n");
        free(mRecBuffer);
        mRecBuffer = NULL;
        goto fail;
    }

    ALOGV("mRecBufSize is %d", mRecBufSize);

    return OK;

fail:
    mBufferManager->bufferDeAlloc();
    delete mEncoder;
    mEncoder = NULL;
    return -ENOMEM;
}

status_t SoundRecorder::audioEncoderExit()
{
    if (mRecBuffer) {
        free (mRecBuffer);
        mRecBuffer = NULL;
    }

    if (mRecEcBuffer) {
        free(mRecEcBuffer);
        mRecEcBuffer = NULL;
    }

    mBufferManager->bufferDeAlloc();

    delete mEncoder;
    mEncoder = NULL;
    return OK;
}

int SoundRecorder::rtspGetAudioData(unsigned char *data, unsigned int size, long long *pts)
{
    Mutex::Autolock autoLock(mLock);

    if (!mStarted) {
        ALOGW("recorder has been stopped.");
        return 0;
    }

    ALOGV("%s", __FUNCTION__);
    AudioBuffer *buffer = mBufferManager->getFirstAudioBuffer();
    if (!buffer) {
        return 0;
    }

    uint32_t outSize = buffer->size;
    if (size < outSize) {
        ALOGE("outSize is %d, large than %d", outSize, size);
        outSize = size;
    }
    memcpy(data, buffer->data, buffer->size);
    *pts = buffer->pts;
    mBufferManager->pushEmptyBuffer(buffer);

    return outSize;
}

status_t SoundRecorder::setAecEnabled_l(bool enabled)
{
    if (enabled) {
        if (mStarted) {
            if (aec_recorder_start(mSampleRate, mChannels) < 0) {
                return UNKNOWN_ERROR;
            }
        }
        mAecState = AEC_ENABLED;
    } else {
        mAecState = AEC_DISABLED;
        if (mStarted) {
            aec_recorder_stop();
        }
    }

    return OK;
}

status_t SoundRecorder::setAecEnabled(bool enabled)
{
	Mutex::Autolock autoLock(mLock);
    ALOGV("setAecEnabled: enabled(%d)", enabled);
    if (mStarted) {
        return UNKNOWN_ERROR;
    }
    mAecState = enabled ? AEC_ENABLED : AEC_DISABLED;
    return OK;
}

bool SoundRecorder::isAecEnabled()
{
	Mutex::Autolock autoLock(mLock);
    return (mAecState == AEC_ENABLED ? 1 : 0);
}

bool SoundRecorder::audioRecordThread()
{
    ssize_t len = mAudioRecord->read(mRecBuffer, mRecBufSize);
    if (len == 0) {
        ALOGE("mAudioRecord read error, len(%ld)", len);
        return false;
    }

    //ALOGV("AudioRecord read buffer len is %d", len);

    int16_t * samples;

    if (mAecState == AEC_ENABLED) {
        if (aec_recorder_process(mRecBuffer, mRecEcBuffer, len) == 0) {
            samples = (int16_t *)mRecEcBuffer;
        } else {
            samples = (int16_t *)mRecBuffer;
        }
    } else {
        samples = (int16_t *)mRecBuffer;
    }

    AudioBuffer * buffer;
    for (int i = 0; i < mAecFrameCnt; i++) {
        buffer = mBufferManager->getFirstEmptyBuffer();
        int length = mEncoder->encode(buffer->data, (int16_t *)samples + i * mSampleCount);
        if (length <= 0) {
            ALOGW("audio encode error");
            mBufferManager->pushEmptyBuffer(buffer);
            continue;
        }

        buffer->size = length;
        buffer->pts = mPts;

        mPts += (mSampleCount * 1000)/(mSampleRate / 1000);
        mBufferManager->pushAudioBuffer(buffer);

        if (mCbf) {
            mCbf(mUserData);
        }
    }

    return true;
}

status_t SoundRecorder::init(void *aec, CodecParams *params, void *user, callback_t cbf)
{
    status_t ret = 0;
    Mutex::Autolock autoLock(mLock);

    if (mInitCheck) {
        return UNKNOWN_ERROR;
    }

    if (params->channels != 1
        || (params->sampleRate != 8000 && params->sampleRate != 16000)) {
        ALOGE("sampleRate(%d), channels(%d)",params->sampleRate, params->channels);
        return UNKNOWN_ERROR;
    }

    mSampleRate = params->sampleRate;
    mChannels = params->channels;

    ret = audioEncoderInit(params);
    if (ret != OK) {
        ALOGE("audioEncoderInit failed.");
        return ret;
    }

    ret = createAudioRecorder(mSampleRate, mChannels);
    if (ret != OK) {
        ALOGE("createAudioRecorder failed.");
        audioEncoderExit();
        return ret;
    }

    mUserData = user;
    mCbf = cbf;
    mAec = aec;

    mInitCheck = true;
    ALOGV("Sound Recorder init success.");
    return OK;
}


status_t SoundRecorder::start()
{
    Mutex::Autolock autoLock(mLock);

    ALOGV("%s", __func__);

    if (mStarted) {
        return UNKNOWN_ERROR;
    }

    if(mInitCheck != true) {
        return NO_INIT;
    }

    mPts = 0;
    mBufferManager->bufferFlush();

    status_t ret = mAudioRecord->start();
    if (ret != OK) {
        mAudioRecord.clear();
        return ret;
    }

    if (!mRecordThread->start()) {
        mAudioRecord->stop();
        return UNKNOWN_ERROR;
    }

    if (mAecState == AEC_ENABLED) {
        aec_recorder_start(mSampleRate, mChannels);
    }

    mStarted = true;
    return OK;
}

status_t SoundRecorder::stop_l()
{
    if (!mStarted) {
        return UNKNOWN_ERROR;
    }
    mStarted = false;

    mRecordThread->stop();

    mAudioRecord->stop();

    if (mAecState == AEC_ENABLED) {
        aec_recorder_stop();
    }

    return OK;
}

status_t SoundRecorder::stop()
{
    Mutex::Autolock autoLock(mLock);
    ALOGV("stop(): mStarted is %d", mStarted);

    return stop_l();
}

status_t SoundRecorder::exit()
{
    Mutex::Autolock autoLock(mLock);

    if(mInitCheck != true) {
        return NO_INIT;
    }
    mInitCheck = 0;

    stop_l();

    mAudioRecord.clear();

    audioEncoderExit();

    return 0;
}

