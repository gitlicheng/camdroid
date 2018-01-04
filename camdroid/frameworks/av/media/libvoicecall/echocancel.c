/*
 * Author: yewenliang@allwinnertech.com
 * Copyright (C) 2014 Allwinner Technology Co., Ltd.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "EchoCancellation"
//#include <cutils/log.h>
#include "dbg_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <utils/threads.h>
//#include <pthread.h>

#include "echo_control.h"
#include "echocancel.h"

typedef struct aec_buffer_manager {
    char *                  buffers;
    uint32_t                totalSize;
    uint32_t                availSize;
    volatile    uint32_t    user;
    volatile    uint32_t    server;
    pthread_mutex_t         mutex;
} AecBuffManager;

typedef struct AecStreamContext {
    AecStreamState          state;
    int32_t                 sampleRate;
    int32_t                 channels;
} AecStreamCtx;

typedef struct Aec_Context_t {
    void *                  handle;
    AecBuffManager *        bufManager;
    char *                  echoBuffer;
    uint32_t                echoBuffSize;
    uint32_t                sliceSize;
    int32_t                 sampleRate;
    //int32_t                 channels;
    int                     hasAec;
    pthread_mutex_t         mutex;
    int                     isInited;
} AecContext;

//#define AEC_DUMP
//#define AEC_USE_COEF

#ifdef AEC_DUMP
#define ECHO_FILE "/mnt/extsd/pcmEcho.pcm"
#define IN_FILE "/mnt/extsd/pcmIn.pcm"
#define EC_OUT_FILE "/mnt/extsd/pcmEcout.pcm"

static FILE * pcmEcho = NULL;
static FILE * pcmIn = NULL;
static FILE * pcmEcOut = NULL;
#endif

static int g_max_delay = 40;
static int g_near_delay = 12;
static int g_tail_length = 128;
static int g_save_coef = 0;
static int g_delay_margin = 7;
static int g_max_suppression = -12;

#ifdef AEC_USE_COEF
/* NOTE! coef.bin should not be too large, for data area is limit */
#define AEC_COEF_PATH "/data/coef.bin"
static const char* aec_coef_file = AEC_COEF_PATH;
static void load_aec_coef(void* aec)
{
    FILE *fcoef;
    int length1 = 0, length2 = 0;
    short* coef_buffer;
    if ((fcoef = fopen(aec_coef_file, "rb")) != NULL)
    {
        fread(&length1, sizeof(int), 1, fcoef);
        fread(&length2, sizeof(int), 1, fcoef);
        /*
        fseek(fcoef, 0, SEEK_END);
        length = ftell(fcoef);

        fseek(fcoef, 0, SEEK_SET);
        */
        if ((length1+length2) >= 4)
        {
            coef_buffer = (short*)malloc(sizeof(char)*(length1+length2));
            fread(coef_buffer, sizeof(char), length1+length2, fcoef);

            if(ec_command(aec, EC_AEC_SET_FCOEFFSIZE, &length1) == 0)
            {
                ec_command(aec, EC_AEC_SET_FCOEFF, coef_buffer);
            }
            if(ec_command(aec, EC_BDC_SET_DELAYSIZE, &length2) == 0)
            {
                ec_command(aec, EC_BDC_SET_DELAY, &coef_buffer[length1/2]);
            }
            free(coef_buffer);
        }
        fclose(fcoef);
    }
    else
    {
        ALOGV("in %s, fopen fail! errno:%d, str:%s", __func__, errno, strerror(errno));
    }
}

static void store_aec_coef(void* aec)
{
    FILE *fcoef;
    int length1 = 0, length2 = 0;
    short* coef_buffer;
    int ret1, ret2;
    ret1 = ec_command(aec, EC_AEC_GET_FCOEFFSIZE, &length1);
    ret2 = ec_command(aec, EC_BDC_GET_DELAYSIZE, &length2);
    if((fcoef = fopen(aec_coef_file, "wb+")) != NULL && ret1 == 0 && ret2 == 0)
    {
        fwrite(&length1, sizeof(int), 1, fcoef);
        fwrite(&length2, sizeof(int), 1, fcoef);
        coef_buffer = (short*)malloc(length1+length2);
        ec_command(aec, EC_AEC_GET_FCOEFF, coef_buffer);
        ec_command(aec, EC_BDC_GET_DELAY, &coef_buffer[length1/2]);
        fwrite(coef_buffer, 1, length1+length2, fcoef);
        free(coef_buffer);
        fclose(fcoef);
    }
    else
    {
        ALOGV("in %s, ret:%d, fopen fail! errno:%d, str:%s", __func__, ret2, errno, strerror(errno));
    }
}
#endif

