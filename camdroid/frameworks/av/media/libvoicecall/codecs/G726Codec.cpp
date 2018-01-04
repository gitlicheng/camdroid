/*
 * Author: yewenliang@allwinnertech.com
 * Copyright (C) 2014 Allwinner Technology Co., Ltd.
 */

#define LOG_TAG "G726Codec"
//#define LOG_NDEBUG 0
//#include <cutils/log.h>
#include "dbg_log.h"

#include <string.h>
#include "AudioCodec.h"

extern "C" {
//#include "g72x.h"
#include "codec_g726.h"
}

#include "lowcfe.h"

#define SAMPLE_RATE 8000
#define PTIME 20
#define SAMPLES_PER_FRAME ((SAMPLE_RATE * PTIME) / 1000)
#define PCM_FRAME_SIZE (2 * SAMPLES_PER_FRAME)

#define G726_BITRATE_16K    16000
#define G726_BITRATE_24K    24000
#define G726_BITRATE_32K    32000
#define G726_BITRATE_40K    40000

#define PLC_SAMPLE      80      /*10ms*/

//typedef int (*decode_func_t) (int, int, g726_state*);
//typedef int (*encode_func_t) (int, int, g726_state*);

class G726Encoder : public AudioEncoder
{
public:
    G726Encoder() {
        mEncoder = NULL;
    }

    ~G726Encoder() {
        if (mEncoder) {
            g726_state_destroy(&mEncoder);
        }
    }

    int set(int sampleRate, const int channels, const int mode);
    int encode(void *payload, int16_t *samples);
    int getEncodedFrameMaxSize();

private:
    unsigned char * mEncoder;
    uint32_t        mBitRate;
    uint32_t        mEncodedFrameSize;
};

int G726Encoder::set(int sampleRate, const int channels, const int mode)
{
    int ret;
    
    if (sampleRate != SAMPLE_RATE || channels != 1) {
        return -1;
    }
 
    switch (mode) {
    case G726_16:
        mBitRate = G726_16;
        mEncodedFrameSize = (SAMPLES_PER_FRAME * 2) / 8;
        break;
    case G726_24:
        mBitRate = G726_24;
        mEncodedFrameSize = (SAMPLES_PER_FRAME * 3) / 8;
        break;
    case G726_32:
        mBitRate = G726_32;
        mEncodedFrameSize = (SAMPLES_PER_FRAME * 4) / 8;
        break;
    case G726_40:
        mBitRate = G726_40;
        mEncodedFrameSize = (SAMPLES_PER_FRAME * 5) / 8;
        break;
    default:
        return -1;
    }

    ret = g726_state_create(mBitRate, AUDIO_ENCODING_LINEAR, &mEncoder);
    if (ret != TRUE) {
        return -1;
    }

    ALOGV("mEncodedFrameSize is %d", mEncodedFrameSize);

    return SAMPLES_PER_FRAME;
}

int G726Encoder::encode(void *payload, int16_t *samples)
{
    int ret;
    unsigned long outLen;

    outLen = mEncodedFrameSize;
    ret = g726_encode(mEncoder, (unsigned char *)samples, PCM_FRAME_SIZE,
                (unsigned char *)payload, &outLen);
    if (ret != PCM_FRAME_SIZE || outLen != mEncodedFrameSize) {
        ALOGE("encode: ret(%d), outLen(%ld)", ret, outLen);
        return -1;
    }

    return outLen;
}

int G726Encoder::getEncodedFrameMaxSize()
{
    return mEncodedFrameSize;
}

G726Encoder *newG726Encoder()
{
    ALOGD("newG726Encoder");
    return new G726Encoder;
}

#if 1
class G726Decoder : public AudioDecoder
{
public:
    G726Decoder() {
        mDecoder = NULL;
        mPLC = g711plc_init(SAMPLE_RATE, PLC_SAMPLE);
    }

    ~G726Decoder() {
        if (mDecoder) {
            g726_state_destroy(&mDecoder);
        }
        if (mPLC) {
            g711plc_destroy(mPLC);
        }
    }

    int set(int sampleRate, const int channels, const int mode);
	int decode(int16_t *samples, void *payload, int length);
    int decodePLC(int16_t *samples, int16_t number_of_lost_frames);
	int getEncodedFrameMaxSize();

private:
    unsigned char * mDecoder;
    uint32_t        mBitRate;
    uint32_t        mEncodedFrameSize;
    LowcFE_c *      mPLC;
};

int G726Decoder::set(int sampleRate, const int channels, const int mode)
{
    int ret;

    if (sampleRate != SAMPLE_RATE || channels != 1) {
        return -1;
    }
 
    switch (mode) {
    case G726_16:
        mBitRate = G726_16;
        mEncodedFrameSize = (SAMPLES_PER_FRAME * 2) / 8;
        break;
    case G726_24:
        mBitRate = G726_24;
        mEncodedFrameSize = (SAMPLES_PER_FRAME * 3) / 8;
        break;
    case G726_32:
        mBitRate = G726_32;
        mEncodedFrameSize = (SAMPLES_PER_FRAME * 4) / 8;
        break;
    case G726_40:
        mBitRate = G726_40;
        mEncodedFrameSize = (SAMPLES_PER_FRAME * 5) / 8;
        break;
    default:
        return -1;
    }

    ret = g726_state_create(mBitRate, AUDIO_ENCODING_LINEAR, &mDecoder);
    if (ret != TRUE) {
        ALOGE("g726_state_create failed.");
        return -1;
    }

    ALOGV("mEncodedFrameSize is %d", mEncodedFrameSize);

    return SAMPLES_PER_FRAME;
}


int G726Decoder::decode(int16_t *samples, void *payload, int length)
{
    int ret, i;
    unsigned long outLen;

    if (length != mEncodedFrameSize) {
        ALOGE("decode: length(%d)", length);
        return -1;
    }

    outLen = PCM_FRAME_SIZE;
    ret = g726_decode(mDecoder, (unsigned char *)payload, length,
                (unsigned char *)samples, &outLen);
    if (ret != length || outLen != PCM_FRAME_SIZE) {
        ALOGE("decode: ret(%d), outLen(%ld)", ret, outLen);
        return -1;
    }

    for(i = 0; i < 2; i++) {
        g711plc_addtohistory(mPLC, (short *)samples + i * PLC_SAMPLE);
    }

    return (outLen / 2);
}

int G726Decoder::decodePLC(int16_t *samples, int16_t number_of_lost_frames)
{
    int16_t i = 0, j = 0;
    int decoded_samples = 0;

    for (i = 0; i < number_of_lost_frames; i++) {
        samples += i * 160;//(PLC_SAMPLE * 2);
        for(j = 0; j < 2; j++) {
            g711plc_dofe(mPLC, (short *)samples + j * PLC_SAMPLE);
        }
    }

    decoded_samples = number_of_lost_frames * 160;//PLC_SAMPLE * 2;

    ALOGV("decodePLC: decoded_samples(%d), number_of_lost_frames(%d)",
        decoded_samples, number_of_lost_frames);

    return decoded_samples;
}

int G726Decoder::getEncodedFrameMaxSize()
{
    return mEncodedFrameSize;
}

G726Decoder *newG726Decoder()
{
    return new G726Decoder;
}
#endif
