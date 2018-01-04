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

#ifndef CDX_Common_H_
#define CDX_Common_H_

#include <pthread.h>
#include <CDX_Types.h>
#include <CDX_PlayerAPI.h>
#include <CDX_Subtitle.h>
#include <CDX_Fileformat.h>
#include <include_omx/OMX_Types.h>
//add by weihongqiang for distinguishing
//stream source type.
typedef enum CEDARX_STREAM_SOURCE_TYPE {
	CDX_STREAM_SOURCE_UNKOWN = -1,
	CDX_STREAM_SOURCE_WD_AWTS = 0,
	CDX_STREAM_SOURCE_WD_TS = 1,
	//normal ts stream, need sync.
	CDX_STREAM_SOURCE_NORMAL_TS = 2,
}CEDARX_STREAM_SOURCE_TYPE;

typedef enum CEDARX_SOURCETYPE{
	CEDARX_SOURCE_FD,
	CEDARX_SOURCE_FILEPATH,
	CEDARX_SOURCE_M3U8,
	CEDARX_SOURCE_DRAMBUFFER,
	CEDARX_SOURCE_SFT_STREAM, //for ics
	CEDARX_SOURCE_NORMAL_STREAM,
	CEDARX_SOURCE_WRITER_CALLBACK, //for recoder writer
	CEDARX_SOURCE_NETWORK_RTSP,
	CEDARX_SOURCE_WIFI_DISPLAY,
}CEDARX_SOURCETYPE;

typedef enum MEDIA_3DMODE_TYPE{
	VIDEORENDER_3DMODE_NONE = 0, //don't touch it
	VIDEORENDER_3DMODE_LR_INTERLEAVE,
	VIDEORENDER_3DMODE_TD_INTERLEAVE
}MEDIA_3DMODE_TYPE;

typedef enum CEDARV_3D_MODE
{
	//* for 2D pictures.
	CEDARV_3D_MODE_NONE 				= 0,

	//* for double stream video like MVC and MJPEG.
	CEDARV_3D_MODE_DOUBLE_STREAM,

	//* for single stream video.
	CEDARV_3D_MODE_SIDE_BY_SIDE,
	CEDARV_3D_MODE_TOP_TO_BOTTOM,
	CEDARV_3D_MODE_LINE_INTERLEAVE,
	CEDARV_3D_MODE_COLUME_INTERLEAVE

}cedarv_3d_mode_e;

typedef enum CDX_DECODE_VIDEO_STREAM_TYPE
{
	CDX_VIDEO_STREAM_MAJOR = 0,
	CDX_VIDEO_STREAM_MINOR,
	CDX_VIDEO_STREAM_NONE,
}CDX_VIDEO_STREAM_TYPE;

typedef struct CedarXDataSourceDesc{
	CEDARX_THIRDPART_STREAMTYPE thirdpart_stream_type;
	CEDARX_STREAMTYPE stream_type;
	CEDARX_SOURCETYPE source_type;
	CEDARX_MEDIA_TYPE media_type;
	MEDIA_3DMODE_TYPE media_subtype_3d;
	CEDARX_STREAM_SOURCE_TYPE stream_source_type;
//	void *stream_info; //used for m3u/ts
	void *m3u_handle;

	char *source_url; //SetDataSource url
	CedarXExternFdDesc ext_fd_desc;
	int thirdpart_encrypted_type;
	void *url_headers;
	void *sft_stream_handle;
	pthread_mutex_t sft_handle_mutex;
	void *sft_cached_source2;
	void *sft_http_source;
	void *sft_rtsp_source;
	//For local drm protected source.
	void *sft_file_source;
	long long sft_stream_length;
	int	 sft_stream_handle_num;

	int  demux_type;    //CDX_MEDIA_FILE_FORMAT

	int  mp_stream_cache_size; //unit KByte used for mplayer cache size setting
	char* bd_source_url;
    int   playBDFile;
    int   disable_seek;

	pthread_mutex_t m3u_handle_mutex;
}CedarXDataSourceDesc;

