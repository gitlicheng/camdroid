#include <CDX_LogNDebug.h>
#define LOG_TAG "CedarXRecorder"
#include <CDX_Debug.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <ctype.h>
#include <cutils/properties.h>

#include "CedarXRecorder.h"

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

#if (CEDARX_ANDROID_VERSION < 7)
#include <media/stagefright/MediaDebug.h>
#include <media/stagefright/MetadataBufferType.h>
#include <surfaceflinger/Surface.h>
#else
#include <media/stagefright/foundation/ADebug.h>
#include <MetadataBufferType.h>
//#include <gui/ISurfaceTexture.h>
//#include <gui/SurfaceTextureClient.h>
//#include <gui/Surface.h>
#endif

#include <media/stagefright/MetaData.h>
#include <media/MediaProfiles.h>
#include <camera/ICamera.h>
#include <camera/CameraParameters.h>

#include <CDX_Common.h>

#include <type_camera.h>
#include <OMX_Core.h>
#include <OMX_Component.h>

#include <CDX_Recorder.h>
#include <include_writer/recorde_writer.h>
#include <include_base/FsWriter.h>
#include <include_base/CDX_SystemBase.h>
#include <ConfigOption.h>
//#include <linux/videodev2.h>

#define F_LOG 	LOGV("%s, line: %d", __FUNCTION__, __LINE__);

#if (CEDARX_ANDROID_VERSION < 7)
#define OUTPUT_FORMAT_RAW 10
#define AUDIO_SOURCE_AF 8
#define MEDIA_RECORDER_VENDOR_EVENT_EMPTY_BUFFER_ID 3000
#define MEDIA_RECORDER_VENDOR_EVENT_BSFRAME_AVAILABLE 3001
#endif

#define SIMPLE_CACHE_SIZE_VFS       (4*1024)
#define SIMPLE_CACHE_SIZE_DIRECTIO  (512*1024)    //(64*1024)  //byte
//typedef struct
//{
//    CDX_U32 extra_data_len;
//    CDX_U8 extra_data[32];
//} __extra_data_t;

extern "C" int CedarXRecorderCallbackWrapper(void *cookie, int event, void *info);
//extern "C" int MuxerGenerateAudioExtraData(__extra_data_t *pExtra, _media_file_inf_t *pAudioInf);

namespace android {

#define GET_CALLING_PID	(IPCThreadState::self()->getCallingPid())
#ifdef FOR_CTS_TEST
void getCallingProcessName(char *name)
{
	char proc_node[128];

	if (name == 0)
	{
		LOGE("error in params");
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
		LOGD("Calling process is: %s !", name);
	}
	else 
	{
		LOGE("Obtain calling process failed");
	}
}
#endif

int SoftFrameRateCtrlDestroy(SoftFrameRateCtrl *pThiz)
{
    pThiz->enable = false;
    pThiz->count = 0;
    pThiz->total = 0;
    pThiz->capture = 0;
    pThiz->mBasePts = -1;
    pThiz->mCurrentWantedPts = -1;
    //pThiz->mSoftFrameRate = 0;
    pThiz->mFrameCounter = 0;
    return 0;
}

//----------------------------------------------------------------------------------
// CDXCameraListener
//----------------------------------------------------------------------------------
struct CDXCameraListener : public CameraListener 
{
	CDXCameraListener(CedarXRecorder * recorder);

    virtual void notify(int32_t msgType, int32_t ext1, int32_t ext2);
    virtual void postData(int32_t msgType, const sp<IMemory> &dataPtr, camera_frame_metadata_t *metadata);
    virtual void postDataTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr);

protected:
    virtual ~CDXCameraListener();

private:
	CedarXRecorder * mRecorder;

    CDXCameraListener(const CDXCameraListener &);
    CDXCameraListener &operator=(const CDXCameraListener &);
};


CDXCameraListener::CDXCameraListener(CedarXRecorder * recorder)
    : mRecorder(recorder) 
{
	LOGV("CDXCameraListener Construct\n");
}

CDXCameraListener::~CDXCameraListener() {
	LOGV("CDXCameraListener Destruct\n");
}

void CDXCameraListener::notify(int32_t msgType, int32_t ext1, int32_t ext2) 
{
    LOGV("notify(%d, %d, %d)", msgType, ext1, ext2);
}

void CDXCameraListener::postData(int32_t msgType, const sp<IMemory> &dataPtr, camera_frame_metadata_t *metadata) 
{
    LOGV("postData(%d, ptr:%p, size:%d)",
         msgType, dataPtr->pointer(), dataPtr->size());
}

void CDXCameraListener::postDataTimestamp(
        nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr) 
{
	// LOGD("CDXCameraListener::postDataTimestamp\n");
	mRecorder->dataCallbackTimestamp(timestamp, msgType, dataPtr);
}

// To collect the encoder usage for the battery app
static void addBatteryData(uint32_t params) {
    sp<IBinder> binder =
        defaultServiceManager()->getService(String16("media.player"));
    sp<IMediaPlayerService> service = interface_cast<IMediaPlayerService>(binder);
    CHECK(service.get() != NULL);

    service->addBatteryData(params);
}
static MUXERMODES convertMuxerMode(int inputFormat)
{
	MUXERMODES muxer_mode;

	switch(inputFormat) {
	case OUTPUT_FORMAT_AWTS:
		muxer_mode = MUXER_MODE_AWTS;
		break;
	case OUTPUT_FORMAT_RAW:
		muxer_mode = MUXER_MODE_RAW;
		break;
	case OUTPUT_FORMAT_MPEG2TS:
		muxer_mode = MUXER_MODE_TS;
		break;
    case OUTPUT_FORMAT_MP3:
		muxer_mode = MUXER_MODE_MP3;
		break;
    case OUTPUT_FORMAT_AAC_ADIF:
    case OUTPUT_FORMAT_AAC_ADTS:
		muxer_mode = MUXER_MODE_AAC;
		break;
	default:
		muxer_mode = MUXER_MODE_MP4;
		break;
	}

	return muxer_mode;
}

static int convertVideoSource(int inputVideoSource)
{
	int outputVideoSource;

	switch (inputVideoSource) {
	case VIDEO_SOURCE_DEFAULT:
		outputVideoSource = CDX_VIDEO_SOURCE_DEFAULT;
		break;
	case VIDEO_SOURCE_CAMERA:
		outputVideoSource = CDX_VIDEO_SOURCE_CAMERA;
		break;
	case VIDEO_SOURCE_GRALLOC_BUFFER:
		outputVideoSource = CDX_VIDEO_SOURCE_GRALLOC_BUFFER;
		break;
	case VIDEO_SOURCE_PUSH_BUFFER:
		outputVideoSource = CDX_VIDEO_SOURCE_PUSH_BUFFER;
		break;
	default:
		outputVideoSource = CDX_VIDEO_SOURCE_DEFAULT;
		break;
	}

	return outputVideoSource;
}

//static int convertMuxerOutputSinkType(int sink_type)
//{
//	int nMuxerSinkType;
//
//	switch(sink_type) 
//    {
//	case SINKTYPE_FILE:
//		nMuxerSinkType = MUXER_SINKTYPE_FILE;
//		break;
//	case SINKTYPE_CALLBACKOUT:
//		nMuxerSinkType = MUXER_SINKTYPE_CALLBACKOUT;
//		break;
//	case SINKTYPE_ALL:
//		nMuxerSinkType = MUXER_SINKTYPE_ALL;
//		break;
//	default:
//		nMuxerSinkType = MUXER_SINKTYPE_ALL;
//		break;
//	}
//	return nMuxerSinkType;
//}

AUDIO_ENCODER_TYPE convertAudioEncoderType(audio_encoder ae)
{
    AUDIO_ENCODER_TYPE  nAudioEncodeType;
    if(ae == AUDIO_ENCODER_AAC || ae == AUDIO_ENCODER_DEFAULT)
	{
		nAudioEncodeType = AUDIO_ENCODER_AAC_TYPE;
	}
    else if(ae == AUDIO_ENCODER_PCM)
    {
        nAudioEncodeType = AUDIO_ENCODER_PCM_TYPE;
    }
    else if(ae == AUDIO_ENCODER_MP3)
    {
        nAudioEncodeType = AUDIO_ENCODER_MP3_TYPE;
    }
	else
	{
        ALOGE("(f:%s, l:%d) fatal error! wrong audio encoder[%d]", __FUNCTION__, __LINE__, ae);
		nAudioEncodeType = AUDIO_ENCODER_LPCM_TYPE; //only used by mpeg2ts
	}
    return nAudioEncodeType;
}

/*******************************************************************************
Function name: android.checkVectorOutputSinkInfoValid
Description: 
    1. valid standard: 
        callback has one valid at most. 
        (fd + callback) must at least one.
Parameters: 
    
Return: 
    
Time: 2014/6/14
*******************************************************************************/
bool checkVectorOutputSinkInfoValid(Vector<OutputSinkInfo> &outputSinkInfoVector)
{
    size_t i;
    int validFdNum = 0;
    int validCbNum = 0;
    for(i=0; i<outputSinkInfoVector.size();i++)
    {
        if(outputSinkInfoVector[i].nOutputFd >= 0 || !outputSinkInfoVector[i].mOutputPath.isEmpty())
        {
            validFdNum++;
        }
        if(true == outputSinkInfoVector[i].nCallbackOutFlag)
        {
            validCbNum++;
        }
    }
    if(validCbNum>1 || validCbNum<0)
    {
        ALOGD("(f:%s, l:%d) validCbNum[%d] invalid", __FUNCTION__, __LINE__, validCbNum);
        return false;
    }
    if(validFdNum+validCbNum <= 0)
    {
        ALOGD("(f:%s, l:%d) (FdNum + CbNum)[%d] invalid", __FUNCTION__, __LINE__, validFdNum+validCbNum);
        return false;
    }
    return true;
}

// Attempt to parse an int64 literal optionally surrounded by whitespace,
// returns true on success, false otherwise.
static bool safe_strtoi64(const char *s, int64_t *val) {
    char *end;

    // It is lame, but according to man page, we have to set errno to 0
    // before calling strtoll().
    errno = 0;
    *val = strtoll(s, &end, 10);

    if (end == s || errno == ERANGE) {
        return false;
    }

    // Skip trailing whitespace
    while (isspace(*end)) {
        ++end;
    }

    // For a successful return, the string must contain nothing but a valid
    // int64 literal optionally surrounded by whitespace.

    return *end == '\0';
}

// Return true if the value is in [0, 0x007FFFFFFF]
static bool safe_strtoi32(const char *s, int32_t *val) {
    int64_t temp;
    if (safe_strtoi64(s, &temp)) {
        if (temp >= 0 && temp <= 0x007FFFFFFF) {
            *val = static_cast<int32_t>(temp);
            return true;
        }
    }
    return false;
}

// Trim both leading and trailing whitespace from the given string.
static void TrimString(String8 *s) {
    size_t num_bytes = s->bytes();
    const char *data = s->string();

    size_t leading_space = 0;
    while (leading_space < num_bytes && isspace(data[leading_space])) {
        ++leading_space;
    }

    size_t i = num_bytes;
    while (i > leading_space && isspace(data[i - 1])) {
        --i;
    }

    s->setTo(String8(&data[leading_space], i - leading_space));
}

CedarXRecorder::CameraProxyListener::CameraProxyListener(CedarXRecorder * recorder) 
	:mRecorder(recorder){
}

void CedarXRecorder::CameraProxyListener::dataCallbackTimestamp(
        nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr) {
    mRecorder->dataCallbackTimestamp(timestamp / 1000, msgType, dataPtr);
}

//----------------------------------------------------------------------------------
// CedarXRecorder
//----------------------------------------------------------------------------------

CedarXRecorder::CedarXRecorder()
	: mOutputFd(-1)
	, mFallocateLength(0)
	, mUrl(NULL)
	, mStarted(false)
	, mPrepared(false)
	, mRecModeFlag(0)
	, mRecord(NULL)
	, mCdxRecorder(NULL)
	, mMuteMode(UNMUTE)
	, mbSdCardState(true)
	, mLatencyStartUs(0)
#ifdef FOR_CTS_TEST
	, mCallingProcessName(NULL)
#endif
	, mSrcFrameRate(0)
	, mSourceChannel(0)
{

    LOGV("Constructor");
    ALOGD("CEDARX_ANDROID_VERSION = [%d]", CEDARX_ANDROID_VERSION);
    ALOGD("this recorderId is [%p]", this);

    mInputBuffer = NULL;
    mLastTimeStamp = -1;
    mBsFrameMemory = NULL;

    mAccumulateFileSizeBytes = 0;
    mAccumulateFileDurationUs = 0;
    //mRecordFileFlag = false;
    //mResetDurationStatistics = false;
    //mDurationStartPts = 0;
    
    reset();

    mFrameHeap = new MemoryHeapBase(2*sizeof(int));
    if(mFrameHeap == NULL)
    {
        LOGE("(f:%s, l:%d) fatal error! new MemoryHeapBase fail", __FUNCTION__, __LINE__);
    }
	if (mFrameHeap->getHeapID() < 0)
	{
		LOGE("ERR(%s): Record heap creation fail", __func__);
        mFrameHeap.clear();
	}
	mFrameBuffer = new MemoryBase(mFrameHeap, 0, 2*sizeof(int));
    if(mFrameBuffer == NULL)
    {
        LOGE("(f:%s, l:%d) fatal error! new MemoryHeapBase fail", __FUNCTION__, __LINE__);
    }

//    pCedarXRecorderAdapter = new CedarXRecorderAdapter(this);
//    if(NULL == pCedarXRecorderAdapter)
//    {
//        LOGW("new CedarXRecorderAdapter fail! File[%s],Line[%d]\n", __FILE__, __LINE__);
//    }

#ifdef FOR_CTS_TEST
	mCallingProcessName = (char *)malloc(sizeof(char) * 128);
	getCallingProcessName(mCallingProcessName);
#endif
	memset(mCameraHalVersion, 0, sizeof(mCameraHalVersion));
	mEncData.clear();
	memset(&mPpsInfo, 0, sizeof(VencHeaderData));

    mSampleCount = 0;
	
}

CedarXRecorder::~CedarXRecorder() 
{
    LOGV("CedarXRecorder Destructor");

    stop();
    if(mBsFrameMemory != NULL) {
    	mBsFrameMemory.clear();
    	mBsFrameMemory = NULL;
    }

	if (mFrameHeap != NULL)
	{
		mFrameHeap.clear();
		mFrameHeap = NULL;
	}

	if (mUrl != NULL) {
		free(mUrl);
		mUrl = NULL;
	}

#ifdef FOR_CTS_TEST
	if (mCallingProcessName != NULL) {
		free(mCallingProcessName);
		mCallingProcessName = NULL;
	}
#endif
    {
        Mutex::Autolock lock(mQueueLock);
        while (!mEncData.isEmpty()) {
            EncQueueBuffer &buf = mEncData.editItemAt(0);
            buf.mem.clear();
            mEncData.removeAt(0);
        }
        mEncData.clear();
    }

//    if(pCedarXRecorderAdapter)
//    {
//        delete pCedarXRecorderAdapter;
//        pCedarXRecorderAdapter = NULL;
//    }
	LOGV("CedarXRecorder Destructor OK");
}

