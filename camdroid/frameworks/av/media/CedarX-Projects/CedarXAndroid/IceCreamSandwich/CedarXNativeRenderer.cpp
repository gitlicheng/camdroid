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

#define LOG_TAG "CedarXNativeRenderer"
#include <CDX_Debug.h>

#include "CedarXNativeRenderer.h"
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MetaData.h>

#include <binder/MemoryHeapBase.h>
#if (CEDARX_ANDROID_VERSION < 7)
#include <binder/MemoryHeapPmem.h>
#include <surfaceflinger/Surface.h>
#include <ui/android_native_buffer.h>
#endif
//#include <virtual_hwcomposer.h>
//#include <ui/GraphicBufferMapper.h>
//#include <gui/ISurfaceTexture.h>

namespace android {

//CedarXNativeRenderer::CedarXNativeRenderer(const sp<ANativeWindow> &nativeWindow, const sp<MetaData> &meta)
//    : mNativeWindow(nativeWindow)
CedarXNativeRenderer::CedarXNativeRenderer(const unsigned int hlay, const sp<MetaData> &meta)
    : mVideoLayerId(hlay)
{
    int32_t halFormat,screenID; //halFormat = e_hwc_format <==> VirtualHWCRenderFormat
    size_t bufWidth, bufHeight;

    //CHECK(meta->findInt32(kKeyScreenID, &screenID));
    CHECK(meta->findInt32(kKeyColorFormat, &halFormat));
    CHECK(meta->findInt32(kKeyWidth, &mWidth)); //width and height is display_width and display_height.
    CHECK(meta->findInt32(kKeyHeight, &mHeight));

    int32_t rotationDegrees;
    if (!meta->findInt32(kKeyRotation, &rotationDegrees))
        rotationDegrees = 0;
	mHwDisplay = HwDisplay::getInstance();
		
    //when YV12:
    //vdec_buffer's Y must be 16 pixel align in width, 16 lines align in height. (16*16)
    //vdec_buffer's V now is 16 pixel align in width, 8 lines align in height(16*8). we want to change to 8pixel*8line(8*8)!
    
    //when MB32: one Y_MB is 32pixel * 32 lines. spec is 16*16, but hw decoder extend to 32*32!
    //vdec_buffer's Y must be 32pixel * 32line(32*32)
    //vdec_buffer's uv must be 32byte * 32line(32*32). uv combined to store, so if u is a byte, v is a byte, then 32byte for 16 pixel_point. 
    //  uv may be large than y, so uv MB will partly be filled with stuf data.
    
//    bufWidth = (mWidth + 15) & ~15;  //the vdec_buffer's width and height which will be told to display device .
//    bufHeight = (mHeight + 15) & ~15;
    bufWidth = mWidth;  //the vdec_buffer's width and height which will be told to display device .
    bufHeight = mHeight;
    if(bufWidth != ((mWidth + 15) & ~15))
    {
        LOGW("(f:%s, l:%d) bufWidth[%d]!=display_width[%d]", __FUNCTION__, __LINE__, ((mWidth + 15) & ~15), mWidth);
    }
    if(bufHeight != ((mHeight + 15) & ~15))
    {
        LOGW("(f:%s, l:%d) bufHeight[%d]!=display_height[%d]", __FUNCTION__, __LINE__, ((mHeight + 15) & ~15), mHeight);
    }

    //CHECK(mNativeWindow != NULL);
    CHECK(mVideoLayerId >= 0);
	struct src_info src;
	src.w = ALIGN16(mWidth);
	src.h = ALIGN16(mHeight);
    src.crop_x = 0;
    src.crop_y = 0;
    src.crop_w = mWidth;
	src.crop_h = mHeight;
	src.format = halFormat;
	mHwDisplay->hwd_layer_set_src(mVideoLayerId, &src);
#if 0
    if(halFormat == HAL_PIXEL_FORMAT_YV12)
    {
        ALOGE("(f:%s, l:%d) fatal error! can't be HAL_PIXEL_FORMAT_YV12", __FUNCTION__, __LINE__);
        #if 0
        native_window_set_usage(mNativeWindow.get(),
        		                GRALLOC_USAGE_SW_READ_NEVER  |
        		                GRALLOC_USAGE_SW_WRITE_OFTEN |
        		                GRALLOC_USAGE_HW_TEXTURE     |
        		                GRALLOC_USAGE_EXTERNAL_DISP);

//        native_window_set_scaling_mode(mNativeWindow.get(), NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW);

        // Width must be multiple of 32???
        native_window_set_buffers_geometry(mNativeWindow.get(),
        								   bufWidth,
        								   bufHeight,
                                           0x58);   //HWC_FORMAT_YUV420PLANAR

        uint32_t transform;
        switch (rotationDegrees)
        {
            case 0: transform = 0; break;
            case 90: transform = HAL_TRANSFORM_ROT_90; break;
            case 180: transform = HAL_TRANSFORM_ROT_180; break;
            case 270: transform = HAL_TRANSFORM_ROT_270; break;
            default: transform = 0; break;
        }

        if (transform)
            native_window_set_buffers_transform(mNativeWindow.get(), transform);

        #endif
    }
    else
    {
        ALOGE("(f:%s, l:%d) fatal error! need hwLayer operation", __FUNCTION__, __LINE__);
        //fot test
        #if 0
        native_window_set_usage(mNativeWindow.get(),
        		                GRALLOC_USAGE_SW_READ_NEVER  |
        		                GRALLOC_USAGE_SW_WRITE_OFTEN |
        		                GRALLOC_USAGE_HW_TEXTURE     |
        		                GRALLOC_USAGE_EXTERNAL_DISP);
#if (defined(__CHIP_VERSION_F23) || defined(__CHIP_VERSION_F51)) //a10 do this, a10'sdk is old.
        native_window_set_scaling_mode(mNativeWindow.get(), NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW);

        // Width must be multiple of 32???
        native_window_set_buffers_geometryex(mNativeWindow.get(),
        		                             bufWidth,
                                             bufHeight,
                                             halFormat,
                                             screenID);
#elif (defined(__CHIP_VERSION_F33) || defined(__CHIP_VERSION_F50))
        //        native_window_set_scaling_mode(mNativeWindow.get(), NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW);

        // Width must be multiple of 32???
        native_window_set_buffers_geometry(mNativeWindow.get(),
            		                       bufWidth,
                                           bufHeight,
                                           halFormat);
#else
        #error "Unknown chip type!"
#endif

        uint32_t transform;
        switch (rotationDegrees)
        {
            case 0: transform = 0; break;
            case 90: transform = HAL_TRANSFORM_ROT_90; break;
            case 180: transform = HAL_TRANSFORM_ROT_180; break;
            case 270: transform = HAL_TRANSFORM_ROT_270; break;
            default: transform = 0; break;
        }

        if (transform)
            native_window_set_buffers_transform(mNativeWindow.get(), transform);

        #endif
    }
#endif
    mLayerShowed = 0;

//    pCedarXNativeRendererAdapter = new CedarXNativeRendererAdapter(this);
//    if(NULL == pCedarXNativeRendererAdapter)
//    {
//        LOGW("create CedarXNativeRendererAdapter fail");
//    }
}

CedarXNativeRenderer::~CedarXNativeRenderer()
{
//    if(pCedarXNativeRendererAdapter)
//    {
//        delete pCedarXNativeRendererAdapter;
//        pCedarXNativeRendererAdapter = NULL;
//    }
}
//extern int ion_alloc_phy2vir(void * pbuf);
void CedarXNativeRenderer::render(const void *data, size_t size, void *platformPrivate)
{
    Virtuallibhwclayerpara *pVirtuallibhwclayerpara = (Virtuallibhwclayerpara*)data;
	libhwclayerpara_t overlay_para;
	memset(&overlay_para, 0, sizeof(libhwclayerpara_t));
    overlay_para.number            = pVirtuallibhwclayerpara->number;
    overlay_para.bProgressiveSrc   = pVirtuallibhwclayerpara->bProgressiveSrc;
    overlay_para.bTopFieldFirst    = pVirtuallibhwclayerpara->bTopFieldFirst;
    overlay_para.flag_addr         = pVirtuallibhwclayerpara->flag_addr;
    overlay_para.flag_stride       = pVirtuallibhwclayerpara->flag_stride;    
    overlay_para.maf_valid         = pVirtuallibhwclayerpara->maf_valid;      
    overlay_para.pre_frame_valid   = pVirtuallibhwclayerpara->pre_frame_valid;
	overlay_para.top_y             = pVirtuallibhwclayerpara->top_y;
    overlay_para.top_c             = pVirtuallibhwclayerpara->top_u;
    overlay_para.bottom_y          = 0;
    overlay_para.bottom_c          = 0;
	mHwDisplay->hwd_layer_render(mVideoLayerId, &overlay_para);
//    convertlibhwclayerpara_NativeRendererVirtual2Arch(&overlay_para, pVirtuallibhwclayerpara);
//    mNativeWindow->perform(mNativeWindow.get(), NATIVE_WINDOW_SETPARAMETER, HWC_LAYER_SETFRAMEPARA, (uint32_t)(&overlay_para));
}


int CedarXNativeRenderer::control(int cmd, int para)
{

    return 0;
}
}  // namespace android

