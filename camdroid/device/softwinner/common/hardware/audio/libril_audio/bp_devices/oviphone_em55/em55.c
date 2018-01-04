
#define LOG_TAG "audio_ril_em55"
#define LOG_NDEBUG 0

#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>

#include <cutils/properties.h>

#include "hal_bp.h"

#define AT_PATH "/dev/mux2"

static char tty_dev[32] = AT_PATH;

static int em55_get_tty_dev(char *name)
{
   ALOGD("em55_get_tty_dev\n");
   return 0;
}

static int em55_set_call_volume(ril_audio_path_type_t path, int vol)
{
   ALOGD("em55_set_call_volume\n");
   return 0;
}

static int em55_set_call_path(ril_audio_path_type_t path)
{
	int channel = 0;
	char cmdline[50]={0};
	//char tty_dev[32]={0};

#if 1
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
            case RIL_AUDIO_PATH_MAIM_MIC:
            case RIL_AUDIO_PATH_HEADSET_MIC:
            case RIL_AUDIO_PATH_EARPIECE_LOOP:
            case RIL_AUDIO_PATH_HEADSET_LOOP:
            case RIL_AUDIO_PATH_SPK_LOOP:
		channel = 2;
		exec_at(tty_dev,"AT+BTON=0");
		break;
            case RIL_AUDIO_PATH_BT:
		exec_at(tty_dev,"AT+BTON=1");
		break;
#endif
            default:
                channel = 0;
                break;
        }
#endif
 
   ALOGD("em55_set_call_path\n");
   return 0;
}

static int em55_set_call_at(char *at)
{
    exec_at(tty_dev,at);

   ALOGD("em55_set_call_at\n");
   return 0;
}

struct bp_ops em55_ops = {
    .get_tty_dev = em55_get_tty_dev,
    .set_call_volume= em55_set_call_volume,
    .set_call_path = em55_set_call_path,
    .set_call_at = em55_set_call_at,
};



