//#define LOG_NDEBUG 0
#define LOG_TAG "IMediaVideoResizer"
#include <utils/Log.h>
#include <unistd.h>
#include <binder/Parcel.h>
#include <media/IMediaVideoResizerClient.h>
#include <media/IMediaVideoResizer.h>


namespace android {

enum {
    SET_DATA_SOURCE_FD = IBinder::FIRST_CALL_TRANSACTION,
    SET_DATA_SOURCE_URL,
    SET_VIDEO_SIZE,
    SET_OUTPUT_PATH_URL,
    SET_FRAME_RATE,
    SET_BIT_RATE,
    SET_LISTENER,
    PREPARE,
    START,
    STOP,
    RESET,
    PAUSE,
    SEEK_TO,
    RELEASE,
    GET_DURATION,
    GET_ENC_DATA_HEADER,
    GET_ONE_BSFRAME,
};

class BpMediaVideoResizer: public BpInterface<IMediaVideoResizer>
{
public:
    BpMediaVideoResizer(const sp<IBinder>& impl)
    : BpInterface<IMediaVideoResizer>(impl)
    {
    }

    status_t setDataSource(const char* path)
    {
        ALOGV("setDataSource(%s)", path);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaVideoResizer::getInterfaceDescriptor());
        data.writeCString(path);
        remote()->transact(SET_DATA_SOURCE_URL, data, &reply);
        return reply.readInt32();
    }

