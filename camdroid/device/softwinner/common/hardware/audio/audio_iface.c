
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>
#include <stdlib.h>

#include <cutils/log.h>
#include <fcntl.h>

#include "audio_iface.h"


static struct codec_client* codec_client;

int codec_dev_init()
{
   codec_client= codec_client_new();

   if (codec_client == NULL){
	ALOGE("****LINE:%d,FUNC:%s",__LINE__,__FUNCTION__);
	return -1;
   }

  return 0;
}

void codec_dev_exit()
{
   if (codec_client != NULL)
	codec_client_free(codec_client);
   codec_client = NULL;
}

//normal play and record 
void normal_play_enable(bool enable)
{
  // init volume
  //disable other mode
}

void normal_play_route(int path)
{
   int ret = 0; 
   if ((codec_client != NULL) && (codec_client->normal_ops != NULL)){
	ret = codec_client->normal_ops->set_normal_path(codec_client,path);
   }
}

/*
hp3p,hp34p max 31
spk max 60
*/
void normal_play_volume(int path, int vol) 
{
   int ret = 0; 
   if ((codec_client != NULL) && (codec_client->normal_ops != NULL)){
	ret = codec_client->normal_ops->set_normal_volume(codec_client,path,vol);
   }
}

void normal_record_enable(bool enable)
{
   int ret = 0; 
   if ((codec_client != NULL) && (codec_client->normal_ops != NULL)){
	ret = codec_client->normal_ops->set_normal_record_enable(codec_client,enable);
   }
}

void normal_record_route(int path)
{
   int ret = 0; 
   if ((codec_client != NULL) && (codec_client->normal_ops != NULL)){
	ret = codec_client->normal_ops->set_normal_record(codec_client,path);
   }
}

//FM play and record
void fm_play_enable(bool enable){


}

void fm_play_route(int path)
{
   int ret = 0; 
   if ((codec_client != NULL) && (codec_client->fm_ops != NULL)){
	ret = codec_client->fm_ops->set_fm_path(codec_client,path);
   }
}


void fm_record_enable(bool enable)
{
   int ret = 0; 
   if ((codec_client != NULL) && (codec_client->fm_ops != NULL)){
	ret = codec_client->fm_ops->set_fm_record_enable(codec_client,enable);
   }
}

void fm_record_route(int path)
{
   int ret = 0; 
   if ((codec_client != NULL) && (codec_client->fm_ops != NULL)){
	ret = codec_client->fm_ops->set_fm_record(codec_client,path);
   }
}


void fm_volume(int path,int volume)
{
   if ((codec_client != NULL) && (codec_client->fm_ops != NULL)){
	codec_client->fm_ops->set_fm_volume(codec_client,path,volume);
   }
}

//Factory test
void factory_enable(bool enable){

}

void factory_route(int path){
   int ret = 0; 
   if ((codec_client != NULL) && (codec_client->factory_ops != NULL)){
	ret = codec_client->factory_ops->set_factory_path(codec_client,path);
   }
}


//Ringtone 
void ringtone_enable(bool enable){

}

void ringtone_path(int path){

}

void ringtone_volume(float volume){

}

//phone play and record
void phone_play_enable(bool enable){

}

void phone_play_route(int path)
{
   int ret = 0; 
   if ((codec_client != NULL) && (codec_client->phone_ops != NULL)){
	ret = codec_client->phone_ops->set_phone_path(codec_client,path);
   }
}

void phone_record_enable(bool enable)
{
   int ret = 0; 
   if ((codec_client != NULL) && (codec_client->phone_ops != NULL)){
	ret = codec_client->phone_ops->set_phone_record_enable(codec_client,enable);
   }
}

void phone_record_route(int path)
{
   int ret = 0; 
   if ((codec_client != NULL) && (codec_client->phone_ops != NULL)){
	ret = codec_client->phone_ops->set_phone_record(codec_client,path);
   }
}

int phone_record_read_pcm_buf(void* buffer, int bytes)
{
   int ret = 0; 
   if ((codec_client != NULL) && (codec_client->phone_ops != NULL)){
	ret = codec_client->phone_ops->record_read_pcm_buf(codec_client,buffer,bytes);
   }
   return ret;
}


void phone_volume(int path, int volume)
{
   int ret = 0; 
   if ((codec_client != NULL) && (codec_client->phone_ops != NULL)){
	ret = codec_client->phone_ops->set_phone_volume(codec_client,path,volume);
   }
}




/****************************************/
static struct bp_client* bp_client;

void ril_set_call_volume(ril_audio_path_type_t path, int volume)
{
   if ((bp_client != NULL) && (bp_client->bp_ops != NULL)){
	bp_client->bp_ops->set_call_volume(path,volume);
   }
}

void ril_set_call_audio_path(ril_audio_path_type_t path)
{
   if ((bp_client != NULL) && (bp_client->bp_ops != NULL)){
	bp_client->bp_ops->set_call_path(path);
   }
}

void ril_set_call_at(char *at)
{
   if ((bp_client != NULL) && (bp_client->bp_ops != NULL)){
	bp_client->bp_ops->set_call_at(at);
   }
}


int ril_dev_init()
{
   bp_client= bp_client_new();

   if (bp_client == NULL)
	return -1;
	
  return 0;
}

void ril_dev_exit()
{
   if (bp_client != NULL)
	bp_client_free(bp_client);
   bp_client = NULL;
}


