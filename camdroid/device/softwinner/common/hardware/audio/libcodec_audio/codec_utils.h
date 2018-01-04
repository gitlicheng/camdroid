
#ifndef __CODEC_UTILS_H__
#define __CODEC_UTILS_H__

#include "asoundlib.h"
#include <pthread.h>
#include <semaphore.h>

typedef enum voice_type {
    BT,
    FM,
    BT_FM,
    CODEC,
    BP,
}voice_type_t;

enum device_type {
    CARD_UNKNOWN=-1,
    CARD_CODEC=0,
    CARD_PCM=1,
    CARD_I2S=2,
};

typedef enum voice_direction {
    DOWNSTREAM,
    UPSTREAM,
}voice_direction_t;

extern char *audio_dev_name[3];
 

typedef enum stream_direction_type {
    SENDER, 
    RECEIVER,
}stream_direction_type_t;

struct dev_stream{
    voice_type_t type;
    stream_direction_type_t direction;
    struct pcm_config config;

    struct pcm * dev;
    char *buf;
    unsigned int buf_size;
};

typedef void *(* voice_thread)(void *param);

struct stream_transfer{
    struct dev_stream *stream_sender;
    struct dev_stream *stream_receiver;

    voice_direction_t  voice_direction;
    sem_t sem;
    pthread_t pid;
    voice_thread func;

    volatile int manage_thread_run_flag;    //flag, 管理线程运行标志
    volatile int voice_thread_run_flag;          //flag, 语音传输标志
    volatile int voice_thread_exit_flag;          //flag, 语音线程退出标志
    volatile int record_flag;          //flag, 语音传输标志
};

int init_stream(struct dev_stream *dev_stream);
void close_stream(struct dev_stream *dev_stream);

void ReduceVolume(char *buf, int size, int repeat);

extern struct pcm_config bt_pcm_out_config;
extern struct pcm_config bt_pcm_in_config ;
extern struct pcm_config codec_out_config ;
extern struct pcm_config codec_in_config ;

//v3 codec
#if 0
/* Mixer control names */
#define MIXER_MASTER_PLAYBACK_VOLUME   					"Master Playback Volume"
#define MIXER_LINEIN_G_BOOST_STAGE_OUTPUT_MIXER_CONTROL	"LINEIN_G boost stage output mixer control"
#define MIXER_LINEOUT_VOLUME_CONTROL					"Lineout volume control"
#define MIXER_PHONE_G_BOOST_STAGE_OUTPUT_MIXER_CONTROL	"PHONE_G boost stage output mixer control"
#define MIXER_PHONEOUT_GAIN_CONTROL						"Phoneout gain control"
#define MIXER_PHONEP_PHONEN_PRE_AMP_GAIN_CONTROL		"PHONEP-PHONEN pre-amp gain control"

#define MIXER_AUDIO_MIC1_GAIN_CONTROL		"MIC1 boost AMP gain control"
#define MIXER_AUDIO_MIC2_GAIN_CONTROL		"MIC2 boost AMP gain control"

//#define MIXER_AUDIO_SPK_SWITCH                          "Audio Spk Switch"
#define MIXER_AUDIO_PHONE_OUT             				"Audio phone out"
#define MIXER_AUDIO_PHONE_IN             				"Audio phone in"
#define MIXER_AUDIO_PHONE_RECORD             			"Audio phone record"
#define MIXER_AUDIO_EARPIECE_OUT             			"Audio earpiece out"
#define MIXER_AUDIO_HEADPHONE_OUT             			"Audio headphone out"
#define MIXER_AUDIO_SPEAKER_OUT             			"Audio speaker out"
#define MIXER_AUDIO_LINEIN_RECORD             			"Audio linein record"
#define MIXER_AUDIO_LINEIN_IN             				"Audio linein in"

#define MIXER_AUDIO_DAC_PHONE_OUT	"Audio dac phoneout"
#define MIXER_AUDIO_ADC_PHONE_IN	"Audio adc phonein"
#define MIXER_AUDIO_PHONE_MAIN_MIC 	"Audio phone mic"
#define MIXER_AUDIO_PHONE_HEADSET_MIC 	"Audio phone headsetmic"
#define MIXER_AUDIO_PHONE_VOICE_RECORDER 	"Audio phone voicerecord"
#define MIXER_AUDIO_FM_HEADSET             				"Audio fm headset"
#define MIXER_AUDIO_FM_SPEAKER             				"Audio fm speaker"
#define MIXER_AUDIO_NORMAL_SPEAKER_HEADSET    				"Speaker Function"

#define MIXER_AUDIO_PHONE_ENDCALL 	"Audio phone endcall"
#define MIXER_AUDIO_FM_RECORD	 	"Audio linein record"

