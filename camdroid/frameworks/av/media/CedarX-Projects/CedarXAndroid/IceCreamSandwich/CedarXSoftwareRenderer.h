/*
 * Copyright (C) 2009 The Android Open Source Project
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

#ifndef SOFTWARE_RENDERER_H_

#define SOFTWARE_RENDERER_H_

#include <utils/RefBase.h>
#if (CEDARX_ANDROID_VERSION < 7)
#include <ui/android_native_buffer.h>
#endif
//#include <hardware/hwcomposer.h>
//#include "virtual_hwcomposer.h"
#include <CDX_PlayerAPI.h>

#define ADAPT_A10_GPU_RENDER (1)
#define ADAPT_A31_GPU_RENDER (1)

namespace android {

struct MetaData;

class CedarXSoftwareRenderer {
public:
//    CedarXSoftwareRenderer(
//            const sp<ANativeWindow> &nativeWindow, const sp<MetaData> &meta);
    CedarXSoftwareRenderer(const unsigned int hlay, const sp<MetaData> &meta);

    ~CedarXSoftwareRenderer();

    void render(
            const void *data, size_t size, void *platformPrivate);
    int dequeueFrame(ANativeWindowBufferCedarXWrapper *pObject);
    int enqueueFrame(ANativeWindowBufferCedarXWrapper *pObject);

private:
    enum YUVMode {
        None,
    };

    //OMX_COLOR_FORMATTYPE mColorFormat;
    YUVMode mYUVMode;
    //sp<ANativeWindow> mNativeWindow;
    unsigned int mVideoLayerId;
    int32_t mWidth, mHeight;
    //int32_t mCropLeft, mCropTop, mCropRight, mCropBottom;
    //int32_t mCropWidth, mCropHeight;

    CedarXSoftwareRenderer(const CedarXSoftwareRenderer &);
    CedarXSoftwareRenderer &operator=(const CedarXSoftwareRenderer &);
};

}  // namespace android

#endif  // SOFTWARE_RENDERER_H_
