
#define LOG_TAG "codec_audio_plan_two"
#define LOG_NDEBUG 0

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>

#include <sys/ioctl.h>

#include <stdbool.h>
#include <utils/Log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <alloca.h>



#include <sound/asound.h>
//#include <tinyalsa/asoundlib.h>

#include <semaphore.h>

#include "codec_utils.h"



struct record_data{
	unsigned char* record_buf;      //record
	int record_lenth;
volatile	int lenwrite;
volatile	int lenwritedown;
volatile	int lenwriteup;
 volatile	int lenread;
};

static struct record_data record_data;

static sem_t g_sem_record;
static bool g_enable_record;

static struct dev_stream g_bp_send_stream ;
static struct dev_stream g_bp_receive_stream ;
static struct dev_stream g_bt_send_stream ;
static struct dev_stream g_bt_receive_stream ;
static struct dev_stream g_codec_send_stream ;
static struct dev_stream g_codec_receive_stream ;

static struct stream_transfer g_bt_upload_voice ;
static struct stream_transfer g_bt_download_voice ;

static struct stream_transfer g_bp_upload_voice ;
static struct stream_transfer g_bp_download_voice ;

static void *voice_down_thread(void *param);
static void *voice_up_thread(void *param);
static void *voice_bt_down_thread(void *param);
static void *voice_bt_up_thread(void *param);
static void *manager_thread(void *param);
static void *manage_voice_thread(void *param);

static int stream_transfer(struct stream_transfer *stream_transfer);

extern struct pcm_config bp_i2s_out_config ;
extern struct pcm_config bp_i2s_in_config ;
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

//start phone voice , except bt
int plan_two_start_voice(void)
{
	g_bp_upload_voice.voice_thread_run_flag = 1;
	g_bp_download_voice.voice_thread_run_flag = 1;

	g_bp_upload_voice.voice_thread_exit_flag = 1;
	g_bp_download_voice.voice_thread_exit_flag = 1;

	while(!(g_bt_download_voice.voice_thread_exit_flag && g_bt_upload_voice.voice_thread_exit_flag) ){
		ALOGD("plan_two_start_voice: wait bt voice ending");
		usleep(1000);
	}

	ALOGD("start_bp_voice g_bp_upload_voice.voice_thread_run_flag = %d", g_bp_upload_voice.voice_thread_run_flag);

	sem_post(&g_bp_download_voice.sem);
	sem_post(&g_bp_upload_voice.sem);

	return 0;
}

//stop phone voice , except bt
int plan_two_stop_voice(void)
{
	g_bp_upload_voice.voice_thread_run_flag = 0;
	g_bp_download_voice.voice_thread_run_flag = 0;

	if(g_bp_upload_voice.stream_sender->dev)
		pcm_stop(g_bp_upload_voice.stream_sender->dev);
	if(g_bp_upload_voice.stream_receiver->dev)
		pcm_stop(g_bp_upload_voice.stream_receiver->dev);
	if(g_bp_download_voice.stream_sender->dev)
		pcm_stop(g_bp_download_voice.stream_sender->dev);
	if(g_bp_download_voice.stream_receiver->dev)
		pcm_stop(g_bp_download_voice.stream_receiver->dev);

	while(!(g_bp_download_voice.voice_thread_exit_flag && g_bp_upload_voice.voice_thread_exit_flag )){
		ALOGD("plan_two_stop_voice:  wait bp voice ending");
		usleep(1000);
	}
	return 0;
}


int plan_two_start_bt_voice(int up_vol)
{
	g_bt_upload_voice.voice_thread_run_flag = 1;
	g_bt_download_voice.voice_thread_run_flag = 1;
	g_bt_upload_voice.voice_thread_exit_flag = 1;
	g_bt_download_voice.voice_thread_exit_flag = 1;

	while(!(g_bp_download_voice.voice_thread_exit_flag && g_bp_upload_voice.voice_thread_exit_flag )){
		ALOGD("plan_two_start_bt_voice: wait bp voice ending");
		usleep(1000);
	}
	ALOGD("start_bt_voice g_bt_upload_voice.voice_thread_run_flag = %d", g_bt_upload_voice.voice_thread_run_flag);

	sem_post(&g_bt_download_voice.sem);
	sem_post(&g_bt_upload_voice.sem);

	return 0;
}

