#ifndef ANDROID_MEDIAVIDEORESIZECLIENT_H
#define ANDROID_MEDIAVIDEORESIZECLIENT_H

#include <utils/Mutex.h>
#include <media/IMediaVideoResizer.h>
#include <VideoResizer.h>

namespace android {

class MediaPlayerService;

class MediaVideoResizerClient : public BnMediaVideoResizer
{
public:
    virtual status_t    setDataSource(const char *url);
    virtual status_t    setDataSource(int fd, int64_t offset, int64_t length);
    virtual status_t    setVideoSize(int32_t width, int32_t height);
    virtual status_t    setOutputPath(const char *url);
    virtual status_t    setFrameRate(int32_t framerate);
    virtual status_t    setBitRate(int32_t bitrate);
    virtual status_t    prepare();
    virtual status_t    start();
    virtual status_t    stop();
    virtual status_t    pause();
    virtual status_t    seekTo(int msec);
    virtual status_t    reset();
    virtual status_t    release();
    virtual status_t    getDuration(int *msec);
    virtual sp<IMemory> getEncDataHeader();
    //virtual	sp<IMemory> getOneBsFrame();
    virtual status_t    getPacket(sp<IMemory> &rawFrame);
    virtual status_t    setListener(const sp<IMediaVideoResizerClient>& listener);

private:
    friend class        MediaPlayerService;  // for accessing private constructor

                        MediaVideoResizerClient(const sp<MediaPlayerService>& service, pid_t pid);
    virtual             ~MediaVideoResizerClient();

    pid_t                   mPid;
    Mutex                   mLock;
    VideoResizer             *mResizer;
    sp<MediaPlayerService>  mMediaPlayerService;
};

}; /* namespace android */

#endif /* ANDROID_MEDIAVIDEORESIZECLIENT_H */

