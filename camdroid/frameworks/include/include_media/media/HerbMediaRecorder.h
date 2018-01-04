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

/**
 * Used to record audio and video. The recording control is based on a
 * simple state machine (see below).
 *
 * <p><img src="{@docRoot}images/mediarecorder_state_diagram.gif" border="0" />
 * </p>
 *
 * <p>A common case of using MediaRecorder to record audio works as follows:
 *
 * <pre>MediaRecorder recorder = new MediaRecorder();
 * recorder.setAudioSource(MediaRecorder.AudioSource.MIC);
 * recorder.setOutputFormat(MediaRecorder.OutputFormat.THREE_GPP);
 * recorder.setAudioEncoder(MediaRecorder.AudioEncoder.AMR_NB);
 * recorder.setOutputFile(PATH_NAME);
 * recorder.prepare();
 * recorder.start();   // Recording is now started
 * ...
 * recorder.stop();
 * recorder.reset();   // You can reuse the object by going back to setAudioSource() step
 * recorder.release(); // Now the object cannot be reused
 * </pre>
 *
 * <p>Applications may want to register for informational and error
 * events in order to be informed of some internal update and possible
 * runtime errors during recording. Registration for such events is
 * done by setting the appropriate listeners (via calls
 * (to {@link #setOnInfoListener(OnInfoListener)}setOnInfoListener and/or
 * {@link #setOnErrorListener(OnErrorListener)}setOnErrorListener).
 * In order to receive the respective callback associated with these listeners,
 * applications are required to create MediaRecorder objects on threads with a
 * Looper running (the main UI thread by default already has a Looper running).
 *
 * <p><strong>Note:</strong> Currently, MediaRecorder does not work on the emulator.
 *
 * <div class="special reference">
 * <h3>Developer Guides</h3>
 * <p>For more information about how to use MediaRecorder for recording video, read the
 * <a href="{@docRoot}guide/topics/media/camera.html#capture-video">Camera</a> developer guide.
 * For more information about how to use MediaRecorder for recording sound, read the
 * <a href="{@docRoot}guide/topics/media/audio-capture.html">Audio Capture</a> developer guide.</p>
 * </div>
 */
 
#ifndef _HERBMEDIARECORDER_H_
#define _HERBMEDIARECORDER_H_

#include <utils/Mutex.h>
#include <utils/Vector.h>
#include <utils/KeyedVector.h>
#include <utils/String8.h>

#include <camera/ICameraService.h>
#include <camera/Camera.h>
#include <media/mediarecorder.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <utils/threads.h>
#include <binder/IMemory.h>

#include <system/audio.h>

#include <MediaCallbackDispatcher.h>
#include <HerbCamcorderProfile.h>

#include <HerbCamera.h>


namespace android {

class VEncBuffer : public virtual RefBase
{
public:
	int total_size;
	int stream_type; /* 0:video, 1:audio data, 128:video data header, 129:audio data header */
	int data_size;
	long long pts;
    //for video frame.
    int CurrQp;
    int avQp;
	int nGopIndex;      //index of gop
	int nFrameIndex;    //index of current frame in gop.
	int nTotalIndex;    //index of current frame in whole encoded frames.
	char *data;
};


class HerbMediaRecorder
{
private:
    class EventHandler;
public:
    class OnErrorListener;
    class OnInfoListener;
    class OnDataListener;
private:
    String8 mPath;
    int mFd;
    sp<EventHandler> mEventHandler;
    OnErrorListener *mOnErrorListener;
    OnInfoListener *mOnInfoListener;
    OnDataListener *mOnDataListener;
	int mNativeContext;
	int mSurface;

	static Mutex msLock;
	int64_t mLength;

	class EventHandler : public MediaCallbackDispatcher
	{
	private:
//		static const int MEDIA_RECORDER_EVENT_LIST_START = 1;
//		static const int MEDIA_RECORDER_EVENT_ERROR      = 1;
//		static const int MEDIA_RECORDER_EVENT_INFO       = 2;
//		static const int MEDIA_RECORDER_EVENT_LIST_END   = 99;
//		static const int MEDIA_RECORDER_TRACK_EVENT_LIST_START = 100;
//		static const int MEDIA_RECORDER_TRACK_EVENT_ERROR      = 100;
//		static const int MEDIA_RECORDER_TRACK_EVENT_INFO       = 101;
//		static const int MEDIA_RECORDER_TRACK_EVENT_LIST_END   = 1000;

