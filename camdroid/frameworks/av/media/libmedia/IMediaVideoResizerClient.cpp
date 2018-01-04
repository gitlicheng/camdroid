#define LOG_NDEBUG 0
#define LOG_TAG "IMediaVideoResizerClient"

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>

#include <media/IMediaVideoResizerClient.h>

namespace android {

enum {
    NOTIFY = IBinder::FIRST_CALL_TRANSACTION,
};

class BpMediaVideoResizerClient: public BpInterface<IMediaVideoResizerClient>
{
public:
    BpMediaVideoResizerClient(const sp<IBinder>& impl)
        : BpInterface<IMediaVideoResizerClient>(impl)
    {
    }

    virtual void notify(int msg, int ext1, int ext2)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaVideoResizerClient::getInterfaceDescriptor());
        data.writeInt32(msg);
        data.writeInt32(ext1);
        data.writeInt32(ext2);
        remote()->transact(NOTIFY, data, &reply, IBinder::FLAG_ONEWAY);
    }
};

IMPLEMENT_META_INTERFACE(MediaVideoResizerClient, "android.media.IMediaVideoResizerClient");

// ----------------------------------------------------------------------

status_t BnMediaVideoResizerClient::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch (code) {
        case NOTIFY: {
            CHECK_INTERFACE(IMediaVideoResizerClient, data, reply);
            int msg = data.readInt32();
            int ext1 = data.readInt32();
            int ext2 = data.readInt32();
            notify(msg, ext1, ext2);
            return NO_ERROR;
        } break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

}; // namespace android

