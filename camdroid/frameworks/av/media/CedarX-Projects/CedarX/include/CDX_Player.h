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

#ifndef CDX_Player_H_
#define CDX_Player_H_

#include <stdio.h>
#include <CDX_Types.h>
#include <CDX_Component.h>
#include <CDX_Common.h>
#include <CDX_PlayerAPI.h>
#include <cedarx_demux.h>
#include <GetAudio_format.h>
//#include <CDX_Resource_Manager.h>
#include <tmessage.h>
#include <tsemaphore.h>
#include <CDX_UglyDef.h>

//#include "libcedarv.h"
#include <vdecoder.h>

typedef OMX_ERRORTYPE *(*ThirdpartComponentInit)(
		OMX_HANDLETYPE hComponent);

typedef struct CedarXPlayerContext{
	CedarXDataSourceDesc data_src_desc;
	CDX_S32 comp_exist_flags;       //setDataSource() and prepare() decide its value. Record the exist component. Keep value during play.
	CDX_S32 comp_response_flags;    //for prepare(), wait component response to turn state. set value temporary.
	CDX_S32 exit_flags; //when exit, record which component not exit. when set to 0, can send CDX_EVENT_PLAYBACK_COMPLETE. clear when CDXPlayer_SetDataSource(), set value when CDXPlayer_Prepare(), only with VDRENDER_COMP_ID,ADRENDER_COMP_ID
	CDX_S32 exit_flags_backup;
	CDX_S32 eof_flags;  //record which component notify eof. clear when play() and seek().
	CDX_S32 msg_queue_thread_exit;
	CDX_S32 playback_complete_flag;
	CDX_S32 position_invalid;
	CDX_S64 seek_position;
	CEDARX_STATES states;
	CEDARX_STATES transient_states;
	CEDARX_AUDIO_OUT_TYPE audio_out_type;
	CDX_S64 temp_position;
	CDX_S32 is_pausing;
	CDX_S32 is_manual_pause;

	ThirdpartComponentInit thirdpart_component_init;
	void *libHandle;

	void *cdx_player;
	OMX_PTR pAppData;   //CedarXPlayerContext*
	CedarXPlayerCallbackType callback_info;
	CedarXMediainfo cdx_detail_mediainfo;
	CedarXMediaInformations cdx_mediainfo;
	VideoThumbnailInfo vd_thumb_info;
	OMX_S32 file_fmt_type;
	//OMX_S32 is_hardware_init;
	OMX_S32 fatal_error;
	OMX_S32	disable_xxxx_track;
	OMX_S32	disable_media_type;
	OMX_S32	audio_mute_mode; //0: none 1: mute left 2: mute right 3: mute all
	CedarXMetaData cdx_metadata;
	//ve_mutex_t cedarv_req_ctx;

	OMX_S32 sub_enable;
	OMX_S32 curr_subtrack_idx;
	OMX_S32 sub_position;
	OMX_S32 sub_delay;
	OMX_S32 curr_audiotrack_idx;

	CDX_TUNNELLINKTYPE cdx_tunnels_link;

	pthread_mutex_t cdx_player_mutex;
	pthread_mutex_t cdx_event_mutex;
	pthread_t thread_id;
	message_queue_t  msg_queue;
	cdx_sem_t cdx_sem_wait_message;
	pthread_mutex_t msg_sync_mutex;
	int msg_id_processed;
	int msg_id_index;
	int msg_processed_stop_async;

	cdx_sem_t cdx_sem_demux_cmd;
	cdx_sem_t cdx_sem_video_decoder_cmd;
	cdx_sem_t cdx_sem_audio_decoder_cmd;
	cdx_sem_t cdx_sem_video_render_cmd;
	cdx_sem_t cdx_sem_audio_render_cmd;
	cdx_sem_t cdx_sem_clock_cmd;
	cdx_sem_t cdx_sem_subtitle_cmd;

	OMX_HANDLETYPE hnd_demuxer;
	OMX_HANDLETYPE hnd_video_decoder;
	OMX_HANDLETYPE hnd_audio_decoder;
	OMX_HANDLETYPE hnd_video_render;
	OMX_HANDLETYPE hnd_audio_render;
	OMX_HANDLETYPE hnd_clock;
	OMX_HANDLETYPE hnd_subtitle;

	//* for transforming 'side by side' or 'top to bottom' pictures to 'anaglath' pictures.
	//cedarv_anaglath_trans_mode_e	anaglath_trans_mode;
	//OMX_S32							anaglath_trans_on;

	//* for 3d picture display control.
	cedarv_3d_mode_e				_3d_mode;
	//cedarx_display_3d_mode_e		_3d_display_mode;

	OMX_S32 	cedarv_rotation;    // static rotate, 0: no rotate, 1: 90 degree (clock wise), 2: 180, 3: 270, 4: horizon flip, 5: vertical flip;
	OMX_S32 	cedarv_max_width;
	OMX_S32 	cedarv_max_height;
	//OMX_S32 	cedarv_output_setting;  //CEDARX_OUTPUT_SETTING_MODE_PLANNER

	OMX_S32     soft_chip_version;
	OMX_S32		max_resolution; //chip max decode resolution. eg, 1288<<16 | 1288
	OMX_S32		network_engine;

	OMX_S8		player_can_seek;
	//0:normal foramt; 1:ts, 2:m2ts
	OMX_S8		container_type;
	OMX_S8      av_sync;
    OMX_U8     play_bd_file;
    OMX_S32		stream_source_type; //CDX_STREAM_SOURCE_NORMAL_TS
    //OMX_S32    dynamic_rotation;    //the value definition is same as cedarv_rotation .

    CDX_S32     vpsspeed;   //vps play, -40~100, default:0

    CDX_S32    present_screen;
}CedarXPlayerContext;

#include "CDX_PlayerAPI.h"

typedef struct CedarXMediaRetriverContext{
	//int bIsCaptureInit;
	int bCaptureThumbnailStream;    //0:thumbnail picture; 1:thumbnail video stream
	CedarXMetaData cdx_metadata;
	CedarXDataSourceDesc data_src_desc;
	//ve_mutex_t cedarv_req_ctx;
	//int        bIsVeLocked;
	void *vd_thumb_ctx;
}CedarXMediaRetriverContext;

#endif
