
#ifndef __BP_UTILS_H__
#define __BP_UTILS_H__

int exec_at(char *tty_dev, char *at);

typedef enum {
	RIL_AUDIO_PATH_EARPIECE = 0,//听筒
	RIL_AUDIO_PATH_HEADSET,//耳机
	RIL_AUDIO_PATH_SPK,//免提
	RIL_AUDIO_PATH_BT,//蓝牙
	RIL_AUDIO_PATH_MAIM_MIC,//主mic
	RIL_AUDIO_PATH_HEADSET_MIC,//耳机mic
	RIL_AUDIO_PATH_EARPIECE_LOOP,//听筒回环
	RIL_AUDIO_PATH_HEADSET_LOOP,//耳机回环
	RIL_AUDIO_PATH_SPK_LOOP,//喇叭回环

	RIL_AUDIO_PATH_CNT,
	RIL_AUDIO_PATH_MAX =RIL_AUDIO_PATH_CNT-1,
}ril_audio_path_type_t;


#endif



