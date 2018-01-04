
#define LOG_TAG "audio_ril_usi6276"
#define LOG_NDEBUG 0

#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>

#include <cutils/properties.h>

#include "hal_bp.h"

#define AT_PATH "/dev/ttyS3"


//获得bp设备节点.
static int usi6276_get_tty_dev(char *name)
{
   strncpy(name, AT_PATH, strlen(AT_PATH));
   ALOGD("usi6276_get_tty_dev\n");
   return 0;
}


//声音数组.分６级,顺序依次从第１级到第六级
static int earpiece_vol[]={1,2,3,4,5,6};
static int headset_vol[]={1,2,3,4,5,6};
static int spk_vol[]={1,2,3,4,5,6};
static int bt_vol[]={1,2,3,4,5,6};
static int main_mic_vol[]={1,2,3,4,5,6};
static int headset_mic_vol[]={1,2,3,4,5,6};

//不同路径下,声音设置调用
static int usi6276_set_call_volume(ril_audio_path_type_t path, int volume)
{
	char tty_dev[32]={0};
	char cmdline[30];
	int level, bp_vol;


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

#if 0
      switch(path) {
            case RIL_AUDIO_PATH_EARPIECE:
		sprintf(cmdline, "AT+CLVL=%d",  earpiece_vol[level]);
            	break;
            case RIL_AUDIO_PATH_HEADSET:
		sprintf(cmdline, "AT+CLVL=%d",  headset_vol[level]);
	        break;
            case RIL_AUDIO_PATH_SPK:
		sprintf(cmdline, "AT+CLVL=%d",  spk_vol[level]);
                break;
            case RIL_AUDIO_PATH_BT:
		sprintf(cmdline, "AT+CLVL=%d",  bt_vol[level]);
                break;
            case RIL_AUDIO_PATH_MAIM_MIC:
		sprintf(cmdline, "AT+CLVL=%d",  main_mic_vol[level]);
                break;
            case RIL_AUDIO_PATH_HEADSET_MIC:
		sprintf(cmdline, "AT+CLVL=%d",  headset_mic_vol[level]);
                break;

	}
#endif

	usi6276_get_tty_dev(tty_dev);
	//exec_at(tty_dev,cmdline);

   	ALOGD("usi6276_set_call_volume\n");

   return 0;
}

//路径切换调用
static int usi6276_set_call_path(ril_audio_path_type_t path)
{
	int channel = 0;
	char cmdline[50]={0};
	char tty_dev[32]={0};

#if 0
      switch(path) {
#if 0
            case RIL_AUDIO_PATH_EARPIECE:
                channel = 0;
            	break;
            case RIL_AUDIO_PATH_HEADSET:
                channel = 0;
	        break;
            case RIL_AUDIO_PATH_SPK:
                channel = 0;
                break;
            case RIL_AUDIO_PATH_BT:
                channel = 2;
                break;
            case RIL_AUDIO_PATH_MAIM_MIC:
                channel = 0;
                break;
            case RIL_AUDIO_PATH_HEADSET_MIC:
                channel = 0;
                break;
            case RIL_AUDIO_PATH_EARPIECE_LOOP:
                channel = 0;
                break;
            case RIL_AUDIO_PATH_HEADSET_LOOP:
                channel = 0;
                break;
            case RIL_AUDIO_PATH_SPK_LOOP:
                channel = 0;
                break;
#else
            case RIL_AUDIO_PATH_EARPIECE:
            case RIL_AUDIO_PATH_HEADSET:
            case RIL_AUDIO_PATH_SPK:
            case RIL_AUDIO_PATH_BT:
            case RIL_AUDIO_PATH_MAIM_MIC:
            case RIL_AUDIO_PATH_HEADSET_MIC:
            case RIL_AUDIO_PATH_EARPIECE_LOOP:
            case RIL_AUDIO_PATH_HEADSET_LOOP:
            case RIL_AUDIO_PATH_SPK_LOOP:
                channel = 2;
                break;

#endif
            default:
                channel = 0;
                break;
        }
#endif

    usi6276_get_tty_dev(tty_dev);
    //sprintf(cmdline, "AT^SWSPATH=%d", channel);
    //exec_at(tty_dev,cmdline);

   ALOGD("usi6276_set_call_path\n");

   return 0;
}

//调用任意at指令
static int usi6276_set_call_at(char *at)
{
    char tty_dev[32]={0};

    usi6276_get_tty_dev(tty_dev);
    exec_at(tty_dev,at);

   ALOGD("usi6276_set_call_at\n");
   return 0;
}

struct bp_ops usi6276_ops = {
    .get_tty_dev = usi6276_get_tty_dev,
    .set_call_volume= usi6276_set_call_volume,
    .set_call_path = usi6276_set_call_path,
    .set_call_at = usi6276_set_call_at,
};