status_t CedarXRecorder::init()
{
    ALOGV("init");
    return OK;
}

status_t CedarXRecorder::setAudioSource(audio_source_t as) 
{
    ALOGV("setAudioSource: %d", as);
    if (as < AUDIO_SOURCE_DEFAULT ||
        as >= AUDIO_SOURCE_CNT) {
        ALOGE("Invalid audio source: %d", as);
        return BAD_VALUE;
    }

    if (as == AUDIO_SOURCE_DEFAULT) {
        mAudioSource = AUDIO_SOURCE_MIC;
    } else {
        mAudioSource = as;
    }

    return OK;
}

status_t CedarXRecorder::setVideoSource(video_source vs) 
{
    ALOGV("setVideoSource: %d", vs);
    if (vs < VIDEO_SOURCE_DEFAULT ||
        vs >= VIDEO_SOURCE_LIST_END) {
        ALOGE("Invalid video source: %d", vs);
        return BAD_VALUE;
    }

    if (vs == VIDEO_SOURCE_DEFAULT) {
        mVideoSource = VIDEO_SOURCE_CAMERA;
    } else {
        mVideoSource = vs;
    }

    return OK;
}

status_t CedarXRecorder::setOutputFormat(output_format of) 
{
    ALOGV("setOutputFormat: %d", of);
    if (of < OUTPUT_FORMAT_DEFAULT ||
        of >= OUTPUT_FORMAT_LIST_END) {
        ALOGE("Invalid output format: %d", of);
        return BAD_VALUE;
    }

    if (of == OUTPUT_FORMAT_DEFAULT) {
        mOutputFormat = OUTPUT_FORMAT_THREE_GPP;
    } else {
        mOutputFormat = of;
    }

    return OK;
}

status_t CedarXRecorder::setAudioEncoder(audio_encoder ae) 
{
    ALOGV("setAudioEncoder: %d", ae);
   if (ae < AUDIO_ENCODER_DEFAULT ||
        ae >= AUDIO_ENCODER_LIST_END) {
        ALOGE("Invalid audio encoder: %d", ae);
        return BAD_VALUE;
    }
	mRecModeFlag |= RECORDER_MODE_AUDIO;
	if (ae == AUDIO_ENCODER_DEFAULT) {
        mAudioEncoder = AUDIO_ENCODER_AAC;
    } else {
        mAudioEncoder = ae;
    }
    
    return OK;
}

status_t CedarXRecorder::setVideoEncoder(video_encoder ve) 
{
    LOGV("setVideoEncoder: %d", ve);
    
	mRecModeFlag |= RECORDER_MODE_VIDEO;
	
    return OK;

    ALOGV("setVideoEncoder: %d", ve);
    if (ve < VIDEO_ENCODER_DEFAULT ||
        ve >= VIDEO_ENCODER_LIST_END) {
        ALOGE("Invalid video encoder: %d", ve);
        return BAD_VALUE;
    }
    mRecModeFlag |= RECORDER_MODE_VIDEO;
    if (ve == VIDEO_ENCODER_DEFAULT) {
        mVideoEncoder = VIDEO_ENCODER_H264;
    } else {
        mVideoEncoder = ve;
    }

    return OK;
}

status_t CedarXRecorder::setVideoSize(int width, int height) 
{
    ALOGV("setVideoSize: %dx%d", width, height);
    if (width <= 0 || height <= 0) {
        ALOGE("Invalid video size: %dx%d", width, height);
        return BAD_VALUE;
    }

    // Additional check on the dimension will be performed later
    mVideoWidth = width;
    mVideoHeight = height;

    return OK;
}

status_t CedarXRecorder::setOutputVideoSize(int width, int height) 
{
    LOGV("setOutputVideoSize: %dx%d", width, height);
   
    // Additional check on the dimension will be performed later
    mOutputVideoWidth = width;
    mOutputVideoHeight = height;
	mOutputVideosizeflag = true;

    return OK;
}


status_t CedarXRecorder::setVideoFrameRate(int frames_per_second) 
{
    ALOGV("setVideoFrameRate: %d", frames_per_second);
    if ((frames_per_second <= 0 && frames_per_second != -1) ||
        frames_per_second > 120) {
        ALOGE("Invalid video frame rate: %d", frames_per_second);
        return BAD_VALUE;
    }
    
    // Additional check on the frame rate will be performed later
    mFrameRate = frames_per_second;
    mMaxDurationPerFrame = 1000000 / mFrameRate;

	if (mPrepared && !mCapTimeLapse.enable) {
    	Mutex::Autolock lock(mSetParamLock);
    	if (setFrameRateControl(mFrameRate, mSrcFrameRate, &mSoftFrameRateCtrl.capture, &mSoftFrameRateCtrl.total) >= 0) {
            ALOGD("<F;%s, L:%d>Low framerate mode, capture %d in %d", __FUNCTION__, __LINE__,
                mSoftFrameRateCtrl.capture, mSoftFrameRateCtrl.total);
    		mSoftFrameRateCtrl.count = 0;
    		mSoftFrameRateCtrl.enable = true;
    	} else {
    		mSoftFrameRateCtrl.enable = false;
            SoftFrameRateCtrlDestroy(&mSoftFrameRateCtrl);
    	}
	}

    return OK;
}

status_t CedarXRecorder::setMuteMode(bool mute)
{
	LOGV("setMuteMode: %d", mute);

	mMuteMode = mute ? MUTE : UNMUTE;
	return OK;
}

status_t CedarXRecorder::setSdcardState(bool bExist)
{
    mbSdCardState = bExist;
    if (true == mPrepared)
    {
        int ret;
        ALOGD("(f:%s, l:%d) set mbSdCardState[%d] when mPrepared==true.", __FUNCTION__, __LINE__, mbSdCardState);
        ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_SDCARD_STATE, mbSdCardState, 0);
        if (ret != OK) 
        {
            ALOGE("(f:%s, l:%d) fail[%d]", __FUNCTION__, __LINE__, ret);
        }
    }
    else
    {
        ALOGD("(f:%s, l:%d) mbSdCardState[%d]", __FUNCTION__, __LINE__, mbSdCardState);
    }

    return OK;
}
status_t CedarXRecorder::setCamera(const sp<ICamera>& camera, const sp<ICameraRecordingProxy>& proxy)
{
    LOGV("setCamera");
	if (camera == NULL) {
		LOGV("setCamera, camera=NULL");
	}
	if (proxy == NULL) {
		LOGV("setCamera, proxy=NULL");
        return BAD_VALUE;
	}

	int err = UNKNOWN_ERROR;

    int64_t token = IPCThreadState::self()->clearCallingIdentity();
	
	if ((err = isCameraAvailable(camera, proxy, mCameraId)) != OK) {
        LOGE("Camera connection could not be established.");
    	IPCThreadState::self()->restoreCallingIdentity(token);
        return err;
    }

	// get camera hal version
	CameraParameters params(mCameraProxy->getParameters());
	const char * hal_version = params.get("camera-hal-version");
	if (hal_version != NULL)
	{
		strcpy(mCameraHalVersion, hal_version);
	}
	LOGD("camera hal version: %s", mCameraHalVersion);
	
    IPCThreadState::self()->restoreCallingIdentity(token);

    return OK;
}
    
status_t CedarXRecorder::setMediaSource(const sp<MediaSource>& mediasource, int type)
{
    LOGV("setMediaSource");

    if(CDX_RECORDER_MEDIATYPE_VIDEO == type) {
    	mMediaSourceVideo = mediasource;
    }
    else {
    	;
    }

    return OK;
}

status_t CedarXRecorder::isCameraAvailable(
    const sp<ICamera>& camera, const sp<ICameraRecordingProxy>& proxy,
    int32_t cameraId) 
{
    mCameraProxy = proxy;
    mDeathNotifier = new DeathNotifier();
    // isBinderAlive needs linkToDeath to work.
    mCameraProxy->asBinder()->linkToDeath(mDeathNotifier);

    // This CHECK is good, since we just passed the lock/unlock
    // check earlier by calling mCamera->setParameters().
    // if (mPreviewSurface != NULL)
    //if (mPreviewLayerId >= 0)
    //{
    //	CHECK_EQ((status_t)OK, mCameraProxy->setPreviewDisplay(mPreviewLayerId));
    //}
    mCameraProxy->lock();
#if (CEDARX_ANDROID_VERSION == 6 && CEDARX_ADAPTER_VERSION == 4)
    mCameraProxy->storeMetaDataInBuffers(true);
#elif (CEDARX_ANDROID_VERSION >= 6)
    mCameraProxy->sendCommand(CAMERA_CMD_SET_CEDARX_RECORDER, 0, 0);
#else
    #error "Unknown chip type!"
#endif
    return OK;
}


//status_t CedarXRecorder::setPreviewSurface(const sp<Surface>& surface)
status_t CedarXRecorder::setPreviewSurface(const unsigned int hlay)
{
    LOGV("setPreviewSurface: %d", hlay);
    //mPreviewSurface = surface;
    mPreviewLayerId = (int)hlay;

    return OK;
}

// The client side of mediaserver asks it to creat a SurfaceMediaSource
// and return a interface reference. The client side will use that
// while encoding GL Frames
unsigned int CedarXRecorder::querySurfaceMediaSource() const {
    ALOGV("Get SurfaceMediaSource");
    //return mSurfaceMediaSource->getBufferQueue();
    ALOGE("(f:%s, l:%d) mLayerMediaSource is not support!", __FUNCTION__, __LINE__);
    return 0;
}

status_t CedarXRecorder::queueBuffer(int index, int addr_y, int addr_c, int64_t timestamp)
{
    LOGV("++++queueBuffer index:%d, mFrameRate=%d timestamp=%lld",index,mFrameRate,timestamp);
    V4L2BUF_t buf;
    int ret;
    int rand_access;
	
	if(addr_y >= 0x40000000) {
		addr_y -= 0x40000000;
	}

	memset(&buf, 0, sizeof(V4L2BUF_t));
    rand_access = !!(index & 0x80000000);
    buf.index = index & 0x7fffffff;
    buf.timeStamp = timestamp;
    buf.addrPhyY = addr_y;
    buf.width = mVideoWidth;
    buf.height = mVideoHeight;
    buf.crop_rect.top = 0;
    buf.crop_rect.left = 0;
    buf.crop_rect.width = buf.width;
    buf.crop_rect.height = buf.height;
	buf.format = 0x3231564e; // NV12 fourcc

#if 0
    //LOGV("test frame pts c:%lld l:%lld diff:%lld mMaxDurationPerFrame=%d",buf.timeStamp, mLastTimeStamp,buf.timeStamp - mLastTimeStamp,mMaxDurationPerFrame);
    if (mLastTimeStamp != -1 && buf.timeStamp - mLastTimeStamp < mMaxDurationPerFrame) {
    	LOGV("----drop frame pts %lld %lld",buf.timeStamp, mLastTimeStamp);
    	CedarXReleaseFrame(buf.index);
    	return OK;
    }
    LOGV("++++enc frame pts %lld",buf.timeStamp);
#endif

    mLastTimeStamp = buf.timeStamp;

    // if encoder is stopped or paused ,release this frame
	if (mStarted == false)
	{
		CedarXReleaseFrame(buf.index);
		return OK;
	}

//	if ((readTimeUs - mLatencyStartUs) < VIDEO_LATENCY_TIME)
//	{
//		CedarXReleaseFrame(buf.index);
//		return OK;
//	}
    //if(buf.index < 0 || buf.index > NB_BUFFER)
    //{
    //    ALOGW("fatal error! buf.index[%d], check camera", buf.index);
    //}
	ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SEND_BUF, (unsigned int)&buf, rand_access);
	if (ret != 0)
	{
		CedarXReleaseFrame(buf.index);
	}

    return OK;
}

status_t CedarXRecorder::setParamGeoDataLongitude(int64_t longitudex10000) 
{
    LOGV("setParamGeoDataLongitude: %lld", longitudex10000);
    if (longitudex10000 > 1800000 || longitudex10000 < -1800000) {
        return BAD_VALUE;
    }
	mGeoAvailable = true;
    mLongitudex10000 = longitudex10000;
    return OK;
}

status_t CedarXRecorder::setParamGeoDataLatitude(int64_t latitudex10000) 
{
    LOGV("setParamGeoDataLatitude: %lld", latitudex10000);
    if (latitudex10000 > 900000 || latitudex10000 < -900000) {
        return BAD_VALUE;
    }
	mGeoAvailable = true;
    mLatitudex10000 = latitudex10000;
    return OK;
}

status_t CedarXRecorder::setOutputFile(const char *path) {
    ALOGV("setOutputFile(%s)", path);
    // We don't actually support this at all, as the media_server process
    // no longer has permissions to create files.
    //return -EPERM;
    return setOutputFile(path, 0, 0);
}

status_t CedarXRecorder::setOutputFile(const char *path, int64_t offset, int64_t length)
{
    ALOGV("setOutputFile(%s) %lld, %lld", path, offset, length);
    if(mUrl)
    {
        ALOGW("(f:%s, l:%d) mUrl[%s] is not null, free it.", __FUNCTION__, __LINE__, mUrl);
        free(mUrl);
        mUrl = NULL;
        mFallocateLength = 0;
    }
    if(path != NULL)
    {
        mUrl = strdup(path);
        mFallocateLength = length;
    }
    return OK;
}

status_t CedarXRecorder::setOutputFile(int fd, int64_t offset, int64_t length) 
{
    ALOGV("setOutputFile: %d, %lld, %lld", fd, offset, length);
    // These don't make any sense, do they?
    //CHECK_EQ(offset, 0ll);
    //CHECK_EQ(length, 0ll);

    if (fd < 0) {
        ALOGE("Invalid file descriptor: %d", fd);
        return -EBADF;
    }
	int ret;

    if (mOutputFd >= 0) {
        if(0 == mOutputFd)
        {
            ALOGW("(f:%s, l:%d) Be careful! fd == 0", __FUNCTION__, __LINE__);
        }
        ::close(mOutputFd);
        mOutputFd = -1;
    }
    mOutputFd = dup(fd);
	mFallocateLength = length;

    return OK;
}

status_t CedarXRecorder::setParamVideoCameraId(int32_t cameraId) 
{
    LOGV("setParamVideoCameraId: %d", cameraId);
	
    mCameraId = cameraId;
	
    return OK;
}

status_t CedarXRecorder::setParamAudioEncodingBitRate(int32_t bitRate) 
{
    LOGV("setParamAudioEncodingBitRate: %d", bitRate);
	
    mAudioBitRate = bitRate;
	
    return OK;
}

status_t CedarXRecorder::setParamAudioSamplingRate(int32_t sampleRate) 
{
	LOGV("setParamAudioSamplingRate: %d", sampleRate);

	// Additional check on the sample rate will be performed later.
	mSampleRate = sampleRate;
    return OK;
}

status_t CedarXRecorder::setParamAudioNumberOfChannels(int32_t channels) 
{
    LOGV("setParamAudioNumberOfChannels: %d", channels);

    // Additional check on the number of channels will be performed later.
    mAudioChannels = channels;
    return OK;
}

