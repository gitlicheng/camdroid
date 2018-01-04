/*
 * Copyright (C) 2007 The Android Open Source Project
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
#define LOG_TAG "HerbMediaRecorder"
#include <utils/Log.h>

//#include <gui/Surface.h>
#include <camera/ICameraService.h>
#include <camera/Camera.h>
#include <media/mediarecorder.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <utils/threads.h>

//#include "jni.h"
//#include "JNIHelp.h"
//#include "android_runtime/AndroidRuntime.h"

#include <system/audio.h>

#include <HerbCamera.h>
#include <HerbMediaRecorder.h>
namespace android {

// helper function to extract a native Camera object from a Camera Java object
//static Mutex sLock;
Mutex HerbMediaRecorder::msLock;
const char HerbMediaRecorder::FatFsPrefixString[] = "0:/";    //"/mnt/extsd/video/"
HerbMediaRecorder::EventHandler::EventHandler(HerbMediaRecorder *pC)
{
	mpMediaRecorder = pC;
}

HerbMediaRecorder::EventHandler::~EventHandler()
{
}

void HerbMediaRecorder::EventHandler::handleMessage(const MediaCallbackMessage &msg)
{
	if (mpMediaRecorder->mNativeContext == 0) {
		ALOGW("mediarecorder went away with unhandled events");
		return;
	}
	switch (msg.what) {
		case MEDIA_RECORDER_EVENT_ERROR:
		case MEDIA_RECORDER_TRACK_EVENT_ERROR:
			if (mpMediaRecorder->mOnErrorListener != NULL) {
				mpMediaRecorder->mOnErrorListener->onError(mpMediaRecorder, msg.arg1, msg.arg2);
			}
			return;
		case MEDIA_RECORDER_EVENT_INFO:
		case MEDIA_RECORDER_TRACK_EVENT_INFO:
			if (mpMediaRecorder->mOnInfoListener != NULL) {
				mpMediaRecorder->mOnInfoListener->onInfo(mpMediaRecorder, msg.arg1, msg.arg2);
			}
			return;
		case MEDIA_RECORDER_VENDOR_EVENT_BSFRAME_AVAILABLE:
			if (mpMediaRecorder->mOnDataListener != NULL) {
				mpMediaRecorder->mOnDataListener->onData(mpMediaRecorder, msg.arg1, msg.arg2);
			}
			return;
		default:
			ALOGE("Unknown message type %d", msg.what);
			return;
	}
}



HerbMediaRecorder::HerbMediaRecorderListener::HerbMediaRecorderListener(HerbMediaRecorder *pMr)
{
	mpMediaRecorder= pMr;
}

HerbMediaRecorder::HerbMediaRecorderListener::~HerbMediaRecorderListener()
{
}

void HerbMediaRecorder::HerbMediaRecorderListener::notify(int msg, int ext1, int ext2)
{
    mpMediaRecorder->postEventFromNative(mpMediaRecorder, msg, ext1, ext2);
}

sp<MediaRecorder> HerbMediaRecorder::getMediaRecorder()
{
    Mutex::Autolock _l(msLock);
    
    MediaRecorder* const p = reinterpret_cast<MediaRecorder*>(mNativeContext);
	return sp<MediaRecorder>(p);
}

sp<MediaRecorder> HerbMediaRecorder::setMediaRecorder(const sp<MediaRecorder>& recorder)
{
    Mutex::Autolock _l(msLock);
    sp<MediaRecorder> old = reinterpret_cast<MediaRecorder*>(mNativeContext);
    if (recorder.get()) {
        recorder->incStrong(this);
    }
    if (old != 0) {
        old->decStrong(this);
    }
    mNativeContext = (int)recorder.get();
    return old;
}
HerbMediaRecorder::HerbMediaRecorder() :
	mOnErrorListener(NULL),
	mOnInfoListener(NULL),
	mOnDataListener(NULL),
	mNativeContext(0),
	mLength(0)
{
	ALOGV("Constructor");
    mFd = -1;
	sp<MediaRecorder> mr = new MediaRecorder();
	if (mr == NULL) {
		ALOGE("Failed to new MediaRecorder!");
		return;
	}
	if (mr->initCheck() != NO_ERROR) {
		ALOGE("Unable to initialize media recorder");
		return;
	}

	mEventHandler = new EventHandler(this);
	sp<HerbMediaRecorderListener> listener = new HerbMediaRecorderListener(this);
	mr->setListener(listener);
	setMediaRecorder(mr);
}

HerbMediaRecorder::~HerbMediaRecorder()
{
	ALOGV("destructor");
    if(mFd>=0)
    {
        ::close(mFd);
        mFd = -1;
    }
}

bool HerbMediaRecorder::process_media_recorder_call(status_t opStatus, const char* message)
{
   ALOGV("process_media_recorder_call");
   if (opStatus == (status_t)INVALID_OPERATION) {
       ALOGE("INVALID_OPERATION");
       return true;
   } else if (opStatus != (status_t)OK) {
       ALOGE("%s", message);
       return true;
   }
   return false;
}

status_t HerbMediaRecorder::setCamera(HerbCamera *pC)
{
    ALOGV("setCamera");
	if (pC == NULL) {
		ALOGE("camera object is a NULL pointer");
		return BAD_VALUE;
	}
    sp<Camera> c = HerbCamera::get_native_camera(pC, NULL);
	if (c == NULL) {
		return NO_INIT;
	}
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}
    status_t opStatus = mr->setCamera(c->remote(), c->getRecordingProxy());
    process_media_recorder_call(opStatus, "setCamera failed.");
    return opStatus;
}

void HerbMediaRecorder::setPreviewDisplay(int hlay)
{
    ALOGV("setPreviewDisplay");
	mSurface = hlay;
}

status_t HerbMediaRecorder::setAudioSource(int audio_source)
{
    ALOGV("setAudioSource");
	if (audio_source < AUDIO_SOURCE_DEFAULT || audio_source >= AUDIO_SOURCE_CNT) {
		ALOGE("Invalid audio source %d", audio_source);
		return BAD_VALUE;
	}
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}
    status_t opStatus = mr->setAudioSource(audio_source);
    process_media_recorder_call(opStatus, "setAudioSource failed.");
    return opStatus;
}

status_t HerbMediaRecorder::setVideoSource(int video_source)
{
    ALOGV("setVideoSource");
	if (video_source < VIDEO_SOURCE_DEFAULT || video_source >= VIDEO_SOURCE_LIST_END) {
		ALOGE("Invalid video source");
		return BAD_VALUE;
	}
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}
    status_t opStatus = mr->setVideoSource(video_source);
    process_media_recorder_call(opStatus, "setVideoSource failed.");
    return opStatus;
}

void HerbMediaRecorder::setProfile(HerbCamcorderProfile& profile)
{
    ALOGV("setProfile");
	setOutputFormat(profile.fileFormat);
	setVideoFrameRate(profile.videoFrameRate);
	setVideoSize(profile.videoFrameWidth, profile.videoFrameHeight);
	setVideoEncodingBitRate(profile.videoBitRate);
	setVideoEncoder(profile.videoCodec);
	if (profile.quality >= HerbCamcorderProfile::QUALITY_TIME_LAPSE_LOW &&
		profile.quality <= HerbCamcorderProfile::QUALITY_TIME_LAPSE_QVGA) {
		// Nothing needs to be done. Call to setCaptureRate() enables
		// time lapse video recording.
	} else {
		setAudioEncodingBitRate(profile.audioBitRate);
		setAudioChannels(profile.audioChannels);
		setAudioSamplingRate(profile.audioSampleRate);
		setAudioEncoder(profile.audioCodec);
	}
}

status_t HerbMediaRecorder::setCaptureRate(double fps)
{
    ALOGV("setCaptureRate(%f)", fps);
	// Make sure that time lapse is enabled when this method is called.
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}
    status_t ret;
    if (fps < 0.00001) {
        ret = mr->setParameters(String8("time-lapse-enable=0"));
        if (ret != NO_ERROR) {
            ALOGE("Failed to set time-lapse-enable=0");
            return ret;
        }
        return NO_ERROR;
    }
	ret = mr->setParameters(String8("time-lapse-enable=1"));
	if (ret != NO_ERROR) {
		ALOGE("Failed to set time-lapse-enable=1");
		return ret;
	}

	double timeBetweenFrameCapture = 1 / fps;
	int64_t timeBetweenFrameCaptureMs = (int64_t) (1000 * timeBetweenFrameCapture);
	char buffer[128];
	snprintf(buffer, 128, "time-between-time-lapse-frame-capture=%lld", timeBetweenFrameCaptureMs);
    status_t opStatus = mr->setParameters(String8(buffer));
    process_media_recorder_call(opStatus, "setCaptureRate failed.");
    return opStatus;
}

status_t HerbMediaRecorder::setOrientationHint(int degrees)
{
    ALOGV("setOrientationHint");
	if (degrees != 0 && degrees != 90 && degrees != 180 && degrees != 270) {
		ALOGE("Unsupported angle: %d", degrees);
		return BAD_VALUE;
	}
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}
	char buffer[64];
	snprintf(buffer, 64, "video-param-rotation-angle-degrees=%d", degrees);
    status_t opStatus = mr->setParameters(String8(buffer));
    process_media_recorder_call(opStatus, "setOrientationHint failed.");
    return opStatus;
}

status_t HerbMediaRecorder::setLocation(float latitude, float longitude)
{
    ALOGV("setLocation");
	int latitudex10000  = (int) (latitude * 10000 + 0.5);
	int longitudex10000 = (int) (longitude * 10000 + 0.5);

	if (latitudex10000 > 900000 || latitudex10000 < -900000) {
		ALOGE("Latitude: %f out of range.", latitude);
		return BAD_VALUE;
	}
	if (longitudex10000 > 1800000 || longitudex10000 < -1800000) {
		ALOGE("Longitude: %f out of range.", longitude);
		return BAD_VALUE;
	}

	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}
	char buffer[64];
	status_t ret;
	snprintf(buffer, 64, "param-geotag-latitude=%d", latitudex10000);
	ret = mr->setParameters(String8(buffer));
	if (ret != NO_ERROR) {
		ALOGE("Failed to set param-geotag-latitude");
		return ret;
	}
	snprintf(buffer, 64, "param-geotag-longitude=%d", longitudex10000);
    status_t opStatus = mr->setParameters(String8(buffer));
    process_media_recorder_call(opStatus, "setLocation failed.");
    return opStatus;
}

status_t HerbMediaRecorder::setOutputFormat(int output_format)
{
    ALOGV("setOutputFormat");
	if (output_format < OUTPUT_FORMAT_DEFAULT || output_format >= OUTPUT_FORMAT_LIST_END) {
		ALOGE("Invalid output format");
		return BAD_VALUE;
	}
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}
    status_t opStatus = mr->setOutputFormat(output_format);
    process_media_recorder_call(opStatus, "setOutputFormat failed.");
    return opStatus;
}

status_t HerbMediaRecorder::setVideoSize(int width, int height)
{
    ALOGV("setVideoSize");
	if (width <= 0 || height <= 0) {
		ALOGE("invalid video size");
		return BAD_VALUE;
	}

	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}
    status_t opStatus = mr->setVideoSize(width, height);
    process_media_recorder_call(opStatus, "setVideoSize failed.");
    return opStatus;
}

status_t HerbMediaRecorder::setVideoFrameRate(int rate)
{
    ALOGV("setVideoFrameRate");
	if (rate <= 0) {
		ALOGE("invalid frame rate");
		return BAD_VALUE;
	}

	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}
    status_t opStatus = mr->setVideoFrameRate(rate);
    process_media_recorder_call(opStatus, "setVideoFrameRate failed.");
    return opStatus;
}

status_t HerbMediaRecorder::setMuteMode(bool mute)
{
    ALOGV("setMuteMode");
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}
    status_t opStatus = mr->setMuteMode(mute);
    process_media_recorder_call(opStatus, "setMuteMode failed.");
    return opStatus;
}


status_t HerbMediaRecorder::setMaxDuration(int max_duration_ms)
{
    ALOGV("setMaxDuration");
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}

#if 0
	char buffer[64];
	snprintf(buffer, 64, "max-duration=%d", max_duration_ms);
    status_t opStatus = mr->setParameters(String8(buffer));
#else
	status_t opStatus = mr->setMaxDuration(max_duration_ms);
#endif
    process_media_recorder_call(opStatus, "setMaxDuration failed.");
    return opStatus;
}

status_t HerbMediaRecorder::setMaxFileSize(int64_t max_filesize_bytes)
{
    ALOGV("setMaxFileSize");
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}

#if 0
	char buffer[64];
	snprintf(buffer, 64, "max-filesize=%lld", (int64_t)max_filesize_bytes);
    status_t opStatus = mr->setParameters(String8(buffer));
#else
	status_t opStatus = mr->setMaxFileSize(max_filesize_bytes);
#endif
    process_media_recorder_call(opStatus, "setMaxFileSize failed.");
    return opStatus;
}

status_t HerbMediaRecorder::setAudioEncoder(int audio_encoder)
{
    ALOGV("setAudioEncoder");
	if (audio_encoder < AUDIO_ENCODER_DEFAULT || audio_encoder >= AUDIO_ENCODER_LIST_END) {
		ALOGE("Invalid audio encoder");
		return BAD_VALUE;
	}

	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}
    status_t opStatus = mr->setAudioEncoder(audio_encoder);
    process_media_recorder_call(opStatus, "setAudioEncoder failed.");
    return opStatus;
}

status_t HerbMediaRecorder::setVideoEncoder(int video_encoder)
{
    ALOGV("setVideoEncoder");
	if (video_encoder < VIDEO_ENCODER_DEFAULT || video_encoder >= VIDEO_ENCODER_LIST_END) {
		ALOGE("Invalid video encoder");
		return BAD_VALUE;
	}

	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}
    status_t opStatus = mr->setVideoEncoder(video_encoder);
    process_media_recorder_call(opStatus, "setVideoEncoder failed.");
    return opStatus;
}

status_t HerbMediaRecorder::setAudioSamplingRate(int samplingRate)
{
    ALOGV("setAudioSamplingRate");
	if (samplingRate <= 0) {
		ALOGE("Audio sampling rate is not positive");
		return BAD_VALUE;
	}

	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}
	char buffer[64];
	snprintf(buffer, 64, "audio-param-sampling-rate=%d", samplingRate);
    status_t opStatus = mr->setParameters(String8(buffer));
    process_media_recorder_call(opStatus, "setAudioSamplingRate failed.");
    return opStatus;
}

status_t HerbMediaRecorder::setAudioChannels(int numChannels)
{
    ALOGV("setAudioChannels");
	if (numChannels <= 0) {
		ALOGE("Number of channels is not positive");
		return BAD_VALUE;
	}
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}

	char buffer[64];
	snprintf(buffer, 64, "audio-param-number-of-channels=%d", numChannels);
    status_t opStatus = mr->setParameters(String8(buffer));
    process_media_recorder_call(opStatus, "setAudioChannels failed.");
    return opStatus;
}

status_t HerbMediaRecorder::setAudioEncodingBitRate(int bitRate)
{
    ALOGV("setAudioEncodingBitRate");
	if (bitRate <= 0) {
		ALOGE("Audio encoding bit rate is not positive");
		return BAD_VALUE;
	}
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}

	char buffer[64];
	snprintf(buffer, 64, "audio-param-encoding-bitrate=%d", bitRate);
    status_t opStatus = mr->setParameters(String8(buffer));
    process_media_recorder_call(opStatus, "setAudioEncodingBitRate failed.");
    return opStatus;
}

status_t HerbMediaRecorder::setVideoEncodingBitRate(int bitRate)
{
    ALOGV("setVideoEncodingBitRate");
	if (bitRate <= 0) {
		ALOGE("Video encoding bit rate is not positive");
		return BAD_VALUE;
	}
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}

	char buffer[64];
	snprintf(buffer, 64, "video-param-encoding-bitrate=%d", bitRate);
    status_t opStatus = mr->setParameters(String8(buffer));
    process_media_recorder_call(opStatus, "setVideoEncodingBitRate failed.");
    return opStatus;
}

status_t HerbMediaRecorder::setVideoEncodingBitRateSync(int bitRate)
{
    ALOGV("setVideoEncodingBitRateSync(%d)", bitRate);
	if (bitRate <= 0) {
		ALOGE("Video encoding bit rate is not positive");
		return BAD_VALUE;
	}
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}
    status_t opStatus = mr->setVideoEncodingBitRateSync(bitRate);
    process_media_recorder_call(opStatus, "setVideoEncodingBitRateSync failed.");
    return opStatus;
}

status_t HerbMediaRecorder::setVideoFrameRateSync(int frames_per_second)
{
    ALOGV("setVideoFrameRateSync(%d)", frames_per_second);
	if (frames_per_second <= 0) {
		ALOGE("Video encoding bit rate is not positive");
		return BAD_VALUE;
	}
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}
    status_t opStatus = mr->setVideoFrameRateSync(frames_per_second);
    process_media_recorder_call(opStatus, "setVideoFrameRateSync failed.");
    return opStatus;
}

void HerbMediaRecorder::setOutputFile(int fd)
{
    ALOGV("setOutputFile fd=%d", fd);
    if(mFd >= 0)
    {
        ALOGD("(f:%s, l:%d) Be careful! mFd[%d] will close", __FUNCTION__, __LINE__, mFd);
        ::close(mFd);
        mFd = -1;
    }
    if(fd >= 0)
    {
	    mFd = dup(fd);
    }
	mPath.clear();
	mLength = 0;
}

void HerbMediaRecorder::setOutputFile(int fd, int64_t length)
{
    ALOGV("setOutputFile fd=%d", fd);
    if(mFd >= 0)
    {
        ALOGD("(f:%s, l:%d) Be careful! mFd[%d] will close", __FUNCTION__, __LINE__, mFd);
        ::close(mFd);
        mFd = -1;
    }
    if(fd >= 0)
    {
	    mFd = dup(fd);
    }
	mPath.clear();
	mLength = length;
}

void HerbMediaRecorder::setOutputFile(char* path)
{
    ALOGV("setOutputFile path=%s", path);
    if(mFd >= 0)
    {
        ALOGD("(f:%s, l:%d) Be careful! mFd[%d] will close", __FUNCTION__, __LINE__, mFd);
        ::close(mFd);
        mFd = -1;
    }
    if(path)
    {
        mPath = path;
    }
    else
    {
        mPath.clear();
    }
	mLength = 0;
}

void HerbMediaRecorder::setOutputFile(char* path, int64_t length)
{
    ALOGV("setOutputFile path=%s, length[%lld]", path, length);
    if(path)
    {
        mPath = path;
        mLength = length;
    }
    else
    {
        mPath.clear();
        mLength = 0;
    }
}

status_t HerbMediaRecorder::setOutputFileSync(int fd, int64_t fallocateLength, int muxerId)
{
    ALOGV("setOutputFileSync fd=%d", fd);
	if (fd < 0) {
		ALOGE("Invalid parameter");
		return BAD_VALUE;
	}
    sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}
    status_t opStatus = mr->setOutputFileSync(fd, fallocateLength, muxerId);
    process_media_recorder_call(opStatus, "setOutputFileSync failed.");
    return opStatus;
}

status_t HerbMediaRecorder::setOutputFileSync(char* path, int64_t fallocateLength, int muxerId)
{
    ALOGV("setOutputFileSync path=%s", path);
    status_t ret;
    sp<MediaRecorder> mr = getMediaRecorder();
    if(path != NULL)
    {
        if (strncasecmp(path, "http://", 7) != 0 && strncasecmp(path, FatFsPrefixString, strlen(FatFsPrefixString)) != 0)
        {
            int fd = open(path, O_RDWR | O_CREAT, 0666);
            if (fd < 0) 
            {
                ALOGE("Failed to open %s(%s)", path, strerror(errno));
                return UNKNOWN_ERROR;
            }
            //mPath = path;
            ret = mr->setOutputFileSync(fd, fallocateLength, muxerId);
            ::close(fd);
            return ret;
        }
    }
    return mr->setOutputFileSync(path, fallocateLength, muxerId);
}

status_t HerbMediaRecorder::setOutputFileFd(int fd, long offset, long length)
{
    ALOGV("setOutputFileFd");
	if (fd < 0) {
		ALOGE("Invalid parameter");
		return BAD_VALUE;
	}
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}
    status_t opStatus = mr->setOutputFile(fd, offset, length);
    process_media_recorder_call(opStatus, "setOutputFile failed.");
    return opStatus;
}


status_t HerbMediaRecorder::prepare()
{
    ALOGV("prepare");
	sp<MediaRecorder> mr = getMediaRecorder();
	status_t ret;
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}
    int fd = -1;
	if (mPath.length() > 0) 
    {
		if (strncasecmp(mPath.string(), "http://", 7) == 0 
            || strncasecmp(mPath.string(), FatFsPrefixString, strlen(FatFsPrefixString)) == 0) 
        {
			ret = mr->setOutputFile(mPath.string(), 0, mLength);
            process_media_recorder_call(ret, "setOutputFile path failed");
			if (ret != NO_ERROR) 
            {
				return ret;
			}
		} 
        else 
        {
			fd = open(mPath.string(), O_RDWR | O_CREAT, 0666);
			if (fd < 0) 
            {
				ALOGE("Failed to open %s", mPath.string());
				return UNKNOWN_ERROR;
			}
			ret = setOutputFileFd(fd, 0, mLength);
			close(fd);
            fd = -1;
			if (ret != NO_ERROR) 
            {
				return ret;
			}
		}
	}
    else if (mFd >= 0) 
    {
		ret = setOutputFileFd(mFd, 0, mLength);
        ::close(mFd);
        mFd = -1;
		if (ret != NO_ERROR) 
        {
			return ret;
		}
	} 
    else 
    {
		//ALOGE("No valid output file");
		//return INVALID_OPERATION;
		ALOGD("(f:%s, l:%d) maybe use extend method", __FUNCTION__, __LINE__);
	}
	// ??????????????????????????????????Surface
    status_t opStatus = mr->prepare();
    process_media_recorder_call(opStatus, "prepare failed.");
    return opStatus;
}

status_t HerbMediaRecorder::start()
{
    ALOGV("start");
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}
    status_t opStatus = mr->start();
    process_media_recorder_call(opStatus, "start failed.");
    return opStatus;
}

status_t HerbMediaRecorder::stop()
{
    ALOGD("HerbMediaRecorder stop");
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}
    status_t opStatus = mr->stop();
    process_media_recorder_call(opStatus, "stop failed.");
    return opStatus;
}

status_t HerbMediaRecorder::reset()
{
    ALOGV("reset");
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}
    status_t opStatus = mr->reset();
    process_media_recorder_call(opStatus, "native_reset failed.");
    return opStatus;
}

int HerbMediaRecorder::getMaxAmplitude()
{
    ALOGV("getMaxAmplitude");
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
        return -1;
	}
    int result = 0;
    process_media_recorder_call(mr->getMaxAmplitude(&result), "getMaxAmplitude failed.");
    return result;
}

void HerbMediaRecorder::setOnErrorListener(OnErrorListener *pl)
{
	mOnErrorListener = pl;
}

void HerbMediaRecorder::setOnInfoListener(OnInfoListener *pListener)
{
	mOnInfoListener = pListener;
}

/* for recorder data callback */
void HerbMediaRecorder::setOnDataListener(OnDataListener *pl)
{
	mOnDataListener = pl;
}

