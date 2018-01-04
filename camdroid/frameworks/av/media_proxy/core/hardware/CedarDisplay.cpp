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

//#define LOG_NDEBUG 0
#define LOG_TAG "CedarDisplay"
#include <utils/Log.h>

#include <cutils/properties.h>
#include <utils/Vector.h>
#include <utils/RefBase.h>

//#include <gui/SurfaceTexture.h>
//#include <gui/Surface.h>
//#include <binder/IMemory.h>
#include <display/Display.h>
#include <CedarDisplay.h>

namespace android {
const String8 CedarDisplay::TAG("CedarDisplay");

CedarDisplay::CedarDisplay(int displayId)
{
    sp<Display> mediaDisplay = Display::connect(displayId);
	
    if (mediaDisplay == NULL) {
        //jniThrowRuntimeException(env, "Fail to connect to camera service");
        ALOGE("(f:%s, l:%d) Fail to connect to Display service", __FUNCTION__, __LINE__);
        return;
    }
	
    mediaDisplay->incStrong(this);
	
    // save context in opaque field
    mNativeContext = (void*)mediaDisplay.get();
}

CedarDisplay::~CedarDisplay()
{
    // clean up if release has not been called before
    Display*   pDisplay = reinterpret_cast<Display*>(mNativeContext);
    if (pDisplay != NULL) {
        pDisplay->disconnect();
        // remove context to prevent further Java access
        pDisplay->decStrong(this);
    }
}

unsigned int CedarDisplay::requestSurface(view_info *sur)
{
    ALOGV("requestSurface");
	
    sp<Display> mediaDisplay = reinterpret_cast<Display*>(mNativeContext);
	
    if (mediaDisplay == 0) return BAD_VALUE;
	
	unsigned int hlay = mediaDisplay->requestSurface((unsigned int)sur);
    mhlayVector.add(hlay);
    return hlay;
}

status_t CedarDisplay::setPreviewBottom(unsigned int hlay)
{
    ALOGV("setPreviewBottom");
    sp<Display> mediaDisplay = reinterpret_cast<Display*>(mNativeContext);
    if (mediaDisplay == 0) return BAD_VALUE;
	
	return mediaDisplay->setPreviewBottom(hlay);
}

status_t CedarDisplay::setPreviewTop(unsigned int hlay)
{
    ALOGV("setPreviewBottom");
    sp<Display> mediaDisplay = reinterpret_cast<Display*>(mNativeContext);
    if (mediaDisplay == 0) return BAD_VALUE;
	
	return mediaDisplay->setPreviewTop(hlay);
}


status_t CedarDisplay::setPreviewRect(view_info *pRect)
{
    ALOGV("setPreviewRect");
    sp<Display> mediaDisplay = reinterpret_cast<Display*>(mNativeContext);
    if (mediaDisplay == 0) return BAD_VALUE;
	
	return mediaDisplay->setPreviewRect((unsigned int)pRect);
}

status_t CedarDisplay::releaseSurface(unsigned int hlay)
{
    ALOGV("releaseSurface");
    sp<Display> mediaDisplay = reinterpret_cast<Display*>(mNativeContext);
    if (mediaDisplay == 0) return BAD_VALUE;
    size_t i=0;
    for(i=0;i < mhlayVector.size(); i++)
    {
        if(mhlayVector[i] == hlay)
        {
            break;
        }
    }
    if(i == mhlayVector.size())
    {
        ALOGE("(f:%s, l:%d) not find hlay[%d] in hlayVector_size[%d]", __FUNCTION__, __LINE__, hlay, mhlayVector.size());
        return BAD_VALUE;
    }
    else
    {
        mhlayVector.removeAt(i);
	    return mediaDisplay->releaseSurface(hlay);
    }
}

status_t CedarDisplay::exchangeSurface(unsigned int hlay1, unsigned int hlay2, int otherOnTop)
{
	sp<Display> mediaDisplay = reinterpret_cast<Display*>(mNativeContext);
    if (mediaDisplay == 0) return BAD_VALUE;

	return mediaDisplay->exchangeSurface(hlay1, hlay2, otherOnTop);
}

status_t CedarDisplay::clearSurface(unsigned int hlay1)
{
	sp<Display> mediaDisplay = reinterpret_cast<Display*>(mNativeContext);
    if (mediaDisplay == 0) return BAD_VALUE;

	return mediaDisplay->clearSurface(hlay1);
}

status_t CedarDisplay::otherScreen(int screen, unsigned int hlay1, unsigned int hlay2)
{
	sp<Display> mediaDisplay = reinterpret_cast<Display*>(mNativeContext);
    if (mediaDisplay == 0) return BAD_VALUE;

	return mediaDisplay->otherScreen(screen, hlay1, hlay2);
}
status_t CedarDisplay::open(unsigned int hlay, int open)
{
	sp<Display> mediaDisplay = reinterpret_cast<Display*>(mNativeContext);
    if (mediaDisplay == 0) return BAD_VALUE;

	return mediaDisplay->open(hlay, open);
}

status_t CedarDisplay::openAdasScreen(unsigned int hlay, int open)
{
	sp<Display> mediaDisplay = reinterpret_cast<Display*>(mNativeContext);
    if (mediaDisplay == 0) return BAD_VALUE;

	return mediaDisplay->open(hlay, open);
}


}; // namespace android

