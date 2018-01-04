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

#ifndef CDX_PlayerAPI_H_
#define CDX_PlayerAPI_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define CDX_VPS_SLOWEST       (-40)    //slowest, 100-40 = 60%
#define CDX_VPS_NOMAL         (0)     // normal play
#define CDX_VPS_FASTEST       (100)    //fastest, 100+100=200%

typedef enum {
    CDX_CMD_NOP           =  0, // interface test message
    CDX_CMD_GET_VERSION       ,
    CDX_SET_DATASOURCE_URL    ,
    CDX_SET_DATASOURCE_FD	  ,
    CDX_CMD_GET_POSITION	  ,
    CDX_CMD_GET_DURATION	  ,

    CDX_CMD_PREPARE			  ,
    CDX_CMD_START			  ,
    CDX_CMD_TAG_START		  ,
    CDX_CMD_STOP 			  ,
    CDX_CMD_PAUSE			  ,
    CDX_CMD_SEEK			  ,

    CDX_CMD_PREPARE_ASYNC     , //12
    CDX_CMD_START_ASYNC  	  ,
    CDX_CMD_TAG_START_ASYNC	  ,
    CDX_CMD_PAUSE_TAG_START_ASYNC,  //for pause_jump, resume play
    CDX_CMD_STOP_ASYNC		  ,
    CDX_CMD_PAUSE_ASYNC		  ,
    CDX_CMD_SEEK_ASYNC        ,

    CDX_CMD_RESUME			  ,
    CDX_CMD_RESET			  ,
    CDX_CMD_GETSTATE		  ,
    CDX_CMD_REGISTER_CALLBACK ,
    CDX_CMD_GET_MEDIAINFO     ,
    CDX_CMD_SWITCH_AUDIO      ,
    CDX_CMD_SWITCH_SUBTITLE   ,
    CDX_CMD_SELECT_AUDIO_OUT  ,
    CDX_CMD_CAPTURE_THUMBNAIL ,
    CDX_CMD_CLOSE_CAPTURE     ,
    CDX_CMD_QUIT_RETRIVER     ,
    CDX_CMD_GET_METADATA 	  ,
    CDX_CMD_SET_STREAM_TYPE   ,
    CDX_CMD_GET_STREAM_TYPE   ,
    CDX_CMD_SET_VOLUME        ,
    CDX_CMD_SET_SOFT_CHIP_VERSION,
    CDX_CMD_SET_MAX_RESOLUTION,

    CDX_CMD_SET_STREAMING_TYPE,
    CDX_SET_DATASOURCE_SFT_STREAM,
    CDX_SET_THIRDPART_STREAM  ,
    CDX_CMD_SUPPORT_SEEK,

    CDX_CMD_NOTIFY_EOF,

    CDX_CMD_SET_VPS = 0x30,           
    CDX_CMD_GET_VPS,

	CDX_CMD_SEND_BUF	 = 150, //recorder, 0x96
	CDX_CMD_SET_SAVE_FILE	  ,
	CDX_CMD_SET_VIDEO_INFO	  ,
	CDX_CMD_SET_AUDIO_INFO	  ,
	CDX_CMD_SET_REC_MODE	  ,
	CDX_CMD_SET_PREVIEW_INFO  ,
	CDX_CMD_GET_FILE_SIZE	  ,
	CDX_CMD_SET_TIME_LAPSE    ,
	CDX_CMD_SET_OUTPUT_FORMAT ,
	CDX_CMD_SET_SAVE_URL	  ,
	CDX_CMD_GET_ONE_BSFRAME   ,
	CDX_CMD_FREE_ONE_BSFRAME  ,
	//CDX_CMD_SET_EXTRA_VIDEO_CHANNEL,
	//CDX_CMD_SEND_BUF_EXTRA  ,
	//CDX_CMD_SET_SAVE_FILE_EXTRA,
	//CDX_CMD_SET_VIDEO_INFO_EXTRA,
	CDX_CMD_SET_CAMERA_ID,
	CDX_CMD_GET_CAMERA_ID,
	//CDX_CMD_SET_MOTION_PARAM,
	CDX_CMD_GET_SPSPPS_INFO,
	CDX_CMD_SET_SDCARD_STATE,
	CDX_CMD_ADD_OUTPUTSINKINFO,
	CDX_CMD_REMOVE_OUTPUTSINKINFO,

    CDX_CMD_GETSUBCOUNT  = 200,
    CDX_CMD_GETSUBLIST,
    CDX_CMD_GETCURSUB,
    CDX_CMD_SWITCHSUB,
    CDX_CMD_SETSUBGATE,
    CDX_CMD_GETSUBGATE,
    CDX_CMD_SETSUBCOLOR,
    CDX_CMD_GETSUBCOLOR,
    CDX_CMD_SETSUBFRAMECOLOR,
    CDX_CMD_GETSUBFRAMECOLOR,
    CDX_CMD_SETSUBFONTSIZE,
    CDX_CMD_GETSUBFONTSIZE,
    CDX_CMD_SETSUBCHARSET,
    CDX_CMD_GETSUBCHARSET,
    CDX_CMD_SETSUBPOSITION,
    CDX_CMD_GETSUBPOSITION,
    CDX_CMD_SETSUBDELAY,
    CDX_CMD_GETSUBDELAY,
    CDX_CMD_GETTRACKCOUNT,
    CDX_CMD_GETTRACKLIST,
    CDX_CMD_GETCURTRACK,
    CDX_CMD_SWITCHTRACK,

    CDX_CMD_SET_NETWORK_ENGINE = 300,

	CDX_CMD_SET_PICTURE_3D_MODE = 400,
	CDX_CMD_GET_PICTURE_3D_MODE,
	CDX_CMD_GET_SOURCE_3D_MODE,
	//CDX_CMD_SET_DISPLAY_MODE,
	//CDX_CMD_SET_ANAGLAGH_TYPE,
	CDX_CMD_GET_ALL_DISPLAY_MODE,

	
	CDX_CMD_SET_VIDEO_ROTATION, //static rotation
	CDX_CMD_SET_VIDEO_MAXWIDTH,
	CDX_CMD_SET_VIDEO_MAXHEIGHT,
	//CDX_CMD_SET_VIDEO_OUTPUT_SETTING,

	CDX_CMD_DISABLE_XXXX,
	CDX_CMD_SET_AUDIOCHANNEL_MUTE,
	CDX_CMD_GET_AUDIOCHANNEL_MUTE,

	CDX_CMD_DISABLE_MEDIA_TYPE,

	CDX_CMD_REGISTER_DEMUXER = 0x800,
	CDX_CMD_SELECT_DEMUXER   ,

    CDX_CMD_PLAY_BD_FILE,
    CDX_CMD_IS_PLAY_BD_FILE,

    //wvm
    CDX_CMD_SET_WVM_INFO,
    CDX_CMD_SET_URL_HEADERS,
	
	//CDX_CMD_SET_DYNAMIC_ROTATE,   //dynamic rotation

	CDX_CMD_SET_DISABLE_SEEK,
	
    CDX_CMD_CAPTURE_THUMBNAIL_STREAM,

    //for cedarx

	//for stagefright
	CDX_CMD_SET_DEFAULT_LOW_WATER_THRESHOLD,
	CDX_CMD_SET_DEFAULT_HIGH_WATER_THRESHOLD,


	//add by yaosen 2013-1-23
	CDX_CMD_SET_CACHE_PARAMS,
	CDX_CMD_GET_CACHE_PARAMS,
	CDX_CMD_GET_CURRETN_CACHE_SIZE,
	CDX_CMD_GET_CURRENT_BITRATE,

	//add by weihongqiang for IPTV.
	CDX_CMD_SET_AV_SYNC,
	CDX_CMD_CLEAR_BUFFER_ASYNC,
	CDX_CMD_SWITCH_AUDIO_CHANNEL,

    CDX_CMD_SET_PRESENT_SCREEN,
    CDX_CMD_GET_PRESENT_SCREEN,

    //add by weihongqiang for external fd
    CDX_CMD_ADD_EXTERNAL_SOURCE_FD,

    // for encode param.
	CDX_CMD_SET_BITRATE,
	CDX_CMD_SET_MAX_KEY_FRAME_INTERVAL,

	CDX_CMD_SET_IMPACT_FILE_DURATION,
	CDX_CMD_SWITCH_FILE,        //switch file immediately.
	CDX_CMD_SET_SAVE_IMPACT_FILE,

	CDX_CMD_SET_FILE_DURATION,

	//CDX_CMD_FALLOCATE_FILE,

    CDX_CMD_SET_FSWRITE_MODE,
    CDX_CMD_SET_SIMPLE_CACHE_SIZE,
	CDX_CMD_REENCODE_KEY_FRAME,
	CDX_CMD_ENABLE_DYNAMIC_BITRATE_CONTROL,
	CDX_CMD_NULL,
}CEDARX_COMMAND_TYPE;

