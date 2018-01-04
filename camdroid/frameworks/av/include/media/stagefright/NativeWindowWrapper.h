/*
 * Copyright (C) 2011 The Android Open Source Project
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

#ifndef NATIVE_WINDOW_WRAPPER_H_

#define NATIVE_WINDOW_WRAPPER_H_

//#include <gui/SurfaceTextureClient.h>

namespace android {

// SurfaceTextureClient derives from ANativeWindow which derives from multiple
// base classes, in order to carry it in AMessages, we'll temporarily wrap it
// into a NativeWindowWrapper.

struct NativeWindowWrapper : RefBase {
    NativeWindowWrapper(
            const unsigned int layerId) :
        mLayerId(layerId) { }

    unsigned int getNativeWindow() const {
        return mLayerId;
    }

    unsigned int getSurfaceTextureClient() const {
        return mLayerId;
    }

private:
    //const sp<SurfaceTextureClient> mSurfaceTextureClient;
    const unsigned int mLayerId;

    DISALLOW_EVIL_CONSTRUCTORS(NativeWindowWrapper);
};

}  // namespace android

#endif  // NATIVE_WINDOW_WRAPPER_H_
