#ifndef __VIDEO_RESIZER_DECODER_H__
#define __VIDEO_RESIZER_DECODER_H__

#include <utils/Errors.h>
#include <utils/StrongPointer.h>
#include <utils/RefBase.h>
#include <utils/Vector.h>
#include <utils/List.h>
#include <utils/Mutex.h>
#include <utils/Condition.h>
#include <utils/threads.h>

#include "VideoResizerComponentCommon.h"

//#include <CDX_Resource_Manager.h>
//#include <libcedarv.h>
#include <CDX_Common.h>
#include <vdecoder.h>
#include <include_omx/OMX_Video.h>
#include <cedarx_demux.h>
namespace android
{

class VdecOutFrame : virtual public RefBase
{
public:
    //char        *mBuf;
    //char        *mPhyBuf;
    const int   mStoreWidth;
    const int   mStoreHeight;
    int         mDisplayWidth;
    int         mDisplayHeight;
    //int         mBufSize;
    int64_t     mPts;
    //int         mBufferID;
    //const cedarv_pixel_format_e mPixelFormat;
    enum EPIXELFORMAT mPixelFormat;
    VideoPicture *mPicture;
    enum Status
    {
        OwnedByUs = 0,
        OwnedByVdec,
        OwnedByDownStream,
    };
    Status      mStatus;
    VdecOutFrame(int storeWidth, int storeHeight, enum EPIXELFORMAT format);
    ~VdecOutFrame();
    status_t setFrameInfo(VideoPicture *pCedarvPic);
    //status_t copyFrame(VideoPicture *pCedarvPic);
};

class VideoResizerDecoder
{
public:
    VideoResizerDecoder(); // set state to Loaded.
    ~VideoResizerDecoder();

    status_t    setVideoFormat(OMX_VIDEO_PORTDEFINITIONTYPE *pVideoFormat, CDX_MEDIA_FILE_FORMAT nFileFormat, int nOutWidth, int nOutHeight);  // Loaded->Idle
    status_t    start();    // Idle,Pause->Executing
    status_t    stop();     // Executing,Pause->Idle
    status_t    pause();    // Executing->Pause
    status_t    reset();    // Invalid,Idle,Executing,Pause->Loaded.
    status_t    seekTo(int msec);   //call in state Idle,Pause

    status_t    setEncoderComponent(VideoResizerEncoder *pEncoderComp); //call in state Idle
    status_t    requstBuffer(OMX_BUFFERHEADERTYPE* pBuffer);   //call in state Executing,Pause
    status_t    updateBuffer(OMX_BUFFERHEADERTYPE* pBuffer);   //call in state Executing,Pause
    status_t    FillThisBuffer(OMX_BUFFERHEADERTYPE* pBuffer); //call in state Idle,Executing,Pause
    status_t    SetConfig(VideoResizerIndexType nIndexType, void *pParam); //call in any state

protected:
    class DoDecodeThread : public Thread
    {
    public:
        DoDecodeThread(VideoResizerDecoder *pOwner);
        void startThread();
        void stopThread();
        virtual status_t readyToRun() ;
        virtual bool threadLoop();
    private:
        VideoResizerDecoder* const mpOwner;
        android_thread_id_t mThreadId;
    };

public:
    bool decodeThread();

private:
    status_t    stop_l();
    status_t    releaseFrame(sp<VdecOutFrame>& outFrame);
    status_t    createFrameArray();
    status_t    destroyFrameArray();
    void        resetSomeMembers();    //call in reset() and construct.
    sp<DoDecodeThread>          mDecodeThread;
    Mutex                       mLock;
    volatile VideoResizerComponentState mState;

    //cedarv_decoder_t        *mpCdxDecoder;
	VideoDecoder*           mpCdxDecoder; //cedarv_decoder_t
    //cedarv_stream_info_t    mCedarvStreamInfo;  //init_data will share demux component buffer.
	VideoStreamInfo         mCedarvStreamInfo;  //cedarv_stream_info_t
	VConfig                 mDecoderConfig;
    int                     mDesOutWidth;
    int                     mDesOutHeight;
    int                     mVdecStoreWidth;    //vdec output frame buffer's width and height
    int                     mVdecStoreHeight;
    int                     mRotation;
    bool                    mbScaleEnableFlag;
    VideoPicture*           mCedarvPic; //cedarv_picture_t 
    //bool                    mCedarvReqCtxInitFlag;
    //ve_mutex_t              mCedarvReqCtx;
    volatile bool           mEofFlag;
    volatile bool           mDecodeEndFlag;
    volatile bool           mDecodeEofFlag; //after decode end, send all decoded frames to next component, then can set decode eof.

    #define FRAME_ARRAY_SIZE    (4)
    Vector<sp<VdecOutFrame> >   mFrameArray;
    Mutex                       mIdleFrameListLock;
    List<sp<VdecOutFrame> >     mIdleFrameList;
    bool                        mOutFrmUnderFlow;

    Mutex           mVbsLock;
    int             mVbsSemValue;
    //Mutex           mOutFrmLock;
    
    VideoResizerEncoder *mpEncoder;
    enum VRDecoderMessageType {
        VR_DECODER_MSG_EXECUTING    = 0,  //pause->executing
        VR_DECODER_MSG_PAUSE,   //executing->pause
        VR_DECODER_MSG_SEEK,    //run in state pause!
        VR_DECODER_MSG_STOP,    //stop Thread.
        VR_DECODER_MSG_VBS_AVAILABLE,
        VR_DECODER_MSG_OUTFRAME_AVAILABLE,
    };
    Mutex       mMessageQueueLock;
    Condition   mMessageQueueChanged;
    List<VideoResizerMessage> mMessageQueue;

    Mutex       mStateCompleteLock;
    int         mnExecutingDone;
    Condition   mExecutingCompleteCond;
    int         mnPauseDone;
    Condition   mPauseCompleteCond;
    int         mnSeekDone;
    Condition   mSeekCompleteCond;

    //for debug
    int     mDebugFrameCnt;
    int64_t mDecodeTotalDuration;   //us
    int64_t mLockDecodeTotalDuration;   //us
    
};

}; /* namespace android */

#endif /* __VIDEO_RESIZER_DECODE_H__ */

