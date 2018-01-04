/*
 * Author: yewenliang@allwinnertech.com
 * Copyright (C) 2014 Allwinner Technology Co., Ltd.
 */

#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_G726_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_G726_H_

#include "typedefs.h"

#include "codec_g726.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WebrtcG726Inst G726Inst;

int16_t WebRtcG726_CreateDecoder(G726Inst **g726dec_inst, int mode);

int16_t WebRtcG726_FreeDecoder(G726Inst *g726dec_inst);

int16_t WebRtcG726_DecoderInit(void* state);

int16_t WebRtcG726_Decode(void* state, const int16_t* encoded,
                          int16_t encoded_bytes, int16_t* decoded,
                          int16_t* audio_type);

int16_t WebRtcG726_DecodePlc(void* state,  int16_t* decoded,
                             int16_t number_of_lost_frames);

#ifdef __cplusplus
}
#endif

#endif /* WEBRTC_MODULES_AUDIO_CODING_CODECS_G726_H_ */
