#ifndef CEDARX_RECORDER_H_
#define CEDARX_RECORDER_H_

#include <media/MediaRecorderBase.h>
#include <camera/CameraParameters.h>
#include <utils/String8.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
#include <media/AudioRecord.h>
#include <camera/ICameraRecordingProxyListener.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MediaBuffer.h>

#include <CDX_Recorder.h>
#include <type_camera.h>
#include <CDX_Types.h>
#include <include_writer/recorde_writer.h>
#include <include_vencoder/encoder_type.h>

#include <vencoder.h>

namespace android {

class Camera;
class AudioRecord;
//class CedarXRecorderAdapter;

#define AUDIO_LATENCY_TIME	0   //700000		// US
#define VIDEO_LATENCY_TIME	0   //700000		// US
#define AUDIO_LATENCY_MUTE_TIME	0   //(700000)        //US
#define AUDIO_LATENCY_TIME_CTS	  0   //fuqiang
#define VIDEO_LATENCY_TIME_CTS	  0
#define MAX_FILE_SIZE		(2*1024*1024*1024LL - 200*1024*1024)
#define ENC_BACKUP_BUFFER_NUM	10
//#define ADD_CEDARXRECORDER_NOTIFICATIONCLIENT

struct OutputSinkInfo   //counterparts: CdxOutputSinkInfo
{
    int             mMuxerId;
    output_format   nOutputFormat;
    int             nOutputFd;
    String8         mOutputPath;
    int             nFallocateLen;
    bool            nCallbackOutFlag;
};
extern bool checkVectorOutputSinkInfoValid(Vector<OutputSinkInfo> &outputSinkInfoVector);

typedef enum CDX_RECORDER_MEDIATYPE {
	CDX_RECORDER_MEDIATYPE_VIDEO = 0,
	CDX_RECORDER_MEDIATYPE_AUDIO
}CDX_RECORDER_MEDIATYPE;
typedef struct EncQueueBuffer
{
	sp<IMemory>	mem;
	bool		isUsing;
} EncQueueBuffer;


typedef struct SoftFrameRateCtrl
{
    int capture;
    int total;
    int count;
    bool enable;
    //method2: by pts.
    int64_t mBasePts;   //unit:us
    int64_t mCurrentWantedPts;
    //int     mSoftFrameRate;
    int64_t mFrameCounter;
} SoftFrameRateCtrl;
extern int SoftFrameRateCtrlDestroy(SoftFrameRateCtrl *pThiz);

typedef struct CaptureTimeLapse
{
    bool enable;
    int64_t capFrameIntervalUs;
    int64_t videoFrameIntervalUs;
    int64_t lastCapTimeUs;
    int64_t lastTimeStamp;
} CaptureTimeLapse;

class CedarXRecorder : public MediaRecorderBase 
{
public:
    CedarXRecorder();
    virtual ~CedarXRecorder();
	virtual status_t init();
    virtual status_t setAudioSource(audio_source_t as);
    virtual status_t setVideoSource(video_source vs);
    virtual status_t setOutputFormat(output_format of);
    virtual status_t setAudioEncoder(audio_encoder ae);
    virtual status_t setVideoEncoder(video_encoder ve);
    virtual status_t setVideoSize(int width, int height);
    virtual status_t setVideoFrameRate(int frames_per_second);
    virtual status_t setMuteMode(bool mute);
    virtual status_t setCamera(const sp<ICamera>& camera, const sp<ICameraRecordingProxy>& proxy);
    virtual status_t setPreviewSurface(const unsigned int hlay);
    virtual status_t setOutputFile(const char *path);
    virtual status_t setOutputFile(const char *path, int64_t offset, int64_t length);
    virtual status_t setOutputFile(int fd, int64_t offset, int64_t length);
    virtual status_t setParameters(const String8& params);
    virtual status_t setListener(const sp<IMediaRecorderClient>& listener);
    virtual status_t prepare();
    virtual status_t start();
    virtual status_t stop();
    virtual status_t close();
    virtual status_t reset();
    virtual status_t getMaxAmplitude(int *max);
    virtual status_t dump(int fd, const Vector<String16>& args) const;
    // Querying a SurfaceMediaSourcer
    virtual unsigned int querySurfaceMediaSource() const;
    virtual status_t queueBuffer(int index, int addr_y, int addr_c, int64_t timestamp);
    virtual	sp<IMemory> getOneBsFrame(int mode);
    virtual	void freeOneBsFrame();
    virtual	sp<IMemory> getEncDataHeader();
    virtual status_t setVideoEncodingBitRateSync(int bitRate);
    virtual status_t setVideoFrameRateSync(int frames_per_seconid);
    virtual status_t setVideoEncodingIFramesNumberIntervalSync(int nMaxKeyItl);
    virtual status_t reencodeIFrame();
    virtual status_t setOutputFileSync(int fd, int64_t fallocateLength, int muxerId);
    virtual status_t setOutputFileSync(const char* path, int64_t fallocateLength, int muxerId);
    virtual status_t setSdcardState(bool bExist);
    virtual int addOutputFormatAndOutputSink(int of, int fd, int FallocateLen, bool callback_out_flag);
    virtual int addOutputFormatAndOutputSink(int of, const char* path, int FallocateLen, bool callback_out_flag);
    virtual status_t removeOutputFormatAndOutputSink(int muxerId);
    virtual status_t setMaxDuration(int max_duration_ms);
	virtual status_t setMaxFileSize(int64_t max_filesize_bytes);
	virtual status_t setImpactOutputFile(int fd, int64_t fallocateLength, int muxerId);
    virtual status_t setImpactOutputFile(const char* path, int64_t fallocateLength, int muxerId);
    
