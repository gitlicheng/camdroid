/*
 * Copyright (C) 2010 The Android Open Source Project
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

/**
 * Retrieves the
 * predefined camcorder profile settings for camcorder applications.
 * These settings are read-only.
 *
 * <p>The compressed output from a recording session with a given
 * CamcorderProfile contains two tracks: one for audio and one for video.
 *
 * <p>Each profile specifies the following set of parameters:
 * <ul>
 * <li> The file output format
 * <li> Video codec format
 * <li> Video bit rate in bits per second
 * <li> Video frame rate in frames per second
 * <li> Video frame width and height,
 * <li> Audio codec format
 * <li> Audio bit rate in bits per second,
 * <li> Audio sample rate
 * <li> Number of audio channels for recording.
 * </ul>
 */
 
//#define LOG_NDEBUG 0
#define LOG_TAG "HerbCamcorderProfile"
#include <utils/Log.h>

#include <stdio.h>
#include <utils/threads.h>

//#include "jni.h"
//#include "JNIHelp.h"
//#include "android_runtime/AndroidRuntime.h"
#include <media/MediaProfiles.h>

#include <utils/String8.h>

#include <HerbCamera.h>
#include <HerbCamcorderProfile.h>

namespace android {

static Mutex sLock;

static MediaProfiles* MediaProfiles_native_init()
{
    ALOGV("native_init");
    Mutex::Autolock lock(sLock);
    return MediaProfiles::getInstance();
}
static MediaProfiles *sProfiles = MediaProfiles_native_init();

static bool isCamcorderQualityKnown(int quality)
{
    return ((quality >= CAMCORDER_QUALITY_LIST_START &&
             quality <= CAMCORDER_QUALITY_LIST_END) ||
            (quality >= CAMCORDER_QUALITY_TIME_LAPSE_LIST_START &&
             quality <= CAMCORDER_QUALITY_TIME_LAPSE_LIST_END));
}
HerbCamcorderProfile* HerbCamcorderProfile::get(int quality)
{
    int numberOfCameras = HerbCamera::getNumberOfCameras();
    HerbCamera::CameraInfo *pCameraInfo = new HerbCamera::CameraInfo();
    for (int i = 0; i < numberOfCameras; i++) {
        HerbCamera::getCameraInfo(i, pCameraInfo);
        if (pCameraInfo->facing == HerbCamera::CameraInfo::CAMERA_FACING_BACK) {
            return get(i, quality);
        }
    }
    return NULL;
}

HerbCamcorderProfile* HerbCamcorderProfile::get(int cameraId, int quality)
{
    if (!((quality >= QUALITY_LIST_START &&
           quality <= QUALITY_LIST_END) ||
          (quality >= QUALITY_TIME_LAPSE_LIST_START &&
           quality <= QUALITY_TIME_LAPSE_LIST_END))) {
        char str[16];
        sprintf(str, "%d", quality);
        String8 errMessage = String8("Unsupported quality level: ") + str;
        //throw new IllegalArgumentException(errMessage);
        ALOGE("(f:%s, l:%d) %s", __FUNCTION__, __LINE__, errMessage.string());
    }

    ALOGV("native_get_camcorder_profile: %d %d", cameraId, quality);
    if (!isCamcorderQualityKnown(quality)) {
        //jniThrowException(env, "java/lang/RuntimeException", "Unknown camcorder profile quality");
        ALOGE("(f:%s, l:%d) Unknown camcorder profile quality", __FUNCTION__, __LINE__);
        return NULL;
    }

    camcorder_quality q = static_cast<camcorder_quality>(quality);
    int duration         = sProfiles->getCamcorderProfileParamByName("duration",    cameraId, q);
    int fileFormat       = sProfiles->getCamcorderProfileParamByName("file.format", cameraId, q);
    int videoCodec       = sProfiles->getCamcorderProfileParamByName("vid.codec",   cameraId, q);
    int videoBitRate     = sProfiles->getCamcorderProfileParamByName("vid.bps",     cameraId, q);
    int videoFrameRate   = sProfiles->getCamcorderProfileParamByName("vid.fps",     cameraId, q);
    int videoFrameWidth  = sProfiles->getCamcorderProfileParamByName("vid.width",   cameraId, q);
    int videoFrameHeight = sProfiles->getCamcorderProfileParamByName("vid.height",  cameraId, q);
    int audioCodec       = sProfiles->getCamcorderProfileParamByName("aud.codec",   cameraId, q);
    int audioBitRate     = sProfiles->getCamcorderProfileParamByName("aud.bps",     cameraId, q);
    int audioSampleRate  = sProfiles->getCamcorderProfileParamByName("aud.hz",      cameraId, q);
    int audioChannels    = sProfiles->getCamcorderProfileParamByName("aud.ch",      cameraId, q);

    // Check on the values retrieved
    if (duration == -1 || fileFormat == -1 || videoCodec == -1 || audioCodec == -1 ||
        videoBitRate == -1 || videoFrameRate == -1 || videoFrameWidth == -1 || videoFrameHeight == -1 ||
        audioBitRate == -1 || audioSampleRate == -1 || audioChannels == -1) {

        //jniThrowException(env, "java/lang/RuntimeException", "Error retrieving camcorder profile params");
        ALOGE("(f:%s, l:%d) Error retrieving camcorder profile params", __FUNCTION__, __LINE__);
        return NULL;
    }

    return new HerbCamcorderProfile(duration,
                          quality,
                          fileFormat,
                          videoCodec,
                          videoBitRate,
                          videoFrameRate,
                          videoFrameWidth,
                          videoFrameHeight,
                          audioCodec,
                          audioBitRate,
                          audioSampleRate,
                          audioChannels);
}
bool HerbCamcorderProfile::hasProfile(int quality)
{
    int numberOfCameras = HerbCamera::getNumberOfCameras();
    HerbCamera::CameraInfo *pCameraInfo = new HerbCamera::CameraInfo();
    for (int i = 0; i < numberOfCameras; i++) {
        HerbCamera::getCameraInfo(i, pCameraInfo);
        if (pCameraInfo->facing == HerbCamera::CameraInfo::CAMERA_FACING_BACK) {
            return hasProfile(i, quality);
        }
    }
    return false;
}

bool HerbCamcorderProfile::hasProfile(int cameraId, int quality)
{
    ALOGV("native_has_camcorder_profile: %d %d", cameraId, quality);
    if (!isCamcorderQualityKnown(quality)) {
        return false;
    }

    camcorder_quality q = static_cast<camcorder_quality>(quality);
    return sProfiles->hasCamcorderProfile(cameraId, q);
}

HerbCamcorderProfile::HerbCamcorderProfile(int duration,
                             int quality,
                             int fileFormat,
                             int videoCodec,
                             int videoBitRate,
                             int videoFrameRate,
                             int videoWidth,
                             int videoHeight,
                             int audioCodec,
                             int audioBitRate,
                             int audioSampleRate,
                             int audioChannels) 
{
    this->duration         = duration;
    this->quality          = quality;
    this->fileFormat       = fileFormat;
    this->videoCodec       = videoCodec;
    this->videoBitRate     = videoBitRate;
    this->videoFrameRate   = videoFrameRate;
    this->videoFrameWidth  = videoWidth;
    this->videoFrameHeight = videoHeight;
    this->audioCodec       = audioCodec;
    this->audioBitRate     = audioBitRate;
    this->audioSampleRate  = audioSampleRate;
    this->audioChannels    = audioChannels;
}

};

