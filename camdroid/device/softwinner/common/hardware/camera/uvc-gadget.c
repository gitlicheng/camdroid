/*
 * UVC gadget test application
 *
 * Copyright (C) 2010 Ideas on board SPRL <laurent.pinchart@ideasonboard.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 */
#include "CameraDebug.h"
#if DBG_UVC_GADGET
#define LOG_NDEBUG 0
#endif
#define LOG_TAG "uvc-gadget"
#include <cutils/log.h>

#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <pthread.h>
#include <linux/usb/ch9.h>

#include "videodev2.h"
#include "video.h"
#include "uvc.h"
#include <vencoder.h>

#include <type_camera.h>


//#define UVC_DEBUG

#define clamp(val, min, max) ({                 \
        typeof(val) __val = (val);              \
        typeof(min) __min = (min);              \
        typeof(max) __max = (max);              \
        (void) (&__val == &__min);              \
        (void) (&__val == &__max);              \
        __val = __val < __min ? __min: __val;   \
        __val > __max ? __max: __val; })

#define ARRAY_SIZE(a)   ((sizeof(a) / sizeof(a[0])))

//static pthread_mutex_t uvc_mutex=PTHREAD_MUTEX_INITIALIZER;

struct uvc_device
{
    int fd;

    struct uvc_streaming_control probe;
    struct uvc_streaming_control commit;

    int control;

    int stream_state;

    unsigned int fcc;
    unsigned int width;
    unsigned int height;

    void **mem;
    unsigned int nbufs;
    unsigned int bufsize;

    unsigned int bulk;
    uint8_t color;
    unsigned int imgsize;
    void *imgdata;

    int force_exit;
    int thread_state;
    pthread_t thread_id;

    int inited;
    pthread_mutex_t uvc_mutex;
};

static void getCurrentDateTime(char *dateTime)
{
    time_t t;
    struct tm *tm_t;
    time(&t);
    tm_t = localtime(&t);
    sprintf(dateTime, "%4d:%02d:%02d %02d:%02d:%02d", 
        tm_t->tm_year+1900, tm_t->tm_mon+1, tm_t->tm_mday,
        tm_t->tm_hour, tm_t->tm_min, tm_t->tm_sec);
}

