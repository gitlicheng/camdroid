#ifndef __VIDEO_RESIZER_H__
#define __VIDEO_RESIZER_H__

#include <fcntl.h> 
#include <utils/Thread.h>
#include <media/IMediaVideoResizerClient.h>
#include <utils/List.h>

#include "VideoResizerDemuxer.h"
#include "VideoResizerDecoder.h"
#include "VideoResizerEncoder.h"
#include "VideoResizerMuxer.h"
#include "vencoder.h"

namespace android {

enum VideoResizerStates {
    VIDEO_RESIZER_STATE_ERROR        = 0,
    VIDEO_RESIZER_IDLE               = 1,
    VIDEO_RESIZER_INITIALIZED        = 2,
    VIDEO_RESIZER_PREPARING          = 3,
    VIDEO_RESIZER_PREPARED           = 4,
    VIDEO_RESIZER_STARTED            = 5,
    VIDEO_RESIZER_PAUSED             = 6,
    VIDEO_RESIZER_STOPPED            = 7,
    VIDEO_RESIZER_PLAYBACK_COMPLETE  = 8,
};

typedef struct ResizerOutputDest {
    char *url;
    int fd;
    int32_t width;  //output video width
    int32_t height; //output video height

    int32_t frameRate;
    int32_t bitRate;
} ResizerOutputDest;


class VideoResizer
{
public:
    VideoResizer();
    virtual     ~VideoResizer();
    status_t    setDataSource(const char *url);
    status_t    setDataSource(int fd, int64_t offset, int64_t length);
    status_t    setVideoSize(int32_t width, int32_t height);
    status_t    setOutputPath(const char *url);
    status_t    setFrameRate(int32_t framerate);
    status_t    setBitRate(int32_t bitrate);
    status_t    prepare();
    status_t    start();
    status_t    stop();
    status_t    pause();
    status_t    seekTo(int msec);
    status_t    reset();
    status_t    getDuration(int *msec);
    status_t    setListener(const sp<IMediaVideoResizerClient> &listener);
    sp<IMemory> getEncDataHeader();
    //sp<IMemory> getOneBsFrame();
    status_t    getPacket(sp<IMemory> &rawFrame);
protected:
    class DoResizeThread : public Thread
    {
    public:
        DoResizeThread(VideoResizer *pOwner);
        void startThread();
        void stopThread();
        virtual status_t readyToRun() ;
        virtual bool threadLoop();
    private:
        VideoResizer* const mpOwner;
        android_thread_id_t mThreadId;
    };

private:
    status_t stop_l();
    void resetSomeMembers();
    bool resizeThread();
    Mutex   mLock;
    volatile VideoResizerStates  mStatus;
    //ResizerInputSource  mInSource;
    ResizerOutputDest   mOutDest;
    sp<IMediaVideoResizerClient> mListener;
    sp<DoResizeThread>  mResizeThread;

    bool                mResizeFlag;
    //videoResizer's component
    VideoResizerDemuxer *mpDemuxer;
    VideoResizerDecoder *mpDecoder;
    VideoResizerEncoder *mpEncoder;
    VideoResizerMuxer   *mpMuxer;

    VencHeaderData  mPpsInfo;    //ENCEXTRADATAINFO_t //spspps info.

    Mutex mMessageQueueLock;
    Condition mMessageQueueChanged;
    List<VideoResizerMessage> mMessageQueue;

    Mutex       mStateCompleteLock;
    int         mnSeekDone;
    Condition   mSeekCompleteCond;
    volatile status_t   mSeekResult;

    class MuxerComponentListener: public VideoResizerComponentListener
    {
	public:
        MuxerComponentListener(VideoResizer *pVR);
        virtual ~MuxerComponentListener() {}
        virtual void notify(int msg, int ext1, int ext2, void *obj);
    private:
        VideoResizer* const mpVideoResizer;
	};
    sp<MuxerComponentListener> mMuxerCompListener;
};

}; /* namespace android */

#endif /* __VIDEO_RESIZER_H__ */