struct mixer_ctls
{
    struct mixer_ctl *master_playback_volume;		
    struct mixer_ctl *linein_g_boost_stage_output_mixer_control;	
    struct mixer_ctl *lineout_volume_control;	
    struct mixer_ctl *phone_g_boost_stage_output_mixer_control;
    struct mixer_ctl *phoneout_gain_control;	
    struct mixer_ctl *phonep_phonen_pre_amp_gain_control;		
    struct mixer_ctl *audio_spk_switch;
    struct mixer_ctl *audio_phone_out;     
    struct mixer_ctl *audio_phone_in ;     
    struct mixer_ctl *audio_phone_record;  
    struct mixer_ctl *audio_earpiece_out;   
    struct mixer_ctl *audio_headphone_out;  
    struct mixer_ctl *audio_speaker_out;   
    struct mixer_ctl *audio_linein_record ;
    struct mixer_ctl *audio_linein_in ;    
    struct mixer_ctl *audio_fm_headset ;    
    struct mixer_ctl *audio_fm_speaker ;    
    struct mixer_ctl *audio_adc_phone_in;   
    struct mixer_ctl *audio_dac_phone_out;   
    struct mixer_ctl *audio_phone_main_mic;
    struct mixer_ctl *audio_phone_headset_mic; 
    struct mixer_ctl *audio_phone_voice_record; 
    struct mixer_ctl *audio_phone_end_call; 
    struct mixer_ctl *audio_fm_record; 
    struct mixer_ctl *audio_spk_headset_switch;
    struct mixer_ctl *audio_mic1_gain; 
    struct mixer_ctl *audio_mic2_gain;
};

#else
//v3 codec
/* Mixer control names */
#define MIXER_MASTER_PLAYBACK_VOLUME   					"Master Playback Volume"
#define MIXER_LINE_VOLUME					            "Line Volume"
#define MIXER_MIC1_G_BOOST_STAGE_OUTPUT_MIXER_CONTROL	"MIC1_G boost stage output mixer control"
#define MIXER_MIC2_G_BOOST_STAGE_OUTPUT_MIXER_CONTROL	"MIC2_G boost stage output mixer control"

#define MIXER_AUDIO_MIC1_GAIN_CONTROL		            "MIC1 boost AMP gain control"
#define MIXER_AUDIO_MIC2_GAIN_CONTROL		            "MIC2 boost AMP gain control"

#define MIXER_ADC_INPUT_GAIN_CTRL                       "ADC input gain ctrl"

#define MIXER_AUDIO_HEADPHONE_OUT             			"Audio headphone out"
#define MIXER_AUDIO_SPEAKER_OUT             			"Audio speaker out"

#define MIXER_AUDIO_MAIN_MIC                            "Audio main mic"
#define MIXER_AUDIO_SUB_MIC                             "Audio sub mic"

#define MIXER_AUDIO_RECORD_SOURCE                       "Audio record source"
#define MIXER_AUDIO_NOISE_REDUCED             			"Audio noise reduced"
#define MIXER_AUDIO_LINEIN             				    "Audio linein"

#define MIXER_AUDIO_CAPTURE_ROUTE_SWITCH                "Audio capture route switch"
#define MIXER_AUDIO_CLEAR_PATH                          "Audio clear path"
#define MIXER_AUDIO_NORMAL_SPEAKER_HEADSET    			"Speaker Function"

struct mixer_ctls
{
    struct mixer_ctl *master_playback_volume;		
    struct mixer_ctl *line_volume;	
    struct mixer_ctl *mic1_boost_stage_output_mixer_control;
    struct mixer_ctl *mic2_boost_stage_output_mixer_control;
    struct mixer_ctl *audio_mic1_gain;
    struct mixer_ctl *audio_mic2_gain;
    struct mixer_ctl *adc_input_gain_ctrl; 
    struct mixer_ctl *audio_headphone_out;
    struct mixer_ctl *audio_speaker_out;   
    struct mixer_ctl *audio_main_mic;
    struct mixer_ctl *audio_sub_mic;       
    struct mixer_ctl *audio_record_source;   
    struct mixer_ctl *audio_noise_reduced;  
    struct mixer_ctl *audio_linein;  
    struct mixer_ctl *audio_capture_route_switch;
    struct mixer_ctl *audio_clear_path; 
    struct mixer_ctl *audio_spk_headset_switch;
};
#endif


int get_mixer(struct mixer_ctls *mixer_ctls);
extern void c_plus_plus_grabPartialWakeLock();
extern void c_plus_plus_releaseWakeLock();
void grabPartialWakeLock();
void releaseWakeLock();
#endif