typedef enum CEDARX_EVENT_TYPE{
	CDX_EVENT_NONE                 = 0,
    CDX_EVENT_PREPARED             = 1,
    CDX_EVENT_PLAYBACK_COMPLETE    = 2,
    CDX_EVENT_SEEK_COMPLETE        = 4,
    CDX_EVENT_FATAL_ERROR          = 8,
    CDX_MEDIA_INFO_BUFFERING_START = 16,
    CDX_MEDIA_INFO_BUFFERING_END   = 32,
    CDX_MEDIA_BUFFERING_UPDATE     = 64,
    CDX_MEDIA_WHOLE_BUFFERING_UPDATE = 128,
    CDX_EVENT_VDEC_OUT_OF_MEMORY        = 256,


	CDX_EVENT_VIDEORENDERINIT    = 65536,
	CDX_EVENT_VIDEORENDERDATA	  ,
	CDX_EVENT_VIDEORENDERGETDISPID,
	CDX_EVENT_VIDEORENDER_DEQUEUEFRAME,
	CDX_EVENT_VIDEORENDER_ENQUEUEFRAME,
	CDX_EVENT_AUDIORENDERINIT     ,
	CDX_EVENT_AUDIORENDEREXIT     ,
	CDX_EVENT_AUDIORENDERDATA     ,
	CDX_EVENT_AUDIORENDERGETSPACE ,
	CDX_EVENT_AUDIORENDERGETDELAY ,
	CDX_EVENT_VIDEORENDEREXIT     ,
	CDX_EVENT_AUDIORENDERFLUSHCACHE,
	CDX_EVENT_AUDIORENDERPAUSE    ,
	CDA_EVENT_AUDIORAWPLAY,

	CDX_MEDIA_INFO_SRC_3D_MODE = 65536+1000,
	CDX_EVENT_NATIVE_SUSPEND,

	CDX_EVENT_READ_AUDIO_BUFFER		= 65535+2000,
	CDX_EVENT_RELEASE_VIDEO_BUFFER,
	CDX_EVENT_BSFRAME_AVAILABLE,
	
	CDX_EVENT_NETWORK_STREAM_INFO,

	CDX_EVENT_MEDIASOURCE_READ = CDX_EVENT_READ_AUDIO_BUFFER + 100,
	//CDX_EVENT_RELEASE_VIDEO_BUFFER_EXTRA_CHANNEL,
	CDX_EVENT_MOTION_DETECTION,
	CDX_EVENT_RECORD_DONE,

	CDX_EVENT_TIMED_TEXT,//support android4.2 subtitle interface.
	CDX_EVENT_IS_ENCRYPTED_MEDIA,

	CDX_EVENT_NEED_NEXT_FD,
	CDX_EVENT_REC_VBV_FULL,
	CDX_EVENT_WRITE_DISK_ERROR,
}CEDARX_EVENT_TYPE;

