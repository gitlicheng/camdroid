

#define LOG_TAG "codec_audio"
#define LOG_NDEBUG 0

#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>
#include <cutils/properties.h>

#include "codec_devices.h"
#include "hal_codec.h"
#include "record.h"

static struct mixer_ctls mixer_ctls;

extern struct phone_common_record_ops phone_common_record_ops;

static int codec_device_detect(void)
{
    int index = -1, i=-1, ret = 0;
    char device[20]={0};

    ret = property_get("ro.sw.audio.codec_plan_name", device, "0");
    if(ret <= 0){
        ALOGE("wrn: get ro.sw.audio.codec_plan_name failed");
        return -1;
    }

    ALOGD("get ro.sw.audio.codec_plan_name =%s", device);

    for (i=0 ; i < codec_devices_count ; i++)
    {
        if (strstr(device, codec_devices[i].plan_name) != NULL)
        {
            index = i;
            break;
        }
    }

    if (index == -1){
	    index = 0 ; //default v3
    }

    ALOGD("get index =%d", index);

    return index;
}

static struct volume_array vol_array;

extern int get_volume_config(struct volume_array *vol_array);

struct codec_client* codec_client_new()
{
    struct codec_client *client=NULL;
    int device_index = -1;
    int ret = -1; 

    device_index = codec_device_detect();

    if (device_index < 0 || device_index > codec_devices_count)
        return NULL;

    client = (struct codec_client*) malloc(sizeof(struct codec_client));
    if (client == NULL){
		return NULL;	
    }
    memset(client, 0, sizeof(struct codec_client));

    if (codec_devices[device_index].device_init != NULL){
	ret = codec_devices[device_index].device_init();	
	if (ret != 0){
		ALOGE("%s device_init failed\n", codec_devices[device_index].name);
		free(client);
		return NULL;
	}
    }

    if(codec_devices[device_index].normal_ops)
	client->normal_ops = codec_devices[device_index].normal_ops;


    if(codec_devices[device_index].fm_ops)
       client->fm_ops = codec_devices[device_index].fm_ops;


    if(codec_devices[device_index].factory_ops)
	client->factory_ops = codec_devices[device_index].factory_ops;


    if(codec_devices[device_index].phone_ops)
      client->phone_ops = codec_devices[device_index].phone_ops;

    client->record_ops = &phone_common_record_ops;


    if(get_mixer(&mixer_ctls) < 0){
		ALOGE("get_mixer failed");
		free(client);
		return NULL;
    }

    client->mixer_ctls = &mixer_ctls;

    if(client->record_ops->init_record(client) < 0){
		ALOGE("phone_common_record_ops init_record failed");
		free(client);
		return NULL;
    }

    if(get_volume_config(&vol_array)<0){
		ALOGE("get_volume_config failed");
		free(client);
		return NULL;
    }

    client->vol_array = &vol_array;

	ALOGD("get_volume_config pcm_vol: %d", client->vol_array->up_pcm_gain);

    return client;
}

void codec_client_free(struct codec_client *client)
{
    client->record_ops->exit_record();
    free(client);
    //client = NULL;
}





