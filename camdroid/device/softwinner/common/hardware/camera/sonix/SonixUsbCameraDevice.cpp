#include "CameraDebug.h"
#if DBG_V4L2_CAMERA
#define LOG_NDEBUG 0
#endif
#define LOG_TAG "SonixUsbCameraDevice"
#include <cutils/log.h>

#include <sys/mman.h> 
#include <fcntl.h> 
#include <utils/Thread.h>
#include <hardware/camera.h>
#include <videodev2.h>
#include <linux/videodev.h> 
#include "utils/Errors.h"  // for status_t
#include "utils/String8.h"

#include "SonixUsbCameraDevice.h"

#include "v4l2uvc.h"
#include "sonix_xu_ctrls.h"
#include "nalu.h"

namespace android {

SonixUsbCameraDevice::SonixUsbCameraDevice(void)
	: mH264NumBufs(0)
	, mH264Fd(-1)
	, mUseH264DevRec(false)
{
}

SonixUsbCameraDevice::~SonixUsbCameraDevice(void)
{
}

int SonixUsbCameraDevice::video_open(const char *devname)
{
	struct v4l2_capability cap;
	int dev, ret;

	dev = open(devname, O_RDWR);
	if (dev < 0) {
		LOGE("Error opening device %s: %d.", devname, errno);
		return dev;
	}

	memset(&cap, 0, sizeof cap);
	ret = ioctl(dev, VIDIOC_QUERYCAP, &cap);
	if (ret < 0) {
		LOGE("Error opening device %s: unable to query device.",
			devname);
		close(dev);
		return ret;
	}

#if 0
	if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) {
		LOGV("Error opening device %s: video capture not supported.",
			devname);
		close(dev);
		return -EINVAL;
	}
#endif

	LOGV("Device %s opened: %s.", devname, cap.card);
	return dev;
}

void SonixUsbCameraDevice::uvc_set_control(int dev, unsigned int id, int value)
{
	struct v4l2_control ctrl;
	int ret;

	ctrl.id = id;
	ctrl.value = value;

	ret = ioctl(dev, VIDIOC_S_CTRL, &ctrl);
	if (ret < 0) {
		LOGE("unable to set gain control: %s (%d).",
			strerror(errno), errno);
		return;
	}
}

