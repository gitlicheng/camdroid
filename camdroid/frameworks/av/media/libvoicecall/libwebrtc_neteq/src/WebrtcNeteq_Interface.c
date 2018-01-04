/*
 * Author: yewenliang@allwinnertech.com
 * Copyright (C) 2014 Allwinner Technology Co., Ltd.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "WebrtcNeteq_Interface"
//#include <utils/Log.h>
#include "dbg_log.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

#include "modules/audio_coding/codecs/amrnb/include/amrnb_interface.h"
#include "modules/audio_coding/codecs/g726/include/g726_interface.h"
#include "modules/audio_coding/codecs/opus/interface/opus_interface.h"
#include "modules/audio_coding/neteq/interface/webrtc_neteq.h"
#include "modules/audio_coding/neteq/interface/webrtc_neteq_help_macros.h"
#include "modules/audio_coding/neteq/interface/webrtc_neteq_internal.h"
#include "typedefs.h"
#include "WebrtcNeteq_Interface.h"

/*
#include <utils/threads.h>
//#include <pthread.h>    //jni
enum NeteqThreadState {
    Exited    = 0,
    Executing = 1,
};

typedef struct NeteqBufferManager {
    char *                  buffers;
    uint32_t                total_size;
    uint32_t                avail_size;
    volatile    uint32_t    user;
    volatile    uint32_t    server;
    pthread_mutex_t         mutex;
}NeteqBufferManager_t;
*/

typedef struct NeteqContext_t {
    void *                  neteqInst;
    void *                  neteqInstMem;
    int16_t *               packetBuffer;
    void *                  decoderInst;
    enum WebRtcNetEQDecoder decoderType;
    int                     payloadType;
    int                     sampleRate;
    int                     channels;
    //int                     bitRate;
    int                     decoderMode;
    int                     waitFirstPacket;
    int                     avSync;

    int                     RecOutSyncTime;         //unit: us
    int64_t                 lastRecOutSyncTime;     //unit: us
/*
    NeteqBufferManager_t    bufferManager;
    pthread_mutex_t         mutex;
    pthread_cond_t          cond;
    pthread_t               thread_id;
    int                     forceExit;
    enum NeteqThreadState   threadState;
*/
} NeteqContext;

//static const int kMaxChannels = 1;
//static const int kMaxSamplesPerMs = 48000 / 1000;
//static const int kOutputBlockSizeMs = 20;
//static const int kPacketTimeMs = 20;
static const uint32_t kMaskTimestamp = 0x03ffffff;

//static void* WebrtcNeteqRecOutThread(void *arg);
//int WebrtcNeteq_bufferInit(NeteqContext *pNeteqCtx);
//int WebrtcNeteq_bufferDeInit(NeteqContext *pNeteqCtx);

static uint32_t NowTimestamp(int sampleRate)
{
    int sample_rate_khz = sampleRate / 1000;
#if 1
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t timeMs = (1000000LL * (int64_t)(tv.tv_sec) +
                (int64_t)(tv.tv_usec)) / 1000LL;
#else
//    struct timespec ts;
//    //clock_gettime(CLOCK_REALTIME, &ts);
//    clock_gettime(CLOCK_MONOTONIC, &ts);
//    int64_t timeMs = (1000000000LL * static_cast<int64_t>(ts.tv_sec) +
//                static_cast<int64_t>(ts.tv_nsec)) / 1000000LL;
    int64_t timeMs = systemTime() / 1000000LL;
#endif
    const uint32_t now_in_ms = (uint32_t)(timeMs & kMaskTimestamp);

    return (uint32_t)(sample_rate_khz * now_in_ms);
}

static int64_t getTimeUs(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000000LL + tv.tv_usec;
}