static int JpegEncode(struct uvc_device *dev, const void* frame, void* pOutBuffer, unsigned int* pOutBufferSize)
{
    V4L2BUF_t * pbuf = (V4L2BUF_t *)frame;
    int result = 0;
    int src_format = 0;
    unsigned int src_addr_phy = 0;
    int src_width = 0;
    int src_height = 0;
    char dataTime[64];
    int nVbvBufferSize = 2*1024*1024;
    JpegEncInfo sjpegInfo;
    EXIFInfo   exifInfo;
    VideoEncoder* pVideoEnc = NULL;
    VencInputBuffer inputBuffer;
    VencOutputBuffer outputBuffer;

    src_format          = pbuf->format;
    src_addr_phy        = pbuf->addrPhyY;
    src_width           = pbuf->crop_rect.width;
    src_height          = pbuf->crop_rect.height;

    memset(&sjpegInfo, 0, sizeof(JpegEncInfo));
    memset(&exifInfo, 0, sizeof(EXIFInfo));

    sjpegInfo.sBaseInfo.nInputWidth = src_width;
    sjpegInfo.sBaseInfo.nInputHeight = src_height;
    sjpegInfo.sBaseInfo.nDstWidth = dev->width;
    sjpegInfo.sBaseInfo.nDstHeight = dev->height;
    sjpegInfo.sBaseInfo.eInputFormat = (src_format == V4L2_PIX_FMT_NV21) ? VENC_PIXEL_YVU420SP: VENC_PIXEL_YUV420SP;
    sjpegInfo.quality       = 80;//90;

    sjpegInfo.pAddrPhyY = (unsigned char*)src_addr_phy;
    sjpegInfo.pAddrPhyC = (unsigned char*)src_addr_phy + ALIGN_16B(src_width) * ALIGN_16B(src_height);

    sjpegInfo.bEnableCorp  = 0;
    sjpegInfo.sCropInfo.nLeft = 0;
    sjpegInfo.sCropInfo.nTop = 0;
    sjpegInfo.sCropInfo.nWidth = src_width;
    sjpegInfo.sCropInfo.nHeight = src_height;

    //getCurrentDateTime(dataTime);
    //strcpy((char*)exifInfo.CameraMake,  "MID MAKE");
    //strcpy((char*)exifInfo.CameraModel, "MID MODEL");
    //strcpy((char*)exifInfo.DateTime, dataTime);

    exifInfo.ThumbWidth = 320;
    exifInfo.ThumbHeight = 240;
    exifInfo.Orientation = 0;
    exifInfo.enableGpsInfo = 0;
    exifInfo.WhiteBalance = 0;

    pVideoEnc = VideoEncCreate(VENC_CODEC_JPEG);
    if (pVideoEnc == NULL) {
        ALOGE("<F:%s, L:%d> VideoEncCreate failed!", __FUNCTION__, __LINE__);
        return -1;
    }
    //VideoEncSetParameter(pVideoEnc, VENC_IndexParamJpegExifInfo, &exifInfo);
    VideoEncSetParameter(pVideoEnc, VENC_IndexParamJpegQuality, &sjpegInfo.quality);
    VideoEncSetParameter(pVideoEnc, VENC_IndexParamSetVbvSize, &nVbvBufferSize);

    if (VideoEncInit(pVideoEnc, &sjpegInfo.sBaseInfo)< 0) {
        ALOGE("VideoEncInit failed");
        return -1;
    }

    memset(&inputBuffer, 0, sizeof(VencInputBuffer));

#ifdef UVC_DEBUG
    VideoEncDestroy(pVideoEnc);
    memcpy(pOutBuffer, dev->imgdata, dev->imgsize);
    *pOutBufferSize = dev->imgsize;
    return 0;
#endif

    inputBuffer.pAddrPhyY = sjpegInfo.pAddrPhyY;
    inputBuffer.pAddrPhyC = sjpegInfo.pAddrPhyC;
    inputBuffer.bEnableCorp         = sjpegInfo.bEnableCorp;
    inputBuffer.sCropInfo.nLeft     =  sjpegInfo.sCropInfo.nLeft;
    inputBuffer.sCropInfo.nTop      =  sjpegInfo.sCropInfo.nTop;
    inputBuffer.sCropInfo.nWidth    =  sjpegInfo.sCropInfo.nWidth;
    inputBuffer.sCropInfo.nHeight   =  sjpegInfo.sCropInfo.nHeight;

    AddOneInputBuffer(pVideoEnc, &inputBuffer);
    if (VideoEncodeOneFrame(pVideoEnc)!= 0) {
        ALOGE("(f:%s, l:%d) jpeg encoder error", __FUNCTION__, __LINE__);
    }

    AlreadyUsedInputBuffer(pVideoEnc,&inputBuffer);

    memset(&outputBuffer, 0, sizeof(VencOutputBuffer));
    result = GetOneBitstreamFrame(pVideoEnc, &outputBuffer);
    if (result < 0) {
        ALOGE("GetOneBitstreamFrame return ret(%d)", result);
        goto Exit;
    }

    //ALOGV("pData0(%p), nSize0(%u), pData1(%p), nSize1(%u)",
    //    outputBuffer.pData0, outputBuffer.nSize0, outputBuffer.pData1, outputBuffer.nSize1);

    if (outputBuffer.nSize0 + outputBuffer.nSize1 > dev->imgsize) {
        ALOGE("nSize0(%d) + nSize1(%d) > imagsize(%d)", outputBuffer.nSize0,
            outputBuffer.nSize1, dev->imgsize);
        result = -1;
    } else {
        memcpy(pOutBuffer, outputBuffer.pData0, outputBuffer.nSize0);
        if(outputBuffer.nSize1) {
            memcpy(((unsigned char*)pOutBuffer + outputBuffer.nSize0), outputBuffer.pData1, outputBuffer.nSize1);

            *pOutBufferSize = outputBuffer.nSize0 + outputBuffer.nSize1;
        } else {
            *pOutBufferSize = outputBuffer.nSize0;
        }
    }
    FreeOneBitStreamFrame(pVideoEnc, &outputBuffer);

Exit:
    if (pVideoEnc) {
        VideoEncDestroy(pVideoEnc);
    }
    return result;
}

