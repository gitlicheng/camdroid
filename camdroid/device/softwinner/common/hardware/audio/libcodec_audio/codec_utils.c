
#define LOG_TAG "codec_audio"
#define LOG_NDEBUG 0

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <utils/Log.h>
#include <cutils/properties.h>

#include "asoundlib.h"

#include "codec_utils.h"

struct pcm_config bp_i2s_out_config =
{
	.channels = 1,
	.rate = 8000,
	.period_size = 512,
	.period_count = 4,
	.format = PCM_FORMAT_S16_LE,
	.start_threshold = 0,
	.stop_threshold = 0,
	.silence_threshold = 0,
};

struct pcm_config bp_i2s_in_config =
{
	.channels = 1,
	.rate = 8000,
	.period_size = 512,
	.period_count = 4,
	.format = PCM_FORMAT_S16_LE,
	.start_threshold = 0,
	.stop_threshold = 0,
	.silence_threshold = 0,
};

struct pcm_config bt_pcm_out_config =
{
	.channels = 1,
	.rate = 8000,
	.period_size = 512,
	.period_count =4,
	.format = PCM_FORMAT_S16_LE,
	.start_threshold = 0,
	.stop_threshold = 0,
	.silence_threshold = 0,
};

struct pcm_config bt_pcm_in_config =
{
	.channels = 1,
	.rate = 8000,                
	.period_size = 512,          
	.period_count = 4,           
	.format = PCM_FORMAT_S16_LE, 
	.start_threshold = 0,        
	.stop_threshold = 0,         
	.silence_threshold = 0,  
};

struct pcm_config codec_out_config =
{
	.channels = 1,
	.rate = 8000,                
	.period_size = 512,          
	.period_count = 4,           
	.format = PCM_FORMAT_S16_LE, 
	.start_threshold = 0,        
	.stop_threshold = 0,         
	.silence_threshold = 0,  

};
struct pcm_config codec_in_config =
{
	.channels = 1,
	.rate = 8000,                
	.period_size = 512,          
	.period_count = 4,           
	.format = PCM_FORMAT_S16_LE, 
	.start_threshold = 0,        
	.stop_threshold = 0,         
	.silence_threshold = 0,  
};


void grabPartialWakeLock() 
{
	c_plus_plus_grabPartialWakeLock();
}

void releaseWakeLock() 
{
	c_plus_plus_releaseWakeLock();
}

char *audio_dev_name[3]={"audiocodec","sndpcm", "sndi2s"} ;

int init_stream(struct dev_stream *dev_stream) 
{
	enum device_type dev_type = CARD_UNKNOWN;
	int dev_direction =0;
	int dev_node = -1;

	switch (dev_stream->type){
		case BT:
			dev_type = CARD_PCM;
			break;
		case BT_FM:
			dev_type = CARD_PCM;
			break;	
		case BP:
			dev_type = CARD_I2S;
			break;
		case FM:
			dev_type = CARD_I2S;
			break;
		case CODEC:
			dev_type = CARD_CODEC;
			break;
		default:
			dev_type = CARD_UNKNOWN;
			break;
	};

	if(dev_type == CARD_UNKNOWN){
		ALOGE("unknown stream");
		return -1;
	}

	dev_node = pcm_get_node_number(audio_dev_name[dev_type]);
	if (dev_node < 0) {
		ALOGE("err: get %s node number failed ", audio_dev_name[dev_type]);
		return -1;
	}


	dev_direction = dev_stream->direction == SENDER ? PCM_IN : PCM_OUT;

	dev_stream->dev = pcm_open(dev_node, 0, dev_direction, &(dev_stream->config));
	if (!pcm_is_ready(dev_stream->dev )) {
		ALOGE("err: Unable to open  device (%s)", pcm_get_error(dev_stream->dev));
		goto open_failed;
	}

	if (dev_direction == PCM_IN){ //only pcm_in alloc buffer
		dev_stream->buf_size = pcm_get_buffer_size(dev_stream->dev);
		dev_stream->buf = (void *)malloc(dev_stream->buf_size);
		if (dev_stream->buf == NULL) {
			ALOGE("Unable to allocate %d bytes", dev_stream->buf_size);
			goto malloc_failed;
		}
		//ALOGD("sender stream type:%d, pcm_read buf=%d bytes",dev_stream->type, dev_stream->buf_size);
	}
	memset(dev_stream->buf, 0, dev_stream->buf_size);

	ALOGD("dev_stream dev node =%d, type=%d, direction:%s, buf size:%d", 
			dev_node, dev_stream->type, dev_stream->direction == SENDER ? "PCM_IN" : "PCM_OUT", pcm_get_buffer_size(dev_stream->dev));

    	return 0;
malloc_failed:
	if (dev_stream->dev){
		pcm_close(dev_stream->dev);	
	}

open_failed:
    	return -1;
}

