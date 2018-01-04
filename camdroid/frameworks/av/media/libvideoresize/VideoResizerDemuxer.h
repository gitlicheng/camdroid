#ifndef __VIDEO_RESIZER_DEMUXER_H__
#define __VIDEO_RESIZER_DEMUXER_H__

#include <utils/Errors.h>
#include <utils/StrongPointer.h>
#include <utils/RefBase.h>
#include <utils/Vector.h>
#include <utils/List.h>
#include <utils/Mutex.h>
#include <utils/Condition.h>
#include <utils/threads.h>

#include "VideoResizerComponentCommon.h"
#include "VideoResizerEncoder.h"

#include <cedarx_demux.h>

namespace android
{

typedef struct ResizerInputSource {
    int fd;
    int64_t offset;
    int64_t length;
    char *url;
} ResizerInputSource;

class VideoResizerDemuxer
{
public:
    VideoResizerDemuxer(); // set state to Loaded.
    ~VideoResizerDemuxer();

    status_t    setDataSource(const char *url);
    status_t    setDataSource(int fd, int64_t offset, int64_t length);  // Loaded->Idle
    status_t    start();    // Idle,Pause->Executing
    status_t    stop();     // Executing,Pause->Idle
    status_t    pause();    // Executing->Pause
    status_t    reset();    // Invalid,Idle,Executing,Pause->Loaded.
    
    status_t    seekTo(int msec);   //call in state Idle,Pause
    CDX_MEDIA_FILE_FORMAT   getMediaFileFormat(); //call in state Idle
    CedarXMediainfo*    getMediainfo(); //call in state Idle
    status_t    setResizeFlag(bool flag); //call in state Idle
    status_t    setDecoderComponent(VideoResizerDecoder *pDecoderComp); //call in state Idle
    status_t    setMuxerComponent(VideoResizerMuxer *pMuxerComp); //call in state Idle
    status_t    FillThisBuffer(OMX_BUFFERHEADERTYPE* pBuffer);
    

protected:
    class DoDemuxThread : public Thread
    {
    public:
        DoDemuxThread(VideoResizerDemuxer *pDemuxer);
        void startThread();
        void stopThread();
        virtual status_t readyToRun() ;
        virtual bool threadLoop();
    private:
        VideoResizerDemuxer* mpDemuxer;
        android_thread_id_t mThreadId;
    };

public:
    bool demuxThread();

private:
    status_t stop_l();
    void resetSomeMembers(); //call in reset() and VideoResizerDemuxer().
    sp<DoDemuxThread>           mDemuxThread;
    Mutex                       mLock;
    volatile VideoResizerComponentState mState;
    ResizerInputSource          mInSource;

    CedarXDemuxerAPI            *mpCdxEpdkDmx;
    CedarXDataSourceDesc        mDataSrcDesc;
    CedarXMediainfo             mCdxMediainfo;
    CedarXPacket                mCdxPkt;
    CedarXSeekPara              mSeekPara;
    CDX_DEMUX_MODE_E            mDemuxMode;
    bool                        mEofFlag;

    bool                        mResizeFlag;
    VideoResizerDecoder         *mpDecoder;
    VideoResizerMuxer           *mpMuxer;

    void            *mpOutChunk;
    int             mOutChunkSize;
    Mutex           mOutChunkLock;
    sp<VencOutVbs>  mpOutVbs;
    bool            mbOutChunkUnderflow;

    enum VRDemuxerMessageType {
        VR_DEMUXER_MSG_EXECUTING    = 0,  //pause->executing
        VR_DEMUXER_MSG_PAUSE,   //executing->pause
        VR_DEMUXER_MSG_SEEK,    //run in state pause!
        VR_DEMUXER_MSG_STOP,    //stop DemuxerThread.
        VR_DEMUXER_MSG_OUTVBS_AVAILABLE,
    };
    #define WaitRequestBufferTime (100*1000*1000)  //unit:ns
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

#endif /* __VIDEO_RESIZER_DEMUX_H__ */

