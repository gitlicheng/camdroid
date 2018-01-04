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

#include <stdint.h>
#include <sys/types.h>

#include <binder/Parcel.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

#include <display/IDisplayService.h>

namespace android {

class BpDisplayService: public BpInterface<IDisplayService>
{
public:
    BpDisplayService(const sp<IBinder>& impl)
        : BpInterface<IDisplayService>(impl)
    {
    }

    // connect to display service
    virtual sp<IDisplay> connect(const sp<IDisplayClient>& displayClient, int displayId)
    {
    	
        Parcel data, reply;
        data.writeInterfaceToken(IDisplayService::getInterfaceDescriptor());
        data.writeStrongBinder(displayClient->asBinder());
		data.writeInt32(displayId);
        remote()->transact(BnDisplayService::CONNECT, data, &reply);
        return interface_cast<IDisplay>(reply.readStrongBinder());
    }
};

IMPLEMENT_META_INTERFACE(DisplayService, "android.hardware.IDisplayService");

// ----------------------------------------------------------------------

status_t BnDisplayService::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch(code) {
        case CONNECT: {
            CHECK_INTERFACE(IDisplayService, data, reply);
            sp<IDisplayClient> displayClient = interface_cast<IDisplayClient>(data.readStrongBinder());
            sp<IDisplay> display = connect(displayClient, data.readInt32());
            reply->writeStrongBinder(display->asBinder());
            return NO_ERROR;
        } break;
        /* add by Michael. end   -----------------------------------}} */
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

// ----------------------------------------------------------------------------

}; // namespace android
