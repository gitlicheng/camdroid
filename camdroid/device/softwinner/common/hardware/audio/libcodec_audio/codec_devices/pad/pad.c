
#define LOG_TAG "codec_audio_pad"
#define LOG_NDEBUG 0

#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>

#include <cutils/properties.h>

#include <system/audio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "hal_codec.h"
#include "pad.h"


static int set_normal_volume(struct codec_client *client, int path, int vol)
{
	int headset_on=0, headphone_on=0, speaker_on=0;

	headset_on = path & AUDIO_DEVICE_OUT_WIRED_HEADSET;  // hp4p
	headphone_on = path & AUDIO_DEVICE_OUT_WIRED_HEADPHONE; // hp3p 
	speaker_on = path & AUDIO_DEVICE_OUT_SPEAKER;

	if (speaker_on){
		ALOGV("****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
		mixer_ctl_set_value(client->mixer_ctls->lineout_volume_control, 0, vol);
	} else if ((headset_on || headphone_on)){
		ALOGV("****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
		mixer_ctl_set_value(client->mixer_ctls->master_playback_volume, 0, vol);
	}

	return 0;
}

static int set_normal_path(struct codec_client *client, int path)
{
	int headset_on=0, headphone_on=0, speaker_on=0, earpiece_on=0;
	int switch_to_headset  =0;
	int ret = -1, fd=0;
	char prop_value[20]={0};
	char h2w_state[2]={0};

	headset_on = path & AUDIO_DEVICE_OUT_WIRED_HEADSET;  // hp4p
	headphone_on = path & AUDIO_DEVICE_OUT_WIRED_HEADPHONE; // hp3p 
	speaker_on = path & AUDIO_DEVICE_OUT_SPEAKER;
	earpiece_on = path & AUDIO_DEVICE_OUT_EARPIECE;

        mixer_ctl_set_value(client->mixer_ctls->audio_linein_in, 0, 0);  //turn off fm

	ret = property_get("dev.bootcomplete", prop_value, "0");
	if (ret > 0)
	{
		if (atoi(prop_value) == 0){
			fd = open("/sys/class/switch/h2w/state", O_RDONLY);
			if(fd>0){
				ret = read(fd, h2w_state, sizeof(h2w_state));
				close(fd);

				if ( (atoi(h2w_state) == 2 || atoi(h2w_state) == 1) )
				{
					switch_to_headset  =1;
				}
			}
		}
	}

	if (((headset_on || headphone_on) && speaker_on)){
		ALOGV("in normal mode, headset and speaker on,****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
		mixer_ctl_set_enum_by_string(client->mixer_ctls->audio_spk_headset_switch, "spk_headset");
	} else if(earpiece_on) {
		mixer_ctl_set_enum_by_string(client->mixer_ctls->audio_spk_headset_switch, "spk");
		ALOGV("in earpiece mode, pad no earpiece but spk,****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__); //no earpiece
	} else if(switch_to_headset) {
		mixer_ctl_set_enum_by_string(client->mixer_ctls->audio_spk_headset_switch, "headset");
		ALOGV("in boot switch_to_headset mode, headset,****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__); 
		switch_to_headset = 0;
	} else {
		ALOGV("in normal mode, headset or speaker on,****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
		mixer_ctl_set_enum_by_string(client->mixer_ctls->audio_spk_headset_switch, speaker_on ? "spk" : "headset");
	}
	return 0;
}

static int set_normal_record_enable(struct codec_client *client, bool enable)
{
	mixer_ctl_set_value(client->mixer_ctls->audio_fm_record, 0, 0);
	mixer_ctl_set_value(client->mixer_ctls->audio_phone_voice_record, 0, 0);
	ALOGV("normal record mode 4,****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
	return 0;
}

static int set_normal_record(struct codec_client *client, int path)
{
	ALOGV("normal record mode 4,****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
	return 0;
}


static int mixer_vol[]={0,1,2,3,4,5,6,7}; //0~15
static int spk_vol[]={16,18,21,23,25,26,28,30}; //0~31
static int hp_vol[]={40,44,48,50,52,56,58,61}; //0~62
static int set_fm_volume(struct codec_client *client, int path, int volume)
{
		int speaker_on=0,headset_on=0 ,headphone_on=0;
		int speaker_vol=0, headset_vol=0;

	headset_on = path & AUDIO_DEVICE_OUT_WIRED_HEADSET;  // hp4p
	headphone_on = path & AUDIO_DEVICE_OUT_WIRED_HEADPHONE; // hp3p 
	speaker_on = path & AUDIO_DEVICE_OUT_SPEAKER;



		int val = 0;
		if (volume >= 10) {
			val = 7 ;	
			speaker_vol = 25;
			headset_vol = 35;
		} else if (volume >= 8){
			val = 6 ;	
			speaker_vol = 25;
			headset_vol = 35;
		} else if (volume >= 6){
			val = 5 ;	
			speaker_vol = 25;
			headset_vol = 35;
		} else if (volume >= 4){
			val = 4 ;	
			speaker_vol = 25;
			headset_vol = 35;
		} else if (volume >= 2){
			val = 3 ;	
			speaker_vol = 25;
			headset_vol = 35;
		} else {
			val = 2 ;	
			speaker_vol = 25;
			headset_vol = 35;
		}

	mixer_ctl_set_value(client->mixer_ctls->linein_g_boost_stage_output_mixer_control, 0, mixer_vol[val]);

	if (speaker_on){
		mixer_ctl_set_value(client->mixer_ctls->lineout_volume_control, 0, spk_vol[val]);
	} else {
		mixer_ctl_set_value(client->mixer_ctls->master_playback_volume, 0, hp_vol[val]);
	} 
	ALOGV("4 set fm , adev_set_voice_volume, volume: %d, val=%d", volume, val);
	return 0;
}

static int set_fm_path(struct codec_client *client, int path)
{
	int headset_on=0, headphone_on=0, speaker_on=0;

	headset_on = path & AUDIO_DEVICE_OUT_WIRED_HEADSET;  // hp4p
	headphone_on = path & AUDIO_DEVICE_OUT_WIRED_HEADPHONE; // hp3p 
	speaker_on = path & AUDIO_DEVICE_OUT_SPEAKER;

	mixer_ctl_set_value(client->mixer_ctls->audio_linein_in, 0, 1);  

	if (speaker_on){
		mixer_ctl_set_value(client->mixer_ctls->audio_headphone_out, 0, 0);
		mixer_ctl_set_value(client->mixer_ctls->audio_speaker_out, 0, 1);
	} else {
		mixer_ctl_set_value(client->mixer_ctls->audio_speaker_out, 0, 0);
		mixer_ctl_set_value(client->mixer_ctls->audio_headphone_out, 0, 1);
	} 
	ALOGV("FM mode 4, devices is %s, ****LINE:%d,FUNC:%s", speaker_on ? "spk" : "headset",__LINE__,__FUNCTION__);

	return 0;
}
static int set_fm_record_enable(struct codec_client *client, bool enable)
{
	mixer_ctl_set_value(client->mixer_ctls->audio_phone_voice_record, 0, 0);

	if (enable){
		mixer_ctl_set_value(client->mixer_ctls->audio_fm_record, 0, 1);
	} else {
		mixer_ctl_set_value(client->mixer_ctls->audio_fm_record, 0, 0);
	}
	ALOGV("fm record mode 4,****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
	return 0;
}

static int set_fm_record(struct codec_client *client, int path)
{
	ALOGV("FM record mode 4, ****LINE:%d,FUNC:%s", __LINE__,__FUNCTION__);
	return 0;
}



int pad_init(void)
{
	ALOGV("****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
	return 0;
}

void pad_exit(void)
{
	ALOGV("****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
}




struct normal_ops pad_normal_ops = {
    .set_normal_volume=set_normal_volume,
    .set_normal_path=set_normal_path,
    .set_normal_record_enable =set_normal_record_enable, 
    .set_normal_record=set_normal_record,
};

struct fm_ops pad_fm_ops = {
    .set_fm_volume=set_fm_volume,
    .set_fm_path=set_fm_path,
    .set_fm_record_enable =set_fm_record_enable, 
    .set_fm_record=set_fm_record,
};


