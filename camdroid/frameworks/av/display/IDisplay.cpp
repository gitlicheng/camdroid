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
#define LOG_TAG "IDisplay"
#include <utils/Log.h>
#include <stdint.h>
#include <sys/types.h>
#include <binder/Parcel.h>
#include <display/IDisplay.h>
#include <include_media/media/hwdisp_def.h>



namespace android {

enum {
    DISCONNECT = IBinder::FIRST_CALL_TRANSACTION,
    CONNECT,
    REQUEST_SURFACE,
    RELEASE_SURFACE,
    SET_PREVIEWRECT,
    SET_PREVIEWBOTTOM,
    EXCHAGE_SURFACE,
    OPEN_SURFACE,
    SECOND_SCREEN,
    CLEAR_SURFACE,
    SET_PREVIEWTOP,
};

class BpDisplay: public BpInterface<IDisplay>
{
public:
    BpDisplay(const sp<IBinder>& impl)
        : BpInterface<IDisplay>(impl)
    {
    }

    // disconnect from display service
    void disconnect()
    {
        
        Parcel data, reply;
        data.writeInterfaceToken(IDisplay::getInterfaceDescriptor());
        remote()->transact(DISCONNECT, data, &reply);
    }
	virtual status_t connect(const sp<IDisplayClient>& displayClient)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IDisplay::getInterfaceDescriptor());
        data.writeStrongBinder(displayClient->asBinder());
        remote()->transact(CONNECT, data, &reply);
        return reply.readInt32();
    }
	
	status_t requestSurface(const unsigned int sur)
    {
        ALOGV("requestSurface");
        Parcel data, reply;
		struct view_info *pSur = (struct view_info *)sur;
        data.writeInterfaceToken(IDisplay::getInterfaceDescriptor());
		data.writeInt32(pSur->x);
		data.writeInt32(pSur->y);
		data.writeInt32(pSur->w);
		data.writeInt32(pSur->h);
        remote()->transact(REQUEST_SURFACE, data, &reply);
        return reply.readInt32();
    }

    status_t setPreviewBottom(const unsigned int hlay)
    {
        ALOGV("setPreviewBottom");
        Parcel data, reply;
        data.writeInterfaceToken(IDisplay::getInterfaceDescriptor());
		data.writeInt32(hlay);
        remote()->transact(SET_PREVIEWBOTTOM, data, &reply);
        return reply.readInt32();
    }


	status_t setPreviewTop(const unsigned int hlay)
	 {
		 ALOGV("setPreviewTop");
		 Parcel data, reply;
		 data.writeInterfaceToken(IDisplay::getInterfaceDescriptor());
		 data.writeInt32(hlay);
		 remote()->transact(SET_PREVIEWTOP, data, &reply);
		 return reply.readInt32();
	 }


    status_t setPreviewRect(const unsigned int pRect)
    {
        ALOGV("setPreviewRect");
		struct view_info *rect = (struct view_info *)pRect;
        Parcel data, reply;
        data.writeInterfaceToken(IDisplay::getInterfaceDescriptor());
		data.writeInt32(rect->x);
		data.writeInt32(rect->y);
		data.writeInt32(rect->w);
		data.writeInt32(rect->h);
        remote()->transact(SET_PREVIEWRECT, data, &reply);
        return reply.readInt32();
    }

    status_t releaseSurface(const unsigned int hlay)
    {
        ALOGV("releaseSurface");
        Parcel data, reply;
        data.writeInterfaceToken(IDisplay::getInterfaceDescriptor());
		data.writeInt32(hlay);
        remote()->transact(RELEASE_SURFACE, data, &reply);
        return reply.readInt32();
    }

	status_t exchangeSurface(const unsigned int hlay1, const unsigned int hlay2, const int otherOnTop)
	{
		Parcel data, reply;
        data.writeInterfaceToken(IDisplay::getInterfaceDescriptor());
		data.writeInt32(hlay1);
		data.writeInt32(hlay2);
		data.writeInt32(otherOnTop);
        remote()->transact(EXCHAGE_SURFACE, data, &reply);
        return reply.readInt32();
	}

	status_t clearSurface(const unsigned int hlay1)
	{
		Parcel data, reply;
        data.writeInterfaceToken(IDisplay::getInterfaceDescriptor());
		data.writeInt32(hlay1);
        remote()->transact(CLEAR_SURFACE, data, &reply);
        return reply.readInt32();
	}
	
	status_t otherScreen(const int screen, const unsigned int hlay1, const unsigned int hlay2)
	{
		Parcel data, reply;
        data.writeInterfaceToken(IDisplay::getInterfaceDescriptor());
		data.writeInt32(screen);
		data.writeInt32(hlay1);
		data.writeInt32(hlay2);
        remote()->transact(SECOND_SCREEN, data, &reply);
        return reply.readInt32();
	}

	status_t openSurface(const unsigned int hlay, const int open)
	{
		Parcel data, reply;
		data.writeInterfaceToken(IDisplay::getInterfaceDescriptor());
		data.writeInt32(hlay);
		data.writeInt32(open);
		remote()->transact(OPEN_SURFACE, data, &reply);
		return reply.readInt32();
	}
};