		HerbMediaRecorder *mpMediaRecorder;

	public:
		EventHandler(HerbMediaRecorder *pC);
		~EventHandler();
		virtual void handleMessage(const MediaCallbackMessage &msg);
	};

	class HerbMediaRecorderListener: public MediaRecorderListener
	{
	private:
//		EventHandler *mEventHandler;
        HerbMediaRecorder *mpMediaRecorder;
	public:
		HerbMediaRecorderListener(HerbMediaRecorder *pMr);
		~HerbMediaRecorderListener();
		void notify(int msg, int ext1, int ext2);
	};

	sp<MediaRecorder> setMediaRecorder(const sp<MediaRecorder>& recorder);
	sp<MediaRecorder> getMediaRecorder();
	bool process_media_recorder_call(status_t opStatus, const char* message);


public:
    /**
     * Default constructor.
     */
    HerbMediaRecorder();
	~HerbMediaRecorder();
    /**
     * Sets a Camera to use for recording. Use this function to switch
     * quickly between preview and capture mode without a teardown of
     * the camera object. {@link android.hardware.Camera#unlock()} should be
     * called before this. Must call before prepare().
     *
     * @param c the Camera to use for recording
     */
    status_t setCamera(HerbCamera *pC);

    /**
     * Sets a Surface to show a preview of recorded media (video). Calls this
     * before prepare() to make sure that the desirable preview display is
     * set. If {@link #setCamera(Camera)} is used and the surface has been
     * already set to the camera, application do not need to call this. If
     * this is called with non-null surface, the preview surface of the camera
     * will be replaced by the new surface. If this method is called with null
     * surface or not called at all, media recorder will not change the preview
     * surface of the camera.
     *
     * @param sv the Surface to use for the preview
     * @see android.hardware.Camera#setPreviewDisplay(android.view.SurfaceHolder)
     */
    void setPreviewDisplay(int hlay);

    /**
     * Defines the audio source. These constants are used with
     * {@link MediaRecorder#setAudioSource(int)}.
     */
    class AudioSource {
      /* Do not change these values without updating their counterparts
       * in system/core/include/system/audio.h!
       */
    private:
        AudioSource() {};
    public:

        /** Default audio source **/
        static const int DEFAULT = 0;

        /** Microphone audio source */
        static const int MIC = 1;

        /** Voice call uplink (Tx) audio source */
        static const int VOICE_UPLINK = 2;

        /** Voice call downlink (Rx) audio source */
        static const int VOICE_DOWNLINK = 3;

        /** Voice call uplink + downlink audio source */
        static const int VOICE_CALL = 4;

        /** Microphone audio source with same orientation as camera if available, the main
         *  device microphone otherwise */
        static const int CAMCORDER = 5;

        /** Microphone audio source tuned for voice recognition if available, behaves like
         *  {@link #DEFAULT} otherwise. */
        static const int VOICE_RECOGNITION = 6;

        /** Microphone audio source tuned for voice communications such as VoIP. It
         *  will for instance take advantage of echo cancellation or automatic gain control
         *  if available. It otherwise behaves like {@link #DEFAULT} if no voice processing
         *  is applied.
         */
        static const int VOICE_COMMUNICATION = 7;

        /**
         * @hide
         * Audio source for remote submix.
         */
        static const int REMOTE_SUBMIX_SOURCE = 8;
		
      /** FM audio source */
        static const int VOICE_FM = 9;
    };
    /**
     * Defines the video source. These constants are used with
     * {@link MediaRecorder#setVideoSource(int)}.
     */
    class VideoSource {
      /* Do not change these values without updating their counterparts
       * in include/media/mediarecorder.h!
       */
    private:
        VideoSource() {};
    public:
        static const int DEFAULT = 0;
        /** Camera video source */
        static const int CAMERA = 1;
        /** @hide */
        static const int GRALLOC_BUFFER = 2;
    };
    /**
     * Defines the output format. These constants are used with
     * {@link MediaRecorder#setOutputFormat(int)}.
     */
    class OutputFormat {
      /* Do not change these values without updating their counterparts
       * in include/media/mediarecorder.h!
       */
    private:
        OutputFormat() {};
    public:
        static const int DEFAULT = 0;
        /** 3GPP media file format*/
        static const int THREE_GPP = 1;
        /** MPEG4 media file format*/
        static const int MPEG_4 = 2;

