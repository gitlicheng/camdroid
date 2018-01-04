/*
********************************************************************************
*                                    camDroid SDK
*                                  videoResize module
*
*          (c) Copyright 2010-2014, Allwinner Microelectronic Co., Ltd.
*                              All Rights Reserved
*
* File   : videoResizerComponentCommon.h
* Version: V1.0
* By     : eric_wang
* Date   : 2014-12-2
* Description:
********************************************************************************
*/
#ifndef _VIDEORESIZERCOMPONENTCOMMON_H_
#define _VIDEORESIZERCOMPONENTCOMMON_H_

#include <utils/Errors.h>
#include <utils/StrongPointer.h>
#include <utils/RefBase.h>
#include <utils/Mutex.h>
#include <utils/Condition.h>
#include <utils/Vector.h>
#include <utils/List.h>
#include <binder/IMemory.h>

//extern "C" void *cedar_sys_phymalloc_map(unsigned int size, int align);
//extern "C" void cedar_sys_phyfree_map(void *buf);
//extern "C" unsigned int cedarv_address_vir2phy(void *addr);
//extern "C" unsigned int cedarv_address_phy2vir(void *addr);
extern "C" int64_t CDX_GetNowUs();


namespace android
{

#define MaxVideoEncodedFrameNum (60)

//state illustration ref to OMX_STATETYPE of OMX_Core.h
typedef enum VideoResizerComponentState 
{
    VRComp_StateInvalid     = 0,
    VRComp_StateLoaded,
    VRComp_StateIdle,
    VRComp_StateExecuting,
    VRComp_StatePause,
    VRComp_StateMax         = 0x7FFFFFFF,
}VideoResizerComponentState;

typedef enum VideoResizerComponentType 
{
    VRComp_TypeUnknown = 0,
    VRComp_TypeDemuxer = 1,
    VRComp_TypeDecoder,
    VRComp_TypeEncoder,
    VRComp_TypeMuxer,
    VRComp_TypeMax = 0x7FFFFFFF,
}VideoResizerComponentType;

typedef enum VideoResizerIndexType
{
    VR_IndexComponentStartUnused = 0x01000000,
    VR_IndexConfigSetStreamEof,
}VideoResizerIndexType;

typedef enum VideoResizerDemuxPortIndex 
{
    VR_DemuxVideoOutputPortIndex = 0x0,
    VR_DemuxAudioOutputPortIndex = 0x1,
}VideoResizerDemuxPortIndex;

typedef enum VideoResizerMessageType 
{
    VIDEO_RESIZER_MESSAGE_SEEK       = 0x0,
    VIDEO_RESIZER_MESSAGE_STOP, //stop resizeThread.

    VR_EventCmdComplete = 0x100,         /**< component has sucessfully completed a command */
    VR_EventError,               /**< component has detected an error condition */
    VR_EventBufferFlag,          /**< component has detected an EOS */ 
    VR_EventBsAvailable,
    VR_EventMax         = 0x7FFFFFFF
}VideoResizerMessageType;

struct VideoResizerMessage 
{
    int what;
    int arg1; 
    int arg2;
    sp<IMemory> obj;
};

class VideoResizerComponentListener: virtual public RefBase
{
public:
	VideoResizerComponentListener(){}
	virtual ~VideoResizerComponentListener() {}
	virtual void notify(int msg, int ext1, int ext2, void *obj) = 0;
};

#define ALIGN( num, to ) (((num) + (to-1)) & (~(to-1)))
#define ALIGN32(num) (ALIGN(num, 32))
#define ALIGN16(num) (ALIGN(num, 16))

class VideoResizerDemuxer;
class VideoResizerDecoder;
class VideoResizerEncoder;
class VideoResizerMuxer;

}; /* namespace android */

#endif  /* _VIDEORESIZERCOMPONENTCOMMON_H_ */

