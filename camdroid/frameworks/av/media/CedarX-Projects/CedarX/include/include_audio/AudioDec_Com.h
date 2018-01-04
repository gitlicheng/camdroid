#ifndef AUDIODEC_COM_H_
#define AUDIODEC_COM_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <ad_cedarlib_com.h>
#include <ad_cedar.h>
#include <asoundlib.h>

#define TEMP_RESAMPLE_BUFFER_SIZE (64*1024)

typedef struct cedar_quality_t
{
	OMX_U32 vbvbuffer_validsize_percent; //[0:100]
	OMX_U32 element_in_vbv;				   //[0:MAX_PIC_NUM_IN_VBV]
}cedar_quality_t;
typedef struct Cedar_raw_data
{
	int nRawDataFlag;//UI set 0:pcm;1:hdmi raw data;2:spdif raw data;
	int RawInitFlag;//if init flag 0:no init raw;1:init raw
	struct pcm_config config;
	struct pcm *pcm;
	unsigned int device;
}cedar_raw_data;
typedef struct AUDIODECDATATYPE AUDIODECDATATYPE;

struct AUDIODECDATATYPE {
	OMX_STATETYPE state;
	OMX_CALLBACKTYPE *pCallbacks;
	OMX_PTR pAppData;
	OMX_HANDLETYPE hSelf;
	OMX_PORT_PARAM_TYPE sPortParam;
    OMX_PARAM_PORTDEFINITIONTYPE sInPortDef;
    OMX_PARAM_PORTDEFINITIONTYPE sOutPortDef;
    OMX_AUDIO_PARAM_PORTFORMATTYPE sInPortFormat;
    OMX_AUDIO_PARAM_PORTFORMATTYPE sOutPortFormat;
    OMX_TUNNELINFOTYPE sInPortTunnelInfo;
    OMX_TUNNELINFOTYPE sOutPortTunnelInfo;
    OMX_PORTSCALLBACKTYPE sInPortCallbackInfo;
    OMX_PORTSCALLBACKTYPE sOutPortCallbackInfo;
    OMX_HANDLETYPE sInPortTunnelHandle;
	OMX_HANDLETYPE sOutPortTunnelHandle;
	OMX_PRIORITYMGMTTYPE sPriorityMgmt;
	OMX_PARAM_BUFFERSUPPLIERTYPE sInBufSupplier;
	OMX_PARAM_BUFFERSUPPLIERTYPE sOutBufSupplier;
	pthread_t thread_id;
	ThrCmdType eTCmd;
	message_queue_t  cmd_queue;
	cdx_sem_t cdx_sem_wait_message;
	pthread_mutex_t mutex_audiodec_thread;
	//cdx_sem_t cdx_sem_wait_outbuffer_empty;
	OMX_S32 demux_type;
	OMX_S32 decode_mode;
    OMX_S32 codec_type;
	OMX_S32 priv_flag;
	OMX_S32 audio_info_flags;
	OMX_S32 file_size;  //used for raw music
	OMX_S32 enable_resample;
	OMX_S32 get_audio_info_flag;
	OMX_S32 volumegain;
	OMX_S32 mute;
	OMX_S32 last_touch_cpu_time;

	OMX_S32   CedarAbsPackHdrLen;
	OMX_U8    CedarAbsPackHdr[16];
	CDX_U32   adcedar_last_valid_pts;
	CDX_S32   is_raw_music_mode;
	OMX_U32	  audio_channel;
	ad_cedar_context ad_cedar_ctx;

	AudioDEC_AC320   *cedar_audio_dec;  //ret = ERR_AUDIO_DEC_NONE and so on
	BsInFor          ad_cedar_info;
	Ac320FileRead    DecFileInfo;
	com_internal     pInternal;
	cedar_raw_data   raw_data;
	CDX_S32			 force_exit;
	void*      libHandle;
	int  (*cedar_init						)(AUDIODECDATATYPE *pAudioDecData);
	void (*cedar_exit						)(AUDIODECDATATYPE *pAudioDecData);
	int  (*cedar_parse_requeset_ain_buffer	)(AUDIODECDATATYPE *pAudioDecData, OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);
	int  (*cedar_parse_update_ain_buffer	)(AUDIODECDATATYPE *pAudioDecData, OMX_BUFFERHEADERTYPE* pBuffer);
	int  (*cedar_dec_requeset_aout_buffer	)(AUDIODECDATATYPE *pAudioDecData, char **aout_write_ptr);
	int  (*cedar_dec_update_aout_buffer		)(AUDIODECDATATYPE *pAudioDecData, int pcm_out_size);
	int  (*cedar_plybk_requeset_aout_buffer	)(AUDIODECDATATYPE *pAudioDecData, unsigned char **aout_read_ptr, int *size);
	int  (*cedar_plybk_update_aout_buffer	)(AUDIODECDATATYPE *pAudioDecData, int pcm_out_size);
	int  (*cedar_plybk_requeset_aout_pts	)(AUDIODECDATATYPE *pAudioDecData, int decode_mode);
	void (*cedar_query_quality				)(AUDIODECDATATYPE *pAudioDecData, cedar_quality_t *aq);
	void (*cedar_query_aout_quality			)(AUDIODECDATATYPE *pAudioDecData, cedar_quality_t *aq);
	void (*cedar_seek_sync					)(AUDIODECDATATYPE *pAudioDecData, CedarXSeekPara *seek_para, int decode_mode);
	
};// AUDIODECDATATYPE;

int  ad_cedar_init(AUDIODECDATATYPE *pAudioDecData);
void ad_cedar_exit(AUDIODECDATATYPE *pAudioDecData);
int  ad_cedar_parse_requeset_ain_buffer(AUDIODECDATATYPE *pAudioDecData, OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);
int  ad_cedar_parse_update_ain_buffer(AUDIODECDATATYPE *pAudioDecData, OMX_BUFFERHEADERTYPE* pBuffer);
int  ad_cedar_dec_requeset_aout_buffer(AUDIODECDATATYPE *pAudioDecData, char **aout_write_ptr);
int  ad_cedar_dec_update_aout_buffer(AUDIODECDATATYPE *pAudioDecData, int pcm_out_size);
int  ad_cedar_plybk_requeset_aout_buffer(AUDIODECDATATYPE *pAudioDecData, unsigned char **aout_read_ptr, int *size);
int  ad_cedar_plybk_update_aout_buffer(AUDIODECDATATYPE *pAudioDecData, int pcm_out_size);
int  ad_cedar_plybk_requeset_aout_pts(AUDIODECDATATYPE *pAudioDecData, int decode_mode);
void ad_cedar_query_quality(AUDIODECDATATYPE *pAudioDecData, cedar_quality_t *aq);
void ad_cedar_query_aout_quality(AUDIODECDATATYPE *pAudioDecData, cedar_quality_t *aq);
void ad_cedar_seek_sync(AUDIODECDATATYPE *pAudioDecData, CedarXSeekPara *seek_para, int decode_mode);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif//AUDIODEC_COM_H_
