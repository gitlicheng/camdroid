
#define LOG_TAG "codec_audio_plan_v3"
#define LOG_NDEBUG 0

#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>

#include <cutils/properties.h>

#include <system/audio.h>

#include "hal_codec.h"
#include "plan_v3.h"


static bool g_is_bp_thread_running = false;
static int no_earpiece = 0;
static bool wake_lock = false;

static int set_normal_volume(struct codec_client *client, int path, int vol)
{
	int headset_on=0, headphone_on=0, speaker_on=0;

	headset_on = path & AUDIO_DEVICE_OUT_WIRED_HEADSET;  // hp4p
	headphone_on = path & AUDIO_DEVICE_OUT_WIRED_HEADPHONE; // hp3p 
	speaker_on = path & AUDIO_DEVICE_OUT_SPEAKER;

	if (speaker_on){
		ALOGV("****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
		mixer_ctl_set_value(client->mixer_ctls->line_volume, 0, vol);
	} else if ((headset_on || headphone_on)){
		ALOGV("****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
		mixer_ctl_set_value(client->mixer_ctls->master_playback_volume, 0, vol);
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

	if(wake_lock){
		releaseWakeLock();
		wake_lock=false;
	}

	if ((headset_on || headphone_on) && speaker_on){
		ALOGV("in normal mode, headset and speaker on,****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
		mixer_ctl_set_enum_by_string(client->mixer_ctls->audio_spk_headset_switch, "headset-spk");
//    } else if(earpiece_on && (no_earpiece!=1)) {
//           mixer_ctl_set_enum_by_string(client->mixer_ctls->audio_spk_headset_switch, "earpiece");// "spk" : "headset");
//           ALOGV("in normal mode, earpiece on,****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
	} else if(headset_on | headphone_on) {
		mixer_ctl_set_enum_by_string(client->mixer_ctls->audio_spk_headset_switch, "headset");
		ALOGV("in normal mode, headset on,****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
	} else {
		ALOGV("in normal mode, speaker on,****LINE:%d,FUNC:%s, speaker_on=%d", __LINE__,__FUNCTION__, speaker_on);
		mixer_ctl_set_enum_by_string(client->mixer_ctls->audio_spk_headset_switch, "spk" );
	}

	return 0;
}

static int set_normal_record_enable(struct codec_client *client, bool enable)
{
	ALOGV("normal record mode 4,****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
	return 0;
}

static int set_normal_record(struct codec_client *client, int path)
{
	ALOGV("normal record mode 4,****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
	mixer_ctl_set_enum_by_string(client->mixer_ctls->audio_record_source, "mic1");
	return 0;
}

int plan_v3_init(void)
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

	//ret = plan_v3_init_voice();

	ALOGV("****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);

	//return ret;
	return 0;
}

void plan_v3_exit(void)
{
	//plan_v3_exit_voice();
	ALOGV("****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
}


struct normal_ops plan_v3_normal_ops = {
    .set_normal_volume=set_normal_volume,
    .set_normal_path=set_normal_path,
    .set_normal_record_enable =set_normal_record_enable, 
    .set_normal_record=set_normal_record,
};