status_t CedarXRecorder::setParamMaxFileDurationUs(int64_t timeUs) 
{
    LOGV("setParamMaxFileDurationUs: %lld us", timeUs);

	Mutex::Autolock autoLock(mDurationLock);
    mMaxFileDurationUs = timeUs;
	if (mPrepared) {
		mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_FILE_DURATION, (unsigned int)(mMaxFileDurationUs / 1000), 0);
	}
    return OK;
}

status_t CedarXRecorder::setParamMaxFileSizeBytes(int64_t bytes) 
{
    LOGV("setParamMaxFileSizeBytes: %lld bytes", bytes);
	
	Mutex::Autolock autoLock(mDurationLock);
	mMaxFileSizeBytes = bytes;

	if (mMaxFileSizeBytes > (int64_t)MAX_FILE_SIZE)
	{
		mMaxFileSizeBytes = (int64_t)MAX_FILE_SIZE;
    	LOGD("force maxFileSizeBytes to %lld bytes", mMaxFileSizeBytes);
	}
	
    return OK;
}

status_t CedarXRecorder::setParamVideoEncodingBitRate(int32_t bitRate) 
{
    LOGV("setParamVideoEncodingBitRate: %d", bitRate);
    bitRate = (bitRate/(50*1024))*(50*1024);
    if(0 == bitRate)
    {
        bitRate += 50*1024;
    }
    if(mVideoBitRate == bitRate)
    {
        return OK;
    }
    mVideoBitRate = bitRate;
	if (mPrepared) {
    	Mutex::Autolock lock(mSetParamLock);
		mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_BITRATE, mVideoBitRate, 0);
	}
    return OK;
}

// Always rotate clockwise, and only support 0, 90, 180 and 270 for now.
status_t CedarXRecorder::setParamVideoRotation(int32_t degrees) 
{
    LOGV("setParamVideoRotation: %d", degrees);
	
    mRotationDegrees = degrees % 360;

    return OK;
}

status_t CedarXRecorder::setParamVideoIFramesNumberInterval(int32_t nMaxKeyItl)
{
    ALOGV("(f:%s, l:%d) setParamVideoIFramesNumberInterval: %d", __FUNCTION__, __LINE__, nMaxKeyItl);
    if(mVideoMaxKeyItl == nMaxKeyItl)
    {
        ALOGV("(f:%s, l:%d) same IFrameInverval[%d]", __FUNCTION__, __LINE__, nMaxKeyItl);
        return OK;
    }
    mVideoMaxKeyItl = nMaxKeyItl;
	if (mPrepared) {
    	Mutex::Autolock lock(mSetParamLock);
		mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_MAX_KEY_FRAME_INTERVAL, mVideoMaxKeyItl, 0);
	}
    return OK;
}

status_t CedarXRecorder::reencodeIFrame()
{
    ALOGV("(f:%s, l:%d) reencodeIFrame", __FUNCTION__, __LINE__);
	if (mPrepared) {
    	Mutex::Autolock lock(mSetParamLock);
		mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_REENCODE_KEY_FRAME, 0, 0);
	}
    return OK;
}

status_t CedarXRecorder::setParamTimeLapseEnable(int32_t timeLapseEnable) {
    LOGV("setParamTimeLapseEnable: %d", timeLapseEnable);

	mCapTimeLapse.enable = timeLapseEnable;
    return OK;
}

status_t CedarXRecorder::setParamTimeBetweenTimeLapseFrameCapture(int64_t timeUs) {
    LOGD("setParamTimeBetweenTimeLapseFrameCapture: %lld us", timeUs);

    mCapTimeLapse.capFrameIntervalUs = timeUs;
    return OK;
}

status_t CedarXRecorder::setParameter(
        const String8 &key, const String8 &value) {
    ALOGV("setParameter: key (%s) => value (%s)", key.string(), value.string());
    if (key == "max-duration") {
        int64_t max_duration_ms;
        if (safe_strtoi64(value.string(), &max_duration_ms)) {
            return setParamMaxFileDurationUs(1000LL * max_duration_ms);
        }
    } else if (key == "max-filesize") {
        int64_t max_filesize_bytes;
        if (safe_strtoi64(value.string(), &max_filesize_bytes)) {
            return setParamMaxFileSizeBytes(max_filesize_bytes);
        }
    } /*else if (key == "interleave-duration-us") {
        int32_t durationUs;
        if (safe_strtoi32(value.string(), &durationUs)) {
            return setParamInterleaveDuration(durationUs);
        }
    } else if (key == "param-movie-time-scale") {
        int32_t timeScale;
        if (safe_strtoi32(value.string(), &timeScale)) {
            return setParamMovieTimeScale(timeScale);
        }
    } else if (key == "param-use-64bit-offset") {
        int32_t use64BitOffset;
        if (safe_strtoi32(value.string(), &use64BitOffset)) {
            return setParam64BitFileOffset(use64BitOffset != 0);
        }
    } */else if (key == "param-geotag-longitude") {
        int64_t longitudex10000;
        if (safe_strtoi64(value.string(), &longitudex10000)) {
            return setParamGeoDataLongitude(longitudex10000);
        }
    } else if (key == "param-geotag-latitude") {
        int64_t latitudex10000;
        if (safe_strtoi64(value.string(), &latitudex10000)) {
            return setParamGeoDataLatitude(latitudex10000);
        }
    } /*else if (key == "param-track-time-status") {
        int64_t timeDurationUs;
        if (safe_strtoi64(value.string(), &timeDurationUs)) {
            return setParamTrackTimeStatus(timeDurationUs);
        }
    }*/ else if (key == "audio-param-sampling-rate") {
        int32_t sampling_rate;
        if (safe_strtoi32(value.string(), &sampling_rate)) {
            return setParamAudioSamplingRate(sampling_rate);
        }
    } else if (key == "audio-param-number-of-channels") {
        int32_t number_of_channels;
        if (safe_strtoi32(value.string(), &number_of_channels)) {
            return setParamAudioNumberOfChannels(number_of_channels);
        }
    } else if (key == "audio-param-encoding-bitrate") {
        int32_t audio_bitrate;
        if (safe_strtoi32(value.string(), &audio_bitrate)) {
            return setParamAudioEncodingBitRate(audio_bitrate);
        }
    } /*else if (key == "audio-param-time-scale") {
        int32_t timeScale;
        if (safe_strtoi32(value.string(), &timeScale)) {
            return setParamAudioTimeScale(timeScale);
        }
    }*/ else if (key == "video-param-encoding-bitrate") {
        int32_t video_bitrate;
        if (safe_strtoi32(value.string(), &video_bitrate)) {
            return setParamVideoEncodingBitRate(video_bitrate);
        }
    } else if (key == "video-param-rotation-angle-degrees") {
        int32_t degrees;
        if (safe_strtoi32(value.string(), &degrees)) {
            return setParamVideoRotation(degrees);
        }
    } /*else if (key == "video-param-i-frames-interval") {
        int32_t seconds;
        if (safe_strtoi32(value.string(), &seconds)) {
            return setParamVideoIFramesInterval(seconds);
        }
    } else if (key == "video-param-encoder-profile") {
        int32_t profile;
        if (safe_strtoi32(value.string(), &profile)) {
            return setParamVideoEncoderProfile(profile);
        }
    } else if (key == "video-param-encoder-level") {
        int32_t level;
        if (safe_strtoi32(value.string(), &level)) {
            return setParamVideoEncoderLevel(level);
        }
    }*/ else if (key == "video-param-camera-id") {
        int32_t cameraId;
        if (safe_strtoi32(value.string(), &cameraId)) {
            return setParamVideoCameraId(cameraId);
        }
    } /*else if (key == "video-param-time-scale") {
        int32_t timeScale;
        if (safe_strtoi32(value.string(), &timeScale)) {
            return setParamVideoTimeScale(timeScale);
        }
    }*/ else if (key == "time-lapse-enable") {
        int32_t timeLapseEnable;
        if (safe_strtoi32(value.string(), &timeLapseEnable)) {
            return setParamTimeLapseEnable(timeLapseEnable);
        }
    } else if (key == "time-between-time-lapse-frame-capture") {
        int64_t timeBetweenTimeLapseFrameCaptureMs;
        if (safe_strtoi64(value.string(), &timeBetweenTimeLapseFrameCaptureMs)) {
            return setParamTimeBetweenTimeLapseFrameCapture(
                    1000LL * timeBetweenTimeLapseFrameCaptureMs);
        }
    } else if (key == "video-param-i-frames-number-interval") {
        int32_t nMaxKeyItl;
        if (safe_strtoi32(value.string(), &nMaxKeyItl)) {
            return setParamVideoIFramesNumberInterval(nMaxKeyItl);
        }
    } else if (key == "param-impact-file-duration-bftime") {
    	int bftime;
        if (safe_strtoi32(value.string(), &bftime)) {
            return setParamImpactDurationBfTime(bftime);
        }
    } else if (key == "param-impact-file-duration-aftime") {
    	int aftime;
        if (safe_strtoi32(value.string(), &aftime)) {
            return setParamImpactDurationAfTime(aftime);
        }
    } else if (key == "param-camera-source-channel") {
        int channel;
        if (safe_strtoi32(value.string(), &channel)) {
            return setSourceChannel(channel);
        }
    } else if (key == "dynamic-bitrate-control") {
        int nEnable;
        if (safe_strtoi32(value.string(), &nEnable)) {
            return enableDynamicBitRateControl((bool)nEnable);
        }
    } else {
        ALOGE("setParameter: failed to find key %s", key.string());
    }
    return BAD_VALUE;
}

status_t CedarXRecorder::setParameters(const String8 &params) {
    ALOGV("setParameters: %s", params.string());
    const char *cparams = params.string();
    const char *key_start = cparams;
    for (;;) {
        const char *equal_pos = strchr(key_start, '=');
        if (equal_pos == NULL) {
            ALOGE("Parameters %s miss a value", cparams);
            return BAD_VALUE;
        }
        String8 key(key_start, equal_pos - key_start);
        TrimString(&key);
        if (key.length() == 0) {
            ALOGE("Parameters %s contains an empty key", cparams);
            return BAD_VALUE;
        }
        const char *value_start = equal_pos + 1;
        const char *semicolon_pos = strchr(value_start, ';');
        String8 value;
        if (semicolon_pos == NULL) {
            value.setTo(value_start);
        } else {
            value.setTo(value_start, semicolon_pos - value_start);
        }
        if (setParameter(key, value) != OK) {
            return BAD_VALUE;
        }
        if (semicolon_pos == NULL) {
            break;  // Reaches the end
        }
        key_start = semicolon_pos + 1;
    }
    return OK;
}

status_t CedarXRecorder::setListener(const sp<IMediaRecorderClient> &listener) 
{
	ALOGV("setListener");
    mListener = listener;
#ifdef ADD_CEDARXRECORDER_NOTIFICATIONCLIENT
	//added by JM.
	pid_t pid = GET_CALLING_PID;
	mNotificationClient = new NotificationClient(listener,this, pid);
    sp<IBinder> binder = listener->asBinder();
    binder->linkToDeath(mNotificationClient);
	//ended by JM.
#endif
    return OK;
}

#ifdef ADD_CEDARXRECORDER_NOTIFICATIONCLIENT
//added by JM.

CedarXRecorder::NotificationClient::NotificationClient(const sp<IMediaRecorderClient>& mediaRecorderClient,
                                     CedarXRecorder* cedarXRecorder,
                                     pid_t pid)
    : mMediaRecorderClient(mediaRecorderClient), mCedarXRecorder(cedarXRecorder),mPid(pid)
{
}

CedarXRecorder::NotificationClient::~NotificationClient()
{
    //mClient.clear();
}

void CedarXRecorder::NotificationClient::binderDied(const wp<IBinder>& who)
{
    LOGD("CedarXRecorder::NotificationClient::binderDied, pid %d", mPid);
    sp<NotificationClient> keep(this);
    {
		//mMediaRecorderClient->stop();      
		mCedarXRecorder->stop();
    }
}
//ended add by JM.
#endif

static void AudioRecordCallbackFunction(int event, void *user, void *info) {
    switch (event) {
        case AudioRecord::EVENT_MORE_DATA: {
            LOGW("AudioRecord reported EVENT_MORE_DATA!");
            break;
        }
        case AudioRecord::EVENT_OVERRUN: {
            LOGW("AudioRecord reported EVENT_OVERRUN!");
            break;
        }
        default:
            // does nothing
            break;
    }
}

status_t CedarXRecorder::CreateAudioRecorder()
{
	CHECK(mAudioChannels == 1 || mAudioChannels == 2);
	
#if (CEDARX_ANDROID_VERSION < 7)
    uint32_t flags = AudioRecord::RECORD_AGC_ENABLE |
                     AudioRecord::RECORD_NS_ENABLE  |
                     AudioRecord::RECORD_IIR_ENABLE;
	
    mRecord = new AudioRecord(
                mAudioSource, mSampleRate, AUDIO_FORMAT_PCM_16_BIT,
                mAudioChannels > 1? AUDIO_CHANNEL_IN_STEREO: AUDIO_CHANNEL_IN_MONO,
                4 * kMaxBufferSize / sizeof(int16_t), /* Enable ping-pong buffers */
                flags,
                NULL,	// AudioRecordCallbackFunction,
                this);
	
#endif

#if (CEDARX_ANDROID_VERSION == 7)

    int minFrameCount;
    audio_channel_mask_t channelMask;
    status_t status = AudioRecord::getMinFrameCount(&minFrameCount,
                                           mSampleRate,
                                           AUDIO_FORMAT_PCM_16_BIT,
                                           mAudioChannels);
    if (status == OK) {
        // make sure that the AudioRecord callback never returns more than the maximum
        // buffer size
        int frameCount = kMaxBufferSize / sizeof(int16_t) / mAudioChannels;

        // make sure that the AudioRecord total buffer size is large enough
        int bufCount = 2;
        while ((bufCount * frameCount) < minFrameCount) {
            bufCount++;
        }

        AudioRecord::record_flags flags = (AudioRecord::record_flags)
                        (AudioRecord::RECORD_AGC_ENABLE |
                         AudioRecord::RECORD_NS_ENABLE  |
                         AudioRecord::RECORD_IIR_ENABLE);
        mRecord = new AudioRecord(
                    mAudioSource, mSampleRate, AUDIO_FORMAT_PCM_16_BIT,
                    audio_channel_in_mask_from_count(mAudioChannels),
                    bufCount * frameCount,
                    flags,
                    NULL,	// AudioRecordCallbackFunction,
                    this,
                    frameCount);
    }
#endif

#if ((CEDARX_ANDROID_VERSION == 8) || (CEDARX_ANDROID_VERSION == 9))
    int minFrameCount;

    status_t status = AudioRecord::getMinFrameCount(&minFrameCount,
                                           mSampleRate,
                                           AUDIO_FORMAT_PCM_16_BIT,
                   audio_channel_in_mask_from_count(mAudioChannels));
    if (status == OK) {
        // make sure that the AudioRecord callback never returns more than the maximum
        // buffer size
        int frameCount = kMaxBufferSize / sizeof(int16_t) / mAudioChannels;

        // make sure that the AudioRecord total buffer size is large enough
        int bufCount = 2;
        while ((bufCount * frameCount) < minFrameCount) {
            bufCount++;
        }
        mRecord = new AudioRecord(
                    mAudioSource, mSampleRate, AUDIO_FORMAT_PCM_16_BIT,
                    audio_channel_in_mask_from_count(mAudioChannels),
                    bufCount * frameCount,
                    NULL,	// AudioRecordCallbackFunction,
                    this,
                    frameCount);
         
    }

#endif

	if (mRecord == NULL)
	{
		LOGE("create AudioRecord failed");
		return UNKNOWN_ERROR;
	}

	status_t err = mRecord->initCheck();
	if (err != OK)
	{
		LOGE("AudioRecord is not initialized");
		return UNKNOWN_ERROR;
	}

	LOGV("CreateAudioRecorder framesize: %d, frameCount: %d, channelCount: %d, sampleRate: %d", 
		mRecord->frameSize(), mRecord->frameCount(), mRecord->channelCount(), mRecord->getSampleRate());

	return OK;
}