//static char echo_buffer[2048];
static int aec_fill_echo_buffer(AecBuffManager *buffer_manager, const void *buffer, uint32_t size)
{
    //AecBuffManager* buffer_manager = aec->bufManager;
    uint32_t buf_end =
        (uint32_t)buffer_manager->buffers + buffer_manager->totalSize;

    ALOGV("%s, size(%d)", __func__, size);

    pthread_mutex_lock(&buffer_manager->mutex);
    if (buffer_manager->availSize < size) {
        buffer_manager->user += size - buffer_manager->availSize;
        if (buffer_manager->user >= buf_end) {
            buffer_manager->user = (uint32_t)buffer_manager->buffers +
                (buffer_manager->user - buf_end);
        }
        buffer_manager->availSize = size;
        //pthread_mutex_unlock(&buffer_manager->mutex);
        ALOGW("%s: buffer overflow", __func__);
        //return -1;
    }

    if (buffer_manager->server + size > buf_end) {
        ALOGD("aec_fill_echo_buffer: buffer reach the boundary");
        uint32_t write_size = buf_end - buffer_manager->server;
        memcpy((void *)buffer_manager->server, buffer, write_size);
        memcpy(buffer_manager->buffers, (char *)buffer + write_size,
            size - write_size);
        buffer_manager->server =
            (uint32_t)buffer_manager->buffers + size - write_size;
    } else {
        memcpy((void *)buffer_manager->server, buffer, size);
        buffer_manager->server += size;
        if (buffer_manager->server == buf_end) {
            buffer_manager->server = (uint32_t)buffer_manager->buffers;
        }
    }

    buffer_manager->availSize -= size;
    pthread_mutex_unlock(&buffer_manager->mutex);
    return 0;
}

static int aec_get_echo_buffer(AecBuffManager *buffer_manager, void *buffer, uint32_t size)
{
    //AecBuffManager* buffer_manager = aec->bufManager;
    uint32_t buf_end =
        (uint32_t)buffer_manager->buffers + buffer_manager->totalSize;
    uint32_t used_size;

    ALOGV("%s", __func__);

    pthread_mutex_lock(&buffer_manager->mutex);
    used_size = buffer_manager->totalSize - buffer_manager->availSize;
    if (used_size < size) {
        ALOGV("%s: has not enough buffer.", __func__);
        pthread_mutex_unlock(&buffer_manager->mutex);
        return -1;
    }

    if (buffer_manager->user + size > buf_end) {
        ALOGD("aec_get_echo_buffer: buffer reach the boundary");
        uint32_t read_size = buf_end - buffer_manager->user;
        memcpy(buffer, (void *)buffer_manager->user, read_size);
        memcpy((char *)buffer + read_size, buffer_manager->buffers,
            size - read_size);
        buffer_manager->user =
            (uint32_t)buffer_manager->buffers + size - read_size;
    } else {
        memcpy(buffer, (void *)buffer_manager->user, size);
        buffer_manager->user += size;
        if (buffer_manager->user == buf_end) {
            buffer_manager->user = (uint32_t)buffer_manager->buffers;
        }
    }

    buffer_manager->availSize += size;
    pthread_mutex_unlock(&buffer_manager->mutex);
    return 0;
}

static int aec_flush_echo_buffer(AecBuffManager *buffer_manager)
{
    //AecBuffManager* buffer_manager = aec->bufManager;
    pthread_mutex_lock(&buffer_manager->mutex);
    buffer_manager->availSize = buffer_manager->totalSize;
    buffer_manager->user      = (uint32_t)buffer_manager->buffers;
    buffer_manager->server    = (uint32_t)buffer_manager->buffers;
    pthread_mutex_unlock(&buffer_manager->mutex);

    return 0;
}

static int aec_check_echo_buffer(AecBuffManager *buffer_manager, uint32_t size)
{
    int ret;
    //AecBuffManager* buffer_manager = aec->bufManager;
    pthread_mutex_lock(&buffer_manager->mutex);
    ret = ((buffer_manager->totalSize - buffer_manager->availSize) < size)? 0 : 1;
    pthread_mutex_unlock(&buffer_manager->mutex);
    return ret;

}

