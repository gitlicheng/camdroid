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

/*
class AppConfig {
public:
	int mPreviewEnableFlag;
	int mRecordEnableFlag;
	int mTestSensorFlag;
	int mTestUvcFlag;
	unsigned int mRecordedTime;
	int mEnableWaterMark;
	int mVideoOnly;
	int mSensorBitRate;
	int mUvcBitRate;
	AppConfig(): mPreviewEnableFlag(1), mRecordEnableFlag(1), mTestSensorFlag(1), mTestUvcFlag(1), mRecordedTime(60),
			mEnableWaterMark(0), mVideoOnly(1), mSensorBitRate(5*1024*1024), mUvcBitRate(5*1024*1024){} 

	void clearConfig() {
		mPreviewEnableFlag = 0;
		mRecordEnableFlag = 0;
		mTestSensorFlag = 0;
		mTestUvcFlag = 0;
		mRecordedTime = 0;
		mEnableWaterMark = 0;
		mVideoOnly = 0;
		mSensorBitRate = 0;
		mUvcBitRate = 0;
	}

	bool isConfigValid() {
		bool nValid = true;
		if ((mPreviewEnableFlag || mRecordEnableFlag) && (mTestSensorFlag || mTestUvcFlag)) {
			if (mRecordEnableFlag) {
				if (mRecordedTime >= 5) {
					nValid = true;
				} else {
					db_warn("mRecordedTime(%d) too small, must over 5s!\n", mRecordedTime);
					return false;
				}
				if (mTestSensorFlag) {
					if (mSensorBitRate > 0)
						nValid = true;
					else
						return false;
				}
				if (mTestUvcFlag) {
					if (mUvcBitRate > 0)
						nValid = true;
					else
						return false;
				}
			}
			return true;
		} else {
			db_warn("video parameter wrong, check out it!\n");
			db_debug("[Parameter] mPreviewEnableFlag:[%d],\tmRecordEnableFlag:[%d],\tmTestSensorFlag:[%d],\tmTestUvcFlag:[%d]\n", 
				mPreviewEnableFlag, mRecordEnableFlag, mTestSensorFlag, mTestUvcFlag);
			return false;
		}
	}
};
*/

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

	String8 outputFile("/mnt/extsd/sensor.mp4");
	String8 uvcOutputFile("/mnt/extsd/uvc.mp4");
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
		HCParam.getVideoSize(defaultVideoSize);
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
		mUvcHC = HerbCamera::open(1);
		mUvcHC->getParameters(&uvcHCParam);
		uvcHCParam.dump();
		uvcHCParam.setPreviewSize(uvcPreviewSize.width, uvcPreviewSize.height);
		uvcHCParam.setVideoSize(uvcVideoSize.width, uvcVideoSize.height);
		uvcHCParam.setPreviewFrameRate(uvcFrameRate);
		mUvcHC->setParameters(&uvcHCParam);
	}

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
		sur.h = fb_h;			// check out sensor video's height!!!
		sur2.x = fb_w/2;
		sur2.y = 0;				// check out y position!!!
		sur2.w = fb_w/2;
		sur2.h = fb_h;			// check out uvc video's height!!!

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
/*
		if (0 == nRecordEnableFlag) {
			db_msg("keep preview forever!\n");
			while (1) {
				sleep(5);
			}
			db_msg("HerbCamera stopPreview!\n");
			mHC->stopPreview();
			mCD->releaseSurface(hlay);
			if (nTestUvcFlag) {
				mUvcHC->stopPreview();
				mUvcCD->releaseSurface(hlay2);
			}
		}
*/
	}
/*
	if (nRecordEnableFlag) {
		db_msg("HerbCamera::unlock()\n");
		if (nTestSensorFlag) {
			mHC->unlock();
			mHMR = new HerbMediaRecorder();
			db_verb("[Sensor] HerbCamera::setCamera()\n");
			mHMR->setCamera(mHC);
			if (false == nVideoOnly) {
				db_verb("hHMR::setAudioSource=[MIC]\n");
				mHMR->setAudioSource(HerbMediaRecorder::AudioSource::MIC);
			}
			db_verb("[Sensor] HerbCamera::setVideoSource,setOutputFormat,setOutputFile...\n");
			mHMR->setVideoSource(HerbMediaRecorder::VideoSource::CAMERA);
			mHMR->setOutputFormat(HerbMediaRecorder::OutputFormat::MPEG_4);
			mHMR->setOutputFile((char*)outputFile.string());
			mHMR->setVideoEncodingBitRate(nSensorBitRate);
			mHMR->setVideoFrameRate(wantedFrameRate);
			mHMR->setVideoSize(wantedVideoSize.width, wantedVideoSize.height);
			mHMR->setVideoEncoder(videoCodec);
			if (false == nVideoOnly) {
				db_verb("[Sensor] mHMR::setAudioEncoder=[ACC]\n");
                		mHMR->setAudioEncoder(HerbMediaRecorder::AudioEncoder::AAC);
			}
			nSourceChannel = 0;
			mHMR->setSourceChannel(nSourceChannel);
			mHMR->prepare();
			mHMR->start();
			db_msg("[Sensor] recordint...\n");
		}
		if (nTestUvcFlag) {
			mUvcHC->unlock();
			mUvcHMR = new HerbMediaRecorder();
			db_verb("[Uvc] HerbCamera::setCamera()\n");
			mUvcHMR->setCamera(mUvcHC);
			if (false == nVideoOnly) {
				db_verb("[Uvc] mUvcHMR::setAudioSource=[MIC]\n");
				mUvcHMR->setAudioSource(HerbMediaRecorder::AudioSource::MIC);
			}
			db_verb("[Uvc] HerbCamera::setVideoSource,setOutputFormat,setOutputFile...\n");
			mUvcHMR->setVideoSource(HerbMediaRecorder::VideoSource::CAMERA);
			mUvcHMR->setOutputFormat(HerbMediaRecorder::OutputFormat::MPEG_4);
			mUvcHMR->setOutputFile((char*)uvcOutputFile.string());
			mUvcHMR->setVideoEncodingBitRate(nUvcBitRate);
			mUvcHMR->setVideoFrameRate(uvcFrameRate);
			mUvcHMR->setVideoSize(uvcEncodeOutVideoSize.width, uvcEncodeOutVideoSize.height);
			mUvcHMR->setVideoEncoder(videoCodec);
			if (false == nVideoOnly) {
				db_verb("[Uvc] mUvcHMR::msetAudioEncoder=[ACC]\n");
                		mUvcHMR->setAudioEncoder(HerbMediaRecorder::AudioEncoder::AAC);
			}
			nSourceChannel = 0;
			mUvcHMR->setSourceChannel(nSourceChannel);
			mUvcHMR->prepare();
			mUvcHMR->start();
			db_msg("[Uvc] recording...\n");
		}
		db_msg("will record [%d]s\n", nRecordedTime);
		sleep(nRecordedTime);
		db_msg("record done!\n");
		if (nTestSensorFlag) {
			mHMR->stop();
			mHMR->release();
			mHC->lock();
		}
	}
*/

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
