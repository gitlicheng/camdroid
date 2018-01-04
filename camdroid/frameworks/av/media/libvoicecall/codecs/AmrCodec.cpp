/*
 * Copyrightm (C) 2010 The Android Open Source Project
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
#define LOG_TAG "AmrCodec"
//#define LOG_NDEBUG 0
//#include <cutils/log.h>
#include "dbg_log.h"

#include <string.h>

#include "AudioCodec.h"

#include "gsmamr_dec.h"
#include "gsmamr_enc.h"


const int gFrameBits[8] = {95, 103, 118, 134, 148, 159, 204, 244};
static const uint8_t frame_sizes_nb[16] = {
    12, 13, 15, 17, 19, 20, 26, 31, 5, 0, 0, 0, 0, 0, 0, 0
};

static const int MAX_OUTPUT_BUFFER_SIZE = 32;

//------------------------------------------------------------------------------

// See RFC 4867 for the encoding details.

class AmrEncoder : public AudioEncoder
{
public:
    AmrEncoder() {
        if (AMREncodeInit(&mEncoder, &mSidSync, false)) {
            mEncoder = NULL;
        }
        mMode = 0;
    }

    ~AmrEncoder() {
        if (mEncoder) {
            AMREncodeExit(&mEncoder, &mSidSync);
        }
    }

    int set(int sampleRate, const int channels, const int mode);
    int encode(void *payload, int16_t *samples);
    int getEncodedFrameMaxSize();

private:
    void *          mEncoder;
    void *          mSidSync;
    int             mMode;
};

int AmrEncoder::set(int sampleRate, const int channels, const int mode)
{
    mMode = mode;
    return (sampleRate == 8000 && channels == 1 && mEncoder) ? 160 : -1;
}

int AmrEncoder::encode(void *payload, int16_t *samples)
{
    unsigned char *bytes = (unsigned char *)payload;
    Frame_Type_3GPP type;

    int length = AMREncode(mEncoder, mSidSync, (Mode)mMode,
        samples, bytes, &type, AMR_TX_WMF);

    if (type != mMode) {
        return -1;
    }

    if (length > 0) {
      bytes[0] = (bytes[0] << 3) | 0x4;
    }

    return length;
}

int AmrEncoder::getEncodedFrameMaxSize()
{
    return MAX_OUTPUT_BUFFER_SIZE;
}

AmrEncoder *newAmrEncoder()
{
    return new AmrEncoder;
}

#if 1
#include "lowcfe.h"
#define SAMPLE_RATE     8000
#define PLC_SAMPLE      80      /*10ms*/

class AmrDecoder : public AudioDecoder
{
public:
    AmrDecoder() {
        if (GSMInitDecode(&mDecoder, (Word8 *)"RTP")) {
            mDecoder = NULL;
        }
        mPLC = g711plc_init(SAMPLE_RATE, PLC_SAMPLE);
        mMode = 0;
    }

    ~AmrDecoder() {
        if (mDecoder) {
            GSMDecodeFrameExit(&mDecoder);
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
    void *          mDecoder;
    LowcFE_c *      mPLC;
    int             mMode;
};


int AmrDecoder::set(int sampleRate, const int channels, const int mode)
{
    mMode = mode;
    if (mMode > 7 || mMode < 0) {
        mMode = 0;                  //default mode
    }
    return (sampleRate == 8000 && channels == 1 && mDecoder) ? 160 : -1;
}

int AmrDecoder::decode(int16_t *samples, void *payload, int length)
{
    int i;
    unsigned char *bytes = (unsigned char *)payload;
    Frame_Type_3GPP type;

    if (length < 2) {
        return -1;
    }

    if (length != frame_sizes_nb[mMode]) {
        type = (Frame_Type_3GPP)(bytes[0] >> 3);
        if (length != (frame_sizes_nb[type] + 1)) {
            ALOGE("decode: length(%d), type(%d)", length, type);
            return -1;
        }
        //mMode = type;
        ++bytes;
        --length;
    } else {
        type = (Frame_Type_3GPP)mMode;
    }

    if (AMRDecode(mDecoder, type, bytes, samples, MIME_IETF) != length) {
        ALOGE("AMRDecode return error, length(%d), type(%d)", length, type);
        return -1;
    }

    for(i = 0; i < 2; i++) {
        g711plc_addtohistory(mPLC, (short *)samples + i * PLC_SAMPLE);
    }

    return 160;
}

int AmrDecoder::decodePLC(int16_t *samples, int16_t number_of_lost_frames)
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

int AmrDecoder::getEncodedFrameMaxSize()
{
    return MAX_OUTPUT_BUFFER_SIZE;
}

AmrDecoder *newAmrDecoder()
{
    return new AmrDecoder;
}
#endif

//------------------------------------------------------------------------------
#if 0
// See RFC 3551 for the encoding details.
class GsmEfrCodec : public AudioCodec
{
public:
    GsmEfrCodec() {
        if (AMREncodeInit(&mEncoder, &mSidSync, false)) {
            mEncoder = NULL;
        }
        if (GSMInitDecode(&mDecoder, (Word8 *)"RTP")) {
            mDecoder = NULL;
        }
    }

    ~GsmEfrCodec() {
        if (mEncoder) {
            AMREncodeExit(&mEncoder, &mSidSync);
        }
        if (mDecoder) {
            GSMDecodeFrameExit(&mDecoder);
        }
    }

    int set(int sampleRate, const int mode) {
        return (sampleRate == 8000 && mEncoder && mDecoder) ? 160 : -1;
    }

    int encode(void *payload, int16_t *samples);
    int decode(int16_t *samples, int count, void *payload, int length);

private:
    void *mEncoder;
    void *mSidSync;
    void *mDecoder;
};

int GsmEfrCodec::encode(void *payload, int16_t *samples)
{
    unsigned char *bytes = (unsigned char *)payload;
    Frame_Type_3GPP type;

    int length = AMREncode(mEncoder, mSidSync, MR122,
        samples, bytes, &type, AMR_TX_WMF);

    if (type == AMR_122 && length == 32) {
        bytes[0] = 0xC0 | (bytes[1] >> 4);
        for (int i = 1; i < 31; ++i) {
            bytes[i] = (bytes[i] << 4) | (bytes[i + 1] >> 4);
        }
        return 31;
    }
    return length;
}

int GsmEfrCodec::decode(int16_t *samples, int count, void *payload, int length)
{
    unsigned char *bytes = (unsigned char *)payload;
    int n = 0;
    while (n + 160 <= count && length >= 31 && (bytes[0] >> 4) == 0x0C) {
        for (int i = 0; i < 30; ++i) {
            bytes[i] = (bytes[i] << 4) | (bytes[i + 1] >> 4);
        }
        bytes[30] <<= 4;

        if (AMRDecode(mDecoder, AMR_122, bytes, &samples[n], MIME_IETF) != 31) {
            break;
        }
        n += 160;
        length -= 31;
        bytes += 31;
    }
    return n;
}
#endif

