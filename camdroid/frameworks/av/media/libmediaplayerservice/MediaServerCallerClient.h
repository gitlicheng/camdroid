#ifndef __MEDIA_SERVER_CALLER_CLIENT_H__
#define __MEDIA_SERVER_CALLER_CLIENT_H__

#include <utils/threads.h>
#include <binder/IMemory.h>
#include <utils/List.h>
#include <utils/Errors.h>
#include <media/IMediaServerCaller.h>
#include <jpegDecoder.h>

namespace android {

class IMediaPlayerService;

class MediaServerCallerClient : public BnMediaServerCaller
{
public:
    MediaServerCallerClient(const sp<IMediaPlayerService>& service, pid_t pid, int32_t connId);

    virtual void disconnect();
    virtual sp<IMemory> jpegDecode(int fd);
    virtual sp<IMemory> jpegDecode(const char *path);

private:
    sp<IMemory> jpegDecodeBuf(sp<JpegDecoder> jpgDecoder, uint8_t *jpgbuf, int filesize);
    friend class MediaPlayerService;

    explicit MediaServerCallerClient(pid_t pid);
    virtual ~MediaServerCallerClient();
    mutable Mutex  mLock;
    pid_t mPid;
};

}; /* namespace android */

#endif /* __MEDIA_SERVER_CALLER_CLIENT_H__ */
