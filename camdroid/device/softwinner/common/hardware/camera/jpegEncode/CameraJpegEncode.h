
#ifndef __CAMERA_JPEG_ENCODE_H__
#define __CAMERA_JPEG_ENCODE_H__

#include <vencoder.h>


namespace android {

typedef struct CameraJpegEncConfig
{
    unsigned int nVbvBufferSize;
} CameraJpegEncConfig;

class CameraJpegEncode
{
public:
    CameraJpegEncode();
    ~CameraJpegEncode();

    status_t initialize(CameraJpegEncConfig *pConfig, JpegEncInfo *pJpegInfo, EXIFInfo *pExifInfo);
    status_t destroy();
    status_t encode(JpegEncInfo *pJpegInfo);
    int getFrame();
    int returnFrame();
    size_t getDataSize();
    status_t getDataToBuffer(void *buffer);

private:
    VideoEncoder *mpVideoEnc;
    VencOutputBuffer mOutBuffer;
};

};

#endif /* __CAMERA_JPEG_ENCODE_H__ */