sp<VEncBuffer> HerbMediaRecorder::getOneBsFrame(int mode, sp<IMemory> *frame)
{
	//ALOGV("getOneBsFrame");
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NULL;
	}
	sp<IMemory> mem = mr->getOneBsFrame(mode);
	if (mem == NULL) {
		//ALOGV("getOneBsFrame get buffer NULL!");
		return NULL;
	}
	sp<VEncBuffer> recData = new VEncBuffer;
	char *ptr_dst = (char *)mem->pointer();
	memcpy(&recData->total_size, ptr_dst, sizeof(int));
	ptr_dst += sizeof(int);
	memcpy(&recData->stream_type, ptr_dst, sizeof(int));
	ptr_dst += sizeof(int);
	if (recData->stream_type != 0) {
        freeOneBsFrame(recData, mem);
		return NULL;
	}
	memcpy(&recData->data_size, ptr_dst, sizeof(int));
	ptr_dst += sizeof(int);
	memcpy(&recData->pts, ptr_dst, sizeof(long long));
	ptr_dst += sizeof(long long);
    memcpy(&recData->CurrQp, ptr_dst, sizeof(int));
    ptr_dst += sizeof(int);
    memcpy(&recData->avQp, ptr_dst, sizeof(int));
    ptr_dst += sizeof(int);
    memcpy(&recData->nGopIndex, ptr_dst, sizeof(int));
    ptr_dst += sizeof(int);
    memcpy(&recData->nFrameIndex, ptr_dst, sizeof(int));
    ptr_dst += sizeof(int);
    memcpy(&recData->nTotalIndex, ptr_dst, sizeof(int));
    ptr_dst += sizeof(int);
	recData->data = ptr_dst;
	*frame = mem;
	//ALOGV("getOneBsFrame: total_size=%d, stream_type=%d, data_size=%d, pts=%lld, data=%p\n",
	//	recData->total_size, recData->stream_type, recData->data_size, recData->pts, recData->data);
	return recData;
}

