/*
 * Author: yewenliang@allwinnertech.com
 * Copyright (C) 2014 Allwinner Technology Co., Ltd.
 */

#define LOG_TAG "SpeexCodec"
//#define LOG_NDEBUG 0
//#include <cutils/log.h>
#include "dbg_log.h"

#include <string.h>
#include "AudioCodec.h"

#include "speex/speex.h"

#define SAMPLE_RATE 8000
#define PTIME 20
#define SAMPLES_PER_FRAME ((SAMPLE_RATE * PTIME) / 1000)
#define PCM_FRAME_SIZE (2 * SAMPLES_PER_FRAME)

//+------+---------------+-------------+
//| mode | Speex quality |   bit-rate  |
//+------+---------------+-------------+
//|   1  |       0       | 2.15 kbit/s |
//|   2  |       2       | 5.95 kbit/s |
//|   3  |     3 or 4    | 8.00 kbit/s |
//|   4  |     5 or 6    | 11.0 kbit/s |
//|   5  |     7 or 8    | 15.0 kbit/s |
//|   6  |       9       | 18.2 kbit/s |
//|   7  |      10       | 24.6 kbit/s |
//|   8  |       1       | 3.95 kbit/s |
//+------+---------------+-------------+
static unsigned const ModeTableNB[9] = {
  2150, 5950, 8000, 11000,
  15000, 18200, 24600, 3950
};

//+------+---------------+-------------------+------------------------+
//| mode | Speex quality | wideband bit-rate |     ultra wideband     |
//|      |               |                   |        bit-rate        |
//+------+---------------+-------------------+------------------------+
//|   0  |       0       |    3.95 kbit/s    |       5.75 kbit/s      |
//|   1  |       1       |    5.75 kbit/s    |       7.55 kbit/s      |
//|   2  |       2       |    7.75 kbit/s    |       9.55 kbit/s      |
//|   3  |       3       |    9.80 kbit/s    |       11.6 kbit/s      |
//|   4  |       4       |    12.8 kbit/s    |       14.6 kbit/s      |
//|   5  |       5       |    16.8 kbit/s    |       18.6 kbit/s      |
//|   6  |       6       |    20.6 kbit/s    |       22.4 kbit/s      |
//|   7  |       7       |    23.8 kbit/s    |       25.6 kbit/s      |
//|   8  |       8       |    27.8 kbit/s    |       29.6 kbit/s      |
//|   9  |       9       |    34.2 kbit/s    |       36.0 kbit/s      |
//|  10  |       10      |    42.2 kbit/s    |       44.0 kbit/s      |
//+------+---------------+-------------------+------------------------+
static unsigned const ModeTableWB[11] = {
  3950, 5750, 7750, 9800,
  12800, 16800, 20600, 23800,
  27800, 34200, 42200
};

static unsigned const ModeTableUWB[11] = {
  5750, 7550, 9550, 11600,
  14600, 18600, 22400, 25600,
  29600, 36000, 44000
};

class SpeexEncoder : public AudioEncoder
{
public:
    SpeexEncoder() {
        mEncoder = NULL;
        mVbr = 0;
        mCng = 0;
    }

    ~SpeexEncoder() {
        if (mEncoder) {
            g726_state_destroy(&mEncoder);
        }
    }

    int set(int sampleRate, const int channels, const int mode);
    int encode(void *payload, int16_t *samples);
    int getEncOutputMaxSize();

private:
    void *          mEncoder;       //state
    int             mVbr;
    int             mCng;
    int             mMode;
    int             mBitRate;
    int             mFrameSize;
    uint32_t        mEncodedFrameMaxSize;
};