static enum WebRtcNetEQDecoder DecoderType2WebRtcNetEQDecoder(DecoderType type,
        int sampleRate, int mode)
{
    enum WebRtcNetEQDecoder decoder_type = kDecoderReservedStart;

    switch (type) {
    case DecoderReservedStart:
        decoder_type = kDecoderReservedStart;
        break;
    case DecoderAMR:
        decoder_type = kDecoderAMR;
        break;
    case DecoderOpus:
        if (sampleRate == 8000) {
            decoder_type = kDecoderOpus_8;
        } else {
            decoder_type = kDecoderOpus_16;
        }
        break;
    case DecoderG726:
        switch (mode) {
        case G726_16:
            decoder_type = kDecoderG726_16;
            break;
        case G726_24:
            decoder_type = kDecoderG726_24;
            break;
        case G726_32:
            decoder_type = kDecoderG726_32;
            break;
        case G726_40:
            decoder_type = kDecoderG726_40;
            break;
        default:
            decoder_type = kDecoderReservedEnd;
            break;
        }
        break;
    case DecoderReservedEnd:
        decoder_type = kDecoderReservedEnd;
        break;
    default:
        decoder_type = kDecoderReservedEnd;
        break;
    }

    return decoder_type;
}

static int WebrtcNeteqAddCodec(NeteqContext *pNeteqCtx)
{
    int error;
    enum WebRtcNetEQDecoder decoderType = pNeteqCtx->decoderType;
    int sampleRate = pNeteqCtx->sampleRate;
    int channels = pNeteqCtx->channels;
    //int bitRate = pNeteqCtx->bitRate;
    int mode = pNeteqCtx->decoderMode;
/*
    if (decoderType == kDecoderReservedStart
       || decoderType == kDecoderReservedEnd) {
        ALOGE("WebrtcNeteq not support decoderType(%d)", decoderType);
        return -1;
    }
*/
    WebRtcNetEQ_CodecDef codec_definition;
#ifdef USE_CODEC_AMR
    // Register amr decoder.
    if (decoderType == kDecoderAMR) {
        if (sampleRate != 8000 || channels != 1) {
            ALOGE("AMR don't the audio format: sampleRate(%d), channels(%d)",
                sampleRate, channels);
            return -1;
        }

        pNeteqCtx->payloadType = PAYLOAD_TYPE_AMR;
        if (WebRtcAmr_CreateDec((AMR_decinst_t_ **)&pNeteqCtx->decoderInst, mode) < 0)
        {
            ALOGE("Error returned from WebRtcAmr_CreateDec.");
            return -1;
        }

        SET_CODEC_PAR(codec_definition, pNeteqCtx->decoderType,
            pNeteqCtx->payloadType, pNeteqCtx->decoderInst, sampleRate);

        SET_AMR_FUNCTIONS(codec_definition);
    }
#endif
#ifdef USE_CODEC_OPUS
    //regitster opus decoder
    if (decoderType == kDecoderOpus_8 || decoderType == kDecoderOpus_16) {
        if (WebRtcOpus_DecoderCreate((OpusDecInst **)&pNeteqCtx->decoderInst,
                                    sampleRate, channels) < 0) {
            ALOGE("Error returned from WebRtcAmr_CreateDec.");
            return -1;
        }

        pNeteqCtx->payloadType = PAYLOAD_TYPE_OPUS;
        SET_CODEC_PAR(codec_definition, pNeteqCtx->decoderType,
            pNeteqCtx->payloadType, pNeteqCtx->decoderInst, sampleRate);

        SET_OPUS_FUNCTIONS(codec_definition);
    }
#endif
#ifdef USE_CODEC_G726
    //regitster g726 decoder
    if (decoderType == kDecoderG726_16 || decoderType == kDecoderG726_24
        || decoderType == kDecoderG726_32 || decoderType == kDecoderG726_40) {
        if (WebRtcG726_CreateDecoder((G726Inst **)&pNeteqCtx->decoderInst,
                                    mode) < 0) {	//bitRate
            ALOGE("Error returned from WebRtcG726_CreateDecoder.");
            return -1;
        }

        pNeteqCtx->payloadType = PAYLOAD_TYPE_G726;
        SET_CODEC_PAR(codec_definition, pNeteqCtx->decoderType,
            pNeteqCtx->payloadType, pNeteqCtx->decoderInst, sampleRate);

        SET_G726_FUNCTIONS(codec_definition);
    }
#endif
    error = WebRtcNetEQ_CodecDbAdd(pNeteqCtx->neteqInst, &codec_definition);
    if (error) {
        ALOGE("Cannot register decoder.");
        goto FreeDec;
    }

    return 0;
FreeDec:
#ifdef USE_CODEC_AMR
    if (decoderType == kDecoderAMR) {
        WebRtcAmr_FreeDec((AMR_decinst_t_ *)pNeteqCtx->decoderInst);
    }
#endif
#ifdef USE_CODEC_OPUS
    if (decoderType == kDecoderOpus_8
        || decoderType == kDecoderOpus_16) {
        WebRtcOpus_DecoderFree((OpusDecInst *)pNeteqCtx->decoderInst);
    }
#endif
#ifdef USE_CODEC_G726
    if (decoderType == kDecoderG726_16 || decoderType == kDecoderG726_24
        || decoderType == kDecoderG726_32 || decoderType == kDecoderG726_40) {
        WebRtcG726_FreeDecoder((G726Inst *)pNeteqCtx->decoderInst);
    }
#endif
    return -1;
}

