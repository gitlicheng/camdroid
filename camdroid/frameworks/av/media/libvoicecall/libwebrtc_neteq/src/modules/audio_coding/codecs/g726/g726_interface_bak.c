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
//#include "codec_g726.h"
#include "g72x.h"
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

typedef int (*decode_func_t) (int, int, g726_state*);
typedef int (*encode_func_t) (int, int, g726_state*);

struct WebrtcG726Inst
{
    g726_state    encoder;
    g726_state    decoder;
    encode_func_t encode_func;
    decode_func_t decode_func;
    unsigned      bitrate;
    unsigned      code_bits; /* Number of bits for each coded audio sample. */
    unsigned      code_bit_mask; /* Bit mask for the encoded bits */
    unsigned      encoded_frame_size; /* Size in bytes of an encoded 10ms frame. */
#if USE_G726_PLC
    LowcFE_c *    plc;
#endif
};

static int16_t WebrtcG726_setBitRate(G726Inst *g726_inst, int bit_rate)
{
   switch (bit_rate) {
    case G726_BITRATE_16K:
        g726_inst->encode_func = g726_16_encoder;
        g726_inst->decode_func = g726_16_decoder;
        g726_inst->bitrate = 16000;
        g726_inst->code_bits = 2;
        g726_inst->code_bit_mask = 0x03;
        g726_inst->encoded_frame_size = (SAMPLES_PER_FRAME * 2) / 8;
        break;
    case G726_BITRATE_24K:
        g726_inst->encode_func = g726_24_encoder;
        g726_inst->decode_func = g726_24_decoder;
        g726_inst->bitrate = 24000;
        g726_inst->code_bits = 3;
        g726_inst->code_bit_mask = 0x07;
        g726_inst->encoded_frame_size = (SAMPLES_PER_FRAME * 3) / 8;
        break;
    case G726_BITRATE_32K:
        g726_inst->encode_func = g726_32_encoder;
        g726_inst->decode_func = g726_32_decoder;
        g726_inst->bitrate = 32000;
        g726_inst->code_bits = 4;
        g726_inst->code_bit_mask = 0x0F;
        g726_inst->encoded_frame_size = (SAMPLES_PER_FRAME * 4) / 8;      
        break;
    case G726_BITRATE_40K:
        g726_inst->encode_func = g726_40_encoder;
        g726_inst->decode_func = g726_40_decoder;
        g726_inst->bitrate = 40000;
        g726_inst->code_bits = 5;
        g726_inst->code_bit_mask = 0x1F;
        g726_inst->encoded_frame_size = (SAMPLES_PER_FRAME * 5) / 8;   
        break;
    default:
        return -1;
    }
    return 0;
}

int16_t WebRtcG726_CreateDecoder(G726Inst **g726dec_inst, int bit_rate)
{
    G726Inst *state;
    state = (G726Inst *)calloc(1, sizeof(G726Inst));
    if (state != NULL) {
        if (WebrtcG726_setBitRate(state, bit_rate) == 0) {
            g726_init_state(&state->decoder);
        #if USE_G726_PLC
            state->plc = g711plc_init(SAMPLE_RATE, PLC_SAMPLE);
        #endif
            *g726dec_inst = state;        
            return 0;
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
    
    if (g726dec_inst) {
        int samples = encoded_bytes * SAMPLES_PER_FRAME / g726dec_inst->encoded_frame_size;
        int i;
        uint8_t code;
        int in_bits = 0;
        uint32_t in_buffer = 0;
        uint8_t *src;

        ALOGD("encoded_bytes is %d", encoded_bytes);

        src = (uint8_t *)encoded;
        for (i=0; i<samples; i++) {
            if (in_bits < g726dec_inst->code_bits) {
                in_buffer |= (*src++ << in_bits);
                in_bits += 8;
            }
            code = in_buffer & g726dec_inst->code_bit_mask;
            in_buffer >>= g726dec_inst->code_bits;
            in_bits -= g726dec_inst->code_bits;
            decoded[i] = (int16_t)g726dec_inst->decode_func(code,
                                    AUDIO_ENCODING_LINEAR, &g726dec_inst->decoder);
        }

        *audio_type = 1;
    #if USE_G726_PLC
        for(i = 0; i < 2; i++) {
            g711plc_addtohistory(g726dec_inst->plc, (short *)decoded + i * PLC_SAMPLE);
        }
    #endif
        ALOGD("samples is %d", samples);
        return samples;
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