typedef enum CDX_AUDIO_CODEC_TYPE {
	CDX_AUDIO_NONE = 0,
	CDX_AUDIO_UNKNOWN,
	CDX_AUDIO_MP1,
	CDX_AUDIO_MP2,
	CDX_AUDIO_MP3,
	CDX_AUDIO_MPEG_AAC_LC,
	CDX_AUDIO_AC3,
	CDX_AUDIO_DTS,
	CDX_AUDIO_LPCM_V,
	CDX_AUDIO_LPCM_A,
	CDX_AUDIO_ADPCM,
	CDX_AUDIO_PCM,
	CDX_AUDIO_WMA_STANDARD,
	CDX_AUDIO_FLAC,
	CDX_AUDIO_APE,
	CDX_AUDIO_OGG,
	CDX_AUDIO_RAAC,
	CDX_AUDIO_COOK,
	CDX_AUDIO_SIPR,
	CDX_AUDIO_ATRC,
	CDX_AUDIO_AMR,
	CDX_AUDIO_RA,

	CDX_AUDIO_MLP = CDX_AUDIO_UNKNOWN,
	CDX_AUDIO_PPCM = CDX_AUDIO_UNKNOWN,
	CDX_AUDIO_WMA_LOSS = CDX_AUDIO_UNKNOWN,
	CDX_AUDIO_WMA_PRO = CDX_AUDIO_UNKNOWN,
	CDX_AUDIO_MP3_PRO = CDX_AUDIO_UNKNOWN,
}CDX_AUDIO_CODEC_TYPE;

typedef enum CDX_DECODE_MODE {
	CDX_DECODER_MODE_NORMAL = 0,
	CDX_DECODER_MODE_RAWMUSIC,
}CDX_DECODE_MODE;

typedef struct CedarXScreenInfo {
	int screen_width;
	int screen_height;
}CedarXScreenInfo;

#define SINKINFO_SIZE   (2)

typedef enum MUXERMODES {
	MUXER_MODE_MP4 = 0,
    MUXER_MODE_MP3,
    MUXER_MODE_AAC,
	MUXER_MODE_AWTS,
	MUXER_MODE_RAW,
	MUXER_MODE_TS,

	MUXER_MODE_USER0,
	MUXER_MODE_USER1,
	MUXER_MODE_USER2,
	MUXER_MODE_USER3,
}MUXERMODES;

//typedef enum MuxerOutputSinkType
//{
//    MUXER_SINKTYPE_NONE           = 0x00,
//    MUXER_SINKTYPE_FILE           = 0x01, //one muxer write to sd file.
//    MUXER_SINKTYPE_CALLBACKOUT    = 0x02, //one muxer transport by RTSP
//    MUXER_SINKTYPE_ALL            = 0x03, //one muxer write && transport.
//}MuxerOutputSinkType;

typedef struct DynamicBuffer
{
    OMX_S8*     mpBuffer;
    OMX_S32     mSize;
    struct list_head mList;
}DynamicBuffer;

typedef struct tag_CdxFdT
{
    int mFd;
    char *mPath;
    int mnFallocateLen;
    int mIsImpact;
    int mMuxerId;
}CdxFdT;

typedef struct CdxOutputSinkInfo
{
    int                 mMuxerId;
    MUXERMODES          nMuxerMode;
    int                 nOutputFd;
    char                *mPath;
    int                 nFallocateLen;
    int                 nCallbackOutFlag;
    struct list_head    mList;
}CdxOutputSinkInfo;
int CdxOutputSinkInfoInit(CdxOutputSinkInfo *pInfo);
int CdxOutputSinkInfoDestroy(CdxOutputSinkInfo *pInfo);
int copyCdxOutputSinkInfo(CdxOutputSinkInfo *pDes, CdxOutputSinkInfo *pSrc);

//typedef struct CdxRemoveOutputSinkInfo
//{
//    MUXERMODES          nMuxerMode;
//    MuxerOutputSinkType nSinkType;
//}CdxRemoveOutputSinkInfo;

#define ALIGN( num, to ) (((num) + (to-1)) & (~(to-1)))
#define ALIGN16(num) (ALIGN(num, 16))
#define ALIGN32(num) (ALIGN(num, 32))

#endif
/* File EOF */