void close_stream(struct dev_stream *dev_stream) 
{

	if (dev_stream->buf){
		free(dev_stream->buf);	
	}

	if (dev_stream->dev){
		pcm_close(dev_stream->dev);	
	}
}

void ReduceVolume(char *buf, int size, int repeat)
{
        int i,j;
        int zhen_shu;
        signed long minData = -0x8000;
        signed long maxData = 0x7FFF;
        signed short data ;
        unsigned char low, hight;

        if(!size){
                return;
        }   

	zhen_shu = size - size%2;

        for(i=0; i<zhen_shu; i+=2){
		low = buf[i];
                hight = buf[i+1];
                data = low | (hight << 8);  
                for(j=0; j< repeat; j++){
                        data = data / 1.25;    
                        if(data < minData){
                                data = minData; 
                        } else if (data > 0x7fff){
                                data = maxData;
                        }   
                }   
                buf[i] = (data) & 0x00ff;
                buf[i+1] = ((data)>>8) & 0xff;
        } 
}

#if 0
int get_mixer(struct mixer_ctls *mixer_ctls)
{
    struct mixer *mixer;

    mixer = mixer_open(0);
    if (!mixer) {
	    ALOGE("Unable to open the mixer, aborting.");
	    return -1;
    }


    mixer_ctls->phonep_phonen_pre_amp_gain_control = mixer_get_ctl_by_name(mixer,
		    MIXER_PHONEP_PHONEN_PRE_AMP_GAIN_CONTROL);
    if (!mixer_ctls->phonep_phonen_pre_amp_gain_control) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_PHONEP_PHONEN_PRE_AMP_GAIN_CONTROL);
	    goto error_out;
    }

    mixer_ctls->phoneout_gain_control = mixer_get_ctl_by_name(mixer,
		    MIXER_PHONEOUT_GAIN_CONTROL);
    if (!mixer_ctls->phoneout_gain_control) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_PHONEOUT_GAIN_CONTROL);
	    goto error_out;
    }

    mixer_ctls->phone_g_boost_stage_output_mixer_control = mixer_get_ctl_by_name(mixer,
		    MIXER_PHONE_G_BOOST_STAGE_OUTPUT_MIXER_CONTROL);
    if (!mixer_ctls->phone_g_boost_stage_output_mixer_control) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_PHONE_G_BOOST_STAGE_OUTPUT_MIXER_CONTROL);
	    goto error_out;
    }

    mixer_ctls->master_playback_volume = mixer_get_ctl_by_name(mixer,
		    MIXER_MASTER_PLAYBACK_VOLUME);
    if (!mixer_ctls->master_playback_volume) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_MASTER_PLAYBACK_VOLUME);
	    goto error_out;
    }

    mixer_ctls->lineout_volume_control = mixer_get_ctl_by_name(mixer,
		    MIXER_LINEOUT_VOLUME_CONTROL);
    if (!mixer_ctls->lineout_volume_control) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_LINEOUT_VOLUME_CONTROL);
	    goto error_out;
    }

    mixer_ctls->linein_g_boost_stage_output_mixer_control = mixer_get_ctl_by_name(mixer,
		    MIXER_LINEIN_G_BOOST_STAGE_OUTPUT_MIXER_CONTROL);
    if (!mixer_ctls->linein_g_boost_stage_output_mixer_control) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_LINEIN_G_BOOST_STAGE_OUTPUT_MIXER_CONTROL);
	    goto error_out;
    }

    mixer_ctls->audio_phone_out = mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_PHONE_OUT);
    if (!mixer_ctls->audio_phone_out) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_AUDIO_PHONE_OUT);
	    goto error_out;
    }
    mixer_ctls->audio_phone_in = mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_PHONE_IN);
    if (!mixer_ctls->audio_phone_in) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_AUDIO_PHONE_IN);
	    goto error_out;
    }

    mixer_ctls->audio_earpiece_out = mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_EARPIECE_OUT);
    if (!mixer_ctls->audio_earpiece_out) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_AUDIO_EARPIECE_OUT);
	    goto error_out;
    }
    mixer_ctls->audio_headphone_out = mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_HEADPHONE_OUT);
    if (!mixer_ctls->audio_headphone_out) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_AUDIO_HEADPHONE_OUT);
	    goto error_out;
    }
    mixer_ctls->audio_speaker_out = mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_SPEAKER_OUT);
    if (!mixer_ctls->audio_speaker_out) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_AUDIO_SPEAKER_OUT);
	    goto error_out;
    }

    mixer_ctls->audio_adc_phone_in= mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_ADC_PHONE_IN);
    if (!mixer_ctls->audio_adc_phone_in) {
	    ALOGE("Unable to find '%s' mixer control", MIXER_AUDIO_ADC_PHONE_IN);
	    goto error_out;
    }
    mixer_ctls->audio_dac_phone_out= mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_DAC_PHONE_OUT);
    if (!mixer_ctls->audio_dac_phone_out) {
	    ALOGE("Unable to find '%s' mixer control", MIXER_AUDIO_DAC_PHONE_OUT);
	    goto error_out;
    }

    mixer_ctls->audio_phone_main_mic= mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_PHONE_MAIN_MIC);
    if (!mixer_ctls->audio_phone_main_mic) {
	    ALOGE("Unable to find '%s' mixer control", MIXER_AUDIO_PHONE_MAIN_MIC);
	    goto error_out;
    }

    mixer_ctls->audio_phone_headset_mic= mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_PHONE_HEADSET_MIC);
    if (!mixer_ctls->audio_phone_headset_mic) {
	    ALOGE("Unable to find '%s' mixer control", MIXER_AUDIO_PHONE_HEADSET_MIC);
	    goto error_out;
    }

    mixer_ctls->audio_phone_voice_record= mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_PHONE_VOICE_RECORDER);
    if (!mixer_ctls->audio_phone_voice_record) {
	    ALOGE("Unable to find '%s' mixer control", MIXER_AUDIO_PHONE_VOICE_RECORDER);
	    goto error_out;
    }

    mixer_ctls->audio_linein_in = mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_LINEIN_IN);
    if (!mixer_ctls->audio_linein_in) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_AUDIO_LINEIN_IN);
	    goto error_out;
    }