int SonixUsbCameraDevice::video_set_format(int dev, unsigned int w, unsigned int h, unsigned int format)
{
	struct v4l2_format fmt;
	int ret;

	memset(&fmt, 0, sizeof fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = w;
	fmt.fmt.pix.height = h;
	fmt.fmt.pix.pixelformat = format;
	fmt.fmt.pix.field = V4L2_FIELD_ANY;

	ret = ioctl(dev, VIDIOC_S_FMT, &fmt);
	if (ret < 0) {
		LOGE("Unable to set format: %s.", strerror(errno));
		return ret;
	}

	LOGV("Video format set: width: %u height: %u buffer size: %u",
		fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.sizeimage);
	return 0;
}

int SonixUsbCameraDevice::video_set_framerate(int dev)
{
	struct v4l2_streamparm parm;
	int ret;

	memset(&parm, 0, sizeof parm);
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	ret = ioctl(dev, VIDIOC_G_PARM, &parm);
	if (ret < 0) {
		LOGE("Unable to get frame rate: %d.", errno);
		return ret;
	}

	LOGV("Current frame rate: %u/%u",
		parm.parm.capture.timeperframe.numerator,
		parm.parm.capture.timeperframe.denominator);

	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = 30;	//30fps
	parm.parm.capture.capturemode = 0x0002;

	ret = ioctl(dev, VIDIOC_S_PARM, &parm);
	if (ret < 0) {
		LOGE("Unable to set frame rate: %d.", errno);
		return ret;
	}

	ret = ioctl(dev, VIDIOC_G_PARM, &parm);
	if (ret < 0) {
		LOGE("Unable to get frame rate: %d.", errno);
		return ret;
	}

	LOGV("Frame rate set: %u/%u",
		parm.parm.capture.timeperframe.numerator,
		parm.parm.capture.timeperframe.denominator);
	return 0;
}

int SonixUsbCameraDevice::video_reqbufs(int dev, int nbufs)
{
	struct v4l2_requestbuffers rb;
	int ret;

	memset(&rb, 0, sizeof rb);
	rb.count = nbufs;
	rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	rb.memory = V4L2_MEMORY_MMAP;

	ret = ioctl(dev, VIDIOC_REQBUFS, &rb);
	if (ret < 0) {
		LOGE("Unable to allocate buffers: %d.", errno);
		return ret;
	}

	LOGV("%u buffers allocated.", rb.count);
	return rb.count;
}


int SonixUsbCameraDevice::video_querybuf(int dev, Mem_Map_t* mem_map, int nbufs)
{
	struct v4l2_buffer buf0;
	int ret, i;
	/* Map the buffers. */
	for (i = 0; i < nbufs; ++i) {
		memset(&buf0, 0, sizeof buf0);
		buf0.index = i;
		buf0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf0.memory = V4L2_MEMORY_MMAP;
		
		ret = ioctl(dev, VIDIOC_QUERYBUF, &buf0);
		if (ret < 0) {
			LOGE("Unable to query buffer %u (%d).", i, errno);
			return ret;
		}
		
		LOGV("length: %u offset: %10u     --  ", buf0.length, buf0.m.offset);

		mem_map->mem[i] = mmap(0, buf0.length, PROT_READ | PROT_WRITE, 
								MAP_SHARED, dev, buf0.m.offset);
		
		mem_map->length = buf0.length;
	
		if (mem_map->mem[i] == MAP_FAILED) {
			return ret;
		}
		
		LOGV("Buffer %u mapped at address %p.", i, mem_map->mem[i]);
	}

	/* Queue the buffers. */
	for (i = 0; i < nbufs; ++i) {
		memset(&buf0, 0, sizeof buf0);
		buf0.index = i;
		buf0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf0.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(dev, VIDIOC_QBUF, &buf0);
		if (ret < 0) {
			LOGE("Unable to queue buffer0(%d).", errno);
			return ret;
		}
	}

	return 0;
}


int SonixUsbCameraDevice::video_unmapBuf(Mem_Map_t* mem_map, int nbufs)
{
	int ret, i;

	for (i = 0; i < nbufs; i++) 
	{
		ret = munmap(mem_map->mem[i], mem_map->length);
		if (ret < 0) 
		{
			LOGE("v4l2CloseBuf Unmap failed"); 
			return ret;
		}

		mem_map->mem[i] = NULL;

	}
	
	return 0;
}


int SonixUsbCameraDevice::video_enable(int dev, int enable)
{
	int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	int ret;

	ret = ioctl(dev, enable ? VIDIOC_STREAMON : VIDIOC_STREAMOFF, &type);
	if (ret < 0) {
		LOGE("Unable to %s capture: %d.",
			enable ? "start" : "stop", errno);
		return ret;
	}

	return 0;
}

void SonixUsbCameraDevice::video_query_menu(int dev, unsigned int id)
{
	struct v4l2_querymenu menu;
	int ret;

	menu.index = 0;
	while (1) {
		menu.id = id;
		ret = ioctl(dev, VIDIOC_QUERYMENU, &menu);
		if (ret < 0)
			break;

		LOGV("  %u: %.32s", menu.index, menu.name);
		menu.index++;
	};
}

void SonixUsbCameraDevice::video_list_controls(int dev)
{
	struct v4l2_queryctrl query;
	struct v4l2_control ctrl;
	char value[12];
	int ret;

#ifndef V4L2_CTRL_FLAG_NEXT_CTRL
	unsigned int i;
	for (i = V4L2_CID_BASE; i <= V4L2_CID_LASTP1; ++i)
#else
	query.id = 0;
	while (1)
#endif
	{
#ifndef V4L2_CTRL_FLAG_NEXT_CTRL
		query.id = i;
#else
		query.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
#endif
		ret = ioctl(dev, VIDIOC_QUERYCTRL, &query);
		if (ret < 0)
			break;

		if (query.flags & V4L2_CTRL_FLAG_DISABLED)
			continue;

		ctrl.id = query.id;
		ret = ioctl(dev, VIDIOC_G_CTRL, &ctrl);
		if (ret < 0)
			strcpy(value, "n/a");
		else
			sprintf(value, "%d", ctrl.value);

		LOGV("control 0x%08x %s min %d max %d step %d default %d current %s.",
			query.id, query.name, query.minimum, query.maximum,
			query.step, query.default_value, value);

		if (query.type == V4L2_CTRL_TYPE_MENU)
			video_query_menu(dev, query.id);

	}
}

void SonixUsbCameraDevice::video_enum_inputs(int dev)
{
	struct v4l2_input input;
	unsigned int i;
	int ret;

	for (i = 0; ; ++i) {
		memset(&input, 0, sizeof input);
		input.index = i;
		ret = ioctl(dev, VIDIOC_ENUMINPUT, &input);
		if (ret < 0)
			break;

		if (i != input.index)
			LOGW("Warning: driver returned wrong input index "
				"%u.", input.index);

		LOGV("Input %u: %s.", i, input.name);
	}
}

int SonixUsbCameraDevice::video_get_input(int dev)
{
	__u32 input;
	int ret;

	ret = ioctl(dev, VIDIOC_G_INPUT, &input);
	if (ret < 0) {
		LOGE("Unable to get current input: %s.", strerror(errno));
		return ret;
	}

	return input;
}

int SonixUsbCameraDevice::video_dequeue_buffer(int dev, struct v4l2_buffer *buf)
{
	int ret;
	
	buf->type	= V4L2_BUF_TYPE_VIDEO_CAPTURE; 
	buf->memory = V4L2_MEMORY_MMAP; 
 
	ret = ioctl(dev, VIDIOC_DQBUF, buf); 
	if (ret < 0) 
	{ 
		LOGW("Unable to dequeue buffer0 (%d).", errno);
		return ret;			// can not return false
	}
	
	return 0;
}

int SonixUsbCameraDevice::video_requeue_buffer(int dev, int index)
{
	int ret;
	struct v4l2_buffer buf;
	
	memset(&buf, 0, sizeof(struct v4l2_buffer));
	buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
	buf.memory = V4L2_MEMORY_MMAP; 
	buf.index = index;

	ret = ioctl(dev, VIDIOC_QBUF, &buf);
	if (ret < 0) {
		LOGV("Unable to requeue buffer0 (%s).", strerror(errno));
		return ret;
	}
	
	return 0;
}

int SonixUsbCameraDevice::video_set_input(int dev, unsigned int input)
{
	__u32 _input = input;
	int ret;

	ret = ioctl(dev, VIDIOC_S_INPUT, &_input);
	if (ret < 0)
		LOGE("Unable to select input %u: %s.", input,
			strerror(errno));

	return ret;
}

int SonixUsbCameraDevice::video_try_fmt_size(int dev, 
	int * width, int * height, unsigned int format)
{
	int ret = -1;
	struct v4l2_format fmt;

	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
	fmt.fmt.pix.width  = *width; 
	fmt.fmt.pix.height = *height; 
	fmt.fmt.pix.pixelformat = format; 
	fmt.fmt.pix.field = V4L2_FIELD_NONE;

	ret = ioctl(dev, VIDIOC_TRY_FMT, &fmt); 
	if (ret < 0) 
	{ 
		LOGE("VIDIOC_TRY_FMT Failed: %s", strerror(errno)); 
		return ret; 
	} 

	// driver surpport this size
	*width = fmt.fmt.pix.width; 
	*height = fmt.fmt.pix.height; 
	LOGV("video_try_fmt_size: w: %d, h: %d", *width, *height);

	return 0;
}

int SonixUsbCameraDevice::waitH264FrameReady(void)
{
	fd_set fds;		
	struct timeval tv;
	int r;

	FD_ZERO(&fds);
	FD_SET(mH264Fd, &fds);		
	
	/* Timeout */
	tv.tv_sec  = 1;
	tv.tv_usec = 0;
	
	r = select(mH264Fd + 1, &fds, NULL, NULL, &tv);
	if (r == -1) 
	{
		LOGE("select err, %s", strerror(errno));
		return -1;
	} 
	else if (r == 0) 
	{
		LOGE("select timeout, mH264Fd = %d", mH264Fd);
		return -1;
	}

	return 0;
}


int SonixUsbCameraDevice::openH264CameraDevice(const char *dev_name, int width, int height)
{
	int ret;
	unsigned int nbufs = V4L_BUFFERS_DEFAULT;

	if (mH264Fd >= 0) {
		LOGE("h264 device has been opened."); 
		return -1;
	}
	
	/* Open the video device. */
	mH264Fd = video_open(dev_name);
	if (mH264Fd < 0)
	{
		LOGE("ERROR opening %s: %s", dev_name, strerror(errno)); 
		mH264Fd = -1;
		return -1; 
	}
	
	strcpy(mH264DevName, dev_name);
	
	ret = XU_Ctrl_ReadChipID(mH264Fd);
	if (ret < 0) {
		LOGE("SONiX_UVC_TestAP @main : XU_Ctrl_ReadChipID Failed");
		goto ERROR;
	}
	
	if (video_try_fmt_size(mH264Fd, &width, &height, V4L2_PIX_FMT_H264) < 0) {
		goto ERROR;
	}

	/* Set the video format. */
	if (video_set_format(mH264Fd, width, height, V4L2_PIX_FMT_H264) < 0) {
		goto ERROR;
	}

	/* Set the frame rate. */
	if (video_set_framerate(mH264Fd) < 0) {
		goto ERROR;
	}

	/* Allocate buffers. */
	if ((int)(nbufs = video_reqbufs(mH264Fd, nbufs)) < 0) {
		goto ERROR;
	}

	if (video_querybuf(mH264Fd, &mH264MemMap, nbufs) < 0) {
		goto ERROR;
	}

	mH264NumBufs = nbufs;

	/* Start streaming. */
	//video_enable(mH264Fd, 1);
	return 0;

ERROR:
	close(mH264Fd);
	mH264Fd = -1;
	return -1;
}


int SonixUsbCameraDevice::startRecording()
{
	int i;

	if (isUseH264DevRec()) {
		if(mH264Fd < 0)
			return -1;
		
		for(i = 0; i < mH264NumBufs; i++) {
			video_requeue_buffer(mH264Fd, i);
		}
		/* Start streaming. */
		video_enable(mH264Fd, 1);
	}

	return 0;
}

int SonixUsbCameraDevice::stopRecording()
{
	/* Stop streaming. */
	if (isUseH264DevRec()) {
		if(mH264Fd < 0)
			return -1;
		video_enable(mH264Fd, 0);
	}

	return 0;
}

void SonixUsbCameraDevice::useH264DevRec(bool isH264)
{
	if (isH264) {
		if(mH264Fd >= 0)
			mUseH264DevRec = true;
		else
			mUseH264DevRec = false;
	} else {
		mUseH264DevRec = false;
	}
}


bool SonixUsbCameraDevice::isUseH264DevRec(void)
{
	return mUseH264DevRec;
}

int SonixUsbCameraDevice::setVideoSize(int w, int h)
{
	int ret;
	
	if (isUseH264DevRec()) {
		if (mH264Fd < 0)
			return -1;
		if (video_try_fmt_size(mH264Fd, &w, &h, V4L2_PIX_FMT_H264) < 0) {
			return -1;
		}
		closeH264CameraDevice();
		ret = openH264CameraDevice(mH264DevName, w , h);
		if (ret < 0) {
			LOGE("setVideoSize : open h264 device failed.");
			useH264DevRec(false);
		}
	}
	return 0;
}

int SonixUsbCameraDevice::tryH264FmtSize(int w, int h)
{
	return video_try_fmt_size(mH264Fd, &w, &h, V4L2_PIX_FMT_H264);
	
}

int SonixUsbCameraDevice::setVideoBitRate(int32_t bitRate)
{
	int ret = 0;
	if (isUseH264DevRec()) {
		if (mH264Fd < 0)
			return -1;

		for(int i = 0; i < 3; i++) {		
			ret = XU_H264_Set_BitRate(mH264Fd, bitRate);
			if (ret == 0) {
				return 0;
			}
		}
	}
	
	LOGE("set H264 Video Bit rate failed.");
	return -1;

}

void* SonixUsbCameraDevice::readOneH264Frame(struct v4l2_buffer *v4l2_buf)
{
	int ret;

	if (mH264Fd < 0) {
		return NULL;
	}	
	
//	ret = waitH264FrameReady(void);
//	if (ret < 0) {
//		return NULL;
//	}

	ret = video_dequeue_buffer(mH264Fd, v4l2_buf);
	if (ret < 0) {
		LOGE("Unable to requeue buffer0 (%s).", strerror(errno));
		return NULL;
	}

	return mH264MemMap.mem[v4l2_buf->index];
}

int SonixUsbCameraDevice::releaseOneH264Frame(int index)
{
	int ret;
	
	if (mH264Fd < 0) {
		return -1;
	}
	
	ret = video_requeue_buffer(mH264Fd, index);
	return ret;
}


int SonixUsbCameraDevice::closeH264CameraDevice(void)
{
	LOGV("closeH264CameraDevice");
	
	if (mH264Fd < 0) {
		return 0;
	}
	
	/* Stop streaming. */
	video_enable(mH264Fd, 0);
	
	video_unmapBuf(&mH264MemMap, mH264NumBufs);
	
	close(mH264Fd);
	mH264Fd = -1;

	return 0;
}

int SonixUsbCameraDevice::updateInternalTime(int dev)
{
	struct tm *t;
	static time_t prevTime = 0;
	time_t curTime = 0;
	
	time(&curTime);
	if (curTime - prevTime < 3) {			//3s
		return 0;
	}
	prevTime = curTime;

	t = localtime(&curTime);
	
	if (XU_OSD_Set_RTC(dev, t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec) < 0) {
        return -1;
	}
	
	//LOGV("%4d-%02d-%02d %02d:%02d:%02d\n",t->tm_year+1900,t->tm_mon+1,
	//	t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec);
	
	return 0;
}

int SonixUsbCameraDevice::initCameraParam(int dev)
{

	if (v4l2ResetControl(dev, V4L2_CID_BRIGHTNESS) < 0) {
        return -1;
	}
  	if (v4l2ResetControl(dev, V4L2_CID_CONTRAST) < 0) {
        return -1;
  	}
  	if (v4l2ResetControl(dev, V4L2_CID_SATURATION) < 0) {
        return -1;
  	}
  	if (v4l2ResetControl(dev, V4L2_CID_GAIN) < 0) {
        return -1;
  	}

	//v4l2SetControl(dev, V4L2_CID_BRIGHTNESS, 0);
	//v4l2SetControl(dev, V4L2_CID_CONTRAST, 32);
	//v4l2SetControl(dev, V4L2_CID_SATURATION, 64);
	//v4l2SetControl(dev, V4L2_CID_GAMMA, 100);
	//v4l2SetControl(dev, V4L2_CID_DO_WHITE_BALANCE, 4600);	
	
	return 0;
}

int SonixUsbCameraDevice::isSonixUVCChip(int dev)
{
	return XU_Ctrl_ReadChipID(dev);
}



};
