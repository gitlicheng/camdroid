

#define LOG_TAG "codec_audio_plan_record"
#define LOG_NDEBUG 0

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>

#include <utils/Log.h>

#include "codec_utils.h"
#include "record.h"
#include "hal_codec.h"
#include "wav.h"

struct record_private{
	volatile bool manage_thread_run_flag;
	volatile bool record_thread_run_flag;
	volatile bool is_record_from_adc;
	volatile bool is_record_exit;
	volatile bool is_adc_stream_opened;
	struct dev_stream record_stream;
	struct codec_client *client;
	char *buf;
	int buf_size;
	sem_t sem;


};
static struct record_private g_record;


static void *record_data(void *param);
static void *record_manage_thread(void *param);
static int create_record_manage();

struct phone_common_record_ops phone_common_record_ops ={
	.set_record_source = set_record_source,
	.init_record = init_record,
	.exit_record = exit_record,
	.start_record = start_record,
	.stop_record = stop_record,
};

int set_record_source(bool is_record_from_adc)
{

	if(g_record.is_record_from_adc == is_record_from_adc ){
		return 0;
	}

	g_record.is_record_from_adc = is_record_from_adc;

	if(is_record_from_adc == true){
		g_record.record_stream.type = CODEC;
		g_record.record_stream.config= codec_in_config; 
		g_record.record_stream.direction = SENDER;
	} else {
		if(g_record.record_stream.dev){
			pcm_stop(g_record.record_stream.dev);
			ALOGD("pcm_stop record_stream");
		}
	}

	ALOGD("set_record_source is_record_from_adc=%d", g_record.is_record_from_adc);

	return 0;
}

int start_record(int record_buf_size)
{
	if(record_buf_size != 0){
		g_record.buf = (char *)malloc(record_buf_size);
		if (g_record.buf  == NULL ){
			ALOGD(" fail to malloc record_buf size=%d", record_buf_size);
			return -1;
		}
		g_record.buf_size = record_buf_size;
	}

	g_record.record_thread_run_flag = 1;

/*	g_record.is_record_from_adc = is_record_from_adc;

	if(is_record_from_adc == true){
		g_record.record_stream.type = CODEC;
		g_record.record_stream.config= codec_in_config; 
		g_record.record_stream.direction = SENDER;
	}
*/
	g_record.is_adc_stream_opened = false;

	ALOGD("start_record record_thread_run_flag=%d, is_record_from_adc=%d", g_record.record_thread_run_flag, g_record.is_record_from_adc);

	g_record.is_record_exit =0;

	sem_post(&g_record.sem);
	return 0;
}

void stop_record(void)
{
	g_record.record_thread_run_flag = 0;


	if(g_record.is_record_from_adc == true){
		pcm_stop(g_record.record_stream.dev);
	}

	while(!g_record.is_record_exit){
		ALOGD("wait record exit flag");
		usleep(10000);
	}
	if(g_record.buf){
		free(g_record.buf);
	}
	g_record.buf_size = 0;

	if(g_record.is_record_from_adc == true){
		memset(&(g_record.record_stream), 0, sizeof(struct dev_stream));
	}

	g_record.is_record_from_adc = false;
	g_record.is_adc_stream_opened = false;

	ALOGD("stop_record record_thread_run_flag=%d, is_record_from_adc=%d", g_record.record_thread_run_flag, g_record.is_record_from_adc);
}

int init_record(struct codec_client *client)
{
	int ret;

	memset(&(g_record), 0, sizeof(struct record_private));

	ret = sem_init(&g_record.sem, 0, 0);
	if (ret) {
		ALOGE("err: record sem failed, ret=%d\n", ret);
		return -1;
	}

	g_record.manage_thread_run_flag = 1;

	g_record.client = client;

	ret = create_record_manage();
	if (ret <0 ){
		g_record.manage_thread_run_flag = 0;
		ALOGE("err: create record_manager, ret=%d\n", ret);
		return -1;
	}

	return 0;
}

