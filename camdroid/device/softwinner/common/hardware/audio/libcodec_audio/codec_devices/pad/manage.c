
#define LOG_TAG "codec_audio_plan_one"
#define LOG_NDEBUG 0

#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <alloca.h>
//#include <tinyalsa/asoundlib.h>

#include <semaphore.h>

#include "codec_utils.h"

struct record_data{
	unsigned char* record_buf;      //record
	int 		record_lenth;
	volatile	int lenwrite;
	volatile	int lenwritedown;
	volatile	int lenwriteup;
	volatile	int lenread;
};
static struct record_data record_data;

static sem_t sem_record;

static struct dev_stream g_bt_send_stream ;
static struct dev_stream g_bt_receive_stream ;
static struct dev_stream g_codec_send_stream ;
static struct dev_stream g_codec_receive_stream ;

static struct stream_transfer g_upload_voice ;
static struct stream_transfer g_download_voice ;

static void *manage_voice_thread(void *param);
static int stream_transfer(struct stream_transfer *stream_transfer);

extern struct pcm_config bt_pcm_out_config;
extern struct pcm_config bt_pcm_in_config ;
extern struct pcm_config codec_out_config ;
extern struct pcm_config codec_in_config ;

static int create_voice_manager(struct stream_transfer *transfer, voice_thread func, void *parg)
{
	int ret;
	pthread_attr_t attr;


	ret = sem_init(&transfer->sem, 0, 0);
	if (ret) {
		ALOGE("err: sem_init failed, ret=%d\n", ret);
		goto sem_init_failed;
	}

	ret = pthread_attr_init (&attr);
	if (ret != 0) {
		ALOGE("err: pthread_attr_init failed err=%s", strerror(ret));
		goto pthread_attr_init_failed;
	}

	ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (ret != 0) {
		ALOGE("err: pthread_attr_setdetachstate failed err=%s", strerror(ret));
		goto pthread_attr_setdetachstate_failed;
	}

	transfer->manage_thread_run_flag = 1;
	transfer->voice_thread_run_flag = 0;
	transfer->voice_thread_exit_flag = 1;
	transfer->record_flag = 0;
	transfer->func = func;

	ret = pthread_create(&transfer->pid, &attr, transfer->func, parg);
	if (ret) {
		ALOGE("err: pthread_create failed, ret=%d\n", ret);
		goto pthread_create_failed;
	}

	return 0;

pthread_create_failed:
pthread_attr_setdetachstate_failed:
pthread_attr_init_failed:
	transfer->manage_thread_run_flag = 0;
	transfer->voice_thread_run_flag = 0;
	transfer->voice_thread_exit_flag = 1;
	transfer->record_flag = 0;
	transfer->func = func;
	sem_destroy(&transfer->sem);

sem_init_failed:
	return -1;
}

int plan_one_start_bt_voice(void)
{
	g_upload_voice.voice_thread_run_flag = 1;
	g_download_voice.voice_thread_run_flag = 1;

	ALOGD("start_bt_voice g_upload_voice.voice_thread_run_flag = %d", g_upload_voice.voice_thread_run_flag);

	while(!(g_download_voice.voice_thread_exit_flag && g_upload_voice.voice_thread_exit_flag) ){
		ALOGD("plan_one_start_bt_voice: wait last voice ending");
		usleep(100);
	}

	sem_post(&g_download_voice.sem);
	sem_post(&g_upload_voice.sem);

	return 0;
}

int plan_one_stop_bt_voice(void)
{
	g_upload_voice.record_flag = 0;
	g_download_voice.record_flag = 0;

	g_upload_voice.voice_thread_run_flag = 0;
	g_download_voice.voice_thread_run_flag = 0;

	while(!(g_download_voice.voice_thread_exit_flag && g_upload_voice.voice_thread_exit_flag) ){
		ALOGD("plan_one_stop_bt_voice: wait voice ending");
		usleep(100);
	}

	return 0;
}

int plan_one_mixer_buf(char *buf, int bytes)
{

    int     Retlen = bytes;
    unsigned char* bufReadingPtr;


    while(1)
    {
	if(record_data.lenwrite - record_data.lenread >= bytes)
        {
            bufReadingPtr = record_data.record_buf + (record_data.lenread%record_data.record_lenth);
            if((bufReadingPtr+bytes) > (record_data.record_buf+record_data.record_lenth))
            {
            	ALOGD("1 bufReadingPtr:0x%p, Len:%d", bufReadingPtr,bytes);
                int len1 = (record_data.record_buf + record_data.record_lenth - bufReadingPtr);
                memcpy((void *)buf,(void *)bufReadingPtr,len1);
                memcpy((void *)((char *)buf+len1),(void *)record_data.record_buf,bytes-len1);
            }
            else
            {

            	ALOGD("2 bufReadingPtr:0x%p, Len:%d",bufReadingPtr,bytes);
                memcpy(buf,bufReadingPtr,bytes);
            }
            Retlen = bytes;
            record_data.lenread += bytes;
	    break;
       } else {
		if(g_upload_voice.record_flag == 0 || g_download_voice.record_flag == 0){
			break;
		}	
		sem_wait(&sem_record);	
		if(g_upload_voice.record_flag == 0 || g_download_voice.record_flag == 0){
			break;
		}	
        }
    }
	ALOGD("mixer bytes = %d", Retlen);

	return Retlen;;
}

