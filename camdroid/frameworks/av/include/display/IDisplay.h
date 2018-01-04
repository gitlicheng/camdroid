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

#ifndef ANDROID_HARDWARE_IDISPLAY_H
#define ANDROID_HARDWARE_IDISPLAY_H

#include <binder/IInterface.h>
#include <display/Display.h>

namespace android {

class IDisplayClient;

class IDisplay: public IInterface
{
public:
    DECLARE_META_INTERFACE(Display);

    virtual void            disconnect() = 0;
    // connect new client with existing display remote
    virtual status_t        connect(const sp<IDisplayClient>& client) = 0;
	virtual status_t        requestSurface(const unsigned int sur) = 0;
	virtual status_t        setPreviewBottom(const unsigned int hlay) = 0;
	virtual status_t        setPreviewTop(const unsigned int hlay) = 0;
	virtual status_t        setPreviewRect(const unsigned int pRect) = 0;
	virtual status_t        releaseSurface(const unsigned int hlay) = 0;
	virtual status_t        exchangeSurface(const unsigned int hlay1, const unsigned int hlay2, const int otherOnTop) = 0;
	virtual status_t        clearSurface(const unsigned int hlay1) = 0;
	virtual status_t		otherScreen(const int screen, const unsigned int hlay1, const unsigned int hlay2) = 0;
	virtual status_t		openSurface(const unsigned int hlay, const int open) = 0;
};

// ----------------------------------------------------------------------------

class BnDisplay: public BnInterface<IDisplay>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};

}; // namespace android

#endif
