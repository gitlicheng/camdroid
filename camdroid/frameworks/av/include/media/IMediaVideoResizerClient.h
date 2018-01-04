#ifndef ANDROID_IVIDEORESIZERCLIENT_H
#define ANDROID_IVIDEORESIZERCLIENT_H

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>

namespace android {

class IMediaVideoResizerClient: public IInterface
{
public:
    DECLARE_META_INTERFACE(MediaVideoResizerClient);

    virtual void notify(int msg, int ext1, int ext2) = 0;
};

// ----------------------------------------------------------------------------

class BnMediaVideoResizerClient: public BnInterface<IMediaVideoResizerClient>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};

}; /* namespace android */

#endif /* ANDROID_IVIDEORESIZERCLIENT_H */