        /** The following formats are audio only .aac or .amr formats */

        /**
         * AMR NB file format
         * @deprecated  Deprecated in favor of MediaRecorder.OutputFormat.AMR_NB
         */
        static const int RAW_AMR = 3;

        /** AMR NB file format */
        static const int AMR_NB = 3;

        /** AMR WB file format */
        static const int AMR_WB = 4;

        /** @hide AAC ADIF file format */
        static const int AAC_ADIF = 5;

        /** AAC ADTS file format */
        static const int AAC_ADTS = 6;

        /** @hide Stream over a socket, limited to a single stream */
        static const int OUTPUT_FORMAT_RTP_AVP = 7;

        /** @hide H.264/AAC data encapsulated in MPEG2/TS */
        static const int OUTPUT_FORMAT_MPEG2TS = 8;
    	static const int OUTPUT_FORMAT_AWTS    = 9;
    	static const int OUTPUT_FORMAT_RAW     = 10;
        static const int OUTPUT_FORMAT_MP3     = 11;
    };
    /**
     * Defines the output sink type. These constants are used with
     * {@link MediaRecorder#removeOutputFormatAndOutputSink(int, int)}.
     */
    class OutputSinkType {
      /* Do not change these values without updating their counterparts
       * in include/media/mediarecorder.h!
       */
    private:
        OutputSinkType() {};
    public:
        /** output to file*/
        static const int SINKTYPE_FILE = 1;
        /** output to app*/
        static const int SINKTYPE_CALLBACKOUT = 2;
        /** output to file and app*/
        static const int SINKTYPE_ALL = 3;
    };
    /**
     * Defines the audio encoding. These constants are used with
     * {@link MediaRecorder#setAudioEncoder(int)}.
     */
    class AudioEncoder {
      /* Do not change these values without updating their counterparts
       * in include/media/mediarecorder.h!
       */
    private:
        AudioEncoder() {};
    public:
        static const int DEFAULT = 0;
        /** AMR (Narrowband) audio codec */
        static const int AMR_NB = 1;
        /** AMR (Wideband) audio codec */
        static const int AMR_WB = 2;
        /** AAC Low Complexity (AAC-LC) audio codec */
        static const int AAC = 3;
        /** High Efficiency AAC (HE-AAC) audio codec */
        static const int HE_AAC = 4;
        /** Enhanced Low Delay AAC (AAC-ELD) audio codec */
        static const int AAC_ELD = 5;
        /** PCM audio codec */
        static const int PCM = 6;
        /** MP3 audio codec */
        static const int MP3 = 7;
        
    };

    /**
     * Defines the video encoding. These constants are used with
     * {@link MediaRecorder#setVideoEncoder(int)}.
     */
    class VideoEncoder {
      /* Do not change these values without updating their counterparts
       * in include/media/mediarecorder.h!
       */
    private:
        VideoEncoder() {};
    public:
        static const int DEFAULT = 0;
        static const int H263 = 1;
        static const int H264 = 2;
        static const int MPEG_4_SP = 3;
    };
    /**
     * Sets the audio source to be used for recording. If this method is not
     * called, the output file will not contain an audio track. The source needs
     * to be specified before setting recording-parameters or encoders. Call
     * this only before setOutputFormat().
     *
     * @param audio_source the audio source to use
     * @throws IllegalStateException if it is called after setOutputFormat()
     * @see android.media.MediaRecorder.AudioSource
     */
    status_t setAudioSource(int audio_source);


    /**
     * Sets the video source to be used for recording. If this method is not
     * called, the output file will not contain an video track. The source needs
     * to be specified before setting recording-parameters or encoders. Call
     * this only before setOutputFormat().
     *
     * @param video_source the video source to use
     * @throws IllegalStateException if it is called after setOutputFormat()
     * @see android.media.MediaRecorder.VideoSource
     */
    status_t setVideoSource(int video_source);