int plan_two_stop_bt_voice(void)
{
	g_bt_upload_voice.voice_thread_run_flag = 0;
	g_bt_download_voice.voice_thread_run_flag = 0;

	if(g_bt_upload_voice.stream_sender->dev)
		pcm_stop(g_bt_upload_voice.stream_sender->dev);
	if(g_bt_upload_voice.stream_receiver->dev)
		pcm_stop(g_bt_upload_voice.stream_receiver->dev);
	if(g_bt_download_voice.stream_sender->dev)
		pcm_stop(g_bt_download_voice.stream_sender->dev);
	if(g_bt_download_voice.stream_receiver->dev)
		pcm_stop(g_bt_download_voice.stream_receiver->dev);

	while(!(g_bt_download_voice.voice_thread_exit_flag && g_bt_upload_voice.voice_thread_exit_flag )){
		ALOGD("plan_two_stop_bt_voice:  wait bt voice ending");
		usleep(1000);
	}
	return 0;
}

static int g_exit_mixer_buf = 0; // 0 start , 1 wait exit

int plan_two_mixer_buf(char *buf, int bytes)
{

    int     Retlen = bytes;
    unsigned char* bufReadingPtr;

		if(g_enable_record == false){
			return 0;
		}	

    while(1)
    {
	if(record_data.lenwrite - record_data.lenread >= bytes)
        {
            bufReadingPtr = record_data.record_buf + (record_data.lenread%record_data.record_lenth);
            if((bufReadingPtr+bytes) > (record_data.record_buf+record_data.record_lenth))
            {
            	ALOGV("bufReadingPtr:0x%p, Len:%d", bufReadingPtr,bytes);
                int len1 = (record_data.record_buf + record_data.record_lenth - bufReadingPtr);
                memcpy((void *)buf,(void *)bufReadingPtr,len1);
                memcpy((void *)((char *)buf+len1),(void *)record_data.record_buf,bytes-len1);
            }
            else
            {
            	ALOGV("7 bufReadingPtr:0x%p, Len:%d",bufReadingPtr,bytes);
                memcpy(buf,bufReadingPtr,bytes);
            }
            Retlen = bytes;
            record_data.lenread += bytes;
	    break;
       } else {	
		ALOGD("mixer wait2");	
		g_exit_mixer_buf = 0;
		sem_wait(&g_sem_record);	
		if(g_enable_record == false){
			break;
		}	
        }
    }
	ALOGD("3 mixer bytes = %d", Retlen);
	g_exit_mixer_buf = 1;

	return Retlen;;
}

int plan_two_start_record(void)
{
	int record_size=0;
	char *record_buf = NULL;
	int i=0,ret;
	struct list_buf *new;

	memset(&(record_data), 0, sizeof(struct record_data));

	record_size = g_bp_upload_voice.voice_thread_run_flag == 1 ? g_bp_upload_voice.stream_sender->buf_size * 10: g_bt_upload_voice.stream_sender->buf_size * 10;

	record_data.record_buf = (unsigned char *)malloc(record_size);
	if (record_data.record_buf == NULL ){
		ALOGD(" fail to malloc record_data.record_buf");
		return -1;
	}
	record_data.record_lenth = record_size; 
	memset(record_data.record_buf, 0, sizeof(record_data.record_lenth));

	ALOGD("record_data.record_lenth:%d, record_data.record_buf:%p", record_data.record_lenth, record_data.record_buf);

	ret = sem_init(&g_sem_record, 0, 0);
	if (ret) {
		ALOGE("err: g_sem_record failed, ret=%d\n", ret);
		return -1;
	}

	g_bt_upload_voice.record_flag = 1;
	g_bt_download_voice.record_flag = 1;
	g_bp_upload_voice.record_flag = 1;
	g_bp_download_voice.record_flag = 1;
	
	g_exit_mixer_buf = 0;
	
	g_enable_record = true;
	ALOGD("plan_two_start_record");
	return 0;
}

int plan_two_stop_record(void)
{
	g_bt_upload_voice.record_flag = 0;
	g_bt_download_voice.record_flag = 0;
	g_bp_upload_voice.record_flag = 0;
	g_bp_download_voice.record_flag = 0;
	g_enable_record =false;

	memset(&(record_data), 0, sizeof(struct record_data));	
	
	while(g_exit_mixer_buf == 0){
		sem_post(&g_sem_record);
		usleep(1000);
	}
	sem_destroy(&g_sem_record);


	if (record_data.record_buf){
		free(record_data.record_buf);
	}
	ALOGD("plan_two_stop_record");
	return 0;
}



