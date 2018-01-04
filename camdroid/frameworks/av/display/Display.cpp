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

#define LOG_NDEBUG 0
#define LOG_TAG "Display"
#include <utils/Log.h>
#include <utils/threads.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/IMemory.h>

#include <display/Display.h>
#include <display/IDisplayService.h>

//#include <gui/ISurfaceTexture.h>
//#include <gui/Surface.h>

namespace android {

// client singleton for display service binder interface
Mutex Display::mLock;
sp<IDisplayService> Display::mDisplayService;
sp<Display::DeathNotifier> Display::mDeathNotifier;

// establish binder interface to display service
const sp<IDisplayService>& Display::getDisplayService()
{
	
    Mutex::Autolock _l(mLock);
    if (mDisplayService.get() == 0) {
        sp<IServiceManager> sm = defaultServiceManager();
        sp<IBinder> binder;
        do {
            binder = sm->getService(String16("media.display"));
            if (binder != 0)
                break;
            ALOGW("DisplayService not published, waiting...");
            usleep(500000); // 0.5 s
        } while(true);
        if (mDeathNotifier == NULL) {
            mDeathNotifier = new DeathNotifier();
        }
        binder->linkToDeath(mDeathNotifier);
        mDisplayService = interface_cast<IDisplayService>(binder);
    }
    ALOGE_IF(mDisplayService==0, "no DisplayService!?");
    return mDisplayService;
}

// ---------------------------------------------------------------------------

Display::Display()
{
	
    init();
}

void Display::init()
{
	
    mStatus = UNKNOWN_ERROR;
}

Display::~Display()
{
	
    // We don't need to call disconnect() here because if the DisplayService
    // thinks we are the owner of the hardware, it will hold a (strong)
    // reference to us, and we can't possibly be here. We also don't want to
    // call disconnect() here if we are in the same process as mediaserver,
    // because we may be invoked by DisplayService::Client::connect() and will
    // deadlock if we call any method of IDisplay here.
}

sp<Display> Display::connect(int displayId)
{
    
    sp<Display> d = new Display();
	const sp<IDisplayService>& ds = getDisplayService();
    if (ds != 0) {
        d->mDisplay = ds->connect(d, displayId);
    }
    if (d->mDisplay != 0) {
        d->mDisplay->asBinder()->linkToDeath(d);
        d->mStatus = NO_ERROR;
    } else {
        d.clear();
    }
    return d;
}

void Display::disconnect()
{
   
    if (mDisplay != 0) {
        mDisplay->disconnect();
        mDisplay->asBinder()->unlinkToDeath(this);
        mDisplay = 0;
    }
}

sp<IDisplay> Display::remote()
{
	
    return mDisplay;
}

void Display::binderDied(const wp<IBinder>& who)
{
    ALOGW("IDisplay died");
}

void Display::DeathNotifier::binderDied(const wp<IBinder>& who)
{
	
    ALOGV("binderDied");
    Mutex::Autolock _l(Display::mLock);
    Display::mDisplayService.clear();
    ALOGW("Display server died!");
}

status_t Display::requestSurface(const unsigned int sur)
{
    sp <IDisplay> d = mDisplay;
    if (d == 0) return NO_INIT;
	return d->requestSurface(sur);
}

status_t Display::setPreviewBottom(const unsigned int hlay)
{	
    sp <IDisplay> d = mDisplay;
    if (d == 0) return NO_INIT;
	return d->setPreviewBottom(hlay);
}

status_t Display::setPreviewTop(const unsigned int hlay)
{	
    sp <IDisplay> d = mDisplay;
    if (d == 0) return NO_INIT;
	return d->setPreviewTop(hlay);
}


status_t Display::setPreviewRect(const unsigned int pRect)
{	
    sp <IDisplay> d = mDisplay;
    if (d == 0) return NO_INIT;
	return d->setPreviewRect(pRect);
}



status_t Display::releaseSurface(const unsigned int hlay)
{	
    sp <IDisplay> d = mDisplay;
    if (d == 0) return NO_INIT;
	return d->releaseSurface(hlay);
}

status_t Display::exchangeSurface(unsigned int hlay1, unsigned int hlay2, int otherOnTop)
{
	sp <IDisplay> d = mDisplay;
    if (d == 0) return NO_INIT;
	return d->exchangeSurface(hlay1, hlay2, otherOnTop);
}

status_t Display::clearSurface(unsigned int hlay1)
{
	sp <IDisplay> d = mDisplay;
    if (d == 0) return NO_INIT;
	return d->clearSurface(hlay1);
}

status_t Display::otherScreen(int screen, unsigned int hlay1, unsigned int hlay2)
{
	sp <IDisplay> d = mDisplay;
    if (d == 0) return NO_INIT;
	return d->otherScreen(screen, hlay1, hlay2);
}

status_t Display::open(unsigned int hlay, int open)
{
	sp <IDisplay> d = mDisplay;
    if (d == 0) return NO_INIT;
	return d->openSurface(hlay, open);
}

status_t Display::openAdasScreen(unsigned int hlay, int open)
{
	sp <IDisplay> d = mDisplay;
    if (d == 0) return NO_INIT;
	return d->openSurface(hlay, open);
}

}; // namespace android