    status_t setDataSource(int fd, int64_t offset, int64_t length)
    {
        ALOGV("setDataSource(%d, %lld, %lld)", fd, offset, length);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaVideoResizer::getInterfaceDescriptor());
        data.writeFileDescriptor(fd);
        data.writeInt64(offset);
        data.writeInt64(length);
        remote()->transact(SET_DATA_SOURCE_FD, data, &reply);
        return reply.readInt32();
    }

    status_t setVideoSize(int width, int height)
    {
        ALOGV("setVideoSize(%d, %d)", width, height);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaVideoResizer::getInterfaceDescriptor());
        data.writeInt32(width);
        data.writeInt32(height);
        remote()->transact(SET_VIDEO_SIZE, data, &reply);
        return reply.readInt32();
    }

    status_t setOutputPath(const char* path)
    {
        ALOGV("setOutputPath(%s)", path);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaVideoResizer::getInterfaceDescriptor());
        data.writeCString(path);
        remote()->transact(SET_OUTPUT_PATH_URL, data, &reply);
        return reply.readInt32();
    }

    status_t setFrameRate(int32_t framerate)
    {
        ALOGV("setFrameRate(%d)", framerate);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaVideoResizer::getInterfaceDescriptor());
        data.writeInt32(framerate);
        remote()->transact(SET_FRAME_RATE, data, &reply);
        return reply.readInt32();
    }

    status_t setBitRate(int32_t bitrate)
    {
        ALOGV("setBitRate(%d)", bitrate);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaVideoResizer::getInterfaceDescriptor());
        data.writeInt32(bitrate);
        remote()->transact(SET_BIT_RATE, data, &reply);
        return reply.readInt32();
    }

    status_t setListener(const sp<IMediaVideoResizerClient>& listener)
    {
        ALOGV("setListener(%p)", listener.get());
        Parcel data, reply;
        data.writeInterfaceToken(IMediaVideoResizer::getInterfaceDescriptor());
        data.writeStrongBinder(listener->asBinder());
        remote()->transact(SET_LISTENER, data, &reply);
        return reply.readInt32();
    }
    status_t prepare()
    {
        ALOGV("prepare");
        Parcel data, reply;
        data.writeInterfaceToken(IMediaVideoResizer::getInterfaceDescriptor());
        remote()->transact(PREPARE, data, &reply);
        return reply.readInt32();
    }
    status_t start()
    {
        ALOGV("start");
        Parcel data, reply;
        data.writeInterfaceToken(IMediaVideoResizer::getInterfaceDescriptor());
        remote()->transact(START, data, &reply);
        return reply.readInt32();
    }

    status_t stop()
    {
        ALOGV("stop");
        Parcel data, reply;
        data.writeInterfaceToken(IMediaVideoResizer::getInterfaceDescriptor());
        remote()->transact(STOP, data, &reply);
        return reply.readInt32();
    }

    status_t pause()
    {
        ALOGV("pause");
        Parcel data, reply;
        data.writeInterfaceToken(IMediaVideoResizer::getInterfaceDescriptor());
        remote()->transact(PAUSE, data, &reply);
        return reply.readInt32();
    }

    status_t seekTo(int msec)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaVideoResizer::getInterfaceDescriptor());
        data.writeInt32(msec);
        remote()->transact(SEEK_TO, data, &reply);
        return reply.readInt32();
    }

    status_t reset()
    {
        ALOGV("reset");
        Parcel data, reply;
        data.writeInterfaceToken(IMediaVideoResizer::getInterfaceDescriptor());
        remote()->transact(RESET, data, &reply);
        return reply.readInt32();
    }
    
    status_t release()
    {
        ALOGV("release");
        Parcel data, reply;
        data.writeInterfaceToken(IMediaVideoResizer::getInterfaceDescriptor());
        remote()->transact(RELEASE, data, &reply);
        return reply.readInt32();
    }

    status_t getDuration(int *msec)
    {
        ALOGV("getDuration");
        Parcel data, reply;
        data.writeInterfaceToken(IMediaVideoResizer::getInterfaceDescriptor());
        remote()->transact(GET_DURATION, data, &reply);
        *msec = reply.readInt32();
        return reply.readInt32();
    }

    sp<IMemory> getEncDataHeader()
    {
        ALOGV("getEncDataHeader");
        Parcel data, reply;
        data.writeInterfaceToken(IMediaVideoResizer::getInterfaceDescriptor());
        remote()->transact(GET_ENC_DATA_HEADER, data, &reply);
        status_t ret = reply.readInt32();
        if (ret != NO_ERROR) {
            return NULL;
        }
        return interface_cast<IMemory>(reply.readStrongBinder());
    }

    //sp<IMemory> getOneBsFrame()
    status_t getPacket(sp<IMemory> &rawFrame)
    {
        ALOGV("getPacket");
        Parcel data, reply;
        data.writeInterfaceToken(IMediaVideoResizer::getInterfaceDescriptor());
        remote()->transact(GET_ONE_BSFRAME, data, &reply);
        status_t ret = reply.readInt32();
        bool bHasFrame = reply.readInt32();
        if(bHasFrame)
        {
            rawFrame = interface_cast<IMemory>(reply.readStrongBinder());
        }
        else
        {
            rawFrame = NULL;
        }
        return ret;
    }

};

IMPLEMENT_META_INTERFACE(MediaVideoResizer, "android.media.IMediaVideoResizer");

