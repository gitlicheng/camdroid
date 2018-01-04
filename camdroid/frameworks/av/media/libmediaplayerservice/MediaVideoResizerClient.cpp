//#define LOG_NDEBUG 0
#define LOG_TAG "MediaVideoResizerClient"
#include <utils/Log.h>

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/MemoryHeapBase.h>
#include <binder/MemoryBase.h>
#include "MediaVideoResizerClient.h"
#include "MediaPlayerService.h"

namespace android {

status_t MediaVideoResizerClient::setDataSource(const char *url)
{
    ALOGV("setDataSource");
    Mutex::Autolock lock(mLock);
    if (mResizer == NULL) {
        ALOGE("mResizer is not initialized");
        return NO_INIT;
    }
    return mResizer->setDataSource(url);
}

status_t MediaVideoResizerClient::setDataSource(int fd, int64_t offset, int64_t length)
{
    ALOGV("setDataSource");
    Mutex::Autolock lock(mLock);
    if (mResizer == NULL) {
        ALOGE("mResizer is not initialized");
        return NO_INIT;
    }
    return mResizer->setDataSource(fd, offset, length);
}

status_t MediaVideoResizerClient::setVideoSize(int32_t width, int32_t height)
{
    ALOGV("setVideoSize");
    Mutex::Autolock lock(mLock);
    if (mResizer == NULL) {
        ALOGE("mResizer is not initialized");
        return NO_INIT;
    }
    return mResizer->setVideoSize(width, height);
}

status_t MediaVideoResizerClient::setOutputPath(const char *url)
{
    ALOGV("setOutputPath");
    Mutex::Autolock lock(mLock);
    if (mResizer == NULL) {
        ALOGE("mResizer is not initialized");
        return NO_INIT;
    }
    return mResizer->setOutputPath(url);
}

status_t MediaVideoResizerClient::setFrameRate(int32_t framerate)
{
    ALOGV("setFrameRate");
    Mutex::Autolock lock(mLock);
    if (mResizer == NULL) {
        ALOGE("mResizer is not initialized");
        return NO_INIT;
    }
    return mResizer->setFrameRate(framerate);
}

status_t MediaVideoResizerClient::setBitRate(int32_t bitrate)
{
    ALOGV("setBitRate");
    Mutex::Autolock lock(mLock);
    if (mResizer == NULL) {
        ALOGE("mResizer is not initialized");
        return NO_INIT;
    }
    return mResizer->setBitRate(bitrate);
}

status_t MediaVideoResizerClient::prepare()
{
    ALOGV("prepare");
    Mutex::Autolock lock(mLock);
    if (mResizer == NULL) {
        ALOGE("mResizer is not initialized");
        return NO_INIT;
    }
    return mResizer->prepare();
}
status_t MediaVideoResizerClient::start()
{
    ALOGV("start");
    Mutex::Autolock lock(mLock);
    if (mResizer == NULL) {
        ALOGE("mResizer is not initialized");
        return NO_INIT;
    }
    return mResizer->start();
}

status_t MediaVideoResizerClient::stop()
{
    ALOGV("stop");
    Mutex::Autolock lock(mLock);
    if (mResizer == NULL) {
        ALOGE("mResizer is not initialized");
        return NO_INIT;
    }
    return mResizer->stop();
}

status_t MediaVideoResizerClient::pause()
{
    ALOGV("pause");
    Mutex::Autolock lock(mLock);
    if (mResizer == NULL) {
        ALOGE("mResizer is not initialized");
        return NO_INIT;
    }
    return mResizer->pause();
}

status_t MediaVideoResizerClient::seekTo(int msec)
{
    ALOGV("seekTo");
    Mutex::Autolock lock(mLock);
    if (mResizer == NULL) {
        ALOGE("mResizer is not initialized");
        return NO_INIT;
    }
    return mResizer->seekTo(msec);
}

status_t MediaVideoResizerClient::reset()
{
    ALOGV("reset");
    Mutex::Autolock lock(mLock);
    if (mResizer == NULL) {
        ALOGE("mResizer is not initialized");
        return NO_INIT;
    }
    return mResizer->reset();
}

status_t MediaVideoResizerClient::release()
{
    ALOGV("release");
    Mutex::Autolock lock(mLock);
    if (mResizer != NULL) {
        mResizer->reset();
        delete mResizer;
        mResizer = NULL;
        wp<MediaVideoResizerClient> client(this);
        mMediaPlayerService->removeMediaVideoResizerClient(client);
    }
    return NO_ERROR;
}

status_t MediaVideoResizerClient::getDuration(int *msec)
{
    ALOGV("getDuration");
    Mutex::Autolock lock(mLock);
    if (mResizer == NULL) {
        ALOGE("mResizer is not initialized");
        return NO_INIT;
    }
    return mResizer->getDuration(msec);
}

sp<IMemory> MediaVideoResizerClient::getEncDataHeader()
{
    ALOGV("getEncDataHeader");
    Mutex::Autolock lock(mLock);
    if (mResizer == NULL) {
        ALOGE("recorder is not initialized");
        return NULL;
    }
    return mResizer->getEncDataHeader();
}

status_t MediaVideoResizerClient::getPacket(sp<IMemory> &rawFrame)
{
    ALOGV("getOneBsFrame");
    Mutex::Autolock lock(mLock);
    if (mResizer == NULL) {
        ALOGE("mResizer is not initialized");
        return NO_INIT;
    }
    return mResizer->getPacket(rawFrame);
}

MediaVideoResizerClient::MediaVideoResizerClient(const sp<MediaPlayerService>& service, pid_t pid)
{
    ALOGV("Client constructor");
    mPid = pid;
    mResizer = new VideoResizer;
    mMediaPlayerService = service;
}

MediaVideoResizerClient::~MediaVideoResizerClient()
{
    ALOGV("Client destructor");
    release();
}

status_t MediaVideoResizerClient::setListener(const sp<IMediaVideoResizerClient>& listener)
{
    ALOGV("setListener");
    Mutex::Autolock lock(mLock);
    if (mResizer == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mResizer->setListener(listener);
}

}; /* namespace android */