int CedarXRecorder::setFrameRateControl(int reqFrameRate, int srcFrameRate, int *numerator, int *denominator)
{
    int i;
    int ratio;

    if (reqFrameRate >= srcFrameRate) {
        ALOGV("reqFrameRate[%d] > srcFrameRate[%d]", reqFrameRate, srcFrameRate);
        *numerator = 0;
        *denominator = 0;
        return -1;
    }

    ratio = (int)((float)reqFrameRate / srcFrameRate * 1000);

    for (i = srcFrameRate; i > 1; --i) {
        if (ratio < (int)((float)1.0 / i * 1000)) {
            break;
        }
    }

    if (i == srcFrameRate) {
        *numerator = 1;
        *denominator = i;
    } else if (i > 1){
        int rt1 = (int)((float)1.0 / i * 1000);
        int rt2 = (int)((float)1.0 / (i+1) * 1000);
        *numerator = 1;
        if (ratio - rt2 > rt1 - ratio) {
            *denominator = i;
        } else {
            *denominator = i + 1;
        }
    } else {
        for (i = 2; i < srcFrameRate - 1; ++i) {
            if (ratio < (int)((float)i / (i+1) * 1000)) {
                break;
            }
        }
        int rt1 = (int)((float)(i-1) / i * 1000);
        int rt2 = (int)((float)i / (i+1) * 1000);
        if (ratio - rt1 > rt2 - ratio) {
            *numerator = i;
            *denominator = i+1;
        } else {
            *numerator = i-1;
            *denominator = i;
        }
    }
    return 0;
}

status_t CedarXRecorder::prepare() 
{
	LOGV("prepare");
	int srcWidth = 0, srcHeight = 0;
	int ret = OK;
    int muxer_mode;
	int capture_fmt = 0;

	if(mCapTimeLapse.enable == false)
	{
		// Create audio recorder
		if (mRecModeFlag & RECORDER_MODE_AUDIO)
		{
			ret = CreateAudioRecorder();
			if (ret != OK)
			{
				LOGE("CreateAudioRecorder failed");
				return ret;
			}
		}
	}
    size_t  i;
    size_t  nSinkInfoSize = mOutputSinkInfoVector.size();
	// CedarX Create
	CDXRecorder_Create((void**)&mCdxRecorder);
	if(mCapTimeLapse.enable)
	{
			mRecModeFlag &= ~RECORDER_MODE_AUDIO;
	}
	
	// set recorder mode to CDX_Recorder
	mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_REC_MODE, mRecModeFlag, 0);

    if(0 == nSinkInfoSize)
    {
	    mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_OUTPUT_FORMAT, convertMuxerMode(mOutputFormat), 0);
        // set file handle to CDX_Recorder render component
        if(mOutputFd >= 0) 
        {
            CdxFdT fileDesc;
            memset(&fileDesc, 0, sizeof(CdxFdT));
            fileDesc.mPath = NULL;
            fileDesc.mFd = mOutputFd;
            fileDesc.mnFallocateLen = mFallocateLength;
            fileDesc.mMuxerId = 0;
            ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_SAVE_FILE, 0, (unsigned int)&fileDesc);
    	}
        else if(mUrl)
        {
            CdxFdT fileDesc;
            memset(&fileDesc, 0, sizeof(CdxFdT));
            fileDesc.mPath = mUrl;
            fileDesc.mFd = -1;
            fileDesc.mnFallocateLen = mFallocateLength;
            fileDesc.mMuxerId = 0;
            ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_SAVE_FILE, 0, (unsigned int)&fileDesc);
        }
        else
        {
            //ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_SAVE_URL, (unsigned int)mUrl, 0);
            ALOGW("(f:%s, l:%d) Be careful! no RecSink!", __FUNCTION__, __LINE__);
            //return BAD_VALUE;
        }
    }
    else
    {
        ALOGD("(f:%s, l:%d) use mOutputSinkInfoVector, size=%d", __FUNCTION__, __LINE__, nSinkInfoSize);
        for(i=0;i<nSinkInfoSize;i++)
        {
            CdxOutputSinkInfo cdxSinkInfo;
            cdxSinkInfo.mMuxerId        = mOutputSinkInfoVector[i].mMuxerId;
            cdxSinkInfo.nMuxerMode      = convertMuxerMode(mOutputSinkInfoVector[i].nOutputFormat);
            cdxSinkInfo.nOutputFd       = mOutputSinkInfoVector[i].nOutputFd;
            cdxSinkInfo.mPath           = (char*)mOutputSinkInfoVector[i].mOutputPath.string();
            cdxSinkInfo.nCallbackOutFlag= mOutputSinkInfoVector[i].nCallbackOutFlag;
            cdxSinkInfo.nFallocateLen   = mOutputSinkInfoVector[i].nFallocateLen;
//            ALOGD("(f:%s, l:%d) muxerId[%d], [%d][%d][%d][%d]", __FUNCTION__, __LINE__, 
//                cdxSinkInfo.mMuxerId, cdxSinkInfo.nMuxerMode, cdxSinkInfo.nOutputFd, cdxSinkInfo.nCallbackOutFlag, cdxSinkInfo.nFallocateLen);
            mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_ADD_OUTPUTSINKINFO, (unsigned int)&cdxSinkInfo, 0);
        }
    }

	// create CedarX component
	ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_PREPARE, 0, 0);
	if( ret == -1)
	{
		LOGE("CEDARX REPARE ERROR!\n");
		return UNKNOWN_ERROR;
	}

	// register callback
	mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_REGISTER_CALLBACK, (unsigned int)&CedarXRecorderCallbackWrapper, (unsigned int)this);

    if (mRecModeFlag & RECORDER_MODE_VIDEO) {
    	if (mVideoSource <= VIDEO_SOURCE_CAMERA)
    	{
    		int64_t token = IPCThreadState::self()->clearCallingIdentity();

    		ALOGD("prepare: mCameraHalVersion='%s'", mCameraHalVersion);
    		if (strlen(mCameraHalVersion) == 0)
    		{
    			ALOGD("prepare: cedarX set camera parameters");
    			// Set the actual video recording frame size
#if 0
    			CameraParameters params(mCameraProxy->getParameters());
    			params.setPreviewSize(mVideoWidth, mVideoHeight);
    			String8 s = params.flatten();
    			if (OK != mCameraProxy->setParameters(s)) {
    				LOGE("Could not change settings."
    					 " Someone else is using camera %d?", mCameraId);
    				IPCThreadState::self()->restoreCallingIdentity(token);
    				return -EBUSY;
    			}
#endif
    			
    			CameraParameters newCameraParams(mCameraProxy->getParameters());
                if (mSourceChannel == 0) {
    			    // Check on video frame size
    			    //newCameraParams.getPreviewSize(&srcWidth, &srcHeight);
                    newCameraParams.getVideoSize(&srcWidth, &srcHeight);
                } else {
                    //newCameraParams.getSubChannelSize(&srcWidth, &srcHeight);
                    newCameraParams.getPreviewSize(&srcWidth, &srcHeight);
                }
    			if (srcWidth  == 0 || srcHeight == 0) {
    				LOGE("Failed to set the video frame size to %dx%d", mVideoWidth, mVideoHeight);
    				IPCThreadState::self()->restoreCallingIdentity(token);
    				return UNKNOWN_ERROR;
    			}
    			mSrcFrameRate = newCameraParams.getPreviewFrameRate();
    			if (mSrcFrameRate <= 0) {
    				LOGE("Camera frame rate is %d!", mSrcFrameRate);
    				IPCThreadState::self()->restoreCallingIdentity(token);
    				return UNKNOWN_ERROR;
    			}
    		}
    		else if (!strncmp(mCameraHalVersion, "30", 2))
    		{
    			ALOGD("prepare: cedarX set camera parameters");
#if 0
    			// Set the actual video recording frame size
    			int video_w = 0, video_h = 0;
    			CameraParameters params(mCameraProxy->getParameters());
    			params.getVideoSize(&video_w, &video_h);
    			if (video_w != mVideoWidth
    				|| video_h != mVideoHeight)
    			{
    				params.setVideoSize(mVideoWidth, mVideoHeight);
    				String8 s = params.flatten();
    				if (OK != mCameraProxy->setParameters(s)) {
    					LOGE("Could not change settings."
    						 " Someone else is using camera %d?", mCameraId);
    					IPCThreadState::self()->restoreCallingIdentity(token);
    					return -EBUSY;
    				}
    			}
#endif

    			CameraParameters newCameraParams(mCameraProxy->getParameters());
                if (mSourceChannel == 0) {
        			// Check on capture frame size
    //    			const char * capture_width_str = newCameraParams.get("capture-size-width");
    //    			const char * capture_height_str = newCameraParams.get("capture-size-height");
    //    			if (capture_width_str != NULL && capture_height_str != NULL)
    //    			{
    //    				srcWidth  = atoi(capture_width_str);
    //    				srcHeight = atoi(capture_height_str);
    //    			}
                    newCameraParams.getVideoSize(&srcWidth, &srcHeight);
                } else {
                    //newCameraParams.getSubChannelSize(&srcWidth, &srcHeight);
                    newCameraParams.getPreviewSize(&srcWidth, &srcHeight);
                }
    			mSrcFrameRate = newCameraParams.getPreviewFrameRate();
    			if (mSrcFrameRate <= 0) {
    				LOGE("Camera frame rate is %d!", mSrcFrameRate);
    				IPCThreadState::self()->restoreCallingIdentity(token);
    				return UNKNOWN_ERROR;
    			}

    			/* gushiming compressed source */
    			const char * capture_fmt_str =  newCameraParams.get("capture-out-format");
    			if(capture_fmt_str != NULL) {
    				int fmt = atoi(capture_fmt_str);

    				if(fmt == V4L2_PIX_FMT_MJPEG || fmt == V4L2_PIX_FMT_JPEG)
    				{
    					LOGD("capture_fmt_str, V4L2_PIX_FMT_MJPEG");
    					capture_fmt = 1; // mjpeg source
    				}
    				else if(fmt == V4L2_PIX_FMT_H264)
    				{
    					LOGD("capture_fmt_str, V4L2_PIX_FMT_H264");
    				    capture_fmt = 2; // h264 source
    				}
    				else
    				{
    					LOGD("capture_fmt_str, common");
    					capture_fmt = 0; // common source
    				}
    			}
    		}
    		else
    		{
    			LOGE("unknow hal version");
    			return -1;
    		}

    		IPCThreadState::self()->restoreCallingIdentity(token);
    	
    		LOGV("src: %dx%d, video: %dx%d", srcWidth, srcHeight, mVideoWidth, mVideoHeight);
    	}
    	else
    	{
    		srcWidth = mVideoWidth;
    		srcHeight = mVideoHeight;
    		mSrcFrameRate = mFrameRate;
    	}

    	// set video size and FrameRate to CDX_Recorder
    	VIDEOINFO_t vInfo;
    	memset((void *)&vInfo, 0, sizeof(VIDEOINFO_t));

#if 0
    	vInfo.video_source 		= mVideoSource;
    	vInfo.src_width			= 176;
    	vInfo.src_height		= 144;
    	vInfo.width				= 160;			// mVideoWidth;
    	vInfo.height			= 120;			// mVideoHeight;
    	vInfo.frameRate			= 30*1000;		// mFrameRate;
    	vInfo.bitRate			= 200000;		// mVideoBitRate;
    	vInfo.profileIdc		= 66;
    	vInfo.levelIdc			= 31;
    	vInfo.geo_available		= mGeoAvailable;
    	vInfo.latitudex10000	= mLatitudex10000;
    	vInfo.longitudex10000	= mLongitudex10000;
    	vInfo.rotate_degree		= mRotationDegrees;
#else
    	vInfo.video_source 		= convertVideoSource(mVideoSource);
    	vInfo.src_width			= srcWidth;
    	vInfo.src_height		= srcHeight;

    	if(mOutputVideosizeflag == false) {
    		vInfo.width				= mVideoWidth;
    		vInfo.height			= mVideoHeight;
    	} else {
    		vInfo.width				= mOutputVideoWidth;
    		vInfo.height			= mOutputVideoHeight;
    	}

    	vInfo.frameRate			= mFrameRate*1000;
    	vInfo.bitRate			= mVideoBitRate;
    	vInfo.profileIdc		= VENC_H264ProfileMain;
    	vInfo.levelIdc			= VENC_H264Level31;
    	vInfo.geo_available		= mGeoAvailable;
    	vInfo.latitudex10000	= mLatitudex10000;
    	vInfo.longitudex10000	= mLongitudex10000;
    	vInfo.rotate_degree		= mRotationDegrees;
        vInfo.MaxKeyFrameInterval = mVideoMaxKeyItl;
    	vInfo.is_compress_source 	= capture_fmt;  /* gushiming compressed source */
        #if (defined(__CHIP_VERSION_F81)) //F81 try to use NV21.
    		vInfo.mPixelFormat = V4L2_PIX_FMT_NV21;
        #elif (defined(__CHIP_VERSION_F23) || defined(__CHIP_VERSION_F51))
            vInfo.mPixelFormat = V4L2_PIX_FMT_NV12;
        #else   //F33, F51 use NV21.
            vInfo.mPixelFormat = V4L2_PIX_FMT_NV21;
        #endif
    	ALOGD("prepare, source: %dx%d, video=%dx%d", vInfo.src_width, vInfo.src_height, vInfo.width, vInfo.height);
    	ALOGD("prepare, frameRate %d, mSrcFrameRate %d, VideoBitRate=%d, mVideoMaxKeyItl=%d", mFrameRate, mSrcFrameRate, vInfo.bitRate, mVideoMaxKeyItl);
#endif

    	if (mVideoWidth == 0
    		|| mVideoHeight == 0
    		|| mFrameRate == 0
    		|| mVideoBitRate == 0)
    	{
    		LOGE("error video para");
    		return -1;
    	}
    	
    	ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_VIDEO_INFO, (unsigned int)&vInfo, (unsigned int)(mImpactFileDuration[0]+mImpactFileDuration[1]));
    	if(ret != OK)
    	{
    		LOGE("CedarXRecorder::prepare, CDX_CMD SET_VIDEO_INFO failed\n");
    		return ret;
    	}

    	ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_GET_SPSPPS_INFO, (unsigned int)&mPpsInfo, 0);
    	if(ret != OK)
    	{
    		LOGE("CedarXRecorder::prepare, CDX_CMD_GET_PPS_SPS_INFO failed\n");
    		return ret;
    	}
    	ALOGD("ppsinfo.length = %d", mPpsInfo.nLength);
    }

	if(mCapTimeLapse.enable == false)
	{
		if (mRecModeFlag & RECORDER_MODE_AUDIO)
		{
			// set audio parameters
			AUDIOINFO_t aInfo;
			memset((void*)&aInfo, 0, sizeof(AUDIOINFO_t));
			aInfo.bitRate = mAudioBitRate;
			aInfo.channels = mAudioChannels;
			aInfo.sampleRate = mSampleRate;
			aInfo.bitsPerSample = 16;
			aInfo.audioEncType = convertAudioEncoderType(mAudioEncoder);
			if (mAudioBitRate == 0
				|| mAudioChannels == 0
				|| mSampleRate == 0)
			{
				LOGE("error audio para");
				return -1;
			}

			ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_AUDIO_INFO, (unsigned int)&aInfo, 0);
			if(ret != OK)
			{
				LOGE("CedarXRecorder::prepare, CDX_CMD_SET_AUDIO_INFO failed\n");
				return ret;
			}	
		}
	}

    if (mRecModeFlag & RECORDER_MODE_VIDEO) {
    	// time lapse mode
    	if (mCapTimeLapse.enable)
    	{
    		LOGV("time lapse mode");
    		mCapTimeLapse.videoFrameIntervalUs = 1E6/mFrameRate;
    		mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_TIME_LAPSE, 0, 0);
    	} else {
        	if (setFrameRateControl(mFrameRate, mSrcFrameRate, &mSoftFrameRateCtrl.capture, &mSoftFrameRateCtrl.total) >= 0) {
                ALOGD("<F;%s, L:%d>Low framerate mode, capture %d in %d", __FUNCTION__, __LINE__,
                    mSoftFrameRateCtrl.capture, mSoftFrameRateCtrl.total);
        		mSoftFrameRateCtrl.count = 0;
        		mSoftFrameRateCtrl.enable = true;
        	} else {
        		mSoftFrameRateCtrl.enable = false;
                SoftFrameRateCtrlDestroy(&mSoftFrameRateCtrl);
        	}
    	}

    	mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_IMPACT_FILE_DURATION, (unsigned int)mImpactFileDuration, 0);

    	mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_FILE_DURATION, (unsigned int)(mMaxFileDurationUs / 1000), 0);

        FSWRITEMODE nFsWriteMode;
        int nSimpleCacheSize = 0;
