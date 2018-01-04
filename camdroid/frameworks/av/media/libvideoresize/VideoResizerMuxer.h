#ifndef __VIDEO_RESIZER_MUXER_H__
#define __VIDEO_RESIZER_MUXER_H__

#include <utils/Errors.h>
#include <utils/StrongPointer.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/List.h>
#include <utils/Mutex.h>
#include <utils/Condition.h>
#include <utils/Thread.h>
#include <binder/MemoryHeapBase.h>
#include <binder/MemoryBase.h>

#include "VideoResizerComponentCommon.h"
#include "VideoResizerEncoder.h"

#include <include_omx/OMX_Core.h>

namespace android
{

typedef enum MuxerType {
	MUXER_TYPE_MP4 = 0,
	MUXER_TYPE_RAW,
}MuxerType;

class VideoResizerMuxer
{
public:
    VideoResizerMuxer(); // set state to Loaded.
    ~VideoResizerMuxer();

    status_t    setMuxerInfo(char *pUrl, int nFd, int nWidth, int nHeight, MuxerType nType);// Loaded->Idle
    status_t    start();    // Idle,Pause->Executing
    status_t    stop();     // Executing,Pause->Idle
    status_t    pause();    // Executing->Pause
    status_t    reset();    // Invalid,Idle,Executing,Pause->Loaded.
    status_t    seekTo(int msec);   //call in state Idle,Pause. need return all transport frames, let them in vdec idle list.

    status_t    setResizeFlag(bool flag); //call in state Idle
    status_t    setEncoderComponent(VideoResizerEncoder *pEncoder); //call in state Idle
    status_t    setDemuxerComponent(VideoResizerDemuxer *pDemuxer); //call in state Idle
    status_t    setListener(sp<VideoResizerComponentListener> pListener);
    status_t    EmptyThisBuffer(OMX_BUFFERHEADERTYPE* pBuffer); //call in state Idle,Executing,Pause; called by previous component.
    status_t    getPacket(sp<IMemory> &rawFrame);
    status_t    SetConfig(VideoResizerIndexType nIndexType, void *pParam); //call in any state

protected:
    class DoMuxThread : public Thread
    {
    public:
        DoMuxThread(VideoResizerMuxer *pOwner);
        void startThread();
        void stopThread();
        virtual status_t readyToRun() ;
        virtual bool threadLoop();
    private:
        VideoResizerMuxer* const mpOwner;
        android_thread_id_t mThreadId;
    };

public:
    bool MuxThread();

private:
    status_t    stop_l();
    void        resetSomeMembers();    //call in reset() and construct.
    sp<DoMuxThread>     mMuxThread;
    Mutex               mLock;
    volatile VideoResizerComponentState mState;

    String8     mOutUrl;
    int         mFd;
    int         mWidth;
    int         mHeight;
    MuxerType   mMuxerType;
    bool        mResizeFlag;
    bool        mbCallbackOutFlag;

    volatile bool           mEofFlag;
    volatile bool           mMuxEofFlag; //after receive eof flag, encode all frames and pass to next component, then can set encode eof.

    Mutex                   mFrameListLock;
    List<sp<VencOutVbs> >   mVideoInputBsList;
    List<sp<VencOutVbs> >   mAudioInputBsList;
    //rawFrame, videoOutBs struct: VRPacketHeader, char[],
    List<sp<IMemory> >      mVideoOutPacketList;
    List<sp<IMemory> >      mAudioOutPacketList;
    bool                    mbWaitBsAvailable;  //for video input bs

    
    VideoResizerEncoder     *mpEncoder;
    VideoResizerDemuxer     *mpDemuxer;
    sp<VideoResizerComponentListener> mpListener;
    enum VRMuxerMessageType {
        VR_MUXER_MSG_EXECUTING    = 0,  //pause->executing
        VR_MUXER_MSG_PAUSE,   //executing->pause
        VR_MUXER_MSG_SEEK,    //run in state pause!
        VR_MUXER_MSG_STOP,    //stop Thread.
        VR_MUXER_MSG_BS_AVAILABLE,  //inputBs and outBs all available.
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
};

}; /* namespace android */

#endif /* __VIDEO_RESIZER_MUXER_H__ */