    /**
     * Uses the settings from a CamcorderProfile object for recording. This method should
     * be called after the video AND audio sources are set, and before setOutputFile().
     * If a time lapse CamcorderProfile is used, audio related source or recording
     * parameters are ignored.
     *
     * @param profile the CamcorderProfile to use
     * @see android.media.CamcorderProfile
     */
    void setProfile(HerbCamcorderProfile& profile);
    /**
     * Set video frame capture rate. This can be used to set a different video frame capture
     * rate than the recorded video's playback rate. This method also sets the recording mode
     * to time lapse. In time lapse video recording, only video is recorded. Audio related
     * parameters are ignored when a time lapse recording session starts, if an application
     * sets them.
     *
     * @param fps Rate at which frames should be captured in frames per second.
     * The fps can go as low as desired. However the fastest fps will be limited by the hardware.
     * For resolutions that can be captured by the video camera, the fastest fps can be computed using
     * {@link android.hardware.Camera.Parameters#getPreviewFpsRange(int[])}. For higher
     * resolutions the fastest fps may be more restrictive.
     * Note that the recorder cannot guarantee that frames will be captured at the
     * given rate due to camera/encoder limitations. However it tries to be as close as
     * possible.
     */
    status_t setCaptureRate(double fps);
    /**
     * Sets the orientation hint for output video playback.
     * This method should be called before prepare(). This method will not
     * trigger the source video frame to rotate during video recording, but to
     * add a composition matrix containing the rotation angle in the output
     * video if the output format is OutputFormat.THREE_GPP or
     * OutputFormat.MPEG_4 so that a video player can choose the proper
     * orientation for playback. Note that some video players may choose
     * to ignore the compostion matrix in a video during playback.
     *
     * @param degrees the angle to be rotated clockwise in degrees.
     * The supported angles are 0, 90, 180, and 270 degrees.
     * @throws IllegalArgumentException if the angle is not supported.
     *
     */
    status_t setOrientationHint(int degrees);
    /**
     * Set and store the geodata (latitude and longitude) in the output file.
     * This method should be called before prepare(). The geodata is
     * stored in udta box if the output format is OutputFormat.THREE_GPP
     * or OutputFormat.MPEG_4, and is ignored for other output formats.
     * The geodata is stored according to ISO-6709 standard.
     *
     * @param latitude latitude in degrees. Its value must be in the
     * range [-90, 90].
     * @param longitude longitude in degrees. Its value must be in the
     * range [-180, 180].
     *
     * @throws IllegalArgumentException if the given latitude or
     * longitude is out of range.
     *
     */
    status_t setLocation(float latitude, float longitude);

    /**
     * Sets the format of the output file produced during recording. Call this
     * after setAudioSource()/setVideoSource() but before prepare().
     *
     * <p>It is recommended to always use 3GP format when using the H.263
     * video encoder and AMR audio encoder. Using an MPEG-4 container format
     * may confuse some desktop players.</p>
     *
     * @param output_format the output format to use. The output format
     * needs to be specified before setting recording-parameters or encoders.
     * @throws IllegalStateException if it is called after prepare() or before
     * setAudioSource()/setVideoSource().
     * @see android.media.MediaRecorder.OutputFormat
     */
    status_t setOutputFormat(int output_format);

    /**
     * Sets the width and height of the video to be captured.  Must be called
     * after setVideoSource(). Call this after setOutFormat() but before
     * prepare().
     *
     * @param width the width of the video to be captured
     * @param height the height of the video to be captured
     * @throws IllegalStateException if it is called after
     * prepare() or before setOutputFormat()
     */
    status_t setVideoSize(int width, int height);

    /**
     * Sets the frame rate of the video to be captured.  Must be called
     * after setVideoSource(). Call this after setOutFormat() but before
     * prepare().
     *
     * @param rate the number of frames per second of video to capture
     * @throws IllegalStateException if it is called after
     * prepare() or before setOutputFormat().
     *
     * NOTE: On some devices that have auto-frame rate, this sets the
     * maximum frame rate, not a constant frame rate. Actual frame rate
     * will vary according to lighting conditions.
     */
    status_t setVideoFrameRate(int rate);


    status_t setMuteMode(bool mute);
    /**
     * Sets the maximum duration (in ms) of the recording session.
     * Call this after setOutFormat() but before prepare().
     * After recording reaches the specified duration, a notification
     * will be sent to the {@link android.media.MediaRecorder.OnInfoListener}
     * with a "what" code of {@link #MEDIA_RECORDER_INFO_MAX_DURATION_REACHED}
     * and recording will be stopped. Stopping happens asynchronously, there
     * is no guarantee that the recorder will have stopped by the time the
     * listener is notified.
     *
     * @param max_duration_ms the maximum duration in ms (if zero or negative, disables the duration limit)
     *
     */
     //we change operation of mediaRecord, it will not stop after send a message, until apk command it to stop.
    status_t setMaxDuration(int max_duration_ms);

