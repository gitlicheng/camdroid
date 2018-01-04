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

#ifndef _HERBCAMCORDERPROFILE_H_
#define _HERBCAMCORDERPROFILE_H_
 
//#include <utils/Mutex.h>
//#include <utils/Vector.h>
//#include <utils/KeyedVector.h>
//#include <utils/String8.h>
 
 //#include <gui/SurfaceTexture.h>
 //#include <gui/Surface.h>
 //#include <camera/Camera.h>
 //#include <binder/IMemory.h>
 
//#include <include_media/media/MediaCallbackDispatcher.h>
 
namespace android {

class HerbCamcorderProfile
{
public:
    // Do not change these values/ordinals without updating their counterpart
    // in include/media/MediaProfiles.h!
    enum
    {
        /**
         * Quality level corresponding to the lowest available resolution.
         */
        QUALITY_LOW  = 0,

        /**
         * Quality level corresponding to the highest available resolution.
         */
        QUALITY_HIGH = 1,

        /**
         * Quality level corresponding to the qcif (176 x 144) resolution.
         */
        QUALITY_QCIF = 2,

        /**
         * Quality level corresponding to the cif (352 x 288) resolution.
         */
        QUALITY_CIF = 3,

        /**
         * Quality level corresponding to the 480p (720 x 480) resolution.
         * Note that the horizontal resolution for 480p can also be other
         * values, such as 640 or 704, instead of 720.
         */
        QUALITY_480P = 4,

        /**
         * Quality level corresponding to the 720p (1280 x 720) resolution.
         */
        QUALITY_720P = 5,

        /**
         * Quality level corresponding to the 1080p (1920 x 1080) resolution.
         * Note that the vertical resolution for 1080p can also be 1088,
         * instead of 1080 (used by some vendors to avoid cropping during
         * video playback).
         */
        QUALITY_1080P = 6,

        /**
         * Quality level corresponding to the QVGA (320x240) resolution.
         */
        QUALITY_QVGA = 7,

        // Start and end of quality list
        QUALITY_LIST_START = QUALITY_LOW,
        QUALITY_LIST_END = QUALITY_QVGA,

        /**
         * Time lapse quality level corresponding to the lowest available resolution.
         */
        QUALITY_TIME_LAPSE_LOW  = 1000,

        /**
         * Time lapse quality level corresponding to the highest available resolution.
         */
        QUALITY_TIME_LAPSE_HIGH = 1001,

        /**
         * Time lapse quality level corresponding to the qcif (176 x 144) resolution.
         */
        QUALITY_TIME_LAPSE_QCIF = 1002,

        /**
         * Time lapse quality level corresponding to the cif (352 x 288) resolution.
         */
        QUALITY_TIME_LAPSE_CIF = 1003,

        /**
         * Time lapse quality level corresponding to the 480p (720 x 480) resolution.
         */
        QUALITY_TIME_LAPSE_480P = 1004,

        /**
         * Time lapse quality level corresponding to the 720p (1280 x 720) resolution.
         */
        QUALITY_TIME_LAPSE_720P = 1005,

        /**
         * Time lapse quality level corresponding to the 1080p (1920 x 1088) resolution.
         */
        QUALITY_TIME_LAPSE_1080P = 1006,

        /**
         * Time lapse quality level corresponding to the QVGA (320 x 240) resolution.
         */
        QUALITY_TIME_LAPSE_QVGA = 1007,

        // Start and end of timelapse quality list
        QUALITY_TIME_LAPSE_LIST_START = QUALITY_TIME_LAPSE_LOW,
        QUALITY_TIME_LAPSE_LIST_END = QUALITY_TIME_LAPSE_QVGA,
    };
    /**
     * Default recording duration in seconds before the session is terminated.
     * This is useful for applications like MMS has limited file size requirement.
     */
    int duration;

    /**
     * The quality level of the camcorder profile
     */
    int quality;