typedef enum CEDARX_STATES{
	CDX_STATE_UNKOWN   = 0, //don't touch the order
	CDX_STATE_PREPARING   ,
	CDX_STATE_IDEL        ,
	CDX_STATE_PAUSE       ,
	CDX_STATE_EXCUTING    ,
	CDX_STATE_SEEKING     ,
	CDX_STATE_STOPPED	  ,
}CEDARX_STATES;

typedef enum CEDARX_AUDIO_OUT_TYPE{
	CDX_AUDIO_OUT_UNKOWN = 0,
	CDX_AUDIO_OUT_DAC       ,
	CDX_AUDIO_OUT_I2S       ,
}CEDARX_AUDIO_OUT_TYPE;

typedef enum CEDARX_STREAMTYPE{
	CEDARX_STREAM_NETWORK,
	CEDARX_STREAM_LOCALFILE,
	CEDARX_STREAM_EXTERNAL_BUFFER,
	CEDARX_STREAM_STREAMMING_SOURCE,
}CEDARX_STREAMTYPE;

typedef enum CEDARX_THIRDPART_STREAMTYPE{
	CEDARX_THIRDPART_STREAM_NONE = 0,
	CEDARX_THIRDPART_STREAM_USER0,
	CEDARX_THIRDPART_STREAM_USER1,
	CEDARX_THIRDPART_STREAM_USER2,
	CEDARX_THIRDPART_STREAM_USER3
}CEDARX_THIRDPART_STREAMTYPE;