/*
    mixer_ctls->audio_fm_headset = mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_FM_HEADSET);
    if (!mixer_ctls->audio_fm_headset) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_AUDIO_FM_HEADSET);
	    goto error_out;
    }

    mixer_ctls->audio_fm_speaker = mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_FM_SPEAKER);
    if (!mixer_ctls->audio_fm_speaker) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_AUDIO_FM_SPEAKER);
	    goto error_out;
    }

    mixer_ctls->audio_spk_switch = mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_SPK_SWITCH);
    if (!mixer_ctls->audio_spk_switch) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_AUDIO_SPK_SWITCH);
	    goto error_out;
    }
*/
    mixer_ctls->audio_spk_headset_switch = mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_NORMAL_SPEAKER_HEADSET);
    if (!mixer_ctls->audio_spk_headset_switch) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_AUDIO_NORMAL_SPEAKER_HEADSET);
	    goto error_out;
    }

    mixer_ctls->audio_phone_end_call = mixer_get_ctl_by_name(mixer,MIXER_AUDIO_PHONE_ENDCALL);
    if (!mixer_ctls->audio_phone_end_call) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_AUDIO_PHONE_ENDCALL);
	    goto error_out;
    }

    mixer_ctls->audio_fm_record = mixer_get_ctl_by_name(mixer,MIXER_AUDIO_FM_RECORD);
    if (!mixer_ctls->audio_fm_record) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_AUDIO_FM_RECORD);
	    goto error_out;
    }

    mixer_ctls->audio_mic1_gain = mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_MIC1_GAIN_CONTROL);
    if (!mixer_ctls->audio_mic1_gain) {
	    ALOGE("Unable to find '%s' mixer control", MIXER_AUDIO_MIC1_GAIN_CONTROL);
	    goto error_out;
    }

    mixer_ctls->audio_mic2_gain = mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_MIC2_GAIN_CONTROL);
    if (!mixer_ctls->audio_mic2_gain) {
	    ALOGE("Unable to find '%s' mixer control", MIXER_AUDIO_MIC2_GAIN_CONTROL);
	    goto error_out;
    }

    return 0;

