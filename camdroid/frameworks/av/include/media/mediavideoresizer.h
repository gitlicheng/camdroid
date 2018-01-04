#ifndef ANDROID_MEDIAVIDEORESIZER_H
#define ANDROID_MEDIAVIDEORESIZER_H

#include <utils/Log.h>
#include <utils/threads.h>
#include <utils/List.h>
#include <utils/Errors.h>
#include <media/IMediaVideoResizerClient.h>
#include <media/IMediaDeathNotifier.h>

namespace android {

enum MediaVideoSizerStates {
    MEDIA_VIDEO_RESIZER_ERROR = 0,
    MEDIA_VIDEO_RESIZER_IDLE,
    MEDIA_VIDEO_RESIZER_INITIALIZED,
    MEDIA_VIDEO_RESIZER_PREPARING,
    MEDIA_VIDEO_RESIZER_PREPARED,
    MEDIA_VIDEO_RESIZER_STARTED,
    MEDIA_VIDEO_RESIZER_PAUSED,
    MEDIA_VIDEO_RESIZER_STOPPED,
    MEDIA_VIDEO_RESIZER_COMPLETE,
};
// The "msg" code passed to the listener in notify.
enum MediaVideoSizerEventType {
    MEDIA_VIDEO_RESIZER_EVENT_PLAYBACK_COMPLETE = 2,
    MEDIA_VIDEO_RESIZER_EVENT_SEEK_COMPLETE = 4,
    MEDIA_VIDEO_RESIZER_EVENT_ERROR = 100,
    MEDIA_VIDEO_RESIZER_EVENT_INFO = 200,
};
enum MediaVideoSizerErrorType {
    MEDIA_VIDEO_RESIZER_ERROR_UNKNOWN = 1,
    MEDIA_VIDEO_RESIZER_ERROR_SEEK_FAIL,
    MEDIA_VIDEO_RESIZER_ERROR_DIED = 100,
};
enum MediaVideoSizerInfoType {
    MEDIA_VIDEO_RESIZER_INFO_UNKNOWN = 1,
    MEDIA_VIDEO_RESIZER_INFO_BSFRAME_AVAILABLE = 2,
};

enum MediaVideoResizerStreamType {
    MediaVideoResizerStreamType_Video = 0,
	MediaVideoResizerStreamType_Audio,

	MediaVideoResizerStreamType_VideoExtra = 128,
	MediaVideoResizerStreamType_AudioExtra
};

typedef struct VRPacketHeader
{
	MediaVideoResizerStreamType mStreamType;
	int     mSize;
	int64_t mPts;   //unit:ms
}__attribute__((packed)) VRPacketHeader;

class MediaVideoResizerListener: virtual public RefBase
{
public:
    virtual void notify(int msg, int ext1, int ext2) = 0;
};

class MediaVideoResizer : public BnMediaVideoResizerClient,
                                  public virtual IMediaDeathNotifier
{
public:
    MediaVideoResizer();
    ~MediaVideoResizer();

    void        died();
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
    status_t    release();
    status_t    getDuration(int *msec);
    sp<IMemory> getEncDataHeader();
    status_t    getPacket(sp<IMemory> &rawFrame);
    status_t    setListener(const sp<MediaVideoResizerListener>& listener);
    void        notify(int msg, int ext1, int ext2);
    

private:
    status_t    seekTo_l(int msec);
    sp<IMediaVideoResizer>          mVideoResizer;
    thread_id_t                     mLockThreadId;
    sp<MediaVideoResizerListener>   mListener;
    MediaVideoSizerStates           mCurrentState;
    Mutex                           mLock;
    Mutex                           mNotifyLock;

    int                             mCurrentPosition;
    int                             mSeekPosition;
};

} /* manespace android */

#endif /* ANDROID_MEDIAVIDEORESIZER_H */
