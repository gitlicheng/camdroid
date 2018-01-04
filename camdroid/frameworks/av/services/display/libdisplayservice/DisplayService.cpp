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

#define LOG_TAG "DisplayService"
//#define LOG_NDEBUG 0

#include <stdio.h>
#include <sys/types.h>
#include <pthread.h>

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
#include <cutils/atomic.h>
#include <cutils/properties.h>
//#include <gui/SurfaceTextureClient.h>
//#include <gui/Surface.h>
#include <hardware/hardware.h>
#include <media/AudioSystem.h>
#include <media/mediaplayer.h>
#include <utils/Errors.h>
#include <utils/Log.h>
#include <utils/String16.h>

#include "DisplayService.h"
#include "DisplayClient.h"
//#include "Display2Client.h"

namespace android {

// ----------------------------------------------------------------------------
// Logging support -- this is for debugging only
// Use "adb shell dumpsys media.display -v 1" to change it.
volatile int32_t gLogLevel = 0;

#define LOG1(...) ALOGD_IF(gLogLevel >= 1, __VA_ARGS__);
#define LOG2(...) ALOGD_IF(gLogLevel >= 2, __VA_ARGS__);
// ----------------------------------------------------------------------------

static int getCallingPid() {
	
    return IPCThreadState::self()->getCallingPid();
}

static int getCallingUid() {
	
    return IPCThreadState::self()->getCallingUid();
}


// ----------------------------------------------------------------------------

// This is ugly and only safe if we never re-create the DisplayService, but
// should be ok for now.
static DisplayService *gDisplayService;

DisplayService::DisplayService()
{
	ALOGI("DisplayService started (pid=%d)", getpid());
	gDisplayService = this;
}

void DisplayService::onFirstRef()
{
	
	BnDisplayService::onFirstRef();
	mNumberOfDisplays = MAX_DISPLAYS;
	for (int i = 0; i < mNumberOfDisplays; i++) {
    	setDisplayFree(i);
    }
}

DisplayService::~DisplayService() {
	
	for (int i = 0; i < mNumberOfDisplays; i++) {
        if (mBusy[i]) {
            ALOGE("display %d is still in use in destructor!", i);
        }
    }

    gDisplayService = NULL;
}

// The reason we need this busy bit is a new DisplayService::connect() request
// may come in while the previous Client's destructor has not been run or is
// still running. If the last strong reference of the previous Client is gone
// but the destructor has not been finished, we should not allow the new Client
// to be created because we need to wait for the previous Client to tear down
// the hardware first.
void DisplayService::setDisplayBusy(int displayId) {
	

    android_atomic_write(1, &mBusy[displayId]);

    ALOGV("setDisplayBusy displayId=%d", displayId);
}

void DisplayService::setDisplayFree(int displayId) {
	
    android_atomic_write(0, &mBusy[displayId]);

    ALOGV("setDisplayFree displayId=%d", displayId);
}

sp<IDisplay> DisplayService::connect(
        const sp<IDisplayClient>& displayClient, int displayId) {

	
	int callingPid = getCallingPid();
    LOG1("DisplayService::connect E (pid %d)", callingPid);

    sp<Client> client;
	if (displayId < 0 || displayId >= mNumberOfDisplays) {
        ALOGE("DisplayService::connect X (pid %d) rejected (invalid displayId %d).",
            callingPid, displayId);
        return NULL;
    }
    Mutex::Autolock lock(mServiceLock);
    if (mClient[displayId] != 0) {
        client = mClient[displayId].promote();
        if (client != 0) {
            if (displayClient->asBinder() == client->getDisplayClient()->asBinder()) {
                LOG1("DisplayService::connect X (pid %d) (the same client)",
                     callingPid);
                return client;
            } else {
                ALOGW("DisplayService::connect X (pid %d) rejected (existing client).",
                      callingPid);
                return NULL;
            }
        }
        mClient[displayId].clear();
    }

    if (mBusy[displayId]) {
        ALOGW("DisplayService::connect X (pid %d) rejected"
                " (display %d is still busy).", callingPid, displayId);
        return NULL;
    }
	
    client = new DisplayClient(this, displayClient, displayId, callingPid, getpid());
    if (client->initialize() != OK) {
        return NULL;
    }

    displayClient->asBinder()->linkToDeath(this);

    mClient[displayId] = client;
    LOG1("DisplayService::connect X (id %d, this pid is %d)", displayId, getpid());
    return client;
	
}

status_t DisplayService::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags) {
    
    // Permission checks
    switch (code) {
        case BnDisplayService::CONNECT:
            const int pid = getCallingPid();
            const int self_pid = getpid();
            if (pid != self_pid) {
                // we're called from a different process, do the real check
                if (!checkCallingPermission(
                        String16("android.permission.DISPLAY"))) {
                    const int uid = getCallingUid();
                    ALOGE("Permission Denial: "
                         "can't use the display pid=%d, uid=%d", pid, uid);
                    return PERMISSION_DENIED;
                }
            }
            break;
    }

    return BnDisplayService::onTransact(code, data, reply, flags);
}

