// 1. change mic record sound for 10s
// 2. adjust capture.wav's path to / and keep music's path to tf

#include "tinyalsa/asoundlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>
#include <pthread.h>

#define TAG "mic&spktester"	// mic and speaker tester
#include <dragonboard.h>

#define FIFO_MIC_DEV  "fifo_mic"
#define FIFO_SPK_DEV  "fifo_spk"

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164
#define FORMAT_PCM 1

int fifoMicFd = 0, fifoSpkFd = 0;
int capturingFlag  = 0;
int playingType = 0;	// 0-raw music file; 1-recorded file
int playSuccess = 1;	// 1-success; 0-failed for spk

struct riff_wave_header {
	uint32_t riff_id;
	uint32_t riff_sz;
	uint32_t wave_id;
};

struct chunk_header {
	uint32_t id;
	uint32_t sz;
};

struct chunk_fmt {
	uint16_t audio_format;
	uint16_t num_channels;
	uint32_t sample_rate;
	uint32_t byte_rate;
	uint16_t block_align;
	uint16_t bits_per_sample;
};

struct wav_header {
	uint32_t riff_id;
	uint32_t riff_sz;
	uint32_t riff_fmt;
	uint32_t fmt_id;
	uint32_t fmt_sz;
	uint16_t audio_format;
	uint16_t num_channels;
	uint32_t sample_rate;
	uint32_t byte_rate;
	uint16_t block_align;
	uint16_t bits_per_sample;
	uint32_t data_id;
	uint32_t data_sz;
};


void play_sample(FILE *fp, unsigned int card, unsigned int device, unsigned int channels, unsigned int rate, unsigned int bits, unsigned int period_size, unsigned int period_count)
{
	struct pcm_config config;
	struct pcm *pcm;
	char *buffer;
	int size;
	int num_read;
	
	config.channels = channels;
	config.rate = rate;
	config.period_size = period_size;
	config.period_count = period_count;
	if (bits == 32)
		config.format = PCM_FORMAT_S32_LE;
	else if (bits == 16)
		config.format = PCM_FORMAT_S16_LE;
	config.start_threshold = 0;
	config.stop_threshold = 0;
	config.silence_threshold = 0;
	
	pcm = pcm_open(card, device, PCM_OUT, &config);
	if (!pcm || !pcm_is_ready(pcm)) {
		db_error("open PCM device %u (%s) failed\n", device, pcm_get_error(pcm));
		playSuccess = 0;
		write(fifoSpkFd, "F[SPK] pcm NOT ready", 20);
		return;
	}
	size = pcm_frames_to_bytes(pcm, pcm_get_buffer_size(pcm));
	buffer = malloc(size);
	if (buffer == NULL) {
		db_error("allocate %d bytes failed\n", size);
		playSuccess = 0;
		write(fifoSpkFd, "F[SPK] memory low!!!", 21);
		pcm_close(pcm);
		return;
	}
	db_verb("playing wav file: %u ch, %u hz, %u bit\n", channels, rate, bits);
	do {
		num_read = fread(buffer, 1, size, fp);
		if (num_read > 0) {
			if (pcm_write(pcm, buffer, num_read)) {
				db_warn("Error playing sample\n");
				playSuccess = 0;
				write(fifoSpkFd, "F[SPK] pcm_write!!!", 20);
				break;
			}
		}
	} while (num_read > 0);
	db_verb("end of the wav file\n");
	
	free(buffer);
	pcm_close(pcm);
}

static void InitTinyMixer(void)
{	
	struct mixer* mixer;
	struct mixer_ctl* ctl;
	int retVal;

	mixer = mixer_open(0);
	if (mixer == NULL) {
		db_warn("open mixer failed\n");
	}

	ctl = mixer_get_ctl(mixer, 16);
	retVal = mixer_ctl_set_value(ctl, 0, 2);	// set speaker = spk(1) 0: headet, 1:spk, 2:headset-spk 
	if (retVal < 0) {
		db_warn("mixer_ctl_set_value(Speaker) failed\n");
	}

	ctl = mixer_get_ctl(mixer, 1);
	retVal = mixer_ctl_set_value(ctl, 0, 31);	// set Line Volume to max
	if (retVal < 0) {
		db_warn("mixer_ctl_set_value(Line Volume) failed\n");
	}

	mixer_close(mixer);
}

