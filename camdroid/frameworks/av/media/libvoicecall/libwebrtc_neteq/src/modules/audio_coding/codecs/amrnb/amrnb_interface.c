/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

//#include "modules/audio_coding/codecs/amrnb/interface/amrnb_interface.h"
//#define LOG_NDEBUG 0
#define LOG_TAG "Amrnb_Interface"
//#include <utils/Log.h>
#include "dbg_log.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "amrnb_interface.h"
#include "gsmamr_dec.h"
#include "lowcfe.h"

struct WebRtcAMRDecInst {
    void *          decoder;
    int             mode;
    LowcFE_c *      plc;

};

#define AMR_SAMPLERATE  8000
#define PLC_SAMPLE      80      /*10ms*/

const int gFrameBits[8] = {95, 103, 118, 134, 148, 159, 204, 244};

static const uint8_t frame_sizes_nb[16] = {
    12, 13, 15, 17, 19, 20, 26, 31, 5, 0, 0, 0, 0, 0, 0, 0
};


int16_t WebRtcAmr_CreateDec(AMR_decinst_t_** dec_inst, int16_t mode)
{
    AMR_decinst_t_* state;

    if (dec_inst != NULL) {
        /* Create Opus decoder state. */
        state = (AMR_decinst_t_ *) calloc(1, sizeof(AMR_decinst_t_));
        if (state == NULL) {
            return -1;
        }
        state->decoder = NULL;
        state->plc = g711plc_init(AMR_SAMPLERATE, PLC_SAMPLE);
        if (mode >= 0 && mode <= 7) {
            state->mode = mode;
        } else {
            state->mode = 0;            //default mode
        }
        *dec_inst = state;
        return 0;
    }
    return -1;
}

int16_t WebRtcAmr_FreeDec(AMR_decinst_t_* dec_inst)
{
    if (dec_inst) {
        if (dec_inst->decoder) {
            GSMDecodeFrameExit(&dec_inst->decoder);
        }
        if (dec_inst->plc) {
            g711plc_destroy(dec_inst->plc);
        }
        free(dec_inst);
        return 0;
    } else {
        return -1;
    }
}

int16_t WebRtcAmr_DecoderInit(void* state)
{
    AMR_decinst_t_* dec_inst = (AMR_decinst_t_ *)state;

    if (dec_inst) {
        if (GSMInitDecode(&dec_inst->decoder, (Word8 *)"RTP")) {
            dec_inst->decoder = NULL;
            return -1;
        }
        return 0;
    }
    return -1;
}

int16_t WebRtcAmr_Decode(void* state, const int16_t* encoded,
                          int16_t encoded_bytes, int16_t* decoded,
                          int16_t* audio_type)
{
    AMR_decinst_t_* dec_inst = (AMR_decinst_t_ *)state;
    int i = 0;

    if (dec_inst && dec_inst->decoder) {
        unsigned char *bytes = (unsigned char *)encoded;
        enum Frame_Type_3GPP type;

        if (encoded_bytes < 2) {
            ALOGE("WebRtcAmr_Decode failed\n");
            return -1;
        }

        if (encoded_bytes != frame_sizes_nb[dec_inst->mode]) {
            type = (enum Frame_Type_3GPP)(bytes[0] >> 3);
            if (encoded_bytes != (frame_sizes_nb[type] + 1)) {
                ALOGE("decode: encoded_bytes(%d), type(%d)", encoded_bytes, type);
                return -1;
            }

            ++bytes;
            --encoded_bytes;
        } else {
            type = (enum Frame_Type_3GPP)dec_inst->mode;
        }

        if (AMRDecode(dec_inst->decoder, type, bytes, decoded, MIME_IETF) != encoded_bytes) {
            ALOGE("WebRtcAmr_Decode failed2\n");
            return -1;
        }

        for(i = 0; i < 2; i++) {
            g711plc_addtohistory(dec_inst->plc, (short *)decoded + i * PLC_SAMPLE);
        }

        *audio_type = 1;

        return 160;
    }
    return -1;
}

int16_t WebRtcAmr_DecodePlc(void* state,  int16_t* decoded,
                             int16_t number_of_lost_frames)
{
    int16_t i = 0, j = 0;
    int decoded_samples = 0;
    AMR_decinst_t_* dec_inst = (AMR_decinst_t_ *)state;

    for (i = 0; i < number_of_lost_frames; i++) {
        decoded += i * 160;//(PLC_SAMPLE * 2);
        for(j = 0; j < 2; j++) {
            g711plc_dofe(dec_inst->plc, (short *)decoded + j * PLC_SAMPLE);
        }
    }

    decoded_samples = number_of_lost_frames * 160;//PLC_SAMPLE * 2;

    ALOGV("WebRtcAmr_DecodePlc: decoded_samples(%d), number_of_lost_frames(%d)",
        decoded_samples, number_of_lost_frames);

    return decoded_samples;
}