sp<DisplayService::Client> DisplayService::findClientUnsafe(
                        const wp<IBinder>& displayClient, int& outIndex) {
    sp<Client> client;
	
    for (int i = 0; i < mNumberOfDisplays; i++) {

        // This happens when we have already disconnected (or this is
        // just another unused display).
        if (mClient[i] == 0) continue;

        // Promote mClient. It can fail if we are called from this path:
        // Client::~Client() -> disconnect() -> removeClient().
        client = mClient[i].promote();

        // Clean up stale client entry
        if (client == NULL) {
            mClient[i].clear();
            continue;
        }

        if (displayClient == client->getDisplayClient()->asBinder()) {
            // Found our display
            outIndex = i;
            return client;
        }
    }

    outIndex = -1;
    return NULL;
}

void DisplayService::removeClient(const sp<IDisplayClient>& displayClient) {
	
    int callingPid = getCallingPid();
    LOG1("DisplayService::removeClient E (pid %d)", callingPid);

    // Declare this before the lock to make absolutely sure the
    // destructor won't be called with the lock held.
    Mutex::Autolock lock(mServiceLock);

    int outIndex;
    sp<Client> client = findClientUnsafe(displayClient->asBinder(), outIndex);

    if (client != 0) {
        // Found our display, clear and leave.
        LOG1("removeClient: clear display %d", outIndex);
        mClient[outIndex].clear();

        client->unlinkToDeath(this);
    }

    LOG1("DisplayService::removeClient X (pid %d)", callingPid);
}

DisplayService::Client::Client(const sp<DisplayService>& displayService,
        const sp<IDisplayClient>& displayClient,
        int displayId, int clientPid, int servicePid) {
    int callingPid = getCallingPid();
	
    LOG1("Client::Client E (pid %d, id %d)", callingPid, displayId);

    mDisplayService = displayService;
    mDisplayClient = displayClient;
    mDisplayId = displayId;
    mClientPid = clientPid;
    mServicePid = servicePid;
    displayService->setDisplayBusy(displayId);
    LOG1("Client::Client X (pid %d, id %d)", callingPid, displayId);
}

// tear down the client
DisplayService::Client::~Client() {
	
    // unconditionally disconnect. function is idempotent
    Client::disconnect();
}

// ----------------------------------------------------------------------------

// NOTE: function is idempotent
void DisplayService::Client::disconnect() {
	
	mDisplayService->removeClient(mDisplayClient);
    mDisplayService->setDisplayFree(mDisplayId);
}

// ----------------------------------------------------------------------------
/*virtual*/void DisplayService::binderDied(
    const wp<IBinder> &who) {
	
    /**
      * While tempting to promote the wp<IBinder> into a sp,
      * it's actually not supported by the binder driver
      */

    ALOGV("java clients' binder died");

}

}; // namespace android