void HerbMediaRecorder::freeOneBsFrame(sp<VEncBuffer> recData, sp<IMemory> frame)
{
	//ALOGV("freeOneBsFrame");
	//delete recData;
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return;
	}
	mr->freeOneBsFrame();
}

sp<IMemory> HerbMediaRecorder::getEncDataHeader()
{
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NULL;
	}
	return mr->getEncDataHeader();
}

//void HerbCamera::postEventFromNative(wp<HerbCamera>& camera_ref, int what, int arg1, int arg2, void *obj)
void HerbMediaRecorder::postEventFromNative(HerbMediaRecorder *pMr, int what, int arg1, int arg2, void *obj)
{
//    sp<HerbCamera> pC = camera_ref.promote();
    if (pMr == NULL)
    {
        ALOGE("(f:%s, l:%d) pMr == NULL", __FUNCTION__, __LINE__);
        return;
    }

    if (pMr->mEventHandler != NULL) {
        MediaCallbackMessage msg;
        msg.what = what;
        msg.arg1 = arg1;
        msg.arg2 = arg2;
        msg.obj = obj;
        pMr->mEventHandler->post(msg);
    }
}
void HerbMediaRecorder::release()
{
    ALOGV("release");
	sp<MediaRecorder> mr = setMediaRecorder(0);
	if (mr != NULL) {
		mr->setListener(NULL);
		mr->release();
	}
}