int plan_two_init_voice(void)
{
	int ret=-1;

	g_enable_record = false;

	memset(&g_bp_send_stream, 0, sizeof(struct dev_stream));
	memset(&g_bp_receive_stream, 0, sizeof(struct dev_stream));
	memset(&g_bt_send_stream, 0, sizeof(struct dev_stream));
	memset(&g_bt_receive_stream, 0, sizeof(struct dev_stream));
	memset(&g_codec_send_stream, 0, sizeof(struct dev_stream));
	memset(&g_codec_receive_stream, 0, sizeof(struct dev_stream));

	memset(&g_bt_upload_voice, 0, sizeof(struct stream_transfer));
	memset(&g_bt_download_voice, 0, sizeof(struct stream_transfer));

	memset(&g_bp_upload_voice, 0, sizeof(struct stream_transfer));
	memset(&g_bp_download_voice, 0, sizeof(struct stream_transfer));


//===============================
	//bp
	g_bp_send_stream.type = BP;
	g_bp_send_stream.direction = SENDER; //RECEIVER;
	g_bp_send_stream.config= bp_i2s_in_config; 

	g_bp_receive_stream.type = BP;
	g_bp_receive_stream.direction = RECEIVER;
	g_bp_receive_stream.config= bp_i2s_out_config; 

	//pcm
	g_bt_send_stream.type = BT;
	g_bt_send_stream.direction = SENDER; //RECEIVER;
	g_bt_send_stream.config= bt_pcm_in_config; 

	g_bt_receive_stream.type = BT;
	g_bt_receive_stream.direction = RECEIVER;
	g_bt_receive_stream.config= bt_pcm_out_config; 

	//codec
	g_codec_send_stream.type = CODEC;
	g_codec_send_stream.direction = SENDER;
	g_codec_send_stream.config= codec_in_config; 

	g_codec_receive_stream.type = CODEC;
	g_codec_receive_stream.direction = RECEIVER;
	g_codec_receive_stream.config= codec_out_config; 


//===============================
	//bt upload voice, and download voice
	g_bt_upload_voice.stream_sender= &g_bt_send_stream ;
	g_bt_upload_voice.stream_receiver= &g_bp_receive_stream ;
	g_bt_upload_voice.voice_direction = UPSTREAM;
                       
	g_bt_download_voice.stream_sender= &g_bp_send_stream;
	g_bt_download_voice.stream_receiver= &g_bt_receive_stream;
	g_bt_download_voice.voice_direction = DOWNSTREAM ;

	//bp upload voice, and download voice
	g_bp_upload_voice.stream_sender= &g_codec_send_stream ;
	g_bp_upload_voice.stream_receiver= &g_bp_receive_stream ;
	g_bp_upload_voice.voice_direction = UPSTREAM;
                       
	g_bp_download_voice.stream_sender= &g_bp_send_stream;
	g_bp_download_voice.stream_receiver= &g_codec_receive_stream;
	g_bp_download_voice.voice_direction = DOWNSTREAM;


#if 0
	ret = create_voice_manager(&g_bt_upload_voice, voice_bt_up_thread, &g_bt_upload_voice);
	if (ret <0 ){
		ALOGE("err: create voice_manager uploading of voice failed, ret=%d\n", ret);
		return -1;
	}

	ret = create_voice_manager(&g_bt_download_voice, voice_bt_down_thread, &g_bt_download_voice);
	if (ret <0 ){
		ALOGE("err: create voice_manager downloading of voice failed, ret=%d\n", ret);
		return -1;
	}
#else
	ret = create_voice_manager(&g_bt_upload_voice, manage_voice_thread, &g_bt_upload_voice);
	if (ret <0 ){
		ALOGE("err: create bt voice_manager uploading of voice failed, ret=%d\n", ret);
		return -1;
	}

	ret = create_voice_manager(&g_bt_download_voice, manage_voice_thread, &g_bt_download_voice);
	if (ret <0 ){
		ALOGE("err: create bt voice_manager downloading of voice failed, ret=%d\n", ret);
		return -1;
	}

	ret = create_voice_manager(&g_bp_upload_voice, manage_voice_thread, &g_bp_upload_voice);
	if (ret <0 ){
		ALOGE("err: create bp voice_manager uploading of voice failed, ret=%d\n", ret);
		return -1;
	}

	ret = create_voice_manager(&g_bp_download_voice, manage_voice_thread, &g_bp_download_voice);
	if (ret <0 ){
		ALOGE("err: create bp voice_manager downloading of voice failed, ret=%d\n", ret);
		return -1;
	}
#endif

	return 0;
}