#if (CDXCFG_FSWRITEMODE==0)
        nFsWriteMode = FSWRITEMODE_CACHETHREAD;
        ALOGD("nFsWriteMode is FSWRITEMODE_CACHETHREAD");
#elif (CDXCFG_FSWRITEMODE==1)
        nFsWriteMode = FSWRITEMODE_SIMPLECACHE;
        ALOGD("nFsWriteMode is FSWRITEMODE_SIMPLECACHE");
    #if (CDXCFG_FILE_SYSTEM==OPTION_FILE_SYSTEM_LINUX_VFS)
        nSimpleCacheSize = SIMPLE_CACHE_SIZE_VFS;
        ALOGD("Read write method is VFS");
    #else
        nSimpleCacheSize = SIMPLE_CACHE_SIZE_DIRECTIO;
        ALOGD("Read write method is DIRECTIO");
    #endif
#elif (CDXCFG_FSWRITEMODE==2)
        nFsWriteMode = FSWRITEMODE_DIRECT;
        ALOGD("nFsWriteMode is FSWRITEMODE_DIRECT");
#else
        nFsWriteMode = FSWRITEMODE_DIRECT;
        ALOGD("nFsWriteMode is FSWRITEMODE_DIRECT");
#endif
        mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_FSWRITE_MODE, nFsWriteMode, 0);
        mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_SIMPLE_CACHE_SIZE, nSimpleCacheSize, 0);
    }

    ALOGD("(f:%s, l:%d) Dynamic BitRate Control %s", __FUNCTION__, __LINE__, mEnableDBRC?"enable":"disable");
    mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_ENABLE_DYNAMIC_BITRATE_CONTROL, mEnableDBRC, 0);
	mPrepared = true;

    mSampleCount = 0;
    return OK;
}

status_t CedarXRecorder::start() 
{
	LOGV("start");
	Mutex::Autolock autoLock(mStateLock);
	
	//CHECK(mOutputFd >= 0);
    if (mRecModeFlag & RECORDER_MODE_VIDEO) {
    	if (mVideoSource <= VIDEO_SOURCE_CAMERA)
    	{
    		LOGV("startCameraRecording");
    		// Reset the identity to the current thread because media server owns the
    		// camera and recording is started by the applications. The applications
    		// will connect to the camera in ICameraRecordingProxy::startRecording.
    		int64_t token = IPCThreadState::self()->clearCallingIdentity();
    		mCameraProxy->unlock();
    		//CHECK_EQ((status_t)OK, mCameraProxy->startRecording(new CameraProxyListener(this), (unsigned int)this));
    		if (mCameraProxy->startRecording(new CameraProxyListener(this), (unsigned int)this) != OK) {
                ALOGE("<F:%s, L:%d>startRecording failed!", __FUNCTION__, __LINE__);
                IPCThreadState::self()->restoreCallingIdentity(token);
                return UNKNOWN_ERROR;
    		}
    		IPCThreadState::self()->restoreCallingIdentity(token);
    	}
    }
	if(mCapTimeLapse.enable == false)
	{
		// audio start
		if ((mRecModeFlag & RECORDER_MODE_AUDIO)
			&& mRecord != NULL)
		{
			status_t ret = mRecord->start();
			if (ret != NO_ERROR) {
				ALOGE("<F:%s, L:%d>AudioRecord start error!", __FUNCTION__, __LINE__);
			}
		}
	}

	mLatencyStartUs = systemTime() / 1000;
	ALOGD("mLatencyStartUs: %lldms", mLatencyStartUs/1000);
	LOGV("VIDEO_LATENCY_TIME: %dus, AUDIO_LATENCY_TIME: %dus", VIDEO_LATENCY_TIME, AUDIO_LATENCY_TIME);

	mStarted = true;
	mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_START, 0, 0);

	LOGV("CedarXRecorder::start OK\n");
    return OK;
}

status_t CedarXRecorder::pause() 
{
    LOGV("pause");
	Mutex::Autolock autoLock(mStateLock);

	mStarted = false;

	if(mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_GETSTATE, 0, 0) == CDX_STATE_PAUSE)
	{
		mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_RESUME, 0, 0);
	}
	else
	{
		mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_PAUSE, 0, 0);
	}
		
    return OK;
}

status_t CedarXRecorder::stop() 
{
    LOGV("stop");
	
    status_t err = OK;
	
	{
		Mutex::Autolock autoLock(mStateLock);
		if (mStarted == true)
		{
            ALOGD("(f:%s, l:%d) turn mStarted to false, CedarXRecorder[%p]", __FUNCTION__, __LINE__, this);
			mStarted = false;
		}
		else
		{
            ALOGD("(f:%s, l:%d) mStarted already false, CedarXRecorder[%p]", __FUNCTION__, __LINE__, this);
			return err;
		}
		mPrepared = false;
	}
    if (mRecModeFlag & RECORDER_MODE_VIDEO) {
    	if (mVideoSource <= VIDEO_SOURCE_CAMERA)
    	{
    		int64_t token = IPCThreadState::self()->clearCallingIdentity();
    		mCameraProxy->stopRecording((unsigned int)this);    //from now, ICameraRecordingProxyListener has been cleared, no more frame can come to cedarX.
    		IPCThreadState::self()->restoreCallingIdentity(token);
    	}
    }
	mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_STOP, 0, 0);
	CDXRecorder_Destroy((void*)mCdxRecorder);
	mCdxRecorder = NULL;
	
    {
        Mutex::Autolock lock(mQueueLock);
        while (!mEncData.isEmpty()) {
            EncQueueBuffer &buf = mEncData.editItemAt(0);
            buf.mem.clear();
            mEncData.removeAt(0);
        }
        mEncData.clear();
    }
    
	mSoftFrameRateCtrl.enable = false;
    SoftFrameRateCtrlDestroy(&mSoftFrameRateCtrl);

    if(mOutputFd>=0)
    {
        if(0 == mOutputFd)
        {
            LOGD("(f:%s, l:%d) Be careful, mOutputFd==0, will close it", __FUNCTION__, __LINE__);
        }
        ::close(mOutputFd);
        mOutputFd = -1;
    }
//    mRecordFileFlag = false;
//    mResetDurationStatistics = false;

    size_t i;
    for(i=0;i<mOutputSinkInfoVector.size();i++)
    {
        if(mOutputSinkInfoVector[i].nOutputFd>=0)
        {
            if(0 == mOutputSinkInfoVector[i].nOutputFd)
            {
                LOGD("(f:%s, l:%d) Be careful, nOutputFd[%d]==0, will close it", __FUNCTION__, __LINE__, i);
            }
            ::close(mOutputSinkInfoVector[i].nOutputFd);
            mOutputSinkInfoVector.editItemAt(i).nOutputFd = -1;
        }
    }
    mOutputSinkInfoVector.clear();

    releaseCamera();    //from now, ICameraRecordingProxy has been cleared, cedarx can't release frame.

	if(mCapTimeLapse.enable == false)
	{
		// audio stop
		if (mRecModeFlag & RECORDER_MODE_AUDIO
			&& mRecord != NULL)
		{
			mRecord->stop();
			delete mRecord;
			mRecord = NULL;
			
		}	
	}
    mMuxerIdCounter = 0;
    mDebugLastVideoPts = -1;
	LOGV("stopped\n");
	
	return err;
}

status_t CedarXRecorder::close() {
    ALOGV("close");
    stop();

    return OK;
}

void CedarXRecorder::releaseCamera()
{
    if (mVideoSource <= VIDEO_SOURCE_CAMERA)
    {
    	LOGV("releaseCamera");

		if (mCameraProxy != 0)
		{
			mCameraProxy->asBinder()->unlinkToDeath(mDeathNotifier);
			mCameraProxy.clear();
		}
    }

    mCameraFlags = 0;
}

status_t CedarXRecorder::reset() 
{
    LOGV("reset");
    stop();

    // No audio or video source by default
    mAudioSource = AUDIO_SOURCE_CNT;
    mVideoSource = VIDEO_SOURCE_LIST_END;

    // Default parameters
    mOutputFormat  = OUTPUT_FORMAT_THREE_GPP;
    mAudioEncoder  = AUDIO_ENCODER_AAC;
    mVideoEncoder  = VIDEO_ENCODER_H264;
    mVideoWidth    = 176;
    mVideoHeight   = 144;
	mOutputVideoWidth    = 176;
    mOutputVideoHeight   = 144;
    mFrameRate     = 25;
    mVideoBitRate  = 3000000;
    mVideoMaxKeyItl = 25;
    mSampleRate    = 8000;
    mAudioChannels = 1;
    mAudioBitRate  = 12200;
    mCameraId      = 0;
	mCameraFlags   = 0;

    mMaxFileDurationUs = 0;
    mMaxFileSizeBytes = 0;
    mAccumulateFileSizeBytes = 0;
    mAccumulateFileDurationUs = 0;

    mRotationDegrees = 0;

    if(mOutputFd >= 0)
    {
        ::close(mOutputFd);
        mOutputFd = -1;
    }

	mStarted = false;
	mRecModeFlag = 0;
	mRecord = NULL;
	mLatencyStartUs = 0;

	mGeoAvailable = 0;
	mLatitudex10000 = -3600000;
    mLongitudex10000 = -3600000;
	mImpactFileDuration[0] = 0;
	mImpactFileDuration[1] = 0;

	mOutputVideosizeflag = false;

	mMuteMode = UNMUTE;
    mbSdCardState = true;
//    mRecordFileFlag = false;
//    mResetDurationStatistics = false;
//    mDurationStartPts = 0;

	mFallocateLength = 0;

    mMuxerIdCounter = 0;
    mDebugLastVideoPts = -1;
    mEnableDBRC = false;

    mCapTimeLapse.enable = false;
    mCapTimeLapse.capFrameIntervalUs = -1;
    mCapTimeLapse.lastCapTimeUs = 0;
    mCapTimeLapse.lastTimeStamp = 0;
    mCapTimeLapse.videoFrameIntervalUs = 0;

    mSoftFrameRateCtrl.enable = false;
    SoftFrameRateCtrlDestroy(&mSoftFrameRateCtrl);

    return OK;
}

status_t CedarXRecorder::getMaxAmplitude(int *max) 
{
    LOGV("getMaxAmplitude");

	// to do
	*max = 100;
	
    return OK;
}

status_t CedarXRecorder::dump(
        int fd, const Vector<String16>& args) const {
    ALOGV("dump");
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
//    if (mWriter != 0) {
//        mWriter->dump(fd, args);
//    } else {
//        snprintf(buffer, SIZE, "   No file writer\n");
//        result.append(buffer);
//    }
    snprintf(buffer, SIZE, "   Recorder: %p\n", this);
    snprintf(buffer, SIZE, "   Output file (fd %d):\n", mOutputFd);
    result.append(buffer);
    snprintf(buffer, SIZE, "     File format: %d\n", mOutputFormat);
    result.append(buffer);
    snprintf(buffer, SIZE, "     Max file size (bytes): %lld\n", mMaxFileSizeBytes);
    result.append(buffer);
    snprintf(buffer, SIZE, "     Max file duration (us): %lld\n", mMaxFileDurationUs);
    result.append(buffer);
//    snprintf(buffer, SIZE, "     File offset length (bits): %d\n", mUse64BitFileOffset? 64: 32);
//    result.append(buffer);
//    snprintf(buffer, SIZE, "     Interleave duration (us): %d\n", mInterleaveDurationUs);
//    result.append(buffer);
//    snprintf(buffer, SIZE, "     Progress notification: %lld us\n", mTrackEveryTimeDurationUs);
//    result.append(buffer);
    snprintf(buffer, SIZE, "   Audio\n");
    result.append(buffer);
    snprintf(buffer, SIZE, "     Source: %d\n", mAudioSource);
    result.append(buffer);
    snprintf(buffer, SIZE, "     Encoder: %d\n", mAudioEncoder);
    result.append(buffer);
    snprintf(buffer, SIZE, "     Bit rate (bps): %d\n", mAudioBitRate);
    result.append(buffer);
    snprintf(buffer, SIZE, "     Sampling rate (hz): %d\n", mSampleRate);
    result.append(buffer);
    snprintf(buffer, SIZE, "     Number of channels: %d\n", mAudioChannels);
    result.append(buffer);
//    snprintf(buffer, SIZE, "     Max amplitude: %d\n", mAudioSourceNode == 0? 0: mAudioSourceNode->getMaxAmplitude());
//    result.append(buffer);
    snprintf(buffer, SIZE, "   Video\n");
    result.append(buffer);
    snprintf(buffer, SIZE, "     Source: %d\n", mVideoSource);
    result.append(buffer);
    snprintf(buffer, SIZE, "     Camera Id: %d\n", mCameraId);
    result.append(buffer);
//    snprintf(buffer, SIZE, "     Start time offset (ms): %d\n", mStartTimeOffsetMs);
//    result.append(buffer);
    snprintf(buffer, SIZE, "     Encoder: %d\n", mVideoEncoder);
    result.append(buffer);
//    snprintf(buffer, SIZE, "     Encoder profile: %d\n", mVideoEncoderProfile);
//    result.append(buffer);
//    snprintf(buffer, SIZE, "     Encoder level: %d\n", mVideoEncoderLevel);
//    result.append(buffer);
//    snprintf(buffer, SIZE, "     I frames interval (s): %d\n", mIFramesIntervalSec);
//    result.append(buffer);
    snprintf(buffer, SIZE, "     Frame size (pixels): %dx%d\n", mVideoWidth, mVideoHeight);
    result.append(buffer);
    snprintf(buffer, SIZE, "     Frame rate (fps): %d\n", mFrameRate);
    result.append(buffer);
    snprintf(buffer, SIZE, "     Bit rate (bps): %d\n", mVideoBitRate);
    result.append(buffer);
    ::write(fd, result.string(), result.size());
    return OK;
}

