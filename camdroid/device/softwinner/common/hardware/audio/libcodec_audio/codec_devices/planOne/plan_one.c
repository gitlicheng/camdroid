
#define LOG_TAG "codec_audio_plan_one"
#define LOG_NDEBUG 0

#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>

#include <cutils/properties.h>

#include <system/audio.h>


#include "hal_codec.h"
#include "plan_one.h"


static int no_earpiece = 0;
static bool last_path_is_bt = false ;
static bool wake_lock = false;
static bool in_call = false;

static void set_headphone_volume(struct mixer_ctl *reg, int volume)
{
	int i = 0; 
	int val=0;

	val = mixer_ctl_get_value(reg, 0);

	if(val == volume){
		//ALOGV("volume equal");
		return;
	} 

	if(val >volume){
		for(i=val; i>=volume; i--){
			mixer_ctl_set_value(reg, 0, i);
			usleep(5000);
		}
	} else {
		for(i=val; i<=volume; i++){
			mixer_ctl_set_value(reg, 0, i);
			usleep(5000);
		}
	}
}
static int set_normal_volume(struct codec_client *client, int path, int vol)
{
	int headset_on=0, headphone_on=0, speaker_on=0;

	headset_on = path & AUDIO_DEVICE_OUT_WIRED_HEADSET;  // hp4p
	headphone_on = path & AUDIO_DEVICE_OUT_WIRED_HEADPHONE; // hp3p 
	speaker_on = path & AUDIO_DEVICE_OUT_SPEAKER;

	if (speaker_on){
		ALOGV("****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
		//mixer_ctl_set_value(client->mixer_ctls->lineout_volume_control, 0, vol);
		set_headphone_volume(client->mixer_ctls->lineout_volume_control, vol);
	} else if ((headset_on || headphone_on)){
		ALOGV("****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
		//mixer_ctl_set_value(client->mixer_ctls->master_playback_volume, 0, vol);
		set_headphone_volume(client->mixer_ctls->master_playback_volume,vol);
	}

	return 0;
}

static int set_normal_path(struct codec_client *client, int path)
{
	int headset_on=0, headphone_on=0, speaker_on=0, earpiece_on=0;

	headset_on = path & AUDIO_DEVICE_OUT_WIRED_HEADSET;  // hp4p
	headphone_on = path & AUDIO_DEVICE_OUT_WIRED_HEADPHONE; // hp3p 
	speaker_on = path & AUDIO_DEVICE_OUT_SPEAKER;
	earpiece_on = path & AUDIO_DEVICE_OUT_EARPIECE;

	if(last_path_is_bt){
		plan_one_stop_bt_voice();
		last_path_is_bt = false;
	}

	if(in_call){
		in_call = false;
		mixer_ctl_set_value(client->mixer_ctls->audio_phone_end_call, 0, 1);
	}


	if(wake_lock){
		releaseWakeLock();
		wake_lock=false;
	}

        mixer_ctl_set_value(client->mixer_ctls->audio_linein_in, 0, 0);  //turn off fm


	if ((headset_on || headphone_on) && speaker_on){
		ALOGV("in normal mode, headset and speaker on,****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
		mixer_ctl_set_enum_by_string(client->mixer_ctls->audio_spk_headset_switch, "spk_headset");
	} else if(earpiece_on && (no_earpiece!=1)) {
		mixer_ctl_set_enum_by_string(client->mixer_ctls->audio_spk_headset_switch, "earpiece");
		ALOGV("in normal mode, earpiece on,****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
	} else if(headset_on | headphone_on) {
		mixer_ctl_set_enum_by_string(client->mixer_ctls->audio_spk_headset_switch, "headset");
		ALOGV("in normal mode, headset on,****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
	} else {
		ALOGV("in normal mode, speaker on,****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
		mixer_ctl_set_enum_by_string(client->mixer_ctls->audio_spk_headset_switch, "spk");
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


static int set_fm_volume(struct codec_client *client, int path, int volume)
{
	int speaker_on=0,headset_on=0 ,headphone_on=0;
	int level;

	headset_on = path & AUDIO_DEVICE_OUT_WIRED_HEADSET;  // hp4p
	headphone_on = path & AUDIO_DEVICE_OUT_WIRED_HEADPHONE; // hp3p 
	speaker_on = path & AUDIO_DEVICE_OUT_SPEAKER;

	if (volume >= 10) {
		level = 5;
	} else if (volume >= 8){
		level = 4;
	} else if (volume >= 6){
		level = 3;
	} else if (volume >= 4){
		level = 2;
	} else if (volume >= 2){
		level = 1;
	} else {
		level = 0;
	}

	if (speaker_on){
		mixer_ctl_set_value(client->mixer_ctls->linein_g_boost_stage_output_mixer_control, 0, client->vol_array->fm_speaker_line_gain[level]);
		//mixer_ctl_set_value(client->mixer_ctls->lineout_volume_control, 0, client->vol_array->fm_speaker_spk_gain[level]);
		set_headphone_volume(client->mixer_ctls->lineout_volume_control, client->vol_array->fm_speaker_spk_gain[level]);
	} else {
		mixer_ctl_set_value(client->mixer_ctls->linein_g_boost_stage_output_mixer_control, 0,  client->vol_array->fm_headset_line_gain[level]);
		//mixer_ctl_set_value(client->mixer_ctls->master_playback_volume, 0, client->vol_array->fm_headset_hp_gain[level]);
		set_headphone_volume(client->mixer_ctls->master_playback_volume, client->vol_array->fm_headset_hp_gain[level]);
	}

	ALOGV("4 set fm , adev_set_voice_volume, volume: %d, level=%d", volume, level);
	return 0;
}

static int fm_last_dev=0; // 1 = speaker, 2= headset;
static int set_fm_path(struct codec_client *client, int path)
{
	int headset_on=0, headphone_on=0, speaker_on=0;

	headset_on = path & AUDIO_DEVICE_OUT_WIRED_HEADSET;  // hp4p
	headphone_on = path & AUDIO_DEVICE_OUT_WIRED_HEADPHONE; // hp3p 
	speaker_on = path & AUDIO_DEVICE_OUT_SPEAKER;


	ALOGV("FM mode, devices is:%d, ****LINE:%d,FUNC:%s", path ,__LINE__,__FUNCTION__);

	mixer_ctl_set_value(client->mixer_ctls->audio_phone_end_call, 0, 1);
	mixer_ctl_set_value(client->mixer_ctls->audio_linein_in, 0, 1);  

	//speaker and headset all turn on , direct return. Don't do with alarm and ringtone in fm mode;
	if( speaker_on && (headphone_on || headset_on) ){ 
		ALOGV("FM mode Don't do with alarm and ringtone in fm mode, ****LINE:%d,FUNC:%s", __LINE__,__FUNCTION__);
		if (fm_last_dev!=0){
			speaker_on = fm_last_dev == 1 ? 1 : 0;
		} else {
			return 0;
		}
	}

	if (speaker_on){
		mixer_ctl_set_value(client->mixer_ctls->audio_headphone_out, 0, 0);
		mixer_ctl_set_value(client->mixer_ctls->audio_speaker_out, 0, 1);
		fm_last_dev=1;
	} else {
		mixer_ctl_set_value(client->mixer_ctls->audio_speaker_out, 0, 0);
		mixer_ctl_set_value(client->mixer_ctls->audio_headphone_out, 0, 1);
		fm_last_dev=2;
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
#if 0
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
#endif	
	ALOGV("FM record mode 4, ****LINE:%d,FUNC:%s", __LINE__,__FUNCTION__);

	return 0;
}



static int set_factory_volume(struct codec_client *client, int path, int vol)
{
	ALOGV("****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
	return 0;
}

static int set_factory_path(struct codec_client *client, int path)
{
	mixer_ctl_set_value(client->mixer_ctls->audio_phone_end_call, 0, 1);

	mixer_ctl_set_value(client->mixer_ctls->audio_phone_out, 0,1);
	mixer_ctl_set_value(client->mixer_ctls->audio_phone_in, 0,1);

	if(no_earpiece == 0){
			mixer_ctl_set_value(client->mixer_ctls->audio_speaker_out, 0, 0);
			mixer_ctl_set_value(client->mixer_ctls->audio_headphone_out, 0, 0);
			mixer_ctl_set_value(client->mixer_ctls->audio_earpiece_out, 0, 1);

			mixer_ctl_set_value(client->mixer_ctls->audio_phone_headset_mic, 0, 0);
			mixer_ctl_set_value(client->mixer_ctls->audio_phone_main_mic, 0, 1);

			ALOGV("in factory earpiece ****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
	} else if(no_earpiece == 1) {
			mixer_ctl_set_value(client->mixer_ctls->audio_earpiece_out, 0, 0);
			mixer_ctl_set_value(client->mixer_ctls->audio_headphone_out, 0, 0);
			mixer_ctl_set_value(client->mixer_ctls->audio_speaker_out, 0, 1);

			mixer_ctl_set_value(client->mixer_ctls->audio_phone_headset_mic, 0, 0);
			mixer_ctl_set_value(client->mixer_ctls->audio_phone_main_mic, 0, 1);

			ALOGV("in factory speaker pa ****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
	}

	mixer_ctl_set_value(client->mixer_ctls->phonep_phonen_pre_amp_gain_control, 0, 7);
	mixer_ctl_set_value(client->mixer_ctls->phone_g_boost_stage_output_mixer_control, 0, 7);
	//mixer_ctl_set_value(client->mixer_ctls->master_playback_volume, 0, 60);
	//mixer_ctl_set_value(client->mixer_ctls->lineout_volume_control, 0, 30);
	set_headphone_volume(client->mixer_ctls->master_playback_volume, 60);
	ALOGV("****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
	return 0;
}



static int set_phone_volume(struct codec_client *client, int path, int volume)
{
	int level;
	int speaker_on=0,headset_on=0 ,headphone_on=0,earpiece_on=0;
	int speaker_vol=0, headset_vol=0, earpiece_vol=0;

	speaker_on = path & AUDIO_DEVICE_OUT_SPEAKER;
	headset_on = path & AUDIO_DEVICE_OUT_WIRED_HEADSET;  // with mic
	headphone_on = path & AUDIO_DEVICE_OUT_WIRED_HEADPHONE; // no mic 
	earpiece_on = path & AUDIO_DEVICE_OUT_EARPIECE;

	if (volume >= 10) {
		level = 5;
	} else if (volume >= 8){
		level = 4;
	} else if (volume >= 6){
		level = 3;
	} else if (volume >= 4){
		level = 2;
	} else if (volume >= 2){
		level = 1;
	} else {
		level = 0;
	}

	ALOGD("adev_set_voice_volume, speaker_on: %d, earpiece_on: %d, headset_on: %d", speaker_on ,earpiece_on, headset_on);
	ALOGD("adev_set_voice_volume, volume:%d, level: %d", volume, level);

	if (speaker_on  || (earpiece_on  && (no_earpiece == 1))){
		mixer_ctl_set_value(client->mixer_ctls->phonep_phonen_pre_amp_gain_control, 0, client->vol_array->speaker_phonepn_gain[level]);
		mixer_ctl_set_value(client->mixer_ctls->phone_g_boost_stage_output_mixer_control, 0, client->vol_array->speaker_mixer_gain[level]);
		//mixer_ctl_set_value(client->mixer_ctls->lineout_volume_control, 0, client->vol_array->speaker_spk_gain[level]);
		set_headphone_volume(client->mixer_ctls->lineout_volume_control,client->vol_array->speaker_spk_gain[level]);//30);
	} else if (earpiece_on && (no_earpiece == 0)){
		mixer_ctl_set_value(client->mixer_ctls->phonep_phonen_pre_amp_gain_control, 0, client->vol_array->earpiece_phonepn_gain[level]);
		mixer_ctl_set_value(client->mixer_ctls->phone_g_boost_stage_output_mixer_control, 0, client->vol_array->earpiece_mixer_gain[level]);
		//mixer_ctl_set_value(client->mixer_ctls->master_playback_volume, 0, client->vol_array->earpiece_hp_gain[level]);
		set_headphone_volume(client->mixer_ctls->master_playback_volume, client->vol_array->earpiece_hp_gain[level]);
	} else if (headset_on || headphone_on){
		mixer_ctl_set_value(client->mixer_ctls->phonep_phonen_pre_amp_gain_control, 0, client->vol_array->headset_phonepn_gain[level]);
		mixer_ctl_set_value(client->mixer_ctls->phone_g_boost_stage_output_mixer_control, 0, client->vol_array->headset_mixer_gain[level]);
		//mixer_ctl_set_value(client->mixer_ctls->master_playback_volume, 0, client->vol_array->headset_hp_gain[level]);
		set_headphone_volume(client->mixer_ctls->master_playback_volume,client->vol_array->headset_hp_gain[level]);//30);
	}

	return 0;
}


static bool is_in_record = false;
static bool bluetooth_in_record = false;

static int set_phone_path(struct codec_client *client, int path)
{
    int ret = -1;
    int earpiece_on=0, headset_on=0, headphone_on=0, bt_on=0, speaker_on=0;
    int pa_should_on=0;

    headset_on = path & AUDIO_DEVICE_OUT_WIRED_HEADSET;  // hp4p
    headphone_on = path & AUDIO_DEVICE_OUT_WIRED_HEADPHONE; // hp3p 
    speaker_on = path & AUDIO_DEVICE_OUT_SPEAKER;
    earpiece_on = path & AUDIO_DEVICE_OUT_EARPIECE;
    bt_on = path & AUDIO_DEVICE_OUT_ALL_SCO;

	ALOGV("mode= phone mode 4, devices is 0x%x, ****LINE:%d,FUNC:%s", path, __LINE__,__FUNCTION__);

	in_call = true;

	if(last_path_is_bt){
		plan_one_stop_bt_voice();

		if (bluetooth_in_record == true){
			plan_one_stop_bt_record();
			bluetooth_in_record = false;
		}

		last_path_is_bt = false;

		if(mixer_ctl_get_value(client->mixer_ctls->audio_adc_phone_in, 0) == 1 ){
			mixer_ctl_set_value(client->mixer_ctls->audio_adc_phone_in, 0, 0);
			mixer_ctl_set_value(client->mixer_ctls->audio_dac_phone_out, 0, 0);
		}
	}

	if (is_in_record){
		client->record_ops->set_record_source(!bt_on);			
	}


	if(mixer_ctl_get_value(client->mixer_ctls->audio_linein_in,0)){
		mixer_ctl_set_value(client->mixer_ctls->audio_linein_in, 0, 0); //turn off fm
	}


	if (bt_on){
		mixer_ctl_set_value(client->mixer_ctls->audio_phone_in, 0,0);
	} else {
		mixer_ctl_set_value(client->mixer_ctls->audio_phone_in, 0,1);
	}

#if 1
	mixer_ctl_set_value(client->mixer_ctls->audio_phone_out, 0,1);

		if(earpiece_on && (no_earpiece == 0)){
			mixer_ctl_set_value(client->mixer_ctls->audio_speaker_out, 0, 0);
			mixer_ctl_set_value(client->mixer_ctls->audio_headphone_out, 0, 0);
			mixer_ctl_set_value(client->mixer_ctls->audio_earpiece_out, 0, 1);

			mixer_ctl_set_value(client->mixer_ctls->audio_phone_headset_mic, 0, 0);
			mixer_ctl_set_value(client->mixer_ctls->audio_phone_main_mic, 0, 1);

			ALOGV("in earpiece ****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
		} else if(speaker_on || (earpiece_on  && (no_earpiece == 1))) {

			mixer_ctl_set_value(client->mixer_ctls->audio_earpiece_out, 0, 0);
			mixer_ctl_set_value(client->mixer_ctls->audio_headphone_out, 0, 0);
			mixer_ctl_set_value(client->mixer_ctls->audio_speaker_out, 0, 1);

			mixer_ctl_set_value(client->mixer_ctls->audio_phone_headset_mic, 0, 0);
			mixer_ctl_set_value(client->mixer_ctls->audio_phone_main_mic, 0, 1);

			ALOGV("in speaker pa ****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
		} else if (headphone_on) { //no mic hp3p 

			mixer_ctl_set_value(client->mixer_ctls->audio_earpiece_out, 0, 0);
			mixer_ctl_set_value(client->mixer_ctls->audio_speaker_out, 0, 0);
			mixer_ctl_set_value(client->mixer_ctls->audio_headphone_out, 0, 1);

			mixer_ctl_set_value(client->mixer_ctls->audio_phone_headset_mic, 0, 0);
			mixer_ctl_set_value(client->mixer_ctls->audio_phone_main_mic, 0, 1);

			ALOGV("in wire head with no mic ****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
		} else if (headset_on) {  //mic // hp4p

			mixer_ctl_set_value(client->mixer_ctls->audio_earpiece_out, 0, 0);
			mixer_ctl_set_value(client->mixer_ctls->audio_speaker_out, 0, 0);
			mixer_ctl_set_value(client->mixer_ctls->audio_headphone_out, 0, 1);

			mixer_ctl_set_value(client->mixer_ctls->audio_phone_main_mic, 0, 0);
			mixer_ctl_set_value(client->mixer_ctls->audio_phone_headset_mic, 0, 1);

			ALOGV("in wire head with mic ****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
		} else if (bt_on) {
			if(!wake_lock){
				grabPartialWakeLock();
				wake_lock=true;
			}

			mixer_ctl_set_value(client->mixer_ctls->audio_earpiece_out, 0, 0);
			mixer_ctl_set_value(client->mixer_ctls->audio_headphone_out, 0, 0);
			mixer_ctl_set_value(client->mixer_ctls->audio_speaker_out, 0, 0);

			mixer_ctl_set_value(client->mixer_ctls->audio_phone_main_mic, 0, 0);
			mixer_ctl_set_value(client->mixer_ctls->audio_phone_headset_mic, 0, 0);

			ALOGV("in bluetooth ****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
			#if 0 // for a23
			mixer_ctl_set_value(client->mixer_ctls->audio_adc_phone_in, 0, 1);
			mixer_ctl_set_value(client->mixer_ctls->audio_dac_phone_out, 0, 1);
			plan_one_start_bt_voice(client->vol_array->up_pcm_gain);
			last_path_is_bt = true;

			if (is_in_record && (bluetooth_in_record == false)){
				plan_one_start_bt_record();	
				bluetooth_in_record = true;	
			}
			#endif
		}
#endif
	return 0;
}


static int set_phone_record_enable(struct codec_client *client, bool enable)
{
	mixer_ctl_set_value(client->mixer_ctls->audio_fm_record, 0, 0);

	if (enable){
		mixer_ctl_set_value(client->mixer_ctls->audio_phone_voice_record, 0, 1);
		is_in_record = true;
	} else {
		if (is_in_record == true){
			mixer_ctl_set_value(client->mixer_ctls->audio_phone_voice_record, 0, 0);
			if( bluetooth_in_record ){
           	             bluetooth_in_record = false;
				plan_one_stop_bt_record();
			}
			client->record_ops->stop_record();
			is_in_record = false;
             	   	bluetooth_in_record = false;
		}
	}
	ALOGV("phone record mode 4,****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
	return 0;
}

static int set_phone_record(struct codec_client *client, int path)
{
    int ret = -1;
    int bt_on=0;

    bt_on = path & AUDIO_DEVICE_OUT_ALL_SCO;

	if(bt_on){
		bluetooth_in_record = true;
		plan_one_start_bt_record();
	} else {
		if( bluetooth_in_record ){
                        bluetooth_in_record = false;
			plan_one_stop_bt_record();	
		}
	}

	client->record_ops->set_record_source(!bt_on);
	if(client->record_ops->start_record(4096) < 0){ // update later !!!!
		ALOGE("common start record failed, ****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
		return -1;
	}

	ALOGV("****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
	return 0;
}

static int record_read_pcm_buf(struct codec_client *client, void* buffer, int bytes)
{
	ALOGV("1****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
	
	return plan_one_mixer_buf(buffer, bytes);
}

int plan_one_init(void)
{
	int ret = -1;
	char prop_value[20];

	ret = property_get("audio.without.earpiece", prop_value, "0");
	if (ret > 0)
	{		
		if (atoi(prop_value) == 1)
		{
			no_earpiece = 1;
			ALOGD("get property audio.without.earpiece: %d", no_earpiece);
		}
	}

	ret = plan_one_init_voice();

	ALOGV("****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);

	return ret;
}

void plan_one_exit(void)
{
	plan_one_exit_voice();
	ALOGV("****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
}




struct normal_ops plan_one_normal_ops = {
    .set_normal_volume=set_normal_volume,
    .set_normal_path=set_normal_path,
    .set_normal_record_enable =set_normal_record_enable, 
    .set_normal_record=set_normal_record,
};

struct fm_ops plan_one_fm_ops = {
    .set_fm_volume=set_fm_volume,
    .set_fm_path=set_fm_path,
    .set_fm_record_enable =set_fm_record_enable, 
    .set_fm_record=set_fm_record,
};

struct factory_ops plan_one_factory_ops= {
    .set_factory_volume=set_factory_volume,
    .set_factory_path=set_factory_path,
};

struct phone_ops plan_one_phone_ops= {
    .set_phone_volume=set_phone_volume,
    .set_phone_path=set_phone_path,
    .set_phone_record_enable =set_phone_record_enable, 
    .set_phone_record=set_phone_record,
    .record_read_pcm_buf=record_read_pcm_buf,
};


