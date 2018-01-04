//#define LOG_NDEBUG 0
#define LOG_TAG "IMediaServerCaller"

#include <utils/Log.h>
#include <stdint.h>
#include <sys/types.h>
#include <binder/Parcel.h>
#include <utils/String8.h>
#include <media/IMediaServerCaller.h>

namespace android {

enum {
    DISCONNECT = IBinder::FIRST_CALL_TRANSACTION,
    JPEG_DECODE,
    JPEG_DECODE_URL,
};

class BpMediaServerCaller: public BpInterface<IMediaServerCaller>
{
public:
    BpMediaServerCaller(const sp<IBinder>& impl)
        : BpInterface<IMediaServerCaller>(impl)
    {
    }

    void disconnect()
    {
        ALOGV("disconnect");
        Parcel data, reply;
        data.writeInterfaceToken(IMediaServerCaller::getInterfaceDescriptor());
        remote()->transact(DISCONNECT, data, &reply);
    }

    sp<IMemory> jpegDecode(int fd)
    {
        ALOGV("jpegDecode(%d)", fd);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaServerCaller::getInterfaceDescriptor());
        data.writeFileDescriptor(fd);
        remote()->transact(JPEG_DECODE, data, &reply);
        status_t ret = reply.readInt32();
        if (ret != NO_ERROR) {
            return NULL;
        }
        return interface_cast<IMemory>(reply.readStrongBinder());
    }

    sp<IMemory> jpegDecode(const char *path)
    {
        ALOGV("jpegDecode(%d)", fd);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaServerCaller::getInterfaceDescriptor());
        data.writeCString(path);
        remote()->transact(JPEG_DECODE_URL, data, &reply);
        status_t ret = reply.readInt32();
        if (ret != NO_ERROR) {
            return NULL;
        }
        return interface_cast<IMemory>(reply.readStrongBinder());
    }
};

IMPLEMENT_META_INTERFACE(MediaServerCaller, "android.media.IMediaServerCaller");

status_t BnMediaServerCaller::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch (code) {
        case DISCONNECT: {
            ALOGV("DISCONNECT");
            CHECK_INTERFACE(IMediaServerCaller, data, reply);
            disconnect();
            return NO_ERROR;
        } break;
        case JPEG_DECODE: {
            ALOGV("JPEG_DECODE");
            CHECK_INTERFACE(IMediaServerCaller, data, reply);
            int fd = dup(data.readFileDescriptor());
            //int fd = data.readFileDescriptor();
            sp<IMemory> data = jpegDecode(fd);
            if (data == NULL) {
                reply->writeInt32(UNKNOWN_ERROR);
            } else {
                reply->writeInt32(NO_ERROR);
                reply->writeStrongBinder(data->asBinder());
            }
            ::close(fd);
            return NO_ERROR;
        } break;
        case JPEG_DECODE_URL: {
            ALOGV("JPEG_DECODE_URL");
            CHECK_INTERFACE(IMediaServerCaller, data, reply);
            const char* path = data.readCString();
            sp<IMemory> data = jpegDecode(path);
            if (data == NULL) {
                reply->writeInt32(UNKNOWN_ERROR);
            } else {
                reply->writeInt32(NO_ERROR);
                reply->writeStrongBinder(data->asBinder());
            }
            return NO_ERROR;
        } break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

}; /* namespace android */

