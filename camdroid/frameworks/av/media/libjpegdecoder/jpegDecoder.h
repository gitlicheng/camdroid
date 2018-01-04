#ifndef __JPEG_DECODER_H__
#define __JPEG_DECODER_H__

#include <vdecoder.h>


namespace android {

typedef struct JpegInfo
{
    int width;
    int height;
    uint8_t *data;
    uint32_t datasize;
} JpegInfo;

class JpegDecoder : public RefBase
{
public:
    JpegDecoder();
    ~JpegDecoder();

    status_t decode();
    status_t initialize(int jpgWidth, int jpgHeight);
    status_t destroy();
    VideoPicture*  getFrame(void);
    status_t releaseFrame(VideoPicture *picture);
    status_t requestBuffer(int size, char **buf0, int *size0, char **buf1, int *size1);
    status_t updateBuffer(int size, int64_t pts, char *data);
    status_t getJpgInfo(JpegInfo *jpgInfo);

    static void YV12_to_RGB24(unsigned char *yv12, unsigned char *rgb24, int width, int height);
    static void RGB24_to_ARGB(unsigned char *rgb24, unsigned char *argb, int width, int height);

private:
    VideoDecoder *mDecoder;
    VideoStreamInfo mStreamInfo;
    VConfig mConfig;
    VideoStreamDataInfo mDataInfo;
};

}; /* namespace android */

#endif /* __JPEG_DECODER_H__ */
