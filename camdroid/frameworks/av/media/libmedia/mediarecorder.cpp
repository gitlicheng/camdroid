/*
 **
 ** Copyright (c) 2008 The Android Open Source Project
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "MediaRecorder"
#include <utils/Log.h>
#include <media/mediarecorder.h>
#include <binder/IServiceManager.h>
#include <utils/String8.h>
#include <media/IMediaPlayerService.h>
#include <media/IMediaRecorder.h>
#include <media/mediaplayer.h>  // for MEDIA_ERROR_SERVER_DIED
//#include <gui/ISurfaceTexture.h>
#include <binder/IPCThreadState.h>
#include <fcntl.h>

namespace android {

#define GET_CALLING_PID	(IPCThreadState::self()->getCallingPid())
void getCallingProcessName(char *name)
{
	char proc_node[128];

	if (name == 0)
	{
		ALOGE("error in params");
		return;
	}
	
	memset(proc_node, 0, sizeof(proc_node));
	sprintf(proc_node, "/proc/%d/cmdline", GET_CALLING_PID);
	int fp = ::open(proc_node, O_RDONLY);
	if (fp > 0) 
	{
		memset(name, 0, 128);
		::read(fp, name, 128);
		::close(fp);
		fp = 0;
		ALOGD("Calling process is: %s !", name);
	}
	else 
	{
		ALOGE("Obtain calling process failed");
	}
}

status_t MediaRecorder::setCamera(const sp<ICamera>& camera, const sp<ICameraRecordingProxy>& proxy)
{
    ALOGV("setCamera(%p,%p)", camera.get(), proxy.get());
    if (mMediaRecorder == NULL) {
        ALOGE("media recorder is not initialized yet");
        return INVALID_OPERATION;
    }
    if (!(mCurrentState & MEDIA_RECORDER_IDLE)) {
        ALOGE("setCamera called in an invalid state(%d)", mCurrentState);
        return INVALID_OPERATION;
    }

    status_t ret = mMediaRecorder->setCamera(camera, proxy);
    if (OK != ret) {
        ALOGV("setCamera failed: %d", ret);
        mCurrentState = MEDIA_RECORDER_ERROR;
        return ret;
    }
    return ret;
}

//status_t MediaRecorder::setPreviewSurface(const sp<Surface>& surface)
status_t MediaRecorder::setPreviewSurface(const unsigned int hlay)
{
    ALOGV("setPreviewSurface(%d)", hlay);
    if (mMediaRecorder == NULL) {
        ALOGE("media recorder is not initialized yet");
        return INVALID_OPERATION;
    }
    if (!(mCurrentState & MEDIA_RECORDER_DATASOURCE_CONFIGURED)) {
        ALOGE("setPreviewSurface called in an invalid state(%d)", mCurrentState);
        return INVALID_OPERATION;
    }
    if (!mIsVideoSourceSet) {
        ALOGE("try to set preview surface without setting the video source first");
        return INVALID_OPERATION;
    }

    status_t ret = mMediaRecorder->setPreviewSurface(hlay);
    if (OK != ret) {
        ALOGV("setPreviewSurface failed: %d", ret);
        mCurrentState = MEDIA_RECORDER_ERROR;
        return ret;
    }
    return ret;
}

status_t MediaRecorder::init()
{
    ALOGV("init");
    if (mMediaRecorder == NULL) {
        ALOGE("media recorder is not initialized yet");
        return INVALID_OPERATION;
    }
    if (!(mCurrentState & MEDIA_RECORDER_IDLE)) {
        ALOGE("init called in an invalid state(%d)", mCurrentState);
        return INVALID_OPERATION;
    }

    status_t ret = mMediaRecorder->init();
    if (OK != ret) {
        ALOGV("init failed: %d", ret);
        mCurrentState = MEDIA_RECORDER_ERROR;
        return ret;
    }

    ret = mMediaRecorder->setListener(this);
    if (OK != ret) {
        ALOGV("setListener failed: %d", ret);
        mCurrentState = MEDIA_RECORDER_ERROR;
        return ret;
    }

    mCurrentState = MEDIA_RECORDER_INITIALIZED;
    return ret;
}

status_t MediaRecorder::setVideoSource(int vs)
{
    ALOGV("setVideoSource(%d)", vs);
    if (mMediaRecorder == NULL) {
        ALOGE("media recorder is not initialized yet");
        return INVALID_OPERATION;
    }
    if (mIsVideoSourceSet) {
        ALOGE("video source has already been set");
        return INVALID_OPERATION;
    }
    if (mCurrentState & MEDIA_RECORDER_IDLE) {
        ALOGV("Call init() since the media recorder is not initialized yet");
        status_t ret = init();
        if (OK != ret) {
            return ret;
        }
    }
    if (!(mCurrentState & MEDIA_RECORDER_INITIALIZED)) {
        ALOGE("setVideoSource called in an invalid state(%d)", mCurrentState);
        return INVALID_OPERATION;
    }

    // following call is made over the Binder Interface
    status_t ret = mMediaRecorder->setVideoSource(vs);

    if (OK != ret) {
        ALOGV("setVideoSource failed: %d", ret);
        mCurrentState = MEDIA_RECORDER_ERROR;
        return ret;
    }
    mIsVideoSourceSet = true;
    return ret;
}

status_t MediaRecorder::setAudioSource(int as)
{
    ALOGV("setAudioSource(%d)", as);
    if (mMediaRecorder == NULL) {
        ALOGE("media recorder is not initialized yet");
        return INVALID_OPERATION;
    }
    if (mCurrentState & MEDIA_RECORDER_IDLE) {
        ALOGV("Call init() since the media recorder is not initialized yet");
        status_t ret = init();
        if (OK != ret) {
            return ret;
        }
    }
    if (mIsAudioSourceSet) {
        ALOGE("audio source has already been set");
        return INVALID_OPERATION;
    }
    if (!(mCurrentState & MEDIA_RECORDER_INITIALIZED)) {
        ALOGE("setAudioSource called in an invalid state(%d)", mCurrentState);
        return INVALID_OPERATION;
    }

    status_t ret = mMediaRecorder->setAudioSource(as);
    if (OK != ret) {
        ALOGV("setAudioSource failed: %d", ret);
        mCurrentState = MEDIA_RECORDER_ERROR;
        return ret;
    }
    mIsAudioSourceSet = true;
    return ret;
}

status_t MediaRecorder::setOutputFormat(int of)
{
    ALOGV("setOutputFormat(%d)", of);
    if (mMediaRecorder == NULL) {
        ALOGE("media recorder is not initialized yet");
        return INVALID_OPERATION;
    }
    if (!(mCurrentState & MEDIA_RECORDER_INITIALIZED)) {
        ALOGE("setOutputFormat called in an invalid state: %d", mCurrentState);
        return INVALID_OPERATION;
    }
    if (mIsVideoSourceSet && of >= OUTPUT_FORMAT_AUDIO_ONLY_START && of != OUTPUT_FORMAT_RTP_AVP && of != OUTPUT_FORMAT_MPEG2TS && of != OUTPUT_FORMAT_AWTS && of != OUTPUT_FORMAT_RAW) { //first non-video output format
        ALOGE("output format (%d) is meant for audio recording only and incompatible with video recording", of);
        return INVALID_OPERATION;
    }

    status_t ret = mMediaRecorder->setOutputFormat(of);
    if (OK != ret) {
        ALOGE("setOutputFormat failed: %d", ret);
        mCurrentState = MEDIA_RECORDER_ERROR;
        return ret;
    }
    mCurrentState = MEDIA_RECORDER_DATASOURCE_CONFIGURED;
    return ret;
}

status_t MediaRecorder::setVideoEncoder(int ve)
{
    ALOGV("setVideoEncoder(%d)", ve);
    if (mMediaRecorder == NULL) {
        ALOGE("media recorder is not initialized yet");
        return INVALID_OPERATION;
    }
    if (!mIsVideoSourceSet) {
        ALOGE("try to set the video encoder without setting the video source first");
        return INVALID_OPERATION;
    }
    if (mIsVideoEncoderSet) {
        ALOGE("video encoder has already been set");
        return INVALID_OPERATION;
    }
    if (!(mCurrentState & MEDIA_RECORDER_DATASOURCE_CONFIGURED)) {
        ALOGE("setVideoEncoder called in an invalid state(%d)", mCurrentState);
        return INVALID_OPERATION;
    }

    status_t ret = mMediaRecorder->setVideoEncoder(ve);
    if (OK != ret) {
        ALOGV("setVideoEncoder failed: %d", ret);
        mCurrentState = MEDIA_RECORDER_ERROR;
        return ret;
    }
    mIsVideoEncoderSet = true;
    return ret;
}

status_t MediaRecorder::setAudioEncoder(int ae)
{
    ALOGV("setAudioEncoder(%d)", ae);
    if (mMediaRecorder == NULL) {
        ALOGE("media recorder is not initialized yet");
        return INVALID_OPERATION;
    }
    if (!mIsAudioSourceSet) {
        ALOGE("try to set the audio encoder without setting the audio source first");
        return INVALID_OPERATION;
    }
    if (mIsAudioEncoderSet) {
        ALOGE("audio encoder has already been set");
        return INVALID_OPERATION;
    }
    if (!(mCurrentState & MEDIA_RECORDER_DATASOURCE_CONFIGURED)) {
        ALOGE("setAudioEncoder called in an invalid state(%d)", mCurrentState);
        return INVALID_OPERATION;
    }

    status_t ret = mMediaRecorder->setAudioEncoder(ae);
    if (OK != ret) {
        ALOGV("setAudioEncoder failed: %d", ret);
        mCurrentState = MEDIA_RECORDER_ERROR;
        return ret;
    }
    mIsAudioEncoderSet = true;
    return ret;
}

status_t MediaRecorder::setOutputFile(const char* path)
{
    ALOGV("setOutputFile(%s)", path);
    if (mMediaRecorder == NULL) {
        ALOGE("media recorder is not initialized yet");
        return INVALID_OPERATION;
    }
    if (mIsOutputFileSet) {
        ALOGE("output file has already been set");
        return INVALID_OPERATION;
    }
    if (!(mCurrentState & MEDIA_RECORDER_DATASOURCE_CONFIGURED)) {
        ALOGE("setOutputFile called in an invalid state(%d)", mCurrentState);
        return INVALID_OPERATION;
    }

    status_t ret = mMediaRecorder->setOutputFile(path);
    if (OK != ret) {
        ALOGV("setOutputFile failed: %d", ret);
        mCurrentState = MEDIA_RECORDER_ERROR;
        return ret;
    }
    mIsOutputFileSet = true;
    return ret;
}

status_t MediaRecorder::setOutputFile(const char* path, int64_t offset, int64_t length)
{
    ALOGV("setOutputFile(%s), %lld, %lld", path, offset, length);
    if (mMediaRecorder == NULL) {
        ALOGE("media recorder is not initialized yet");
        return INVALID_OPERATION;
    }
    if (mIsOutputFileSet) {
        ALOGE("output file has already been set");
        return INVALID_OPERATION;
    }
    if (!(mCurrentState & MEDIA_RECORDER_DATASOURCE_CONFIGURED)) {
        ALOGE("setOutputFile called in an invalid state(%d)", mCurrentState);
        return INVALID_OPERATION;
    }

    status_t ret = mMediaRecorder->setOutputFile(path, offset, length);
    if (OK != ret) {
        ALOGV("setOutputFile failed: %d", ret);
        mCurrentState = MEDIA_RECORDER_ERROR;
        return ret;
    }
    mIsOutputFileSet = true;
    return ret;
}

status_t MediaRecorder::setOutputFile(int fd, int64_t offset, int64_t length)
{
    ALOGV("setOutputFile(%d, %lld, %lld)", fd, offset, length);
    if (mMediaRecorder == NULL) {
        ALOGE("media recorder is not initialized yet");
        return INVALID_OPERATION;
    }

    if (mIsOutputFileSet) {
        ALOGE("output file has already been set");
        return INVALID_OPERATION;
    }
    if (!(mCurrentState & MEDIA_RECORDER_DATASOURCE_CONFIGURED)) {
        ALOGE("setOutputFile called in an invalid state(%d)", mCurrentState);
        return INVALID_OPERATION;
    }

    // It appears that if an invalid file descriptor is passed through
    // binder calls, the server-side of the inter-process function call
    // is skipped. As a result, the check at the server-side to catch
    // the invalid file descritpor never gets invoked. This is to workaround
    // this issue by checking the file descriptor first before passing
    // it through binder call.
    if (fd < 0) {
        ALOGE("Invalid file descriptor: %d", fd);
        return BAD_VALUE;
    }

    status_t ret = mMediaRecorder->setOutputFile(fd, offset, length);
    if (OK != ret) {
        ALOGV("setOutputFile failed: %d", ret);
        mCurrentState = MEDIA_RECORDER_ERROR;
        return ret;
    }
    mIsOutputFileSet = true;
    return ret;
}

status_t MediaRecorder::setVideoSize(int width, int height)
{
    ALOGV("setVideoSize(%d, %d)", width, height);
    if (mMediaRecorder == NULL) {
        ALOGE("media recorder is not initialized yet");
        return INVALID_OPERATION;
    }
    if (!(mCurrentState & MEDIA_RECORDER_DATASOURCE_CONFIGURED)) {
        ALOGE("setVideoSize called in an invalid state: %d", mCurrentState);
        return INVALID_OPERATION;
    }
    if (!mIsVideoSourceSet) {
        ALOGE("Cannot set video size without setting video source first");
        return INVALID_OPERATION;
    }

    status_t ret = mMediaRecorder->setVideoSize(width, height);
    if (OK != ret) {
        ALOGE("setVideoSize failed: %d", ret);
        mCurrentState = MEDIA_RECORDER_ERROR;
        return ret;
    }

    return ret;
}

// Query a SurfaceMediaSurface through the Mediaserver, over the
// binder interface. This is used by the Filter Framework (MeidaEncoder)
// to get an <ISurfaceTexture> object to hook up to ANativeWindow.
unsigned int MediaRecorder::
        querySurfaceMediaSourceFromMediaServer()
{
    Mutex::Autolock _l(mLock);
    mLayerMediaSource =
            mMediaRecorder->querySurfaceMediaSource();
    if (mLayerMediaSource == 0) {
        ALOGE("SurfaceMediaSource could not be initialized!");
    }
    return mLayerMediaSource;
}

status_t MediaRecorder::queueBuffer(int index, int addr_y, int addr_c, int64_t timestamp)
{
	ALOGV("queueBuffer(%d)", index);
	if(mMediaRecorder == NULL) {
		ALOGE("media recorder is not initialized yet");
		return INVALID_OPERATION;
	}

	status_t ret = mMediaRecorder->queueBuffer(index, addr_y, addr_c, timestamp);
	if (OK != ret) {
		ALOGV("setVideoEncoder failed: %d", ret);
		mCurrentState = MEDIA_RECORDER_ERROR;
		return ret;
	}

	return ret;
}

sp<IMemory> MediaRecorder::getOneBsFrame(int mode)
{
	ALOGV("getOneBsFrame");
	if(mMediaRecorder == NULL) {
		ALOGE("media recorder is not initialized yet");
		return NULL;
	}

	return mMediaRecorder->getOneBsFrame(mode);
}

void MediaRecorder::freeOneBsFrame()
{
	ALOGV("freeOneBsFrame");
	if(mMediaRecorder == NULL) {
		ALOGE("media recorder is not initialized yet");
		return;
	}

	mMediaRecorder->freeOneBsFrame();
}

sp<IMemory> MediaRecorder::getEncDataHeader()
{
	ALOGV("getEncDataHeader");
	if(mMediaRecorder == NULL) {
		ALOGE("media recorder is not initialized yet");
		return NULL;
	}

	return mMediaRecorder->getEncDataHeader();
}

status_t MediaRecorder::setVideoEncodingBitRateSync(int bitRate)
{
	ALOGV("setVideoEncodingBitRateSync(%d)", bitRate);
	if(mMediaRecorder == NULL) {
		ALOGE("media recorder is not initialized yet");
		return NO_INIT;
	}

	return mMediaRecorder->setVideoEncodingBitRateSync(bitRate);
}

status_t MediaRecorder::setVideoFrameRateSync(int frames_per_second)
{
	ALOGV("setVideoFrameRateSync(%d)", frames_per_second);
	if(mMediaRecorder == NULL) {
		ALOGE("media recorder is not initialized yet");
		return NO_INIT;
	}

	return mMediaRecorder->setVideoFrameRateSync(frames_per_second);
}

status_t MediaRecorder::setVideoFrameRate(int frames_per_second)
{
    ALOGV("setVideoFrameRate(%d)", frames_per_second);
    if (mMediaRecorder == NULL) {
        ALOGE("media recorder is not initialized yet");
        return INVALID_OPERATION;
    }
    if (!(mCurrentState & MEDIA_RECORDER_DATASOURCE_CONFIGURED)) {
        ALOGE("setVideoFrameRate called in an invalid state: %d", mCurrentState);
        return INVALID_OPERATION;
    }
    if (!mIsVideoSourceSet) {
        ALOGE("Cannot set video frame rate without setting video source first");
        return INVALID_OPERATION;
    }

    status_t ret = mMediaRecorder->setVideoFrameRate(frames_per_second);
    if (OK != ret) {
        ALOGE("setVideoFrameRate failed: %d", ret);
        mCurrentState = MEDIA_RECORDER_ERROR;
        return ret;
    }
    return ret;
}

status_t MediaRecorder::setMuteMode(bool mute)
{
    ALOGV("setMuteMode(%d)", mute);
    if (mMediaRecorder == NULL) {
        ALOGE("media recorder is not initialized yet");
        return INVALID_OPERATION;
    }

    status_t ret = mMediaRecorder->setMuteMode(mute);
    if (OK != ret) {
        ALOGE("setMuteMode failed: %d", ret);
    }

    return ret;
}

status_t MediaRecorder::setParameters(const String8& params) {
    ALOGV("setParameters(%s)", params.string());
    if (mMediaRecorder == NULL) {
        ALOGE("media recorder is not initialized yet");
        return INVALID_OPERATION;
    }

    bool isInvalidState = (mCurrentState &
                           (MEDIA_RECORDER_PREPARED |
                            MEDIA_RECORDER_RECORDING |
                            MEDIA_RECORDER_ERROR));
    if (isInvalidState) {
        ALOGE("setParameters is called in an invalid state: %d", mCurrentState);
        return INVALID_OPERATION;
    }

    status_t ret = mMediaRecorder->setParameters(params);
    if (OK != ret) {
        ALOGE("setParameters(%s) failed: %d", params.string(), ret);
        // Do not change our current state to MEDIA_RECORDER_ERROR, failures
        // of the only currently supported parameters, "max-duration" and
        // "max-filesize" are _not_ fatal.
    }

    return ret;
}

status_t MediaRecorder::prepare()
{
    ALOGV("prepare");
    if (mMediaRecorder == NULL) {
        ALOGE("media recorder is not initialized yet");
        return INVALID_OPERATION;
    }
    if (!(mCurrentState & MEDIA_RECORDER_DATASOURCE_CONFIGURED)) {
        ALOGE("prepare called in an invalid state: %d", mCurrentState);
        return INVALID_OPERATION;
    }
    if (mIsAudioSourceSet != mIsAudioEncoderSet) {
        if (mIsAudioSourceSet) {
            ALOGE("audio source is set, but audio encoder is not set");
        } else {  // must not happen, since setAudioEncoder checks this already
            ALOGE("audio encoder is set, but audio source is not set");
        }
        return INVALID_OPERATION;
    }

    if (mIsVideoSourceSet != mIsVideoEncoderSet) {
        if (mIsVideoSourceSet) {
            ALOGE("video source is set, but video encoder is not set");
        } else {  // must not happen, since setVideoEncoder checks this already
            ALOGE("video encoder is set, but video source is not set");
        }
        return INVALID_OPERATION;
    }

    status_t ret = mMediaRecorder->prepare();
    if (OK != ret) {
        ALOGE("prepare failed: %d", ret);
        mCurrentState = MEDIA_RECORDER_ERROR;
        return ret;
    }
    mCurrentState = MEDIA_RECORDER_PREPARED;
    return ret;
}

status_t MediaRecorder::getMaxAmplitude(int* max)
{
    ALOGV("getMaxAmplitude");
    if (mMediaRecorder == NULL) {
        ALOGE("media recorder is not initialized yet");
        return INVALID_OPERATION;
    }
    if (mCurrentState & MEDIA_RECORDER_ERROR) {
        ALOGE("getMaxAmplitude called in an invalid state: %d", mCurrentState);
        return INVALID_OPERATION;
    }

    status_t ret = mMediaRecorder->getMaxAmplitude(max);
    if (OK != ret) {
        ALOGE("getMaxAmplitude failed: %d", ret);
        mCurrentState = MEDIA_RECORDER_ERROR;
        return ret;
    }
    return ret;
}

status_t MediaRecorder::start()
{
    ALOGV("start");
    if (mMediaRecorder == NULL) {
        ALOGE("media recorder is not initialized yet");
        return INVALID_OPERATION;
    }
    if (!(mCurrentState & MEDIA_RECORDER_PREPARED)) {
        ALOGE("start called in an invalid state: %d", mCurrentState);
        return INVALID_OPERATION;
    }

    status_t ret = mMediaRecorder->start();
    if (OK != ret) {
        ALOGE("start failed: %d", ret);
        mCurrentState = MEDIA_RECORDER_ERROR;
        return ret;
    }
    mCurrentState = MEDIA_RECORDER_RECORDING;
    return ret;
}

status_t MediaRecorder::stop()
{
    ALOGV("stop");
    if (mMediaRecorder == NULL) {
        ALOGE("media recorder is not initialized yet");
        return INVALID_OPERATION;
    }
    if (!(mCurrentState & MEDIA_RECORDER_RECORDING)) {
        ALOGE("stop called in an invalid state: %d", mCurrentState);
        return INVALID_OPERATION;
    }

    status_t ret = mMediaRecorder->stop();
    if (OK != ret) {
        ALOGE("stop failed: %d", ret);
        mCurrentState = MEDIA_RECORDER_ERROR;
        return ret;
    }

    // FIXME:
    // stop and reset are semantically different.
    // We treat them the same for now, and will change this in the future.
    doCleanUp();
    mCurrentState = MEDIA_RECORDER_IDLE;
    return ret;
}

// Reset should be OK in any state
status_t MediaRecorder::reset()
{
    ALOGV("reset");
    if (mMediaRecorder == NULL) {
        ALOGE("media recorder is not initialized yet");
        return INVALID_OPERATION;
    }

#ifdef FOR_CTS_TEST
    char *mCallingProcessName = NULL;
    mCallingProcessName = (char *)malloc(sizeof(char) * 128);
	getCallingProcessName(mCallingProcessName);
    ALOGV("mCallingProcessName %s", mCallingProcessName);
#endif

    doCleanUp();
    status_t ret = UNKNOWN_ERROR;
    switch (mCurrentState) {
        case MEDIA_RECORDER_IDLE:
            ret = OK;
            break;
        case MEDIA_RECORDER_PREPARED:
//        if (strcmp(mCallingProcessName, "com.android.cts.media") == 0) {
//            ret = OK;
//            break;
//        }
        case MEDIA_RECORDER_RECORDING:
        case MEDIA_RECORDER_DATASOURCE_CONFIGURED:
        //case MEDIA_RECORDER_PREPARED:
        case MEDIA_RECORDER_ERROR: {
            ret = doReset();
            if (OK != ret) {
#ifdef FOR_CTS_TEST
                if (mCallingProcessName != NULL) {
            		free(mCallingProcessName);
            		mCallingProcessName = NULL;
            	}
#endif
                return ret;  // No need to continue
            }
        }  // Intentional fall through
        case MEDIA_RECORDER_INITIALIZED:
            ret = close();
            break;

        default: {
            ALOGE("Unexpected non-existing state: %d", mCurrentState);
            break;
        }
    }

#ifdef FOR_CTS_TEST
    if (mCallingProcessName != NULL) {
		free(mCallingProcessName);
		mCallingProcessName = NULL;
	}
#endif

    return ret;
}

status_t MediaRecorder::close()
{
    ALOGV("close");
    if (!(mCurrentState & MEDIA_RECORDER_INITIALIZED)) {
        ALOGE("close called in an invalid state: %d", mCurrentState);
        return INVALID_OPERATION;
    }
    status_t ret = mMediaRecorder->close();
    if (OK != ret) {
        ALOGE("close failed: %d", ret);
        mCurrentState = MEDIA_RECORDER_ERROR;
        return UNKNOWN_ERROR;
    } else {
        mCurrentState = MEDIA_RECORDER_IDLE;
    }
    return ret;
}

status_t MediaRecorder::doReset()
{
    ALOGV("doReset");
    status_t ret = mMediaRecorder->reset();
    if (OK != ret) {
        ALOGE("doReset failed: %d", ret);
        mCurrentState = MEDIA_RECORDER_ERROR;
        return ret;
    } else {
        mCurrentState = MEDIA_RECORDER_INITIALIZED;
    }
    return ret;
}

void MediaRecorder::doCleanUp()
{
    ALOGV("doCleanUp");
    mIsAudioSourceSet  = false;
    mIsVideoSourceSet  = false;
    mIsAudioEncoderSet = false;
    mIsVideoEncoderSet = false;
    mIsOutputFileSet   = false;
}

// Release should be OK in any state
status_t MediaRecorder::release()
{
    ALOGV("release");
    if (mMediaRecorder != NULL) {
        return mMediaRecorder->release();
    }
    return INVALID_OPERATION;
}

MediaRecorder::MediaRecorder() : mLayerMediaSource(0)
{
    ALOGV("constructor");

    const sp<IMediaPlayerService>& service(getMediaPlayerService());
    if (service != NULL) {
        mMediaRecorder = service->createMediaRecorder(getpid());
    }
    if (mMediaRecorder != NULL) {
        mCurrentState = MEDIA_RECORDER_IDLE;
    }


    doCleanUp();
}

status_t MediaRecorder::initCheck()
{
    return mMediaRecorder != 0 ? NO_ERROR : NO_INIT;
}

MediaRecorder::~MediaRecorder()
{
    ALOGV("destructor");
    if (mMediaRecorder != NULL) {
        mMediaRecorder.clear();
    }

    if (mLayerMediaSource != 0) {
        //mSurfaceMediaSource.clear();
        ALOGD("(f:%s, l:%d), destroy MediaRecorder, mLayerMediaSource[%d] is not null", __FUNCTION__, __LINE__, mLayerMediaSource);
    }
}

status_t MediaRecorder::setListener(const sp<MediaRecorderListener>& listener)
{
    ALOGV("setListener");
    Mutex::Autolock _l(mLock);
    mListener = listener;

    return NO_ERROR;
}

void MediaRecorder::notify(int msg, int ext1, int ext2)
{
    ALOGV("message received msg=%d, ext1=%d, ext2=%d", msg, ext1, ext2);

    sp<MediaRecorderListener> listener;
    mLock.lock();
    listener = mListener;
    mLock.unlock();

    if (listener != NULL) {
        Mutex::Autolock _l(mNotifyLock);
        ALOGV("notify callback application");
        listener->notify(msg, ext1, ext2);
        ALOGV("back from notify callback");
    }
}

void MediaRecorder::died()
{
    ALOGV("died");
    notify(MEDIA_RECORDER_EVENT_ERROR, MEDIA_ERROR_SERVER_DIED, 0);
}

status_t MediaRecorder::setVideoEncodingIFramesNumberIntervalSync(int nMaxKeyItl)
{
    ALOGV("setVideoEncodingIFramesNumberIntervalSync(%d)", nMaxKeyItl);
	if(mMediaRecorder == NULL) {
		ALOGE("media recorder is not initialized yet");
		return NO_INIT;
	}

    bool isValidState = (mCurrentState & MEDIA_RECORDER_RECORDING);
    if (!isValidState) {
        ALOGE("setVideoEncodingIFramesNumberIntervalSync is called in an invalid state: %d", mCurrentState);
        return INVALID_OPERATION;
    }
    
	return mMediaRecorder->setVideoEncodingIFramesNumberIntervalSync(nMaxKeyItl);
}

status_t MediaRecorder::reencodeIFrame()
{
    ALOGV("reencodeIFrame");
	if(mMediaRecorder == NULL) {
		ALOGE("media recorder is not initialized yet");
		return NO_INIT;
	}

    bool isValidState = (mCurrentState & MEDIA_RECORDER_RECORDING);
    if (!isValidState) {
        ALOGE("reencodeIFrame is called in an invalid state: %d", mCurrentState);
        return INVALID_OPERATION;
    }
    
	return mMediaRecorder->reencodeIFrame();
}

status_t MediaRecorder::setOutputFileSync(int fd, int64_t fallocateLength, int muxerId)
{
    ALOGV("setOutputFileSync(%d, %lld, %lld)", fd, offset, length);
    if (mMediaRecorder == NULL) {
        ALOGE("media recorder is not initialized yet");
        return INVALID_OPERATION;
    }

//    if (mIsOutputFileSet) {
//        ALOGE("output file has already been set");
//        return INVALID_OPERATION;
//    }
    if (!(mCurrentState & MEDIA_RECORDER_RECORDING)) {
        ALOGE("setOutputFileSync called in an invalid state(%d)", mCurrentState);
        return INVALID_OPERATION;
    }

    // It appears that if an invalid file descriptor is passed through
    // binder calls, the server-side of the inter-process function call
    // is skipped. As a result, the check at the server-side to catch
    // the invalid file descritpor never gets invoked. This is to workaround
    // this issue by checking the file descriptor first before passing
    // it through binder call.
    if (fd < 0) {
        ALOGE("Invalid file descriptor: %d", fd);
        return BAD_VALUE;
    }

    status_t ret = mMediaRecorder->setOutputFileSync(fd, fallocateLength, muxerId);
    if (OK != ret) {
        ALOGV("setOutputFileSync /failed: %d", ret);
        mCurrentState = MEDIA_RECORDER_ERROR;
        return ret;
    }
    return ret;
}

status_t MediaRecorder::setOutputFileSync(const char* path, int64_t fallocateLength, int muxerId)
{
    ALOGV("setOutputFileSync(%s, %lld, %lld)", path, offset, length);
    if (mMediaRecorder == NULL) {
        ALOGE("media recorder is not initialized yet");
        return INVALID_OPERATION;
    }

//    if (mIsOutputFileSet) {
//        ALOGE("output file has already been set");
//        return INVALID_OPERATION;
//    }
    if (!(mCurrentState & MEDIA_RECORDER_RECORDING)) {
        ALOGE("setOutputFileSync called in an invalid state(%d)", mCurrentState);
        return INVALID_OPERATION;
    }

    // It appears that if an invalid file descriptor is passed through
    // binder calls, the server-side of the inter-process function call
    // is skipped. As a result, the check at the server-side to catch
    // the invalid file descritpor never gets invoked. This is to workaround
    // this issue by checking the file descriptor first before passing
    // it through binder call.
    if (path == NULL) {
        ALOGE("Invalid file path=NULL");
        return BAD_VALUE;
    }

    status_t ret = mMediaRecorder->setOutputFileSync(path, fallocateLength, muxerId);
    if (OK != ret) {
        ALOGV("setOutputFileSync /failed: %d", ret);
        mCurrentState = MEDIA_RECORDER_ERROR;
        return ret;
    }
    return ret;
}

status_t MediaRecorder::setSdcardState(bool bExist)
{
    ALOGV("setSdcardState(%d)", bExist);
	if(mMediaRecorder == NULL) {
		ALOGE("media recorder is not initialized yet");
		return NO_INIT;
	}

	return mMediaRecorder->setSdcardState(bExist);
}

int MediaRecorder::addOutputFormatAndOutputSink(int of, int fd, int FallocateLen, bool callback_out_flag)
{
    ALOGV("addOutputFormatAndOutputSink(%d)", of);
    if (mMediaRecorder == NULL) {
        ALOGE("media recorder is not initialized yet");
        //return INVALID_OPERATION;
        return -1;
    }
    if (!(mCurrentState & MEDIA_RECORDER_INITIALIZED) && !(mCurrentState & MEDIA_RECORDER_DATASOURCE_CONFIGURED) && !(mCurrentState & MEDIA_RECORDER_RECORDING)) {
//        ALOGE("setOutputFormat called in an invalid state: %d", mCurrentState);
        ALOGE("(f:%s, l:%d) addOutputFormatAndOutputSink called in an invalid state: %d", __FUNCTION__, __LINE__, mCurrentState);
        //return INVALID_OPERATION;
        return -1;
    }
    if (mIsVideoSourceSet && of >= OUTPUT_FORMAT_AUDIO_ONLY_START && of != OUTPUT_FORMAT_RTP_AVP && of != OUTPUT_FORMAT_MPEG2TS && of != OUTPUT_FORMAT_AWTS && of != OUTPUT_FORMAT_RAW) { //first non-video output format
        ALOGE("output format (%d) is meant for audio recording only and incompatible with video recording", of);
        //return INVALID_OPERATION;
        return -1;
    }

    int muxerId = mMediaRecorder->addOutputFormatAndOutputSink(of, fd, FallocateLen, callback_out_flag);
    if (muxerId < 0) 
    {
        if((mCurrentState & MEDIA_RECORDER_INITIALIZED))
        {
            ALOGE("addOutputFormatAndOutputSink failed: %d, turn mCurrentState state[%d]->[%d]", muxerId, mCurrentState, MEDIA_RECORDER_ERROR);
            mCurrentState = MEDIA_RECORDER_ERROR;
        }
        else
        {
            ALOGE("(f:%s, l:%d) addOutputFormatAndOutputSink failed: %d, keep mCurrentState state[%d]", __FUNCTION__, __LINE__, muxerId, mCurrentState);
        }
        return -1;
    }
    if ((mCurrentState & MEDIA_RECORDER_INITIALIZED))
    {
        mCurrentState = MEDIA_RECORDER_DATASOURCE_CONFIGURED;
    }
    return muxerId;
}

int MediaRecorder::addOutputFormatAndOutputSink(int of, const char* path, int FallocateLen, bool callback_out_flag)
{
    ALOGV("addOutputFormatAndOutputSink(%d), path[%s]", of, path);
    if (mMediaRecorder == NULL) {
        ALOGE("media recorder is not initialized yet");
        //return INVALID_OPERATION;
        return -1;
    }
    if (!(mCurrentState & MEDIA_RECORDER_INITIALIZED) && !(mCurrentState & MEDIA_RECORDER_DATASOURCE_CONFIGURED) && !(mCurrentState & MEDIA_RECORDER_RECORDING)) {
//        ALOGE("setOutputFormat called in an invalid state: %d", mCurrentState);
        ALOGE("(f:%s, l:%d) addOutputFormatAndOutputSink called in an invalid state: %d", __FUNCTION__, __LINE__, mCurrentState);
        //return INVALID_OPERATION;
        return -1;
    }
    if (mIsVideoSourceSet && of >= OUTPUT_FORMAT_AUDIO_ONLY_START && of != OUTPUT_FORMAT_RTP_AVP && of != OUTPUT_FORMAT_MPEG2TS && of != OUTPUT_FORMAT_AWTS && of != OUTPUT_FORMAT_RAW) { //first non-video output format
        ALOGE("output format (%d) is meant for audio recording only and incompatible with video recording", of);
        //return INVALID_OPERATION;
        return -1;
    }

    int muxerId = mMediaRecorder->addOutputFormatAndOutputSink(of, path, FallocateLen, callback_out_flag);
    if (muxerId < 0) 
    {
        if((mCurrentState & MEDIA_RECORDER_INITIALIZED))
        {
            ALOGE("addOutputFormatAndOutputSink failed: %d, turn mCurrentState state[%d]->[%d]", muxerId, mCurrentState, MEDIA_RECORDER_ERROR);
            mCurrentState = MEDIA_RECORDER_ERROR;
        }
        else
        {
            ALOGE("(f:%s, l:%d) addOutputFormatAndOutputSink failed: %d, keep mCurrentState state[%d]", __FUNCTION__, __LINE__, muxerId, mCurrentState);
        }
        return -1;
    }
    if ((mCurrentState & MEDIA_RECORDER_INITIALIZED))
    {
        mCurrentState = MEDIA_RECORDER_DATASOURCE_CONFIGURED;
    }
    return muxerId;
}

status_t MediaRecorder::removeOutputFormatAndOutputSink(int muxerId)
{
    ALOGD("removeOutputFormatAndOutputSink(%d), in state[%d]", muxerId, mCurrentState);
    if (mMediaRecorder == NULL) {
        ALOGE("media recorder is not initialized yet");
        return INVALID_OPERATION;
    }
    if (!(mCurrentState & MEDIA_RECORDER_DATASOURCE_CONFIGURED) && !(mCurrentState & MEDIA_RECORDER_RECORDING)) {
        ALOGE("(f:%s, l:%d) removeOutputFormatAndOutputSink called in an invalid state: %d", __FUNCTION__, __LINE__, mCurrentState);
        return INVALID_OPERATION;
    }
    if (mIsVideoSourceSet && muxerId < 0) {
        ALOGE("muxerId (%d) is invalid", muxerId);
        return INVALID_OPERATION;
    }

    status_t ret = mMediaRecorder->removeOutputFormatAndOutputSink(muxerId);
    if (OK != ret) {
        ALOGE("removeOutputFormatAndOutputSink failed: %d, muxerId[%d]", ret, muxerId);
        //mCurrentState = MEDIA_RECORDER_ERROR;
        return ret;
    }
    return ret;
}

status_t MediaRecorder::setMaxDuration(int max_duration_ms)
{
    ALOGV("setMaxDuration");
	if(mMediaRecorder == NULL) {
		ALOGE("<F:%s,L:%d>MediaRecorder not initialize yet!", __FUNCTION__, __LINE__);
		return NO_INIT;
	}

	return mMediaRecorder->setMaxDuration(max_duration_ms);
}

status_t MediaRecorder::setMaxFileSize(int64_t max_filesize_bytes)
{
    ALOGV("setMaxFileSize");
	if(mMediaRecorder == NULL) {
		ALOGE("<F:%s,L:%d>MediaRecorder not initialize yet!", __FUNCTION__, __LINE__);
		return NO_INIT;
	}

	return mMediaRecorder->setMaxFileSize(max_filesize_bytes);
}

status_t MediaRecorder::setImpactOutputFile(int fd, int64_t fallocateLength, int muxerId)
{
    ALOGV("setImpactOutputFile");
	if(mMediaRecorder == NULL) {
		ALOGE("<F:%s,L:%d>MediaRecorder not initialize yet!", __FUNCTION__, __LINE__);
		return NO_INIT;
	}
	return mMediaRecorder->setImpactOutputFile(fd, fallocateLength, muxerId);
}

status_t MediaRecorder::setImpactOutputFile(const char* path, int64_t fallocateLength, int muxerId)
{
    ALOGV("setImpactOutputFile(%s)", path);
	if(mMediaRecorder == NULL) {
		ALOGE("<F:%s,L:%d>MediaRecorder not initialize yet!", __FUNCTION__, __LINE__);
		return NO_INIT;
	}
	return mMediaRecorder->setImpactOutputFile(path, fallocateLength, muxerId);
}

}; // namespace android
