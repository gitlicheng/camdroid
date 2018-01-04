//#define LOG_NDEBUG 0
#define LOG_TAG "SoundPlayer"
#include <utils/Log.h>

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <linux/delay.h>

#include "WebrtcNeteq_Interface.h"
#include "SoundPlayer.h"

//webrtc neteq
static const int kMaxChannels = 1;
static const int kMaxSamplesPerMs = 48000 / 1000;
static const int kOutputBlockSizeMs = 20;
static const int kPacketTimeMs = 20;

using namespace android;

typedef struct DecoderTypeMap {
    const char *name;
    DecoderType type;
} DecoderTypeMap;

DecoderTypeMap DecoderTypeMaps[] = {
    {"OPUS",    DecoderOpus},
    {"G726",    DecoderG726},
    {"AMR",     DecoderAMR},
};
/*
static const char * DecoderType2DecoderName(DecoderType type)
{
    for(int i=0; i< (int)(sizeof(DecoderTypeMaps)/sizeof(DecoderTypeMap)); i++) {
        if(DecoderTypeMaps[i].type == type) {
            return DecoderTypeMaps[i].name;
        }
    }

    return "NULL";
}
*/
static DecoderType DecoderName2DecoderType(const char* name)
{
    for(int i=0; i< (int)(sizeof(DecoderTypeMaps)/sizeof(DecoderTypeMap)); i++) {
        if(strcasecmp(DecoderTypeMaps[i].name, name) == 0) {
            return DecoderTypeMaps[i].type;
        }
    }

    ALOGW("DecoderName2DecoderType: name(%s) return unknown type", name);
    return DecoderReservedStart;
}

static int audioGetPacketDuration(DecoderType type)
{
    int duration;
    switch (type) {
    case DecoderAMR:
    case DecoderOpus:
    case DecoderG726:
        duration = 20;       //20ms
        break;
    default:
        duration = -1;
        break;
    }

    return duration;
}

SoundPlayer::SoundPlayer()
{
    mStarted = false;
    mInitCheck = false;
    mAudioTrack = NULL;
#ifndef PLAY_USE_WEBRTC_NETEQ
    mBufferManager = new BufferManager();
#endif
    pthread_mutex_init(&mInMutex, NULL);
    pthread_cond_init(&mInCond, NULL);
    mPlayThread = new PlayThread(this);
}

SoundPlayer::~SoundPlayer()
{
    exit();
#ifndef PLAY_USE_WEBRTC_NETEQ
    delete mBufferManager;
#endif
    pthread_mutex_destroy(&mInMutex);
    pthread_cond_destroy(&mInCond);
}

int SoundPlayer::createAudioTrack(int sampleRate, int channels)
{
    status_t err;

    if (mAudioTrack) {
        delete mAudioTrack;
        mAudioTrack = NULL;
    }

    int32_t minFrameCount;
    status_t status = AudioTrack::getMinFrameCount(&minFrameCount,
                                    AUDIO_STREAM_MUSIC, sampleRate);

    ALOGI("minFrameCount is %d", minFrameCount);

    int32_t outBlockSizeSamples = kOutputBlockSizeMs * sampleRate / 1000;
    if (minFrameCount < outBlockSizeSamples * 3) {
        minFrameCount = outBlockSizeSamples * 3;
    }

    // Open audio track in mono, PCM 16bit, default sampling rate, default buffer size
    mAudioTrack = new AudioTrack();
    ALOGV("Create Track: %p", mAudioTrack);

    mAudioTrack->set(AUDIO_STREAM_MUSIC,
                    sampleRate,    // sampleRate
                    AUDIO_FORMAT_PCM_16_BIT,
                    (channels == 2)? AUDIO_CHANNEL_OUT_STEREO: AUDIO_CHANNEL_OUT_MONO,
                    minFrameCount,          // frameCount
                    AUDIO_OUTPUT_FLAG_NONE, //AUDIO_OUTPUT_FLAG_DIRECT,
                    NULL,
                    NULL, // user
                    0,    // notificationFrames
                    0,    // sharedBuffer
                    0);   // threadCanCallJava

    if ((err = mAudioTrack->initCheck()) != NO_ERROR) {
        ALOGE("AudioTrack->initCheck failed");
        delete mAudioTrack;
        mAudioTrack = NULL;
        return err;
    }

    uint32_t latency = mAudioTrack->latency();
    size_t frameSize = mAudioTrack->frameSize();
    uint32_t frameCount = mAudioTrack->frameCount();

    ALOGI("latency(%d), frameSize(%d), frameCount(%d)",latency, frameSize, frameCount);

    return OK;
}