void CedarXRecorder::releaseOneRecordingFrame(const sp<IMemory>& frame, int bufIdx) 
{
    if (mCameraProxy != NULL) {
        mCameraProxy->releaseRecordingFrame(frame, bufIdx);
    }
	else
	{
		ALOGW("(f:%s, l:%d) mCameraProxy is null", __FUNCTION__, __LINE__);
	}
}

void CedarXRecorder::dataCallbackTimestamp(int64_t timestampUs,
        int32_t msgType, const sp<IMemory> &data) 
{
	int64_t duration = 0;
	int64_t fileSizeBytes = 0;
	V4L2BUF_t buf;
	int ret = -1;
	int64_t readTimeUs = systemTime() / 1000;
	//Mutex::Autolock lock(mLock);
	if (data == NULL)
	{
		LOGE("error IMemory data\n");
		return;
	}

	memcpy((void *)&buf, data->pointer(), sizeof(V4L2BUF_t));
    //ALOGD("(f:%s, l:%d) sysTime[%lld]ms, pts[%lld]ms, frameRate[%d][%d], cedarXRecorder[%p]", __FUNCTION__, __LINE__, 
    //    readTimeUs/1000, buf.timeStamp/1000, mSrcFrameRate, mFrameRate, this);
    //if(buf.index < 0 || buf.index > NB_BUFFER)
    //{
    //    ALOGW("fatal error! buf.index[%d], check camera", buf.index);
    //}
    if (mSourceChannel == 1) {
        if(buf.isThumbAvailable)
        {
            buf.thumbUsedForVideo = 1;
        }
    }
	buf.overlay_info = NULL; //add this for cts test
	
	{
		Mutex::Autolock autoLock(mStateLock);
		// if encoder is stopped or paused ,release this frame
		if (mStarted == false)
		{
			CedarXReleaseFrame(buf.index);
			return ;
		}
	}

#ifdef FOR_CTS_TEST
	if ((strcmp(mCallingProcessName, "com.android.cts.media") == 0)||(strcmp(mCallingProcessName, "com.android.cts.mediastress") == 0))
	{
		if ((readTimeUs - mLatencyStartUs) < VIDEO_LATENCY_TIME_CTS)
		{
			CedarXReleaseFrame(buf.index);
			return ;
		}
	}
	else
#endif
	{
		if ((readTimeUs - mLatencyStartUs) < VIDEO_LATENCY_TIME)
		{
            ALOGD("(f:%s, l:%d) video latency[%lld][%lld]ms", __FUNCTION__, __LINE__, readTimeUs/1000, mLatencyStartUs/1000);
			CedarXReleaseFrame(buf.index);
			return ;
		}
	}

	// time lapse mode
	if (mCapTimeLapse.enable)
	{
		// LOGV("readTimeUs : %lld, lapse: %lld", readTimeUs, mCapTimeLapse.lastCapTimeUs + mCapTimeLapse.capFrameIntervalUs);
		if (readTimeUs < mCapTimeLapse.lastCapTimeUs + mCapTimeLapse.capFrameIntervalUs)
		{
			CedarXReleaseFrame(buf.index);
			return ;
		}
        mCapTimeLapse.lastCapTimeUs = readTimeUs;

		buf.timeStamp = mCapTimeLapse.lastTimeStamp + mCapTimeLapse.videoFrameIntervalUs;
        mCapTimeLapse.lastTimeStamp = buf.timeStamp;
	} else {
		Mutex::Autolock lock(mSetParamLock);
		if (mSoftFrameRateCtrl.enable) {
//            if (mSoftFrameRateCtrl.count >= mSoftFrameRateCtrl.total) {
//                mSoftFrameRateCtrl.count = 0;
//            }
//            if (mSoftFrameRateCtrl.count >= mSoftFrameRateCtrl.capture) {
//                ++mSoftFrameRateCtrl.count;
//                CedarXReleaseFrame(buf.index);
//                return;
//            }
//            ++mSoftFrameRateCtrl.count;

            if(-1 == mSoftFrameRateCtrl.mBasePts)
            {
                mSoftFrameRateCtrl.mBasePts = buf.timeStamp;
                mSoftFrameRateCtrl.mCurrentWantedPts = mSoftFrameRateCtrl.mBasePts;
                mSoftFrameRateCtrl.mFrameCounter = 0;
            }
            if(buf.timeStamp>=mSoftFrameRateCtrl.mCurrentWantedPts)
            {
                //ALOGD("(f:%s, l:%d) softFrameRate accept bufIndex[%d], pts[%lld]ms>=[%lld]ms", 
                //    __FUNCTION__, __LINE__, buf.index, buf.timeStamp/1000, mSoftFrameRateCtrl.mCurrentWantedPts/1000);
                mSoftFrameRateCtrl.mFrameCounter++;
                mSoftFrameRateCtrl.mCurrentWantedPts = mSoftFrameRateCtrl.mBasePts + (int64_t)mSoftFrameRateCtrl.mFrameCounter*(1000*1000)/mFrameRate;
            }
            else
            {
                //ALOGD("(f:%s, l:%d) softFrameRate release bufIndex[%d], pts[%lld]ms<[%lld]ms", 
                //    __FUNCTION__, __LINE__, buf.index, buf.timeStamp/1000, mSoftFrameRateCtrl.mCurrentWantedPts/1000);
                CedarXReleaseFrame(buf.index);
                return;
            }
		}
	}

	if(buf.format == V4L2_PIX_FMT_JPEG || buf.format == V4L2_PIX_FMT_MJPEG || buf.format == V4L2_PIX_FMT_H264) {  /* gushiming compressed source */
		buf.addrPhyY = buf.addrVirY;
		buf.width = buf.bytesused;
	}
	
	// LOGV("CedarXRecorder::dataCallbackTimestamp: addrPhyY %x, timestamp %lld us", buf.addrPhyY, timestampUs);
    if(-1 == mDebugLastVideoPts)
    {
        mDebugLastVideoPts = buf.timeStamp;
    }
    if(buf.timeStamp - mDebugLastVideoPts >= mMaxDurationPerFrame*2)
    {
        ALOGW("(f:%s, l:%d) Be careful! linux csi discard frame! buf.timeStamp[%lld]-lastPts[%lld]=[%lld]us >[%d]us, frameRate[%d][%d]", 
            __FUNCTION__, __LINE__, buf.timeStamp, mDebugLastVideoPts, buf.timeStamp-mDebugLastVideoPts, mMaxDurationPerFrame*2, mSrcFrameRate, mFrameRate);
    }
    mDebugLastVideoPts = buf.timeStamp;
	ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SEND_BUF, (unsigned int)&buf, 0); 
	if (ret != 0)
	{
        ALOGW("(f:%s, l:%d) fatal error! discard frame! buf.index[%d], send_buf ret[%d]", __FUNCTION__, __LINE__, buf.index, ret);
		CedarXReleaseFrame(buf.index);
	}
#if 0
    if(mRecordFileFlag)
    {
    	mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_GET_DURATION, (unsigned int)&duration, 0); 
    	// LOGV("duration : %d", duration);
    	if(mResetDurationStatistics)
    	{
//            ALOGD("(f:%s, l:%d) cedarx[%p] videosize[%dx%d] need reset duration[%d] statistics", __FUNCTION__, __LINE__,
//                this, mVideoWidth, mVideoHeight, duration);
            mDurationStartPts = duration;
            mAccumulateFileDurationUs = 0;
            mResetDurationStatistics = false;
    	}

		Mutex::Autolock autoLock(mDurationLock);
    	if (mMaxFileDurationUs != 0 
    		&& duration >= mDurationStartPts + (mAccumulateFileDurationUs + mMaxFileDurationUs) / 1000)
    	{
//            ALOGD("(f:%s, l:%d) cedarx[%p] videosize[%dx%d] will send MEDIA_RECORDER_INFO_MAX_DURATION_REACHED, duration[%d]s, mAccumulateFileDurationUs[%lld]s, mMaxFileDurationUs[%lld]s", __FUNCTION__, __LINE__,
//                this, mVideoWidth, mVideoHeight, duration/1000, mAccumulateFileDurationUs/(1000*1000), mMaxFileDurationUs/(1000*1000));
    		mListener->notify(MEDIA_RECORDER_EVENT_INFO, MEDIA_RECORDER_INFO_MAX_DURATION_REACHED, 0);
            //mAccumulateFileDurationUs = (int64_t)duration*1000;
            mAccumulateFileDurationUs += mMaxFileDurationUs;
    	}

    	mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_GET_FILE_SIZE, (unsigned int)&fileSizeBytes, 0); 
    	// LOGV("fileSizeBytes : %lld", fileSizeBytes);
    	
    	if (mMaxFileSizeBytes > 0 
    		&& fileSizeBytes >= mAccumulateFileSizeBytes + mMaxFileSizeBytes)
    	{
    //        ALOGD("(f:%s, l:%d) will send MEDIA_RECORDER_INFO_MAX_FILESIZE_REACHED, fileSizeBytes[%lld], mAccumulateFileSizeBytes[%lld], mMaxFileSizeBytes[%lld]", __FUNCTION__, __LINE__,
    //            fileSizeBytes, mAccumulateFileSizeBytes, mMaxFileSizeBytes);
    		mListener->notify(MEDIA_RECORDER_EVENT_INFO, MEDIA_RECORDER_INFO_MAX_FILESIZE_REACHED, 0);
            mAccumulateFileSizeBytes = fileSizeBytes;
    	}
    }
#endif
}

void CedarXRecorder::CedarXReleaseFrame(int index)
{
    Mutex::Autolock lock(mLock);
	if (mVideoSource <= VIDEO_SOURCE_CAMERA)
	{
		int * p = (int *)(mFrameBuffer->pointer());

		*p = index;
        *(p+1) = (int)this;
        //LOGD("(f:%s, l:%d) cedarx[%p] release frameIndex[%d]", __FUNCTION__, __LINE__, this, index);
		releaseOneRecordingFrame(mFrameBuffer, index);
	}
	else
	{
		LOGV("---- CedarXReleaseFrame index=%d", index);
		mListener->notify(MEDIA_RECORDER_VENDOR_EVENT_EMPTY_BUFFER_ID, index, 0);
	}
}

status_t CedarXRecorder::CedarXReadAudioBuffer(void *pbuf, int *size, int64_t *timeStamp)
{
    int64_t readTimeUs = systemTime() / 1000;

	// LOGV("CedarXRecorder::CedarXReadAudioBuffer, readTimeUs: %lld", readTimeUs);

	if (mRecord == NULL) {
		return UNKNOWN_ERROR;
	}
    *size = 0;
	*timeStamp = readTimeUs;
	ssize_t n = mRecord->read(pbuf, kMaxBufferSize);
	if (n < 0)
	{
		LOGE("mRecord read audio buffer failed");
		return UNKNOWN_ERROR;
	}
    //mSampleCount += n/(mAudioChannels*2);
    //ALOGD("inAudio [%lld]samples, [%lld]ms, sysTm[%lld]ms", mSampleCount, mSampleCount*1000/mSampleRate, readTimeUs/1000);
	if (mMuteMode == MUTE) {
		LOGV("CedarXReadAudioBuffer: mute audio record");
		memset(pbuf, 0x00, n);
	}

	//LOGV("readTimeUs:%lld,mLatencyStartUs:%lld diff:%lld",readTimeUs, mLatencyStartUs, readTimeUs - mLatencyStartUs);
#ifdef FOR_CTS_TEST
	if ((strcmp(mCallingProcessName, "com.android.cts.media") == 0)||(strcmp(mCallingProcessName, "com.android.cts.mediastress") == 0))
	{
		if ((readTimeUs - mLatencyStartUs) < AUDIO_LATENCY_TIME_CTS)
		{
			if (mAudioSource != AUDIO_SOURCE_AF)
				return INVALID_OPERATION;
		}
	}
	else
#endif
	{
		if ((readTimeUs - mLatencyStartUs) < AUDIO_LATENCY_TIME)
		{
            ALOGD("(f:%s, l:%d) audio latency[%lld][%lld]ms", __FUNCTION__, __LINE__, readTimeUs/1000, mLatencyStartUs/1000);
			if (mAudioSource != AUDIO_SOURCE_AF)
				return INVALID_OPERATION;
		}
        if ((readTimeUs - mLatencyStartUs) < AUDIO_LATENCY_MUTE_TIME)
        {
            ALOGD("(f:%s, l:%d) audio latency mute[%lld][%lld]ms", __FUNCTION__, __LINE__, readTimeUs/1000, mLatencyStartUs/1000);
            memset(pbuf, 0x00, n);
        }
	}
	
	*size = n;

	// LOGV("timestamp: %lld, len: %d", readTimeUs, n);

	return OK;
}

status_t CedarXRecorder::readMediaBufferCallback(void *buf_header)
{
	OMX_BUFFERHEADERTYPE *omx_buf_header = (OMX_BUFFERHEADERTYPE*)buf_header;
    ALOGE("(f:%s, l:%d) fatal error! impossible to come here! ", __FUNCTION__, __LINE__);
    #if 0
	if(omx_buf_header->nFlags == OMX_PortDomainVideo)
	{
		//LOGV("readMediaBufferCallback 1");
		if (mInputBuffer) {
			mInputBuffer->release();
			mInputBuffer = NULL;
		}
		mMediaSourceVideo->read(&mInputBuffer, NULL);

		if (mInputBuffer != NULL) {
			OMX_U32 type;
			buffer_handle_t buffer_handle;
			char *data = (char *)mInputBuffer->data();

			type = *((OMX_U32*)data);
			memcpy(&buffer_handle, data+4, 4);

			LOGV("mInputBuffer->size() = %d type = %d buffer_handle = %p",mInputBuffer->size(),type,buffer_handle);
			if (type == kMetadataBufferTypeGrallocSource) {

			}

			mInputBuffer->meta_data()->findInt64(kKeyTime, &omx_buf_header->nTimeStamp);
		}
		else {
			return NO_MEMORY;
		}
	}
    #endif
	return OK;
}

