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

namespace android {

typedef struct media_probe_data_t{
	unsigned char *buf;
	int buf_size;
}media_probe_data_t;

typedef enum MEDIA_CONTAINER_FORMAT{
	MEDIA_FORMAT_UNKOWN = 0, //don't touch the order
	MEDIA_FORMAT_STAGEFRIGHT_MIN = 0,
	MEDIA_FORMAT_MP3 = 1,
	MEDIA_FORMAT_OGG = 2,
	MEDIA_FORMAT_AMR = 3,
	MEDIA_FORMAT_M4A = 4,
	MEDIA_FORMAT_3GP = 5,
	MEDIA_FORMAT_STAGEFRIGHT_MAX = 8,

	MEDIA_FORMAT_CEDARX_MIN = 8,
	MEDIA_FORMAT_CEDARX_MAX = 16,

	MEDIA_FORMAT_CEDARA_MIN = 16,
	MEDIA_FORMAT_APE  = 17,
	MEDIA_FORMAT_WMA  = 19,
	MEDIA_FORMAT_AC3  = 20,
	MEDIA_FORMAT_DTS  = 21,
	MEDIA_FORMAT_AAC  = 22,
	MEDIA_FORMAT_WAV  = 23,
	MEDIA_FORMAT_FLAC = 24,
	MEDIA_FORMAT_CEDARA  = 31,
	MEDIA_FORMAT_CEDARA_MAX = 32,
}MEDIA_CONTAINER_FORMAT;

extern int audio_format_detect(unsigned char *buf, int buf_size, int fd, int64_t offset);
extern int stagefright_system_format_detect(unsigned char *buf, int buf_size);

} // android namespace