#ifndef PLAY_USE_WEBRTC_NETEQ
status_t SoundPlayer::audioDecoderInit(CodecParams *params)
{
    mDecoder = newAudioDecoder(params->name);
    if (mDecoder == NULL) {
        return UNKNOWN_ERROR;
    }

    mSampleCount = mDecoder->set(params->sampleRate, params->channels, params->mode);
    mFrameSize = sizeof(uint16_t) * params->channels;

    ALOGI("sampleCount is %d , mFrameSize is %d", mSampleCount, mFrameSize);

    mBufferSize = mDecoder->getEncodedFrameMaxSize();
    if (mBufferManager->bufferAlloc(ENC_BUF_COUNT, mBufferSize) != OK) {
        delete mDecoder;
        return -ENOMEM;
    }

    return OK;
}

status_t SoundPlayer::audioDecoderExit()
{
    mBufferManager->bufferDeAlloc();
    delete mDecoder;
    return OK;
}
#endif

bool SoundPlayer::audioPlayThread()
{
    if (mFirstPacket) {
        ALOGV("audioplaythread tid is %d", gettid());
        pthread_mutex_lock(&mInMutex);
        if (mFirstPacket) {
            pthread_cond_wait(&mInCond, &mInMutex);
        }
        pthread_mutex_unlock(&mInMutex);
        if (mStarted == false) {
            return false;
        }
    }

    int16_t samples_per_channel;
#ifdef PLAY_USE_WEBRTC_NETEQ
    int error = WebrtcNeteqRecOut(mNeteqInst, (int16_t *)mPcmBuffer, &samples_per_channel);
    if (error != 0) {
        ALOGE("WebRtcNetEQ_RecOut returned error");
        return true;
    }
#else
    AudioBuffer *buffer = mBufferManager->getFirstAudioBuffer();
    if (buffer) {
        samples_per_channel = mDecoder->decode((int16_t *)mPcmBuffer, buffer->data, buffer->size);
        mBufferManager->pushEmptyBuffer(buffer);
    } else {
        samples_per_channel = mDecoder->decodePLC((int16_t *)mPcmBuffer, 1);
    }

    if (samples_per_channel <= 0) {
        ALOGW("audio decode error");
        samples_per_channel = mSampleCount;
        memset(mPcmBuffer, 0, mSampleCount * mFrameSize);
        //return true;
    }
#endif

    ALOGV("samples_per_channel is %d", samples_per_channel);

    ssize_t bytesWrite = mAudioTrack->write((char *)mPcmBuffer, samples_per_channel * sizeof(int16_t));
    if (bytesWrite != (ssize_t)(samples_per_channel * sizeof(int16_t))) {
        ALOGW("bytesWrite is %ld", bytesWrite);
    }

    aec_player_add_echo_data(mPcmBuffer, samples_per_channel * sizeof(int16_t));

    return true;
}

status_t SoundPlayer::rtspFeedAudioData(char *buffer, unsigned int len, long long pts)
{
    Mutex::Autolock autoLock(mLock);

    if (!mStarted || (!buffer || len == 0)) {
        ALOGE("rtspFeedAudioData: mStarted(%d), buffer(%p), len(%d)", mStarted, buffer, len);
        return UNKNOWN_ERROR;
    }

    ALOGV("rtspFeedAudioData: len(%d), pts(%lld)", len, pts);

#ifdef PLAY_USE_WEBRTC_NETEQ
    PacketInfo_t info;
    info.sequenceNumber  = mSequenceNumber;  //pBuffer->nPktSeqNumber;
    info.timeStamp       = mSequenceNumber * mDuration;
    info.payloadPtr      = (uint8_t *)buffer;
    info.payloadLenBytes = len;
    // Insert packet.
    int error = WebrtcNeteqRecIn(mNeteqInst, &info);
    if (error != 0) {
        return -1;
    }
#else
    if (len > mBufferSize) {
        ALOGE("rtspFeedAudioData: len(%d) larger than mBufferSize(%d)",
            len, mBufferSize);
        return -1;
    }
    AudioBuffer * ABuffer = mBufferManager->getFirstEmptyBuffer();
    if (ABuffer == NULL) {
        ALOGE("rtspFeedAudioData: ABuffer is NULL");
        return -1;
    }
    memcpy(ABuffer->data, buffer, len);
    ABuffer->size = len;
    ABuffer->pts = mSequenceNumber;

    mBufferManager->pushAudioBuffer(ABuffer);
#endif

    //tutk interface hasn't correct pts
    mSequenceNumber += pts / (mDuration * 1000);

    if (mFirstPacket) {
        pthread_mutex_lock(&mInMutex);
        pthread_cond_signal(&mInCond);
        mFirstPacket = false;
        pthread_mutex_unlock(&mInMutex);
    }

    return 0;
}