IMPLEMENT_META_INTERFACE(Display, "android.hardware.IDisplay");

// ----------------------------------------------------------------------

status_t BnDisplay::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
	
    switch(code) {
        case DISCONNECT: {
            ALOGV("DISCONNECT");
            CHECK_INTERFACE(IDisplay, data, reply);
            disconnect();
            return NO_ERROR;
        } break;
        case CONNECT: {
            CHECK_INTERFACE(IDisplay, data, reply);
            sp<IDisplayClient> displayClient = interface_cast<IDisplayClient>(data.readStrongBinder());
            reply->writeInt32(connect(displayClient));
            return NO_ERROR;
        } break;
		case REQUEST_SURFACE: {
			ALOGV("REQUEST_SURFACE");
            CHECK_INTERFACE(IDisplay, data, reply);
			struct view_info sur;
			sur.x 		= data.readInt32();
			sur.y 		= data.readInt32();
			sur.w 		= data.readInt32();
			sur.h 		= data.readInt32();
            reply->writeInt32(requestSurface((unsigned int)&sur));
            return NO_ERROR;
		}break;
		case SET_PREVIEWRECT: {
			ALOGV("SET_PREVIEWRECT");
            CHECK_INTERFACE(IDisplay, data, reply);
			struct view_info rect;
			rect.x 	= data.readInt32();
			rect.y  = data.readInt32();
			rect.w 	= data.readInt32();
			rect.h 	= data.readInt32();
            reply->writeInt32(setPreviewRect((unsigned int)&rect));
            return NO_ERROR;
		}break;
		case SET_PREVIEWBOTTOM: {
			ALOGV("SET_PREVIEWBOTTOM");
            CHECK_INTERFACE(IDisplay, data, reply);
			unsigned int hlay;
			hlay = data.readInt32();
            reply->writeInt32(setPreviewBottom(hlay));
            return NO_ERROR;
		}break;
		case RELEASE_SURFACE: {
			ALOGV("RELEASE_SURFACE");
            CHECK_INTERFACE(IDisplay, data, reply);
			unsigned int hlay;
			hlay = data.readInt32();
            reply->writeInt32(releaseSurface(hlay));
            return NO_ERROR;
		}break;
		case EXCHAGE_SURFACE: {
			CHECK_INTERFACE(IDisplay, data, reply);
			unsigned int hlay1;
			unsigned int hlay2;
			int otherOnTop;
			hlay1 = data.readInt32();
			hlay2 = data.readInt32();
			otherOnTop = data.readInt32();
            reply->writeInt32(exchangeSurface(hlay1, hlay2, otherOnTop));
			return NO_ERROR;
		}break;
		case CLEAR_SURFACE: {
			CHECK_INTERFACE(IDisplay, data, reply);
			unsigned int hlay1;
			hlay1 = data.readInt32();
            reply->writeInt32(clearSurface(hlay1));
			return NO_ERROR;
		}break;
		case OPEN_SURFACE: {
			CHECK_INTERFACE(IDisplay, data, reply);
			unsigned int hlay;
			int open;
			hlay = data.readInt32();
			open = data.readInt32();
			reply->writeInt32(openSurface(hlay, open));
			return NO_ERROR;
		}break;
		case SECOND_SCREEN: {
			CHECK_INTERFACE(IDisplay, data, reply);
			unsigned int hlay1;
			unsigned int hlay2;
			int screen;
			screen = data.readInt32();
			hlay1 = data.readInt32();
			hlay2 = data.readInt32();
            reply->writeInt32(otherScreen(screen, hlay1, hlay2));
			return NO_ERROR;
		}break;
		case SET_PREVIEWTOP: {
			ALOGV("SET_PREVIEWBOTTOM");
            CHECK_INTERFACE(IDisplay, data, reply);
			unsigned int hlay;
			hlay = data.readInt32();
            reply->writeInt32(setPreviewTop(hlay));
            return NO_ERROR;
		}break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

// ----------------------------------------------------------------------------

}; // namespace android