    /**
     * Sets the maximum filesize (in bytes) of the recording session.
     * Call this after setOutFormat() but before prepare().
     * After recording reaches the specified filesize, a notification
     * will be sent to the {@link android.media.MediaRecorder.OnInfoListener}
     * with a "what" code of {@link #MEDIA_RECORDER_INFO_MAX_FILESIZE_REACHED}
     * and recording will be stopped. Stopping happens asynchronously, there
     * is no guarantee that the recorder will have stopped by the time the
     * listener is notified.
     *
     * @param max_filesize_bytes the maximum filesize in bytes (if zero or negative, disables the limit)
     *
     */
    status_t setMaxFileSize(int64_t max_filesize_bytes);

    /**
     * Sets the audio encoder to be used for recording. If this method is not
     * called, the output file will not contain an audio track. Call this after
     * setOutputFormat() but before prepare().
     *
     * @param audio_encoder the audio encoder to use.
     * @throws IllegalStateException if it is called before
     * setOutputFormat() or after prepare().
     * @see android.media.MediaRecorder.AudioEncoder
     */
    status_t setAudioEncoder(int audio_encoder);

    /**
     * Sets the video encoder to be used for recording. If this method is not
     * called, the output file will not contain an video track. Call this after
     * setOutputFormat() and before prepare().
     *
     * @param video_encoder the video encoder to use.
     * @throws IllegalStateException if it is called before
     * setOutputFormat() or after prepare()
     * @see android.media.MediaRecorder.VideoEncoder
     */
    status_t setVideoEncoder(int video_encoder);

    /**
     * Sets the audio sampling rate for recording. Call this method before prepare().
     * Prepare() may perform additional checks on the parameter to make sure whether
     * the specified audio sampling rate is applicable. The sampling rate really depends
     * on the format for the audio recording, as well as the capabilities of the platform.
     * For instance, the sampling rate supported by AAC audio coding standard ranges
     * from 8 to 96 kHz, the sampling rate supported by AMRNB is 8kHz, and the sampling
     * rate supported by AMRWB is 16kHz. Please consult with the related audio coding
     * standard for the supported audio sampling rate.
     *
     * @param samplingRate the sampling rate for audio in samples per second.
     */
    status_t setAudioSamplingRate(int samplingRate);

    /**
     * Sets the number of audio channels for recording. Call this method before prepare().
     * Prepare() may perform additional checks on the parameter to make sure whether the
     * specified number of audio channels are applicable.
     *
     * @param numChannels the number of audio channels. Usually it is either 1 (mono) or 2
     * (stereo).
     */
    status_t setAudioChannels(int numChannels);

    /**
     * Sets the audio encoding bit rate for recording. Call this method before prepare().
     * Prepare() may perform additional checks on the parameter to make sure whether the
     * specified bit rate is applicable, and sometimes the passed bitRate will be clipped
     * internally to ensure the audio recording can proceed smoothly based on the
     * capabilities of the platform.
     *
     * @param bitRate the audio encoding bit rate in bits per second.
     */
    status_t setAudioEncodingBitRate(int bitRate);

    /**
     * Sets the video encoding bit rate for recording. Call this method before prepare().
     * Prepare() may perform additional checks on the parameter to make sure whether the
     * specified bit rate is applicable, and sometimes the passed bitRate will be
     * clipped internally to ensure the video recording can proceed smoothly based on
     * the capabilities of the platform.
     *
     * @param bitRate the video encoding bit rate in bits per second.
     */
    status_t setVideoEncodingBitRate(int bitRate);
	status_t setVideoEncodingBitRateSync(int bitRate);
	status_t setVideoFrameRateSync(int frames_per_second);

    /**
     * Pass in the file descriptor of the file to be written. Call this after
     * setOutputFormat() but before prepare().
     *
     * @param fd an open file descriptor to be written into.
     * @throws IllegalStateException if it is called before
     * setOutputFormat() or after prepare()
     */
    void setOutputFile(int fd);
	void setOutputFile(int fd, int64_t length);

