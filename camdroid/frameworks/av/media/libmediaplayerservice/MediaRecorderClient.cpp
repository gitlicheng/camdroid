/*
 ** Copyright 2008, The Android Open Source Project
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
#define LOG_TAG "MediaRecorderService"
#include <utils/Log.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <cutils/atomic.h>
#include <cutils/properties.h> // for property_get
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/MemoryHeapBase.h>
#include <binder/MemoryBase.h>

#include <utils/String16.h>

#include <system/audio.h>

#include "MediaRecorderClient.h"
#include "MediaPlayerService.h"

#include "CedarXRecorder.h"
//#include "StagefrightRecorder.h"
//#include <gui/ISurfaceTexture.h>

namespace android {

const char* cameraPermission = "android.permission.CAMERA";
const char* recordAudioPermission = "android.permission.RECORD_AUDIO";

static bool checkPermission(const char* permissionString) {
#ifndef HAVE_ANDROID_OS
    return true;
#endif
    if (getpid() == IPCThreadState::self()->getCallingPid()) return true;
    bool ok = checkCallingPermission(String16(permissionString));
    if (!ok) ALOGE("Request requires %s", permissionString);
    return ok;
}


unsigned int MediaRecorderClient::querySurfaceMediaSource()
{
    ALOGV("Query SurfaceMediaSource");
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return 0;
    }
    return mRecorder->querySurfaceMediaSource();
}

status_t MediaRecorderClient::queueBuffer(int index, int addr_y, int addr_c, int64_t timestamp)
{
    ALOGV("queueBuffer");
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->queueBuffer(index, addr_y, addr_c, timestamp);
}

status_t MediaRecorderClient::setCamera(const sp<ICamera>& camera,
                                        const sp<ICameraRecordingProxy>& proxy)
{
    ALOGV("setCamera");
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->setCamera(camera, proxy);
}

//status_t MediaRecorderClient::setPreviewSurface(const sp<Surface>& surface)
status_t MediaRecorderClient::setPreviewSurface(const unsigned int hlay)
{
    ALOGV("setPreviewSurface");
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->setPreviewSurface(hlay);
}

status_t MediaRecorderClient::setVideoSource(int vs)
{
    ALOGV("setVideoSource(%d)", vs);
    if (!checkPermission(cameraPermission)) {
        return PERMISSION_DENIED;
    }
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL)	{
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->setVideoSource((video_source)vs);
}

status_t MediaRecorderClient::setAudioSource(int as)
{
    ALOGV("setAudioSource(%d)", as);
    if (!checkPermission(recordAudioPermission)) {
        return PERMISSION_DENIED;
    }
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL)  {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->setAudioSource((audio_source_t)as);
}

status_t MediaRecorderClient::setOutputFormat(int of)
{
    ALOGV("setOutputFormat(%d)", of);
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->setOutputFormat((output_format)of);
}

status_t MediaRecorderClient::setVideoEncoder(int ve)
{
    ALOGV("setVideoEncoder(%d)", ve);
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->setVideoEncoder((video_encoder)ve);
}

status_t MediaRecorderClient::setAudioEncoder(int ae)
{
    ALOGV("setAudioEncoder(%d)", ae);
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->setAudioEncoder((audio_encoder)ae);
}

status_t MediaRecorderClient::setOutputFile(const char* path)
{
    ALOGV("setOutputFile(%s)", path);
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->setOutputFile(path);
}

status_t MediaRecorderClient::setOutputFile(const char* path, int64_t offset, int64_t length)
{
    ALOGV("setOutputFile(%s, %lld, %lld)", path, offset, length);
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->setOutputFile(path, offset, length);
}

status_t MediaRecorderClient::setOutputFile(int fd, int64_t offset, int64_t length)
{
    ALOGV("setOutputFile(%d, %lld, %lld)", fd, offset, length);
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->setOutputFile(fd, offset, length);
}

status_t MediaRecorderClient::setVideoSize(int width, int height)
{
    ALOGV("setVideoSize(%dx%d)", width, height);
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->setVideoSize(width, height);
}

status_t MediaRecorderClient::setVideoFrameRate(int frames_per_second)
{
    ALOGV("setVideoFrameRate(%d)", frames_per_second);
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->setVideoFrameRate(frames_per_second);
}

status_t MediaRecorderClient::setMuteMode(bool mute)
{
    ALOGV("setMuteMode(%d)", mute);
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->setMuteMode(mute);
}

status_t MediaRecorderClient::setParameters(const String8& params) {
    ALOGV("setParameters(%s)", params.string());
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->setParameters(params);
}

status_t MediaRecorderClient::prepare()
{
    ALOGV("prepare");
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->prepare();
}


status_t MediaRecorderClient::getMaxAmplitude(int* max)
{
    ALOGV("getMaxAmplitude");
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->getMaxAmplitude(max);
}

sp<IMemory> MediaRecorderClient::getOneBsFrame(int mode)
{
    ALOGV("getOneBsFrame");
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NULL;
    }
    return mRecorder->getOneBsFrame(mode);
}

void MediaRecorderClient::freeOneBsFrame()
{
    ALOGV("freeOneBsFrame");
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return;
    }
    mRecorder->freeOneBsFrame();
}

sp<IMemory> MediaRecorderClient::getEncDataHeader()
{
    ALOGV("getEncDataHeader");
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NULL;
    }
    return mRecorder->getEncDataHeader();
}

status_t MediaRecorderClient::setVideoEncodingBitRateSync(int bitRate)
{
    ALOGV("setVideoEncodingBitRateSync(%d)", bitRate);
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
	return mRecorder->setVideoEncodingBitRateSync(bitRate);
}

status_t MediaRecorderClient::setVideoFrameRateSync(int frames_per_second)
{
    ALOGV("setVideoFrameRateSync(%d)", frames_per_second);
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
	return mRecorder->setVideoFrameRateSync(frames_per_second);
}

status_t MediaRecorderClient::start()
{
    ALOGV("start");
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->start();

}

status_t MediaRecorderClient::stop()
{
    ALOGV("stop");
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->stop();
}

status_t MediaRecorderClient::init()
{
    ALOGV("init");
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->init();
}

status_t MediaRecorderClient::close()
{
    ALOGV("close");
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->close();
}


status_t MediaRecorderClient::reset()
{
    ALOGV("reset");
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->reset();
}

status_t MediaRecorderClient::release()
{
    ALOGV("release");
    Mutex::Autolock lock(mLock);
    if (mRecorder != NULL) {
        delete mRecorder;
        mRecorder = NULL;
        wp<MediaRecorderClient> client(this);
        mMediaPlayerService->removeMediaRecorderClient(client);
    }
    return NO_ERROR;
}

MediaRecorderClient::MediaRecorderClient(const sp<MediaPlayerService>& service, pid_t pid)
{
    ALOGV("Client constructor");
    mPid = pid;
    //mRecorder = new StagefrightRecorder;
    mRecorder = new CedarXRecorder;
    mMediaPlayerService = service;
}

MediaRecorderClient::~MediaRecorderClient()
{
    ALOGV("Client destructor");
    release();
}

status_t MediaRecorderClient::setListener(const sp<IMediaRecorderClient>& listener)
{
    ALOGV("setListener");
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->setListener(listener);
}

status_t MediaRecorderClient::dump(int fd, const Vector<String16>& args) const {
    if (mRecorder != NULL) {
        return mRecorder->dump(fd, args);
    }
    return OK;
}

status_t MediaRecorderClient::setVideoEncodingIFramesNumberIntervalSync(int nMaxKeyItl)
{
    ALOGV("setVideoEncodingIFramesNumberIntervalSync(%d)", nMaxKeyItl);
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
	return mRecorder->setVideoEncodingIFramesNumberIntervalSync(nMaxKeyItl);
}

status_t MediaRecorderClient::reencodeIFrame()
{
    ALOGV("reencodeIFrame");
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
	return mRecorder->reencodeIFrame();
}

status_t MediaRecorderClient::setOutputFileSync(int fd, int64_t fallocateLength, int muxerId)
{
    ALOGV("setOutputFileSync(%d, %lld, %d)", fd, fallocateLength, muxerId);
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->setOutputFileSync(fd, fallocateLength, muxerId);
}

status_t MediaRecorderClient::setOutputFileSync(const char* path, int64_t fallocateLength, int muxerId)
{
    ALOGV("setOutputFileSync(%s, %lld, %d)", path, fallocateLength, muxerId);
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->setOutputFileSync(path, fallocateLength, muxerId);
}

status_t MediaRecorderClient::setSdcardState(bool bExist)
{
    ALOGV("setSdcardState(%d)", bExist);
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
	return mRecorder->setSdcardState(bExist);
}

int MediaRecorderClient::addOutputFormatAndOutputSink(int of, int fd, int FallocateLen, bool callback_out_flag)
{
    ALOGV("addOutputFormatAndOutputSink(of:%d, fd:%d, FallocateLen:%d, callback_out_flag:%d)", of, fd, FallocateLen, callback_out_flag);
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->addOutputFormatAndOutputSink(of, fd, FallocateLen, callback_out_flag);
}

int MediaRecorderClient::addOutputFormatAndOutputSink(int of, const char* path, int FallocateLen, bool callback_out_flag)
{
    ALOGV("addOutputFormatAndOutputSink(of:%d, path:%s, FallocateLen:%d, callback_out_flag:%d)", of, path, FallocateLen, callback_out_flag);
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->addOutputFormatAndOutputSink(of, path, FallocateLen, callback_out_flag);
}

status_t MediaRecorderClient::removeOutputFormatAndOutputSink(int muxerId)
{
    ALOGV("removeOutputFormatAndOutputSink(of:%d, sink_type:%d)", of, sink_type);
    Mutex::Autolock lock(mLock);
    if (mRecorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return mRecorder->removeOutputFormatAndOutputSink(muxerId);
}

status_t MediaRecorderClient::setMaxDuration(int max_duration_ms)
{
    ALOGV("setMaxDuration");
    Mutex::Autolock lock(mLock);
	if(mRecorder == NULL) {
		ALOGE("<F:%s,L:%d>MediaRecorder not initialize yet!", __FUNCTION__, __LINE__);
		return NO_INIT;
	}

	return mRecorder->setMaxDuration(max_duration_ms);
}

status_t MediaRecorderClient::setMaxFileSize(int64_t max_filesize_bytes)
{
    ALOGV("setMaxFileSize");
    Mutex::Autolock lock(mLock);
	if(mRecorder == NULL) {
		ALOGE("<F:%s,L:%d>MediaRecorder not initialize yet!", __FUNCTION__, __LINE__);
		return NO_INIT;
	}

	return mRecorder->setMaxFileSize(max_filesize_bytes);
}

status_t MediaRecorderClient::setImpactOutputFile(int fd, int64_t fallocateLength, int muxerId)
{
    ALOGV("setImpactOutputFile");
    if (mRecorder == NULL) {
        ALOGE("<F:%s,L:%d>recorder is not initialized", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
	return mRecorder->setImpactOutputFile(fd, fallocateLength, muxerId);
}

status_t MediaRecorderClient::setImpactOutputFile(const char* path, int64_t fallocateLength, int muxerId)
{
    ALOGV("setImpactOutputFile(%s)", path);
    if (mRecorder == NULL) {
        ALOGE("<F:%s,L:%d>recorder is not initialized", __FUNCTION__, __LINE__);
        return NO_INIT;
    }
	return mRecorder->setImpactOutputFile(path, fallocateLength, muxerId);
}

}; // namespace android