typedef enum CEDARX_THIRDPART_ENCRPYTED_TYPE{
	CEDARX_THIRDPART_ENCRYPTED_ALL 		 = 0,
	CEDARX_THIRDPART_ENCRYPTED_VIDEO 	 = 1,
	CEDARX_THIRDPART_ENCRYPTED_AUDIO 	 = 2,
	CEDARX_THIRDPART_ENCRYPTED_SUBTITLE  = 4,
	CEDARX_THIRDPART_ENCRYPTED_OTHERS	 = 8,
}CEDARX_THIRDPART_ENCRPYTED_TYPE;

//typedef enum CEDARX_DISPLAY_3D_MODE
//{
//	CEDARX_DISPLAY_3D_MODE_2D,
//	CEDARX_DISPLAY_3D_MODE_3D,
//	CEDARX_DISPLAY_3D_MODE_HALF_PICTURE,
//	CEDARX_DISPLAY_3D_MODE_ANAGLAGH,
//}cedarx_display_3d_mode_e;

typedef enum CEDARX_THIRDPART_DEMUXER_TYPE{
	CEDARX_THIRDPART_DEMUXER_0 = 0x70000001,
	CEDARX_THIRDPART_DEMUXER_1,
	CEDARX_THIRDPART_DEMUXER_2,
	CEDARX_THIRDPART_DEMUXER_3,
}CEDARX_THIRDPART_DEMUXER_TYPE;


//add by weihongqiang.

typedef enum CEDARX_AUDIO_CHANNEL_TYPE {
	CEDARX_AUDIO_CHANNEL_STEREO = 0,
	CEDARX_AUDIO_CHANNEL_LEFT ,
	CEDARX_AUDIO_CHANNEL_RIGHT,
} CEDARX_AUDIO_CHANNEL_TYPE;


#define CEDARX_MAX_AUDIO_STREAM_NUM    16
#define CEDARX_MAX_VIDEO_STREAM_NUM     1
#define CEDARX_MAX_SUBTITLE_STREAM_NUM 32

#define CEDARX_MAX_LANGUANGE_LEN (32)
#define CEDARX_MAX_MIME_TYPE_LEN (32)

typedef struct CedarXExternFdDesc{
	int		fd;   //SetDataSource FD
	long long offset;   //byte position to start play.
	//recoder where fd is now.
	long long cur_offset;
	long long length;   //bytes to be played
}CedarXExternFdDesc;

typedef enum {
	VIDEO_THUMB_UNKOWN = 0,
	VIDEO_THUMB_JPEG,
	VIDEO_THUMB_YUVPLANNER,
	VIDEO_THUMB_RGB565,
}VIDEOTHUMBNAILFORMAT;

typedef struct VideoThumbnailInfo{
	int format; //0: JPEG STREAM  1: YUV RAW STREAM 2:RGB565
	int use_hardware_capture;
	int capture_time; //
	int require_width;  //now indicate display_width
	int require_height;
	void *thumb_stream_address;
	int thumb_stream_size;
	int capture_result; //0:fail 1:ok
}VideoThumbnailInfo;

//typedef enum {
//	CEDARX_OUTPUT_SETTING_MODE_PLANNER      = 1<<1,
//	CEDARX_OUTPUT_SETTING_HARDWARE_CONVERT  = 1<<2,
//}CEDARX_VIDEOOUT_MODE;

typedef enum {
	CEDARX_NETWORK_ENGINE_DEFAULT      = 0,
	CEDARX_NETWORK_ENGINE_SFT          = 1,
}CEDARX_NETWORK_ENGINE;

