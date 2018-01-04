/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "ICameraRecordingProxy"
#include <camera/ICameraRecordingProxy.h>
#include <camera/ICameraRecordingProxyListener.h>
#include <binder/IMemory.h>
#include <binder/Parcel.h>
#include <stdint.h>
#include <utils/Log.h>
#include <utils/String8.h>

namespace android {

enum {
    START_RECORDING = IBinder::FIRST_CALL_TRANSACTION,
    STOP_RECORDING,
    RELEASE_RECORDING_FRAME,
    SET_PREVIEW_DISPLAY,
    SEND_COMMAND,
    LOCK,
    UNLOCK,
    GET_PARAMETERS,
    SET_PARAMETERS,
};


class BpCameraRecordingProxy: public BpInterface<ICameraRecordingProxy>
{
public:
    BpCameraRecordingProxy(const sp<IBinder>& impl)
        : BpInterface<ICameraRecordingProxy>(impl)
    {
    }

    status_t startRecording(const sp<ICameraRecordingProxyListener>& listener, unsigned int recorderId)
    {
        ALOGV("startRecording");
        Parcel data, reply;
        data.writeInterfaceToken(ICameraRecordingProxy::getInterfaceDescriptor());
		data.writeInt32(recorderId);
        data.writeStrongBinder(listener->asBinder());
        remote()->transact(START_RECORDING, data, &reply);
        return reply.readInt32();
    }

    void stopRecording(unsigned int recorderId)
    {
        ALOGV("stopRecording");
        Parcel data, reply;
        data.writeInterfaceToken(ICameraRecordingProxy::getInterfaceDescriptor());
		data.writeInt32(recorderId);
        remote()->transact(STOP_RECORDING, data, &reply);
    }

    void releaseRecordingFrame(const sp<IMemory>& mem, int bufIdx)
    {
        ALOGV("releaseRecordingFrame");
        Parcel data, reply;
        data.writeInterfaceToken(ICameraRecordingProxy::getInterfaceDescriptor());
        data.writeInt32(bufIdx);
        data.writeStrongBinder(mem->asBinder());
        remote()->transact(RELEASE_RECORDING_FRAME, data, &reply);
    }

	status_t setPreviewDisplay(const unsigned int hlay)
	{
        ALOGV("setPreviewDisplay");
        Parcel data, reply;
        data.writeInterfaceToken(ICameraRecordingProxy::getInterfaceDescriptor());
		data.writeInt32(hlay);
		remote()->transact(SET_PREVIEW_DISPLAY, data, &reply);
		return reply.readInt32();
	}

	status_t sendCommand(int32_t cmd, int32_t arg1, int32_t arg2)
	{
        ALOGV("sendCommand");
        Parcel data, reply;
        data.writeInterfaceToken(ICameraRecordingProxy::getInterfaceDescriptor());
		data.writeInt32(cmd);
		data.writeInt32(arg1);
		data.writeInt32(arg2);
		remote()->transact(SEND_COMMAND, data, &reply);
		return reply.readInt32();
	}
	status_t lock()
	{
        ALOGV("lock");
        Parcel data, reply;
        data.writeInterfaceToken(ICameraRecordingProxy::getInterfaceDescriptor());
        remote()->transact(LOCK, data, &reply);
		return reply.readInt32();
	}
	status_t unlock()
	{
        ALOGV("unlock");
        Parcel data, reply;
        data.writeInterfaceToken(ICameraRecordingProxy::getInterfaceDescriptor());
        remote()->transact(UNLOCK, data, &reply);
		return reply.readInt32();
	}

	String8 getParameters() const
	{
        ALOGV("getParameters");
        Parcel data, reply;
        data.writeInterfaceToken(ICameraRecordingProxy::getInterfaceDescriptor());
        remote()->transact(GET_PARAMETERS, data, &reply);
		return reply.readString8();
	}

	status_t setParameters(const String8& params)
	{
        ALOGV("setParameters");
        Parcel data, reply;
        data.writeInterfaceToken(ICameraRecordingProxy::getInterfaceDescriptor());
        data.writeString8(params);
        remote()->transact(SET_PARAMETERS, data, &reply);
        return reply.readInt32();
	}
};

IMPLEMENT_META_INTERFACE(CameraRecordingProxy, "android.hardware.ICameraRecordingProxy");

// ----------------------------------------------------------------------

status_t BnCameraRecordingProxy::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch(code) {
        case START_RECORDING: {
            ALOGV("START_RECORDING");
            CHECK_INTERFACE(ICameraRecordingProxy, data, reply);
			unsigned int id = data.readInt32();
            sp<ICameraRecordingProxyListener> listener =
                interface_cast<ICameraRecordingProxyListener>(data.readStrongBinder());
            reply->writeInt32(startRecording(listener, id));
            return NO_ERROR;
        } break;
        case STOP_RECORDING: {
            ALOGV("STOP_RECORDING");
            CHECK_INTERFACE(ICameraRecordingProxy, data, reply);
			unsigned int id = data.readInt32();
            stopRecording(id);
            return NO_ERROR;
        } break;
        case RELEASE_RECORDING_FRAME: {
            ALOGV("RELEASE_RECORDING_FRAME");
            CHECK_INTERFACE(ICameraRecordingProxy, data, reply);
            int index = data.readInt32();
            sp<IMemory> mem = interface_cast<IMemory>(data.readStrongBinder());
            releaseRecordingFrame(mem, index);
            return NO_ERROR;
        } break;
		case SET_PREVIEW_DISPLAY: {
            ALOGV("SET_PREVIEW_DISPLAY");
            CHECK_INTERFACE(ICameraRecordingProxy, data, reply);
			int hlay = data.readInt32();
			reply->writeInt32(setPreviewDisplay(hlay));
			return NO_ERROR;
		} break;
		case SEND_COMMAND: {
            ALOGV("SEND_COMMAND");
            CHECK_INTERFACE(ICameraRecordingProxy, data, reply);
			int32_t cmd = data.readInt32();
			int32_t arg1 = data.readInt32();
			int32_t arg2 = data.readInt32();
			reply->writeInt32(sendCommand(cmd, arg1, arg2));
			return NO_ERROR;
		} break;
		case LOCK: {
            ALOGV("LOCK");
            CHECK_INTERFACE(ICameraRecordingProxy, data, reply);
			reply->writeInt32(lock());
			return NO_ERROR;
		} break;
		case UNLOCK: {
            ALOGV("UNLOCK");
            CHECK_INTERFACE(ICameraRecordingProxy, data, reply);
			reply->writeInt32(unlock());
			return NO_ERROR;
		} break;
		case GET_PARAMETERS: {
            ALOGV("GET_PARAMETERS");
            CHECK_INTERFACE(ICameraRecordingProxy, data, reply);
			reply->writeString8(getParameters());
			return NO_ERROR;
		} break;
		case SET_PARAMETERS: {
            ALOGV("SET_PARAMETERS");
            CHECK_INTERFACE(ICameraRecordingProxy, data, reply);
            String8 params(data.readString8());
            reply->writeInt32(setParameters(params));
			return NO_ERROR;
		} break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

// ----------------------------------------------------------------------------

}; // namespace android

