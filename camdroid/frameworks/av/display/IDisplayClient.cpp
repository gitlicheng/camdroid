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
#define LOG_TAG "IDisplayClient"
#include <utils/Log.h>
#include <stdint.h>
#include <sys/types.h>
#include <display/IDisplayClient.h>

namespace android {

enum {
    NOTIFY_CALLBACK = IBinder::FIRST_CALL_TRANSACTION,
};

class BpDisplayClient: public BpInterface<IDisplayClient>
{
public:
    BpDisplayClient(const sp<IBinder>& impl)
        : BpInterface<IDisplayClient>(impl)
    {
    }
};

IMPLEMENT_META_INTERFACE(DisplayClient, "android.hardware.IDisplayClient");

// ----------------------------------------------------------------------

status_t BnDisplayClient::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch(code) {
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

// ----------------------------------------------------------------------------

}; // namespace android