void plan_two_exit_voice(void)
{
	g_enable_record = false;

	g_bt_upload_voice.manage_thread_run_flag= 0;
	g_bt_download_voice.manage_thread_run_flag= 0;
	g_bp_upload_voice.manage_thread_run_flag= 0;
	g_bp_download_voice.manage_thread_run_flag= 0;

	sem_post(&g_bt_download_voice.sem);
	sem_post(&g_bt_upload_voice.sem);
	sem_post(&g_bp_download_voice.sem);
	sem_post(&g_bp_upload_voice.sem);

	close_stream(&g_bt_send_stream);
	close_stream(&g_bt_receive_stream);
	close_stream(&g_codec_send_stream);
	close_stream(&g_codec_receive_stream); 
	close_stream(&g_bp_send_stream);
	close_stream(&g_bp_receive_stream);

	sem_destroy(&g_bt_download_voice.sem);
	sem_destroy(&g_bt_upload_voice.sem);
	sem_destroy(&g_bp_download_voice.sem);
	sem_destroy(&g_bp_upload_voice.sem);
	
	memset(&g_bt_upload_voice, 0, sizeof(struct stream_transfer));
	memset(&g_bt_download_voice, 0, sizeof(struct stream_transfer));
	memset(&g_bp_upload_voice, 0, sizeof(struct stream_transfer));
	memset(&g_bp_download_voice, 0, sizeof(struct stream_transfer));
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
				goto stream_close;
				//return NULL;
			}

			ret = init_stream(transfer_stream->stream_sender);
			if (ret <0 ){
				ALOGE("err: voice_thread init stream  send_stream failed, ret=%d, ****LINE:%d,FUNC:%s", ret, __LINE__, __FUNCTION__);
				goto stream_close;
				//return NULL;
			}

			transfer_stream->voice_thread_exit_flag = 0;

			ALOGD("voice_thread start\n");
			stream_transfer(transfer_stream);
			ALOGD("voice_thread end\n");

stream_close:

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
	short* Srcptr;
	short* Drcptr;
	int size_transfer = 0;
	int ret   =0;
	int exit_flag   =0;
    	int i=0;

	stream_sender = stream_transfer->stream_sender;
	stream_receiver = stream_transfer->stream_receiver;
	size_transfer = stream_sender->buf_size;


#ifdef  START_ZERO_BUFFER
	/* 消除开头杂音 */
	memset(stream_sender->buf, 0, stream_sender->buf_size);
	pcm_write(stream_receiver->dev, stream_sender->buf, stream_sender->buf_size);
#endif


	ret =pcm_wait(stream_sender->dev, 0);
	ret =pcm_wait(stream_receiver->dev, 0);

	pcm_stop(stream_receiver->dev);
	pcm_start(stream_receiver->dev);


	/* 消除开头pa音 */
	memset(stream_sender->buf, 0, stream_sender->buf_size);
	pcm_write(stream_receiver->dev, stream_sender->buf, stream_sender->buf_size);


	while( 1 ){

		if ( (!stream_transfer->voice_thread_run_flag)){
			break;	
		}
#if 0
		if (SNDRV_PCM_STATE_XRUN == get_pcm_state(stream_sender->dev) ){
			//ALOGD("read  SNDRV_PCM_STATE_XRUN ");
			if(ioctl(stream_sender->dev->fd, SNDRV_PCM_IOCTL_PREPARE)){
                		ALOGE("in read, fail to prepare SNDRV_PCM_STATE_XRUN ");
			}
			//usleep(3 * 1000);
			ret =pcm_wait(stream_sender->dev, 0);
			//ALOGD("pcm_read, pcm_wait ret=%d", ret);

			//ALOGD("read after prepare state:%d ",get_pcm_state(stream_sender->dev));
		}
#endif
		ret = pcm_read(stream_sender->dev, stream_sender->buf, size_transfer);
		if (ret != 0) {
			//exit_flag = 1;
			ALOGE("err: read codec err:%s, ret=%d", strerror(errno), ret);
			//break;
		}



		if ( (!stream_transfer->voice_thread_run_flag)){
			break;	
		}
#if 0
		if (SNDRV_PCM_STATE_XRUN == get_pcm_state(stream_receiver->dev) ){
			//ALOGD("write  SNDRV_PCM_STATE_XRUN ");
			pcm_stop(stream_receiver->dev);
			usleep(3 * 1000);
			//ALOGD("write after stop state:%d ",get_pcm_state(stream_receiver->dev));
			pcm_start(stream_receiver->dev);


			ret =pcm_wait(stream_receiver->dev, 0);
			//ALOGD("pcm_write, pcm_wait ret=%d", ret);

			//usleep(3 * 1000);
			//ALOGD("write after prepare state:%d ",get_pcm_state(stream_receiver->dev));
		}
#endif
		ret = pcm_write(stream_receiver->dev, stream_sender->buf, size_transfer);
		if (ret != 0) {
			//exit_flag = 1;
			ALOGE("err: write pcm err:%s, ret=%d", strerror(errno), ret);
		}

		if ( (!stream_transfer->voice_thread_run_flag)){
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
				}
				record_data.lenwritedown += size_transfer;
			}	
			sem_post(&g_sem_record);
		}

		if ( (!stream_transfer->voice_thread_run_flag)){
			break;	
		}
	}

	return 0;
}

