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

//#define LOG_NDEBUG 0
#define LOG_TAG "CedarXSoftwareRenderer"
#include <CDX_Debug.h>

#include <binder/MemoryHeapBase.h>

#if (CEDARX_ANDROID_VERSION < 7)
#include <binder/MemoryHeapPmem.h>
#include <surfaceflinger/Surface.h>
#include <ui/android_native_buffer.h>
#else
#include <system/window.h>
#endif

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MetaData.h>
//#include <ui/GraphicBufferMapper.h>
//#include <gui/ISurfaceTexture.h>

#include "CedarXSoftwareRenderer.h"

extern "C" {
extern unsigned int cedarv_address_phy2vir(void *addr);
}
namespace android {


#if 0
CedarXSoftwareRenderer::CedarXSoftwareRenderer(
        const sp<ANativeWindow> &nativeWindow, const sp<MetaData> &meta)
    : mYUVMode(None),
      mNativeWindow(nativeWindow) 
{
    int32_t tmp;
    CHECK(meta->findInt32(kKeyColorFormat, &tmp));
    //mColorFormat = (OMX_COLOR_FORMATTYPE)tmp;

    //CHECK(meta->findInt32(kKeyScreenID, &screenID));
    //CHECK(meta->findInt32(kKeyColorFormat, &halFormat));
    CHECK(meta->findInt32(kKeyWidth, &mWidth));
    CHECK(meta->findInt32(kKeyHeight, &mHeight));

    int32_t nVdecInitRotation;  //clock wise.
    int32_t rotationDegrees;    //anti-clock wise,
    if (!meta->findInt32(kKeyRotation, &nVdecInitRotation)) {
        LOGD("(f:%s, l:%d) find fail nVdecInitRotation[%d]", __FUNCTION__, __LINE__, nVdecInitRotation);
        rotationDegrees = 0;
    }
    else
    {
        switch(nVdecInitRotation)
        {
            case 0:
            {
                rotationDegrees = 0;
                break;
            }
            case 1:
            {
                rotationDegrees = 270;
                break;
            }
            case 2:
            {
                rotationDegrees = 180;
                break;
            }
            case 3:
            {
                rotationDegrees = 90;
                break;
            }
            default:
            {
                LOGW("(f:%s, l:%d) nInitRotation=%d", __FUNCTION__, __LINE__, nVdecInitRotation);
                rotationDegrees = 0;
                break;
            }
        }
    }
	
    int halFormat;
    size_t bufWidth, bufHeight;
    size_t nGpuBufWidth, nGpuBufHeight;

    halFormat = HAL_PIXEL_FORMAT_YV12;
    bufWidth = mWidth;
    bufHeight = mHeight;
    if(bufWidth != ((mWidth + 15) & ~15))
    {
        LOGW("(f:%s, l:%d) bufWidth[%d]!=display_width[%d]", __FUNCTION__, __LINE__, ((mWidth + 15) & ~15), mWidth);
    }
    if(bufHeight != ((mHeight + 15) & ~15))
    {
        LOGW("(f:%s, l:%d) bufHeight[%d]!=display_height[%d]", __FUNCTION__, __LINE__, ((mHeight + 15) & ~15), mHeight);
    }

    CHECK(mNativeWindow != NULL);
    int usage = 0;
    meta->findInt32(kKeyIsDRM, &usage);
    if(usage) {
    	LOGV("protected video");
    	usage = GRALLOC_USAGE_PROTECTED;
    }
    usage |= GRALLOC_USAGE_SW_READ_NEVER /*| GRALLOC_USAGE_SW_WRITE_OFTEN*/
            | GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_EXTERNAL_DISP;

    CHECK_EQ(0,
            native_window_set_usage(
            mNativeWindow.get(),
            usage));

    CHECK_EQ(0,
            native_window_set_scaling_mode(
            mNativeWindow.get(),
            NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW));

    // Width must be multiple of 32??? 
    nGpuBufWidth = mWidth;
    nGpuBufHeight = mHeight;
#if (defined(__CHIP_VERSION_F23) || defined(__CHIP_VERSION_F51) || defined(__CHIP_VERSION_F50))
  #if (1 == ADAPT_A10_GPU_RENDER)
    nGpuBufWidth = (mWidth + 15) & ~15;
    //A10's GPU has a bug, we can avoid it
    if(nGpuBufHeight%8 != 0)
    {
        LOGD("(f:%s, l:%d) original gpu buf align_width[%d], height[%d]mod8 = %d", __FUNCTION__, __LINE__, nGpuBufWidth, nGpuBufHeight, nGpuBufHeight%8);
        if((nGpuBufWidth*nGpuBufHeight)%256 != 0)
        {   // e:\videofile\test_stream\AVI������\û��index�?����\������-��.avi, 800*450
            LOGW("(f:%s, l:%d) original gpu buf align_width[%d]*height[%d]mod1024 = %d", __FUNCTION__, __LINE__, nGpuBufWidth, nGpuBufHeight, (nGpuBufWidth*nGpuBufHeight)%1024);
            nGpuBufHeight = (nGpuBufHeight+7)&~7;
            LOGW("(f:%s, l:%d) change gpu buf height to [%d]", __FUNCTION__, __LINE__, nGpuBufHeight);
        }
    }
    nGpuBufWidth = mWidth;  //restore GpuBufWidth to mWidth;
  #endif
#elif (defined(__CHIP_VERSION_F33))
  #if (1 == ADAPT_A31_GPU_RENDER)
    //A31 GPU rect width should 8 align.
    if(mWidth%8 != 0)
    {
        nGpuBufWidth = (mWidth + 7) & ~7;
        LOGD("(f:%s, l:%d) A31 gpuBufWidth[%d]mod16=[%d] ,should 8 align, change to [%d]", __FUNCTION__, __LINE__, mWidth, mWidth%8, nGpuBufWidth);
    }
  #endif
#else
    #error "Unknown chip type!"
#endif
    CHECK_EQ(0, native_window_set_buffers_geometry(
                mNativeWindow.get(),
                //(bufWidth + 15) & ~15,
                //(bufHeight+ 15) & ~15,
                //(bufWidth + 15) & ~15,
                //bufHeight,
                //bufWidth,
                //bufHeight,
                nGpuBufWidth,
                nGpuBufHeight,
                halFormat));
    uint32_t transform;
    switch (rotationDegrees) {
        case 0: transform = 0; break;
        case 90: transform = HAL_TRANSFORM_ROT_90; break;
        case 180: transform = HAL_TRANSFORM_ROT_180; break;
        case 270: transform = HAL_TRANSFORM_ROT_270; break;
        default: transform = 0; break;
    }

    if (transform) {
        CHECK_EQ(0, native_window_set_buffers_transform(
                    mNativeWindow.get(), transform));
    }
    Rect crop;
    crop.left = 0;
    crop.top  = 0;
    crop.right = bufWidth;
    crop.bottom = bufHeight;
    mNativeWindow->perform(mNativeWindow.get(), NATIVE_WINDOW_SET_CROP, &crop);
}
#endif

CedarXSoftwareRenderer::CedarXSoftwareRenderer(
        const unsigned int hlay, const sp<MetaData> &meta)
    : mYUVMode(None),
      mVideoLayerId(hlay) 
{
    ALOGE("(f:%s, l:%d) not implement this!", __FUNCTION__, __LINE__);
}

CedarXSoftwareRenderer::~CedarXSoftwareRenderer() {
}

static int ALIGN(int x, int y) {
    // y must be a power of 2.
    return (x + y - 1) & ~(y - 1);
}


void CedarXSoftwareRenderer::render(const void *pObject, size_t size, void *platformPrivate)
{
#if 0    
    ANativeWindowBuffer *buf;
	//libhwclayerpara_t*  pOverlayParam = NULL;
    Virtuallibhwclayerpara *pVirtuallibhwclayerpara = (Virtuallibhwclayerpara*)pObject;
    int err;

#if ((CEDARX_ANDROID_VERSION == 8) || (CEDARX_ANDROID_VERSION == 9))
    if ((err = mNativeWindow->dequeueBuffer_DEPRECATED(mNativeWindow.get(), &buf)) != 0) {
        LOGW("Surface::dequeueBuffer returned error %d", err);
        return;
    }
    CHECK_EQ(0, mNativeWindow->lockBuffer_DEPRECATED(mNativeWindow.get(), buf));
#else
    if ((err = mNativeWindow->dequeueBuffer(mNativeWindow.get(), &buf)) != 0)
    {
        LOGW("Surface::dequeueBuffer returned error %d", err);
        return;
    }
    CHECK_EQ(0, mNativeWindow->lockBuffer(mNativeWindow.get(), buf));
#endif

    GraphicBufferMapper &mapper = GraphicBufferMapper::get();


    //a10_GPUbuffer_YV12, Y:16*2 align, UV:16*1 align.
    //a10_vdecbuffer_YV12, Y:16*16 align, UV:16*8 align.
    //a31_GPUbuffer_YV12, Y:32*2 align, UV:16*1 align.
    //a31_vdecbuffer_YV12, Y:16*16 align, UV:16*8 or 8*8 align

    //we make the rule: the buffersize request from GPU is Y:16*16 align!
    //But in A31, gpu buffer is 32align in width at least, 
    //so the requested a31_gpu_buffer is Y_32*16align, uv_16*8 actually.
    
    //Rect bounds((mWidth+15)&~15, (mHeight+15)&~15);  
    //Rect bounds((mWidth+15)&~15, (mHeight+7)&~7);
    //Rect bounds((mWidth+15)&~15, mHeight);
    Rect bounds(mWidth, mHeight);

    void *dst;
    CHECK_EQ(0, mapper.lock(buf->handle, GRALLOC_USAGE_SW_WRITE_OFTEN, bounds, &dst));

    //LOGD("buf->stride:%d, buf->width:%d, buf->height:%d buf->format:%d, buf->usage:%d,WXH:%dx%d dst:%p", 
    //    buf->stride, buf->width, buf->height, buf->format, buf->usage, mWidth, mHeight, dst);
    
    //LOGV("mColorFormat: %d", mColorFormat);
#if (defined(__CHIP_VERSION_F23) || (defined(__CHIP_VERSION_F51)) || defined(__CHIP_VERSION_F50))
    void* data = (void*)pVirtuallibhwclayerpara->top_y2;
    size_t dst_y_size = buf->stride * buf->height;
    size_t dst_c_stride = ALIGN(buf->stride / 2, 16);    //16
    size_t dst_c_size = dst_c_stride * buf->height / 2;

    int src_y_width = (mWidth+15)&~15;
    int src_y_height = (mHeight+15)&~15;
    int src_c_width = ALIGN(src_y_width / 2, 16);   //uv_width_align=16
    int src_c_height = src_y_height/2;
    int src_display_y_width = mWidth;
    int src_display_y_height = mHeight;
    int src_display_c_width = src_display_y_width/2;
    int src_display_c_height = src_display_y_height/2;
    
    size_t src_display_y_size = src_y_width * src_display_y_height;
    size_t src_display_c_size = src_c_width * src_display_c_height;
    size_t src_y_size = src_y_width * src_y_height;
    size_t src_c_size = src_c_width * src_c_height;
    //LOGV("buf->stride:%d buf->height:%d WXH:%dx%d dst:%p data:%p", buf->stride, buf->height, mWidth, mHeight, dst, data);
    //copy_method_1:
    //memcpy(dst, data, dst_y_size + dst_c_size*2);
    //copy_method_2:
    //memcpy(dst, data, src_y_size + src_c_size*2);
    //copy_method_3:
    memcpy(dst, data, src_display_y_size);
    dst += dst_y_size;
    data += src_y_size;
    memcpy(dst, data, src_display_c_size);
    dst += dst_c_size;
    data += src_c_size;
    memcpy(dst, data, src_display_c_size);
    //LOGD("dst_y_size[%d],dst_c_size[%d];src_y_size[%d],src_c_size[%d];src_display_y_size[%d],src_display_c_size[%d]", 
    //    dst_y_size, dst_c_size, src_y_size, src_c_size, src_display_y_size, src_display_c_size);
#elif (defined(__CHIP_VERSION_F33))
    //libhwclayerpara_t   OverlayParam;
    //convertlibhwclayerpara_SoftwareRendererVirtual2Arch(&OverlayParam, pVirtuallibhwclayerpara);
    //pOverlayParam = (libhwclayerpara_t*)&OverlayParam;

//    LOGD("buf->stride:%d buf->height:%d WXH:%dx%d cstride:%d", buf->stride, buf->height, mWidth, mHeight, dst_c_stride);
//    {
//    	struct timeval t;
//    	int64_t startTime;
//    	int64_t endTime;
//
//    	gettimeofday(&t, NULL);
//    	startTime = (int64_t)t.tv_sec*1000000 + t.tv_usec;
    {
    	int i;
    	int vdecBuf_widthAlign;
    	int vdecBuf_heightAlign;
    	int vdecBuf_cHeightAlign;
    	int vdecBuf_cWidthAlign;
    	int dstCStride;
        int extraHeight;    //the interval between heightalign and display_height

    	unsigned char* dstPtr;
    	unsigned char* srcPtr;
    	dstPtr = (unsigned char*)dst;
//    	srcPtr = (unsigned char*)cedarv_address_phy2vir((void*)pOverlayParam->addr[0]);
    	srcPtr = (unsigned char*)cedarv_address_phy2vir((void*)pVirtuallibhwclayerpara->top_y);
    	vdecBuf_widthAlign = (mWidth + 15) & ~15;
    	vdecBuf_heightAlign = (mHeight + 15) & ~15;
        //1. yv12_y copy
    	for(i=0; i<mHeight; i++)
    	{
    		memcpy(dstPtr, srcPtr, vdecBuf_widthAlign);
    		dstPtr += buf->stride;
    		srcPtr += vdecBuf_widthAlign;
    	}
        //skip hw decoder's extra line of yv12_y
        extraHeight = vdecBuf_heightAlign - mHeight;
        if(extraHeight > 0)
        {
//            LOGD("extraHeight[%d],heightAlign[%d],display_height[%d], need skip hwdecoderBuffer extra lines",
//                extraHeight, vdecBuf_heightAlign, mHeight);
            for(i=0; i<extraHeight; i++)
            {
                srcPtr += vdecBuf_widthAlign;
            }
        }
        //2. yv12_v copy
    	//vdecBuf_cWidthAlign = (mWidth/2 + 15) & ~15;
    	vdecBuf_cWidthAlign = ((mWidth + 15) & ~15)/2;
    	vdecBuf_cHeightAlign = ((mHeight + 15) & ~15)/2;
    	for(i=0; i<(mHeight+1)/2; i++)
    	{
    		memcpy(dstPtr, srcPtr, vdecBuf_cWidthAlign);
    		dstPtr += buf->stride/2;
    		srcPtr += vdecBuf_cWidthAlign;
    	}
        //skip hw decoder's extra line of yv12_v
        extraHeight = vdecBuf_cHeightAlign - (mHeight+1)/2;
        if(extraHeight > 0)
        {
//            ALOGD("uv extraHeight[%d],heightAlign[%d],display_height[%d], need skip hwdecoderBuffer uv extra lines",
//                extraHeight, vdecBuf_heightAlign, mHeight);
            for(i=0;i<extraHeight;i++)
            {
                srcPtr += vdecBuf_cWidthAlign;
            }
        }
        //3. yv12_u copy
        for(i=0; i<(mHeight+1)/2; i++)
    	{
            memcpy(dstPtr, srcPtr, vdecBuf_cWidthAlign);
            dstPtr += buf->stride/2;
            srcPtr += vdecBuf_cWidthAlign;
    	}
    }
//        {
//        	int height_align;
//        	int width_align;
//        	height_align = (buf->height+15) & ~15;
//        	memcpy(dst, (void*)pOverlayParam->addr[0], dst_y_size);
//        	memcpy((unsigned char*)dst + dst_y_size, (void*)(pOverlayParam->addr[0] + height_align*dst_c_stride), dst_c_size*2);
//        }
//    	gettimeofday(&t, NULL);
//        endTime = (int64_t)t.tv_sec*1000000 + t.tv_usec;
//        LOGD("memory copy cost %lld us for %d bytes.", endTime - startTime, dst_y_size + dst_c_size*2);
//    }
#else
    #error "Unknown chip type!"
#endif

#if 0
		{
			static int dec_count = 0;
			static FILE *fp;

			if(dec_count == 60)
			{
				fp = fopen("/data/camera/t.yuv","wb");
				LOGD("write start fp:%d addr:%p",fp,data);
				fwrite(dst,1,buf->stride * buf->height,fp);
				fwrite(dst + buf->stride * buf->height * 5/4,1,buf->stride * buf->height / 4,fp);
				fwrite(dst + buf->stride * buf->height,1,buf->stride * buf->height / 4,fp);
				fclose(fp);
				LOGD("write finish");
			}

			dec_count++;
		}
#endif

    CHECK_EQ(0, mapper.unlock(buf->handle));

#if ((CEDARX_ANDROID_VERSION == 8) || (CEDARX_ANDROID_VERSION == 9))
    if ((err = mNativeWindow->queueBuffer_DEPRECATED(mNativeWindow.get(), buf)) != 0) {
        LOGW("Surface::queueBuffer returned error %d", err);
    }
    buf = NULL;
#else 
    if ((err = mNativeWindow->queueBuffer(mNativeWindow.get(), buf)) != 0) {
        LOGW("Surface::queueBuffer returned error %d", err);
    }
    buf = NULL;
#endif

#endif
}

int CedarXSoftwareRenderer::dequeueFrame(ANativeWindowBufferCedarXWrapper *pObject)
{
#if 0    
    ANativeWindowBuffer *buf;
	//libhwclayerpara_t*  pOverlayParam = NULL;
    //Virtuallibhwclayerpara *pVirtuallibhwclayerpara = (Virtuallibhwclayerpara*)pObject;
    ANativeWindowBufferCedarXWrapper *pANativeWindowBuffer = (ANativeWindowBufferCedarXWrapper*)pObject;
    int err;

#if ((CEDARX_ANDROID_VERSION == 8) || (CEDARX_ANDROID_VERSION == 9))
    if ((err = mNativeWindow->dequeueBuffer_DEPRECATED(mNativeWindow.get(), &buf)) != 0) {
        LOGW("Surface::dequeueBuffer returned error %d", err);
        return -1;
    }
    CHECK_EQ(0, mNativeWindow->lockBuffer_DEPRECATED(mNativeWindow.get(), buf));
#else
    if ((err = mNativeWindow->dequeueBuffer(mNativeWindow.get(), &buf)) != 0)
    {
        LOGW("Surface::dequeueBuffer returned error %d", err);
        return -1;
    }
    CHECK_EQ(0, mNativeWindow->lockBuffer(mNativeWindow.get(), buf));
#endif

    GraphicBufferMapper &mapper = GraphicBufferMapper::get();


    //a10_GPUbuffer_YV12, Y:16*2 align, UV:16*1 align.
    //a10_vdecbuffer_YV12, Y:16*16 align, UV:16*8 align.
    //a31_GPUbuffer_YV12, Y:32*2 align, UV:16*1 align.
    //a31_vdecbuffer_YV12, Y:16*16 align, UV:16*8 or 8*8 align

    //we make the rule: the buffersize request from GPU is Y:16*16 align!
    //But in A31, gpu buffer is 32align in width at least, 
    //so the requested a31_gpu_buffer is Y_32*16align, uv_16*8 actually.
    
    //Rect bounds((mWidth+15)&~15, (mHeight+15)&~15);  
    //Rect bounds((mWidth+15)&~15, (mHeight+7)&~7);
    //Rect bounds((mWidth+15)&~15, mHeight);
    Rect bounds(mWidth, mHeight);

    void *dst;
    void *dstPhyAddr;
    int phyaddress;
    CHECK_EQ(0, mapper.lock(buf->handle, GRALLOC_USAGE_SW_WRITE_OFTEN, bounds, &dst));
//    mapper.get_phy_addess(buf->handle, &dstPhyAddr);
    phyaddress = (int)dstPhyAddr;
//    LOGD("test phyaddress[0x%x]", phyaddress);
//	if(phyaddress >= 0x40000000)
//	{
//		phyaddress -= 0x40000000;
//        LOGW("phyaddress -= 0x40000000, test phyaddress2[0x%x]", phyaddress);
//	}

//    LOGD("buf->stride:%d, buf->width:%d, buf->height:%d buf->format:%d, buf->usage:%d,WXH:%dx%d dst:%p", 
//        buf->stride, buf->width, buf->height, buf->format, buf->usage, mWidth, mHeight, dst);
    pANativeWindowBuffer->width     = buf->width;
    pANativeWindowBuffer->height    = buf->height;
    pANativeWindowBuffer->stride    = buf->stride;
    pANativeWindowBuffer->format    = buf->format;
    pANativeWindowBuffer->usage     = buf->usage;
    pANativeWindowBuffer->dst       = dst;
    pANativeWindowBuffer->dstPhy    = (void*)phyaddress;
    pANativeWindowBuffer->pObjANativeWindowBuffer = (void*)buf;
#endif
    return 0;
}

int CedarXSoftwareRenderer::enqueueFrame(ANativeWindowBufferCedarXWrapper *pObject)
{
#if 0
    int err;
    ANativeWindowBuffer *buf = (ANativeWindowBuffer*)pObject->pObjANativeWindowBuffer;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    CHECK_EQ(0, mapper.unlock(buf->handle));

#if ((CEDARX_ANDROID_VERSION == 8) || (CEDARX_ANDROID_VERSION == 9))
    if ((err = mNativeWindow->queueBuffer_DEPRECATED(mNativeWindow.get(), buf)) != 0) {
        LOGW("Surface::queueBuffer returned error %d", err);
    }
    buf = NULL;
#else 
    if ((err = mNativeWindow->queueBuffer(mNativeWindow.get(), buf)) != 0) {
        LOGW("Surface::queueBuffer returned error %d", err);
    }
    buf = NULL;
#endif
#endif
    return 0;
}

}  // namespace android

