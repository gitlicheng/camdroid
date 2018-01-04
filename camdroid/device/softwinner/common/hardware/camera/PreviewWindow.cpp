
#include "CameraDebug.h"
#if DBG_PREVIEW
#define LOG_NDEBUG 0
#endif
#define LOG_TAG "PreviewWindow"
#include <cutils/log.h>

#include <ui/Rect.h>
//#include <ui/GraphicBufferMapper.h>
//#include "V4L2CameraDevice.h"
#include "V4L2CameraDevice2.h"
#include "PreviewWindow.h"

namespace android {

/*#define CAMHAL_GRALLOC_USAGE GRALLOC_USAGE_HW_TEXTURE | \
								 GRALLOC_USAGE_HW_RENDER | \
								 GRALLOC_USAGE_SW_READ_RARELY | \
								 GRALLOC_USAGE_SW_WRITE_NEVER
*/
#define CAMHAL_GRALLOC_USAGE GRALLOC_USAGE_SW_READ_RARELY | \
									 GRALLOC_USAGE_SW_WRITE_NEVER


static int calculateFrameSize(int width, int height, uint32_t pix_fmt)
{
	int frame_size = 0;
	switch (pix_fmt) {
		case V4L2_PIX_FMT_YVU420:
		case V4L2_PIX_FMT_YUV420:
		case V4L2_PIX_FMT_NV21:
		case V4L2_PIX_FMT_NV12:
			frame_size = (ALIGN_16B(width) * height * 12) / 8;
			break;
		case V4L2_PIX_FMT_YUYV:
			frame_size = (width * height) << 1;
			break;
		default:
			ALOGE("%s: Unknown pixel format %d(%.4s)",
				__FUNCTION__, pix_fmt, reinterpret_cast<const char*>(&pix_fmt));
			break;
	}
	return frame_size;
}

static void NV12ToNV21(const void* nv12, void* nv21, int width, int height)
{       
    char * src_uv = (char *)nv12 + width * height;
    char * dst_uv = (char *)nv21 + width * height;

    memcpy(nv21, nv12, width * height);

    for(int i = 0; i < width * height / 2; i += 2)
    {
           *(dst_uv + i) = *(src_uv + i + 1);
           *(dst_uv + i + 1) = *(src_uv + i);
    }
}
   
static void NV12ToNV21_shift(const void* nv12, void* nv21, int width, int height)
{       
    char * src_uv = (char *)nv12 + width * height;
    char * dst_uv = (char *)nv21 + width * height;

    memcpy(nv21, nv12, width * height);
    memcpy(dst_uv, src_uv + 1, width * height / 2 - 1);
}

static void formatToNV21(void *dst,
		               void *src,
		               int width,
		               int height,
		               size_t stride,
		               uint32_t offset,
		               unsigned int bytesPerPixel,
		               size_t length,
		               int pixelFormat)
{
    unsigned int alignedRow, row;
    unsigned char *bufferDst, *bufferSrc;
    unsigned char *bufferDstEnd, *bufferSrcEnd;
    uint16_t *bufferSrc_UV;

    unsigned int y_uv[2];
    y_uv[0] = (unsigned int)src;
	y_uv[1] = (unsigned int)src + width*height;

	// NV12 -> NV21
    if (pixelFormat == V4L2_PIX_FMT_NV12) {
        bytesPerPixel = 1;
        bufferDst = ( unsigned char * ) dst;
        bufferDstEnd = ( unsigned char * ) dst + width*height*bytesPerPixel;
        bufferSrc = ( unsigned char * ) y_uv[0] + offset;
        bufferSrcEnd = ( unsigned char * ) ( ( size_t ) y_uv[0] + length + offset);
        row = width*bytesPerPixel;
        alignedRow = stride-width;
        int stride_bytes = stride / 8;
        uint32_t xOff = offset % stride;
        uint32_t yOff = offset / stride;

        // going to convert from NV12 here and return
        // Step 1: Y plane: iterate through each row and copy
        for ( int i = 0 ; i < height ; i++) {
            memcpy(bufferDst, bufferSrc, row);
            bufferSrc += stride;
            bufferDst += row;
            if ( ( bufferSrc > bufferSrcEnd ) || ( bufferDst > bufferDstEnd ) ) {
                break;
            }
        }

        bufferSrc_UV = ( uint16_t * ) ((uint8_t*)y_uv[1] + (stride/2)*yOff + xOff);

        uint16_t *bufferDst_UV;

        // Step 2: UV plane: convert NV12 to NV21 by swapping U & V
        bufferDst_UV = (uint16_t *) (((uint8_t*)dst)+row*height);

        for (int i = 0 ; i < height/2 ; i++, bufferSrc_UV += alignedRow/2) {
            int n = width;
            asm volatile (
            "   pld [%[src], %[src_stride], lsl #2]                         \n\t"
            "   cmp %[n], #32                                               \n\t"
            "   blt 1f                                                      \n\t"
            "0: @ 32 byte swap                                              \n\t"
            "   sub %[n], %[n], #32                                         \n\t"
            "   vld2.8  {q0, q1} , [%[src]]!                                \n\t"
            "   vswp q0, q1                                                 \n\t"
            "   cmp %[n], #32                                               \n\t"
            "   vst2.8  {q0,q1},[%[dst]]!                                   \n\t"
            "   bge 0b                                                      \n\t"
            "1: @ Is there enough data?                                     \n\t"
            "   cmp %[n], #16                                               \n\t"
            "   blt 3f                                                      \n\t"
            "2: @ 16 byte swap                                              \n\t"
            "   sub %[n], %[n], #16                                         \n\t"
            "   vld2.8  {d0, d1} , [%[src]]!                                \n\t"
            "   vswp d0, d1                                                 \n\t"
            "   cmp %[n], #16                                               \n\t"
            "   vst2.8  {d0,d1},[%[dst]]!                                   \n\t"
            "   bge 2b                                                      \n\t"
            "3: @ Is there enough data?                                     \n\t"
            "   cmp %[n], #8                                                \n\t"
            "   blt 5f                                                      \n\t"
            "4: @ 8 byte swap                                               \n\t"
            "   sub %[n], %[n], #8                                          \n\t"
            "   vld2.8  {d0, d1} , [%[src]]!                                \n\t"
            "   vswp d0, d1                                                 \n\t"
            "   cmp %[n], #8                                                \n\t"
            "   vst2.8  {d0[0],d1[0]},[%[dst]]!                             \n\t"
            "   bge 4b                                                      \n\t"
            "5: @ end                                                       \n\t"
#ifdef NEEDS_ARM_ERRATA_754319_754320
            "   vmov s0,s0  @ add noop for errata item                      \n\t"
#endif
            : [dst] "+r" (bufferDst_UV), [src] "+r" (bufferSrc_UV), [n] "+r" (n)
            : [src_stride] "r" (stride_bytes)
            : "cc", "memory", "q0", "q1"
            );
        }
    }
}

DBG_TIME_AVG_BEGIN(TAG_CPY);
DBG_TIME_AVG_BEGIN(TAG_DQBUF);
DBG_TIME_AVG_BEGIN(TAG_LKBUF);
DBG_TIME_AVG_BEGIN(TAG_MAPPER);
DBG_TIME_AVG_BEGIN(TAG_EQBUF);
DBG_TIME_AVG_BEGIN(TAG_UNLKBUF);

PreviewWindow::PreviewWindow()
    : mPreviewWindow(WINDOW_UNINITED)
    , mPreviewFrameWidth(0)
    , mPreviewFrameHeight(0)
    , mPreviewFrameSize(0)
    , mCurPixelFormat(0)
    , mPreviewEnabled(false)
    , mPreviewNeedSetSrc(false)
    , mShouldAdjustDimensions(true)
{
	F_LOG;
	mHwDisplay = HwDisplay::getInstance();
}

PreviewWindow::~PreviewWindow()
{
	F_LOG;
	mPreviewWindow = WINDOW_UNINITED;
}

/****************************************************************************
 * Camera API
 ***************************************************************************/

status_t PreviewWindow::setPreviewWindow(struct preview_stream_ops* window)
{
    LOGD("%s: current: %p -> new: %p", __FUNCTION__, mPreviewWindow, window);
	
    status_t res = NO_ERROR;
    Mutex::Autolock locker(&mObjectLock);
    /* Reset preview info. */
    mPreviewFrameWidth = mPreviewFrameHeight = 0;
	mPreviewWindow = window;
    mHlay = (int)window;
    return res;
}

status_t PreviewWindow::startPreview()
{
    F_LOG;
    Mutex::Autolock locker(&mObjectLock);
    mPreviewEnabled = true;
    mPreviewNeedSetSrc = true;
	DBG_TIME_AVG_INIT(TAG_CPY);
	DBG_TIME_AVG_INIT(TAG_DQBUF);
	DBG_TIME_AVG_INIT(TAG_LKBUF);
	DBG_TIME_AVG_INIT(TAG_MAPPER);
	DBG_TIME_AVG_INIT(TAG_EQBUF);
	DBG_TIME_AVG_INIT(TAG_UNLKBUF);
    return NO_ERROR;
}

void PreviewWindow::stopPreview()
{
    F_LOG;
    Mutex::Autolock locker(&mObjectLock);
    mPreviewEnabled = false;
	mShouldAdjustDimensions = true;

	DBG_TIME_AVG_END(TAG_CPY, "copy ");
	DBG_TIME_AVG_END(TAG_DQBUF, "deque ");
	DBG_TIME_AVG_END(TAG_LKBUF, "lock ");
	DBG_TIME_AVG_END(TAG_MAPPER, "mapper ");
	DBG_TIME_AVG_END(TAG_EQBUF, "enque ");
	DBG_TIME_AVG_END(TAG_UNLKBUF, "unlock ");
}

#include "FormatConvert.h"
/****************************************************************************
 * Public API
 ***************************************************************************/

bool PreviewWindow::setSrcFormat(struct src_info *src, libhwclayerpara_t *pic, void *pbuf, int index)
{
	if (mPreviewNeedSetSrc) {
        V4l2PreviewBuffer_tag *pPrevbuf = (V4l2PreviewBuffer_tag*)pbuf;

	    LOGD("mPreviewNeedSetSrc\n");
		switch (pPrevbuf->format)
		{
			case V4L2_PIX_FMT_NV21:
				src->format = HWC_FORMAT_YUV420VUC;
				LOGD("preview_format:HWC_FORMAT_YUV420VUC\n");
				break;
			case V4L2_PIX_FMT_NV12:
		    #ifdef __SUNXI__
				src->format = HWC_FORMAT_YUV420UVC;
				LOGD("preview_format:HWC_FORMAT_YUV420UVC\n");
				break;
		    #else
				src->format = HWC_FORMAT_YUV420UVC; 
				LOGD("preview_format:HWC_FORMAT_YUV420UVC\n");
				break;
		    #endif
			case V4L2_PIX_FMT_YVU420:
			case V4L2_PIX_FMT_YUV420:	// to do
				src->format = HWC_FORMAT_YUV420PLANAR;
				pic->bottom_y = pic->top_c + (pPrevbuf->width * pPrevbuf->height / 4);
				LOGD("preview_format:HWC_FORMAT_YUV420PLANAR\n");
				break;
			case V4L2_PIX_FMT_YUYV:
				src->format = HWC_FORMAT_YUV422PLANAR;
				pic->bottom_y = pic->top_c + (pPrevbuf->width * pPrevbuf->height / 2);
				LOGD("preview_format:HWC_FORMAT_YUV422PLANAR\n");
				break;
			
			default:
				LOGD("preview unknown pixel format: %08x", pPrevbuf->format);
				return false;
		}
		mHwDisplay->hwd_layer_set_src(mHlay, src);
		mPreviewNeedSetSrc = false;
	}
	return true;
}

bool PreviewWindow::onNextFrameAvailable(const void* frame)
{
    Mutex::Autolock locker(&mObjectLock);
    V4l2PreviewBuffer_tag *pPrevbuf = (V4l2PreviewBuffer_tag *)frame;
    struct src_info src;

    if (!isPreviewEnabled() || mPreviewWindow == WINDOW_UNINITED || mHlay < 0) 
    {
        //LOGD("cannot start csi render!");
        return true;
    }
    libhwclayerpara_t pic;

    src.w = pPrevbuf->width;
    src.h = pPrevbuf->height;
    src.crop_x = pPrevbuf->crop_rect.left;
    src.crop_y = pPrevbuf->crop_rect.top;
    src.crop_w = pPrevbuf->crop_rect.width;
    src.crop_h = pPrevbuf->crop_rect.height;
    pic.top_y = pPrevbuf->addrPhyY;
    pic.top_c = pic.top_y + (pPrevbuf->width * pPrevbuf->height);

    setSrcFormat(&src, &pic, pPrevbuf, 0);
    mHwDisplay->hwd_layer_render(mHlay, &pic);

    return true;
    }

/***************************************************************************
 * Private API
 **************************************************************************/

bool PreviewWindow::adjustPreviewDimensions(V4L2BUF_t* pbuf)
{
	/* Match the cached frame dimensions against the actual ones. */
	if ((pbuf->isThumbAvailable == 1)
		&& (pbuf->thumbUsedForPreview == 1))		// use thumb frame for preview
	{
		if ((mPreviewFrameWidth == (int)pbuf->thumbWidth)
			&& (mPreviewFrameHeight == (int)pbuf->thumbHeight)
			&& (mCurPixelFormat == pbuf->thumbFormat)) 
		{
			/* They match. */
			return false;
		}

		LOGV("cru: [%d, %d], get: [%d, %d]", mPreviewFrameWidth, mPreviewFrameHeight,
			pbuf->thumbWidth, pbuf->thumbHeight);
		/* They don't match: adjust the cache. */
		mPreviewFrameWidth = pbuf->thumbWidth;
		mPreviewFrameHeight = pbuf->thumbHeight;
		mCurPixelFormat = pbuf->thumbFormat;

		mPreviewFrameSize = calculateFrameSize(pbuf->thumbWidth, pbuf->thumbHeight, pbuf->thumbFormat);
	}
	else
	{
	    if ((mPreviewFrameWidth == (int)pbuf->width)
			&& (mPreviewFrameHeight == (int)pbuf->height)
			&& (mCurPixelFormat == pbuf->format)) 
		{
	        /* They match. */
	        return false;
	    }

		LOGV("cru: [%d, %d], get: [%d, %d]", mPreviewFrameWidth, mPreviewFrameHeight,
			pbuf->width, pbuf->height);
	    /* They don't match: adjust the cache. */
	    mPreviewFrameWidth = pbuf->width;
	    mPreviewFrameHeight = pbuf->height;
		mCurPixelFormat = pbuf->format;

		mPreviewFrameSize = calculateFrameSize(pbuf->width, pbuf->height, pbuf->format);
	}

	mShouldAdjustDimensions = false;
    return true;
}

}; /* namespace android */
