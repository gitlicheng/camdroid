/*******************************************************************************
--                                                                            --
--                    CedarX Multimedia Framework                             --
--                                                                            --
--          the Multimedia Framework for Linux/Android System                 --
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                         Softwinner Products.                               --
--                                                                            --
--                   (C) COPYRIGHT 2011 SOFTWINNER PRODUCTS                   --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
*******************************************************************************/

#ifndef CDX_COMPONENT_H_
#define CDX_COMPONENT_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <CDX_Common.h>

#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Core.h>
#include <OMX_Index.h>
#include <OMX_Image.h>
#include <OMX_Audio.h>
#include <OMX_Video.h>
#include <OMX_IVCommon.h>
#include <OMX_Other.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <vencoder.h>

#define BITSTREAM_FRAME_SIZE    (256)   // ref to BITSTREAM_FRAME_FIFO_SIZE

typedef enum tag_VideoRenderMode
{
    VideoRender_HW      = 0,    //direct set to hardware composer
    VideoRender_SIMUHW  = 1,    //copy to gui_buffer, the method is like set hwlayer, so name as simulate hw.
    VideoRender_SW      = 2,    //dequeue, convert to gui_buf, enqueue
    VideoRender_GUI     = 3,    //display frame is malloc from GUI. 
}VideoRenderMode;

typedef enum DEMUX_DISABLE_TRACKINFO{
	DEMUX_DISABLE_AUDIO_TRACK    = 1,
	DEMUX_DISABLE_VIDEO_TRACK    = 2,
	DEMUX_DISABLE_SUBTITLE_TRACK = 4
}DEMUX_DISABLE_TRACKINFO;

typedef enum CLOCK_COMP_PORT_INDEX{
	CLOCK_PORT_INDEX_AUDIO = 0,
	CLOCK_PORT_INDEX_VIDEO = 1,
	CLOCK_PORT_INDEX_DEMUX = 2,
	CLOCK_PORT_INDEX_VDEC  = 3,
}CLOCK_COMP_PORT_INDEX;

typedef enum CDX_COMP_EVENTS{
	CDX_COMP_EVENTS_UNKONW = 0,
	CDX_COMP_EVENTS_START_TO_RUN ,
}CDX_COMP_EVENTS;

typedef enum CDX_COMP_PRIV_FLAGS{
	CDX_COMP_PRIV_FLAGS_REINIT    = 1,
	CDX_COMP_PRIV_FLAGS_STREAMEOF = 2,
}CDX_COMP_PRIV_FLAGS;

typedef struct CDX_TUNNELINFOTYPE{
	OMX_HANDLETYPE hTunnelMaster;
	OMX_HANDLETYPE hTunnelSlave;
	OMX_U32 nMasterPortIndex;
	OMX_S32 nSlavePortIndex;
	OMX_TUNNEL_TYPE eTunnelType;
	struct CDX_TUNNELINFOTYPE *hNext;
}CDX_TUNNELINFOTYPE;

typedef struct CDX_TUNNELLINKTYPE{
	CDX_TUNNELINFOTYPE *head;
	CDX_TUNNELINFOTYPE *tail;
}CDX_TUNNELLINKTYPE;

typedef struct VEncCompOutputBuffer
{
    VencOutputBuffer mOutBuf;
    OMX_S32 mUsedRefCnt;    // recRender component use it to record used ref count.
    struct list_head mList;
}VEncCompOutputBuffer;

typedef struct AEncCompOutputBuffer
{
    OMX_S8* mpBuf;
	OMX_S32 mSize;
	OMX_S64 mTimeStamp;
    OMX_S32 mBufId;
    OMX_S32 mUsedRefCnt;
    struct list_head mList;
}AEncCompOutputBuffer;

typedef struct CacheState   //OMX_BUFFERSTATE
{
    OMX_U32 mValidSizePercent;  //0~100
    OMX_U32 mValidSize; //unit:kB
    OMX_U32 mTotalSize; //unit:kB
}CacheState;

//------------------------------custom common component define--------------------------------------
typedef enum ThrCmdType {
	SetState, Flush, StopPort, RestartPort, MarkBuf, Stop, FillBuf, EmptyBuf, 
    VendorAddOutputSinkInfo, VendorRemoveOutputSinkInfo, SwitchFile, SwitchFileDone,
    VEncComp_InputFrameAvailable,
    AEncComp_OutFrameAvailable,
    AEncComp_InputPcmAvailable,
    RecRenderComp_InputFrameAvailable,
    RecSink_InputPacketAvailable,
} ThrCmdType;

#define OMX_CONF_INIT_STRUCT_PTR(_s_, _name_)	\
    memset((_s_), 0x0, sizeof(_name_));	\
    (_s_)->nSize = sizeof(_name_);		\
    (_s_)->nVersion.s.nVersionMajor = 0x1;	\
    (_s_)->nVersion.s.nVersionMinor = 0x0;	\
    (_s_)->nVersion.s.nRevision = 0x0;		\
    (_s_)->nVersion.s.nStep = 0x0

#define OMX_CONF_CHK_VERSION(_s_, _name_, _e_)				\
    if((_s_)->nSize != sizeof(_name_)) _e_ = OMX_ErrorBadParameter;	\
    if(((_s_)->nVersion.s.nVersionMajor != 0x1)||			\
       ((_s_)->nVersion.s.nVersionMinor != 0x0)||			\
       ((_s_)->nVersion.s.nRevision != 0x0)||				\
       ((_s_)->nVersion.s.nStep != 0x0)) _e_ = OMX_ErrorVersionMismatch;\
    if(_e_ != OMX_ErrorNone) goto OMX_CONF_CMD_BAIL;

