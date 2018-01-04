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

#ifndef CDX_UGLYDEF_H_
#define CDX_UGLYDEF_H_

#include <CDX_Types.h>
#include <GetAudio_format.h>

enum CDX_METADATA_TYPE{
	CDX_METADATA_TYPE_AUDIO = 0,
	CDX_METADATA_TYPE_VIDEO
};

typedef struct CedarXMetaData {
	int cdx_metadata_type;
	audio_file_info_t audio_metadata;
	char geo_data[18];
	int  geo_len;
	unsigned int duration;
	int  width;
	int  height;
	int  nHasVideo;
	int  nHasAudio;
	int  nHasSubtitle;
	int  nRotationAngle;
}CedarXMetaData;

#endif