error_out:  
    mixer_close(mixer);
    return -1;
}
#else
//v3 codec
int get_mixer(struct mixer_ctls *mixer_ctls)
{
    struct mixer *mixer;

    mixer = mixer_open(0);
    if (!mixer) {
	    ALOGE("Unable to open the mixer, aborting.");
	    return -1;
    }

    mixer_ctls->master_playback_volume = mixer_get_ctl_by_name(mixer,
		    MIXER_MASTER_PLAYBACK_VOLUME);
    if (!mixer_ctls->master_playback_volume) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_MASTER_PLAYBACK_VOLUME);
	    goto error_out;
    }

    mixer_ctls->line_volume = mixer_get_ctl_by_name(mixer,
		    MIXER_LINE_VOLUME);
    if (!mixer_ctls->line_volume) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_LINE_VOLUME);
	    goto error_out;
    }

    mixer_ctls->mic1_boost_stage_output_mixer_control = mixer_get_ctl_by_name(mixer,
		    MIXER_MIC1_G_BOOST_STAGE_OUTPUT_MIXER_CONTROL);
    if (!mixer_ctls->mic1_boost_stage_output_mixer_control) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_MIC1_G_BOOST_STAGE_OUTPUT_MIXER_CONTROL);
	    goto error_out;
    }

    mixer_ctls->mic2_boost_stage_output_mixer_control = mixer_get_ctl_by_name(mixer,
		    MIXER_MIC2_G_BOOST_STAGE_OUTPUT_MIXER_CONTROL);
    if (!mixer_ctls->mic2_boost_stage_output_mixer_control) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_MIC2_G_BOOST_STAGE_OUTPUT_MIXER_CONTROL);
	    goto error_out;
    }

    mixer_ctls->audio_mic1_gain = mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_MIC1_GAIN_CONTROL);
    if (!mixer_ctls->audio_mic1_gain) {
	    ALOGE("Unable to find '%s' mixer control", MIXER_AUDIO_MIC1_GAIN_CONTROL);
	    goto error_out;
    }

    mixer_ctls->audio_mic2_gain = mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_MIC2_GAIN_CONTROL);
    if (!mixer_ctls->audio_mic2_gain) {
	    ALOGE("Unable to find '%s' mixer control", MIXER_AUDIO_MIC2_GAIN_CONTROL);
	    goto error_out;
    }

    mixer_ctls->adc_input_gain_ctrl = mixer_get_ctl_by_name(mixer,
		    MIXER_ADC_INPUT_GAIN_CTRL);
    if (!mixer_ctls->adc_input_gain_ctrl) {
	    ALOGE("Unable to find '%s' mixer control", MIXER_ADC_INPUT_GAIN_CTRL);
	    goto error_out;
    }

    mixer_ctls->audio_headphone_out = mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_HEADPHONE_OUT);
    if (!mixer_ctls->audio_headphone_out) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_AUDIO_HEADPHONE_OUT);
	    goto error_out;
    }

    mixer_ctls->audio_speaker_out = mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_SPEAKER_OUT);
    if (!mixer_ctls->audio_speaker_out) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_AUDIO_SPEAKER_OUT);
	    goto error_out;
    }

    mixer_ctls->audio_main_mic = mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_MAIN_MIC);
    if (!mixer_ctls->audio_main_mic) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_AUDIO_MAIN_MIC);
	    goto error_out;
    }

    mixer_ctls->audio_sub_mic = mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_SUB_MIC);
    if (!mixer_ctls->audio_sub_mic) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_AUDIO_SUB_MIC);
	    goto error_out;
    }

    mixer_ctls->audio_record_source = mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_RECORD_SOURCE);
    if (!mixer_ctls->audio_record_source) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_AUDIO_RECORD_SOURCE);
	    goto error_out;
    }

    mixer_ctls->audio_noise_reduced = mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_NOISE_REDUCED);
    if (!mixer_ctls->audio_noise_reduced) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_AUDIO_NOISE_REDUCED);
	    goto error_out;
    }

    mixer_ctls->audio_linein = mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_LINEIN);
    if (!mixer_ctls->audio_linein) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_AUDIO_LINEIN);
	    goto error_out;
    }

    mixer_ctls->audio_capture_route_switch = mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_CAPTURE_ROUTE_SWITCH);
    if (!mixer_ctls->audio_capture_route_switch) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_AUDIO_CAPTURE_ROUTE_SWITCH);
	    goto error_out;
    }

    mixer_ctls->audio_clear_path = mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_CLEAR_PATH);
    if (!mixer_ctls->audio_clear_path) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_AUDIO_CLEAR_PATH);
	    goto error_out;
    }

    mixer_ctls->audio_spk_headset_switch = mixer_get_ctl_by_name(mixer,
		    MIXER_AUDIO_NORMAL_SPEAKER_HEADSET);
    if (!mixer_ctls->audio_spk_headset_switch) {
	    ALOGE("Unable to find '%s' mixer control",MIXER_AUDIO_NORMAL_SPEAKER_HEADSET);
	    goto error_out;
    }

    return 0;

error_out:
    mixer_close(mixer);
    return -1;
}


#endif