sp<IMemory> CedarXRecorder::getOneBsFrame(int mode)
{
    //ALOGV("getOneBsFrame");
	Mutex::Autolock lock(mQueueLock);
	if (!mEncData.isEmpty()) {
        EncQueueBuffer &buf = mEncData.editItemAt(0);
		if (buf.isUsing) {
			ALOGW("getOneBsFrame: APP need return buffer before get another one!!");
			return NULL;
		}
		buf.isUsing = true;
        sp<IMemory> mem = buf.mem;
        mEncData.removeAt(0);
    	return mem;
	} else {
        //ALOGW("queue buffer is empty mEncData.size = %d", mEncData.size());
		return NULL;
	}
}

void CedarXRecorder::freeOneBsFrame()
{
    //ALOGV("freeOneBsFrame");
  #if 0
	Mutex::Autolock lock(mQueueLock);
	if (!mEncData.isEmpty()) {
		EncQueueBuffer &buf = mEncData.editItemAt(0);
		buf.mem.clear();
		buf.mem = NULL;
		buf.isUsing = false;
		mEncData.removeAt(0);
        //ALOGV("remove mEncData.size = %d", mEncData.size());
	}
  #endif
}

int CedarXRecorder::pushOneBsFrame(int mode)
{
    int i;
    CDXRecorderBsInfo frame;
	sp<IMemory>         bsFrameMemory;

    Mutex::Autolock lock(mLock);
    //mBsFrameMemory.clear();
    if (mCdxRecorder == NULL) {
        LOGE("mCdxRecorder is not initialized");
        return -1;
    }

    if (mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_GET_ONE_BSFRAME, mode, (unsigned int)&frame) < 0) {
        LOGE("failed to get one frame");
        return -1;
    }
    //store videoExtraData, process change fd!
    {
        RawPacketType type = (RawPacketType)(((RawPacketHeader*)frame.bs_data[0])->stream_type);
        char* data0 = frame.bs_data[1];
        int flags = 0;
        if (type == RawPacketTypeVideo) 
        {
            if (data0[0] == 0x00 && data0[1] == 0x00 && 
                data0[2] == 0x00 && data0[3] == 0x01 && 
                data0[4] == 0x65) 
            {
                flags =  AVPACKET_FLAG_KEYFRAME;
            }
        }
        else if(type == RawPacketTypeVideoExtra) 
        {
            /*video extra data*/
            char* data0 = frame.bs_data[1];
            int size0 = frame.bs_size[1];
            ALOGD("find VideoExtraData[%p]size[%d], store it!", data0, size0);
    	} 
    }

	if (((RawPacketHeader*)frame.bs_data[0])->stream_type != RawPacketTypeVideo)
	{
		mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_FREE_ONE_BSFRAME, 0, (unsigned int)&frame);
		return -1;
	}

    size_t size = frame.total_size + 4;
    sp<MemoryHeapBase> heap = new MemoryHeapBase(size);
    if (heap == NULL) {
        LOGE("failed to create MemoryDealer");
        return -1;
    }

    bsFrameMemory = new MemoryBase(heap, 0, size);
    if (bsFrameMemory == NULL) {
        LOGE("not enough memory for VideoFrame size=%u", size);
        return -1;
    }

    uint8_t *ptr_dst = (uint8_t *)bsFrameMemory->pointer();

    memcpy(ptr_dst, &frame.total_size, 4);
    ptr_dst += 4;

    int total_bs_size=0;
    for (i=0; i<frame.bs_count; i++) {
    	memcpy(ptr_dst, frame.bs_data[i], frame.bs_size[i]);
    	ptr_dst += frame.bs_size[i];
        total_bs_size+=frame.bs_size[i];
	}
    if(total_bs_size != frame.total_size)
    {
        ALOGE("(f:%s, l:%d) fatal error! BsFrameSize[%d]!=[%d]", __FUNCTION__, __LINE__, total_bs_size, frame.total_size);
    }

#if 0
{
	int *total_size, *stream_type, *data_size;
	long long *pts;
	ptr_dst = (uint8_t *)bsFrameMemory->pointer();
	total_size = (int*)ptr_dst;
	ptr_dst += sizeof(int);
	stream_type = (int*)ptr_dst;
	ptr_dst += sizeof(int);
	data_size = (int*)ptr_dst;
	ptr_dst += sizeof(int);
	pts = (long long *)ptr_dst;
	ALOGV("pushOneBsFrame: total_size=%d, stream_type=%d, data_size=%d, pts=%lld", *total_size, *stream_type, *data_size, *pts);
}
#endif

    mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_FREE_ONE_BSFRAME, 0, (unsigned int)&frame);
    {
        Mutex::Autolock lock(mQueueLock);
    	if (mEncData.size() > ENC_BACKUP_BUFFER_NUM) {
    		mListener->notify(MEDIA_RECORDER_EVENT_ERROR, MEDIA_ERROR_BACKUP_BUFFER_OVERFLOW, 0);
    		{
    			//Mutex::Autolock lock(mQueueLock);
    			EncQueueBuffer &buf = mEncData.editItemAt(0);
    			ALOGW("pushOneBsFrame: queue_buffer_size %d, isUsing=%d", mEncData.size(), buf.isUsing);
    			if (!buf.isUsing) {
    				buf.mem.clear();
    				buf.mem = NULL;
    				buf.isUsing = false;
    				mEncData.removeAt(0);
    			} else {
    				ALOGW("pushOneBsFrame: APP need return buffer!!");
    				for (size_t i = 1; i < mEncData.size(); i++) {
    					EncQueueBuffer &buf = mEncData.editItemAt(i);
    					if (!buf.isUsing) {
    						buf.mem.clear();
    						buf.mem = NULL;
    						buf.isUsing = false;
    						mEncData.removeAt(i);
    						break;
    					}
    				}
    			}
    		}
    	}
        
    	{
    		EncQueueBuffer buf;
    		buf.isUsing = false;
    		buf.mem = bsFrameMemory;
    		//Mutex::Autolock lock(mQueueLock);
    		mEncData.push_back(buf);
            //ALOGD("buf.mem.getMemory()->getHeapID()=%d", buf.mem->getMemory()->getHeapID());
            //ALOGV("add mEncData.size = %d", mEncData.size());
    	}
    }
	return 0;
}

sp<IMemory> CedarXRecorder::getEncDataHeader()
{
	size_t size = mPpsInfo.nLength + sizeof(int);
    sp<MemoryHeapBase> heap = new MemoryHeapBase(size);
    if (heap == NULL) {
        LOGE("failed to create MemoryDealer");
        return NULL;
    }

    sp<IMemory> ppsInfoMemory = new MemoryBase(heap, 0, size);
    if (ppsInfoMemory == NULL) {
        LOGE("not enough memory for ppsInfo size=%u", size);
        return NULL;
    }

	uint8_t *ptr_dst = (uint8_t *)ppsInfoMemory->pointer();
	memcpy(ptr_dst, mPpsInfo.pBuffer, mPpsInfo.nLength);
	ptr_dst += mPpsInfo.nLength;
	memcpy(ptr_dst, &mPpsInfo.nLength, sizeof(int));
	return ppsInfoMemory;
}

status_t CedarXRecorder::setVideoEncodingBitRateSync(int bitRate)
{
    ALOGV("setVideoEncodingBitRateSync(%d)", bitRate);
    return setParamVideoEncodingBitRate(bitRate);
}

status_t CedarXRecorder::setVideoFrameRateSync(int frames_per_second) 
{
    ALOGV("setVideoFrameRateSync: %d", frames_per_second);
    return setVideoFrameRate(frames_per_second);
}

status_t CedarXRecorder::setVideoEncodingIFramesNumberIntervalSync(int nMaxKeyItl)
{
    ALOGV("setVideoEncodingIFramesNumberIntervalSync(%d)", nMaxKeyItl);
    return setParamVideoIFramesNumberInterval(nMaxKeyItl);
}

status_t CedarXRecorder::setOutputFileSync(int fd, int64_t fallocateLength, int muxerId)
{
    LOGV("setOutputFileSync: %d", fd);
	int ret;

	if (mPrepared == true) 
    {
        CdxFdT fileDesc;
        memset(&fileDesc, 0, sizeof(CdxFdT));
        fileDesc.mFd = fd;
        fileDesc.mnFallocateLen = fallocateLength;
        fileDesc.mMuxerId = muxerId;
        //ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_SAVE_FILE, (unsigned int)fd, fallocateLength);
        ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_SAVE_FILE, 0, (unsigned int)&fileDesc);
		if (ret != OK) 
        {
			ALOGE("SET SAVE FILE failed");
			return ret;
		}
//        ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_FALLOCATE_FILE, 0, (unsigned int)fileLength);
//		if (ret != OK) 
//        {
//			ALOGE("FALLOCATE_FILE failed");
//			return ret;
//		}
	}
    else
    {
        ALOGE("(f:%s, l:%d) fatal error! why call setOutputFileSync() when mPrepared[%d]!=true!", __FUNCTION__, __LINE__, mPrepared);
    }

    return OK;
}

status_t CedarXRecorder::setOutputFileSync(const char* path, int64_t fallocateLength, int muxerId)
{
    ALOGV("(f:%s, l:%d) path[%s], len[%lld], muxerId[%d]!", __FUNCTION__, __LINE__, path, fallocateLength, muxerId);
    int ret;

    if (mPrepared == true) 
    {
        CdxFdT fileDesc;
        memset(&fileDesc, 0, sizeof(CdxFdT));
        fileDesc.mPath = (char*)path;
        fileDesc.mFd = -1;
        fileDesc.mnFallocateLen = fallocateLength;
        fileDesc.mMuxerId = muxerId;
        ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_SAVE_FILE, 0, (unsigned int)&fileDesc);
        if (ret != OK) 
        {
            ALOGE("SET SAVE FILE failed");
            return ret;
        }
    }
    else
    {
        ALOGE("(f:%s, l:%d) fatal error! why call setOutputFileSync() when mPrepared[%d]!=true!", __FUNCTION__, __LINE__, mPrepared);
    }

    return OK;
}

int CedarXRecorder::addOutputFormatAndOutputSink(int of, int fd, int FallocateLen, bool callback_out_flag)
{
    ALOGD("(f:%s, l:%d) (of:%d, fd:%d, FallocateLen:%d, callback_out_flag:%d)", __FUNCTION__, __LINE__, of, fd, FallocateLen, callback_out_flag);
    OutputSinkInfo sinkInfo;
    bool bValidFlag;
    int i = 0;
    int nSelect = -1;
    int nSize;
    if(fd < 0 && false == callback_out_flag)
    {
        ALOGE("(f:%s, l:%d) fatal error! no sink!", __FUNCTION__, __LINE__);
        return -1;
    }
    if(fd >= 0 && true == callback_out_flag)
    {
        ALOGE("(f:%s, l:%d) fatal error! one muxer cannot support two sink methods!", __FUNCTION__, __LINE__);
        return -1;
    }
    //find if the same output_format sinkInfo exist
    nSize = mOutputSinkInfoVector.size();
    for(i=0; i<nSize; i++)
    {
        if(mOutputSinkInfoVector[i].nOutputFormat == of)
        {
            ALOGD("(f:%s, l:%d) Be careful! same outputForamt[%d] exist at array[%d]", __FUNCTION__, __LINE__, of, i);
        }
    }
    sinkInfo.mMuxerId = mMuxerIdCounter;
    sinkInfo.nOutputFormat = (output_format)of;
    sinkInfo.nCallbackOutFlag = callback_out_flag;
    if(fd >= 0)
    {
        sinkInfo.nOutputFd = fd;
        sinkInfo.nFallocateLen = FallocateLen;
        ALOGD("(f:%s, l:%d) i=%d, fd[%d], FallocateLen[%d]!", __FUNCTION__, __LINE__, i, fd, FallocateLen);
    }
    else
    {
        sinkInfo.nOutputFd = -1;
        sinkInfo.nFallocateLen = 0;
    }
    mOutputSinkInfoVector.add(sinkInfo);
    nSelect = mOutputSinkInfoVector.size()-1;
    bValidFlag = checkVectorOutputSinkInfoValid(mOutputSinkInfoVector);
    if(false == bValidFlag)
    {
        ALOGE("(f:%s, l:%d) fatal error! of[%d], fd[%d], callback_out_flag[%d], check code!", __FUNCTION__, __LINE__, of, fd, callback_out_flag);
        //delete inserted one.
        mOutputSinkInfoVector.pop();
        return -1;
    }
    else
    {
        mMuxerIdCounter++;
        if(fd >= 0)
        {
            if(mOutputSinkInfoVector[nSelect].nOutputFd != fd)
            {
                ALOGE("(f:%s, l:%d) fatal error! check code! array[%d]nOutputFd[%d]!=fd[%d], !", __FUNCTION__, __LINE__, nSelect, mOutputSinkInfoVector[nSelect].nOutputFd, fd);
            }
            mOutputSinkInfoVector.editItemAt(nSelect).nOutputFd = dup(fd);
        }
        
        if(true == mPrepared)   // cedarXRecorder status: recording
        {
            CdxOutputSinkInfo cdxSinkInfo;
            cdxSinkInfo.mMuxerId = mOutputSinkInfoVector[nSelect].mMuxerId;
            cdxSinkInfo.nMuxerMode = convertMuxerMode(of);
            cdxSinkInfo.nOutputFd = mOutputSinkInfoVector[nSelect].nOutputFd;
            cdxSinkInfo.nCallbackOutFlag = callback_out_flag;
            cdxSinkInfo.nFallocateLen = FallocateLen;
            int ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_ADD_OUTPUTSINKINFO, (unsigned int)&cdxSinkInfo, 0);
            if(ret!=NO_ERROR)
            {
                ALOGE("(f:%s, l:%d) fatal error! use_CDX_CMD_ADD_OUTPUTSINKINFO fail!", __FUNCTION__, __LINE__);
            }
//            if(cdxSinkInfo.nOutputFd >= 0)
//            {
//                if(false == mRecordFileFlag)
//                {
//                    mRecordFileFlag = true;
//                }
//                else
//                {
//                    ALOGD("(f:%s, l:%d) mRecordFileFlag already true!", __FUNCTION__, __LINE__);
//                }
//                if(false == mResetDurationStatistics)
//                {
//                    mResetDurationStatistics = true;
//                }
//                else
//                {
//                    ALOGD("(f:%s, l:%d) mResetDurationStatistics[%d] already true!!", __FUNCTION__, __LINE__);
//                }
//            }
        }
        return mOutputSinkInfoVector[nSelect].mMuxerId;
    }
}