    /**
     * Sets the path of the output file to be produced. Call this after
     * setOutputFormat() but before prepare().
     *
     * @param path The pathname to use.
     * @throws IllegalStateException if it is called before
     * setOutputFormat() or after prepare()
     */
    void setOutputFile(char* path);
    void setOutputFile(char* path, int64_t length);
    /**
     * Prepares the recorder to begin capturing and encoding data. This method
     * must be called after setting up the desired audio and video sources,
     * encoders, file format, etc., but before start().
     *
     * @throws IllegalStateException if it is called after
     * start() or before setOutputFormat().
     * @throws IOException if prepare fails otherwise.
     */
    status_t prepare();

    /**
     * Begins capturing and encoding data to the file specified with
     * setOutputFile(). Call this after prepare().
     *
     * <p>Since API level 13, if applications set a camera via
     * {@link #setCamera(Camera)}, the apps can use the camera after this method
     * call. The apps do not need to lock the camera again. However, if this
     * method fails, the apps should still lock the camera back. The apps should
     * not start another recording session during recording.
     *
     * @throws IllegalStateException if it is called before
     * prepare().
     */
    status_t start();

    /**
     * Stops recording. Call this after start(). Once recording is stopped,
     * you will have to configure it again as if it has just been constructed.
     * Note that a RuntimeException is intentionally thrown to the
     * application, if no valid audio/video data has been received when stop()
     * is called. This happens if stop() is called immediately after
     * start(). The failure lets the application take action accordingly to
     * clean up the output file (delete the output file, for instance), since
     * the output file is not properly constructed when this happens.
     *
     * @throws IllegalStateException if it is called before start()
     */
    status_t stop();

    /**
     * Restarts the MediaRecorder to its idle state. After calling
     * this method, you will have to configure it again as if it had just been
     * constructed.
     */
    status_t reset();

    /**
     * Returns the maximum absolute amplitude that was sampled since the last
     * call to this method. Call this only after the setAudioSource().
     *
     * @return the maximum absolute amplitude measured since the last call, or
     * 0 when called for the first time
     * @throws IllegalStateException if it is called before
     * the audio source has been set.
     */
    int getMaxAmplitude();

    /* Do not change this value without updating its counterpart
     * in include/media/mediarecorder.h or mediaplayer.h!
     */
    enum
    {
        /** Unspecified media recorder error.
         * @see android.media.MediaRecorder.OnErrorListener
         */
        MEDIA_RECORDER_ERROR_UNKNOWN = 1,
        /** Media server died. In this case, the application must release the
         * MediaRecorder object and instantiate a new one.
         * @see android.media.MediaRecorder.OnErrorListener
         */
        MEDIA_ERROR_SERVER_DIED = 100,
    };

    /**
     * Interface definition for a callback to be invoked when an error
     * occurs while recording.
     */
    class OnErrorListener
    {
    public:
		OnErrorListener()
		{
		}
		virtual ~OnErrorListener()
		{
		}
        /**
         * Called when an error occurs while recording.
         *
         * @param mr the MediaRecorder that encountered the error
         * @param what    the type of error that has occurred:
         * <ul>
         * <li>{@link #MEDIA_RECORDER_ERROR_UNKNOWN}
         * <li>{@link #MEDIA_ERROR_SERVER_DIED}
         * </ul>
         * @param extra   an extra code, specific to the error type
         */
        virtual void onError(HerbMediaRecorder *pMr, int what, int extra) = 0;
    };

    /**
     * Register a callback to be invoked when an error occurs while
     * recording.
     *
     * @param l the callback that will be run
     */
    void setOnErrorListener(OnErrorListener *pl);

    /* Do not change these values without updating their counterparts
     * in include/media/mediarecorder.h!
     */
    enum
    {
        /** Unspecified media recorder error.
         * @see android.media.MediaRecorder.OnInfoListener
         */
        MEDIA_RECORDER_INFO_UNKNOWN              = 1,
        /** A maximum duration had been setup and has now been reached.
         * @see android.media.MediaRecorder.OnInfoListener
         */
        MEDIA_RECORDER_INFO_MAX_DURATION_REACHED = 800,
        /** A maximum filesize had been setup and has now been reached.
         * @see android.media.MediaRecorder.OnInfoListener
         */
        MEDIA_RECORDER_INFO_MAX_FILESIZE_REACHED = 801,