static int aec_buffer_manager_create(AecBuffManager** buffer_manager, int total_size)
{
    AecBuffManager *state;

    state = (AecBuffManager *)calloc(1, sizeof(AecBuffManager));
    if (state) {
        state->totalSize = total_size;
        state->buffers = (char *)malloc(state->totalSize);
        if (state->buffers) {
            state->availSize = state->totalSize;
            state->user = (uint32_t)state->buffers;
            state->server = (uint32_t)state->buffers;
            pthread_mutex_init(&state->mutex, NULL);
            *buffer_manager = state;
            return 0;
        }
        free(state);
    }
    return -1;
}

static int aec_buffer_manager_free(AecBuffManager* buffer_manager)
{
    if (buffer_manager) {
        pthread_mutex_lock(&buffer_manager->mutex);
        if (buffer_manager->buffers) {
            free(buffer_manager->buffers);
            //buffer_manager->buffers = NULL;
        }
        pthread_mutex_unlock(&buffer_manager->mutex);

        pthread_mutex_destroy(&buffer_manager->mutex);
        free(buffer_manager);
        return 0;
    }
    return -1;
}

static int aec_process_internal(AecContext *aec, void *inBuffer, void *outBuffer,
                uint32_t size)
{
    uint32_t i = 0;
    int slice_size;

    ALOGV("%s", __func__);

    if (aec->isInited) {
        pthread_mutex_lock(&aec->mutex);
        slice_size = aec->sliceSize;
        if(size % (2 * slice_size) || size > aec->echoBuffSize) {
            ALOGE("size is %d", size);
            pthread_mutex_unlock(&aec->mutex);
            return -1;
        }

        if (aec_get_echo_buffer(aec->bufManager, aec->echoBuffer, size) < 0) {
            pthread_mutex_unlock(&aec->mutex);
            return -1;
        }

#ifdef AEC_DUMP
        if (pcmEcho) {
            fwrite(aec->echoBuffer, 1, size, pcmEcho);
        }
        if (pcmIn) {
            fwrite(inBuffer, 1, size, pcmIn);
        }
#endif
        ALOGV("ec_process_frame");
        for(i = 0; i < size/(2 * slice_size); i++) {
            ec_process_frame(aec->handle,
                             (short *)inBuffer + i*slice_size,
                             (short *)aec->echoBuffer + i*slice_size,
                             (short *)outBuffer + i*slice_size,
                             NULL);
        }
        aec->hasAec = 1;
#ifdef AEC_DUMP
        if (pcmEcOut) {
            fwrite(outBuffer, 1, size, pcmEcOut);
        }
#endif
        pthread_mutex_unlock(&aec->mutex);

        return 0;
    }
    return -1;
}

static void aec_set_prms(ec_prms_t* aec_prms, uint32_t sample_rate)
{
    static eq_core_prms_t eq_core_prms[1]=
    {
        {-10, 6300, 4, BANDPASS_PEAK},
    };

    ALOGV("aec param g_max_delay:%d, g_near_delay:%d, g_tail_length:%d, g_save_coef:%d",
            g_max_delay, g_near_delay, g_tail_length, g_save_coef);
    ALOGV("g_delay_margin:%d, g_max_suppression:%d", g_delay_margin, g_max_suppression);
    memset(aec_prms, 0, sizeof(aec_prms));
    /* create echo controller handle */
    /* common init prms */
    aec_prms->enable_cdc = 0;
    aec_prms->enable_aec = 1;
    aec_prms->aec_prms.sampling_rate = sample_rate;
    aec_prms->aec_prms.frame_size = EC_FRAME_SIZE / 8;
    aec_prms->aec_prms.tail_length = g_tail_length; //128ms
    aec_prms->aec_prms.enable_nlp = 1;
    aec_prms->aec_prms.nlp_prms.nlp_overlap_percent =  OVERLAP_FIFTY; //OVERLAP_SEVENTY_FIVE;
    /* tx drc init prms */
    aec_prms->enable_txdrc = 1;
    aec_prms->txdrc_prms.sampling_rate = sample_rate;
    aec_prms->txdrc_prms.attack_time = 1;
    aec_prms->txdrc_prms.max_gain = 6;
    aec_prms->txdrc_prms.min_gain = -9;
    aec_prms->txdrc_prms.noise_threshold = -45;
    aec_prms->txdrc_prms.release_time = 100;
    aec_prms->txdrc_prms.target_level = -3;

    /* tx eq init prms */
    aec_prms->enable_txeq = 1;
    aec_prms->txeq_prms.biq_num = 1;
    aec_prms->txeq_prms.sampling_rate = sample_rate;
    aec_prms->txeq_prms.core_prms = eq_core_prms;

    /* bulk delay estimation prms */
    aec_prms->enable_bdc = 1;
    aec_prms->bdc_prms.sampling_rate = sample_rate;
    aec_prms->bdc_prms.block_size = aec_prms->bdc_prms.sampling_rate * 8 / 1000; //8ms
    aec_prms->bdc_prms.delay_margin = g_delay_margin;
    aec_prms->bdc_prms.max_delay = g_max_delay; // 375; //40 50 60
    aec_prms->bdc_prms.near_delay = g_near_delay; //12; //12 24

    /* noise suppression prms */
    aec_prms->enable_ns = 1;
    aec_prms->ns_prms.sampling_rate = sample_rate;
    aec_prms->ns_prms.max_suppression = g_max_suppression; //-21;
    aec_prms->ns_prms.nonstat = LOW_NONSTATIONAL;
    aec_prms->ns_prms.overlap_percent = OVERLAP_FIFTY; //OVERLAP_FIFTY;

    aec_prms->enable_txfade = 1;
}


