#define LOG_NDEBUG 0
#define LOG_TAG "MediaServerCallerClient"
#include <utils/Log.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include <string.h>
#include <cutils/atomic.h>
#include <cutils/properties.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <private/media/VideoFrame.h>
#include <media/stagefright/ColorConverter.h>
#include <media/stagefright/foundation/ADebug.h>

#include <memoryAdapter.h>

#include <jpegDecoder.h>

#include "MediaServerCallerClient.h"
#include <cedarx_stream.h>

namespace android {

#define ALIGN4(x) ((((x)+3)>>2)<<2)
#define ALIGN8(x) ((((x)+7)>>3)<<3)
#define ALIGN16(x) ((((x)+15)>>4)<<4)
#define ALIGN32(x) ((((x)+31)>>5)<<5)

enum FORMAT_CONVERT_COLORFORMAT {
	CONVERT_COLOR_FORMAT_NONE = 0,
	CONVERT_COLOR_FORMAT_YUV420PLANNER,
	CONVERT_COLOR_FORMAT_YVU420PLANNER,
	CONVERT_COLOR_FORMAT_YUV420MB,
	CONVERT_COLOR_FORMAT_YUV422MB,
};

//#define DEBUG_JPEG_DEC_SAVE_DATA

typedef struct ScalerParameter{
	int mode; //0: YV12 1:thumb yuv420p
	int format_in;
	int format_out;

	int width_in;
	int height_in;

	int width_out;
	int height_out;

	void *addr_y_in;
	void *addr_c_in;
	unsigned int addr_y_out;
	unsigned int addr_u_out;
	unsigned int addr_v_out;
}ScalerParameter;

extern "C" int SoftwarePictureScaler(ScalerParameter *scalerPara);

#ifdef DEBUG_JPEG_DEC_SAVE_DATA
static int saveDataInFile(const char *filename, void *buffer, int bufsize)
{
    FILE *fp;
    int ret;

    fp = fopen(filename, "wb");
    if (fp == NULL) {
        ALOGE("Failed to open file %s!", filename);
        return -1;
    }

    ret = fwrite(buffer, bufsize, 1, fp);
    if(ret < 0) {
        ALOGE("Failed to write file %s, ret=%d(%s)", filename, ret, strerror(errno));
    }
    fclose(fp);
    return 0;
}
#endif

MediaServerCallerClient::MediaServerCallerClient(pid_t pid)
{
    ALOGV("constructor pid(%d)", pid);
    mPid = pid;
}

MediaServerCallerClient::~MediaServerCallerClient()
{
    ALOGV("destructor");
    disconnect();
}

void MediaServerCallerClient::disconnect()
{
    ALOGV("disconnect from pid %d", mPid);
    Mutex::Autolock lock(mLock);
    IPCThreadState::self()->flushCommands();
}

sp<IMemory> MediaServerCallerClient::jpegDecodeBuf(sp<JpegDecoder> jpgDecoder, uint8_t *jpgbuf, int filesize)
{
    char *buf0, *buf1;
    int size0, size1;
    void *addrYVir = NULL, *addrUVir = NULL, *addrVVir = NULL;
    VideoPicture *picture;
    JpegInfo jpgInfo;
    status_t ret;
    int i;
    
    jpgInfo.data = jpgbuf;
    jpgInfo.datasize = filesize;
    ret = jpgDecoder->getJpgInfo(&jpgInfo);
    if (ret != NO_ERROR) {
        ALOGE("Failed to get jpeg image info!");
        free(jpgbuf);
        return NULL;
    }
    ALOGD("jpeg size(%dx%d)", jpgInfo.width, jpgInfo.height);
    ret = jpgDecoder->initialize(jpgInfo.width, jpgInfo.height);
    if (ret != NO_ERROR) {
        ALOGE("initialize error!");
        free(jpgbuf);
        return NULL;
    }
    ret = jpgDecoder->requestBuffer(filesize, &buf0, &size0, &buf1, &size1);
    if (ret != NO_ERROR) {
        ALOGE("requestBuffer error!");
        free(jpgbuf);
        jpgDecoder->destroy();
        return NULL;
    }
    if (filesize <= size0) {
        memcpy(buf0, jpgbuf, filesize);
    } else if (filesize <= size0 + size1) {
        memcpy(buf0, jpgbuf, size0);
        memcpy(buf1, jpgbuf + size0, filesize - size0);
    } else {
        ALOGE("requestBuffer too small!");
        free(jpgbuf);
        jpgDecoder->destroy();
        return NULL;
    }
    free(jpgbuf);

    jpgDecoder->updateBuffer(filesize, 0, buf0);

    ret = jpgDecoder->decode();
    if (ret < 0) {
        ALOGE("decode error!");
        jpgDecoder->destroy();
        return NULL;
    }

    for (i = 0; i < 100; i++) {
        picture = jpgDecoder->getFrame();
        if (picture != NULL) {
            break;
        }
        usleep(10 * 1000);
    }
    if (i >= 100) {
        ALOGE("getFrame error!");
        jpgDecoder->destroy();
        return NULL;
    }
    //addrYVir = MemAdapterGetVirtualAddress(picture->pData0);
    //addrUVir = MemAdapterGetVirtualAddress(picture->pData1);
    //if (picture->v != NULL) {
    //    addrVVir = MemAdapterGetVirtualAddress(picture->v);
    //}

    int align_w = ALIGN16(picture->nWidth);
    int align_h = ALIGN16(picture->nHeight);
    int width = picture->nRightOffset - picture->nLeftOffset;
    int height = picture->nBottomOffset - picture->nTopOffset;
    ALOGD("output size(%dx%d), alignSize(%dx%d), PixelFormat=%d", width, height, align_w, align_h, picture->ePixelFormat);

    size_t sizeY = align_w * align_h;
    uint8_t *pBuf;
    pBuf = (uint8_t*)malloc((sizeY>>1)*3);
    if (pBuf == NULL) {
        ALOGE("Failed to alloc yv12 buffer!");
        jpgDecoder->releaseFrame(picture);
        jpgDecoder->destroy();
        return NULL;
    }
    int ylen = sizeY;
    int ulen = sizeY >> 2;
    int vlen = sizeY >> 2;
    uint8_t *y_addr = pBuf;
    uint8_t *u_addr = y_addr + ylen;
    uint8_t *v_addr = u_addr + ulen;

    if(picture->ePixelFormat == PIXEL_FORMAT_YUV_MB32_420) {
        ScalerParameter scalerPara;
        scalerPara.format_in = CONVERT_COLOR_FORMAT_YUV420MB;
        scalerPara.format_out = CONVERT_COLOR_FORMAT_YUV420PLANNER;
        scalerPara.width_in   = align_w;
        scalerPara.height_in  = align_h;
        scalerPara.width_out  = align_w;
        scalerPara.height_out = align_h;
        scalerPara.addr_y_in  = (void*)picture->pData0;
        scalerPara.addr_c_in  = (void*)picture->pData1;
        scalerPara.mode = 1;
        scalerPara.addr_y_out = (unsigned int)y_addr;
        scalerPara.addr_u_out = (unsigned int)u_addr;
        scalerPara.addr_v_out = (unsigned int)v_addr;
        SoftwarePictureScaler(&scalerPara);
    } else if (picture->ePixelFormat == PIXEL_FORMAT_YV12) {
        memcpy(y_addr, picture->pData0, ylen);
        memcpy(v_addr, picture->pData1, vlen);
        memcpy(u_addr, picture->pData1+vlen, ulen);
    } else if (picture->ePixelFormat == PIXEL_FORMAT_YUV_PLANER_420) {
        memcpy(y_addr, picture->pData0, ylen+ulen+vlen);
    } else {
        ALOGE("unsupport PixelFormat(%d)!", picture->ePixelFormat);
        jpgDecoder->releaseFrame(picture);
        jpgDecoder->destroy();
        free(pBuf);
        return NULL;
    }
    jpgDecoder->releaseFrame(picture);
    jpgDecoder->destroy();

    VideoFrame *frame = new VideoFrame;
    if (frame == NULL) {
        ALOGE("Failed to alloc VideoFrame!");
        free(pBuf);
        return NULL;
    }
    frame->mWidth         = width;
    frame->mHeight        = height;
    frame->mDisplayWidth  = width;
    frame->mDisplayHeight = height;
    frame->mSize          = width * height * 2;
    frame->mRotationAngle = 0;
    frame->mData          = new uint8_t[frame->mSize];
    if (frame->mData == NULL) {
        ALOGE("Failed to alloc frame->mData!");
        delete frame;
        free(pBuf);
        return NULL;
    }
    ColorConverter converter((OMX_COLOR_FORMATTYPE)OMX_COLOR_FormatYUV420Planar, OMX_COLOR_Format16bitRGB565);
    CHECK(converter.isValid());

    ret = converter.convert(y_addr,
                      align_w,
                      align_h,
                      0,
                      0,
                      frame->mWidth - 1,
                      frame->mHeight - 1,
                      frame->mData,
                      frame->mWidth,
                      frame->mHeight,
                      0,
                      0,
                      frame->mWidth - 1,
                      frame->mHeight - 1);
    free(pBuf);
    if (ret != NO_ERROR) {
        ALOGE("ColorConverter error(%d)", ret);
        delete frame;
        return NULL;
    }

    size_t memsize = sizeof(VideoFrame) + frame->mSize;
    sp<MemoryHeapBase> heap = new MemoryHeapBase(memsize);
    if (heap == NULL) {
        ALOGE("failed to create MemoryHeapBase");
        delete frame;
        return NULL;
    }
    sp<IMemory> memory = new MemoryBase(heap, 0, memsize);
    if (memory == NULL) {
        ALOGE("not enough memory for argbMemory size=%u", memsize);
        delete frame;
        return NULL;
    }
    VideoFrame *frameCopy = static_cast<VideoFrame *>(memory->pointer());
    frameCopy->mWidth = frame->mWidth;
    frameCopy->mHeight = frame->mHeight;
    frameCopy->mDisplayWidth = frame->mDisplayWidth;
    frameCopy->mDisplayHeight = frame->mDisplayHeight;
    frameCopy->mSize = frame->mSize;
    frameCopy->mRotationAngle = frame->mRotationAngle;
    frameCopy->mData = (uint8_t *)frameCopy + sizeof(VideoFrame);
    memcpy(frameCopy->mData, frame->mData, frame->mSize);
    ALOGV("jpegDecode end");
    delete frame;
    return memory;
}

sp<IMemory> MediaServerCallerClient::jpegDecode(int fd)
{
    char *buf0, *buf1;
    int size0, size1;
    void *addrYVir = NULL, *addrUVir = NULL, *addrVVir = NULL;
    VideoPicture *picture;
    JpegInfo jpgInfo;
    status_t ret;
    int i;

    ALOGV("jpegDecode(%d)", fd);
    sp<JpegDecoder> jpgDecoder = new JpegDecoder();
    if (jpgDecoder == NULL) {
        ALOGE("Failed to alloc JpegDecoder!!");
        return NULL;
    }

    int fd_t = dup(fd);
    FILE *fp = fdopen(fd_t, "rb");
    if (fp == NULL) {
        ALOGE("fdopen error(%s), fd=%d", strerror(errno), fd);
        ::close(fd_t);
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    int filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    uint8_t *jpgbuf = (uint8_t*)malloc(filesize);
    if (jpgbuf == NULL) {
        ALOGE("Failed to alloc jpg buffer!");
        fclose(fp);
        return NULL;
    }
    fread(jpgbuf, filesize, 1, fp);
    fclose(fp);
    
    return jpegDecodeBuf(jpgDecoder, jpgbuf, filesize);
}

sp<IMemory> MediaServerCallerClient::jpegDecode(const char *path)
{
    char *buf0, *buf1;
    int size0, size1;
    void *addrYVir = NULL, *addrUVir = NULL, *addrVVir = NULL;
    VideoPicture *picture;
    JpegInfo jpgInfo;
    status_t ret;
    int i;

    ALOGV("jpegDecode(%s)", path);
    sp<JpegDecoder> jpgDecoder = new JpegDecoder();
    if (jpgDecoder == NULL) {
        ALOGE("Failed to alloc JpegDecoder!!");
        return NULL;
    }
    CedarXDataSourceDesc dataSourceDesc;
    memset(&dataSourceDesc, 0, sizeof(CedarXDataSourceDesc));
    dataSourceDesc.source_url = (char*)path;
    dataSourceDesc.source_type = CEDARX_SOURCE_FILEPATH;
    dataSourceDesc.stream_type = CEDARX_STREAM_LOCALFILE;
    struct cdx_stream_info *pStream = create_stream_handle(&dataSourceDesc);
    if (pStream == NULL) {
        ALOGE("(f:%s, l:%d) create_stream_handle error(%s), path=%s", __FUNCTION__, __LINE__, strerror(errno), path);
        return NULL;
    }
    pStream->seek(pStream, 0, SEEK_END);
    int filesize = pStream->tell(pStream);
    pStream->seek(pStream, 0, SEEK_SET);

    uint8_t *jpgbuf = (uint8_t*)malloc(filesize);
    if (jpgbuf == NULL) {
        ALOGE("Failed to alloc jpg buffer!");
        destory_stream_handle(pStream);
        return NULL;
    }
    pStream->read(jpgbuf, 1, filesize, pStream);
    destory_stream_handle(pStream);

    return jpegDecodeBuf(jpgDecoder, jpgbuf, filesize);
}
}; /* namespace android */