        /** informational events for individual tracks, for testing purpose.
         * The track informational event usually contains two parts in the ext1
         * arg of the onInfo() callback: bit 31-28 contains the track id; and
         * the rest of the 28 bits contains the informational event defined here.
         * For example, ext1 = (1 << 28 | MEDIA_RECORDER_TRACK_INFO_TYPE) if the
         * track id is 1 for informational event MEDIA_RECORDER_TRACK_INFO_TYPE;
         * while ext1 = (0 << 28 | MEDIA_RECORDER_TRACK_INFO_TYPE) if the track
         * id is 0 for informational event MEDIA_RECORDER_TRACK_INFO_TYPE. The
         * application should extract the track id and the type of informational
         * event from ext1, accordingly.
         *
         * FIXME:
         * Please update the comment for onInfo also when these
         * events are unhidden so that application knows how to extract the track
         * id and the informational event type from onInfo callback.
         *
         * {@hide}
         */
        MEDIA_RECORDER_TRACK_INFO_LIST_START        = 1000,
        /** Signal the completion of the track for the recording session.
         * {@hide}
         */
        MEDIA_RECORDER_TRACK_INFO_COMPLETION_STATUS = 1000,
        /** Indicate the recording progress in time (ms) during recording.
         * {@hide}
         */
        MEDIA_RECORDER_TRACK_INFO_PROGRESS_IN_TIME  = 1001,
        /** Indicate the track type: 0 for Audio and 1 for Video.
         * {@hide}
         */
        MEDIA_RECORDER_TRACK_INFO_TYPE              = 1002,
        /** Provide the track duration information.
         * {@hide}
         */
        MEDIA_RECORDER_TRACK_INFO_DURATION_MS       = 1003,
        /** Provide the max chunk duration in time (ms) for the given track.
         * {@hide}
         */
        MEDIA_RECORDER_TRACK_INFO_MAX_CHUNK_DUR_MS  = 1004,
        /** Provide the total number of recordd frames.
         * {@hide}
         */
        MEDIA_RECORDER_TRACK_INFO_ENCODED_FRAMES    = 1005,
        /** Provide the max spacing between neighboring chunks for the given track.
         * {@hide}
         */
        MEDIA_RECORDER_TRACK_INTER_CHUNK_TIME_MS    = 1006,
        /** Provide the elapsed time measuring from the start of the recording
         * till the first output frame of the given track is received, excluding
         * any intentional start time offset of a recording session for the
         * purpose of eliminating the recording sound in the recorded file.
         * {@hide}
         */
        MEDIA_RECORDER_TRACK_INFO_INITIAL_DELAY_MS  = 1007,
        /** Provide the start time difference (delay) betweeen this track and
         * the start of the movie.
         * {@hide}
         */
        MEDIA_RECORDER_TRACK_INFO_START_OFFSET_MS   = 1008,
        /** Provide the total number of data (in kilo-bytes) encoded.
         * {@hide}
         */
        MEDIA_RECORDER_TRACK_INFO_DATA_KBYTES       = 1009,
        /**
         * {@hide}
         */
        MEDIA_RECORDER_TRACK_INFO_LIST_END          = 2000,
    };

    /**
     * Interface definition for a callback to be invoked when an error
     * occurs while recording.
     */
    class OnInfoListener
    {
    public:
		OnInfoListener()
		{
		}
		virtual ~OnInfoListener()
		{
		}
        /**
         * Called when an error occurs while recording.
         *
         * @param mr the MediaRecorder that encountered the error
         * @param what    the type of error that has occurred:
         * <ul>
         * <li>{@link #MEDIA_RECORDER_INFO_UNKNOWN}
         * <li>{@link #MEDIA_RECORDER_INFO_MAX_DURATION_REACHED}
         * <li>{@link #MEDIA_RECORDER_INFO_MAX_FILESIZE_REACHED}
         * </ul>
         * @param extra   an extra code, specific to the error type
         */
        virtual void onInfo(HerbMediaRecorder *pMr, int what, int extra) = 0;
    };

    /**
     * Register a callback to be invoked when an informational event occurs while
     * recording.
     *
     * @param listener the callback that will be run
     */
    void setOnInfoListener(OnInfoListener *pListener);