static int WebrtcNeteqRemoveCodec(NeteqContext *pNeteqCtx)
{
#ifdef USE_CODEC_AMR
    if (pNeteqCtx->decoderType == kDecoderAMR) {
        WebRtcAmr_FreeDec((AMR_decinst_t_ *)pNeteqCtx->decoderInst);
    }
#endif
#ifdef USE_CODEC_OPUS
    if (pNeteqCtx->decoderType == kDecoderOpus_8
       || pNeteqCtx->decoderType == kDecoderOpus_16) {
        WebRtcOpus_DecoderFree((OpusDecInst *)pNeteqCtx->decoderInst);
    }
#endif
#ifdef USE_CODEC_G726
    if (pNeteqCtx->decoderType == kDecoderG726_16
        || pNeteqCtx->decoderType == kDecoderG726_24
        || pNeteqCtx->decoderType == kDecoderG726_32
        || pNeteqCtx->decoderType == kDecoderG726_40) {
        WebRtcG726_FreeDecoder((G726Inst *)pNeteqCtx->decoderInst);
    }
#endif
    pNeteqCtx->decoderInst = NULL;
    if (WebRtcNetEQ_CodecDbRemove(pNeteqCtx->neteqInst,
       pNeteqCtx->decoderType) < 0) {
        ALOGE("CodecDB_Remove Error.");
    }

    return 0;
}

