/*
**
** Copyright (C) 2008, The Android Open Source Project
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

#ifndef ANDROID_SERVERS_DISPLAY_DISPLAYSERVICE_H
#define ANDROID_SERVERS_DISPLAY_DISPLAYSERVICE_H

#include <binder/BinderService.h>
#include <display/IDisplayService.h>
#include <hardware/display.h>

/* This needs to be increased if we can have more displays */
#define MAX_DISPLAYS 2

namespace android {

extern volatile int32_t gLogLevel;

class DisplayService :
    public BinderService<DisplayService>,
    public BnDisplayService,
    public IBinder::DeathRecipient
{
    friend class BinderService<DisplayService>;
public:
    class Client;
    static char const* getServiceName() { return "media.display"; }
                        DisplayService();
    virtual             ~DisplayService();
    virtual sp<IDisplay> connect(const sp<IDisplayClient>& displayClient, int displayId);
	virtual void        removeClient(const sp<IDisplayClient>& displayClient);
    virtual status_t    onTransact(uint32_t code, const Parcel& data,
                                   Parcel* reply, uint32_t flags);
    virtual void onFirstRef();

    class Client : public BnDisplay
    {
    public:
        // IDisplay interface (see IDisplay for details)
        virtual void          disconnect();
        virtual status_t      connect(const sp<IDisplayClient>& client) = 0;
        // Interface used by DisplayService
        Client();
		Client(const sp<DisplayService>& displayService,
                const sp<IDisplayClient>& displayClient,
                int displayId,
                int clientPid,
                int servicePid);
        ~Client();
		virtual status_t initialize() = 0;
		const sp<IDisplayClient>&    getDisplayClient() {
            return mDisplayClient;
        }

        // these are initialized in the constructor.
        sp<DisplayService>               mDisplayService;  // immutable after constructor
        sp<IDisplayClient>               mDisplayClient;
        int                             mDisplayId;       // immutable after constructor
        pid_t                           mClientPid;
        pid_t                           mServicePid;     // immutable after constructor

    };

private:
    Mutex               mServiceLock;
    wp<Client>          mClient[MAX_DISPLAYS];  // protected by mServiceLock
    int                 mNumberOfDisplays;
	// needs to be called with mServiceLock held
    sp<Client>          findClientUnsafe(const wp<IBinder>& displayClient, int& outIndex);

    // atomics to record whether the hardware is allocated to some client.
    volatile int32_t    mBusy[MAX_DISPLAYS];
    void                setDisplayBusy(int displayId);
    void                setDisplayFree(int displayId);

    // IBinder::DeathRecipient implementation
    virtual void binderDied(const wp<IBinder> &who);
};

} // namespace android

#endif