status_t HerbMediaRecorder::setVideoEncodingIFramesNumberInterval(int nMaxKeyItl)
{
    ALOGV("setVideoEncodingIFramesNumberInterval");
	if (nMaxKeyItl <= 0) {
		ALOGE("Video encoding i frames number interval is not positive");
		return BAD_VALUE;
	}
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}

	char buffer[64];
	snprintf(buffer, 64, "video-param-i-frames-number-interval=%d", nMaxKeyItl);
    status_t opStatus = mr->setParameters(String8(buffer));
    process_media_recorder_call(opStatus, "setVideoEncodingIFramesNumberInterval failed.");
    return opStatus;
}

status_t HerbMediaRecorder::setVideoEncodingIFramesNumberIntervalSync(int nMaxKeyItl)
{
    ALOGV("setVideoEncodingIFramesNumberIntervalSync(%d)", nMaxKeyItl);
    if (nMaxKeyItl <= 0) {
        ALOGE("Video encoding bit rate is not positive");
        return BAD_VALUE;
    }

	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}
    status_t opStatus = mr->setVideoEncodingIFramesNumberIntervalSync(nMaxKeyItl);
    process_media_recorder_call(opStatus, "setVideoEncodingIFramesNumberIntervalSync failed.");
    return opStatus;
}