    /**
     * The file output format of the camcorder profile
     * @see android.media.MediaRecorder.OutputFormat
     */
    int fileFormat;

    /**
     * The video encoder being used for the video track
     * @see android.media.MediaRecorder.VideoEncoder
     */
    int videoCodec;

    /**
     * The target video output bit rate in bits per second
     */
    int videoBitRate;

    /**
     * The target video frame rate in frames per second
     */
    int videoFrameRate;

    /**
     * The target video frame width in pixels
     */
    int videoFrameWidth;

    /**
     * The target video frame height in pixels
     */
    int videoFrameHeight;

    /**
     * The audio encoder being used for the audio track.
     * @see android.media.MediaRecorder.AudioEncoder
     */
    int audioCodec;

    /**
     * The target audio output bit rate in bits per second
     */
    int audioBitRate;

    /**
     * The audio sampling rate used for the audio track
     */
    int audioSampleRate;

    /**
     * The number of audio channels used for the audio track
     */
    int audioChannels;

    /**
     * Returns the camcorder profile for the first back-facing camera on the
     * device at the given quality level. If the device has no back-facing
     * camera, this returns null.
     * @param quality the target quality level for the camcorder profile
     * @see #get(int, int)
     */
    static HerbCamcorderProfile* get(int quality);

    /**
     * Returns the camcorder profile for the given camera at the given
     * quality level.
     *
     * Quality levels QUALITY_LOW, QUALITY_HIGH are guaranteed to be supported, while
     * other levels may or may not be supported. The supported levels can be checked using
     * {@link #hasProfile(int, int)}.
     * QUALITY_LOW refers to the lowest quality available, while QUALITY_HIGH refers to
     * the highest quality available.
     * QUALITY_LOW/QUALITY_HIGH have to match one of qcif, cif, 480p, 720p, or 1080p.
     * E.g. if the device supports 480p, 720p, and 1080p, then low is 480p and high is
     * 1080p.
     *
     * The same is true for time lapse quality levels, i.e. QUALITY_TIME_LAPSE_LOW,
     * QUALITY_TIME_LAPSE_HIGH are guaranteed to be supported and have to match one of
     * qcif, cif, 480p, 720p, or 1080p.
     *
     * A camcorder recording session with higher quality level usually has higher output
     * bit rate, better video and/or audio recording quality, larger video frame
     * resolution and higher audio sampling rate, etc, than those with lower quality
     * level.
     *
     * @param cameraId the id for the camera
     * @param quality the target quality level for the camcorder profile.
     * @see #QUALITY_LOW
     * @see #QUALITY_HIGH
     * @see #QUALITY_QCIF
     * @see #QUALITY_CIF
     * @see #QUALITY_480P
     * @see #QUALITY_720P
     * @see #QUALITY_1080P
     * @see #QUALITY_TIME_LAPSE_LOW
     * @see #QUALITY_TIME_LAPSE_HIGH
     * @see #QUALITY_TIME_LAPSE_QCIF
     * @see #QUALITY_TIME_LAPSE_CIF
     * @see #QUALITY_TIME_LAPSE_480P
     * @see #QUALITY_TIME_LAPSE_720P
     * @see #QUALITY_TIME_LAPSE_1080P
     */
    static HerbCamcorderProfile* get(int cameraId, int quality);

    /**
     * Returns true if camcorder profile exists for the first back-facing
     * camera at the given quality level.
     * @param quality the target quality level for the camcorder profile
     */
    static bool hasProfile(int quality);

    /**
     * Returns true if camcorder profile exists for the given camera at
     * the given quality level.
     * @param cameraId the id for the camera
     * @param quality the target quality level for the camcorder profile
     */
    static bool hasProfile(int cameraId, int quality);
    
private:
    // Private constructor called by JNI
    HerbCamcorderProfile(int duration,
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
                             int audioChannels);
};

};

#endif  /* _HERBCAMCORDERPROFILE_H_ */