int SpeexEncoder::set(int sampleRate, const int channels, const int mode)
{
    int pps;
	const SpeexMode *speexMode = NULL;
  	int _mode = 0;

	switch (sampleRate) {
    case 8000:
        _mode = SPEEX_MODEID_NB;    /* rate = 8000Hz */
        break;
    case 16000:
        _mode = SPEEX_MODEID_WB;    /* rate = 16000Hz */
        break;
        /* should be supported in the future */
    case 32000:
        _mode = SPEEX_MODEID_UWB;   /* rate = 32000Hz */
        break;
    default:
        ALOGE("Unsupported rate for speex encoder (back to default rate=8000).");
        _mode = SPEEX_MODEID_NB;    /* rate = 8000Hz */
	}

    /* warning: speex_lib_get_mode() is not available on speex<1.1.12 */
	speexMode = speex_lib_get_mode(_mode);
	if (speexMode == NULL) {
		return -1;
    }

	if (mVbr == 1) {
		if (speex_encoder_ctl(mEncoder, SPEEX_SET_VBR, &mVbr) != 0) {
			ALOGE("Could not set vbr mode to speex encoder.");
		}
		/* implicit VAD */
		speex_encoder_ctl (mEncoder, SPEEX_SET_DTX, &mVbr);
	} else if (mVbr == 2) {
		int vad = 1;
		/* VAD */
		speex_encoder_ctl (mEncoder, SPEEX_SET_VAD, &vad);
		speex_encoder_ctl (mEncoder, SPEEX_SET_DTX, &vad);
	} else if (mCng == 1) {
		speex_encoder_ctl (mEncoder, SPEEX_SET_VAD, &mCng);
	}

	mEncoder = speex_encoder_init(speexMode);
	if (sampleRate == 8000) {
		if (mode = 0 || mode > 8) {
			mode = 3; /* default mode */
        }
        mBitRate = ModeTableNB[mode];

		if (mBitRate != -1) {
			if (speex_encoder_ctl(mEncoder, SPEEX_SET_BITRATE, &mBitRate) != 0){
				ALOGE("Could not set bitrate %i to speex encoder.", mBitRate);
			}
		}
	} else if (sampleRate == 16000 || sampleRate == 32000) {
		int q = 0;
		if (mode < 0 || mode > 10) {
			mode = 8; /* default mode */
        }
		q = mode;

		if (speex_encoder_ctl(mEncoder, SPEEX_SET_QUALITY, &q) != 0){
			ALOGE("Could not set quality %i to speex encoder.", q);
		}
	}

    pps = 1000 / PTIME; //mPtime;
    if (mVbr != 1) {
        mEncodedFrameMaxSize = mBitRate / pps;
    } else {
        mEncodedFrameMaxSize = 3 * mBitRate / pps;      //multiple 3
    }

    speex_mode_query(speexMode, SPEEX_MODE_FRAME_SIZE, &mFrameSize);

    ALOGD("mFrameSize is %d", mFrameSize);
    return mFrameSize;  //?
}

int SpeexEncoder::encode(void *payload, int16_t *samples)
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

    if (outLen > mEncodedFrameMaxSize) {
        ALOGE("encode: outLen(%d) larger than %d", outLen, mEncodedFrameMaxSize);
    }
    
    return outLen;
}

int SpeexEncoder::getEncOutputMaxSize()
{
    return mEncodedFrameMaxSize;
}

SpeexEncoder *newSpeexEncoder()
{
    ALOGD("newSpeexEncoder");
    return new SpeexEncoder;
}

#if 0
class SpeexDecoder : public AudioDecoder
{
public:
    SpeexDecoder() {
        mDecoder = NULL;
        mPLC = g711plc_init(SAMPLE_RATE, PLC_SAMPLE);
    }

    ~SpeexDecoder() {
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
	int getDecInputMaxSize();

private:
    unsigned char * mDecoder;
    uint32_t        mBitRate;
    uint32_t        mEncodedFrameSize;
    LowcFE_c *      mPLC;
};

int SpeexDecoder::set(int sampleRate, const int channels, const int mode)
{
    int ret;

    if (sampleRate != SAMPLE_RATE || channels != 1) {
        return -1;
    }

    switch (mode) {
    case G726_BITRATE_16K:
        mBitRate = G726_16;
        mEncodedFrameSize = (SAMPLES_PER_FRAME * 2) / 8;
        break;
    case G726_BITRATE_24K:
        mBitRate = G726_24;
        mEncodedFrameSize = (SAMPLES_PER_FRAME * 3) / 8;
        break;
    case G726_BITRATE_32K:
        mBitRate = G726_32;
        mEncodedFrameSize = (SAMPLES_PER_FRAME * 4) / 8;
        break;
    case G726_BITRATE_40K:
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


int SpeexDecoder::decode(int16_t *samples, void *payload, int length)
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

int SpeexDecoder::decodePLC(int16_t *samples, int16_t number_of_lost_frames)
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

int SpeexDecoder::getDecInputMaxSize()
{
    return mEncodedFrameSize;
}

SpeexDecoder *newSpeexDecoder()
{
    return new SpeexDecoder;
}
#endif
