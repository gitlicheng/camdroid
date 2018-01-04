/*
********************************************************************************
*                           Android multimedia module
*
*          (c) Copyright 2010-2015, Allwinner Microelectronic Co., Ltd.
*                              All Rights Reserved
*
* File   : RecAVSync.h
* Version: V1.0
* By     : eric_wang
* Date   : 2015-3-7
* Description:
********************************************************************************
*/
#ifndef _RECAVSYNC_H_
#define _RECAVSYNC_H_

#include <include_omx/OMX_Types.h>
#include <include_omx/OMX_Core.h>
#include <cedarx_avs_counter.h>

#define AVS_ADJUST_PERIOD_MS 5*1000		// 5s
typedef struct RecAVSync
{
    OMX_S64 mGetFirstVideoTime;
    OMX_S64 mGetFirstAudioTime;
    OMX_S64 mFirstDurationDiff; //videoDuration - audioDuration
    OMX_S64 mVideoBasePts;  //unit:ms
    OMX_S64 mAudioBasePts;  //unit:ms
    OMX_S64 mAudioDataSize; //unit:byte
    OMX_S32 mSampleRate;
    OMX_S32 mChannelNum;
    OMX_S32 mBitsPerSample;
    OMX_S64 mLastAdjustPts;
    CedarxAvscounterContext *mpAvsCounter;
    OMX_ERRORTYPE (*SetVideoPts)(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_S64 nPts);
    OMX_ERRORTYPE (*SetAudioInfo)(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_S32 nSampleRate,
        OMX_IN OMX_S32 nChannelNum,
        OMX_IN OMX_S32 nBitsPerSample);
    OMX_ERRORTYPE (*AddAudioDataLen)(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_S32 nLen);
    OMX_ERRORTYPE (*GetTimeDiff)(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_S64* pDiff);
}RecAVSync;

OMX_ERRORTYPE RecAVSyncInit(RecAVSync *pThiz);
OMX_ERRORTYPE RecAVSyncDestroy(RecAVSync *pThiz);
RecAVSync* RecAVSyncConstruct();
void RecAVSyncDestruct(RecAVSync *pThiz);

#endif  /* _RECAVSYNC_H_ */

