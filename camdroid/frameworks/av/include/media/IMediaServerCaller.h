#ifndef __IMEDIA_SERVER_CALLER_H__
#define __IMEDIA_SERVER_CALLER_H__

#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <binder/IMemory.h>
#include <utils/RefBase.h>

namespace android {

class IMediaServerCaller: public IInterface
{
public:
    DECLARE_META_INTERFACE(MediaServerCaller);
    virtual void            disconnect() = 0;
    virtual sp<IMemory>     jpegDecode(int fd) = 0;
    virtual sp<IMemory>     jpegDecode(const char *path) = 0;
};

// ----------------------------------------------------------------------------

class BnMediaServerCaller: public BnInterface<IMediaServerCaller>
{
public:
    virtual status_t    onTransact(uint32_t code,
                                   const Parcel& data,
                                   Parcel* reply,
                                   uint32_t flags = 0);
};

}; /* namespace android */

#endif /* __IMEDIA_SERVER_CALLER_H__ */

