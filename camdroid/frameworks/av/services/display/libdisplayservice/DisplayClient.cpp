/*
 * Copyright (C) 2012 The Android Open Source Project
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

#define LOG_TAG "DisplayClient"
//#define LOG_NDEBUG 0

#include <cutils/properties.h>
//#include <gui/SurfaceTextureClient.h>
//#include <gui/Surface.h>

#include "DisplayClient.h"
#include "DisplayService.h"

namespace android {

#define LOG1(...) ALOGD_IF(gLogLevel >= 1, __VA_ARGS__);
#define LOG2(...) ALOGD_IF(gLogLevel >= 2, __VA_ARGS__);

static int getCallingPid() {
	
    return IPCThreadState::self()->getCallingPid();
}

static int getCallingUid() {
	
    return IPCThreadState::self()->getCallingUid();
}

status_t DisplayClient::initialize() {
	ALOGE("%s %d\n", __FUNCTION__, __LINE__);
    return OK;
}

status_t DisplayClient::checkPid() const {
	
    int callingPid = getCallingPid();
    if (callingPid == mClientPid) return NO_ERROR;

    ALOGW("attempt to use a locked display from a different process"
         " (old pid %d, new pid %d)", mClientPid, callingPid);
    return EBUSY;
}

DisplayClient::DisplayClient(const sp<DisplayService>& displayService,
		const sp<IDisplayClient>& displayClient,
		int displayId, int clientPid, int servicePid):
		Client(displayService, displayClient,
				displayId, clientPid, servicePid)
{
	
	mHlay = 0;
	mHwDisplay = HwDisplay::getInstance();
}


// tear down the client
DisplayClient::~DisplayClient() {
	
    int callingPid = getCallingPid();
	mHwDisplay->hwd_layer_release(mHlay);
    LOG1("DisplayClient::~DisplayClient E (pid %d, this %p)", callingPid, this);
    LOG1("DisplayClient::~DisplayClient X (pid %d, this %p)", callingPid, this);
}

// connect a new client to the display
status_t DisplayClient::connect(const sp<IDisplayClient>& client) {
	

	int callingPid = getCallingPid();
    LOG1("connect E (pid %d)", callingPid);
    Mutex::Autolock lock(mLock);

    if (mClientPid != 0 && checkPid() != NO_ERROR) {
        ALOGW("Tried to connect to a locked display (old pid %d, new pid %d)",
                mClientPid, callingPid);
        return EBUSY;
    }

    if (mDisplayClient != 0 && (client->asBinder() == mDisplayClient->asBinder())) {
        LOG1("Connect to the same client");
        return NO_ERROR;
    }

    mClientPid = callingPid;
    mDisplayClient = client;

    LOG1("connect X (pid %d)", callingPid);
    return NO_ERROR;
}

void DisplayClient::disconnect() {
	
	int callingPid = getCallingPid();
    LOG1("disconnect E (pid %d)", callingPid);
    Mutex::Autolock lock(mLock);

    // Allow both client and the media server to disconnect at all times
    if (callingPid != mClientPid && callingPid != mServicePid) {
        ALOGW("different client - don't disconnect");
        return;
    }

    if (mClientPid <= 0) {
        LOG1("display is unlocked (mClientPid = %d), don't tear down hardware", mClientPid);
        return;
    }
    LOG1("hardware teardown");
    DisplayService::Client::disconnect();
    LOG1("disconnect X (pid %d)", callingPid);
}

status_t DisplayClient::requestSurface(const unsigned int sur)
{ALOGE("%s %d\n", __FUNCTION__, __LINE__);
	mHlay = mHwDisplay->hwd_layer_request((struct view_info *)sur);
	return mHlay;
}

status_t DisplayClient::setPreviewBottom(const unsigned int hlay)
{
	return mHwDisplay->hwd_layer_bottom(hlay);
}

status_t DisplayClient::setPreviewTop(const unsigned int hlay)
{
	return mHwDisplay->hwd_layer_top(hlay);
}


status_t DisplayClient::setPreviewRect(const unsigned int pRect)
{
	return mHwDisplay->hwd_layer_set_rect(mHlay, (struct view_info *)pRect);
}


status_t DisplayClient::releaseSurface(const unsigned int hlay)
{
   return mHwDisplay->hwd_layer_release(mHlay);
}

status_t DisplayClient::exchangeSurface(const unsigned int hlay1, const unsigned int hlay2, const int otherOnTop)
{
	return mHwDisplay->hwd_layer_exchange(hlay1, hlay2, otherOnTop);
}

status_t DisplayClient::clearSurface(const unsigned int hlay1)
{
	return mHwDisplay->hwd_layer_clear(hlay1);
}

status_t DisplayClient::otherScreen(int screen, unsigned int hlay1, unsigned int hlay2)
{
	return mHwDisplay->hwd_layer_other_screen(screen, hlay1, hlay2);
}

status_t DisplayClient::openSurface(const unsigned int hlay, const int open)
{
	if (open) {
		return mHwDisplay->hwd_layer_open(hlay);
	} else {
		return mHwDisplay->hwd_layer_close(hlay);
	}
}


}; // namespace android