status_t HerbMediaRecorder::reencodeIFrame()
{
    ALOGV("reencodeIFrame");

	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}
    status_t opStatus = mr->reencodeIFrame();
    process_media_recorder_call(opStatus, "reencodeIFrame failed.");
	return opStatus;
}

status_t HerbMediaRecorder::setSdcardState(bool bExist)
{
    ALOGV("setSdcardState(%d)", bExist);

    sp<MediaRecorder> mr = getMediaRecorder();
    if (mr == NULL) {
        ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
        return NO_INIT;
    }
    status_t opStatus = mr->setSdcardState(bExist);
    process_media_recorder_call(opStatus, "setSdcardState failed.");
    return opStatus;
}

int HerbMediaRecorder::addOutputFormatAndOutputSink(int output_format, int fd, int FallocateLen, bool callback_out_flag)
{
    ALOGV("addOutputFormatAndOutputSink, output_format[%d], fd[%d], FallocateLen[%d], callback_out_flag[%d]", output_format, fd, FallocateLen, callback_out_flag);
	if (output_format < OUTPUT_FORMAT_DEFAULT || output_format >= OUTPUT_FORMAT_LIST_END) {
		ALOGE("Invalid output format");
		//return BAD_TYPE;
		return -1;
	}
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		//return NO_INIT;
		return -1;
	}
    int muxerId = mr->addOutputFormatAndOutputSink(output_format, fd, FallocateLen, callback_out_flag);
    if(muxerId < 0)
    {
        ALOGE("[%s](f:%s, l:%d) muxerId[%d] is invalid.", strrchr(__FILE__, '/')+1, __FUNCTION__, __LINE__, muxerId);
    }
    return muxerId;
}