status_t BnMediaVideoResizer::onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch (code) {
        case SET_DATA_SOURCE_URL: {
            ALOGV("SET_DATA_SOURCE_URL");
            CHECK_INTERFACE(IMediaVideoResizer, data, reply);
            const char *path = data.readCString();
            reply->writeInt32(setDataSource(path));
            return NO_ERROR;
        } break;
        case SET_DATA_SOURCE_FD: {
            ALOGV("SET_DATA_SOURCE_FD");
            CHECK_INTERFACE(IMediaVideoResizer, data, reply);
            int fd = data.readFileDescriptor();
            int64_t offset = data.readInt64();
            int64_t length = data.readInt64();
            reply->writeInt32(setDataSource(fd, offset, length));
            return NO_ERROR;
        } break;
        case SET_VIDEO_SIZE: {
            ALOGV("SET_VIDEO_SIZE");
            CHECK_INTERFACE(IMediaVideoResizer, data, reply);
            int width = data.readInt32();
            int height = data.readInt32();
            reply->writeInt32(setVideoSize(width, height));
            return NO_ERROR;
        } break;
        case SET_OUTPUT_PATH_URL: {
            ALOGV("SET_OUTPUT_PATH_URL");
            CHECK_INTERFACE(IMediaVideoResizer, data, reply);
            const char *path = data.readCString();
            reply->writeInt32(setOutputPath(path));
            return NO_ERROR;
        } break;
        case SET_FRAME_RATE: {
            ALOGV("SET_FRAME_RATE");
            CHECK_INTERFACE(IMediaVideoResizer, data, reply);
            int framerate = data.readInt32();
            reply->writeInt32(setFrameRate(framerate));
            return NO_ERROR;
        } break;
        case SET_BIT_RATE: {
            ALOGV("SET_BIT_RATE");
            CHECK_INTERFACE(IMediaVideoResizer, data, reply);
            int bitrate = data.readInt32();
            reply->writeInt32(setBitRate(bitrate));
            return NO_ERROR;
        } break;
        case SET_LISTENER: {
            ALOGV("SET_LISTENER");
            CHECK_INTERFACE(IMediaVideoResizer, data, reply);
            sp<IMediaVideoResizerClient> listener = interface_cast<IMediaVideoResizerClient>(data.readStrongBinder());
            reply->writeInt32(setListener(listener));
            return NO_ERROR;
        } break;
        case PREPARE: {
            ALOGV("PREPARE");
            CHECK_INTERFACE(IMediaVideoResizer, data, reply);
            reply->writeInt32(prepare());
            return NO_ERROR;
        } break;
        case START: {
            ALOGV("START");
            CHECK_INTERFACE(IMediaVideoResizer, data, reply);
            reply->writeInt32(start());
            return NO_ERROR;
        } break;
        case STOP: {
            ALOGV("STOP");
            CHECK_INTERFACE(IMediaVideoResizer, data, reply);
            reply->writeInt32(stop());
            return NO_ERROR;
        } break;
        case PAUSE: {
            ALOGV("PAUSE");
            CHECK_INTERFACE(IMediaVideoResizer, data, reply);
            reply->writeInt32(pause());
            return NO_ERROR;
        } break;
        case SEEK_TO: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(seekTo(data.readInt32()));
            return NO_ERROR;
        } break;
        case RESET: {
            ALOGV("RESET");
            CHECK_INTERFACE(IMediaVideoResizer, data, reply);
            reply->writeInt32(reset());
            return NO_ERROR;
        } break;
        case RELEASE: {
            ALOGV("RELEASE");
            CHECK_INTERFACE(IMediaVideoResizer, data, reply);
            reply->writeInt32(release());
            return NO_ERROR;
        } break;
        case GET_DURATION: {
            ALOGV("GET_DURATION");
            CHECK_INTERFACE(IMediaVideoResizer, data, reply);
            int msec;
            status_t ret = getDuration(&msec);
            reply->writeInt32(msec);
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case GET_ENC_DATA_HEADER: {
        	ALOGV("GET_ENC_DATA_HEADER");
			CHECK_INTERFACE(IMediaVideoResizer, data, reply);
            sp<IMemory> header = getEncDataHeader();
            if (header != NULL) {  // Don't send NULL across the binder interface
                reply->writeInt32(NO_ERROR);
                reply->writeStrongBinder(header->asBinder());
            } else {
                reply->writeInt32(UNKNOWN_ERROR);
            }
			return NO_ERROR;
        } break;
        case GET_ONE_BSFRAME: {
        	ALOGV("GET_ONE_BSFRAME");
			CHECK_INTERFACE(IMediaVideoResizer, data, reply);
            sp<IMemory> rawFrame = NULL;
            status_t ret;
            ret = getPacket(rawFrame);
            reply->writeInt32(ret);
            if(NO_ERROR == ret)
            {
                if(rawFrame!=NULL)
                {
                    reply->writeInt32(true);
                    reply->writeStrongBinder(rawFrame->asBinder());
                }
                else
                {
                    reply->writeInt32(false);
                }
            }
            else
            {
                reply->writeInt32(false);
            }
            return NO_ERROR;
        } break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

}; /* namespace android */

