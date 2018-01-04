//#define LOG_NDEBUG 0
#define LOG_TAG "MediaServerCaller"

#include <utils/Log.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <media/IMediaPlayerService.h>
#include <media/mediaservercaller.h>

namespace android {

Mutex MediaServerCaller::sServiceLock;
sp<IMediaPlayerService> MediaServerCaller::sService;
sp<MediaServerCaller::DeathNotifier> MediaServerCaller::sDeathNotifier;

const sp<IMediaPlayerService>& MediaServerCaller::getService()
{
    Mutex::Autolock lock(sServiceLock);
    if (sService == 0) {
        sp<IServiceManager> sm = defaultServiceManager();
        sp<IBinder> binder;
        do {
            binder = sm->getService(String16("media.player"));
            if (binder != 0) {
                break;
            }
            ALOGW("MediaPlayerService not published, waiting...");
            usleep(500000); // 0.5 s
        } while (true);
        if (sDeathNotifier == NULL) {
            sDeathNotifier = new DeathNotifier();
        }
        binder->linkToDeath(sDeathNotifier);
        sService = interface_cast<IMediaPlayerService>(binder);
    }
    ALOGE_IF(sService == 0, "no MediaPlayerService!?");
    return sService;
}

MediaServerCaller::MediaServerCaller()
{
    ALOGV("constructor");
    const sp<IMediaPlayerService>& service(getService());
    if (service == 0) {
        ALOGE("failed to obtain MediaServerCallerService");
        return;
    }
    sp<IMediaServerCaller> caller(service->createMediaServerCaller(getpid()));
    if (caller == 0) {
        ALOGE("failed to create IMediaServerCaller object from server");
    }
    mCaller = caller;
}

MediaServerCaller::~MediaServerCaller()
{
    ALOGV("destructor");
    disconnect();
    IPCThreadState::self()->flushCommands();
}

void MediaServerCaller::disconnect()
{
    ALOGV("disconnect");
    sp<IMediaServerCaller> caller;
    {
        Mutex::Autolock lock(mLock);
        caller = mCaller;
        mCaller.clear();
    }
    if (caller != 0) {
        caller->disconnect();
    }
}

sp<IMemory> MediaServerCaller::jpegDecode(int fd)
{
    ALOGV("jpegDecode(%d)", fd);
    Mutex::Autolock lock(mLock);
    if (mCaller == NULL) {
        ALOGE("caller is not initialized");
        return NULL;
    }
    return mCaller->jpegDecode(fd);
}

sp<IMemory> MediaServerCaller::jpegDecode(const char *path)
{
    ALOGV("jpegDecode(%s)", path);
    Mutex::Autolock lock(mLock);
    if (mCaller == NULL) {
        ALOGE("caller is not initialized");
        return NULL;
    }
    return mCaller->jpegDecode(path);
}

void MediaServerCaller::DeathNotifier::binderDied(const wp<IBinder>& who) {
    Mutex::Autolock lock(MediaServerCaller::sServiceLock);
    MediaServerCaller::sService.clear();
    ALOGW("MediaServerCaller server died!");
}

MediaServerCaller::DeathNotifier::~DeathNotifier()
{
    Mutex::Autolock lock(sServiceLock);
    if (sService != 0) {
        sService->asBinder()->unlinkToDeath(this);
    }
}

}; /* namespace android */

