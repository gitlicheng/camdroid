
#define LOG_TAG "audio_ril"
#define LOG_NDEBUG 0

#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>

#include <cutils/properties.h>

#include "hal_bp.h"

static int mu509_get_tty_dev(char *name)
{

    char temp_name[32];
    int no = -1;
    int i = 0;
    int ret = 0;

    if(NULL == name){
        return -1;
    }

    for(i = 0; i < 20; i++){
    	memset(temp_name, 0, 32);
    	sprintf(temp_name, "/dev/ttyUSB%d", i);

		ret = access(temp_name, F_OK);
		if(ret != 0){
			continue;
		}

		no = i;
    }

	if(no >= 0){
		sprintf(temp_name, "/dev/ttyUSB%d", no);
		strcpy(name, temp_name);
		ALOGV("get_current_tty_dev_mu509: tty_dev=%s\n", name);
		return 0;
	}else{
		ALOGV("get_current_tty_dev_mu509: can not find the AT device\n");
		return -1;
	}
	return 0;
}

static int mu509_set_call_volume(ril_audio_path_type_t path, int volume)
{
	char tty_dev[32]={0};
	char cmdline[30];

	int level;

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
		level = 1;
	}

	mu509_get_tty_dev(tty_dev);

	sprintf(cmdline, "AT+CLVL=%d", level);

	exec_at(tty_dev,cmdline);

	return 0;
}

static int mu509_set_call_path(ril_audio_path_type_t path)
{
	int channel = 0;
       char cmdline[50]={0};
	char tty_dev[32]={0};

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

    mu509_get_tty_dev(tty_dev);

    ALOGD("mu509: channel=%d, at=%s\n", channel, tty_dev);

    sprintf(cmdline, "AT^SWSPATH=%d", channel);
    exec_at(tty_dev,cmdline);

    return 0;
}

static int mu509_set_call_at(char *at)
{
	char tty_dev[32]={0};
    mu509_get_tty_dev(tty_dev);
    exec_at(tty_dev,at);

    ALOGD("mu509_set_call_at");

	return 0;
}

struct bp_ops mu509_ops = {
    .get_tty_dev = mu509_get_tty_dev,
    .set_call_volume= mu509_set_call_volume,
    .set_call_path = mu509_set_call_path,
    .set_call_at = mu509_set_call_at,
};