static struct uvc_device *uvc_open(const char *devname)
{
    struct uvc_device *dev;
    struct v4l2_capability cap;
    int i, ret;
    int fd = -1;
    char dev_node[64];

    for (i = 0; i < 254; ++i) {
        snprintf(dev_node, 64, "/dev/video%d", i);
        ret = access(dev_node, F_OK);
        if (ret != 0) {
            continue;
        }
        fd = open(dev_node, O_RDWR | O_NONBLOCK);
        if (fd < 0) {
            continue;
        }
        ret = ioctl (fd, VIDIOC_QUERYCAP, &cap);
        if (ret < 0) {
            close(fd);
            continue;
        }
        ALOGV("<F:%s, L:%d>%s driver name %s", __FUNCTION__, __LINE__, dev_node, cap.driver);
        if (strcmp((char*)cap.driver, "g_uvc") == 0) {
            ALOGD("uvcvideo device node is %s", dev_node);
            break;
        }
        close(fd);
        fd = -1;
    }

    if (fd < 0) {
        ALOGE("Could not find g_uvc device");
        return NULL;
    }

    ALOGD("device is %s on bus %s", cap.card, cap.bus_info);

    dev = malloc(sizeof *dev);
    if (dev == NULL) {
        close(fd);
        return NULL;
    }

    memset(dev, 0, sizeof *dev);
    dev->fd = fd;

    return dev;
}

static void uvc_close(struct uvc_device *dev)
{
    close(dev->fd);
    dev->fd = 0;
    //free(dev->imgdata);
    free(dev->mem);
    free(dev);
}

/* ---------------------------------------------------------------------------
 * Video streaming
 */
static void uvc_video_fill_buffer(struct uvc_device *dev, struct v4l2_buffer *buf)
{
    unsigned int bpl;
    unsigned int i;

    switch (dev->fcc) {
    case V4L2_PIX_FMT_YUYV:
        /* Fill the buffer with video data. */
        bpl = dev->width * 2;
        for (i = 0; i < dev->height; ++i)
            memset((char*)dev->mem[buf->index] + i*bpl, dev->color++, bpl);

        buf->bytesused = bpl * dev->height;
        break;

    case V4L2_PIX_FMT_MJPEG:
        memcpy(dev->mem[buf->index], dev->imgdata, dev->imgsize);
        buf->bytesused = dev->imgsize;
        break;
    }
}

