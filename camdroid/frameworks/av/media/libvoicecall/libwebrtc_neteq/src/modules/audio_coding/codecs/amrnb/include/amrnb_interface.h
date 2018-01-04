/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_AMRNB_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_AMRNB_H_
/*
 * Define the fixpoint numeric formats
 */

#include "typedefs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WebRtcAMRDecInst AMR_decinst_t_;

int16_t WebRtcAmr_CreateDec(AMR_decinst_t_** dec_inst, int16_t mode);

int16_t WebRtcAmr_FreeDec(AMR_decinst_t_* dec_inst);

int16_t WebRtcAmr_DecoderInit(void* state);

int16_t WebRtcAmr_Decode(void* state, const int16_t* encoded,
                          int16_t encoded_bytes, int16_t* decoded,
                          int16_t* audio_type);

int16_t WebRtcAmr_DecodePlc(void* state, int16_t* decoded,
                             int16_t number_of_lost_frames);

#ifdef __cplusplus
}
#endif

#endif /* PCM16B */