	/* for recorder data callback */
	class OnDataListener
	{
    public:
		OnDataListener()
		{
		}
		virtual ~OnDataListener()
		{
		}
		virtual void onData(HerbMediaRecorder *pMr, int what, int extra) = 0;
	};
    void setOnDataListener(OnDataListener *pListener);
	sp<VEncBuffer> getOneBsFrame(int mode, sp<IMemory> *frame);
	void freeOneBsFrame(sp<VEncBuffer> recData, sp<IMemory> frame);
	sp<IMemory> getEncDataHeader();
    /** 
     *      AW extend
     * Add the out format and the path of the output file to be produced. The 
     * output sink can be file, or transport out by RTSP. The muxer can support 
     * more than one. Call this before prepare().
     * One output_format is one muxer.
     * Don't call setOutputFormat() and setOutputFile(), when use addOutputFormatAndOutputSink().
     *
     * @return muxerId>=0 if success. muxerId=-1 indicate fail.
     * @param output_format The muxer file format.
     *      @see android.media.MediaRecorder.OutputFormat
     * @param fd The fd of file to use. -1 indicate no fd.
     * @param callback_out_flag Indicate if transport out by RTSP. e.g, "http://mnt/extsd/HVGA_mp4_0.mp4"
     */
    int addOutputFormatAndOutputSink(int output_format, int fd, int FallocateLen, bool callback_out_flag);
    int addOutputFormatAndOutputSink(int output_format, char* path, int FallocateLen, bool callback_out_flag);
    
    
     /* Do not change these values without updating their counterparts
     * in include/media/mediarecorder.h!
     */
    enum
    {
        SINKTYPE_FILE           = 0x01, //one muxer write to sd file.
        SINKTYPE_CALLBACKOUT    = 0x02, //one muxer transport by RTSP
        SINKTYPE_ALL            = 0x03, //one muxer write && transport.
    };
     /** 
     *      AW extend
     * Remove one output_format sink of the muxer. If the muxer of output_format is removed all sink,
     * equivalent to destroy this muxer. If this muxer is the last one, return fail and 
     * don't remove it.
     *
     * @return if success.
     * @param muxerId.
     */
    //status_t removeOutputFormatAndOutputSink(int output_format, int sink_type);
    status_t removeOutputFormatAndOutputSink(int muxerId);
    status_t setOutputFileSync(int fd, int64_t fallocateLength=0, int muxerId=0);
	status_t setOutputFileSync(char* path, int64_t fallocateLength=0, int muxerId=0);
private:
    static void postEventFromNative(HerbMediaRecorder *pMr, int what, int arg1, int arg2, void *obj=NULL);
    status_t setOutputFileFd(int fd, long offset, long length);
public:
    /**
     * Releases resources associated with this MediaRecorder object.
     * It is good practice to call this method when you're done
     * using the MediaRecorder. In particular, whenever an Activity
     * of an application is paused (its onPause() method is called),
     * or stopped (its onStop() method is called), this method should be
     * invoked to release the MediaRecorder object, unless the application
     * has a special need to keep the object around. In addition to
     * unnecessary resources (such as memory and instances of codecs)
     * being held, failure to call this method immediately if a
     * MediaRecorder object is no longer needed may also lead to
     * continuous battery consumption for mobile devices, and recording
     * failure for other applications if no multiple instances of the
     * same codec are supported on a device. Even if multiple instances
     * of the same codec are supported, some performance degradation
     * may be expected when unnecessary multiple instances are used
     * at the same time.
     */
    void release();
    
    status_t setVideoEncodingIFramesNumberInterval(int nMaxKeyItl);
    status_t setVideoEncodingIFramesNumberIntervalSync(int nMaxKeyItl);
    status_t reencodeIFrame();
    status_t setSdcardState(bool bExist);

	/* gushiming add record extra file for CDR start */
	status_t setExtraFileFd(int fd);
	status_t setExtraFileDuration(int bfTimeMs, int afTimeMs);
	status_t startExtraFile();
	status_t stopExtraFile();
	/* gushiming add record extra file for CDR end */
	status_t setImpactFileDuration(int bfTimeMs, int afTimeMs);
	status_t setImpactOutputFile(int fd, int64_t fallocateLength, int muxerId=0);
    status_t setImpactOutputFile(char* path, int64_t fallocateLength, int muxerId=0);
    status_t setSourceChannel(int channel);
    status_t enableDynamicBitRateControl(bool bEnable);

    static const char FatFsPrefixString[];  // "0:/"
};

};

#endif  /* _HERBMEDIARECORDER_H_ */