typedef enum {
    RECORDER_MODE_NONE      = 0,
	RECORDER_MODE_AUDIO		= 1<<0	,		// only audio recorder
	RECORDER_MODE_VIDEO		= 1<<1	,		// only video recorder
	RECORDER_MODE_CAMERA	= (RECORDER_MODE_AUDIO | RECORDER_MODE_VIDEO),		// audio and video recorder
}RECORDER_MODE;

typedef enum CEDARX_MEDIA_TYPE{
	CEDARX_MEDIATYPE_NORMAL = 0 ,
	CEDARX_MEDIATYPE_RAWMUSIC   ,
	CEDARX_MEDIATYPE_3D_VIDEO   ,
	CEDARX_MEDIATYPE_DRM_VIDEO ,
	CEDARX_MEDIATYPE_DRM_WVM_VIDEO ,
	CEDARX_MEDIATYPE_DRM_ES_BASED_VIDEO,
	CEDARX_MEDIATYPE_DRM_CONTAINNER_BASED_VIDEO,
	CEDARX_MEDIATYPE_BD,
	CEDARX_SOURCE_MULTI_URL,
}CEDARX_MEDIA_TYPE;

typedef struct CedarXMediaInformations
{
    unsigned char 	mVideoStreamCount;
    unsigned char 	mAudioStreamCount;
    unsigned char 	mSubtitleStreamCount;
    unsigned int  	mDuration;
    unsigned int  	mFlags; //CanSeek etc.
    unsigned int  	media_type;
    unsigned int	media_subtype_3d;
    unsigned int  	_3d_mode;

    struct CedarXAudioInfo {
    	char    cMIMEType[CEDARX_MAX_MIME_TYPE_LEN];
        int     mChannels;
        int     mSampleRate;
        int     mAvgBitrate;
        char    mLang[CEDARX_MAX_LANGUANGE_LEN];
    }mAudioInfo[CEDARX_MAX_AUDIO_STREAM_NUM];

    struct CedarXVideoInfo {
    	char    cMIMEType[CEDARX_MAX_MIME_TYPE_LEN];
        int 	mFrameWidth;
        int 	mFrameHeight;
        int     mFileSelfRotation;  //mov file will write a nRotation. clock wise. 
    }mVideoInfo[CEDARX_MAX_VIDEO_STREAM_NUM];

    struct CedarXSubtitleInfo {
    	//common fd info.
    	CedarXExternFdDesc 	mMajorFd;
    	//fd info for idx+sub.
    	CedarXExternFdDesc 	mMinorFd;//file.
		char    			cMIMEType[CEDARX_MAX_MIME_TYPE_LEN];//type
		char    			mLang[CEDARX_MAX_LANGUANGE_LEN];//language
		int					mSubType;
		//lang index, if there's only one subtitle,
		//set as 0.
		int 				mLangIndex;
        //how many subtitles in a file.
        int					mSubNum;
    }mSubtitleInfo[CEDARX_MAX_SUBTITLE_STREAM_NUM];

} CedarXMediaInformations;

int CDXPlayer_Create(void **inst);
void CDXPlayer_Destroy(void *player);
int CDXRetriever_Create(void **inst);
void CDXRetriever_Destroy(void *retriever);

typedef struct CDXPlayer
{
	void *context;
	int  (*control)(void *player, int cmd, unsigned int para0, unsigned int para1);
}CDXPlayer;

typedef struct CDXRetriever
{
	void *context;
	int  (*control)(void *retriever, int cmd, unsigned int para0, unsigned int para1);
}CDXRetriever;

typedef int (*CedarXCallbackType)(void *cookie, CEDARX_EVENT_TYPE event, void *p_event_data);

typedef struct CedarXPlayerCallbackType{
	void *cookie;   //CedarXPlayer*
	CedarXCallbackType callback;    //CedarXPlayerCallbackWrapper
}CedarXPlayerCallbackType, CedarXRecorderCallbackType;

typedef int (* reqdata_from_dram)(unsigned char *pbuf, unsigned int buflen);

typedef struct tag_ANativeWindowBufferCedarXWrapper //ANativeWindowBuffer
{
    int width;
    int height;
    int stride;
    int format;
    int usage;

    void*   dst;    //memory address
    void*   dstPhy; //Physical memory address
    
    void*   pObjANativeWindowBuffer;   //ANativeWindowBuffer
    
} ANativeWindowBufferCedarXWrapper;

#ifdef __cplusplus
}
#endif

#endif
/* File EOF */