int WebrtcNeteqInit(void **state, NeteqParam *params/*, int avSync*/)
{
    // Initialize NetEq instance.
    int error;
    int inst_size_bytes;
    NeteqContext * ctx_inst;
    enum WebRtcNetEQDecoder decoderType;

    if (params->channels != 1) {
        ALOGE("now only support 1ch, but channels is %d",params->channels);
        return -1;
    }

    ctx_inst = (NeteqContext *)calloc(1, sizeof(NeteqContext));
    if (ctx_inst == NULL) {
        ALOGE("create NeteqContext inst failed.");
        return -1;
    }

    error = WebRtcNetEQ_AssignSize(&inst_size_bytes);
    if (error) {
        ALOGE("Error returned from WebRtcNetEQ_AssignSize.");
        goto fail1;
    }

    //ctx_inst->neteqInstMem = new char[inst_size_bytes];
    ctx_inst->neteqInstMem = malloc(inst_size_bytes);
    error = WebRtcNetEQ_Assign(&ctx_inst->neteqInst, ctx_inst->neteqInstMem);
    if (error) {
        ALOGE("Error returned from WebRtcNetEQ_Assign.");
        //delete [] ctx_inst->neteqInstMem;
        goto fail2;
    }

    decoderType = DecoderType2WebRtcNetEQDecoder(params->type, params->sampleRate,
                        params->mode);	//parms.bitRate

    // Select decoders.
    int max_number_of_packets;
    int buffer_size_bytes;
    int overhead_bytes_dummy;
    error = WebRtcNetEQ_GetRecommendedBufferSize(ctx_inst->neteqInst,
        &decoderType, 1, kTCPXLargeJitter, &max_number_of_packets,
        &buffer_size_bytes, &overhead_bytes_dummy);
    if (error) {
        ALOGE("Error returned from WebRtcNetEQ_GetRecommendedBufferSize.");
        goto fail2;
    }

    //ctx_inst->packetBuffer = new char[buffer_size_bytes];
    ctx_inst->packetBuffer = (int16_t *)malloc(buffer_size_bytes);
    error = WebRtcNetEQ_AssignBuffer(ctx_inst->neteqInst,
        max_number_of_packets, ctx_inst->packetBuffer, buffer_size_bytes);
    if (error) {
        ALOGE("Error returned from WebRtcNetEQ_AssignBuffer.");
        goto fail3;
    }

    error = WebRtcNetEQ_Init(ctx_inst->neteqInst, params->sampleRate);
    if (error) {
        ALOGE("Error returned from WebRtcNetEQ_Init.");
        goto fail3;
    }

    //parms
    ctx_inst->decoderType = decoderType;
    ctx_inst->sampleRate  = params->sampleRate;
    ctx_inst->channels    = params->channels;
    //ctx_inst->bitRate     = params->bitRate;
    ctx_inst->decoderMode = params->mode;

    error = WebrtcNeteqAddCodec(ctx_inst);
    if (error) {
        ALOGE("Error returned form WebrtcNeteqAddCodec");
        goto fail3;
    }

    ctx_inst->avSync      = 0;//avSync;
    //modify for av sync
    WebRtcNetEQ_EnableAVSync(ctx_inst->neteqInst, ctx_inst->avSync);   //av sync
    if (ctx_inst->avSync) {
        // Minimum playout delay.
        WebRtcNetEQ_SetMinimumDelay(ctx_inst->neteqInst, 20); //0
        // Maximum playout delay.
        WebRtcNetEQ_SetMaximumDelay(ctx_inst->neteqInst, 800); //0
    } else {
        // Minimum playout delay.
        WebRtcNetEQ_SetMinimumDelay(ctx_inst->neteqInst, 0); //0
        // Maximum playout delay.
        //WebRtcNetEQ_SetMaximumDelay(ctx_inst->neteqInst, 1500); //0
    }

    ctx_inst->waitFirstPacket = 1;
/*
    error = WebrtcNeteq_bufferInit(ctx_inst);
    if (error) {
        ALOGE("Error returned form WebrtcNeteq_bufferInit");
        goto fail4;
    }

    pthread_mutex_init(&ctx_inst->mutex, NULL);
    pthread_cond_init(&ctx_inst->cond, NULL);

    ctx_inst->forceExit = 0;
    pthread_create(&ctx_inst->thread_id, NULL, WebrtcNeteqRecOutThread, ctx_inst);
*/
    *state = (void *)ctx_inst;
    return 0;

fail4:
    WebrtcNeteqRemoveCodec(ctx_inst);
fail3:
    //delete [] ctx_inst->neteqInstMem;
    free(ctx_inst->neteqInstMem);
fail2:
    //delete [] ctx_inst->packetBuffer;
    free(ctx_inst->packetBuffer);
fail1:
    free(ctx_inst);
    return -1;
}

int WebrtcNeteqDeInit(void *state)
{
    NeteqContext *pNeteqCtx = (NeteqContext *)state;

    if (pNeteqCtx) {
/*
        pNeteqCtx->forceExit = 1;
        if (pNeteqCtx->threadState == Executing) {
            pthread_mutex_lock(&pNeteqCtx->mutex);
            pthread_cond_signal(&pNeteqCtx->cond);
            pthread_mutex_unlock(&pNeteqCtx->mutex);
            pthread_join(pNeteqCtx->thread_id, NULL);
        }
*/
        WebrtcNeteqRemoveCodec(pNeteqCtx);

        if (pNeteqCtx->neteqInstMem) {
            //delete [] pNeteqCtx->neteqInstMem;
            free(pNeteqCtx->neteqInstMem);
            pNeteqCtx->neteqInstMem = NULL;
        }
        if (pNeteqCtx->packetBuffer) {
            //delete [] pNeteqCtx->packetBuffer;
            free(pNeteqCtx->packetBuffer);
            pNeteqCtx->packetBuffer = NULL;
        }
//        WebrtcNeteq_bufferDeInit(pNeteqCtx);
//        pthread_mutex_destroy(&pNeteqCtx->mutex);
//        pthread_cond_destroy(&pNeteqCtx->cond);
        return 0;
    }
    return -1;
}

