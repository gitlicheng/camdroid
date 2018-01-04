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

#ifndef CEDARXNATIVE_RENDERER_H_

#define CEDARXNATIVE_RENDERER_H_


#include <utils/RefBase.h>
#if (CEDARX_ANDROID_VERSION < 7)
#include <ui/android_native_buffer.h>
#endif
//#include <hardware/hwcomposer.h>
#include <hwdisplay.h>

#include "virtual_hwcomposer.h"

namespace android {

struct MetaData;
//class CedarXNativeRendererAdapter;

class CedarXNativeRenderer
{
public:
    //CedarXNativeRenderer(const sp<ANativeWindow> &nativeWindow, const sp<MetaData> &meta);
    CedarXNativeRenderer(const unsigned int hlay, const sp<MetaData> &meta);

    ~CedarXNativeRenderer();

    void render(const void *data, size_t size, void *platformPrivate);

    int control(int cmd, int para);

private:
    //OMX_COLOR_FORMATTYPE mColorFormat;
    //sp<ANativeWindow> mNativeWindow;
    HwDisplay* mHwDisplay;
    unsigned int mVideoLayerId;
    int32_t mWidth, mHeight;
    int32_t mCropLeft, mCropTop, mCropRight, mCropBottom;
    int32_t mCropWidth, mCropHeight;
    int32_t mLayerShowed;

    CedarXNativeRenderer(const CedarXNativeRenderer &);
    CedarXNativeRenderer &operator=(const CedarXNativeRenderer &);
    //friend class CedarXNativeRendererAdapter;
    //CedarXNativeRendererAdapter *pCedarXNativeRendererAdapter;
};


//class CedarXNativeRendererAdapter  //base adapter, its implementation is put in different AdapterDirectories, selected to compile by makefile according to android version.
//{
//public:
//    CedarXNativeRendererAdapter(CedarXNativeRenderer *vRender);
//    virtual ~CedarXNativeRendererAdapter();
//    int CedarXNativeRendererAdapterIoCtrl(int cmd, int aux, void *pbuffer);  //cmd = CedarXNativeRendererAdapterCmd
//    
//private:
//    CedarXNativeRenderer* const pCedarXNativeRenderer; //CedarXNativeRenderer pointer
//};

}  // namespace android

#endif  // SOFTWARE_RENDERER_H_