int HerbMediaRecorder::addOutputFormatAndOutputSink(int output_format, char* path, int FallocateLen, bool callback_out_flag)
{
    ALOGV("addOutputFormatAndOutputSink, output_format[%d], path[%s], FallocateLen[%d], callback_out_flag[%d]", output_format, path, FallocateLen, callback_out_flag);
	if (output_format < OUTPUT_FORMAT_DEFAULT || output_format >= OUTPUT_FORMAT_LIST_END) {
		ALOGE("Invalid output format");
		//return BAD_TYPE;
		return -1;
	}
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		//return NO_INIT;
		return -1;
	}
    int muxerId;
    if(path!=NULL)
    {
        if (strncasecmp(path, "http://", 7) != 0 && strncasecmp(path, FatFsPrefixString, strlen(FatFsPrefixString)) != 0) 
        {
            int fd = open(path, O_RDWR | O_CREAT, 0666);
			if (fd < 0) 
            {
				ALOGE("Failed to open %s", path);
				return -1;
			}
            muxerId = addOutputFormatAndOutputSink(output_format, fd, FallocateLen, callback_out_flag);
            ::close(fd);
            if(muxerId < 0)
            {
                ALOGE("[%s](f:%s, l:%d) muxerId[%d] is invalid.", strrchr(__FILE__, '/')+1, __FUNCTION__, __LINE__, muxerId);
            }
			return muxerId;
        }
    }
    muxerId = mr->addOutputFormatAndOutputSink(output_format, path, FallocateLen, callback_out_flag);
    if(muxerId < 0)
    {
        ALOGE("[%s](f:%s, l:%d) muxerId[%d] is invalid.", strrchr(__FILE__, '/')+1, __FUNCTION__, __LINE__, muxerId);
    }
    return muxerId;
}
status_t HerbMediaRecorder::removeOutputFormatAndOutputSink(int muxerId)
{
    ALOGV("removeOutputFormatAndOutputSink, muxerId[%d]", muxerId);
	if (muxerId < 0) {
		ALOGE("Invalid muxerId");
		return BAD_TYPE;
	}
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<%s> MediaRecorder not initialize", __FUNCTION__);
		return NO_INIT;
	}
    status_t opStatus = mr->removeOutputFormatAndOutputSink(muxerId);
	process_media_recorder_call(opStatus, "removeOutputFormatAndOutputSink failed.");
    return opStatus;
}

