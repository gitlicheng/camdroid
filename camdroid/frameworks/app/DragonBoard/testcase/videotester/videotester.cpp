#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include <HerbCamera.h>
#include <HerbMediaRecorder.h>
#include <CedarMediaPlayer.h>
#include <CedarDisplay.h>
#include <hwdisp_def.h>
#include <fb.h>

#define TAG "videotester"
#include <dragonboard.h>

using namespace android;

int chipType = 0;

void DetectChipType(void)
{
	db_msg("==============read cpuinfo==================\n");
	int cnt, fd;
	char buf[4096], cpuHardware[10];
	char *cpuHardwarePtr, *cpuInfoPtr;
	cpuHardwarePtr = cpuHardware;
	if ((fd = open("/proc/cpuinfo", O_RDONLY)) > 0) {
		cnt = read(fd, buf, 4096);
		if (cnt != -1) {
			cpuInfoPtr = strstr(buf, "sun");
			while ((*cpuHardwarePtr++ = *cpuInfoPtr++) != '\n');
			*--cpuHardwarePtr = '\0';
			if (strcmp(cpuHardware, "sun8i") == 0)		// V3
				chipType = 0;
			else if (strcmp(cpuHardware, "sun7i") == 0)	// A20
				chipType = 1;
			else {
				chipType = 2;				// unknown
				db_warn("UNKNOWN chip!\n");
			}
		}
		db_msg("Hardware: %s, chipType = %d\n", cpuHardware, chipType);
		close(fd);
	} else {
		chipType = -1;
		db_warn("can NOT open cpuinfo!\n");
	}
	db_msg("=============end of cpuinfo==================\n");

	return;
}