void exit_record()
{
	g_record.record_thread_run_flag = 0;
	g_record.manage_thread_run_flag = 0;

	sem_post(&g_record.sem);
	sem_destroy(&g_record.sem);
	memset(&(g_record), 0, sizeof(struct record_private));
}


static int create_record_manage()
{
	int ret;
	pthread_attr_t attr;
	pthread_t thread_id;

	ret = pthread_attr_init (&attr);
	if (ret != 0) {
		ALOGE("err: pthread_attr_init failed err=%s", strerror(ret));
		goto failed;
	}

	ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (ret != 0) {
		ALOGE("err: pthread_attr_setdetachstate failed err=%s", strerror(ret));
		goto failed;
	}

	ret = pthread_create(&thread_id, &attr, record_manage_thread, &g_record);
	if (ret) {
		ALOGE("err: pthread_create failed, ret=%d\n", ret);
		goto failed;
	}
	return 0;
failed:
	return -1;
}


static volatile long int g_total_size = 0;

static void *record_manage_thread(void *param)
{
	int ret = 0;
	struct record_private *record = (struct record_private*)param;


	while(record->manage_thread_run_flag){

		ALOGV("record manager thread sleep\n");
		sem_wait(&(record->sem));
		ALOGV("record manager thread wakeup\n");

		if(record->record_thread_run_flag){

			g_record.is_record_exit = 0;
			g_total_size = 0 ;

			wav_start();

    			ALOGD("record thread start");
			record_data(param);

			ALOGD("record thread end, g_total_size %ld" ,g_total_size);
			wav_end(g_total_size);

			if(g_record.is_adc_stream_opened == true ){
				close_stream(&(record->record_stream));
				g_record.is_adc_stream_opened = false;
				ALOGV("close_stream  record");
			}

			g_record.is_record_exit = 1;


		}
	}

    ALOGD("record manager thread exit\n");
	return param;
}



//phone record except bt 
static void *record_data(void *param)
{
	int ret = 0;
	int exit_flag   =0;
	char *tmp_buf;
	int  data_count;
	struct record_private *record = (struct record_private*)param;

	while(1){
		if( g_record.is_record_from_adc == true ){
			if(g_record.is_adc_stream_opened == false){
				if( (ret = init_stream(&(record->record_stream)) ) < 0 ){
					ALOGE("err: init_stream  record_stream failed, ret=%d, ****LINE:%d,FUNC:%s", ret, __LINE__, __FUNCTION__);
					g_record.is_record_exit = 1;
					exit_flag = 1;
					//return NULL;
				}
				g_record.is_adc_stream_opened = true;
			}

			ret = pcm_read(record->record_stream.dev, record->record_stream.buf, record->record_stream.buf_size);
			if (ret != 0) {
				exit_flag = 1;
				ALOGE("err: record_data:%s, ret=%d", strerror(errno), ret);
				//break;
			}
			tmp_buf = record->record_stream.buf;
			data_count = record->record_stream.buf_size;
		} else {
			if(g_record.is_adc_stream_opened == true ){
				close_stream(&(record->record_stream));
				g_record.is_adc_stream_opened = false;
				ALOGV("close_stream  record");
			}

		        ALOGD("record_data");
			ret = record->client->phone_ops->record_read_pcm_buf(record->client, record->buf, record->buf_size);
			tmp_buf = record->buf;
			data_count = ret;
		}


		if (wav_save(tmp_buf, data_count)){
			g_total_size += data_count;
			ALOGD(" record count=%ld", g_total_size);		
		}

		if ( (!record->record_thread_run_flag) || (exit_flag == 1)){
			ALOGD(" record data exit, record->record_thread_run_flag=%d, exit_flag=%d", record->record_thread_run_flag, exit_flag);	
			break;	
		}	
	}

	return param;
}

