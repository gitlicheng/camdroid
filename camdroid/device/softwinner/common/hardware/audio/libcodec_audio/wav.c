
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<time.h>


#define LOG_TAG "bluetooth voice"
#define LOG_NDEBUG 0
#include <cutils/log.h>


#include "wav.h"


#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

#define FORMAT_PCM 1
#define RECORD_NAME "/sdcard/record_test.wav"

//static unsigned int frames =0;

struct wav_header {
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t riff_fmt;
    uint32_t fmt_id;//
    uint32_t fmt_sz;
    uint16_t audio_format;//
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    uint32_t data_id;
    uint32_t data_sz;
};


static struct wav_header header;
static FILE *file;

void wav_start()
{
        char *wday[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
        time_t timep;
        struct tm *p; 
        char filename[50]={0};
        time(&timep);
        p=localtime(&timep); /*取得当地时间*/
        //printf ("%d%d%d ", (1900+p->tm_year),( p->tm_mon), p->tm_mday);
        //printf("%s %d:%d:%d\n", wday[p->tm_wday],p->tm_hour, p->tm_min, p->tm_sec);
        sprintf(filename, "%s%d%d%d-%s-%d-%d-%d%s", "/sdcard/", (1900+p->tm_year),(1 + p->tm_mon), p->tm_mday, wday[p->tm_wday],p->tm_hour, p->tm_min, p->tm_sec, ".wav");
        //printf("buf=%s\n", filename);



    header.riff_id = ID_RIFF;
    header.riff_sz = 0;
    header.riff_fmt = ID_WAVE;
    header.fmt_id = ID_FMT;
    header.fmt_sz = 16;
    header.audio_format = FORMAT_PCM;
    header.num_channels = 1;
    header.sample_rate = 8000;
    header.bits_per_sample = 16;
    header.byte_rate = (header.bits_per_sample / 8) * (header.num_channels) * (header.sample_rate) ;
    header.block_align = (header.num_channels) * (header.bits_per_sample / 8);
    header.data_id = ID_DATA;

    file = fopen(filename, "wb+");
    if (!file) {
        ALOGE("in, wav_header, Unable to create record wav ");
        return ;
    }

    fseek(file, sizeof(struct wav_header), SEEK_SET);

    int frames =1;
    header.data_sz = frames * header.block_align;

    //fseek(file, 0, SEEK_SET);
   // fwrite(&header, sizeof(struct wav_header), 1, file);

    //fclose(file);
}

int wav_save(char *buf, unsigned int buf_size)
{
        if (fwrite(buf, buf_size, 1, file) != 1) {
            ALOGE("Error write record file,err:%s\n",strerror(errno));
            return 0;
        }    
	return 1;
}

int wav_end(int data)
{
   int frames = 0;
   
   frames = data/( (header.bits_per_sample / 8) * (header.num_channels) ) ;
   header.data_sz = frames * header.block_align;
   header.riff_sz = header.data_sz + sizeof(struct wav_header) - 8  ;

   fseek(file, 0, SEEK_SET);
   fwrite(&header, sizeof(struct wav_header), 1, file);
   ALOGD("3 Write record file end, header.data_sz=%d,header.riff_sz=%d", header.data_sz, header.riff_sz);
 
   fclose(file);

    return 1;
}
/*
int main(int argc, char **argv) 
{
	char buf[1024]={0};
ALOGE("wav start");
wav_start();
wav_save(buf, 1024);
wav_end(512);
return 0;
}
*/