int plan_one_start_bt_record(void)
{
	int record_size=0;
	char *record_buf = NULL;
	int i=0,ret;
	struct list_buf *new;

	memset(&(record_data), 0, sizeof(struct record_data));

	record_data.record_buf = (unsigned char *)malloc(g_upload_voice.stream_sender->buf_size * 10);
	if (record_data.record_buf == NULL ){
		ALOGD(" fail to malloc record_data.record_buf");
		return -1;
	}
	record_data.record_lenth = g_upload_voice.stream_sender->buf_size * 10 ; 
	memset(record_data.record_buf, 0, sizeof(record_data.record_lenth));

	ALOGD("record_data.record_lenth:%d, record_data.record_buf:%p", record_data.record_lenth, record_data.record_buf);

	ret = sem_init(&sem_record, 0, 0);
	if (ret) {
		ALOGE("err: sem_record failed, ret=%d\n", ret);
		return -1;
	}

	g_upload_voice.record_flag = 1;
	g_download_voice.record_flag = 1;
	return 0;
}

int plan_one_stop_bt_record(void)
{
	g_upload_voice.record_flag = 0;
	g_download_voice.record_flag = 0;

	memset(&(record_data), 0, sizeof(struct record_data));	
	sem_post(&sem_record);
	sem_destroy(&sem_record);
	if (record_data.record_buf){
		free(record_data.record_buf);
	}

	return 0;
}



int plan_one_init_voice(void)
{
	int ret=-1;

	memset(&g_bt_send_stream, 0, sizeof(struct dev_stream));
	memset(&g_bt_receive_stream, 0, sizeof(struct dev_stream));
	memset(&g_codec_send_stream, 0, sizeof(struct dev_stream));
	memset(&g_codec_receive_stream, 0, sizeof(struct dev_stream));

	memset(&g_upload_voice, 0, sizeof(struct stream_transfer));
	memset(&g_download_voice, 0, sizeof(struct stream_transfer));

	//pcm
	g_bt_send_stream.type = BT;
	g_bt_send_stream.direction = SENDER; //RECEIVER;
	g_bt_send_stream.config= bt_pcm_out_config; 

	g_bt_receive_stream.type = BT;
	g_bt_receive_stream.direction = RECEIVER;
	g_bt_receive_stream.config= bt_pcm_in_config; 

	//codec
	g_codec_send_stream.type = CODEC;
	g_codec_send_stream.direction = SENDER;
	g_codec_send_stream.config= codec_out_config; 

	g_codec_receive_stream.type = CODEC;
	g_codec_receive_stream.direction = RECEIVER;
	g_codec_receive_stream.config= codec_in_config; 


	//upload voice, and download voice
	g_upload_voice.stream_sender= &g_bt_send_stream ;
	g_upload_voice.stream_receiver= &g_codec_receive_stream ;
	g_upload_voice.voice_direction = UPSTREAM;
                       
	g_download_voice.stream_sender= &g_codec_send_stream;
	g_download_voice.stream_receiver= &g_bt_receive_stream;
	g_download_voice.voice_direction = DOWNSTREAM;

	ret = create_voice_manager(&g_upload_voice, manage_voice_thread, &g_upload_voice);
	if (ret <0 ){
		ALOGE("err: create voice_manager uploading of voice failed, ret=%d\n", ret);
		return -1;
	}

	ret = create_voice_manager(&g_download_voice, manage_voice_thread, &g_download_voice);
	if (ret <0 ){
		ALOGE("err: create voice_manager downloading of voice failed, ret=%d\n", ret);
		return -1;
	}

	return 0;
}

void plan_one_exit_voice(void)
{
	g_upload_voice.manage_thread_run_flag= 0;
	g_download_voice.manage_thread_run_flag= 0;

	sem_post(&g_download_voice.sem);
	sem_post(&g_upload_voice.sem);

	close_stream(&g_bt_send_stream);
	close_stream(&g_bt_receive_stream);
	close_stream(&g_codec_send_stream);
	close_stream(&g_codec_receive_stream); 

	sem_destroy(&g_download_voice.sem);
	sem_destroy(&g_upload_voice.sem);
	
	memset(&g_upload_voice, 0, sizeof(struct stream_transfer));
	memset(&g_download_voice, 0, sizeof(struct stream_transfer));
}