#define OMX_CONF_CHECK_CMD(_ptr1, _ptr2, _ptr3)	\
{						\
    if(!_ptr1 || !_ptr2 || !_ptr3){		\
        eError = OMX_ErrorBadParameter;		\
	goto OMX_CONF_CMD_BAIL;			\
    }						\
}

#define OMX_CONF_BAIL_IF_ERROR(_eError)		\
{						\
    if(_eError != OMX_ErrorNone)		\
        goto OMX_CONF_CMD_BAIL;			\
}

#define OMX_CONF_SET_ERROR_BAIL(_eError, _eCode)\
{						\
    _eError = _eCode;				\
    goto OMX_CONF_CMD_BAIL;			\
}

#define AVS_ADJUST_PERIOD (5*1000*1000) //us

typedef OMX_ERRORTYPE (*ComponentInit)(OMX_IN OMX_HANDLETYPE hComponent);
extern OMX_ERRORTYPE StubGetConfig(OMX_IN OMX_HANDLETYPE hComponent,OMX_IN OMX_INDEXTYPE nIndex,OMX_INOUT OMX_PTR pComponentConfigStructure);
extern OMX_ERRORTYPE StubSetConfig(OMX_IN OMX_HANDLETYPE hComponent,OMX_IN OMX_INDEXTYPE nIndex,OMX_IN OMX_PTR pComponentConfigStructure);
extern int CDX_CreateComponent(OMX_HANDLETYPE *hnd_omx, ComponentInit comp_init, OMX_CALLBACKTYPE *callback, OMX_PTR pAppData, OMX_COMPONENTNAMETYPE type);
OMX_API OMX_ERRORTYPE OMX_UpdateTunnelSlavePortDefine(OMX_IN  OMX_HANDLETYPE hOutput,OMX_IN  OMX_U32 nPortOutput,OMX_IN  OMX_HANDLETYPE hInput,OMX_IN  OMX_U32 nPortInput);
OMX_API OMX_ERRORTYPE ClockComponentInit(OMX_IN OMX_HANDLETYPE hComponent);
OMX_API OMX_ERRORTYPE DemuxComponentInit(OMX_IN OMX_HANDLETYPE hComponent);
OMX_API OMX_ERRORTYPE DemuxNetworkComponentInit(OMX_IN OMX_HANDLETYPE hComponent);
OMX_API OMX_ERRORTYPE DemuxSftNetworkComponentInit(OMX_IN OMX_HANDLETYPE hComponent);
OMX_API OMX_ERRORTYPE DemuxStreammingSourceComponentInit(OMX_IN OMX_HANDLETYPE hComponent);
OMX_API OMX_ERRORTYPE aw_demux_component_init(OMX_IN OMX_HANDLETYPE hComponent);
OMX_API OMX_ERRORTYPE VideoDecComponentInit(OMX_IN OMX_HANDLETYPE hComponent);
OMX_API OMX_ERRORTYPE VideoRenderComponentInit(OMX_IN OMX_HANDLETYPE hComponent);
OMX_API OMX_ERRORTYPE AudioDecComponentInit(OMX_IN OMX_HANDLETYPE hComponent);
OMX_API OMX_ERRORTYPE AudioRenderComponentInit(OMX_IN OMX_HANDLETYPE hComponent);
OMX_API OMX_ERRORTYPE SubtitleComponentInit(OMX_IN OMX_HANDLETYPE hComponent);

OMX_API OMX_ERRORTYPE VideoSourceComponentInit(OMX_IN OMX_HANDLETYPE hComponent);
OMX_API OMX_ERRORTYPE AudioSourceComponentInit(OMX_IN OMX_HANDLETYPE hComponent);
OMX_API OMX_ERRORTYPE VideoEncComponentInit(OMX_IN OMX_HANDLETYPE hComponent);
OMX_API OMX_ERRORTYPE AudioEncComponentInit(OMX_IN OMX_HANDLETYPE hComponent);
OMX_API OMX_ERRORTYPE AudioTransmitComponentInit(OMX_IN OMX_HANDLETYPE hComponent);
OMX_API OMX_ERRORTYPE RecRenderComponentInit(OMX_IN OMX_HANDLETYPE hComponent);

OMX_API OMX_ERRORTYPE OMX_AddTunnelInfoChain(CDX_TUNNELLINKTYPE *cdx_tunnel_link,OMX_IN  OMX_HANDLETYPE hOutput,OMX_IN  OMX_U32 nPortOutput,OMX_IN  OMX_HANDLETYPE hInput,OMX_IN  OMX_U32 nPortInput);
OMX_API OMX_ERRORTYPE OMX_DeleteTunnelInfoChain(CDX_TUNNELLINKTYPE *cdx_tunnel_link);
OMX_API OMX_ERRORTYPE OMX_QueryTunnelInfoChain(CDX_TUNNELLINKTYPE *cdx_tunnel_link,OMX_HANDLETYPE master,OMX_HANDLETYPE slave,CDX_TUNNELINFOTYPE *cdx_tunnel_info);


static int GetTimerSeconds(void)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    //s = tv.tv_usec; s *= 0.000001; s += tv.tv_sec;
    return tv.tv_sec;
}

int CDX_SystemControlInit();
int CDX_SystemControlExit();
int CDX_SystemControlIoctrl(int cmd, int data);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
