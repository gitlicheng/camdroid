#ifndef ANDROID_IMEDIAVIDEORESIZER_H
#define ANDROID_IMEDIAVIDEORESIZER_H

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <utils/KeyedVector.h>
#include <binder/IMemory.h>

namespace android {

class IMediaVideoResizerClient;

class IMediaVideoResizer: public IInterface
{
public:
    DECLARE_META_INTERFACE(MediaVideoResizer);

    virtual status_t setDataSource(const char* path) = 0;
    virtual status_t setDataSource(int fd, int64_t offset, int64_t length) = 0;
    virtual status_t setVideoSize(int width, int height) = 0;
    virtual status_t setOutputPath(const char *url) = 0;
    virtual status_t setFrameRate(int32_t framerate) = 0;
    virtual status_t setBitRate(int32_t bitrate) = 0;
    //virtual status_t setParameters(const String8& params) = 0;
    virtual status_t setListener(const sp<IMediaVideoResizerClient>& listener) = 0;
    virtual status_t prepare() = 0;
    virtual status_t start() = 0;
    virtual status_t stop() = 0;
    virtual status_t pause() = 0;
    virtual status_t seekTo(int msec) = 0;
    virtual status_t reset() = 0;
    virtual status_t release() = 0;
    virtual status_t getDuration(int *msec) = 0;
    virtual sp<IMemory> getEncDataHeader() = 0;
    //virtual sp<IMemory> getOneBsFrame() = 0;
    virtual status_t getPacket(sp<IMemory> &rawFrame) = 0;
};

// ----------------------------------------------------------------------------

class BnMediaVideoResizer: public BnInterface<IMediaVideoResizer>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};

}; /* namespace android */

#endif /* ANDROID_IMEDIAVIDEORESIZER_H */

