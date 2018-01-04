#ifndef __MEDIA_SERVER_CALLER_H__
#define __MEDIA_SERVER_CALLER_H__

#include <binder/IMemory.h>
#include <utils/Mutex.h>
#include <media/IMediaServerCaller.h>

namespace android {

class IMediaPlayerService;
class IMediaServerCaller;

class MediaServerCaller : public RefBase
{
public:
    MediaServerCaller();
    ~MediaServerCaller();

    sp<IMemory> jpegDecode(int fd);
    sp<IMemory> jpegDecode(const char *path);
    void disconnect();

private:
    static const sp<IMediaPlayerService>& getService();

    class DeathNotifier: public IBinder::DeathRecipient
    {
    public:
        DeathNotifier() {}
        virtual ~DeathNotifier();
        virtual void binderDied(const wp<IBinder>& who);
    };

    static sp<DeathNotifier>            sDeathNotifier;
    static Mutex                        sServiceLock;
    static sp<IMediaPlayerService>      sService;

    Mutex                               mLock;
    sp<IMediaServerCaller>              mCaller;
};

}; /* namespace android */

#endif /* __MEDIA_SERVER_CALLER_H__ */