int WebrtcNeteqRecIn(void *state, PacketInfo *info)
{
    NeteqContext *pNeteqCtx = (NeteqContext *)state;

    int sample_rate_khz = pNeteqCtx->sampleRate / 1000;
    int64_t last_receive_timestamp = NowTimestamp(pNeteqCtx->sampleRate);

    //generator rtp info
    WebRtcNetEQ_RTPInfo rtp_info;
    rtp_info.payloadType = pNeteqCtx->payloadType;
    rtp_info.sequenceNumber = info->sequenceNumber;
    rtp_info.timeStamp = info->timeStamp * sample_rate_khz;
    rtp_info.SSRC = 0x12345678;
    rtp_info.markerBit = 0;

    int error = WebRtcNetEQ_RecInRTPStruct(pNeteqCtx->neteqInst, &rtp_info,
            info->payloadPtr, info->payloadLenBytes, last_receive_timestamp);
    if (error != 0) {
        ALOGE("WebRtcNetEQ_RecInRTPStruct returned error code(%d)",
            WebRtcNetEQ_GetErrorCode(pNeteqCtx->neteqInst));
        return -1;
    }

    if (pNeteqCtx->waitFirstPacket) {
        //pthread_mutex_lock(&pNeteqCtx->mutex);
        //pthread_cond_signal(&pNeteqCtx->cond);
        //pthread_mutex_unlock(&pNeteqCtx->mutex);
        pNeteqCtx->lastRecOutSyncTime = getTimeUs();
        pNeteqCtx->waitFirstPacket = 0;
    }

    return 0;
}

#if 1
int WebrtcNeteqRecOut(void *state, int16_t *outData, int16_t *outlen)
{
    NeteqContext *pNeteqCtx = (NeteqContext *)state;
    enum WebRtcNetEQOutputType type;

    if (pNeteqCtx->waitFirstPacket) {
        //pNeteqCtx->lastRecOutSyncTime = getTimeUs();
        //pNeteqCtx->waitFirstPacket = 0;
        return -1;
    } else {
        pNeteqCtx->lastRecOutSyncTime += 10 * 1000;         //10ms
        int64_t curTimeUs = getTimeUs();
        if (curTimeUs < pNeteqCtx->lastRecOutSyncTime) {
            uint32_t sleepTime = pNeteqCtx->lastRecOutSyncTime - curTimeUs;
            usleep(sleepTime);
            //pNeteqCtx->lastRecOutSyncTime = getTimeUs();  //sync
        } else {
            ALOGV("curTimeUs is larger than last Sync Time %lld(us)",
                curTimeUs - pNeteqCtx->lastRecOutSyncTime);
        }
    }

    ALOGV("enter WebRtcNetEQ_RecOut");
    int error = WebRtcNetEQ_RecOut(pNeteqCtx->neteqInst, outData, outlen);
    if (error != 0) {
        ALOGE("WebRtcNetEQ_RecOut returned error code(%d)",
            WebRtcNetEQ_GetErrorCode(pNeteqCtx->neteqInst));
        return -1;
    }

    //WebRtcNetEQ_GetSpeechOutputType(pNeteqCtx->neteqInst, &type);

    return 0;
}

int WebrtcNeteqGetPlayoutTimestamp(void *state, uint32_t *pTimeStamp)
{
    NeteqContext *pNeteqCtx = (NeteqContext *)state;
    int sample_rate_khz = pNeteqCtx->sampleRate / 1000;

    if (WebRtcNetEQ_GetSpeechTimeStamp(pNeteqCtx->neteqInst, pTimeStamp) == 0) {
        *pTimeStamp = *pTimeStamp / sample_rate_khz;
        ALOGV("current Playout timeStamp is %d", *pTimeStamp);
        return 0;
    } else {
        ALOGE("GetSpeechTimeStamp failed.");
        return -1;
    }
}