static int uvc_video_process_internal(struct uvc_device *dev, V4L2BUF_t *pbuf)
{
    struct v4l2_buffer buf;
    int ret;

    memset(&buf, 0, sizeof buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    buf.memory = V4L2_MEMORY_MMAP;

    if ((ret = ioctl(dev->fd, VIDIOC_DQBUF, &buf)) < 0) {
        ALOGE("Unable to dequeue buffer: %s (%d).", strerror(errno), errno);
        return ret;
    }

    //uvc_video_fill_buffer(dev, &buf, pbuf);
    //memcpy(dev->mem[buf.index], pbuf->addrVirY, pbuf->bytesused);
    //buf.bytesused = pbuf->bytesused;
    if (JpegEncode(dev, pbuf, dev->mem[buf.index], &buf.bytesused) < 0) {
        ALOGE("%s: JpegEncode failed.", __func__);
        buf.bytesused = 0;
    }

    if ((ret = ioctl(dev->fd, VIDIOC_QBUF, &buf)) < 0) {
        ALOGE("Unable to requeue buffer: %s (%d).", strerror(errno), errno);
        return ret;
    }

    return 0;
}

static int uvc_video_reqbufs(struct uvc_device *dev, int nbufs)
{
    struct v4l2_requestbuffers rb;
    struct v4l2_buffer buf;
    unsigned int i;
    int ret;

    for (i = 0; i < dev->nbufs; ++i)
        munmap(dev->mem[i], dev->bufsize);

    free(dev->mem);
    dev->mem = NULL;
    dev->nbufs = 0;

    memset(&rb, 0, sizeof rb);
    rb.count = nbufs;
    rb.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    rb.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(dev->fd, VIDIOC_REQBUFS, &rb);
    if (ret < 0) {
        ALOGE("Unable to allocate buffers: %s (%d).", strerror(errno), errno);
        return ret;
    }

    ALOGV("%u buffers allocated.", rb.count);

    /* Map the buffers. */
    dev->mem = malloc(rb.count * sizeof dev->mem[0]);

    for (i = 0; i < rb.count; ++i) {
        memset(&buf, 0, sizeof buf);
        buf.index = i;
        buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        buf.memory = V4L2_MEMORY_MMAP;
        ret = ioctl(dev->fd, VIDIOC_QUERYBUF, &buf);
        if (ret < 0) {
            ALOGE("Unable to query buffer %u: %s (%d).", i, strerror(errno), errno);
            return -1;
        }
        ALOGV("length: %u offset: %u", buf.length, buf.m.offset);

        dev->mem[i] = mmap(0, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, dev->fd, buf.m.offset);
        if (dev->mem[i] == MAP_FAILED) {
            ALOGE("Unable to map buffer %u: %s (%d)", i, strerror(errno), errno);
            return -1;
        }
        ALOGV("Buffer %u mapped at address %p.", i, dev->mem[i]);
    }

    dev->bufsize = buf.length;
    dev->nbufs = rb.count;

    return 0;
}

static int uvc_video_stream(struct uvc_device *dev, int enable)
{
    struct v4l2_buffer buf;
    unsigned int i;
    int type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    int ret = 0;

    if (!enable) {
        ALOGV("Stopping video stream.");
        ioctl(dev->fd, VIDIOC_STREAMOFF, &type);
        return 0;
    }

    ALOGV("Starting video stream.");

    for (i = 0; i < dev->nbufs; ++i) {
        memset(&buf, 0, sizeof buf);

        buf.index = i;
        buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        buf.memory = V4L2_MEMORY_MMAP;

        //uvc_video_fill_buffer(dev, &buf);

        ALOGV("Queueing buffer %u.", i);
        if ((ret = ioctl(dev->fd, VIDIOC_QBUF, &buf)) < 0) {
            ALOGE("Unable to queue buffer: %s (%d).", strerror(errno), errno);
            break;
        }
    }

    ioctl(dev->fd, VIDIOC_STREAMON, &type);
    return ret;
}

static int uvc_video_set_format(struct uvc_device *dev)
{
    struct v4l2_format fmt;
    int ret;

    ALOGV("Setting format to 0x%08x %ux%u", dev->fcc, dev->width, dev->height);

    memset(&fmt, 0, sizeof fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    fmt.fmt.pix.width = dev->width;
    fmt.fmt.pix.height = dev->height;
    fmt.fmt.pix.pixelformat = dev->fcc;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;
    if (dev->fcc == V4L2_PIX_FMT_MJPEG)
        fmt.fmt.pix.sizeimage = dev->imgsize * 1.5;

    if ((ret = ioctl(dev->fd, VIDIOC_S_FMT, &fmt)) < 0)
        ALOGE("Unable to set format: %s (%d).", strerror(errno), errno);

    return ret;
}

static int uvc_video_init(struct uvc_device *dev __attribute__((__unused__)))
{
    return 0;
}

/* ---------------------------------------------------------------------------
 * Request processing
 */

struct uvc_frame_info
{
    unsigned int width;
    unsigned int height;
    unsigned int intervals[8];
};

struct uvc_format_info
{
    unsigned int fcc;
    const struct uvc_frame_info *frames;
};

static const struct uvc_frame_info uvc_frames_yuyv[] = {
    {  640, 360, { 666666, 10000000, 50000000, 0 }, },
    { 1280, 720, { 50000000, 0 }, },
    { 0, 0, { 0, }, },
};

static const struct uvc_frame_info uvc_frames_mjpeg[] = {
    //{  640, 360, { 666666, 10000000, 50000000, 0 }, },
    { 1280, 720, { 333333, 0 }, },
    { 0, 0, { 0, }, },
};

static const struct uvc_format_info uvc_formats[] = {
    //{ V4L2_PIX_FMT_YUYV, uvc_frames_yuyv },
    { V4L2_PIX_FMT_MJPEG, uvc_frames_mjpeg },
};

static void uvc_fill_streaming_control(struct uvc_device *dev,
               struct uvc_streaming_control *ctrl,
               int iframe, int iformat)
{
    const struct uvc_format_info *format;
    const struct uvc_frame_info *frame;
    unsigned int nframes;

    if (iformat < 0)
        iformat = ARRAY_SIZE(uvc_formats) + iformat;
    if (iformat < 0 || iformat >= (int)ARRAY_SIZE(uvc_formats))
        return;
    format = &uvc_formats[iformat];

    nframes = 0;
    while (format->frames[nframes].width != 0)
        ++nframes;

    if (iframe < 0)
        iframe = nframes + iframe;
    if (iframe < 0 || iframe >= (int)nframes)
        return;
    frame = &format->frames[iframe];

    memset(ctrl, 0, sizeof *ctrl);

    ctrl->bmHint = 1;
    ctrl->bFormatIndex = iformat + 1;
    ctrl->bFrameIndex = iframe + 1;
    ctrl->dwFrameInterval = frame->intervals[0];
    switch (format->fcc) {
    case V4L2_PIX_FMT_YUYV:
        ctrl->dwMaxVideoFrameSize = frame->width * frame->height * 2;
        break;
    case V4L2_PIX_FMT_MJPEG:
        ctrl->dwMaxVideoFrameSize = dev->imgsize;
        break;
    }
    ctrl->dwMaxPayloadTransferSize = 512;   /* TODO this should be filled by the driver. */
    ctrl->bmFramingInfo = 3;
    ctrl->bPreferedVersion = 1;
    ctrl->bMaxVersion = 1;
}

static void uvc_events_process_standard(struct uvc_device *dev, struct usb_ctrlrequest *ctrl,
                struct uvc_request_data *resp)
{
    ALOGV("standard request");
    (void)dev;
    (void)ctrl;
    (void)resp;
}

static void uvc_events_process_control(struct uvc_device *dev, uint8_t req, uint8_t cs,
               struct uvc_request_data *resp)
{
    ALOGV("control request (req %02x cs %02x)", req, cs);
    (void)dev;
    (void)resp;
}

static void uvc_events_process_streaming(struct uvc_device *dev, uint8_t req, uint8_t cs,
                 struct uvc_request_data *resp)
{
    struct uvc_streaming_control *ctrl;

    ALOGV("streaming request (req %02x cs %02x)", req, cs);

    if (cs != UVC_VS_PROBE_CONTROL && cs != UVC_VS_COMMIT_CONTROL)
        return;

    ctrl = (struct uvc_streaming_control *)&resp->data;
    resp->length = sizeof *ctrl;

    switch (req) {
    case UVC_SET_CUR:
        dev->control = cs;
        resp->length = 34;
        break;

    case UVC_GET_CUR:
        if (cs == UVC_VS_PROBE_CONTROL)
            memcpy(ctrl, &dev->probe, sizeof *ctrl);
        else
            memcpy(ctrl, &dev->commit, sizeof *ctrl);
        break;

    case UVC_GET_MIN:
    case UVC_GET_MAX:
    case UVC_GET_DEF:
        uvc_fill_streaming_control(dev, ctrl, req == UVC_GET_MAX ? -1 : 0,
                       req == UVC_GET_MAX ? -1 : 0);
        break;

    case UVC_GET_RES:
        memset(ctrl, 0, sizeof *ctrl);
        break;

    case UVC_GET_LEN:
        resp->data[0] = 0x00;
        resp->data[1] = 0x22;
        resp->length = 2;
        break;

    case UVC_GET_INFO:
        resp->data[0] = 0x03;
        resp->length = 1;
        break;
    }
}

static void uvc_events_process_class(struct uvc_device *dev, struct usb_ctrlrequest *ctrl,
             struct uvc_request_data *resp)
{
    if ((ctrl->bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE)
        return;

    switch (ctrl->wIndex & 0xff) {
    case UVC_INTF_CONTROL:
        uvc_events_process_control(dev, ctrl->bRequest, ctrl->wValue >> 8, resp);
        break;

    case UVC_INTF_STREAMING:
        uvc_events_process_streaming(dev, ctrl->bRequest, ctrl->wValue >> 8, resp);
        break;

    default:
        break;
    }
}

static void uvc_events_process_setup(struct uvc_device *dev, struct usb_ctrlrequest *ctrl,
             struct uvc_request_data *resp)
{
    dev->control = 0;

    ALOGV("bRequestType %02x bRequest %02x wValue %04x wIndex %04x wLength %04x",
        ctrl->bRequestType, ctrl->bRequest, ctrl->wValue, ctrl->wIndex, ctrl->wLength);

    switch (ctrl->bRequestType & USB_TYPE_MASK) {
    case USB_TYPE_STANDARD:
        uvc_events_process_standard(dev, ctrl, resp);
        break;

    case USB_TYPE_CLASS:
        uvc_events_process_class(dev, ctrl, resp);
        break;

    default:
        break;
    }
}

static void uvc_events_process_data(struct uvc_device *dev, struct uvc_request_data *data)
{
    struct uvc_streaming_control *target;
    struct uvc_streaming_control *ctrl;
    const struct uvc_format_info *format;
    const struct uvc_frame_info *frame;
    const unsigned int *interval;
    unsigned int iformat, iframe;
    unsigned int nframes;

    switch (dev->control) {
    case UVC_VS_PROBE_CONTROL:
        ALOGV("setting probe control, length = %d", data->length);
        target = &dev->probe;
        break;

    case UVC_VS_COMMIT_CONTROL:
        ALOGV("setting commit control, length = %d", data->length);
        target = &dev->commit;
        break;

    default:
        ALOGV("setting unknown control, length = %d", data->length);
        return;
    }

    ctrl = (struct uvc_streaming_control *)&data->data;
    iformat = clamp((unsigned int)ctrl->bFormatIndex, 1U,
            (unsigned int)ARRAY_SIZE(uvc_formats));
    format = &uvc_formats[iformat-1];

    nframes = 0;
    while (format->frames[nframes].width != 0)
        ++nframes;

    iframe = clamp((unsigned int)ctrl->bFrameIndex, 1U, nframes);
    frame = &format->frames[iframe-1];
    interval = frame->intervals;

    while (interval[0] < ctrl->dwFrameInterval && interval[1])
        ++interval;

    target->bFormatIndex = iformat;
    target->bFrameIndex = iframe;
    switch (format->fcc) {
    case V4L2_PIX_FMT_YUYV:
        target->dwMaxVideoFrameSize = frame->width * frame->height * 2;
        break;
    case V4L2_PIX_FMT_MJPEG:
        if (dev->imgsize == 0)
            ALOGW("WARNING: MJPEG requested and no image loaded.");
        target->dwMaxVideoFrameSize = dev->imgsize;
        break;
    }
    target->dwFrameInterval = *interval;

    if (dev->control == UVC_VS_COMMIT_CONTROL) {
        dev->fcc = format->fcc;
        dev->width = frame->width;
        dev->height = frame->height;

        uvc_video_set_format(dev);
        if (dev->bulk)
            uvc_video_stream(dev, 1);
    }
}

static void uvc_events_process(struct uvc_device *dev)
{
    struct v4l2_event v4l2_event;
    struct uvc_event *uvc_event = (void *)&v4l2_event.u.data;
    struct uvc_request_data resp;
    int ret;

    ALOGV("%s", __func__);

    ret = ioctl(dev->fd, VIDIOC_DQEVENT, &v4l2_event);
    if (ret < 0) {
        ALOGE("VIDIOC_DQEVENT failed: %s (%d)", strerror(errno), errno);
        return;
    }

    memset(&resp, 0, sizeof resp);
    resp.length = -EL2HLT;

    switch (v4l2_event.type) {
    case UVC_EVENT_CONNECT:
        ALOGV("UVC_EVENT_CONNECT");
        return;

    case UVC_EVENT_DISCONNECT:
        ALOGV("UVC_EVENT_DISCONNECT");
        return;

    case UVC_EVENT_SETUP:
        ALOGV("UVC_EVENT_SETUP");
        uvc_events_process_setup(dev, &uvc_event->req, &resp);
        break;

    case UVC_EVENT_DATA:
        ALOGV("UVC_EVENT_DATA");
        uvc_events_process_data(dev, &uvc_event->data);
        return;

    case UVC_EVENT_STREAMON:
        ALOGV("UVC_EVENT_STREAMON");
        pthread_mutex_lock(&dev->uvc_mutex);
        uvc_video_reqbufs(dev, 7);  //4
        uvc_video_stream(dev, 1);
        dev->stream_state = 1;
        //resp.length = 0;
        pthread_mutex_unlock(&dev->uvc_mutex);
        break;

    case UVC_EVENT_STREAMOFF:
        ALOGV("UVC_EVENT_STREAMOFF");
        pthread_mutex_lock(&dev->uvc_mutex);
        dev->stream_state = 0;
        uvc_video_stream(dev, 0);
        uvc_video_reqbufs(dev, 0);
        //resp.length = 0;
        pthread_mutex_unlock(&dev->uvc_mutex);
        break;
    }

    ioctl(dev->fd, UVCIOC_SEND_RESPONSE, &resp);
    if (ret < 0) {
        ALOGE("UVCIOC_S_EVENT failed: %s (%d)", strerror(errno), errno);
        return;
    }
}

static void uvc_events_init(struct uvc_device *dev)
{
    struct v4l2_event_subscription sub;

    uvc_fill_streaming_control(dev, &dev->probe, 0, 0);
    uvc_fill_streaming_control(dev, &dev->commit, 0, 0);

    if (dev->bulk) {
        /* FIXME Crude hack, must be negotiated with the driver. */
        dev->probe.dwMaxPayloadTransferSize = 16 * 1024;
        dev->commit.dwMaxPayloadTransferSize = 16 * 1024;
    }


    memset(&sub, 0, sizeof sub);
    sub.type = UVC_EVENT_SETUP;
    ioctl(dev->fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    sub.type = UVC_EVENT_DATA;
    ioctl(dev->fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    sub.type = UVC_EVENT_STREAMON;
    ioctl(dev->fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    sub.type = UVC_EVENT_STREAMOFF;
    ioctl(dev->fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
}

static void *process_thread(void *arg)
{
    struct uvc_device *dev = (struct uvc_device *)arg;
    fd_set fds;
    int ret;

    dev->thread_state = 1;

    FD_ZERO(&fds);
    FD_SET(dev->fd, &fds);

    while (!dev->force_exit) {
        struct timeval tv;
        fd_set efds = fds;
        //fd_set wfds = fds;
        /* Timeout */
        tv.tv_sec  = 0;
        tv.tv_usec = 300 * 1000;
        ret = select(dev->fd + 1, NULL, NULL/*&wfds*/, &efds, &tv);
        if (FD_ISSET(dev->fd, &efds))
            uvc_events_process(dev);
        //if (FD_ISSET(dev->fd, &wfds))
        //  uvc_video_process(dev);
    }

    dev->thread_state = 0;
    ALOGV("process_thread exit");
    return NULL;
}

#ifdef UVC_DEBUG
static int image_load(struct uvc_device *dev)
{
    int fd = -1;

    fd = open("/mnt/extsd/uvc_test.jpg", O_RDONLY);
    if (fd == -1)
    {
        ALOGE("Unable to open MJPEG image '/mnt/extsd/uvc_test.jpg'");
        return -1;
    }
    dev->imgsize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    dev->imgdata = malloc(dev->imgsize);
    if (dev->imgdata == NULL) {
        ALOGE("Unable to allocate memory for MJPEG image");
        dev->imgsize = 0;
        return -1;
    }
    read(fd, dev->imgdata, dev->imgsize);
    close(fd);
    return 0;
}
#endif

void *uvc_init(const char *devname)
{
    struct uvc_device *dev;

    ALOGI("uvc_init");
    dev = uvc_open(devname);
    if (dev == NULL)
        return NULL;

    dev->bulk = 0;

    uvc_events_init(dev);
    uvc_video_init(dev);

    dev->width = 1280;
    dev->height = 720;

#ifdef UVC_DEBUG
    if (image_load(dev) < 0) {
        uvc_close(dev);
        return NULL;
    }
#else
    dev->imgsize = 200 * 1000;  //dev->width * dev->height / 2;
#endif

    dev->force_exit = 0;
    pthread_create(&dev->thread_id, NULL, process_thread, dev);

    pthread_mutex_init(&dev->uvc_mutex, NULL);
    dev->inited = 1;

    return (void *)dev;
}

int uvc_exit(void *uvc_dev)
{
    struct uvc_device *dev = (struct uvc_device *)uvc_dev;

    ALOGI("uvc_exit");
    //pthread_mutex_lock(&uvc_mutex);
    if (dev && dev->inited) {
        dev->inited = 0;

        dev->force_exit = 1;
        pthread_join(dev->thread_id, NULL);

        if (dev->stream_state) {
            dev->stream_state = 0;
            uvc_video_stream(dev, 0);
            uvc_video_reqbufs(dev, 0);
        }
        pthread_mutex_destroy(&dev->uvc_mutex);
#ifdef UVC_DEBUG
        free(dev->imgdata);
#endif
        uvc_close(dev);
    }
    //pthread_mutex_unlock(&uvc_mutex);

    return 0;
}

int uvc_video_process(void *uvc_dev, V4L2BUF_t *pbuf)
{
    struct uvc_device *dev = (struct uvc_device *)uvc_dev;
    int ret = -1;
    //static int div_rate = 0;

    pthread_mutex_lock(&dev->uvc_mutex);
    if (dev && dev->inited) {
        if (dev->stream_state == 1) {
            ret = uvc_video_process_internal(dev, pbuf);
        } else {
            //ALOGV("uvc_video_process: UVC Stream off");
        }
    }
    pthread_mutex_unlock(&dev->uvc_mutex);
    return ret;
}