int main(int argc, char** argv)
{
	int nPreviewEnableFlag = 1;
	int nRecordEnableFlag = 0;
	int nTestSensorFlag = 1;
	int nTestUvcFlag = 1;
	int nEnableWaterMark = 0;
	bool nVideoOnly = true;
	int nSensorBitRate = 5*1024*1024;
	int nUvcBitRate = 5*1024*1024;

	unsigned int nRecordedTime = 60;

	int nBackSensorExistFlag = 0;

	sp<ProcessState> proc(ProcessState::self());
	ProcessState::self()->startThreadPool();

//	String8 outputFile("/mnt/extsd/sensor.mp4");
//	String8 uvcOutputFile("/mnt/extsd/uvc.mp4");
	HerbCamera* mHC = NULL;
	HerbCamera* mUvcHC = NULL;
	CedarDisplay* mCD = NULL;
	CedarDisplay* mUvcCD = NULL;
	HerbMediaRecorder* mHMR = NULL;
	HerbMediaRecorder* mUvcHMR = NULL;
	HerbCamera::Parameters HCParam;
	HerbCamera::Parameters uvcHCParam;
	int hlay = -1;
	int hlay2 = -1;
	
	int wantedFrameRate = 30;
	Size wantedPreviewSize(640, 360);
	Size wantedVideoSize(1920, 1080);

	int uvcFrameRate = 25;
	Size uvcPreviewSize(640, 360);
	Size uvcVideoSize(1920, 1080);
	Size uvcEncodeOutVideoSize = uvcVideoSize;

	int videoCodec = HerbMediaRecorder::VideoEncoder::H264;
	int nSourceChannel;

	if (nTestSensorFlag) {
		db_msg("before HerbCamera open");
		mHC = HerbCamera::open(0);
		db_msg("after HerbCamera open");
		mHC->getParameters(&HCParam);
		size_t i;
		Vector<Size> supportedPreviewSizes;
		Vector<Size> supportedVideoSizes;
		Vector<int>  supportedFrameRates;
		Size defaultPreviewSize(0, 0);
		Size defaultVideoSize(0, 0);
		int defaultFrameRate;
		HCParam.getSupportedPreviewSizes(supportedPreviewSizes);
		HCParam.getSupportedPreviewFrameRates(supportedFrameRates);
		HCParam.getSupportedVideoSizes(supportedVideoSizes);
		HCParam.getPreviewSize(defaultPreviewSize);
//		HCParam.getVideoSize(defaultVideoSize);
		defaultFrameRate = HCParam.getPreviewFrameRate();
		// preview
		for (i = 0; i < supportedPreviewSizes.size(); i++) {
			if (wantedPreviewSize.width == supportedPreviewSizes[i].width && wantedPreviewSize.height ==  supportedPreviewSizes[i].height) {
				db_verb("find previewSizes(%d)(%d, %d)\n", i, wantedPreviewSize.width, wantedPreviewSize.height);
				break;
			} else {
				db_debug("previewSizes(%d, %d) NOT support\n", wantedPreviewSize.width, wantedPreviewSize.height);
			}
		}
		if (i == supportedPreviewSizes.size()) {
			db_warn("wantedPreviewSize(%d, %d) NOT supported by camera! USE default previewSize(%d, %d)\n", wantedPreviewSize.width, wantedPreviewSize.height, defaultPreviewSize.width, defaultPreviewSize.height);
			wantedPreviewSize = defaultPreviewSize;
		}
		// record
/*
		for (i = 0; i < supportedVideoSizes.size(); i++) {
			if (wantedVideoSize.width == supportedVideoSizes[i].width && wantedVideoSize.height == wantedVideoSize.height) {
				db_verb("find videoSize(%d)(%d, %d)\n", i, wantedVideoSize.width, wantedVideoSize.height);
				break;
			} else {
				db_debug("videoSize(%d)(%d, %d) NOT support\n", i, wantedVideoSize.width, wantedVideoSize.height);
			}
		}
*/
		if (i == supportedVideoSizes.size()) {
			db_warn("wantedVideoSize(%d, %d) NOT supported by camera! USE default videoSize(%d, %d) of camera!\n", wantedVideoSize.width, wantedVideoSize.height, defaultVideoSize.width, defaultVideoSize.height);
			wantedVideoSize = defaultVideoSize;
		}
		// frameRate for preview and record
		for (i = 0; i < supportedFrameRates.size(); i++) {
			if (wantedFrameRate == supportedFrameRates[i]) {
				db_verb("find wantedFrameRate(%d)(%d)\n", i, wantedFrameRate);
				break;
			} else {
				db_debug("frameRate(%d)(%d) NOT support\n", i, wantedFrameRate);
			}
		}
		if (i == supportedFrameRates.size()) {
			db_warn("wantedFrameRate(%d) NOT support! USE default frameRate(%d) of camera\n", wantedFrameRate, defaultFrameRate);
			wantedFrameRate = defaultFrameRate;
		}
		HCParam.setVideoSize(wantedVideoSize.width, wantedVideoSize.height);
		HCParam.setPreviewSize(wantedPreviewSize.width, wantedPreviewSize.height);
		HCParam.setPreviewFrameRate(wantedFrameRate);
		mHC->setParameters(&HCParam);
/*
		if (nEnableWaterMark) {
			db_msg("enable water mark\n");
			mHC->setWaterMark(1, NULL);
		}
*/
	}
	if (nTestUvcFlag) {
		int fd = 0;								// A20 has 2 background video: UVC & TVIN
											// /dev/video0: CSI, this node always exists
											// /dev/video1: TVIN, this node always exists
											// /dev/video2: UVC, this node exists only when UVC is plugged in
		if (chipType == 0) {			// machine: V3
			if ((fd = open("/dev/video1", O_RDONLY)) > 0) {
				db_msg("open UVC(%d) of V3\n", fd);
				mUvcHC = HerbCamera::open(1);					// TODO: 1:Uvc(both V3 and A20 have this device); 2:TVIN(only A20 have)
				close(fd);
			} else {
				db_msg("UVC of V3 NOT FOUND!\n");
			}
		} else if (chipType == 1) { 		// machine: A20
			if ((fd = open("/dev/video2", O_RDONLY)) > 0) {
				db_msg("open UVC(%d) of A20\n", fd);
				mUvcHC = HerbCamera::open(1);					// UVC
				close(fd);
			} else if ((fd = open("/dev/video1", O_RDONLY)) > 0){
				db_msg("open TVIN(%d) of A20\n", fd);
				mUvcHC = HerbCamera::open(2);					// TVIN
				close(fd);
			} else {
				db_msg("Neither UVC nor TVIN of A20 was found!\n");
			}
		//	mUvcHC = HerbCamera::open(2);						// TODO: A20 has two kind of machine(one has Uvc, the other has TVIN)
		}										// here need other methods to distinguish them
		if (mUvcHC != NULL) {
			mUvcHC->getParameters(&uvcHCParam);
			uvcHCParam.dump();
			uvcHCParam.setPreviewSize(uvcPreviewSize.width, uvcPreviewSize.height);
			uvcHCParam.setVideoSize(uvcVideoSize.width, uvcVideoSize.height);
			uvcHCParam.setPreviewFrameRate(uvcFrameRate);
			mUvcHC->setParameters(&uvcHCParam);
		} else {
			nTestUvcFlag = 0;
		}
	}
//	if (nTestUvcFlag) {
//		mUvcHC = HerbCamera::open(1);
//		mUvcHC->getParameters(&uvcHCParam);
//		uvcHCParam.dump();
//		uvcHCParam.setPreviewSize(uvcPreviewSize.width, uvcPreviewSize.height);
//		uvcHCParam.setVideoSize(uvcVideoSize.width, uvcVideoSize.height);
//		uvcHCParam.setPreviewFrameRate(uvcFrameRate);
//		mUvcHC->setParameters(&uvcHCParam);
//	}

	if (nPreviewEnableFlag) {
		unsigned int fb_w;
		unsigned int fb_h;
		int ffb;
		struct fb_var_screeninfo var;
		if ((ffb = open("/dev/graphics/fb0", O_RDWR)) < 0) {
			db_warn("open fb0 failed\n");
			goto _EXIT;
		}
		ioctl(ffb, FBIOGET_VSCREENINFO, &var);
		fb_w = var.xres;
		fb_h = var.yres;
		struct view_info sur;
		struct view_info sur2;
		sur.x = 0;
		sur.y = 0;				// check out y position!!!
		sur.w = fb_w/2;
		sur.h = fb_h;				// check out sensor video's height!!!
		sur2.x = fb_w/2;
		sur2.y = 0;				// check out y position!!!
		sur2.w = fb_w/2;
		sur2.h = fb_h;				// check out uvc video's height!!!

		db_msg("prepare startPreview\n");
		if (nTestSensorFlag) {
			mCD = new CedarDisplay(0);
			hlay = mCD->requestSurface(&sur);
			mHC->setPreviewDisplay(hlay);
			mHC->startPreview();
		}
		if (nTestUvcFlag) {
			mUvcCD = new CedarDisplay(1);
			hlay2 = mUvcCD->requestSurface(&sur2);
			mUvcHC->setPreviewDisplay(hlay2);
			mUvcHC->startPreview();
		}
		db_msg("prepare startPreview finish\n");
		if (0 == nRecordEnableFlag) {
			db_msg("keep preview forever!\n");
			while (1) {
				sleep(1);
			}
			db_msg("HerbCamera stopPreview!\n");
			mHC->stopPreview();
			mCD->releaseSurface(hlay);
			if (nTestUvcFlag) {
				mUvcHC->stopPreview();
				mUvcCD->releaseSurface(hlay2);
			}
		}
	}

_EXIT:
	db_warn("delete HerbCamera and HerbMediaRecorder\n");
	if (mHC) {
		db_msg("[Sensor] HerbCamera::release()\n");
		mHC->release();
		delete mHC;
		mHC == NULL;
	}
	if (mUvcHC) {
		db_msg("[Uvc] HerbCamera::release()\n");
		mUvcHC->release();
		mUvcHC = NULL;
	}
	if (mHMR) {
		delete mHMR;
		mHMR = NULL;
	}
	if (mUvcHMR) {
		delete mUvcHMR;
		mUvcHMR = NULL;
	}
_exit:
	IPCThreadState::self()->joinThreadPool();

	return 0;
}
