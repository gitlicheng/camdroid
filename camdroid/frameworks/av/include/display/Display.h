/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_DISPLAY_H
#define ANDROID_HARDWARE_DISPLAY_H

#include <display/IDisplayClient.h>

namespace android {

class IDisplayService;
class IDisplay;
class Mutex;
class String8;

class Display : public BnDisplayClient, public IBinder::DeathRecipient
{
public:
            // construct a display client from an existing remote
    static  sp<Display>  create(const sp<IDisplay>& display);
    static  sp<Display>  connect(int displayId);
            virtual     ~Display();
            void        init();

            void        disconnect();
            status_t    getStatus() { return mStatus; }
    sp<IDisplay>         remote();
	status_t	requestSurface(const unsigned int sur);
    status_t	setPreviewBottom(const unsigned int hlay);
	status_t	setPreviewTop(const unsigned int hlay);
	status_t	setPreviewRect(const unsigned int pRect);
	status_t	releaseSurface(const unsigned int hlay);
	status_t    exchangeSurface(unsigned int hlay1, unsigned int hlay2, int otherOnTop);
	status_t	otherScreen(int screen, unsigned int hlay1, unsigned int hlay2);
	status_t 	open(unsigned int hlay, int open);
	status_t    openAdasScreen(unsigned int hlay, int open);
	status_t	clearSurface(unsigned int hlay1);
private:
                        Display();
                        Display(const Display&);
                        virtual void binderDied(const wp<IBinder>& who);

            class DeathNotifier: public IBinder::DeathRecipient
            {
            public:
                DeathNotifier() {
                }

                virtual void binderDied(const wp<IBinder>& who);
            };

            static sp<DeathNotifier> mDeathNotifier;

            // helper function to obtain display service handle
            static const sp<IDisplayService>& getDisplayService();

            sp<IDisplay>         mDisplay;
            status_t            mStatus;

            friend class DeathNotifier;

            static  Mutex               mLock;
            static  sp<IDisplayService>  mDisplayService;
};

}; // namespace android

#endif