int CaptureSample(FILE* fp, unsigned int card, unsigned int device, unsigned int channels, unsigned int rate, unsigned int bits, unsigned int period_size, unsigned int period_count)
{
	struct pcm_config config;
	struct pcm* pcm;
	char* buffer;
	unsigned int size;
	unsigned int captureBytes = 0;

	config.channels = channels;
	config.in_init_channels = channels;
	config.rate = rate;
	config.period_size = period_size;
	config.period_count = period_count;
	if (bits == 32) 
		config.format = PCM_FORMAT_S32_LE;
	else
		config.format = PCM_FORMAT_S16_LE;
	config.start_threshold = 0;
	config.stop_threshold = 0;
	config.silence_threshold = 0;
	pcm = pcm_open_req(card, device, PCM_IN, &config, rate);
	if (pcm == NULL || pcm_is_ready(pcm) == 0) {
		db_error("open PCM device failed\n");
		return 0;
	}
	size = pcm_get_buffer_size(pcm);
	buffer = malloc(size);
	if (buffer == NULL) {
		db_error("malloc %d bytes failed\n", size);
		pcm_close(pcm);
		return 0;
	}

	db_msg("capturing sound: %u channels, %u hz, %u bits\n", channels, rate, bits);
	while (capturingFlag && !pcm_read(pcm, buffer, size)) {
		if (fwrite(buffer, 1, size, fp) != size) {
			db_warn("capturing sound error\n");
			break;
		}
		captureBytes += size;
	}
	free(buffer);
	pcm_close(pcm);

	return captureBytes / ((bits / 8) * channels);
}

void* TinyCaptureThread(void* argv)
{
	FILE* recFp;
	unsigned int card = 0;
	unsigned int device = 0;
	unsigned int channels = 2;
	unsigned int rate = 44100;
	unsigned int bits = 16;
	unsigned int frames;
	unsigned int period_size = 1024;
	unsigned int period_count = 4;

	if ((fifoMicFd = open(FIFO_MIC_DEV, O_WRONLY)) < 0) {
		if (mkfifo(FIFO_MIC_DEV, 0666) < 0) {
			db_error("mkfifo failed(%s)\n", strerror(errno));
			return NULL;
		} else {
			fifoMicFd = open(FIFO_MIC_DEV, O_WRONLY);
		}
	}
	write(fifoMicFd, "W[MIC] waiting", 20);		// 20 chars display at most
	
	recFp = fopen((char*)argv, "wb");
	if (recFp == NULL) {
		db_error("open file %s failed\n", (char*)argv);
		write(fifoMicFd, "F[MIC] fopen failed", 20);	// means open or create file failed
		return NULL;
	}

	struct wav_header header = {
		ID_RIFF,
		0,
		ID_WAVE,
		ID_FMT,
		16,
		FORMAT_PCM,
		channels,
		rate,
		(bits / 8) * channels * rate,
		(bits / 8) * channels,
		bits,
		ID_DATA,
		0
	};
	fseek(recFp, sizeof(struct wav_header), SEEK_SET);
//	signal(SIGINT, SigintRecord);
	write(fifoMicFd, "P[MIC] recording...", 20);
	capturingFlag  = 1;		// elapse for 20s, then reset to 0 by main func
	frames = CaptureSample(recFp, card, device, header.num_channels, header.sample_rate, header.bits_per_sample, period_size, period_count);
	write(fifoMicFd, "P[MIC] ready play...", 21);
	db_msg("capture %d frames\n", frames);
	header.data_sz = frames * header.block_align;
	fseek(recFp, 0, SEEK_SET);
	fwrite(&header, sizeof(struct wav_header), 1, recFp);
	fclose(recFp);

	return NULL;
}

