/*
 * Author: yewenliang@allwinnertech.com
 * Copyright (C) 2014 Allwinner Technology Co., Ltd.
 */
//#define LOG_NDEBUG 0
#define LOG_TAG "G726Codec"
//#include <cutils/log.h>
#include "dbg_log.h"

#include <string.h>

#include "g726_interface.h"
#include "codec_g726.h"
//#include "g72x.h"
#include "lowcfe.h"

#define USE_G726_PLC 1

#define SAMPLE_RATE 8000
#define PTIME 20
#define PLC_SAMPLE      80      /*10ms*/
#define SAMPLES_PER_FRAME ((SAMPLE_RATE * PTIME) / 1000)
#define PCM_FRAME_SIZE (2 * SAMPLES_PER_FRAME)

#define G726_BITRATE_16K    16000
#define G726_BITRATE_24K    24000
#define G726_BITRATE_32K    32000
#define G726_BITRATE_40K    40000

//typedef int (*decode_func_t) (int, int, g726_state*);
//typedef int (*encode_func_t) (int, int, g726_state*);

struct WebrtcG726Inst
{
    uint8_t *     encoder;
    uint8_t *     decoder;
    unsigned      bitrate;
    unsigned      encoded_frame_size; /* Size in bytes of an encoded 10ms frame. */
#if USE_G726_PLC
    LowcFE_c *    plc;
#endif
};

static int16_t WebrtcG726_setBitRate(G726Inst *g726_inst, int mode)
{
   switch (mode) {
    case G726_16:
        g726_inst->bitrate = G726_16;
        g726_inst->encoded_frame_size = (SAMPLES_PER_FRAME * 2) / 8;
        break;
    case G726_24:
        g726_inst->bitrate = G726_24;
        g726_inst->encoded_frame_size = (SAMPLES_PER_FRAME * 3) / 8;
        break;
    case G726_32:
        g726_inst->bitrate = G726_32;
        g726_inst->encoded_frame_size = (SAMPLES_PER_FRAME * 4) / 8;      
        break;
    case G726_40:
        g726_inst->bitrate = G726_40;
        g726_inst->encoded_frame_size = (SAMPLES_PER_FRAME * 5) / 8;   
        break;
    default:
        return -1;
    }
    return 0;
}

int16_t WebRtcG726_CreateDecoder(G726Inst **g726dec_inst, int mode)
{
    int ret;
    G726Inst *state;
    state = (G726Inst *)calloc(1, sizeof(G726Inst));
    if (state != NULL) {
        if (WebrtcG726_setBitRate(state, mode) == 0) {
            ret = g726_state_create(state->bitrate, AUDIO_ENCODING_LINEAR,
                            &state->decoder);
            if (ret == TRUE) {
            #if USE_G726_PLC
                state->plc = g711plc_init(SAMPLE_RATE, PLC_SAMPLE);
            #endif
                *g726dec_inst = state;        
                return 0;
            }
        }
        free(state);
    }
    return -1;
}

int16_t WebRtcG726_FreeDecoder(G726Inst *g726dec_inst)
{
    if (g726dec_inst) {
    #if USE_G726_PLC
        if (g726dec_inst->plc) {
            g711plc_destroy(g726dec_inst->plc);
            //g726dec_inst->plc = NULL;
        }
    #endif
        if (g726dec_inst->decoder) {
            g726_state_destroy(&g726dec_inst->decoder);
        }
        free(g726dec_inst);
        return 0;
    }
    return -1;
}

int16_t WebRtcG726_DecoderInit(void* state)
{
    G726Inst* g726dec_inst = (G726Inst *)state;
//#if USE_G726_PLC
//    g726dec_inst->plc = g711plc_init(SAMPLE_RATE, PLC_SAMPLE);
//#endif
    return 0;
}

int16_t WebRtcG726_Decode(void* state, const int16_t* encoded,
                          int16_t encoded_bytes, int16_t* decoded,
                          int16_t* audio_type)
{
    G726Inst* g726dec_inst = (G726Inst *)state;
    int ret, i;
    unsigned long outLen;

    if (g726dec_inst) {
        ALOGV("encoded_bytes is %d", encoded_bytes);
        //if (encoded_bytes != g726_inst->encoded_frame_size) {
        //    ALOGE("WebRtcG726_Decode: encoded_bytes(%d)", encoded_bytes);
        //    return -1;
        //}
        outLen = PCM_FRAME_SIZE;
        ret = g726_decode(g726dec_inst->decoder, (unsigned char *)encoded,
                    encoded_bytes, (unsigned char *)decoded, &outLen);
        if (ret != encoded_bytes/*|| outLen != PCM_FRAME_SIZE*/) {
            ALOGE("WebRtcG726_Decode: ret(%d), outLen(%ld)", ret, outLen);
            return -1;
        }
    
        *audio_type = 1;
    #if USE_G726_PLC
        for(i = 0; i < 2; i++) {
            g711plc_addtohistory(g726dec_inst->plc, (short *)decoded + i * PLC_SAMPLE);
        }
    #endif

        return (outLen / 2);
    }
    return -1;
}

int16_t WebRtcG726_DecodePlc(void* state,  int16_t* decoded,
                             int16_t number_of_lost_frames)
{
    int16_t i = 0, j = 0;
    int decoded_samples = 0;
    G726Inst* g726dec_inst = (G726Inst *)state;

    for (i = 0; i < number_of_lost_frames; i++) {
        decoded += i * 160;//(PLC_SAMPLE * 2);
        for(j = 0; j < 2; j++) {
            g711plc_dofe(g726dec_inst->plc, (short *)decoded + j * PLC_SAMPLE);
        }
    }

    decoded_samples = number_of_lost_frames * 160;//PLC_SAMPLE * 2;

    ALOGV("WebRtcG726_DecodePlc: decoded_samples(%d), number_of_lost_frames(%d)",
        decoded_samples, number_of_lost_frames);

    return decoded_samples;
}
