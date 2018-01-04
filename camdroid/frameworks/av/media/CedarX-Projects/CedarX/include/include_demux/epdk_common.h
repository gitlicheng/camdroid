/*******************************************************************************
--                                                                            --
--                    CedarX Multimedia Framework                             --
--                                                                            --
--          the Multimedia Framework for Linux/Android System                 --
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                         Softwinner Products.                               --
--                                                                            --
--                   (C) COPYRIGHT 2011 SOFTWINNER PRODUCTS                   --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
*******************************************************************************/

#ifndef _EPDK_COMMON_H_
#define _EPDK_COMMON_H_

#include <stdio.h>
//#include <stdlib.h>
#include <string.h>
#include <CDX_Common.h>

#include <OMX_Types.h>
//#include <OMX_Component.h>
#include <OMX_Core.h>
#include <OMX_Index.h>
#include <OMX_Image.h>
#include <OMX_Audio.h>
#include <OMX_Video.h>
#include <OMX_IVCommon.h>
#include <OMX_Other.h>

#include <CDX_Fileformat.h>

#define MALLOC(size)                    malloc(size)
#define FREE(pbuf)                      free(pbuf)
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#include "epdk_abs_header.h"

typedef enum __FILE_PARSER_RETURN_VAL {
	FILE_PARSER_RETURN_ERROR = -1,
	FILE_PARSER_RETURN_OK = 0,

	FILE_PARSER_READ_END,
	FILE_PARSER_BUF_LACK,
	FILE_PARSER_GET_NONE,
	FILE_PARSER_ERROR_IGNORE,
	FILE_PARSER_PAUSE_GET_DATA,
	FILE_PARSER_ERR_VIDEO_BUF_FULL,
	FILE_PARSER_ERR_AUDIO_BUF_FULL,

	FILE_PARSER_PARA_ERR			= -2,
	FILE_PARSER_OPEN_FILE_FAIL		= -3,
	FILE_PARSER_READ_FILE_FAIL		= -4,
	FILE_PARSER_FILE_FMT_ERR		= -5,
	FILE_PARSER_NO_AV				= -6,
	FILE_PARSER_END_OF_MOVI			= -7,
	FILE_PARSER_REQMEM_FAIL			= -8,
	FILE_PARSER_EXCEPTION			= -9,
	FILE_PARSER_,
} __file_parser_return_val_t;

#define MAX_AUDIO_CH_NUM        8
#define ABS_EDIAN_FLAG_MASK         ((unsigned int)(1<<16))
#define ABS_EDIAN_FLAG_LITTLE       ((unsigned int)(0<<16))
#define ABS_EDIAN_FLAG_BIG          ((unsigned int)(1<<16))

typedef struct AUDIO_CODEC_FORMAT {
	//define audio codec type
	unsigned int codec_type;
	unsigned int codec_type_sub; // audio codec sub type, the highest bit mark endian type
	// bit0-bit15, sub type of the audio codec
	// bit16,      endian flag: 0, little ending; 1, big ending;
	// other bits, reserved

	//define audio bitstream format
	unsigned short channels;
	unsigned short bits_per_sample;
	unsigned int sample_per_second;
	unsigned int avg_bit_rate;
	unsigned int max_bit_rate;
	unsigned int file_length;

	//define private information for audio decode
	unsigned short audio_bs_src; // audio bitstream source, __cedarlib_file_fmt_t , CEDARLIB_FILE_FMT_AVI, <==>CEDAR_MEDIA_FILE_FMT_AVI��
	int is_raw_data_output; //0:pcm output, 1:raw data
	short priv_inf_len; // audio bitstream private information length
	void *private_inf; // audio bitstream private information pointer.
} __audio_codec_format_t;

typedef struct SUBTITLE_CODEC_FORMAT {
	unsigned int codec_type; //
	unsigned int data_encode_type; //
	unsigned int language_type; //
} __subtitle_codec_format_t;

#endif //_PSR_MOV_CFG_H_