/* gushiming add record extra file for CDR start */
status_t HerbMediaRecorder::setExtraFileFd(int fd)
{
	return NO_ERROR;
}

status_t HerbMediaRecorder::setExtraFileDuration(int bfTimeMs, int afTimeMs)
{
	return NO_ERROR;
}

status_t HerbMediaRecorder::startExtraFile()
{
	return NO_ERROR;
}

status_t HerbMediaRecorder::stopExtraFile()
{
	return NO_ERROR;
}
/* gushiming add record extra file for CDR end */

status_t HerbMediaRecorder::setImpactFileDuration(int bfTimeMs, int afTimeMs)
{
    ALOGV("setImpactFileDuration(%d, %d)", bfTimeMs, afTimeMs);
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<F:%s,L:%d>MediaRecorder not initialize!", __FUNCTION__, __LINE__);
		return NO_INIT;
	}
	char buffer[64];
	status_t ret;
	snprintf(buffer, 64, "param-impact-file-duration-bftime=%d", bfTimeMs);
	ret = mr->setParameters(String8(buffer));
	if (ret != NO_ERROR) {
		ALOGE("Failed to set param-impact-file-duration-bftime");
		return ret;
	}
	snprintf(buffer, 64, "param-impact-file-duration-aftime=%d", afTimeMs);
    status_t opStatus = mr->setParameters(String8(buffer));
    process_media_recorder_call(opStatus, "setImpactFileDuration failed.");
    return opStatus;
}