    status_t setMediaSource(const sp<MediaSource>& mediasource, int type);
    status_t pause();
	void releaseCamera();

	// Encoding parameter handling utilities

	status_t setOutputVideoSize(int width, int height);

	void CedarXReleaseFrame(int index);
	void dataCallbackTimestamp(int64_t timestampUs, int32_t msgType, const sp<IMemory> &data);
	status_t CedarXReadAudioBuffer(void *pbuf, int *size, int64_t *timeStamp);
    status_t readMediaBufferCallback(void *buf_header);
	int CedarXRecorderCallback(int event, void *info);
	
	status_t setImpactFileDuration(int bfTimeMs, int afTimeMs);
    status_t setSourceChannel(int value);

	class CameraProxyListener: public BnCameraRecordingProxyListener {
	public:
		CameraProxyListener(CedarXRecorder* recorder);
		virtual void dataCallbackTimestamp(int64_t timestampUs, int32_t msgType,
				const sp<IMemory> &data);

	private:
		CedarXRecorder * mRecorder;
	};

	// isBinderAlive needs linkToDeath to work.
	class DeathNotifier: public IBinder::DeathRecipient {
	public:
		DeathNotifier() {}
		virtual void binderDied(const wp<IBinder>& who)
		{
#if (CEDARX_ANDROID_VERSION > 6)
			ALOGI("Camera recording proxy died");
#else
			LOGI("Camera recording proxy died");
#endif
		}
	};
#ifdef ADD_CEDARXRECORDER_NOTIFICATIONCLIENT
//added by JM.
// --- Notification Client ---
    class NotificationClient : public IBinder::DeathRecipient 
    {
        public:
            NotificationClient(const sp<IMediaRecorderClient>& mediaRecorderClient,
				               CedarXRecorder* cedarXRecorder,
							           pid_t pid);
            virtual             ~NotificationClient();
            sp<IMediaRecorderClient>    client() { return mMediaRecorderClient; }
            // IBinder::DeathRecipient
            virtual     void        binderDied(const wp<IBinder>& who);
			
			
        private:
            NotificationClient(const NotificationClient&);
            NotificationClient& operator = (const NotificationClient&);

			sp<IMediaRecorderClient>     mMediaRecorderClient;
			CedarXRecorder*             mCedarXRecorder;
            pid_t                       mPid;            
    };

	sp<NotificationClient>     mNotificationClient;
//end add by JM.
#endif

private:
    status_t setParameter(const String8 &key, const String8 &value);
    // audio
	status_t setParamAudioEncodingBitRate(int32_t bitRate);
    status_t setParamAudioNumberOfChannels(int32_t channles);
    status_t setParamAudioSamplingRate(int32_t sampleRate);
    // video
	status_t setParamVideoEncodingBitRate(int32_t bitRate);
    status_t setParamVideoRotation(int32_t degrees);
    status_t setParamVideoIFramesNumberInterval(int32_t nMaxKeyItl);
    // output
	status_t setParamMaxFileDurationUs(int64_t timeUs);
	status_t setParamMaxFileSizeBytes(int64_t bytes);
    // location
	status_t setParamGeoDataLatitude(int64_t latitudex10000);
	status_t setParamGeoDataLongitude(int64_t longitudex10000);
    status_t setParamVideoCameraId(int32_t cameraId);
    status_t setParamTimeLapseEnable(int32_t timeLapseEnable);
    status_t setParamTimeBetweenTimeLapseFrameCapture(int64_t timeUs);
    status_t setParamImpactDurationBfTime(int bftime);
    status_t setParamImpactDurationAfTime(int aftime);
	int pushOneBsFrame(int mode);
	status_t CreateAudioRecorder();
	void releaseOneRecordingFrame(const sp<IMemory>& frame, int bufIdx);
	status_t isCameraAvailable(const sp<ICamera>& camera,
								   const sp<ICameraRecordingProxy>& proxy,
								   int32_t cameraId);
	audio_source_t mAudioSource;
	video_source mVideoSource;

	enum {
        kMaxBufferSize = 2048,

        // After the initial mute, we raise the volume linearly
        // over kAutoRampDurationUs.
        kAutoRampDurationUs = 300000,

        // This is the initial mute duration to suppress
        // the video recording signal tone
        kAutoRampStartUs = 700000,
    };
	
	char mCameraHalVersion[32];
		
