/*
 * Copyright (C) 2009 The Android Open Source Project
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

#ifndef CEDARADECCOM_H_
#define CEDARADECCOM_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include <ad_cedarlib_com.h>

#define BsBuffLEN 8*1024

typedef struct  __AUDIO_CODEC_FORMAT
{
     //define audio codec type
    unsigned int       codec_type;         // audio codec type, AUDIO_MP3???,
    unsigned int       codec_type_sub;     // audio codec sub type, the highest bit mark endian type
                                    // bit0-bit15, sub type of the audio codec
                                    // bit16,      endian flag: 0, little ending; 1, big ending;
                                    // other bits, reserved

    //define audio bitstream format
    unsigned short       channels;           // channel count
    unsigned short       bits_per_sample;    // bits per sample,
    unsigned int       sample_per_second;  // sample count per second
    unsigned int       avg_bit_rate;       // average bit rate,bit/s
    unsigned int       max_bit_rate;       // maximum bit rate

    unsigned int       file_length;       //
    int       is_raw_data_output; //0:pcm output, 1:raw data
    //define private information for audio decode
    unsigned short       audio_bs_src;       // audio bitstream source, __cedarlib_file_fmt_t , CEDARLIB_FILE_FMT_AVI, <==>CEDAR_MEDIA_FILE_FMT_AVI???

    short       priv_inf_len;       // audio bitstream private information length
    void        *private_inf;       // audio bitstream private information pointer. ??????malloc???????2?D????amalloc??????????????3???????y???AVI_PSR???set_FILE_PARSER_audio_info()

} __audio_codec_format_t;
typedef struct __AudioDEC_DEV
{
    int       uErrFrmCnt;         //error frame count, for check audio decode time out
    __audio_codec_format_t          AudioBsFmt;
    struct  __AudioDEC_AC320        *DecPrivate;    //???????a???astruct
    int       nFfRevStep;         //step of ff/rr, unit:s.
    int        nFfRevDly;          //time delay after one step, esKRNL_TimeDly(), unit: 10ms
    
    void*      libHandle;
    int  (*cedara_decinit	)(void *pDev);
	void (*cedara_decexit	)(void *pDev);
    BsInFor			AudioBsInfo;
	Ac320FileRead	DecFileInfo;
	com_internal    pInternal;
} __Audiodec_dev_t;

#ifdef __cplusplus
}
#endif

#endif

