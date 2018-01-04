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

#ifndef RECORD_WRITER_H_
#define RECORD_WRITER_H_

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum RECODERWRITECOMMANDS
{
	UNKOWNCMD = 0,
    SETAVPARA,
	SETTOTALTIME,
	SETFALLOCATELEN,    //set fallocate size for fd.
	SETCACHEFD,
	SETCACHEFD2,
	SETOUTURL,
	REGISTER_WRITE_CALLBACK,
	SETSDCARDSTATE,
	SETCACHEMEM,
	SET_FS_WRITE_MODE,
	SET_FS_SIMPLE_CACHE_SIZE,
	SET_STREAM_CALLBACK,

	/* gushiming compressed source */
	//SET_VIDEO_CODEC_ID,
	//SET_AUDIO_CODEC_ID,
} RECODERWRITECOMMANDS;

enum CodecType {
    CODEC_TYPE_UNKNOWN = -1,
    CODEC_TYPE_VIDEO = 0,//don't change it!
    CODEC_TYPE_AUDIO = 1,//don't change it!
    CODEC_TYPE_DATA
};

enum CodecID {
    CODEC_ID_MPEG4 = 32,
    CODEC_ID_H264 = 33,
    CODEC_ID_MJPEG = 108,
    CODEC_ID_AAC = 64,
    CODEC_ID_MP3 = 105,
    CODEC_ID_PCM,   // 16-bit little-endian, twos-complement
    CODEC_ID_ADPCM, // DVI/Intel IMAADPCM-ACM code 17
};

enum AVPACKET_FLAGS {
	AVPACKET_FLAG_KEYFRAME = 1,
};

typedef struct AVPacket {
    CDX_S64 pts;
	CDX_S64 dts;
    CDX_S8  *data0;
    CDX_S32 size0;
    CDX_S8  *data1;
    CDX_S32 size1;
    CDX_S32 stream_index;
    CDX_S32 flags;
    CDX_S32 duration;                         ///< presentation duration in time_base units (0 if not available)
    CDX_S64 pos;                            ///< byte position in stream, -1 if unknown

    //for video frame.
    int CurrQp;         //qp of current frame
    int avQp;
	int nGopIndex;      //index of gop
	int nFrameIndex;    //index of current frame in gop.
	int nTotalIndex;    //index of current frame in whole encoded frames
} AVPacket;

typedef struct CDX_RecordWriter{
	const char *info;

	void*   (*MuxerOpen)(int *ret);
	CDX_S32 (*MuxerClose)(void *handle);
	CDX_S32 (*MuxerWriteExtraData)(void *handle, CDX_U8 *vosData, CDX_U32 vosLen, CDX_U32 idx);
	CDX_S32 (*MuxerWriteHeader)(void *handle);
	CDX_S32 (*MuxerWriteTrailer)(void *handle);
	CDX_S32 (*MuxerWritePacket)(void *handle, void *pkt);
	CDX_S32 (*MuxerIoctrl)(void *handle, CDX_U32 uCmd, CDX_U32 uParam);
}CDX_RecordWriter;

CDX_RecordWriter *cedarx_record_writer_create(int mode);
void cedarx_record_writer_destroy(void *handle);


#if defined(__cplusplus)
}
#endif

#endif