int WebrtcNeteqGetDecodedTimeStamp(void *state, uint32_t* pTimeStamp)
{
    NeteqContext *pNeteqCtx = (NeteqContext *)state;
    int sample_rate_khz = pNeteqCtx->sampleRate / 1000;

    int seqNumber;
    uint32_t timestamp;
    if (WebRtcNetEQ_DecodedRtpInfo(pNeteqCtx->neteqInst, &seqNumber, &timestamp) == 0) {
        *pTimeStamp = timestamp / sample_rate_khz;
        ALOGV("current Decoded seqNumber: %d, timestamp: %d", seqNumber, *pTimeStamp);
        return 0;
    } else {
        ALOGE("WebRtcNetEQ_DecodedRtpInfo failed.");
        return -1;
    }
}

int WebrtcNeteqGetNetworkStatistics(void *state)
{
    NeteqContext *pNeteqCtx = (NeteqContext *)state;
    WebRtcNetEQ_NetworkStatistics stats;
    if (WebRtcNetEQ_GetNetworkStatistics(pNeteqCtx->neteqInst, &stats) == 0) {
        ALOGD("currentPacketLossRate: %d, jitterPeaksFound: %d",
            stats.currentPacketLossRate, stats.jitterPeaksFound);
    } else {
        ALOGE("getNetworkStatistics failed.");
        return -1;
    }
    return 0;
}
#else
int WebrtcNeteq_GetAudioBuffer(NeteqContext *pNeteqCtx, char *outData, uint16_t outLen)
{
    NeteqBufferManager_t *bufferManager = &pNeteqCtx->bufferManager;
    uint32_t buf_end = (uint32_t)bufferManager->buffers + bufferManager->total_size;
    //int retSize;
    int buffer_Size;

    pthread_mutex_lock(&pNeteqCtx->mutex);
    buffer_Size = bufferManager->total_size - bufferManager->avail_size;
    if (buffer_Size < outLen) {
        pthread_mutex_unlock(&pNeteqCtx->mutex);
        return -1;
    }

    if (bufferManager->user + outLen > buf_end) {
        uint32_t read_size = buf_end - bufferManager->user;
        memcpy(outData, (void *)bufferManager->user, read_size);
        memcpy((char *)outData + read_size, bufferManager->buffers, outLen - read_size);
        bufferManager->user = (uint32_t)bufferManager->buffers + outLen - read_size;
    } else {
        memcpy(outData, (void *)bufferManager->user, outLen);
        bufferManager->user += outLen;
        if (bufferManager->user == buf_end) {
            bufferManager->user = (uint32_t)bufferManager->buffers;
        }
    }

    bufferManager->avail_size += outLen;
    pthread_mutex_unlock(&pNeteqCtx->mutex);
    return 0;
}

int WebrtcNeteq_fillAudioBuffer(NeteqContext *pNeteqCtx, char *inData, uint16_t inLen)
{
    NeteqBufferManager_t *bufferManager = &pNeteqCtx->bufferManager;
    uint32_t buf_end = (uint32_t)bufferManager->buffers + bufferManager->total_size;

//    ALOGV("%s", __func__);

    pthread_mutex_lock(&bufferManager->mutex);
    if (bufferManager->avail_size < inLen) {
/*
        bufferManager->user += inLen - bufferManager->avail_size;
        if (bufferManager->user >= buf_end) {
            bufferManager->user = (uint32_t)bufferManager->buffers +
                                (bufferManager->user - buf_end);
        }
        bufferManager->avail_size = inLen;
*/
        pthread_mutex_unlock(&bufferManager->mutex);
        ALOGE("%s: buffer overflow", __func__);
        return -1;
    }

    if (bufferManager->server + inLen > buf_end) {
        uint32_t write_size = buf_end - bufferManager->server;
        memcpy((void *)bufferManager->server, inData, write_size);
        memcpy(bufferManager->buffers, (char *)inData + write_size, inLen - write_size);
        bufferManager->server = (uint32_t)bufferManager->buffers + inLen - write_size;
    } else {
        memcpy((void *)bufferManager->server, inData, inLen);
        bufferManager->server += inLen;
        if (bufferManager->server == buf_end) {
            bufferManager->server = (uint32_t)bufferManager->buffers;
        }
    }

    bufferManager->avail_size -= inLen;
    pthread_mutex_unlock(&bufferManager->mutex);
    return 0;
}