int CedarXRecorder::addOutputFormatAndOutputSink(int of, const char* path, int FallocateLen, bool callback_out_flag)
{
    ALOGD("(f:%s, l:%d) (of:%d, path:%s, FallocateLen:%d, callback_out_flag:%d)", __FUNCTION__, __LINE__, of, path, FallocateLen, callback_out_flag);
    OutputSinkInfo sinkInfo;
    bool bValidFlag;
    int i = 0;
    int nSelect = -1;
    int nSize;
    if(NULL==path && false == callback_out_flag)
    {
        ALOGE("(f:%s, l:%d) fatal error! no sink!", __FUNCTION__, __LINE__);
        return -1;
    }
    if(path!=NULL && true == callback_out_flag)
    {
        ALOGE("(f:%s, l:%d) fatal error! one muxer cannot support two sink methods!", __FUNCTION__, __LINE__);
        return -1;
    }
    //find if the same output_format sinkInfo exist
    nSize = mOutputSinkInfoVector.size();
    for(i=0; i<nSize; i++)
    {
        if(mOutputSinkInfoVector[i].nOutputFormat == of)
        {
            ALOGD("(f:%s, l:%d) Be careful! same outputForamt[%d] exist at array[%d]", __FUNCTION__, __LINE__, of, i);
        }
    }
    sinkInfo.mMuxerId = mMuxerIdCounter;
    sinkInfo.nOutputFormat = (output_format)of;
    sinkInfo.nCallbackOutFlag = callback_out_flag;
    sinkInfo.nOutputFd = -1;
    if(path)
    {
        sinkInfo.mOutputPath = path;
        sinkInfo.nFallocateLen = FallocateLen;
        ALOGD("(f:%s, l:%d) i=%d, path[%s], FallocateLen[%d]!", __FUNCTION__, __LINE__, i, sinkInfo.mOutputPath.string(), sinkInfo.nFallocateLen);
    }
    else
    {
        sinkInfo.mOutputPath.clear();
        sinkInfo.nFallocateLen = 0;
    }
    mOutputSinkInfoVector.add(sinkInfo);
    nSelect = mOutputSinkInfoVector.size()-1;
    bValidFlag = checkVectorOutputSinkInfoValid(mOutputSinkInfoVector);
    if(false == bValidFlag)
    {
        ALOGE("(f:%s, l:%d) fatal error! of[%d], path[%s], callback_out_flag[%d], check code!", __FUNCTION__, __LINE__, of, path, callback_out_flag);
        //delete inserted one.
        mOutputSinkInfoVector.pop();
        return -1;
    }
    else
    {
        mMuxerIdCounter++;
        
        if(true == mPrepared)   // cedarXRecorder status: recording
        {
            CdxOutputSinkInfo cdxSinkInfo;
            cdxSinkInfo.mMuxerId = mOutputSinkInfoVector[nSelect].mMuxerId;
            cdxSinkInfo.nMuxerMode = convertMuxerMode(of);
            cdxSinkInfo.nOutputFd = mOutputSinkInfoVector[nSelect].nOutputFd;
            if(mOutputSinkInfoVector[nSelect].mOutputPath.isEmpty())
            {
                cdxSinkInfo.mPath = NULL;
            }
            else
            {
                cdxSinkInfo.mPath = (char*)mOutputSinkInfoVector[nSelect].mOutputPath.string();
            }
            cdxSinkInfo.nCallbackOutFlag = callback_out_flag;
            cdxSinkInfo.nFallocateLen = FallocateLen;
            int ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_ADD_OUTPUTSINKINFO, (unsigned int)&cdxSinkInfo, 0);
            if(ret!=NO_ERROR)
            {
                ALOGE("(f:%s, l:%d) fatal error! use_CDX_CMD_ADD_OUTPUTSINKINFO fail!", __FUNCTION__, __LINE__);
            }
        }
        ALOGD("(f:%s, l:%d) nSelect[%d] muxerId[%d]!", __FUNCTION__, __LINE__, nSelect, mOutputSinkInfoVector[nSelect].mMuxerId);
        return mOutputSinkInfoVector[nSelect].mMuxerId;
    }
}

status_t CedarXRecorder::removeOutputFormatAndOutputSink(int muxerId)
{
    ALOGV("removeOutputFormatAndOutputSink(muxerId:%d)", muxerId);
    status_t ret;
    OutputSinkInfo sinkInfoBackup;
    int i = 0;
    bool bValidFlag;
    int nSize = mOutputSinkInfoVector.size();
    //find exist sinkInfo
    for(i=0;i<nSize;i++)
    {
        if(mOutputSinkInfoVector[i].mMuxerId == muxerId)
        {
            ALOGD("(f:%s, l:%d) array[%d] find an exist muxerId[%d], will delete:\n"
                "format[0x%x], fd[%d], fallocateLen[%d], callbackOutFlag[%d]", __FUNCTION__, __LINE__, i, muxerId,
                mOutputSinkInfoVector[i].nOutputFormat, mOutputSinkInfoVector[i].nOutputFd, 
                mOutputSinkInfoVector[i].nFallocateLen, mOutputSinkInfoVector[i].nCallbackOutFlag);
            break;
        }
    }
    if(i < nSize)   //find an exist one
    {
        sinkInfoBackup = mOutputSinkInfoVector[i];
        mOutputSinkInfoVector.removeAt(i);
    }
    else
    {
        ALOGE("(f:%s, l:%d) fatal error! not find an exist muxerId[%d]", __FUNCTION__, __LINE__, muxerId);
        return BAD_VALUE;
    }

//    bValidFlag = checkVectorOutputSinkInfoValid(mOutputSinkInfoVector);
//    if(false == bValidFlag)
//    {
//        ALOGE("(f:%s, l:%d) fatal error! sinkInfo invalid now, can't accept!", __FUNCTION__, __LINE__);
//        mOutputSinkInfoVector.insertAt(sinkInfoBackup, i);
//        return BAD_VALUE;
//    }
//    else
//    {
        if(true == mStarted)
        {
            int ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_REMOVE_OUTPUTSINKINFO, muxerId, 0);
        	if(ret != OK)
        	{
                ALOGE("(f:%s, l:%d) fatal error! cdx remove muxerId[%d] fail!", __FUNCTION__, __LINE__, muxerId);
        	}
        }
        //if ok, then close fd if necessary.
        if(sinkInfoBackup.nOutputFd >= 0)
        {
            ALOGD("(f:%s, l:%d) close fd[%d]", __FUNCTION__, __LINE__, sinkInfoBackup.nOutputFd);
            ::close(sinkInfoBackup.nOutputFd);
            sinkInfoBackup.nOutputFd = -1;
//            if(true == mRecordFileFlag)
//            {
//                mRecordFileFlag = false;
//            }
//            else
//            {
//                ALOGE("(f:%s, l:%d) fatal error! check code! mRecordFileFlag[%d] should not be false!!", __FUNCTION__, __LINE__, mRecordFileFlag);
//            }
//            if(true == mResetDurationStatistics)
//            {
//                ALOGE("(f:%s, l:%d) fatal error! check code! why mResetDurationStatistics[%d]==true!", __FUNCTION__, __LINE__, mResetDurationStatistics);
//            }
        }
        return OK;
//    }
}

status_t CedarXRecorder::setMaxDuration(int max_duration_ms)
{
    ALOGV("setMaxDuration(%d)", max_duration_ms);
	return setParamMaxFileDurationUs(1000LL * max_duration_ms);
}

status_t CedarXRecorder::setMaxFileSize(int64_t max_filesize_bytes)
{
    ALOGV("setMaxFileSize(%lld)", max_filesize_bytes);
	return setParamMaxFileSizeBytes(max_filesize_bytes);
}

status_t CedarXRecorder::setParamImpactDurationBfTime(int bftime)
{
	if (bftime < 0 || bftime > 10 * 1000) {
		ALOGE("<F:%s, L:%d>Invalid bftime(%d)", __FUNCTION__, __LINE__, bftime);
		return BAD_VALUE;
	}
	mImpactFileDuration[0] = bftime;
	return NO_ERROR;
}

status_t CedarXRecorder::setParamImpactDurationAfTime(int aftime)
{
	if (aftime < 0) {
		ALOGE("<F:%s, L:%d>Invalid aftime(%d)", __FUNCTION__, __LINE__, aftime);
		return BAD_VALUE;
	}
	mImpactFileDuration[1] = aftime;
	return NO_ERROR;
}

status_t CedarXRecorder::setImpactFileDuration(int bfTimeMs, int afTimeMs)
{
    ALOGV("setImpactFileDuration(%d, %d)", bfTimeMs, afTimeMs);
	if (bfTimeMs > 10 * 1000) {
		bfTimeMs = 10 * 1000;
	}
	mImpactFileDuration[0] = bfTimeMs;
	mImpactFileDuration[1] = afTimeMs;
	//if (mPrepared) {
	//	mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_IMPACT_FILE_DURATION, (unsigned int)mImpactFileDuration, 0);
	//}
	return NO_ERROR;
}

status_t CedarXRecorder::setImpactOutputFile(int fd, int64_t fallocateLength, int muxerId)
{
    LOGV("setImpactOutputFile: %d", fd);
	int ret;

	if (mPrepared == true) {
        CdxFdT fileDesc;
        memset(&fileDesc, 0, sizeof(CdxFdT));
        fileDesc.mFd = fd;
        fileDesc.mPath = NULL;
        fileDesc.mnFallocateLen = (int)fallocateLength;
        fileDesc.mMuxerId = muxerId;
        fileDesc.mIsImpact = 1;
        ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_SAVE_IMPACT_FILE, 0, (unsigned int)&fileDesc);    //CDX_CMD_SET_SAVE_FILE
		if (ret != OK) {
			ALOGE("cdx_CMD_SET_SAVE_FILE failed");
			return UNKNOWN_ERROR;
		}
//        ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_FALLOCATE_FILE, 2, (unsigned int)fileLength);
//		if (ret != OK) {
//			ALOGE("CDX_CMD_FALLOCATE_FILE failed");
//			return UNKNOWN_ERROR;
//		}
		ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SWITCH_FILE, muxerId, 0);
		if (ret != OK) {
			ALOGE("CDX CMD_SWITCH_FILE failed");
			return UNKNOWN_ERROR;
		}
	} else {
        ALOGE("<F:%s, L:%d> fatal error! why call setImpactOutputFile() when mPrepared[%d]!=true!", __FUNCTION__, __LINE__, mPrepared);
		return INVALID_OPERATION;
    }

    return OK;
}

status_t CedarXRecorder::setImpactOutputFile(const char* path, int64_t fallocateLength, int muxerId)
{
    ALOGV("(f:%s, l:%d) path[%s], len[%lld], muxerId[%d]!", __FUNCTION__, __LINE__, path, fallocateLength, muxerId);
    int ret;

	if (mPrepared == true) 
    {
        CdxFdT fileDesc;
        memset(&fileDesc, 0, sizeof(CdxFdT));
        fileDesc.mFd = -1;
        fileDesc.mPath = (char*)path;
        fileDesc.mnFallocateLen = (int)fallocateLength;
        fileDesc.mMuxerId = muxerId;
        fileDesc.mIsImpact = 1;
        ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_SAVE_IMPACT_FILE, 0, (unsigned int)&fileDesc);    //CDX_CMD_SET_SAVE_FILE
		if (ret != OK) 
        {
			ALOGE("cdx_CMD_SET_SAVE_FILE failed");
			return UNKNOWN_ERROR;
		}
		ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SWITCH_FILE, muxerId, 0);
		if (ret != OK) 
        {
			ALOGE("CDX CMD_SWITCH_FILE failed");
			return UNKNOWN_ERROR;
		}
	} 
    else 
    {
        ALOGE("<F:%s, L:%d> fatal error! why call setImpactOutputFile() when mPrepared[%d]!=true!", __FUNCTION__, __LINE__, mPrepared);
		return INVALID_OPERATION;
    }

    return OK;
}

/* value: 0-main channel; 1 sub channel */
status_t CedarXRecorder::setSourceChannel(int value)
{
    LOGV("setSourceChannel: %d", value);
    if (value < 0 || value > 1) {
        ALOGE("<F:%s, L:%d>Source channel must be 0 or 1(%d)!", __FUNCTION__, __LINE__, value);
        return BAD_VALUE;
    }
    if (mPrepared) {
        ALOGE("<F:%s, L:%d>Must set source channel before prepare!!", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }
    mSourceChannel = value;
    return NO_ERROR;
}

status_t CedarXRecorder::enableDynamicBitRateControl(bool bEnable)
{
    ALOGV("enableDynamicBitRateControl: %d", bEnable);
    if(mEnableDBRC == bEnable)
    {
        return NO_ERROR;
    }
    mEnableDBRC = bEnable;
    if (mPrepared) 
    {
        mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_ENABLE_DYNAMIC_BITRATE_CONTROL, mEnableDBRC, 0);
    }
    return NO_ERROR;
}

#if 0

extern "C" int CedarXRecReadAudioBuffer(void *p, void *pbuf, int *size, int64_t *timeStamp)
{
	return ((android::CedarXRecorder*)p)->CedarXReadAudioBuffer(pbuf, size, timeStamp);
}

extern "C" void CedarXRecReleaseOneFrame(void *p, int index) 
{
	((android::CedarXRecorder*)p)->CedarXReleaseFrame(index);
}

#else

int CedarXRecorder::CedarXRecorderCallback(int event, void *info)
{
	int ret = 0;
	int *para = (int*)info;

	//LOGV("----------CedarXRecorderCallback event:%d info:%p\n", event, info);

	switch (event) {
	case CDX_EVENT_READ_AUDIO_BUFFER:
		CedarXReadAudioBuffer((void *)para[0], (int*)para[1], (int64_t*)para[2]);
		break;
	case CDX_EVENT_RELEASE_VIDEO_BUFFER:
		CedarXReleaseFrame(*para);
		break;
	case CDX_EVENT_MEDIASOURCE_READ:
		ret = readMediaBufferCallback(info);
		break;
	case CDX_EVENT_BSFRAME_AVAILABLE:
		if (pushOneBsFrame(0) != 0) {
			break;
		}
		mListener->notify(MEDIA_RECORDER_VENDOR_EVENT_BSFRAME_AVAILABLE, (int)info, 0);
		break;

	case CDX_EVENT_NEED_NEXT_FD:
		mListener->notify(MEDIA_RECORDER_EVENT_INFO, MEDIA_RECORDER_INFO_NEED_SET_NEXT_FD, (int)info);
		break;

    case CDX_EVENT_REC_VBV_FULL:
		mListener->notify(MEDIA_RECORDER_EVENT_INFO, MEDIA_RECORDER_INFO_DISK_SPEED_TOO_SLOW, (int)info);
        break;

    case CDX_EVENT_WRITE_DISK_ERROR:
		mListener->notify(MEDIA_RECORDER_EVENT_INFO, MEDIA_RECORDER_INFO_WRITE_DISK_ERROR, (int)info);
        break;

	case CDX_EVENT_RECORD_DONE:
		mListener->notify(MEDIA_RECORDER_EVENT_INFO, MEDIA_RECORDER_INFO_RECORD_FILE_DONE, (int)info);
		break;

	default:
		break;
	}

	return ret;
}

extern "C" int CedarXRecorderCallbackWrapper(void *cookie, int event, void *info)
{
	return ((android::CedarXRecorder *)cookie)->CedarXRecorderCallback(event, info);
}
#endif

}  // namespace android

