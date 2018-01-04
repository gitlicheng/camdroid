/*
 * Author: yewenliang@allwinnertech.com
 * Copyright (C) 2014 Allwinner Technology Co., Ltd.
 */
#ifndef _WEBRTC_NETEQ_INTERFACE_H
#define _WEBRTC_NETEQ_INTERFACE_H

#ifdef __cplusplus
extern "C"
{
#endif

#define PAYLOAD_TYPE_AMR        96
#define PAYLOAD_TYPE_OPUS       97
#define PAYLOAD_TYPE_G726       98

typedef enum DecoderType {
    DecoderReservedStart,
    DecoderAMR,
    DecoderOpus,
    DecoderG726,
    DecoderReservedEnd
} DecoderType;

typedef struct NeteqParam_t {
    DecoderType         type;
    int                 sampleRate;
    int                 channels;
    //int                 bitRate;
    int                 mode;
} NeteqParam;

typedef struct PacketInfo_t {
    uint16_t            sequenceNumber;
    uint32_t            timeStamp;
    uint8_t *           payloadPtr;
    int16_t             payloadLenBytes;
}PacketInfo;

int WebrtcNeteqInit(void **state, NeteqParam *params/*, int avSync*/);

int WebrtcNeteqDeInit(void *state);

int WebrtcNeteqRecIn(void *state, PacketInfo *info);

int WebrtcNeteqRecOut(void *state, int16_t *outData, int16_t *outlen);

int WebrtcNeteqGetPlayoutTimestamp(void *state, uint32_t *pTimeStamp);

int WebrtcNeteqGetDecodedTimeStamp(void *state, uint32_t* pTimeStamp);

#ifdef __cplusplus
}
#endif
#endif