status_t HerbMediaRecorder::setImpactOutputFile(int fd, int64_t fallocateLength, int muxerId)
{
    ALOGV("setImpactOutputFile");
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<F:%s,L:%d>MediaRecorder not initialize!", __FUNCTION__, __LINE__);
		return NO_INIT;
	}
	return mr->setImpactOutputFile(fd, fallocateLength, muxerId);
}

status_t HerbMediaRecorder::setImpactOutputFile(char* path, int64_t fallocateLength, int muxerId)
{
    ALOGV("setImpactOutputFile(%s)", path);
    sp<MediaRecorder> mr = getMediaRecorder();
    if (mr == NULL) {
        ALOGE("<F:%s,L:%d>MediaRecorder not initialize!", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
    status_t ret;
    if(path!=NULL)
    {
        if (strncasecmp(path, "http://", 7) != 0 && strncasecmp(path, FatFsPrefixString, strlen(FatFsPrefixString)) != 0)
        {
            int fd = open(path, O_RDWR | O_CREAT, 0666);
            if (fd < 0) 
            {
                ALOGE("Failed to open %s", path);
                return UNKNOWN_ERROR;
            }
            ret = setImpactOutputFile(fd, fallocateLength, muxerId);
            ::close(fd);
            return ret;
        }
    }
	return mr->setImpactOutputFile(path, fallocateLength, muxerId);
}

/* value: 0-main channel; 1 sub channel */
status_t HerbMediaRecorder::setSourceChannel(int channel)
{
    ALOGV("setSourceChannel(%d)", channel);
	sp<MediaRecorder> mr = getMediaRecorder();
	if (mr == NULL) {
		ALOGE("<F:%s,L:%d>MediaRecorder not initialize!", __FUNCTION__, __LINE__);
		return NO_INIT;
	}
	char buffer[64];
	snprintf(buffer, 64, "param-camera-source-channel=%d", channel);
	status_t opStatus = mr->setParameters(String8(buffer));
    process_media_recorder_call(opStatus, "setSourceChannel failed.");
    return opStatus;
}

status_t HerbMediaRecorder::enableDynamicBitRateControl(bool bEnable)
{
    ALOGV("enableDynamicBitRateControl(%d)", bEnable);
    sp<MediaRecorder> mr = getMediaRecorder();
    if (mr == NULL) 
    {
        ALOGE("(f:%s, l:%d) MediaRecorder not initialize!", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
    char buffer[64];
    snprintf(buffer, 64, "dynamic-bitrate-control=%d", bEnable);
    status_t opStatus = mr->setParameters(String8(buffer));
    process_media_recorder_call(opStatus, "enableDynamicBitRateControl failed.");
    return opStatus;
}

};

