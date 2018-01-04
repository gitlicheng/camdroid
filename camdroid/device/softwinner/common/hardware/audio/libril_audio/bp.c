
#define LOG_TAG "bp_audio"
#define LOG_NDEBUG 0

#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>
#include <cutils/properties.h>

#include "bp_devices.h"
#include "hal_bp.h"

static int bp_device_detect(void)
{
    int index = -1, i=-1, ret = 0;
    char device[20]={0};

    ret = property_get("ro.sw.audio.bp_device_name", device, "0");
    if(ret <= 0){
        ALOGE("wrn: get ro.sw.audio.bp_device_name failed");
        return -1;
    }

    ALOGD("get ro.sw.audio.bp_device_name=%s", device);

    for (i=0 ; i < bp_devices_count ; i++)
    {
        if (strstr(device, bp_devices[i].name) != NULL)
        {
            index = i;
            break;
        }
    }
    if (index == -1){
	    index = 0 ; //default demo
    }

    ALOGD("bp_device_detect index=%d", index);


    return index;
}

struct bp_client* bp_client_new()
{
    struct bp_client *client;
    int device_index = -1;

    device_index = bp_device_detect();

    if (device_index < 0 || device_index > bp_devices_count)
        return NULL;

    client = (struct bp_client*) malloc(sizeof(struct bp_client));
    memset(client, 0, sizeof(struct bp_client));

    if (bp_devices[device_index].ops)
        client->bp_ops = bp_devices[device_index].ops;

    return client;
}

void bp_client_free(struct bp_client *client)
{
    free(client);
    //client = NULL;
}