static void *manage_voice_thread(void *param)
{
	int ret = 0;
	struct stream_transfer *transfer_stream = (struct stream_transfer*)param;

	while(transfer_stream->manage_thread_run_flag){

		ALOGV("common manager thread sleep\n");
		sem_wait(&(transfer_stream->sem));
		ALOGV("common manager thread wakeup\n");

		if(transfer_stream->voice_thread_run_flag){
			ret = init_stream(transfer_stream->stream_receiver);
			if (ret <0 ){
				ALOGE("err: voice_thread init stream receive_stream failed, ret=%d, ****LINE:%d,FUNC:%s", ret, __LINE__, __FUNCTION__);
				return NULL;
			}

			ret = init_stream(transfer_stream->stream_sender);
			if (ret <0 ){
				ALOGE("err: voice_thread init stream  send_stream failed, ret=%d, ****LINE:%d,FUNC:%s", ret, __LINE__, __FUNCTION__);
				return NULL;
			}

			transfer_stream->voice_thread_exit_flag = 0;

			ALOGD("voice_thread start\n");
			stream_transfer(transfer_stream);
			ALOGD("voice_thread end\n");


			close_stream(transfer_stream->stream_receiver);
			close_stream(transfer_stream->stream_sender);

			transfer_stream->voice_thread_exit_flag = 1;
		}
	}

	ALOGD("common manager thread exit\n");

	return param;
}

static int stream_transfer(struct stream_transfer *stream_transfer)
{

	struct dev_stream *stream_sender;
	struct dev_stream *stream_receiver;
	int size_transfer = 0;
	int ret   =0;
	int exit_flag   =0;
	int i   =0;
	short* Srcptr;
	short* Drcptr;

	stream_sender = stream_transfer->stream_sender;
	stream_receiver = stream_transfer->stream_receiver;
	size_transfer = stream_sender->buf_size;


#ifdef  START_ZERO_BUFFER
	/* 消除开头杂音 */
	memset(stream_sender->buf, 0, stream_sender->buf_size);
	pcm_write(stream_receiver->dev, stream_sender->buf, stream_sender->buf_size);
#endif

	while( 1 ){
		if ( (!stream_transfer->voice_thread_run_flag) || (exit_flag == 1)){
			break;	
		}

		ret = pcm_read(stream_sender->dev, stream_sender->buf, size_transfer);
		if (ret != 0) {
			exit_flag = 1;
			ALOGE("err: read codec err:%s, ret=%d", strerror(errno), ret);
			break;
		}

		if ( (!stream_transfer->voice_thread_run_flag) || (exit_flag == 1)){
			break;	
		}

		ret = pcm_write(stream_receiver->dev, stream_sender->buf, size_transfer);
		if (ret != 0) {
			exit_flag = 1;
			ALOGE("err: write pcm err:%s, ret=%d", strerror(errno), ret);
		}

		if ( (!stream_transfer->voice_thread_run_flag) || (exit_flag == 1)){
			break;	
		}

		if (stream_transfer->record_flag == 1){
			//是上行,还是下行.
			if (stream_transfer->voice_direction == UPSTREAM){
				Srcptr = (short*)(stream_sender->buf);
				Drcptr = (short*)(record_data.record_buf + (record_data.lenwriteup%record_data.record_lenth));
				if(record_data.lenwriteup >= record_data.lenwritedown)
				{
					memcpy(Drcptr,Srcptr,size_transfer);
				}
				else
				{
					int i;
					for(i=0;i<size_transfer/2;i++,Drcptr++)
					{
						*Drcptr = (*Drcptr + *Srcptr++)/2;
					}
					record_data.lenwrite += size_transfer;
//				        sem_post(&sem_record);
				}
				record_data.lenwriteup += size_transfer;
				//ALOGD("stream is upload");
			} else {
				Srcptr = (short*)(stream_sender->buf);
				Drcptr = (short*)(record_data.record_buf + (record_data.lenwritedown%record_data.record_lenth));
				if(record_data.lenwritedown >= record_data.lenwriteup)
				{
					memcpy(Drcptr,Srcptr,size_transfer);
				}
				else
				{

					for(i=0;i<size_transfer/2;i++,Drcptr++)
					{
						*Drcptr = ((int)*Drcptr + (int)(*Srcptr++))/2;
					}
					record_data.lenwrite += size_transfer;
//				        sem_post(&sem_record);
				}
				record_data.lenwritedown += size_transfer;
				//ALOGD("stream is download");
			}	
		        sem_post(&sem_record);
		}

		//ALOGD("pcm running ... , type=%d ",stream_sender->type);
		if ( (!stream_transfer->voice_thread_run_flag) || (exit_flag == 1)){
			break;	
		}
	}
	return 0;
}