static int aec_init(AecContext *aec, int sampleRate, int channels,
        int max_frame_size)
{
    ec_prms_t aec_prms;
    int frameSize20Ms, ret;

    ALOGI("%s", __func__);

    if (aec->isInited) {
        ALOGW("aec has already inited.");
        //aec_deinit();
        return -1;
    }

    if (channels != 1 || (sampleRate != 8000 && sampleRate != 16000)) {
        return -1;
    }

    aec->sampleRate = sampleRate;
    //aec->channels = channels;
    aec->sliceSize = EC_FRAME_SIZE * sampleRate / 8000;

    frameSize20Ms = 20 * sizeof(int16_t) * sampleRate / 1000; //Neteq every pkt is 10 ms
    ret = aec_buffer_manager_create(&aec->bufManager, EC_MAX_PACKET * frameSize20Ms);
    if (ret < 0) {
        return -1;
    }

    aec->echoBuffSize = max_frame_size * sizeof(int16_t);
    aec->echoBuffer = (char *)malloc(aec->echoBuffSize);
    if (!aec->echoBuffer) {
        aec_buffer_manager_free(aec->bufManager);
        aec->bufManager = NULL;
        return -1;
    }

    aec_set_prms(&aec_prms, aec->sampleRate);
    aec->handle = ec_create(&aec_prms);
    if (!aec->handle) {
        free(aec->echoBuffer);
        aec_buffer_manager_free(aec->bufManager);
        aec->bufManager = NULL;
        return -1;
    }

    pthread_mutex_init(&aec->mutex, NULL);

#ifdef AEC_USE_COEF
    load_aec_coef(aec->handle);
#endif
    aec->hasAec = 0;
    aec->isInited = 1;

#ifdef AEC_DUMP
    pcmEcho = fopen(ECHO_FILE, "w+");
    pcmIn = fopen(IN_FILE, "w+");
    pcmEcOut = fopen(EC_OUT_FILE, "w+");
#endif

    return 0;
}

static int aec_deinit(AecContext *aec)
{
    ALOGI("%s", __func__);

    if (aec && aec->isInited) {
        aec->isInited = 0;
        pthread_mutex_lock(&aec->mutex);
        if (aec->handle) {
        #ifdef AEC_USE_COEF
            store_aec_coef(aec->handle);
        #endif
            ec_destroy(aec->handle);
            aec->handle = NULL;
        }
        if (aec->echoBuffer) {
            free(aec->echoBuffer);
            aec->echoBuffer = NULL;
        }
        if (aec->bufManager) {
            aec_buffer_manager_free(aec->bufManager);
            aec->bufManager = NULL;
        }
        pthread_mutex_unlock(&aec->mutex);

        usleep(2);
        pthread_mutex_destroy(&aec->mutex);
#ifdef AEC_DUMP
        if (pcmEcho) {
            fclose(pcmEcho);
            pcmEcho = NULL;
        }
        if (pcmIn) {
            fclose(pcmIn);
            pcmIn = NULL;
        }
        if (pcmEcOut) {
            fclose(pcmEcOut);
            pcmEcOut = NULL;
        }
#endif
        return 0;
    }
    return -1;
}