void* TinyPlayThread(void* argv)
{
	FILE *fp;
	struct riff_wave_header riff_wave_header;
	struct chunk_header chunk_header;
	struct chunk_fmt chunk_fmt;
	unsigned int device = 0;
	unsigned int card = 0;
	unsigned int period_size = 1024;
	unsigned int period_count = 4;
	int more_chunks = 1;
	int fd;

	// playingType == 0: music; 1: capture sample(file has been opened)
	if ((playingType == 0) && (fifoSpkFd = open(FIFO_SPK_DEV, O_WRONLY)) < 0) {
		if (mkfifo(FIFO_SPK_DEV, 0666) < 0) {
			db_error("mkfifo failed(%s)\n", strerror(errno));
			return NULL;
		} else {
			fifoSpkFd = open(FIFO_SPK_DEV, O_WRONLY);
		}
	}
	db_verb("open file %s and ready to play\n", (char*)argv);
	if ((fd = open((char *)argv, O_RDONLY | O_NONBLOCK)) > 0) {
		close(fd);
	} else {
		playingType = 1;
		db_warn("open file '%s' failed\n", (char*)argv);
		return NULL;
	}
	fp = fopen((char*)argv, "rb");
	if (fp == NULL) {
		db_error("open file '%s' failed\n", (char*)argv);
		return NULL;
	}
	fread(&riff_wave_header, sizeof(riff_wave_header), 1, fp);
	if ((riff_wave_header.riff_id != ID_RIFF) || (riff_wave_header.wave_id != ID_WAVE)) {
		db_error("Error: '%s' is not a riff/wave file\n", (char*)argv);
		fclose(fp);
		return NULL;
	}
//	InitTinyMixer();		// must be done before play music!!!(V3 need, A20 NOT need)(newest test it can work in A20)
	system("tinymix 1 31");
	system("tinymix 16 1");
	do {
		fread(&chunk_header, sizeof(chunk_header), 1, fp);
		switch (chunk_header.id) {
			case ID_FMT:
				fread(&chunk_fmt, sizeof(chunk_fmt), 1, fp);
				/* If the format header is larger, skip the rest */
				if (chunk_header.sz > sizeof(chunk_fmt))
					fseek(fp, chunk_header.sz - sizeof(chunk_fmt), SEEK_CUR);
			    	break;
			case ID_DATA:
			    	/* Stop looking for chunks */
				more_chunks = 0;
				break;
			default:
			    	/* Unknown chunk, skip bytes */
				fseek(fp, chunk_header.sz, SEEK_CUR);
		}
	} while (more_chunks);
	if (playingType == 0) {			// raw music
		write(fifoSpkFd, "P[SPK] music playing", 21);
	} else {						// noise record file
		write(fifoSpkFd, "P[SPK] rec.. playing", 21);
	}
	play_sample(fp, card, device, chunk_fmt.num_channels, chunk_fmt.sample_rate, chunk_fmt.bits_per_sample, period_size, period_count);
	if (playingType == 0 && playSuccess == 1) {
		write(fifoSpkFd, "P[SPK] music end", 21);
		playingType = 1;				// means end of music, and ready to play capture.wav
	} else if (playingType == 1 && playSuccess == 1){
		write(fifoSpkFd, "P[SPK] rec.. end", 21);
		write(fifoMicFd, "P[MIC] pass", 12);		// NOTICE: make sure len = length(str) + 1
	}

	fclose(fp);
	pthread_exit(NULL);
	return NULL;
}

int main(int argc, char **argv)
{
	int retVal, fd;
	int capCountDown = 5;			// record for 10s
	pthread_t playTid, captureTid, recPlayTid;
	char* musicFile = NULL;
	char* musicPath1 = "/system/res/others/startup.wav";	// for V3
	char* musicPath2 = "/res/others/startup.wav";		// for A20
	char* captureFile = "capture.wav";

	db_msg("playing and capturing the music for 10 seconds\n");
	if ((fd = open(musicPath1, O_RDONLY | O_NONBLOCK)) > 0) {
		musicFile = musicPath1;
		close(fd);
	} else if ((fd = open(musicPath2, O_RDONLY | O_NONBLOCK)) > 0) {
		musicFile = musicPath2;
		close(fd);
	} else {
		musicFile = musicPath1;
	}

//	system("tinymix 1 31");
//	system("tinymix 16 1");

	pthread_create(&playTid, NULL, TinyPlayThread, musicFile);
	pthread_create(&captureTid, NULL, TinyCaptureThread, captureFile);

	while(capturingFlag == 0) {
		usleep(20000);
	}
	while(fifoMicFd == 0) {
		usleep(20000);
	}
	char micInfo[20] = "P[MIC] countdown ";
	char countDownBuf[3];
	while (capCountDown) {			// capture sound for 20->10s seconds
		sprintf(countDownBuf, "%d", capCountDown--);
		strncpy(micInfo + 17, countDownBuf, 3);	// careful here!
		write(fifoMicFd, micInfo, 20);	// add 1 char for '\0'
		sleep(1);
	}
	capturingFlag = 0;			// means that capturing end
//	printf("mic: send end of record for 5s");
//	sleep(5);
	write(fifoMicFd, "P[MIC] end of record", 21);
	sleep(1);
//	printf("mic: end of record for 5s");

//	retVal = pthread_join(playTid, NULL);	// make sure the raw music elaps 20s!
//	sleep(10);
	while (playingType == 0) {
		sleep(1);
	}
	db_msg("Now, play the captured sample...\n");
//	playingType = 1;
	write(fifoMicFd, "P[MIC] start play...", 21);	// here happens a accident because I copy 20 chars!!! Careful for it!
//	sleep(5);
	TinyPlayThread(captureFile);
	db_msg("Now, end of the captured sample\n");
	
	return 0;
}