int WebrtcNeteq_bufferFlush(NeteqContext *pNeteqCtx)
{
    NeteqBufferManager_t *bufferManager = &pNeteqCtx->bufferManager;
    pthread_mutex_lock(&bufferManager->mutex);
    bufferManager->user = (uint32_t)bufferManager->buffers;
    bufferManager->server = (uint32_t)bufferManager->buffers;
    bufferManager->avail_size = bufferManager->total_size;
    pthread_mutex_unlock(&bufferManager->mutex);
    return 0;
}

int WebrtcNeteq_bufferInit(NeteqContext *pNeteqCtx)
{
    NeteqBufferManager_t *bufferManager = &pNeteqCtx->bufferManager;
    int size = 50 * 10 * pNeteqCtx->sampleRate * sizeof(int16_t) / 1000;  //500ms
    bufferManager->buffers = (char *)malloc(size);
    if (!bufferManager->buffers) {
        return -1;
    }
    bufferManager->total_size = size;
    pthread_mutex_init(&bufferManager->mutex, NULL);
    WebrtcNeteq_bufferFlush(pNeteqCtx);
    return 0;
}

int WebrtcNeteq_bufferDeInit(NeteqContext *pNeteqCtx)
{
    NeteqBufferManager_t *bufferManager = &pNeteqCtx->bufferManager;
    if (bufferManager->buffers) {
        free(bufferManager->buffers);
        bufferManager->buffers = NULL;
    }
    pthread_mutex_destroy(&bufferManager->mutex);
    return 0;
}

static void* WebrtcNeteqRecOutThread(void *arg)
{
    NeteqContext *pNeteqCtx = (NeteqContext *)arg;
    //enum WebRtcNetEQOutputType type;
    int16_t samples_per_channel;
    int frameSize = sizeof(int16_t);
    int size10ms = 10 * pNeteqCtx->sampleRate * frameSize / 1000;
    char *buffer10ms = (char *)malloc(size10ms);
    if (!buffer10ms) {
        ALOGE("malloc buffer10ms failed.");
        pNeteqCtx->threadState = Exited;
        return NULL;
    }
    pNeteqCtx->threadState = Executing;

    while (!pNeteqCtx->forceExit) {
        if (pNeteqCtx->waitFirstPacket) {
            pthread_mutex_lock(&pNeteqCtx->mutex);
            if (pNeteqCtx->waitFirstPacket) {
                pthread_cond_wait(&pNeteqCtx->cond, &pNeteqCtx->mutex);
            }
            pthread_mutex_unlock(&pNeteqCtx->mutex);
            if (pNeteqCtx->forceExit) {
                break;
            }
            pNeteqCtx->lastRecOutSyncTime = getTimeUs();
            pNeteqCtx->waitFirstPacket = 0;
        } else {
            pNeteqCtx->lastRecOutSyncTime += 10 * 1000;         //10ms
            int64_t curTimeUs = getTimeUs();
            if (curTimeUs < pNeteqCtx->lastRecOutSyncTime) {
                uint32_t sleepTime = pNeteqCtx->lastRecOutSyncTime - curTimeUs;
                usleep(sleepTime);
                //pNeteqCtx->lastRecOutSyncTime = getTimeUs();  //sync
            } else {
                ALOGW("curTimeUs is larger than last Sync Time %lld(us)",
                    curTimeUs - pNeteqCtx->lastRecOutSyncTime);
            }
        }

        ALOGV("enter WebRtcNetEQ_RecOut");
        int error = WebRtcNetEQ_RecOut(pNeteqCtx->neteqInst,
                        (int16_t *)buffer10ms, &samples_per_channel);
        if (error != 0) {
            ALOGE("WebRtcNetEQ_RecOut returned error code(%d)",
                WebRtcNetEQ_GetErrorCode(pNeteqCtx->neteqInst));
            break;
        }

        WebrtcNeteq_fillAudioBuffer(pNeteqCtx, buffer10ms,
            samples_per_channel * frameSize);
        //WebRtcNetEQ_GetSpeechOutputType(pNeteqCtx->neteqInst, &type);
    }

    free(buffer10ms);
    pNeteqCtx->threadState = Exited;
    return NULL;
}
#endif
