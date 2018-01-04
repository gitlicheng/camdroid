
#ifndef __HAL_CODEC_H__
#define __HAL_CODEC_H__

#include <stdbool.h>
#include "codec_utils.h"

struct codec_client {
    struct mixer_ctls *mixer_ctls;
    struct normal_ops *normal_ops;
    struct fm_ops *fm_ops;
    struct factory_ops *factory_ops;
    struct phone_ops *phone_ops;
    struct phone_common_record_ops *record_ops;
    struct volume_array *vol_array;
};


struct normal_ops {
    int (*set_normal_volume)(struct codec_client *client, int path, int vol);
    int (*set_normal_path)(struct codec_client *client, int path);
    int (*set_normal_record_enable)(struct codec_client *client, bool enable);
    int (*set_normal_record)(struct codec_client *client, int path);
};

struct fm_ops {
    int (*set_fm_volume)(struct codec_client *client, int path, int vol);
    int (*set_fm_path)(struct codec_client *client, int path);
    int (*set_fm_record_enable)(struct codec_client *client, bool enable);
    int (*set_fm_record)(struct codec_client *client, int path);
    int (*record_read_pcm_buf)(struct codec_client *client, void* buffer, int bytes);
};

struct factory_ops {
    int (*set_factory_volume)(struct codec_client *client, int path, int vol);
    int (*set_factory_path)(struct codec_client *client, int path);
};

struct phone_ops {
    int (*set_phone_volume)(struct codec_client *client, int path, int vol);
    int (*set_phone_path)(struct codec_client *client, int path);
    int (*set_phone_record_enable)(struct codec_client *client, bool enable);
    int (*set_phone_record)(struct codec_client *client, int path);
    int (*record_read_pcm_buf)(struct codec_client *client, void* buffer, int bytes);
};

struct phone_common_record_ops {
   int (*set_record_source)(bool is_record_from_adc);
   int (*start_record)(int record_buf_size);
   void (*stop_record)(void);
   int (*init_record)(struct codec_client *client);
   void (*exit_record)(void);
};


struct other_ops {
    int (*other_op)(char *name);
};

struct volume_array{
	int earpiece_phonepn_gain[6];
	int earpiece_mixer_gain[6];
	int earpiece_hp_gain[6];

	int headset_phonepn_gain[6];
	int headset_mixer_gain[6];
	int headset_hp_gain[6];

	int speaker_phonepn_gain[6];
	int speaker_mixer_gain[6];
	int speaker_spk_gain[6];

	int fm_headset_line_gain[6];
	int fm_headset_hp_gain[6];

	int fm_speaker_line_gain[6];
	int fm_speaker_spk_gain[6];

	int up_pcm_gain;
//	int fm_record_gain;
};



#endif


