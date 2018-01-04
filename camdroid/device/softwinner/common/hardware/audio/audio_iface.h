
#ifndef __AUDIO_IFACE_H__
#define __AUDIO_IFACE_H__

//===================================codec===========

struct codec_client {
    struct mixer_ctls *mixer_ctls;
    struct normal_ops *normal_ops;
    struct fm_ops *fm_ops;
    struct factory_ops *factory_ops;
    struct phone_ops *phone_ops;
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

struct other_ops {
    int (*other_op)(char *name);
};


//===================================ril===========
typedef enum {
	RIL_AUDIO_PATH_EARPIECE = 0 ,
	RIL_AUDIO_PATH_HEADSET ,
	RIL_AUDIO_PATH_SPK,
	RIL_AUDIO_PATH_BT,
	RIL_AUDIO_PATH_MAIM_MIC,
	RIL_AUDIO_PATH_HEADSET_MIC,
	RIL_AUDIO_PATH_EARPIECE_LOOP,
	RIL_AUDIO_PATH_HEADSET_LOOP,
	RIL_AUDIO_PATH_SPK_LOOP,

	RIL_AUDIO_PATH_CNT,
	RIL_AUDIO_PATH_MAX =RIL_AUDIO_PATH_CNT-1,
}ril_audio_path_type_t;


struct bp_ops {
    int (*get_tty_dev)(char *name);
    int (*set_call_volume)(ril_audio_path_type_t path, int vol);
    int (*set_call_path)(ril_audio_path_type_t path);
    int (*set_call_at)(char *at);
};

struct bp_client {
    struct bp_ops *bp_ops;
};

struct bp_client* bp_client_new();
void bp_client_free(struct bp_client *client);
int ril_dev_init();
void ril_dev_exit();
void ril_set_call_volume(ril_audio_path_type_t path, int volume);
void ril_set_call_audio_path(ril_audio_path_type_t path);
void ril_set_call_at(char *at);


struct codec_client* codec_client_new();
void codec_client_free(struct codec_client *client);
int codec_dev_init();
void codec_dev_exit();

void normal_play_route(int path);
void fm_play_route(int path);
void phone_play_route(int path);

void normal_play_volume(int path, int vol);
void fm_volume(int path,int volume);
void phone_volume(int path, int volume);

void factory_route(int path);

void normal_record_enable(bool enable);
void normal_record_route(int path);
void fm_record_enable(bool enable);
void fm_record_route(int path);
void phone_record_enable(bool enable);
void phone_record_route(int path);

int phone_record_read_pcm_buf(void* buffer, int bytes);


#endif



