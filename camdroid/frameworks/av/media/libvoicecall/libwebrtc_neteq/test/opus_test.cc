/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "modules/audio_coding/codecs/opus/interface/opus_interface.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>

#define OPUS_COMPLEXITY 5
#define FRAME_SIZE  (20 * 48000 / 1000)

int main(int argc, char ** argv)
{
    static FILE* in_fp = NULL;
    static FILE* out_fp = NULL;
    static FILE* pcm_fp  = NULL;
    OpusEncInst* inst = NULL;
    OpusDecInst* dinst = NULL;
    int ret;
    int frame_size;
    int16_t in_buffer[FRAME_SIZE];
    int16_t out_buffer[1000];
    int16_t pcm_buffer[2000];
    int16_t size;
    int16_t pcm_size;

    int16_t audio_type;

    if (argc < 4) {
        return -1;
    }

    in_fp = fopen(argv[1], "r");
    if (!in_fp) {
        fprintf(stderr, "open %s failed.\n", argv[1]);
        return -1;
    }

    out_fp = fopen(argv[2], "w+");
    if (!out_fp) {
        fprintf(stderr, "open %s failed.\n", argv[2]);
        fclose(in_fp);
        return -1;
    }

    pcm_fp = fopen(argv[3], "w+");
    if (!pcm_fp) {
        fprintf(stderr, "open %s failed.\n", argv[3]);
        fclose(in_fp);
        fclose(out_fp);
        return -1;
    }

    ret = WebRtcOpus_EncoderCreate(&inst, 8000, 1);
    if (ret < 0) {
        fprintf(stderr, "WebRtcOpus_EncoderCreate err\n");
        fclose(in_fp);
        fclose(out_fp);
        fclose(pcm_fp);
        return -1;
    }

    WebRtcOpus_SetBitRate(inst, 16000);

    ret = WebRtcOpus_DecoderCreate(&dinst, 8000, 1);
     if (ret < 0) {
        fprintf(stderr, "WebRtcOpus_DecoderCreate err\n");
        fclose(in_fp);
        fclose(out_fp);
        return -1;
    }

    WebRtcOpus_DecoderInit(dinst);

    while(fread(in_buffer, 2, FRAME_SIZE, in_fp) == FRAME_SIZE) {
        size = WebRtcOpus_Encode(inst, in_buffer, FRAME_SIZE, 1000, (uint8_t *)out_buffer);
        //fprintf(stderr, "WebRtcOpus_Encode: return size is %d\n", size);
        if (size < 0) {
            break;
        }
        fwrite((char *)out_buffer, 1, size, out_fp);

        pcm_size = WebRtcOpus_DecodeNoneResample(dinst, (const int16_t* )out_buffer, size,
                        pcm_buffer, &audio_type);
        if (pcm_size < 0) {
            fprintf(stderr, "WebRtcOpus_DecodeNoneResample return %d\n", pcm_size);
            break;
        }
        fprintf(stderr, "pcm_size is %d\n", pcm_size);
        fwrite(pcm_buffer, 2, pcm_size, pcm_fp);
    }

    WebRtcOpus_EncoderFree(inst);
    WebRtcOpus_DecoderFree(dinst);

    fclose(in_fp);
    fclose(out_fp);
    fclose(pcm_fp);
    return 0;
}
