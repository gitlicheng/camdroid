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
 
 /**
 * Information about a HwDisplay
 */
#ifndef __CEDARMEDIADISPLAY_H__
#define __CEDARMEDIADISPLAY_H__

//#include "jni.h"
//#include "JNIHelp.h"
//#include "android_runtime/AndroidRuntime.h"

//#include <cutils/properties.h>

#include <utils/Mutex.h>
#include <utils/Vector.h>
#include <utils/KeyedVector.h>
#include <utils/String8.h>

#include <ui/Rect.h>
//#include <gui/SurfaceTexture.h>
//#include <gui/Surface.h>
//#include <binder/IMemory.h>
//#include <include_media/media/MediaCallbackDispatcher.h>
#include <include_media/media/hwdisp_def.h>

namespace android {

class CedarDisplay
{
public:
	CedarDisplay(int displayId);
    ~CedarDisplay();
    unsigned int requestSurface(view_info *sur);
    status_t setPreviewBottom(unsigned int hlay);    //setPreviewBottom
    status_t setPreviewTop(unsigned int hlay);    //setPreviewTop
    status_t setPreviewRect(view_info *pRect);       //setPreviewRect
    status_t releaseSurface(unsigned int hlay);
	status_t exchangeSurface(unsigned int hlay1, unsigned int hlay2, int otherOnTop); //if other layer on the top, eg. layer->fb0
	status_t otherScreen(int screen, unsigned int hlay1, unsigned int hlay2);
	status_t open(unsigned int hlay, int open);
	status_t openAdasScreen(unsigned int hlay, int open);	
	status_t clearSurface(unsigned int hlay1);
private:
	int mId;
    static const String8 TAG;   // = "CedarHwLayer";
    void    *mNativeContext;
    Vector<unsigned int> mhlayVector;    //for debug, record current hlays which are requested.
};

};

#endif  /* __CEDARMEDIADISPLAY_H__ */

