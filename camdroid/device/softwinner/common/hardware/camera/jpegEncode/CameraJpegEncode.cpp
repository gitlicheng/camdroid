#define LOG_TAG "CameraJpegEncode"

#include <cutils/log.h>
#include <utils/Errors.h>

#include "CameraJpegEncode.h"


namespace android {

CameraJpegEncode::CameraJpegEncode()
    : mpVideoEnc(NULL)
{
}

CameraJpegEncode::~CameraJpegEncode()
{
}

status_t CameraJpegEncode::initialize(CameraJpegEncConfig * pConfig, JpegEncInfo * pJpegInfo, EXIFInfo * pExifInfo)
{
    mpVideoEnc = VideoEncCreate(VENC_CODEC_JPEG);
    if (mpVideoEnc == NULL) {
        ALOGE("<F:%s, L:%d> VideoEncCreate error!!", __FUNCTION__, __LINE__);
        return UNKNOWN_ERROR;
    }
    VideoEncSetParameter(mpVideoEnc, VENC_IndexParamJpegExifInfo, pExifInfo);
    VideoEncSetParameter(mpVideoEnc, VENC_IndexParamJpegQuality, &pJpegInfo->quality);
    if(pConfig)
    {
        VideoEncSetParameter(mpVideoEnc, VENC_IndexParamSetVbvSize, &pConfig->nVbvBufferSize);
    }

    if(VideoEncInit(mpVideoEnc, &pJpegInfo->sBaseInfo)< 0)
    {
        ALOGE("<F:%s, L:%d> VideoEncInit error!!", __FUNCTION__, __LINE__);
        VideoEncDestroy(mpVideoEnc);
        mpVideoEnc = NULL;
        return UNKNOWN_ERROR;
    }
    return NO_ERROR;
}

status_t CameraJpegEncode::destroy()
{
    if (mpVideoEnc != NULL) {
        VideoEncDestroy(mpVideoEnc);
    }
    return NO_ERROR;
}

status_t CameraJpegEncode::encode(JpegEncInfo *pJpegInfo)
{
    VencAllocateBufferParam bufferParam;
    VencInputBuffer inputBuffer;

    if(pJpegInfo->bNoUseAddrPhy)
    {
        bufferParam.nSizeY = pJpegInfo->sBaseInfo.nStride*pJpegInfo->sBaseInfo.nInputHeight;
        bufferParam.nSizeC = bufferParam.nSizeY>>1;
        bufferParam.nBufferNum = 1;

        if(AllocInputBuffer(mpVideoEnc, &bufferParam)<0)
        {
            ALOGE("<F:%s, L:%d> AllocInputBuffer error!!", __FUNCTION__, __LINE__);
            return UNKNOWN_ERROR;
        }

        GetOneAllocInputBuffer(mpVideoEnc, &inputBuffer);
        memcpy(inputBuffer.pAddrVirY, pJpegInfo->pAddrPhyY, bufferParam.nSizeY);
        memcpy(inputBuffer.pAddrVirC, pJpegInfo->pAddrPhyC, bufferParam.nSizeC);

        FlushCacheAllocInputBuffer(mpVideoEnc, &inputBuffer);
    }
    else
    {
        inputBuffer.pAddrPhyY = pJpegInfo->pAddrPhyY;
        inputBuffer.pAddrPhyC = pJpegInfo->pAddrPhyC;
    }

    inputBuffer.bEnableCorp         = pJpegInfo->bEnableCorp;
    inputBuffer.sCropInfo.nLeft     =  pJpegInfo->sCropInfo.nLeft;
    inputBuffer.sCropInfo.nTop		=  pJpegInfo->sCropInfo.nTop;
    inputBuffer.sCropInfo.nWidth    =  pJpegInfo->sCropInfo.nWidth;
    inputBuffer.sCropInfo.nHeight   =  pJpegInfo->sCropInfo.nHeight;

    AddOneInputBuffer(mpVideoEnc, &inputBuffer);
    if(VideoEncodeOneFrame(mpVideoEnc)!= 0)
    {
        ALOGE("(f:%s, l:%d) jpeg encoder error", __FUNCTION__, __LINE__);
    }

    AlreadyUsedInputBuffer(mpVideoEnc,&inputBuffer);

    if(pJpegInfo->bNoUseAddrPhy)
    {
        ReturnOneAllocInputBuffer(mpVideoEnc, &inputBuffer);
    }

    return NO_ERROR;
}

int CameraJpegEncode::getFrame()
{
    return GetOneBitstreamFrame(mpVideoEnc, &mOutBuffer);
}

int CameraJpegEncode::returnFrame()
{
    return FreeOneBitStreamFrame(mpVideoEnc, &mOutBuffer);
}

unsigned int CameraJpegEncode::getDataSize()
{
    return mOutBuffer.nSize0 + mOutBuffer.nSize1;
}
status_t CameraJpegEncode::getDataToBuffer(void *buffer)
{
    char *p = (char *)buffer;
    memcpy(p, mOutBuffer.pData0, mOutBuffer.nSize0);
    p += mOutBuffer.nSize0;
    if (mOutBuffer.nSize1 > 0) {
        memcpy(p, mOutBuffer.pData1, mOutBuffer.nSize1);
    }
    return NO_ERROR;
}

};

