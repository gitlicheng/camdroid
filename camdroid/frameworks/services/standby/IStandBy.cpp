/*
**
** Copyright 2007, The Android Open Source Project
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

#define LOG_TAG "IStandBy"
//#define LOG_NDEBUG 0
#include <utils/Log.h>

#include <StandBy.h>


namespace android {

enum {
    ENTER_STANDBY = IBinder::FIRST_CALL_TRANSACTION,
};

class BpStandbyService : public BpInterface<IStandbyService>
{
public:
    BpStandbyService(const sp<IBinder>& impl)
        : BpInterface<IStandbyService>(impl)
    {
    }

    virtual  int enterStandby( int args)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IStandbyService::getInterfaceDescriptor());
	data.writeInt32(args);
        status_t lStatus = remote()->transact(ENTER_STANDBY, data, &reply);
	int result = reply.readInt32();
	return result;
    }
};

IMPLEMENT_META_INTERFACE(StandbyService, "android.media.IStandbyService");

// ----------------------------------------------------------------------

status_t BnStandbyService::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch (code) {
        case ENTER_STANDBY: {
		CHECK_INTERFACE(IStandbyService, data, reply);
		int  args = data.readInt32();
		int result = enterStandby(args);
		reply->writeInt32(result);
            return NO_ERROR;
        } break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

// ----------------------------------------------------------------------------

}; // namespace android

