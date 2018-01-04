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

#ifndef ANDROID_SERVERS_DISPLAY_DISPLAYCLIENT_H
#define ANDROID_SERVERS_DISPLAY_DISPLAYCLIENT_H

#include "DisplayService.h"
#include <hwdisplay.h>

namespace android {

class HwDisplay;
	
class DisplayClient : public DisplayService::Client
{
public:
    // IDisplay interface (see IDisplay for details)
    virtual void            disconnect();
    virtual status_t        connect(const sp<IDisplayClient>& client);
    // Interface used by DisplayService
    DisplayClient(const sp<DisplayService>& displayService,
            const sp<IDisplayClient>& displayClient,
            int displayId,
            int clientPid,
            int servicePid);
    ~DisplayClient();
	status_t initialize();
	status_t requestSurface(const unsigned int sur);
	status_t setPreviewBottom(const unsigned int hlay);
	status_t setPreviewTop(const unsigned int hlay);
	status_t releaseSurface(const unsigned int hlay);
	status_t setPreviewRect(const unsigned int pRect);
	status_t exchangeSurface(const unsigned int hlay1, const unsigned int hlay2, const int otherOnTop);
	status_t otherScreen(const int screen, const unsigned int hlay1, const unsigned int hlay2);
	status_t openSurface(const unsigned int hlay, const int open);
	status_t clearSurface(const unsigned int hlay1);
private:
	status_t                checkPid() const;
	mutable Mutex                   mLock;

	HwDisplay* mHwDisplay;
	int mHlay;
};

}

#endif
