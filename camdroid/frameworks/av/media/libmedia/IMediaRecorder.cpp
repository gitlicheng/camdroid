/*
 **
 ** Copyright 2008, The Android Open Source Project
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "IMediaRecorder"
#include <utils/Log.h>
#include <binder/Parcel.h>
#include <camera/ICamera.h>
#include <media/IMediaRecorderClient.h>
#include <media/IMediaRecorder.h>
//#include <gui/Surface.h>
//#include <gui/ISurfaceTexture.h>
#include <unistd.h>


namespace android {

enum {
    RELEASE = IBinder::FIRST_CALL_TRANSACTION,
    INIT,
    CLOSE,
    QUERY_SURFACE_MEDIASOURCE,
    RESET,
    STOP,
    START,
    PREPARE,
    GET_MAX_AMPLITUDE,
    SET_VIDEO_SOURCE,
    SET_AUDIO_SOURCE,
    SET_OUTPUT_FORMAT,
    SET_VIDEO_ENCODER,
    SET_AUDIO_ENCODER,
    SET_OUTPUT_FILE_PATH,
    SET_OUTPUT_FILE_FD,
    SET_VIDEO_SIZE,
    SET_VIDEO_FRAMERATE,
    SET_PARAMETERS,
    SET_PREVIEW_SURFACE,
    SET_CAMERA,
    SET_LISTENER,
    SET_MUTE_MODE,

    QUEUE_BUFFER = RELEASE + 200,
    GET_ONE_BSFRAME,
    FREE_ONE_BSFRAME,
    GET_ENC_DATA_HEADER,
    SET_VIDEO_ENCODEING_BITRATE_SYNC,
    SET_VIDEO_FRAME_RATE_SYNC,
    SET_VIDEO_ENCODING_IFRAMES_NUMBER_INTERVAL_SYNC,
    REENCODE_IFRAME,
    SET_OUTPUT_FILE_FD_SYNC,
    SET_OUTPUT_FILE_FD_SYNC_URL,
    SET_SDCARD_STATE,
    ADD_OUTPUTFORMAT_AND_OUTPUTSINK,
    ADD_OUTPUTFORMAT_AND_OUTPUTSINK_URL,
    REMOVE_OUTPUTFORMAT_AND_OUTPUTSINK,

	SET_MAX_DURATION,
	SET_MAX_FILE_SIZE,

	SET_IMPACT_OUTPUT_FILE,
	SET_IMPACT_OUTPUT_FILE_URL,
};

class BpMediaRecorder: public BpInterface<IMediaRecorder>
{
public:
    BpMediaRecorder(const sp<IBinder>& impl)
    : BpInterface<IMediaRecorder>(impl)
    {
    }

    status_t setCamera(const sp<ICamera>& camera, const sp<ICameraRecordingProxy>& proxy)
    {
        ALOGV("setCamera(%p,%p)", camera.get(), proxy.get());
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        data.writeStrongBinder(camera->asBinder());
        data.writeStrongBinder(proxy->asBinder());
        remote()->transact(SET_CAMERA, data, &reply);
        return reply.readInt32();
    }

    //sp<ISurfaceTexture> querySurfaceMediaSource()
    unsigned int querySurfaceMediaSource()
    {
        ALOGV("Query SurfaceMediaSource");
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        remote()->transact(QUERY_SURFACE_MEDIASOURCE, data, &reply);
        int returnedNull = reply.readInt32();
        if (returnedNull) {
            return 0;
        }
        //return interface_cast<ISurfaceTexture>(reply.readStrongBinder());
        return (unsigned int)reply.readInt32();
    }

    status_t queueBuffer(int index, int addr_y, int addr_c, int64_t timestamp)
    {
		ALOGV("queueBuffer(%d)", index);
		Parcel data, reply;
		data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
		data.writeInt32(index);
		data.writeInt32(addr_y);
		data.writeInt32(addr_c);
		data.writeInt64(timestamp);
		remote()->transact(QUEUE_BUFFER, data, &reply);
		return reply.readInt32();
    }
    
    //status_t setPreviewSurface(const sp<Surface>& surface)
    status_t setPreviewSurface(const unsigned int hlay)
    {
        ALOGV("setPreviewSurface(%d)", hlay);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
//        Surface::writeToParcel(surface, &data);
        data.writeInt32(hlay);
        remote()->transact(SET_PREVIEW_SURFACE, data, &reply);
        return reply.readInt32();
    }

    status_t init()
    {
        ALOGV("init");
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        remote()->transact(INIT, data, &reply);
        return reply.readInt32();
    }

    status_t setVideoSource(int vs)
    {
        ALOGV("setVideoSource(%d)", vs);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        data.writeInt32(vs);
        remote()->transact(SET_VIDEO_SOURCE, data, &reply);
        return reply.readInt32();
    }

    status_t setAudioSource(int as)
    {
        ALOGV("setAudioSource(%d)", as);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        data.writeInt32(as);
        remote()->transact(SET_AUDIO_SOURCE, data, &reply);
        return reply.readInt32();
    }

    status_t setOutputFormat(int of)
    {
        ALOGV("setOutputFormat(%d)", of);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        data.writeInt32(of);
        remote()->transact(SET_OUTPUT_FORMAT, data, &reply);
        return reply.readInt32();
    }

    status_t setVideoEncoder(int ve)
    {
        ALOGV("setVideoEncoder(%d)", ve);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        data.writeInt32(ve);
        remote()->transact(SET_VIDEO_ENCODER, data, &reply);
        return reply.readInt32();
    }

    status_t setAudioEncoder(int ae)
    {
        ALOGV("setAudioEncoder(%d)", ae);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        data.writeInt32(ae);
        remote()->transact(SET_AUDIO_ENCODER, data, &reply);
        return reply.readInt32();
    }

    status_t setOutputFile(const char* path)
    {
        ALOGV("setOutputFile(%s)", path);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        data.writeCString(path);
        data.writeInt64(0);
        data.writeInt64(0);
        remote()->transact(SET_OUTPUT_FILE_PATH, data, &reply);
        return reply.readInt32();
    }
    status_t setOutputFile(const char* path, int64_t offset, int64_t length)
    {
        ALOGV("setOutputFile(%s) %lld, %lld", path, offset, length);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        data.writeCString(path);
        data.writeInt64(offset);
        data.writeInt64(length);
        remote()->transact(SET_OUTPUT_FILE_PATH, data, &reply);
        return reply.readInt32();
    }
    status_t setOutputFile(int fd, int64_t offset, int64_t length) {
        ALOGV("setOutputFile(%d, %lld, %lld)", fd, offset, length);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        data.writeFileDescriptor(fd);
        data.writeInt64(offset);
        data.writeInt64(length);
        remote()->transact(SET_OUTPUT_FILE_FD, data, &reply);
        return reply.readInt32();
    }

    status_t setVideoSize(int width, int height)
    {
        ALOGV("setVideoSize(%dx%d)", width, height);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        data.writeInt32(width);
        data.writeInt32(height);
        remote()->transact(SET_VIDEO_SIZE, data, &reply);
        return reply.readInt32();
    }

    status_t setVideoFrameRate(int frames_per_second)
    {
        ALOGV("setVideoFrameRate(%d)", frames_per_second);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        data.writeInt32(frames_per_second);
        remote()->transact(SET_VIDEO_FRAMERATE, data, &reply);
        return reply.readInt32();
    }

    status_t setMuteMode(bool mute)
    {
        ALOGV("setMuteMode(%d)", mute);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        data.writeInt32(mute);
        remote()->transact(SET_MUTE_MODE, data, &reply);
        return reply.readInt32();
    }

    status_t setParameters(const String8& params)
    {
        ALOGV("setParameter(%s)", params.string());
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        data.writeString8(params);
        remote()->transact(SET_PARAMETERS, data, &reply);
        return reply.readInt32();
    }

    status_t setListener(const sp<IMediaRecorderClient>& listener)
    {
        ALOGV("setListener(%p)", listener.get());
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        data.writeStrongBinder(listener->asBinder());
        remote()->transact(SET_LISTENER, data, &reply);
        return reply.readInt32();
    }

    status_t prepare()
    {
        ALOGV("prepare");
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        remote()->transact(PREPARE, data, &reply);
        return reply.readInt32();
    }

    status_t getMaxAmplitude(int* max)
    {
        ALOGV("getMaxAmplitude");
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        remote()->transact(GET_MAX_AMPLITUDE, data, &reply);
        *max = reply.readInt32();
        return reply.readInt32();
    }

    sp<IMemory> getOneBsFrame(int mode)
	{
		ALOGV("getOneBsFrame");
		Parcel data, reply;
		data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
		data.writeInt32(mode);
		remote()->transact(GET_ONE_BSFRAME, data, &reply);
		status_t ret = reply.readInt32();
		if (ret != NO_ERROR) {
			return NULL;
		}
		return interface_cast<IMemory>(reply.readStrongBinder());
	}

	void freeOneBsFrame()
	{
        ALOGV("freeOneBsFrame");
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        remote()->transact(FREE_ONE_BSFRAME, data, &reply);
	}

	sp<IMemory> getEncDataHeader()
	{
		ALOGV("getEncDataHeader");
		Parcel data, reply;
		data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
		remote()->transact(GET_ENC_DATA_HEADER, data, &reply);
		status_t ret = reply.readInt32();
		if (ret != NO_ERROR) {
			return NULL;
		}
		return interface_cast<IMemory>(reply.readStrongBinder());
	}

	status_t setVideoEncodingBitRateSync(int bitRate)
	{
		ALOGV("setVideoEncodingBitRateSync(%d)", bitRate);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
		data.writeInt32(bitRate);
		remote()->transact(SET_VIDEO_ENCODEING_BITRATE_SYNC, data, &reply);
		return reply.readInt32();
	}

	status_t setVideoFrameRateSync(int frames_per_second)
	{
		ALOGV("setVideoFrameRateSync(%d)", frames_per_second);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
		data.writeInt32(frames_per_second);
		remote()->transact(SET_VIDEO_FRAME_RATE_SYNC, data, &reply);
		return reply.readInt32();
	}

    status_t start()
    {
        ALOGV("start");
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        remote()->transact(START, data, &reply);
        return reply.readInt32();
    }

    status_t stop()
    {
        ALOGV("stop");
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        remote()->transact(STOP, data, &reply);
        return reply.readInt32();
    }

    status_t reset()
    {
        ALOGV("reset");
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        remote()->transact(RESET, data, &reply);
        return reply.readInt32();
    }

    status_t close()
    {
        ALOGV("close");
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        remote()->transact(CLOSE, data, &reply);
        return reply.readInt32();
    }

    status_t release()
    {
        ALOGV("release");
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        remote()->transact(RELEASE, data, &reply);
        return reply.readInt32();
    }

    status_t setVideoEncodingIFramesNumberIntervalSync(int nMaxKeyItl)
    {
        ALOGV("setVideoEncodingIFramesNumberIntervalSync(%d)", nMaxKeyItl);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
		data.writeInt32(nMaxKeyItl);
		remote()->transact(SET_VIDEO_ENCODING_IFRAMES_NUMBER_INTERVAL_SYNC, data, &reply);
		return reply.readInt32();
    }

    status_t reencodeIFrame()
    {
        ALOGV("reencodeIFrame");
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
		remote()->transact(REENCODE_IFRAME, data, &reply);
		return reply.readInt32();
    }

    status_t setOutputFileSync(int fd, int64_t fallocateLength, int muxerId)
    {
        ALOGV("setOutputFileSync(%d, %lld, %d)", fd, fallocateLength, muxerId);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        data.writeFileDescriptor(fd);
        data.writeInt64(fallocateLength);
        data.writeInt32(muxerId);
        remote()->transact(SET_OUTPUT_FILE_FD_SYNC, data, &reply);
        return reply.readInt32();
    }

    status_t setOutputFileSync(const char* path, int64_t fallocateLength, int muxerId)
    {
        ALOGV("setOutputFileSync(%s, %lld, %d)", path, fallocateLength, muxerId);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        if(path != NULL)
        {
            data.writeInt32(true);
            data.writeCString(path);
            data.writeInt64(fallocateLength);
        }
        else
        {
            data.writeInt32(false);
        }
        data.writeInt32(muxerId);
        remote()->transact(SET_OUTPUT_FILE_FD_SYNC_URL, data, &reply);
        return reply.readInt32();
    }

    status_t setSdcardState(bool bExist)
    {
        ALOGV("setSdcardState(%d)", bExist);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
		data.writeInt32(bExist);
		remote()->transact(SET_SDCARD_STATE, data, &reply);
		return reply.readInt32();
    }

    int addOutputFormatAndOutputSink(int of, int fd, int FallocateLen, bool callback_out_flag)
    {
        ALOGV("add OutputFormatAndOutputSink, output_format[%d], fd[%d], FallocateLen[%d], callback_out_flag[%d]", of, fd, FallocateLen, callback_out_flag);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        data.writeInt32(of);
        if (fd >= 0) 
        {
            data.writeInt32(true);
            data.writeFileDescriptor(fd);
            data.writeInt32(FallocateLen);
        } 
        else 
        {
            data.writeInt32(false);
        }
        data.writeInt32(callback_out_flag);
        remote()->transact(ADD_OUTPUTFORMAT_AND_OUTPUTSINK, data, &reply);
        return reply.readInt32();
    }
    int addOutputFormatAndOutputSink(int of, const char* path, int FallocateLen, bool callback_out_flag)
    {
        ALOGV("add OutputFormatAndOutputSink, output_format[%d], path[%s], FallocateLen[%d], callback_out_flag[%d]", of, path, FallocateLen, callback_out_flag);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        data.writeInt32(of);
        if (path!=NULL) 
        {
            data.writeInt32(true);
            data.writeCString(path);
            data.writeInt32(FallocateLen);
        } 
        else 
        {
            data.writeInt32(false);
        }
        data.writeInt32(callback_out_flag);
        remote()->transact(ADD_OUTPUTFORMAT_AND_OUTPUTSINK_URL, data, &reply);
        return reply.readInt32();
    }
    status_t removeOutputFormatAndOutputSink(int muxerId)
    {
        ALOGV("removeOutputFormatAndOutputSink, muxerId[%d]", muxerId);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        data.writeInt32(muxerId);
        remote()->transact(REMOVE_OUTPUTFORMAT_AND_OUTPUTSINK, data, &reply);
        return reply.readInt32();
    }

	status_t setMaxDuration(int max_duration_ms)
	{
        ALOGV("setMaxDuration");
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        data.writeInt32(max_duration_ms);
        remote()->transact(SET_MAX_DURATION, data, &reply);
        return reply.readInt32();

	}
	
	status_t setMaxFileSize(int64_t max_filesize_bytes)
	{
        ALOGV("setMaxFileSize");
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        data.writeInt64(max_filesize_bytes);
        remote()->transact(SET_MAX_FILE_SIZE, data, &reply);
        return reply.readInt32();
	}

	status_t setImpactOutputFile(int fd, int64_t fallocateLength, int muxerId)
	{
        ALOGV("setImpactOutputFile(%d, %lld, %d)", fd, fallocateLength, muxerId);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        if (fd >= 0) 
        {
            data.writeInt32(true);
            data.writeFileDescriptor(fd);
            data.writeInt64(fallocateLength);
        } 
        else 
        {
            data.writeInt32(false);
        }
        data.writeInt32(muxerId);
        remote()->transact(SET_IMPACT_OUTPUT_FILE, data, &reply);
        return reply.readInt32();
	}

    status_t setImpactOutputFile(const char* path, int64_t fallocateLength, int muxerId)
	{
        ALOGV("setImpactOutputFile(%s, %lld, %d)", path, fallocateLength, muxerId);
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorder::getInterfaceDescriptor());
        if (path != NULL)
        {
            data.writeInt32(true);
            data.writeCString(path);
            data.writeInt64(fallocateLength);
        } 
        else 
        {
            data.writeInt32(false);
        }
        data.writeInt32(muxerId);
        remote()->transact(SET_IMPACT_OUTPUT_FILE_URL, data, &reply);
        return reply.readInt32();
	}
};

IMPLEMENT_META_INTERFACE(MediaRecorder, "android.media.IMediaRecorder");

// ----------------------------------------------------------------------

status_t BnMediaRecorder::onTransact(
                                     uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch (code) {
        case RELEASE: {
            ALOGV("RELEASE");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            reply->writeInt32(release());
            return NO_ERROR;
        } break;
        case INIT: {
            ALOGV("INIT");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            reply->writeInt32(init());
            return NO_ERROR;
        } break;
        case CLOSE: {
            ALOGV("CLOSE");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            reply->writeInt32(close());
            return NO_ERROR;
        } break;
        case RESET: {
            ALOGV("RESET");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            reply->writeInt32(reset());
            return NO_ERROR;
        } break;
        case STOP: {
            ALOGV("STOP");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            reply->writeInt32(stop());
            return NO_ERROR;
        } break;
        case START: {
            ALOGV("START");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            reply->writeInt32(start());
            return NO_ERROR;
        } break;
        case PREPARE: {
            ALOGV("PREPARE");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            reply->writeInt32(prepare());
            return NO_ERROR;
        } break;
        case GET_MAX_AMPLITUDE: {
            ALOGV("GET_MAX_AMPLITUDE");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            int max = 0;
            status_t ret = getMaxAmplitude(&max);
            reply->writeInt32(max);
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case GET_ONE_BSFRAME: {
        	ALOGV("GET_ONE_BSFRAME");
			CHECK_INTERFACE(IMediaRecorder, data, reply);
			int mode = data.readInt32();
            sp<IMemory> bitmap = getOneBsFrame(mode);
            if (bitmap != NULL) {  // Don't send NULL across the binder interface
                reply->writeInt32(NO_ERROR);
                reply->writeStrongBinder(bitmap->asBinder());
            } else {
                reply->writeInt32(UNKNOWN_ERROR);
            }
			return NO_ERROR;
        } break;
		case FREE_ONE_BSFRAME: {
        	ALOGV("FREE_ONE_BSFRAME");
			CHECK_INTERFACE(IMediaRecorder, data, reply);
			freeOneBsFrame();
			return NO_ERROR;
		} break;
        case GET_ENC_DATA_HEADER: {
        	ALOGV("GET_ENC_DATA_HEADER");
			CHECK_INTERFACE(IMediaRecorder, data, reply);
			int mode = data.readInt32();
            sp<IMemory> header = getEncDataHeader();
            if (header != NULL) {  // Don't send NULL across the binder interface
                reply->writeInt32(NO_ERROR);
                reply->writeStrongBinder(header->asBinder());
            } else {
                reply->writeInt32(UNKNOWN_ERROR);
            }
			return NO_ERROR;
        } break;
		case SET_VIDEO_ENCODEING_BITRATE_SYNC: {
            ALOGV("SET_VIDEO_ENCODEING_BITRATE_SYNC");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
			int bitRate = data.readInt32();
			reply->writeInt32(setVideoEncodingBitRateSync(bitRate));
			return NO_ERROR;
		} break;
		case SET_VIDEO_FRAME_RATE_SYNC: {
            ALOGV("SET_VIDEO_FRAME_RATE_SYNC");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
			int frames_per_second = data.readInt32();
			reply->writeInt32(setVideoFrameRateSync(frames_per_second));
			return NO_ERROR;
		} break;
        case SET_VIDEO_SOURCE: {
            ALOGV("SET_VIDEO_SOURCE");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            int vs = data.readInt32();
            reply->writeInt32(setVideoSource(vs));
            return NO_ERROR;
        } break;
        case SET_AUDIO_SOURCE: {
            ALOGV("SET_AUDIO_SOURCE");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            int as = data.readInt32();
            reply->writeInt32(setAudioSource(as));
            return NO_ERROR;
        } break;
        case SET_OUTPUT_FORMAT: {
            ALOGV("SET_OUTPUT_FORMAT");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            int of = data.readInt32();
            reply->writeInt32(setOutputFormat(of));
            return NO_ERROR;
        } break;
        case SET_VIDEO_ENCODER: {
            ALOGV("SET_VIDEO_ENCODER");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            int ve = data.readInt32();
            reply->writeInt32(setVideoEncoder(ve));
            return NO_ERROR;
        } break;
        case SET_AUDIO_ENCODER: {
            ALOGV("SET_AUDIO_ENCODER");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            int ae = data.readInt32();
            reply->writeInt32(setAudioEncoder(ae));
            return NO_ERROR;

        } break;
        case SET_OUTPUT_FILE_PATH: {
            ALOGV("SET_OUTPUT_FILE_PATH");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            const char* path = data.readCString();
            int64_t offset = data.readInt64();
            int64_t length = data.readInt64();
            reply->writeInt32(setOutputFile(path, offset, length));
            return NO_ERROR;
        } break;
        case SET_OUTPUT_FILE_FD: {
            ALOGV("SET_OUTPUT_FILE_FD");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            int fd = dup(data.readFileDescriptor());
            int64_t offset = data.readInt64();
            int64_t length = data.readInt64();
            reply->writeInt32(setOutputFile(fd, offset, length));
            ::close(fd);
            return NO_ERROR;
        } break;
        case SET_VIDEO_SIZE: {
            ALOGV("SET_VIDEO_SIZE");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            int width = data.readInt32();
            int height = data.readInt32();
            reply->writeInt32(setVideoSize(width, height));
            return NO_ERROR;
        } break;
        case SET_VIDEO_FRAMERATE: {
            ALOGV("SET_VIDEO_FRAMERATE");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            int frames_per_second = data.readInt32();
            reply->writeInt32(setVideoFrameRate(frames_per_second));
            return NO_ERROR;
        } break;
        case SET_MUTE_MODE: {
            ALOGV("SET_MUTE_MODE");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            bool mute = data.readInt32();
            reply->writeInt32(setMuteMode(mute));
            return NO_ERROR;
        } break;
        case SET_PARAMETERS: {
            ALOGV("SET_PARAMETER");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            reply->writeInt32(setParameters(data.readString8()));
            return NO_ERROR;
        } break;
        case SET_LISTENER: {
            ALOGV("SET_LISTENER");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            sp<IMediaRecorderClient> listener =
                interface_cast<IMediaRecorderClient>(data.readStrongBinder());
            reply->writeInt32(setListener(listener));
            return NO_ERROR;
        } break;
        case SET_PREVIEW_SURFACE: {
            ALOGV("SET_PREVIEW_SURFACE");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
//            sp<Surface> surface = Surface::readFromParcel(data);
//            reply->writeInt32(setPreviewSurface(surface));
            unsigned int hlay = data.readInt32();
            reply->writeInt32(setPreviewSurface(hlay));
            return NO_ERROR;
        } break;
        case SET_CAMERA: {
            ALOGV("SET_CAMERA");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            sp<ICamera> camera = interface_cast<ICamera>(data.readStrongBinder());
            sp<ICameraRecordingProxy> proxy =
                interface_cast<ICameraRecordingProxy>(data.readStrongBinder());
            reply->writeInt32(setCamera(camera, proxy));
            return NO_ERROR;
        } break;
        case QUERY_SURFACE_MEDIASOURCE: {
            ALOGV("QUERY_SURFACE_MEDIASOURCE");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            // call the mediaserver side to create
            // a surfacemediasource
            //sp<ISurfaceTexture> surfaceMediaSource = querySurfaceMediaSource();
            unsigned int hlay = querySurfaceMediaSource();
            // The mediaserver might have failed to create a source
            //int returnedNull= (surfaceMediaSource == NULL) ? 1 : 0 ;
            int returnedNull= (hlay < 100) ? 1 : 0 ;
            reply->writeInt32(returnedNull);
            if (!returnedNull) {
                //reply->writeStrongBinder(surfaceMediaSource->asBinder());
                reply->writeInt32(hlay);
            }
            return NO_ERROR;
        } break;
        case QUEUE_BUFFER: {
        	ALOGV("QUEUE_BUFFER");
			CHECK_INTERFACE(IMediaRecorder, data, reply);
			int32_t index  = data.readInt32();
			int32_t addr_y = data.readInt32();
			int32_t addr_c = data.readInt32();
			int64_t timestamp = data.readInt64();
			reply->writeInt32(queueBuffer(index, addr_y, addr_c, timestamp));
            return NO_ERROR;
        } break;
        case SET_VIDEO_ENCODING_IFRAMES_NUMBER_INTERVAL_SYNC: {
            ALOGV("SET_VIDEO_ENCODING_IFRAMES_NUMBER_INTERVAL_SYNC");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            int nMaxKeyItl = data.readInt32();
            reply->writeInt32(setVideoEncodingIFramesNumberIntervalSync(nMaxKeyItl));
            return NO_ERROR;
        } break;
        case REENCODE_IFRAME: {
            ALOGV("REENCODE_IFRAME");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            reply->writeInt32(reencodeIFrame());
            return NO_ERROR;
        } break;
        case SET_OUTPUT_FILE_FD_SYNC: {
            ALOGV("SET_OUTPUT_FILE_FD_SYNC");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            int fd = data.readFileDescriptor();
            int64_t fallocateLength = data.readInt64();
            int muxerId = data.readInt32();
            reply->writeInt32(setOutputFileSync(fd, fallocateLength, muxerId));
            return NO_ERROR;
        } break;
        case SET_OUTPUT_FILE_FD_SYNC_URL: {
            ALOGV("SET_OUTPUT_FILE_FD_SYNC_URL");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            char* path = NULL;
            int64_t fallocateLength = 0;
            if(data.readInt32() == true)
            {
                path = (char*)data.readCString();
                fallocateLength = data.readInt64();
            }
            int muxerId = data.readInt32();
            reply->writeInt32(setOutputFileSync(path, fallocateLength, muxerId));
            return NO_ERROR;
        } break;
        case SET_SDCARD_STATE: {
            ALOGV("SET_SDCARD_STATE");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            bool bExist = data.readInt32();
            reply->writeInt32(setSdcardState(bExist));
            return NO_ERROR;
        } break;
        case ADD_OUTPUTFORMAT_AND_OUTPUTSINK:
        {
            ALOGV("ADD OUTPUTFORMAT AND OUTPUTSINK");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            int of = data.readInt32();
            int fd = -1;
            int FallocateLen = 0;
            bool callback_out_flag = 0;
            if (data.readInt32() == true)
            {
				fd = data.readFileDescriptor();
                FallocateLen = data.readInt32();
			}
            callback_out_flag = data.readInt32();
            reply->writeInt32(addOutputFormatAndOutputSink(of, fd, FallocateLen, callback_out_flag));
			return NO_ERROR;
        } break;
        case ADD_OUTPUTFORMAT_AND_OUTPUTSINK_URL:
        {
            ALOGV("ADD OUTPUTFORMAT AND OUTPUTSINK_URL");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            int of = data.readInt32();
            char* path = NULL;
            int FallocateLen = 0;
            bool callback_out_flag = 0;
            if (data.readInt32() == true)
            {
                path = (char*)data.readCString();
                FallocateLen = data.readInt32();
			}
            callback_out_flag = data.readInt32();
            reply->writeInt32(addOutputFormatAndOutputSink(of, path, FallocateLen, callback_out_flag));
			return NO_ERROR;
        } break;
        case REMOVE_OUTPUTFORMAT_AND_OUTPUTSINK:
        {
            ALOGV("REMOVE OUTPUTFORMAT AND OUTPUTSINK");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            int muxerId = data.readInt32();
            reply->writeInt32(removeOutputFormatAndOutputSink(muxerId));
			return NO_ERROR;
        } break;
		case SET_MAX_DURATION:
		{
            ALOGV("SET_MAX_DURATION");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            int32_t duration = data.readInt32();
            reply->writeInt32(setMaxDuration(duration));
            return NO_ERROR;
		} break;
		case SET_MAX_FILE_SIZE:
		{
            ALOGV("SET_MAX_FILE_SIZE");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            int32_t filesize = data.readInt64();
            reply->writeInt32(setMaxFileSize(filesize));
            return NO_ERROR;
		} break;
		case SET_IMPACT_OUTPUT_FILE: {
            ALOGV("SET_IMPACT_OUTPUT_FILE");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            int fd = -1;
            int64_t fallocateLength = 0;
            if (data.readInt32() == true)
            {
				fd = data.readFileDescriptor();
                fallocateLength = data.readInt64();
			}
            int muxerId = data.readInt32();
            reply->writeInt32(setImpactOutputFile(fd, fallocateLength, muxerId));
            return NO_ERROR;
		} break;
        case SET_IMPACT_OUTPUT_FILE_URL: {
            ALOGV("SET_IMPACT_OUTPUT_FILE_URL");
            CHECK_INTERFACE(IMediaRecorder, data, reply);
            char* path = NULL;
            int64_t fallocateLength = 0;
            if (data.readInt32() == true)
            {
                path = (char*)data.readCString();
                fallocateLength = data.readInt64();
			}
            int muxerId = data.readInt32();
            reply->writeInt32(setImpactOutputFile(path, fallocateLength, muxerId));
            return NO_ERROR;
		} break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

// ----------------------------------------------------------------------------

}; // namespace android
