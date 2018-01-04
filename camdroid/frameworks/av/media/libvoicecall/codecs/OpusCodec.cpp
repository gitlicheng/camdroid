/*
 * Author: yewenliang@allwinnertech.com
 * Copyright (C) 2014 Allwinner Technology Co., Ltd.
 */

#define LOG_TAG "OpusCodec"
//#define LOG_NDEBUG 0
//#include <cutils/log.h>
#include "dbg_log.h"
#include <string.h>
#include "AudioCodec.h"

//#include "modules/audio_coding/codecs/opus/interface/opus_interface.h"
#include "opus.h"

#define FRAME_LENGTH            20 // ptime may be 20, 40, 60, 80, 100 or 120, packets composed of multiples 20ms frames
#define MAX_BYTES_PER_FRAME     200 //500 // Equals peak bitrate of 200 kbps

enum {
  /* Maximum supported frame size in WebRTC is 60 ms. */
  kOpusMaxEncodeFrameSizeMs = 60,

  /* The format allows up to 120 ms frames. Since we don't control the other
   * side, we must allow for packets of that size. NetEq is currently limited
   * to 60 ms on the receive side. */
  kOpusMaxDecodeFrameSizeMs = 120,

  /* Maximum sample count per channel is 48 kHz * maximum frame size in
   * milliseconds. */
  kOpusMaxFrameSizePerChannel = 48 * kOpusMaxDecodeFrameSizeMs,

  /* Maximum sample count per frame is 48 kHz * maximum frame size in
   * milliseconds * maximum number of channels. */
  kOpusMaxFrameSize = kOpusMaxFrameSizePerChannel * 2,

  /* Maximum sample count per channel for output resampled to 32 kHz,
   * 32 kHz * maximum frame size in milliseconds. */
  kOpusMaxFrameSizePerChannel32kHz = 32 * kOpusMaxDecodeFrameSizeMs,

  /* Number of samples in resampler state. */
  kOpusStateSize = 7,

  /* Default frame size, 20 ms @ 48 kHz, in samples (for one channel). */
  kOpusDefaultFrameSize = 960,
};

typedef struct __OpusEncInst {
  struct OpusEncoder* encoder;
} OpusEncInst;

int16_t Opus_EncoderCreate(OpusEncInst** inst, int32_t sampleRate, int32_t channels) {
  OpusEncInst* state;
  if (inst != NULL) {
    state = (OpusEncInst*) calloc(1, sizeof(OpusEncInst));
    if (state) {
      int error;
      /* Default to VoIP application for mono, and AUDIO for stereo. */
      int application = (channels == 1) ? OPUS_APPLICATION_VOIP :
          OPUS_APPLICATION_AUDIO;

      state->encoder = opus_encoder_create(sampleRate, channels, application,        //48000
                                           &error);
      if (error == OPUS_OK && state->encoder != NULL) {
        *inst = state;
        return 0;
      }
      free(state);
    }
  }
  return -1;
}

int16_t Opus_EncoderFree(OpusEncInst* inst) {
  if (inst) {
    opus_encoder_destroy(inst->encoder);
    free(inst);
    return 0;
  } else {
    return -1;
  }
}

int16_t Opus_Encode(OpusEncInst* inst, int16_t* audio_in, int16_t samples,
                          int16_t length_encoded_buffer, uint8_t* encoded) {
  opus_int16* audio = (opus_int16*) audio_in;
  unsigned char* coded = encoded;
  int res;

  if (samples > 48 * kOpusMaxEncodeFrameSizeMs) {
    return -1;
  }

  res = opus_encode(inst->encoder, audio, samples, coded,
                    length_encoded_buffer);
  ALOGV("WebRtcOpus_Encode: res(%d)", res);
  if (res > 0) {
    return res;
  }
  return -1;
}

int16_t Opus_SetBitRate(OpusEncInst* inst, int32_t rate) {
  if (inst) {
    return opus_encoder_ctl(inst->encoder, OPUS_SET_BITRATE(rate));
  } else {
    return -1;
  }
}

int16_t Opus_SetPacketLossRate(OpusEncInst* inst, int32_t loss_rate) {
  if (inst) {
    return opus_encoder_ctl(inst->encoder,
                            OPUS_SET_PACKET_LOSS_PERC(loss_rate));
  } else {
    return -1;
  }
}

int16_t Opus_EnableVbr(OpusEncInst* inst) {
  if (inst) {
    return opus_encoder_ctl(inst->encoder, OPUS_SET_VBR(1));
  } else {
    return -1;
  }
}

int16_t Opus_DisableVbr(OpusEncInst* inst) {
  if (inst) {
    return opus_encoder_ctl(inst->encoder, OPUS_SET_VBR(0));
  } else {
    return -1;
  }
}

int16_t Opus_EnableFec(OpusEncInst* inst) {
  if (inst) {
    return opus_encoder_ctl(inst->encoder, OPUS_SET_INBAND_FEC(1));
  } else {
    return -1;
  }
}

int16_t Opus_DisableFec(OpusEncInst* inst) {
  if (inst) {
    return opus_encoder_ctl(inst->encoder, OPUS_SET_INBAND_FEC(0));
  } else {
    return -1;
  }
}

int16_t Opus_SetComplexity(OpusEncInst* inst, int32_t complexity) {
  if (inst) {
    return opus_encoder_ctl(inst->encoder, OPUS_SET_COMPLEXITY(complexity));
  } else {
    return -1;
  }
}

class OpusEncoder : public AudioEncoder
{
public:
    OpusEncoder() {
        mEncoder = NULL;
    }

    ~OpusEncoder() {
        if (mEncoder) {
            Opus_EncoderFree(mEncoder);
        }
    }

    int set(int sampleRate, const int channels, const int mode);
    int encode(void *payload, int16_t *samples);
    int getEncodedFrameMaxSize();

private:
    OpusEncInst *mEncoder;
    int32_t mSampleRate;
    int32_t mChannels;
};

int OpusEncoder::set(int sampleRate, const int channels, const int mode)
{
    int ret;

    mSampleRate = sampleRate;
    mChannels = channels;

    ret = Opus_EncoderCreate(&mEncoder, sampleRate, channels);
    if (ret < 0) {
        return -1;
    }

    ret = Opus_SetComplexity(mEncoder, 0);
    if (ret < 0) {
        return -1;
    }

    ret = Opus_SetPacketLossRate(mEncoder, 10);
    if (ret < 0) {
        return -1;
    }

    ret = Opus_EnableVbr(mEncoder);
    if (ret < 0) {
        return -1;
    }

    return (FRAME_LENGTH * mSampleRate / 1000);
}

int OpusEncoder::encode(void *payload, int16_t *samples)
{
    int frame_size = mSampleRate * FRAME_LENGTH / 1000; /* in samples */

    int ret_size = Opus_Encode(mEncoder, samples, frame_size, MAX_BYTES_PER_FRAME, (uint8_t *)payload);

    if (ret_size < 0) {
        return -1;
    }

    //ALOGE("ret_size is %d", ret_size);

    return ret_size;
}

int OpusEncoder::getEncodedFrameMaxSize()
{
    return MAX_BYTES_PER_FRAME;
}

OpusEncoder *newOpusEncoder()
{
    return new OpusEncoder;
}