#if 0
int aec_create(AecContext **aec)
{
    AecContext *state;

    if (aec) {
        state = (AecContext *)calloc(1, sizeof(AecContext));
        if (!state) {
            return -1;
        }
        pthread_mutex_init(&aec->mutex, NULL);

        *aec = state;
        return 0;
    }
    return -1;
}

int aec_destroy(AecContext *aec)
{
    AecContext *state;

    if (aec) {
        //echo_deinit(aec);
        pthread_mutex_destroy(&aec->mutex);
        free(aec);
        return 0;
    }
    return -1;
}
#endif

//API

#define AEC_MAX_FRAME_SIZE   (1280)
static AecContext gAec = {
    .isInited           = 0,
    .hasAec             = 0,
};
static AecStreamState gPlayerState = STREAM_STOPED;
static int isRecStarted = 0;
/*
static AecStreamCtx playerCtx = {
    .state          = STOPED,
    .sampleRate     = 8000,
    .channnel       = 1,
};
static AecStreamCtx recorderCtx = {
    .state          = STOPED,
    .sampleRate     = 8000,
    .channnel       = 1,
};
*/
int aec_recorder_check_state(uint32_t size)
{
    AecContext *aec = &gAec;

    if (!isRecStarted) {
        return -1;
    }

    if (aec->isInited) {
        if (aec->hasAec) {
            #define MAX_LOOP_TIME 8         /*wait 35ms*/
            int loopTime = 0;
            while (loopTime < MAX_LOOP_TIME) {
                if (aec_check_echo_buffer(aec->bufManager, size) > 0) {
                    ALOGV("aec_check_echo_state: loopTime(%d)", loopTime);
                    return 0;
                }
                usleep(5 * 1000);   //5ms
                loopTime++;
            }
            ALOGD("aec_check_echo_state: loopTime(%d)", loopTime);
        } else if (aec_check_echo_buffer(aec->bufManager, size) > 0) {
            return 0;
        }

        if(gPlayerState == STREAM_STOPED) {
            aec_deinit(aec);
        }
        //return -1;
    } else if (gPlayerState == STREAM_STARTED) {
        aec_init(aec, aec->sampleRate, 1, AEC_MAX_FRAME_SIZE);
        //return -1;
    }

    return -1;
}

int aec_recorder_process(void *inBuffer, void *outBuffer, uint32_t size)
{
    AecContext *aec = &gAec;
/*
    if(gPlayerState == STARTED && !aec->isInited) {
        aec_init(aec, aec->sampleRate, aec->channels, AEC_MAX_FRAME_SIZE);
    } else if(gPlayerState == STOPED && aec->isInited) {
        aec_deinit(aec);
        return -1;
    }
*/
    if (!isRecStarted || !inBuffer || !outBuffer) {
        //ALOGE("aec_recorder_process: %d, %p, %p", isRecStarted, inBuffer, outBuffer);
        return -1;
    }

    if (aec_recorder_check_state(size) < 0) {
        return -1;
    }

    return aec_process_internal(aec, inBuffer, outBuffer, size);
}

int aec_recorder_start(int sampleRate, int channels)
{
    AecContext *aec = &gAec;

    if (channels != 1 || (sampleRate != 8000 && sampleRate != 16000)) {
        ALOGE("aec_recorder_start: channels(%d), sampleRate(%d)", channels, sampleRate);
        return -1;
    }

    aec->sampleRate = sampleRate;
    //aec->channels = channels;

    isRecStarted = 1;

    if (gPlayerState == STREAM_STARTED) {
        return aec_init(aec, sampleRate, 1, AEC_MAX_FRAME_SIZE);
    }

    return 0;
}

int aec_recorder_stop()
{
    AecContext *aec = &gAec;

    isRecStarted = 0;

    if (aec->isInited) {
        aec_deinit(aec);
    }

    return 0;
}

int aec_player_add_echo_data(const void *buffer, uint32_t size)
{
    AecContext *aec = &gAec;

    if (aec->isInited) {
        aec_fill_echo_buffer(aec->bufManager, buffer, size);
        return 0;
    }

    return -1;
}

//int aec_player_set_state(PlayerState state, int sampleRate, int channels)
void aec_player_set_state(AecStreamState state)
{
    gPlayerState = state;
}