status_t SoundPlayer::init(void *aec, CodecParams *params)
{
    Mutex::Autolock autoLock(mLock);

    if (mInitCheck) {
        ALOGE("sound player had already been init.");
        return UNKNOWN_ERROR;
    }

    if (params->channels != 1
        || (params->sampleRate != 8000 && params->sampleRate != 16000)) {
        ALOGE("sampleRate(%d), channels(%d)",params->sampleRate, params->channels);
        return UNKNOWN_ERROR;
    }

    mSampleRate = params->sampleRate;
    mChannels   = params->channels;

    mDuration = audioGetPacketDuration(DecoderName2DecoderType(params->name));
    if (mDuration < 0) {
        ALOGE("audioGetPacketDuration failed");
        return UNKNOWN_ERROR;
    }

    status_t ret = createAudioTrack(mSampleRate, mChannels);
    if (ret != OK) {
        ALOGE("create createAudioTrack failed.");
        return ret;
    }

#ifndef PLAY_USE_WEBRTC_NETEQ
    ret = audioDecoderInit(params);
    if (ret != OK) {
        ALOGE("create audio decoder failed.");
        delete mAudioTrack;
        mAudioTrack = NULL;
        return UNKNOWN_ERROR;
    }
    uint32_t kOutDataLen = kOutputBlockSizeMs * kMaxSamplesPerMs * kMaxChannels;
#else
    int sample_rate_khz = mSampleRate / 1000;
    int frameSize = sizeof(int16_t) * mChannels;
    uint32_t kOutDataLen = 10 * sample_rate_khz * frameSize; //neteq: 10ms
#endif

    mPcmBuffer = new char[kOutDataLen];
    mAec = aec;

    mDecName = strdup(params->name);
    mDecMode = params->mode;

    mInitCheck = true;
    return 0;
}

status_t SoundPlayer::start()
{
    Mutex::Autolock autoLock(mLock);

    if (mStarted) {
        return UNKNOWN_ERROR;
    }

    if(mInitCheck != true) {
        return NO_INIT;
    }

#ifdef PLAY_USE_WEBRTC_NETEQ
    NeteqParam params;
    params.type        = DecoderName2DecoderType(mDecName);
    params.sampleRate  = mSampleRate;
    params.channels    = mChannels;
    params.mode        = mDecMode;
    status_t ret = WebrtcNeteqInit(&mNeteqInst, &params);
    if (ret != OK) {
        ALOGE("webrtc_neteq_init failed.");
        return UNKNOWN_ERROR;
    }
#else
    mBufferManager->bufferFlush();
#endif

    mSequenceNumber = 0;
    mFirstPacket = true;

    mAudioTrack->start();

    if (!mPlayThread->start()) {
        mAudioTrack->stop();
#ifdef PLAY_USE_WEBRTC_NETEQ
        WebrtcNeteqDeInit(mNeteqInst);
#endif
        return UNKNOWN_ERROR;
    }

    aec_player_set_state(STREAM_STARTED);

    mStarted = true;
    return OK;
}

status_t SoundPlayer::stop_l()
{
    if (!mStarted) {
        return UNKNOWN_ERROR;
    }
    mStarted = false;

    pthread_mutex_lock(&mInMutex);
    pthread_cond_signal(&mInCond);
    pthread_mutex_unlock(&mInMutex);

    mPlayThread->stop();

#ifdef PLAY_USE_WEBRTC_NETEQ
    WebrtcNeteqDeInit(mNeteqInst);
#else
    mBufferManager->bufferFlush();
#endif

    aec_player_set_state(STREAM_STOPED);

    mAudioTrack->stop();

    return OK;
}

status_t SoundPlayer::stop()
{
    Mutex::Autolock autoLock(mLock);
    return stop_l();
}

status_t SoundPlayer::exit()
{
    Mutex::Autolock autoLock(mLock);

    if(mInitCheck != true) {
        return NO_INIT;
    }
    mInitCheck = false;

    stop_l();

    delete mAudioTrack;
    mAudioTrack = NULL;

#ifndef PLAY_USE_WEBRTC_NETEQ
    audioDecoderExit();
#endif
    free(mDecName);

    delete [] mPcmBuffer;
    mPcmBuffer = NULL;
    return OK;
}

