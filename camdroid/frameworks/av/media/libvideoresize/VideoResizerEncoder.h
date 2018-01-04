#ifndef __VIDEO_RESIZER_ENCODER_H__
#define __VIDEO_RESIZER_ENCODER_H__

#include <utils/Errors.h>
#include <utils/StrongPointer.h>
#include <utils/RefBase.h>
#include <utils/Vector.h>
#include <utils/List.h>
#include <utils/Mutex.h>
#include <utils/Condition.h>
#include <utils/Thread.h>

#include "VideoResizerComponentCommon.h"
#include "VideoResizerDecoder.h"

//#include <CDX_Resource_Manager.h>
#include <type_camera.h>
//#include <H264encLibApi.h>
#include "vencoder.h"
#include <include_omx/OMX_Video.h>

namespace android
{

class VencOutVbs : virtual public RefBase
{
public:
    enum Status
    {
        OwnedByUs = 0,
        OwnedByVenc,
        OwnedByDownStream,
    };
    struct BufInfo
    {
        void    *mpData0;
        int     mDataSize0;
        void    *mpData1;
        int     mDataSize1;
        int64_t mDataPts;   //unit:us
    };
    Status  mStatus;
    void    setVbsInfo(void *pVbs0, int nVbsSize0, void *pVbs1, int nVbsSize1, int64_t nPts, int nVbsIndex);
    void    clearVbsInfo();
    status_t    getVbsBufInfo(BufInfo *pBufInfo) const;
    int         getVbsIndex() const;
    VencOutVbs();
    ~VencOutVbs();
private:
    void    *mBuf0;
    int     mBufSize0;
    void    *mBuf1;
    int     mBufSize1;
    int64_t mPts;
    int     mIndex; //venclib set it. Don't modify it.
};

class VideoResizerEncoder
{
public:
    VideoResizerEncoder(); // set state to Loaded.
    ~VideoResizerEncoder();

    status_t    setEncodeType(OMX_VIDEO_CODINGTYPE nType, int nFrameRate, int nBitRate, int nOutWidth, int nOutHeight, int nSrcFrameRate);  // Loaded->Idle
    status_t    start();    // Idle,Pause->Executing
    status_t    stop();     // Executing,Pause->Idle
    status_t    pause();    // Executing->Pause
    status_t    reset();    // Invalid,Idle,Executing,Pause->Loaded.
    status_t    seekTo(int msec);   //call in state Idle,Pause. need return all transport frames, let them in vdec idle list.

    status_t    setDecoderComponent(VideoResizerDecoder *pDecoder); //call in state Idle
    status_t    setMuxerComponent(VideoResizerMuxer *pMuxer); //call in state Idle
    status_t    GetExtraData(VencHeaderData *pExtraData);
    status_t    EmptyThisBuffer(OMX_BUFFERHEADERTYPE* pBuffer); //call in state Idle,Executing,Pause; called by previous component.
    status_t    FillThisBuffer(OMX_BUFFERHEADERTYPE* pBuffer); //call in state Idle,Executing,Pause; called by next component.
    status_t    SetConfig(VideoResizerIndexType nIndexType, void *pParam); //call in any state
    status_t    setFrameRate(int32_t framerate);
    status_t    setBitRate(int32_t bitrate);

protected:
    class DoEncodeThread : public Thread
    {
    public:
        DoEncodeThread(VideoResizerEncoder *pOwner);
        void startThread();
        void stopThread();
        virtual status_t readyToRun() ;
        virtual bool threadLoop();
    private:
        VideoResizerEncoder* const mpOwner;
        android_thread_id_t mThreadId;
    };

public:
    bool encodeThread();

private:
    status_t    stop_l();
    status_t    createVbsArray();
    status_t    destroyVbsArray();
    status_t    clearVenclibVbsBuffer(VideoEncoder *pVenc);
    void        resetSomeMembers();    //call in reset() and construct.
    sp<DoEncodeThread>          mEncodeThread;
    Mutex                       mLock;
    volatile VideoResizerComponentState mState;

    //VENC_DEVICE             *mpCdxEncoder;
	VideoEncoder*           mpCdxEncoder;
    OMX_VIDEO_CODINGTYPE    mEncodeType;
    int                     mFrameRate; // *1000
    int                     mBitRate;
    int                     mSrcFrameRate;
    int                     mDesOutWidth;       //destination encode video width and height
    int                     mDesOutHeight;
    bool                    mEncoderOpenedFlag;
    //__video_encode_format_t mEncFormat;
    VencHeaderData          mPpsInfo;    //ENCEXTRADATAINFO_t //spspps info.
    //bool                    mCedarvReqCtxInitFlag;
    //ve_mutex_t              mCedarvReqCtx;
    volatile bool           mEofFlag;
    volatile bool           mEncodeEofFlag; //after receive eof flag, encode all frames and pass to next component, then can set encode eof.

    Mutex                       mInputFrameListLock;
    List<sp<VdecOutFrame> >     mInputFrameList;
    bool                        mInputFrameUnderflow;
    int                         mInputFrameRatio;   //e.g, 3 stand for every 3 frames get 1 frame.
    int                         mInputFrameLoopNum;

    Vector<sp<VencOutVbs> >     mOutVbsArray;
    Mutex                       mOutIdleVbsListLock;
    List<sp<VencOutVbs> >       mOutIdleVbsList;
    bool                        mOutVbsUnderflow;
    bool                        mbWaitingOutIdleVbsListFull;
    Condition                   mOutIdleVbsListFullCond;

    VideoResizerDecoder *mpDecoder;
    VideoResizerMuxer   *mpMuxer;
    enum VREncoderMessageType {
        VR_ENCODER_MSG_EXECUTING    = 0,  //pause->executing
        VR_ENCODER_MSG_PAUSE,   //executing->pause
        VR_ENCODER_MSG_SEEK,    //run in state pause!
        VR_ENCODER_MSG_STOP,    //stop Thread.
        VR_ENCODER_MSG_INPUTFRAME_AVAILABLE,
        VR_ENCODER_MSG_OUTVBS_AVAILABLE,
        VR_ENCODER_MSG_EOF,
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
    int mDebugFrameCnt;
    int64_t mEncodeTotalDuration;   //us
    int64_t mLockEncodeTotalDuration;   //us

    typedef struct SoftFrameRateCtrl
    {
        int capture;
        int total;
        int count;
        bool enable;
    } SoftFrameRateCtrl;

    int setFrameRateControl(int reqFrameRate, int srcFrameRate, int *numerator, int *denominator);
    SoftFrameRateCtrl mFrameRateCtrl;
};


}; /* namespace android */

#endif /* __VIDEO_RESIZER_ENCODER_H__ */