    sp<Camera> mCamera;
	sp<ICameraRecordingProxy> mCameraProxy;
    //sp<Surface> mPreviewSurface;
    int mPreviewLayerId;
    sp<IMediaRecorderClient> mListener;

    output_format mOutputFormat;
    audio_encoder mAudioEncoder;
    video_encoder mVideoEncoder;
	
    int32_t mCameraId;
    int32_t mVideoWidth, mVideoHeight;  //encoded output frame size, use them to tell hw encoder to scale.
	int32_t mOutputVideoWidth, mOutputVideoHeight;  //same as mVideoWidth, not use it now.
    int32_t mFrameRate;
    int32_t mVideoBitRate;
    int32_t mVideoMaxKeyItl;
	int64_t mMaxFileDurationUs;
	int64_t mMaxFileSizeBytes;
    int64_t mAccumulateFileSizeBytes;
    int64_t mAccumulateFileDurationUs;
	int32_t mRotationDegrees;  // Clockwise

    int32_t mAudioBitRate;
    int32_t mAudioChannels;
	int32_t mSampleRate;

	int32_t mGeoAvailable;
	int32_t mLatitudex10000;
	int32_t mLongitudex10000;

    CaptureTimeLapse mCapTimeLapse;

    int mOutputFd;
    int	mFallocateLength; //match with mOutputFd
    char *mUrl;

    // mOutputFormat, mOutputFd, mOutputFd2, mResetFdFlag, mUrl is for old method.
    // mOutputSinkInfoVector is for new method. Donot use two methods at the same time.
    int mMuxerIdCounter;
    Vector<OutputSinkInfo> mOutputSinkInfoVector;   //for addOutputFormatAndOutputSink().

	int32_t mCameraFlags;
	int64_t mLastTimeStamp;
	int32_t mMaxDurationPerFrame;   //unit:us

    enum CameraFlags {
        FLAGS_SET_CAMERA = 1L << 0,
        FLAGS_HOT_CAMERA = 1L << 1,
    };

	Mutex mStateLock;
	Mutex mLock;
	Mutex mQueueLock;
	Mutex mSetParamLock;
	Mutex mDurationLock;
	
	sp<MemoryHeapBase>  mFrameHeap;
	sp<MemoryBase>      mFrameBuffer;

	bool				mStarted;
	volatile bool		mPrepared;
	uint 				mRecModeFlag;   //RECORDER_MODE_VIDEO
	AudioRecord 		*mRecord;
	CDXRecorder 		*mCdxRecorder;

	enum MuteMode {
		UNMUTE 	= 0,
		MUTE 	= 1,
	};
	
	MuteMode			mMuteMode;
    bool                mbSdCardState;   //true:exist, false:not exist.

	int64_t				mLatencyStartUs;

	sp<MediaSource> 	mMediaSourceVideo;
	MediaBuffer         *mInputBuffer;

	sp<DeathNotifier>   mDeathNotifier;
	sp<IMemory>         mBsFrameMemory;
#ifdef FOR_CTS_TEST
	char			*mCallingProcessName;   //fuqiang
#endif
	//int                 mAudioEncodeType;
	bool				mOutputVideosizeflag;
	Vector<EncQueueBuffer> mEncData;
	VencHeaderData mPpsInfo;
//    bool mRecordFileFlag;   //true:fwrite file; false:callback out, not fwrite file.
//    bool mResetDurationStatistics;
//    int64_t mDurationStartPts;  //unit:ms
	
    CedarXRecorder(const CedarXRecorder &);
    CedarXRecorder &operator=(const CedarXRecorder &);

	int mSrcFrameRate;

	int64_t				mImpactFileDuration[2];

    int                 mSourceChannel;
    //for debug
    int64_t mSampleCount;
    int64_t mDebugLastVideoPts; //unit:us

    int setFrameRateControl(int reqFrameRate, int srcFrameRate, int *numerator, int *denominator);
    SoftFrameRateCtrl mSoftFrameRateCtrl;
    status_t enableDynamicBitRateControl(bool bEnable);
    bool    mEnableDBRC;
//    friend class CedarXRecorderAdapter;
//    CedarXRecorderAdapter *pCedarXRecorderAdapter;
};

//typedef enum tag_CedarXRecorderAdapterCmd
//{
//    CEDARXRECORDERADAPTER_CMD_NOTIFY_CAMERA_CEDARX_ENCODE = 0,    //need tell camera, we use which encoder, from android4.0.4, change command name
//    
//    CEDARXRECORDERADAPTER_CMD_,
//}CedarXRecorderAdapterCmd;


//class CedarXRecorderAdapter  //base adapter
//{
//public:
//    CedarXRecorderAdapter(CedarXRecorder *recorder);
//    virtual ~CedarXRecorderAdapter();
//    int CedarXRecorderAdapterIoCtrl(int cmd, int aux, void *pbuffer);  //cmd = CedarXRecorderAdapterCmd
//    
//private:
//    CedarXRecorder* const pCedarXRecorder; //CedarXRecorder pointer
//};

}  // namespace android

#endif  // CEDARX_RECORDER_H_