#if 0   //backup
void CedarXSoftwareRenderer::render0(const void *data, size_t size, void *platformPrivate)
{
    ANativeWindowBuffer *buf;
	libhwclayerpara_t*  pOverlayParam;
    int err;

#if (CEDARX_ANDROID_VERSION == 8)
    if ((err = mNativeWindow->dequeueBuffer_DEPRECATED(mNativeWindow.get(), &buf)) != 0) {
        LOGW("Surface::dequeueBuffer returned error %d", err);
        return;
    }
    CHECK_EQ(0, mNativeWindow->lockBuffer_DEPRECATED(mNativeWindow.get(), buf));
#else
    if ((err = mNativeWindow->dequeueBuffer(mNativeWindow.get(), &buf)) != 0)
    {
        LOGW("Surface::dequeueBuffer returned error %d", err);
        return;
    }
    CHECK_EQ(0, mNativeWindow->lockBuffer(mNativeWindow.get(), buf));
#endif

    GraphicBufferMapper &mapper = GraphicBufferMapper::get();


    //a10_GPUbuffer_YV12, Y:16*2 align, UV:16*1 align.
    //a10_vdecbuffer_YV12, Y:16*16 align, UV:16*8 align.
    //a31_GPUbuffer_YV12, Y:32*2 align, UV:16*1 align.
    //a31_vdecbuffer_YV12, Y:16*16 align, UV:16*8 or 8*8 align

    //we make the rule: the buffersize request from GPU is Y:16*16 align!
    //But in A31, gpu buffer is 32align in width at least, 
    //so the requested a31_gpu_buffer is Y_32*16align, uv_16*8 actually.
    
    Rect bounds((mWidth+15)&~15, (mHeight+15)&~15);  
    //Rect bounds(mWidth, mHeight);

    void *dst;
    CHECK_EQ(0, mapper.lock(buf->handle, GRALLOC_USAGE_SW_WRITE_OFTEN, bounds, &dst));

    //LOGD("buf->stride:%d, buf->width:%d, buf->height:%d buf->format:%d, buf->usage:%d,WXH:%dx%d dst:%p data:%p", 
    //    buf->stride, buf->width, buf->height, buf->format, buf->usage, mWidth, mHeight, dst, data);
    
    //LOGV("mColorFormat: %d", mColorFormat);
#if defined(__CHIP_VERSION_F23)
    size_t dst_y_size = buf->stride * buf->height;
    size_t dst_c_stride = ALIGN(buf->stride / 2, 16);    //16
    size_t dst_c_size = dst_c_stride * buf->height / 2;

    int src_y_width = (mWidth+15)&~15;
    int src_y_height = (mHeight+15)&~15;
    int src_c_width = ALIGN(src_y_width / 2, 16);   //uv_width_align=16
    int src_c_height = src_y_height/2;
    int src_display_y_width = mWidth;
    int src_display_y_height = mHeight;
    int src_display_c_width = src_display_y_width/2;
    int src_display_c_height = src_display_y_height/2;
    
    size_t src_display_y_size = src_y_width * src_display_y_height;
    size_t src_display_c_size = src_c_width * src_display_c_height;
    size_t src_y_size = src_y_width * src_y_height;
    size_t src_c_size = src_c_width * src_c_height;
    //LOGV("buf->stride:%d buf->height:%d WXH:%dx%d dst:%p data:%p", buf->stride, buf->height, mWidth, mHeight, dst, data);
    //copy_method_1:
    memcpy(dst, data, dst_y_size + dst_c_size*2);
    //copy_method_2:
    //memcpy(dst, data, src_y_size + src_c_size*2);
    //copy_method_3:
//    memcpy(dst, data, src_display_y_size);
//    dst += dst_y_size;
//    data += src_y_size;
//    memcpy(dst, data, src_display_c_size);
//    dst += dst_c_size;
//    data += src_c_size;
//    memcpy(dst, data, src_display_c_size);
#elif (defined(__CHIP_VERSION_F33) || defined(__CHIP_VERSION_F50))
        pOverlayParam = (libhwclayerpara_t*)data;

//    LOGD("buf->stride:%d buf->height:%d WXH:%dx%d cstride:%d", buf->stride, buf->height, mWidth, mHeight, dst_c_stride);
//    {
//    	struct timeval t;
//    	int64_t startTime;
//    	int64_t endTime;
//
//    	gettimeofday(&t, NULL);
//    	startTime = (int64_t)t.tv_sec*1000000 + t.tv_usec;
    {
    	int i;
    	int widthAlign;
    	int heightAlign;
    	int cHeight;
    	int cWidth;
    	int dstCStride;

    	unsigned char* dstPtr;
    	unsigned char* srcPtr;
    	dstPtr = (unsigned char*)dst;
    	srcPtr = (unsigned char*)cedarv_address_phy2vir((void*)pOverlayParam->addr[0]);
    	widthAlign = (mWidth + 15) & ~15;
    	heightAlign = (mHeight + 15) & ~15;
    	for(i=0; i<heightAlign; i++)
    	{
    		memcpy(dstPtr, srcPtr, widthAlign);
    		dstPtr += buf->stride;
    		srcPtr += widthAlign;
    	}

    	cWidth = (mWidth/2 + 15) & ~15;
    	cHeight = heightAlign;
    	for(i=0; i<cHeight; i++)
    	{
    		memcpy(dstPtr, srcPtr, cWidth);
    		dstPtr += cWidth;
    		srcPtr += cWidth;
    	}
    }
//        {
//        	int height_align;
//        	int width_align;
//        	height_align = (buf->height+15) & ~15;
//        	memcpy(dst, (void*)pOverlayParam->addr[0], dst_y_size);
//        	memcpy((unsigned char*)dst + dst_y_size, (void*)(pOverlayParam->addr[0] + height_align*dst_c_stride), dst_c_size*2);
//        }
//    	gettimeofday(&t, NULL);
//        endTime = (int64_t)t.tv_sec*1000000 + t.tv_usec;
//        LOGD("memory copy cost %lld us for %d bytes.", endTime - startTime, dst_y_size + dst_c_size*2);
//    }
#else
    #error "Unknown chip type!"
#endif

#if 0
		{
			static int dec_count = 0;
			static FILE *fp;

			if(dec_count == 60)
			{
				fp = fopen("/data/camera/t.yuv","wb");
				LOGD("write start fp:%d addr:%p",fp,data);
				fwrite(dst,1,buf->stride * buf->height,fp);
				fwrite(dst + buf->stride * buf->height * 5/4,1,buf->stride * buf->height / 4,fp);
				fwrite(dst + buf->stride * buf->height,1,buf->stride * buf->height / 4,fp);
				fclose(fp);
				LOGD("write finish");
			}

			dec_count++;
		}
#endif

    CHECK_EQ(0, mapper.unlock(buf->handle));

#if (CEDARX_ANDROID_VERSION == 8)
    if ((err = mNativeWindow->queueBuffer_DEPRECATED(mNativeWindow.get(), buf)) != 0) {
        LOGW("Surface::queueBuffer returned error %d", err);
    }
    buf = NULL;
#else 
    if ((err = mNativeWindow->queueBuffer(mNativeWindow.get(), buf)) != 0) {
        LOGW("Surface::queueBuffer returned error %d", err);
    }
    buf = NULL;
#endif

}

#endif
