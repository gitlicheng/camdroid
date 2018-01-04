/*
 * Copyright (C) 2009 The Android Open Source Project
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
#include <CDX_LogNDebug.h>
#define LOG_TAG "CedarXPlayer"
#include <CDX_Debug.h>

#include "CDX_Version.h"

#include <dlfcn.h>
#if (CEDARX_ANDROID_VERSION >= 7)
#include <utils/Trace.h>
#endif 
#include "CedarXPlayer.h"
#include "CedarXNativeRenderer.h"
#include "CedarXSoftwareRenderer.h"

#include <CDX_ErrorType.h>
#include <CDX_Config.h>
//#include <libcedarv.h>
#include <vdecoder.h>
#include <memoryAdapter.h>
#include <CDX_Fileformat.h>

#include <binder/IPCThreadState.h>
#include <media/stagefright/DataSource.h>
#include <media/stagefright/FileSource.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaExtractor.h>
#if (CEDARX_ANDROID_VERSION < 7)
#include <media/stagefright/MediaDebug.h>
#include <media/stagefright/foundation/ADebug.h>
#else
#include <media/stagefright/foundation/ADebug.h>
#endif
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/foundation/ALooper.h>

//#include <AwesomePlayer.h>

#if (CEDARX_ANDROID_VERSION < 7)
#include <surfaceflinger/Surface.h>
#include <surfaceflinger/ISurfaceComposer.h>
#endif

//#include <gui/ISurfaceTexture.h>
//#include <gui/SurfaceTextureClient.h>

#include <include_sft/StreamingSource.h>

//#include <hardware/hwcomposer.h>
#include <stdint.h>
#include <sys/cdefs.h>
#include <hardware/gralloc.h>
#include <hardware/hardware.h>
#include <cutils/native_handle.h>

#include <cutils/properties.h>

#include <CDX_Subtitle.h>
#include <SubtitleRender.h>

//#include "unicode/ucnv.h"
//#include "unicode/ustring.h"

#include <CDX_Component.h>

#define PROP_CHIP_VERSION_KEY  "media.cedarx.chipver"
#define PROP_CONSTRAINT_RES_KEY  "media.cedarx.cons_res"

extern "C" {
extern unsigned int cedarv_address_phy2vir(void *addr);
}

namespace android {

#define MAX_HARDWARE_LAYER_SUPPORT 1
#define TEST_FORCE_GPU_RENDER   0
//#define TEST_FORCE_VDEC_YV12_OUTPUT  1

static int gHardwareLayerRefCounter = 0; //don't touch it

//#define DYNAMIC_ROTATION_ENABLE 1
//static int getmRotation = 0;
//static int dy_rotate_flag=0;
const int ENABLE_HDCP = 0;
extern "C" int CedarXPlayerCallbackWrapper(void *cookie, int event, void *info);


struct CedarXDirectHwRenderer : public CedarXRenderer {
//    CedarXDirectHwRenderer(
//            const sp<ANativeWindow> &nativeWindow, const sp<MetaData> &meta)
//        : mTarget(new CedarXNativeRenderer(nativeWindow, meta)) 
    CedarXDirectHwRenderer(
            const unsigned int hlay, const sp<MetaData> &meta)
        : mTarget(new CedarXNativeRenderer(hlay, meta)) 
    {
    }

    int control(int cmd, int para) {
        return mTarget->control(cmd, para);
    }

    void render(const void *data, size_t size) {
        mTarget->render(data, size, NULL);
    }
    int dequeueFrame(ANativeWindowBufferCedarXWrapper *pObject){
        //return mTarget->dequeueFrame(pObject);
        return -1;
    }
    int enqueueFrame(ANativeWindowBufferCedarXWrapper *pObject)
    {
        //return mTarget->enqueueFrame(pObject);
        return -1;
    }
protected:
    virtual ~CedarXDirectHwRenderer() {
        delete mTarget;
        mTarget = NULL;
    }

private:
    CedarXNativeRenderer *mTarget;

    CedarXDirectHwRenderer(const CedarXDirectHwRenderer &);
    CedarXDirectHwRenderer &operator=(const CedarXDirectHwRenderer &);
};

struct CedarXLocalRenderer : public CedarXRenderer {
    CedarXLocalRenderer(
            unsigned int hlay, const sp<MetaData> &meta)
        : mTarget(new CedarXSoftwareRenderer(hlay, meta)) {
    }

    int control(int cmd, int para) {
        //return mTarget->control(cmd, para);
    	return 0;
    }

    void render(const void *data, size_t size) {
        mTarget->render(data, size, NULL);
    }
    int dequeueFrame(ANativeWindowBufferCedarXWrapper *pObject){
        return mTarget->dequeueFrame(pObject);
    }
    int enqueueFrame(ANativeWindowBufferCedarXWrapper *pObject)
    {
        return mTarget->enqueueFrame(pObject);
    }
protected:
    virtual ~CedarXLocalRenderer() {
        delete mTarget;
        mTarget = NULL;
    }

private:
    CedarXSoftwareRenderer *mTarget;

    CedarXLocalRenderer(const CedarXLocalRenderer &);
    CedarXLocalRenderer &operator=(const CedarXLocalRenderer &);;
};

#if 0
#define GET_CALLING_PID	(IPCThreadState::self()->getCallingPid())
static int getCallingProcessName(char *name)
{
	char proc_node[128];

	if (name == 0)
	{
		LOGE("error in params");
		return -1;
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
		LOGV("Calling process is: %s", name);
        return OK;
	}
	else 
	{
		LOGE("Obtain calling process failed");
        return -1;
    }
}

int IfContextNeedGPURender()
{
    int GPURenderFlag = 0;
    int ret;
    char mCallingProcessName[128];    
    memset(mCallingProcessName, 0, sizeof(mCallingProcessName));	
    ret = getCallingProcessName(mCallingProcessName);	
    if(ret != OK)
    {
        return 0;
    }
    //LOGD("~~~~~ mCallingProcessName %s", mCallingProcessName);

    if (strcmp(mCallingProcessName, "com.google.android.youtube") == 0)
    {           
        LOGV("context is youtube");
        GPURenderFlag = 1;
    }
    else
    {
        GPURenderFlag = 0;
    }
    return GPURenderFlag;
}
#endif
CedarXPlayer::CedarXPlayer()
	: mQueueStarted(false)
	, mVideoLayerId(-1)
	, mVideoRendererIsPreview(false)
	, mSftSource(NULL)
	, mAudioPlayer(NULL)
	, mFlags(0)
	, mExtractorFlags(0)
	, mCanSeek(0)
{
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	LOGV("Construction, this %p", this);
	mAudioSink = NULL;
	//mAwesomePlayer = NULL;

	mExtendMember = (CedarXPlayerExtendMember *)malloc(sizeof(CedarXPlayerExtendMember));
	memset(mExtendMember, 0, sizeof(CedarXPlayerExtendMember));

	reset_l();

	LOGD("=============================================\n");
	LOGD("CDX_PLATFORM: %s\n", CDX_PLATFORM);
	LOGD("CDX_SVN_REPOSITORY: %s\n", CDX_SVN_REPOSITORY);
	LOGD("CDX_SVN_VERSION: %s\n", CDX_SVN_VERSION);
	LOGD("CDX_SVN_COMMITTER: %s\n", CDX_SVN_COMMITTER);
	LOGD("CDX_SVN_DATE: %s\n", CDX_SVN_DATE);
	LOGD("CDX_RELEASE_AUTHOR: %s\n", CDX_RELEASE_AUTHOR);
	LOGD("=============================================\n");
	
	CDXPlayer_Create((void**)&mPlayer);
    
	mPlayer->control(mPlayer, CDX_CMD_REGISTER_CALLBACK, (unsigned int)&CedarXPlayerCallbackWrapper, (unsigned int)this);
	isCedarXInitialized = true;
#if (defined(__CHIP_VERSION_F81))
    mMaxOutputWidth = 1024;
    mMaxOutputHeight = 768;
#else
    mMaxOutputWidth = 0;
    mMaxOutputHeight = 0;
#endif
	mDisableXXXX = 0;
	//mScreenID = 0;
	mVideoRenderer = NULL;

    _3d_mode							= 0;
//    display_3d_mode						= 0;
//    anaglagh_type						= 0;
//    anaglagh_en							= 0;
//    wait_anaglagh_display_change		= 0;
    mVpsspeed                           = 0;
	mRenderToDE                         = 0;

	mVideoScalingMode = NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW;
    //* for cache policy of network streaming playing.
    mMaxCacheSize                       = 0;
    mStartPlayCacheSize                 = 0;
    mMinCacheSize                       = 0;
    mCacheTime                          = 0;
    mUseDefautlCachePolicy              = 0;

    //mDynamicRotation                    = 0;
    mInitRotation                    	= 0;
    mAudioTrackIndex 	= 0;
    mSubTrackIndex 		= 0;

    mIsDRMMedia		= false;
    mHDMIPlugged  	= false;
    //mHDMIListener 	= new HDMIListerner();
}

CedarXPlayer::~CedarXPlayer() {
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	LOGV("~CedarXPlayer()");
//	if(mAwesomePlayer) {
//		delete mAwesomePlayer;
//		mAwesomePlayer = NULL;
//	}

	if (mSftSource != NULL) {
		mSftSource.clear();
		mSftSource = NULL;
	}

	if(isCedarXInitialized){
		mPlayer->control(mPlayer, CDX_CMD_STOP_ASYNC, 0, 0);
		CDXPlayer_Destroy(mPlayer);
		mPlayer = NULL;
		isCedarXInitialized = false;
	}
//    if(pCedarXPlayerAdapter)
//    {
//        delete pCedarXPlayerAdapter;
//        pCedarXPlayerAdapter = NULL;
//    }

	if (mAudioPlayer) {
		LOGV("delete mAudioPlayer");
		delete mAudioPlayer;
		mAudioPlayer = NULL;
	}

	if(mExtendMember != NULL){
		free(mExtendMember);
		mExtendMember = NULL;
	}
//	if(mHDMIListener) {
//		mHDMIListener->setNotifyCallback(0, 0);
//		mHDMIListener->stop();
//		delete mHDMIListener;
//		mHDMIListener = NULL;
//	}
	//LOGV("Deconstruction %x",mFlags);
}

void CedarXPlayer::setUID(uid_t uid) {
    LOGV("CedarXPlayer running on behalf of uid %d", uid);

    mUID = uid;
    mUIDValid = true;
}

void CedarXPlayer::setListener(const wp<MediaPlayerBase> &listener) {
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	Mutex::Autolock autoLock(mLock);
	mListener = listener;
}

status_t CedarXPlayer::setDataSource(const char *uri, const KeyedVector<
		String8, String8> *headers) {
	//Mutex::Autolock autoLock(mLock);
	LOGV("CedarXPlayer::setDataSource (%s)", uri);
	
	memset(&mMediaInfo, 0, sizeof(CedarXMediaInformations));
	
	mUri = uri;

	if(!strncasecmp("file:///", uri, 8)) {
		mUri = &uri[8];
	}
	mSourceType = SOURCETYPE_URL;
	if (headers) {
	    mUriHeaders = *headers;
	    mPlayer->control(mPlayer, CDX_CMD_SET_URL_HEADERS, (unsigned int)&mUriHeaders, 0);
	} else {
		mPlayer->control(mPlayer, CDX_CMD_SET_URL_HEADERS, (unsigned int)0, 0);
	}
//	mUriHeaders.add(String8("SeekToPos"),String8("0"));
	ssize_t index = mUriHeaders.indexOfKey(String8("SeekToPos"));
	if(index >= 0 && !strncasecmp(mUriHeaders.valueAt(index), "0", 1)) {
		//DLNA do not support seek
		mPlayer->control(mPlayer, CDX_CMD_SET_DISABLE_SEEK, 1, 0);
	}
	mPlayer->control(mPlayer, CDX_SET_DATASOURCE_URL, (unsigned int)mUri.string(), 0);
	return OK;
}

status_t CedarXPlayer::setDataSource(int fd, int64_t offset, int64_t length) {
	//Mutex::Autolock autoLock(mLock);
	LOGV("CedarXPlayer::setDataSource fd");
	CedarXExternFdDesc ext_fd_desc;
	
	memset(&mMediaInfo, 0, sizeof(CedarXMediaInformations));
	
	ext_fd_desc.fd = fd;
	ext_fd_desc.offset = offset;
	ext_fd_desc.length = length;
	mSourceType = SOURCETYPE_FD;
	setDataSource_pre();
	status_t ret = OK;
	if((ret = mPlayer->control(mPlayer, CDX_SET_DATASOURCE_FD, (unsigned int)&ext_fd_desc, 0))) {
		if (ret == CDX_ERROR_UNSUPPORT_RAWMUSIC) {
			//fd music  
//			mAwesomePlayer = new AwesomePlayer;
//			mAwesomePlayer->setListener(mListener);
//			if(mAudioSink != NULL) {
//				mAwesomePlayer->setAudioSink(mAudioSink);
//			}
//
//			if(mFlags & LOOPING) {
//				mAwesomePlayer->setLooping(!!(mFlags & LOOPING));
//			}
//			mAwesomePlayer->setDataSource(dup(fd), offset, length);
//			mAwesomePlayer->prepareAsync();
            ALOGE("(f:%s, l:%d) no awesomePlayer now!", __FUNCTION__, __LINE__);
			return OK;			
		}
		else {
			return UNKNOWN_ERROR;
		}
	}

	setDataSource_post();
	return OK;
}

status_t CedarXPlayer::setDataSource(const sp<IStreamSource> &source) {
    ALOGE("(f:%s, l:%d) fatal error! not support IStreamSource now!", __FUNCTION__, __LINE__);
    #if 0
	RefBase *refValue;
	//Mutex::Autolock autoLock(mLock);
	LOGV("CedarXPlayer::setDataSource stream");
	
	memset(&mMediaInfo, 0, sizeof(CedarXMediaInformations));

	mSftSource = new StreamingSource(source);
	refValue = mSftSource.get();

	mSourceType = SOURCETYPE_SFT_STREAM;
	mPlayer->control(mPlayer, CDX_SET_DATASOURCE_SFT_STREAM, (unsigned int)refValue, 0);
    #endif
	return OK;
}


status_t CedarXPlayer::setDataSource_pre()
{
	//do common init here.
	return OK;
}
status_t CedarXPlayer::setDataSource_post()
{
	mPlayer->control(mPlayer, CDX_CMD_GET_MEDIAINFO, (unsigned int)&mMediaInfo, 0);
	return OK;
}

status_t CedarXPlayer::setParameter(int key, const Parcel &request)
{
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    status_t ret = OK;
    switch(key)
    {
        case PARAM_KEY_ENCRYPT_FILE_TYPE_DISABLE:
        {
            mExtendMember->encrypt_type = PARAM_KEY_ENCRYPT_FILE_TYPE_DISABLE;
            LOGV("setParameter: encrypt_type = [%d]",mExtendMember->encrypt_type);
            ret = OK;
            break;
        }
        case PARAM_KEY_ENCRYPT_ENTIRE_FILE_TYPE:
        {
            mExtendMember->encrypt_type = PARAM_KEY_ENCRYPT_ENTIRE_FILE_TYPE;
            mExtendMember->encrypt_file_format = request.readInt32();
            LOGV("setParameter: encrypt_type = [%d]",mExtendMember->encrypt_type);
            LOGV("setParameter: encrypt_file_format = [%d]",mExtendMember->encrypt_file_format);
            ret = OK;
            break;
        }
        case PARAM_KEY_ENCRYPT_PART_FILE_TYPE:
        {
            mExtendMember->encrypt_type = PARAM_KEY_ENCRYPT_PART_FILE_TYPE;
            mExtendMember->encrypt_file_format = request.readInt32();
            LOGV("setParameter: encrypt_type = [%d]",mExtendMember->encrypt_type);
            LOGV("setParameter: encrypt_file_format = [%d]",mExtendMember->encrypt_file_format);
            ret = OK;
            break;
        }
        case PARAM_KEY_SET_AV_SYNC:
        {
        	if(mPlayer != NULL) {
        		int32_t av_sync = request.readInt32();
        		mPlayer->control(mPlayer, CDX_CMD_SET_AV_SYNC, av_sync, 0);
        	}
        	break;
        }
        case PARAM_KEY_ENABLE_KEEP_FRAME:
        {
        	break;
        }
        case PARAM_KEY_ENABLE_BOOTANIMATION:
        {
            mExtendMember->bootanimation_enable = 1;
            LOGV("setParameter: bootanimation_enable = [%d]",mExtendMember->bootanimation_enable);
            ret = OK;
            break;
        }

        case PARAM_KEY_CLEAR_BUFFER:
        	if(mPlayer != NULL) {
        		int32_t clear_buffer = request.readInt32();
        		mPlayer->control(mPlayer, CDX_CMD_CLEAR_BUFFER_ASYNC, clear_buffer, 0);
        	}
        	break;

        case PARAM_KEY_SWITCH_CHANNEL:
        	if(mPlayer != NULL) {
        		int32_t channel = request.readInt32();
        		LOGV("PARAM_KEY_SWITCH_CHANNEL, %d", channel);
        		mPlayer->control(mPlayer, CDX_CMD_SWITCH_AUDIO_CHANNEL, channel, 0);
        	}
        	break;
        default:
        {
            LOGW("unknown Key[0x%x] of setParameter", key);
            ret = ERROR_UNSUPPORTED;
            break;
        }
    }
	return ret;
}

status_t CedarXPlayer::getParameter(int key, Parcel *reply) {
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	return OK;
}

void CedarXPlayer::reset() {
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	//Mutex::Autolock autoLock(mLock);
	LOGV("RESET????, context: %p",this);

//	if(mAwesomePlayer) {
//		mAwesomePlayer->reset();
//	}

	if (mPlayer != NULL) {
		mPlayer->control(mPlayer, CDX_CMD_RESET, 0, 0);

		if (isCedarXInitialized) {
			mPlayer->control(mPlayer, CDX_CMD_STOP_ASYNC, 0, 0);
			CDXPlayer_Destroy(mPlayer);
			mPlayer = NULL;
			isCedarXInitialized = false;
		}
	}

	reset_l();
}

void CedarXPlayer::reset_l() {
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	LOGV("reset_l");

	mPlayerState = PLAYER_STATE_UNKOWN;
	pause_l(true);

	{
		Mutex::Autolock autoLock(mLockNativeWindow);
		if (mVideoRenderer != NULL)
		{
			mVideoRenderer->control(VIDEORENDER_CMD_SHOW, 0);
			mVideoRenderer.clear();
			mVideoRenderer = NULL;
		}

		LOGV("mUseHardwareLayer:%d gHardwareLayerRefCounter:%d",mExtendMember->mUseHardwareLayer,gHardwareLayerRefCounter);
		if (mExtendMember->mUseHardwareLayer) {
			gHardwareLayerRefCounter--;
			mExtendMember->mUseHardwareLayer = 0;
		}
	}

	if(mAudioPlayer){
		delete mAudioPlayer;
		mAudioPlayer = NULL;
	}
	LOGV("RESET End");

	mDurationUs = 0;
	mFlags = 0;
	mFlags |= RESTORE_CONTROL_PARA;
	mVideoWidth = mVideoHeight = -1;
	mVideoTimeUs = 0;

	mTagPlay = 1;
	mSeeking = false;
	mSeekNotificationSent = false;
	mSeekTimeUs = 0;
	mLastValidPosition = 0;

	mAudioTrackIndex = 0;
	mBitrate = -1;
	mUri.setTo("");
	mUriHeaders.clear();

	if (mSftSource != NULL) {
		mSftSource.clear();
		mSftSource = NULL;
	}

    //memset(&mSubtitleParameter, 0, sizeof(struct SubtitleParameter));
    mSubtitleParameter.mSubtitleCharset.clear();
    mSubtitleParameter.mSubtitleDelay = 0;
    mSubtitleParameter.mSubtitleIndex = 0;
    mSubtitleParameter.mSubtitleGate = 0;
}

void CedarXPlayer::notifyListener_l(int msg, int ext1, int ext2) {
    LOGV("(f:%s, l:%d), msg=%d, ext1=%d, ext2=%d", __FUNCTION__, __LINE__, msg, ext1, ext2);
	if (mListener != NULL) {
		sp<MediaPlayerBase> listener = mListener.promote();

		if (listener != NULL) {
			if(mSourceType == SOURCETYPE_SFT_STREAM
				&& msg== MEDIA_INFO && (ext1 == MEDIA_INFO_BUFFERING_START
						|| ext1 == MEDIA_INFO_BUFFERING_END
						|| ext1 == MEDIA_BUFFERING_UPDATE)) {
				LOGV("skip notifyListerner");
				return;
			}

			//if(msg != MEDIA_BUFFERING_UPDATE)
			listener->sendEvent(msg, ext1, ext2);
		}
	}
}

status_t CedarXPlayer::play() {
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    //LOGD("play()!");
	SuspensionState *state = &mSuspensionState;

//	if(mAwesomePlayer) {
//		return mAwesomePlayer->play();
//	}

	if(mFlags & NATIVE_SUSPENDING) {
		LOGW("you has been suspend by other's");
		mFlags &= ~NATIVE_SUSPENDING;
		state->mFlags |= PLAYING;
		return resume();
	}

	Mutex::Autolock autoLock(mLock);

	mFlags &= ~CACHE_UNDERRUN;

	status_t ret = play_l(CDX_CMD_START_ASYNC);

	LOGV("CedarXPlayer::play() end");
	return ret;
}

status_t CedarXPlayer::play_l(int command)
{
	LOGV("CedarXPlayer::play_l()");
    
	if (mFlags & PLAYING)
	{
		return OK;
	}

	int outputSetting = 0;

	if (mSourceType == SOURCETYPE_SFT_STREAM)
	{
		if (!(mFlags & (PREPARED | PREPARING)))
		{
			mFlags |= PREPARING;
			mIsAsyncPrepare = true;

			//mPlayer->control(mPlayer, CDX_CMD_SET_VIDEO_OUTPUT_SETTING, CEDARX_OUTPUT_SETTING_MODE_PLANNER, 0);
			#if 0
			if(mNativeWindow != NULL)
			{
				Mutex::Autolock autoLock(mLockNativeWindow);
//				if (0 == mNativeWindow->perform(mNativeWindow.get(), NATIVE_WINDOW_GETPARAMETER, NATIVE_WINDOW_CMD_GET_SURFACE_TEXTURE_TYPE, 0)
//                    || TEST_FORCE_GPU_RENDER)
                if(1)
					outputSetting |= CEDARX_OUTPUT_SETTING_MODE_PLANNER;
				else
				{
					if (gHardwareLayerRefCounter < MAX_HARDWARE_LAYER_SUPPORT)
					{
						gHardwareLayerRefCounter++;
						mExtendMember->mUseHardwareLayer = 1;
					}
					else
						outputSetting |= CEDARX_OUTPUT_SETTING_MODE_PLANNER;
				}
			}
			else
				outputSetting |= CEDARX_OUTPUT_SETTING_MODE_PLANNER;
            #endif

			//mPlayer->control(mPlayer, CDX_CMD_SET_VIDEO_OUTPUT_SETTING, outputSetting, 0);
			mPlayer->control(mPlayer, CDX_CMD_PREPARE_ASYNC, 0, 0);
		}
	}
	else if (!(mFlags & PREPARED))
	{
        status_t err = prepare_l();

        if (err != OK)
            return err;
    }

	mFlags |= PLAYING;

	if(mAudioPlayer)
	{
		mAudioPlayer->resume();
		mAudioPlayer->setPlaybackEos(false);
	}

	if(mFlags & RESTORE_CONTROL_PARA){
		if(mMediaInfo.mSubtitleStreamCount > 0) {
			LOGV("Restore control parameter!");
			if(mSubtitleParameter.mSubtitleDelay != 0){
				setSubDelay(mSubtitleParameter.mSubtitleDelay);
			}
            if(!mSubtitleParameter.mSubtitleCharset.isEmpty())
            {
                ALOGV("-------mSubtitleParameter.mSubtitleCharset: %s", mSubtitleParameter.mSubtitleCharset.string());
                setSubCharset(mSubtitleParameter.mSubtitleCharset.string());
            }

//			if(mSubtitleParameter.mSubtitleColor != 0){
//				LOGV("-------mSubtitleParameter.mSubtitleColor: %x",mSubtitleParameter.mSubtitleColor);
//				setSubColor(mSubtitleParameter.mSubtitleColor);
//			}
//
//			if(mSubtitleParameter.mSubtitleFontSize != 0){
//				setSubFontSize(mSubtitleParameter.mSubtitleFontSize);
//			}
//
//			if(mSubtitleParameter.mSubtitlePosition != 0){
//				setSubPosition(mSubtitleParameter.mSubtitlePosition);
//			}
//
			setSubGate_l(mSubtitleParameter.mSubtitleGate);
		}

		mFlags &= ~RESTORE_CONTROL_PARA;
	}

	if(mSeeking && mTagPlay && mSeekTimeUs > 0){
        if(!(mFlags & PAUSING))
        {
    		mPlayer->control(mPlayer, CDX_CMD_TAG_START_ASYNC, (unsigned int)&mSeekTimeUs, 0);
    		LOGD("--tag play %lldus, mSeeking=%d, mTagPlay=%d",mSeekTimeUs, mSeeking, mTagPlay);
        }
        else
        {
            int64_t nMediaDuration = mMediaInfo.mDuration;
            if(mSeekTimeUs >= nMediaDuration*1000)
            {
                LOGD("mSeekTimeUs[%lld] >= mDuration[%lld]*1000", mSeekTimeUs, nMediaDuration);
                if(nMediaDuration*1000 > 500*1000)
                {
                    mSeekTimeUs = nMediaDuration*1000 - 500*1000;
                }
            }
            mPlayer->control(mPlayer, CDX_CMD_PAUSE_TAG_START_ASYNC, (unsigned int)&mSeekTimeUs, 0);
    		LOGD("--pause_tag_play %lldus, mSeeking=%d, mTagPlay=%d",mSeekTimeUs, mSeeking, mTagPlay);
        }
	}
	else if(mPlayerState == PLAYER_STATE_SUSPEND || mPlayerState == PLAYER_STATE_RESUME){
		mPlayer->control(mPlayer, CDX_CMD_TAG_START_ASYNC, (unsigned int)&mSuspensionState.mPositionUs, 0);
		LOGD("--tag play %lldus",mSuspensionState.mPositionUs);
	}
	else {
		mPlayer->control(mPlayer, command, (unsigned int)&mSuspensionState.mPositionUs, 0);
	}

    //make sure play state is in!
    int wait_time = 0;
    int cdx_state;
    while(1)
    {
        cdx_state = mPlayer->control(mPlayer, CDX_CMD_GETSTATE, 0, 0);
        if(cdx_state == CDX_STATE_EXCUTING)
        {
            LOGD("(f:%s, l:%d) cdx play alread!", __FUNCTION__, __LINE__);
            break;
        }
        else
        {
            LOGD("(f:%s, l:%d) cdx_state=0x%x, sleep 10ms to wait cdx to play, wait_time=%d!", __FUNCTION__, __LINE__, cdx_state, wait_time);
            usleep(10*1000);
            wait_time++;
        }
        if(wait_time > 30)  //wait 300ms.
        {
            LOGD("(f:%s, l:%d) wait cdx to pause fail, break!", __FUNCTION__, __LINE__);
            break;
        }
    }

    mPlayer->control(mPlayer, CDX_CMD_SWITCHSUB, mSubTrackIndex, 0);
    mPlayer->control(mPlayer, CDX_CMD_SWITCHTRACK, mAudioTrackIndex, 0);
	mSeeking = false;
	mTagPlay = 0;
	mPlayerState = PLAYER_STATE_PLAYING;
	mFlags &= ~PAUSING;

	return OK;
}

status_t CedarXPlayer::stop() {
	LOGV("CedarXPlayer::stop");

//	if(mAwesomePlayer) {
//		return mAwesomePlayer->pause();
//	}

	if(mPlayer != NULL){
		mPlayer->control(mPlayer, CDX_CMD_STOP_ASYNC, 0, 0);
	}
	stop_l();

//	if(display_3d_mode == CEDARX_DISPLAY_3D_MODE_3D)
//	{
//		set3DMode(CEDARV_3D_MODE_NONE, CEDARX_DISPLAY_3D_MODE_2D);
//	}

	_3d_mode 							= CEDARV_3D_MODE_NONE;
//	display_3d_mode 					= CEDARX_DISPLAY_3D_MODE_2D;
//	anaglagh_en						= 0;
//	anaglagh_type						= 0;
//	wait_anaglagh_display_change 		= 0;

	//* need to reset the display?
	//* TODO.

	return OK;
}

status_t CedarXPlayer::stop_l() {
	LOGV("stop() status:%x", mFlags & PLAYING);

	if(!mExtendMember->mPlaybackNotifySend) {
		notifyListener_l(MEDIA_INFO, MEDIA_INFO_BUFFERING_END);
		LOGD("MEDIA_PLAYBACK_COMPLETE");
		notifyListener_l(MEDIA_PLAYBACK_COMPLETE);
		mExtendMember->mPlaybackNotifySend = 1;
	}

	pause_l(true);

	{
		Mutex::Autolock autoLock(mLockNativeWindow);
		if(mVideoRenderer != NULL)
		{
			mVideoRenderer->control(VIDEORENDER_CMD_SHOW, 0);
	        if(1 == mExtendMember->bootanimation_enable)
	        {
	            LOGI("SET LAYER BOTTOM");
	            mVideoRenderer->control(VIDEORENDER_CMD_SETLAYERORDER, 1);
	            mExtendMember->bootanimation_enable = 0;
	        }
			mVideoRenderer.clear();
			mVideoRenderer = NULL;
		}
		LOGV("mUseHardwareLayer:%d gHardwareLayerRefCounter:%d",mExtendMember->mUseHardwareLayer,gHardwareLayerRefCounter);
		if (mExtendMember->mUseHardwareLayer) {
			gHardwareLayerRefCounter--;
			mExtendMember->mUseHardwareLayer = 0;
		}
	}

	mFlags &= ~SUSPENDING;
	LOGV("stop finish 1...");

	return OK;
}

status_t CedarXPlayer::pause() {
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	//Mutex::Autolock autoLock(mLock);
	//LOGD("pause()");

//	if(mAwesomePlayer) {
//		return mAwesomePlayer->pause();
//	}

#if 0
	mFlags &= ~CACHE_UNDERRUN;
	mPlayer->control(mPlayer, CDX_CMD_PAUSE, 0, 0);

	return pause_l(false);
#else
	if (!(mFlags & PLAYING)) {
		return OK;
	}

	pause_l(false);
	mPlayer->control(mPlayer, CDX_CMD_PAUSE_ASYNC, 0, 0);

	return OK;
#endif
}

status_t CedarXPlayer::pause_l(bool at_eos) {
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);

	mPlayerState = PLAYER_STATE_PAUSE;

	if (!(mFlags & PLAYING)) {
		return OK;
	}

	if (mAudioPlayer != NULL) {
		if (at_eos) {
			// If we played the audio stream to completion we
			// want to make sure that all samples remaining in the audio
			// track's queue are played out.
			mAudioPlayer->pause(true /* playPendingSamples */);
		} else {
			mAudioPlayer->pause();    //AudioRender component will pause too!
		}
	}

	mFlags &= ~PLAYING;
	mFlags |= PAUSING;

	return OK;
}

bool CedarXPlayer::isPlaying() const {
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	if(mAwesomePlayer) {
//		return mAwesomePlayer->isPlaying();
//	}
	LOGV("isPlaying cmd mFlags=0x%x",mFlags);
	return (mFlags & PLAYING) || (mFlags & CACHE_UNDERRUN);
}

//status_t CedarXPlayer::setNativeWindow_l(const sp<ANativeWindow> &native) 
status_t CedarXPlayer::setNativeWindow_l(const int hlay) 
{
    //LOGD("(f:%s, l:%d), native_p[%p]", __FUNCTION__, __LINE__, native.get());
    LOGD("(f:%s, l:%d), set layerId[%d]", __FUNCTION__, __LINE__, hlay);
	int i = 0;
	//int  outputSetting = 0;

	//if(mNativeWindow == NULL && (mFlags&PREPARED))	//* setSurface() is called after prepareAsync(), play not start yet.
	if(mVideoLayerId < 0 && (mFlags&PREPARED))	//* setSurface() is called after prepareAsync(), play not start yet.
	{
		Mutex::Autolock autoLock(mLockNativeWindow);
//		if (0 == native->perform(native.get(), NATIVE_WINDOW_GETPARAMETER,NATIVE_WINDOW_CMD_GET_SURFACE_TEXTURE_TYPE, 0)
//            || TEST_FORCE_GPU_RENDER)
//        {
//			LOGI("(f:%s, l:%d) use render GPU 0", __FUNCTION__, __LINE__);
//            mRenderToDE = 0;
//			outputSetting |= CEDARX_OUTPUT_SETTING_MODE_PLANNER;
//		}
#if 0
        else if(IfContextNeedGPURender())
        {
            LOGI("use render GPU context need!");
            mRenderToDE = 0;
            outputSetting |= CEDARX_OUTPUT_SETTING_MODE_PLANNER;
        }
#endif
		//else 
        {
			if (gHardwareLayerRefCounter < MAX_HARDWARE_LAYER_SUPPORT) {
                ALOGI("use render HW");
                mRenderToDE = 1;
				gHardwareLayerRefCounter++;
				mExtendMember->mUseHardwareLayer = 1;

//            #if (defined(__CHIP_VERSION_F23) || defined(__CHIP_VERSION_F51) || defined(__CHIP_VERSION_F50)) //a31 temp change to this, because of vertical screen
//            #elif (defined(__CHIP_VERSION_F33) )
//                if(TEST_FORCE_VDEC_YV12_OUTPUT)
//                {
//                    LOGI("test_force_vdec_yv12_output");
                    //outputSetting |= CEDARX_OUTPUT_SETTING_MODE_PLANNER;
//                }
//            #else
//                #error "Unknown chip type!"
//            #endif
			}
			else {
				ALOGE("fatal error! use render GPU 1");
                mRenderToDE = 0;
//				outputSetting |= CEDARX_OUTPUT_SETTING_MODE_PLANNER;
			}
		}

		//mExtendMember->mOutputSetting = outputSetting;
		//mPlayer->control(mPlayer, CDX_CMD_SET_VIDEO_OUTPUT_SETTING, outputSetting, 0);

	    //0: no rotate, 1: 90 degree (clock wise), 2: 180, 3: 270, 4: horizon flip, 5: vertical flig;
		if(mInitRotation)
		{
	        if(mRenderToDE)
	        {
	        	//* player has been prepared, set rotate may be not work, check it later.
	            ALOGD("init rotate[%d] in setNativeWindow_l()!", mInitRotation);
	            mPlayer->control(mPlayer, CDX_CMD_SET_VIDEO_ROTATION, mInitRotation, 0);
	        }
		}
	}

    //mNativeWindow = native;
    mVideoLayerId = hlay;

    if (mVideoWidth <= 0) {
        return OK;
    }

    ALOGV("attempting to reconfigure to use new surface");

    bool wasPlaying = (mFlags & PLAYING) != 0;
    pause();
    int cdx_state;
    int wait_time = 0;
    while(1)
    {
        cdx_state = mPlayer->control(mPlayer, CDX_CMD_GETSTATE, 0, 0);
        if(cdx_state == CDX_STATE_PAUSE)
        {
            LOGD("(f:%s, l:%d) pause success!", __FUNCTION__, __LINE__);
            break;
        }
        else
        {
            LOGD("(f:%s, l:%d) cdx_state=0x%x, sleep 10ms to wait cdx to pause!", __FUNCTION__, __LINE__, cdx_state);
            usleep(10*1000);
            wait_time++;
        }
        if(wait_time > 30)  //wait 300ms.
        {
            LOGD("(f:%s, l:%d) wait cdx to pause fail, break!", __FUNCTION__, __LINE__);
            break;
        }
    }
    
    {
    	Mutex::Autolock autoLock(mLockNativeWindow);
        if(mVideoRenderer != NULL)
        {
            //if(native == NULL)  //it means user want to clear nativeWindow
            if(hlay < 0)  //it means user want to clear nativeWindow
            {
                LOGD("Func[%s]Line[%d], set nativeWindow null, need close screen.", __FUNCTION__, __LINE__);
                mVideoRenderer->control(VIDEORENDER_CMD_SHOW, 0);
            }
        	mVideoRenderer.clear();
        	mVideoRenderer = NULL;
        }
    }

    if (wasPlaying) {
        play();
    }

    return OK;
}

//status_t CedarXPlayer::setSurface(const sp<Surface> &surface) 
status_t CedarXPlayer::setSurface(const int hlay) 
{
    Mutex::Autolock autoLock(mLock);
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    LOGV("setSurface");
    //mSurface = surface;
    mVideoLayerId = hlay;
    return setNativeWindow_l(hlay);
}

//status_t CedarXPlayer::setSurfaceTexture(const sp<ISurfaceTexture> &surfaceTexture) 
status_t CedarXPlayer::setSurfaceTexture(const int hlay) 
{
    //Mutex::Autolock autoLock(mLock);

    //mSurface.clear();
    LOGV("(f:%s, l:%d) hlay=%d", __FUNCTION__, __LINE__, hlay);

    status_t err;
    //if (surfaceTexture != NULL) 
    LOGV("(f:%s, l:%d) surfaceTexture[%d]", __FUNCTION__, __LINE__, hlay);
    err = setNativeWindow_l(hlay);

    return err;
}

void CedarXPlayer::setAudioSink(const sp<MediaPlayerBase::AudioSink> &audioSink) {
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	//Mutex::Autolock autoLock(mLock);
	mAudioSink = audioSink;
}

status_t CedarXPlayer::setLooping(bool shouldLoop) {
    LOGV("(f:%s, l:%d), shouldLoop=%d", __FUNCTION__, __LINE__, shouldLoop);
	//Mutex::Autolock autoLock(mLock);
//	if (mAwesomePlayer) {
//		mAwesomePlayer->setLooping(shouldLoop);
//	}

	mFlags = mFlags & ~LOOPING;

	if (shouldLoop) {
		mFlags |= LOOPING;
	}

	return OK;
}

status_t CedarXPlayer::getDuration(int64_t *durationUs) {
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	if (mAwesomePlayer) {
//	    int64_t tmp;
//	    status_t err = mAwesomePlayer->getDuration(&tmp);
//
//	    if (err != OK) {
//	        *durationUs = 0;
//	        return OK;
//	    }
//
//	    *durationUs = tmp;
//
//	    return OK;
//	}

	mPlayer->control(mPlayer, CDX_CMD_GET_DURATION, (unsigned int)durationUs, 0);
	*durationUs *= 1000;
	mDurationUs = *durationUs;

	LOGV("mDurationUs %lld", mDurationUs);

	return OK;
}

status_t CedarXPlayer::getPosition(int64_t *positionUs) {
	LOGV("getPosition ");
//	if (mAwesomePlayer) {
//		int64_t tmp;
//		status_t err = mAwesomePlayer->getPosition(&tmp);
//
//		if (err != OK) {
//			return err;
//		}
//
		//*positionUs = (tmp + 500) / 1000;
//        *positionUs = tmp;  //use microsecond! don't convert to millisecond
//		LOGV("getPosition:%lld",*positionUs);
//		return OK;
//	}

	if(mFlags & AT_EOS){
		*positionUs = mDurationUs;
		return OK;
	}

	{
		//Mutex::Autolock autoLock(mLock);
		if(mPlayer != NULL){
			mPlayer->control(mPlayer, CDX_CMD_GET_POSITION, (unsigned int)positionUs, 0);
		}
	}

	if(mSeeking == true) {
		*positionUs = mSeekTimeUs;
	}

	int64_t nowUs = ALooper::GetNowUs();
//	LOGV("nowUs %lld, mLastGetPositionTimeUs %lld", nowUs, mExtendMember->mLastGetPositionTimeUs);
	//Theoretically, below conndition is satisfied.
	CHECK_GT(nowUs, mExtendMember->mLastGetPositionTimeUs);
	if((nowUs - mExtendMember->mLastGetPositionTimeUs) < 40000ll) {

		*positionUs = mExtendMember->mLastPositionUs;
		return OK;
	}
	mExtendMember->mLastGetPositionTimeUs = nowUs;

	*positionUs = (*positionUs / 1000) * 1000; //to fix android 4.0 cts bug
	mExtendMember->mLastPositionUs = *positionUs;
//	LOGV("getPosition: %lld mSeekTimeUs:%lld, nowUs %lld",
//			*positionUs / 1000, mSeekTimeUs, nowUs);

	return OK;
}

status_t CedarXPlayer::seekTo(int64_t timeMs) {
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);

	LOGV("seek to time %lld, mSeeking %d", timeMs, mSeeking);
//	if (mAwesomePlayer)
//	{
//		int ret;
//		ret = mAwesomePlayer->seekTo(timeMs * 1000);
//		return ret;
//	}
		
	int64_t currPositionUs;
    if(mPlayer->control(mPlayer, CDX_CMD_SUPPORT_SEEK, 0, 0) == 0)
    {
    	notifyListener_l(MEDIA_SEEK_COMPLETE);
        return OK;
    }

    if(mSeeking && !(mFlags & PAUSING))
    {
    	notifyListener_l(MEDIA_SEEK_COMPLETE);
    	mSeekNotificationSent = true;
    	return OK;
    }

	getPosition(&currPositionUs);

	{
		Mutex::Autolock autoLock(mLock);
		mSeekNotificationSent = false;
		LOGV("seek cmd to %lld s start", timeMs/1000);

		if (mFlags & CACHE_UNDERRUN) {
			mFlags &= ~CACHE_UNDERRUN;
			play_l(CDX_CMD_START_ASYNC);
		}

		mSeekTimeUs = timeMs * 1000;
		mSeeking = true;
		//This is necessary, if apps want position after seek operation
		//immediately, then the diff between current time and
		// mExtendMember->mLastPositionUs may be smaller than 40ms.
		//In this case, the position we returned is same as that before seek.
		//We can fix this by setting mExtendMember->mLastGetPositionTimeUs to 0
		//or setting mExtendMember->mLastPositionUs to mSeekTimeUs;

//		mExtendMember->mLastGetPositionTimeUs = 0;
		mExtendMember->mLastPositionUs        = mSeekTimeUs;

		mFlags &= ~(AT_EOS | AUDIO_AT_EOS | VIDEO_AT_EOS);

		if (!(mFlags & PLAYING)) {
			LOGV( "seeking while paused, mFlags=0x%x,sending SEEK_COMPLETE notification"
						" immediately.", mFlags);
			mTagPlay = 1;
			notifyListener_l(MEDIA_SEEK_COMPLETE);
			mSeekNotificationSent = true;
			return OK;
		}
	}

	//mPlayer->control(mPlayer, CDX_CMD_SET_AUDIOCHANNEL_MUTE, 3, 0);
	mPlayer->control(mPlayer, CDX_CMD_SEEK_ASYNC, (int)timeMs, (int)(currPositionUs/1000));

	LOGV("seek cmd to %lld s end", timeMs/1000);

	return OK;
}

void CedarXPlayer::finishAsyncPrepare_l(int err){
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);

	LOGV("finishAsyncPrepare_l");
//	if (mAwesomePlayer) {
		//fd music
//		return;
//	}

	if(err == CDX_ERROR_UNSUPPORT_USESFT) {
		//current http+mp3 etc goes here
//		mAwesomePlayer = new AwesomePlayer;
//		mAwesomePlayer->setListener(mListener);
//		if(mAudioSink != NULL) {
//			mAwesomePlayer->setAudioSink(mAudioSink);
//		}
//
//		if(mFlags & LOOPING) {
//			mAwesomePlayer->setLooping(!!(mFlags & LOOPING));
//		}
//		mAwesomePlayer->setDataSource(mUri.string(), &mUriHeaders);
//		mAwesomePlayer->prepareAsync();
		return;
	}

	if(err < 0){
		LOGE("CedarXPlayer:prepare error! %d", err);
		abortPrepare(UNKNOWN_ERROR);
		return;
	}

	mPlayer->control(mPlayer, CDX_CMD_GET_STREAM_TYPE, (unsigned int)&mStreamType, 0);
	if(mSourceType != SOURCETYPE_FD) {
		mPlayer->control(mPlayer, CDX_CMD_GET_MEDIAINFO, (unsigned int)&mMediaInfo, 0);
	}
	//if(mStreamType != CEDARX_STREAM_LOCALFILE) {
	mFlags &= ~CACHE_UNDERRUN;
	//}
	//must be same with the value set to native_window_set_buffers_geometry()!
	mVideoWidth  = mMediaInfo.mVideoInfo[0].mFrameWidth; 
	mVideoHeight = mMediaInfo.mVideoInfo[0].mFrameHeight;
	mCanSeek = mMediaInfo.mFlags & 1;
	if (mVideoWidth && mVideoHeight) {
        LOGD("(f:%s, l:%d) mVideoWidth[%d], mVideoHeight[%d], notifyListener_l(MEDIA_SET_VIDEO_SIZE_)", __FUNCTION__, __LINE__, mVideoWidth, mVideoHeight);
		notifyListener_l(MEDIA_SET_VIDEO_SIZE, mVideoWidth, mVideoHeight);
	}
	else {
		LOGW("unkown video size after prepared");
		//notifyListener_l(MEDIA_SET_VIDEO_SIZE, 640, 480);
	}
	mFlags &= ~(PREPARING|PREPARE_CANCELLED);
	mFlags |= PREPARED;

	//mPlayer->control(mPlayer, CDX_CMD_SET_AUDIOCHANNEL_MUTE, 1, 0);

	if(mIsAsyncPrepare && mSourceType != SOURCETYPE_SFT_STREAM){
		notifyListener_l(MEDIA_PREPARED);
	}

//	if(mHDMIListener && mIsDRMMedia) {
//		/*only  start thread if media is encrypted*/
//		mHDMIListener->setNotifyCallback(this, HDMINotify);
//		mHDMIListener->start();
//	}

	return;
}

void CedarXPlayer::finishSeek_l(int err){
	Mutex::Autolock autoLock(mLock);
	LOGV("finishSeek_l");

	if(mAudioPlayer){
		mAudioPlayer->seekTo(0);
	}
	mSeeking = false;
	if (!mSeekNotificationSent) {
		LOGV("MEDIA_SEEK_COMPLETE return");
		notifyListener_l(MEDIA_SEEK_COMPLETE);
		mSeekNotificationSent = true;
	}
	//mPlayer->control(mPlayer, CDX_CMD_SET_AUDIOCHANNEL_MUTE, 0, 0);

	return;
}

status_t CedarXPlayer::prepareAsync() {
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	Mutex::Autolock autoLock(mLock);
	//int  outputSetting = 0;
	int  disable_media_type = 0;
	char prop_value[4];

	if ((mFlags & PREPARING) || (mPlayer == NULL)) {
		return UNKNOWN_ERROR; // async prepare already pending
	}

	property_get(PROP_CHIP_VERSION_KEY, prop_value, "3");
	mPlayer->control(mPlayer, CDX_CMD_SET_SOFT_CHIP_VERSION, atoi(prop_value), 0);

#if 1
	if (atoi(prop_value) == 5) {
		property_get(PROP_CONSTRAINT_RES_KEY, prop_value, "1");
		if (atoi(prop_value) == 1) {
			mPlayer->control(mPlayer, CDX_CMD_SET_MAX_RESOLUTION, 1288<<16 | 1288, 0);
		}
	}
#endif

	if (mSourceType == SOURCETYPE_SFT_STREAM) {
		notifyListener_l(MEDIA_PREPARED);
		return OK;
	}

	mFlags |= PREPARING;
	mIsAsyncPrepare = true;


	if(mSourceType == SOURCETYPE_SFT_STREAM) {
		//outputSetting |= CEDARX_OUTPUT_SETTING_MODE_PLANNER;
	}

	//if(mNativeWindow != NULL) 
    if(mVideoLayerId >= 0) 
    {
		Mutex::Autolock autoLock(mLockNativeWindow);
		//if (0 == mNativeWindow->perform(mNativeWindow.get(), NATIVE_WINDOW_GETPARAMETER,NATIVE_WINDOW_CMD_GET_SURFACE_TEXTURE_TYPE, 0)
        //    || TEST_FORCE_GPU_RENDER) 
        if(0)
        {
			LOGI("use render GPU 0");
            mRenderToDE = 0;
			//outputSetting |= CEDARX_OUTPUT_SETTING_MODE_PLANNER;
		}

#if 0
        else if(IfContextNeedGPURender())
        {
            LOGI("use render GPU context need!");
            mRenderToDE = 0;
            outputSetting |= CEDARX_OUTPUT_SETTING_MODE_PLANNER;
        }
#endif
		else {
			if (gHardwareLayerRefCounter < MAX_HARDWARE_LAYER_SUPPORT) {
                LOGI("use render HW");
                mRenderToDE = 1;
				gHardwareLayerRefCounter++;
				mExtendMember->mUseHardwareLayer = 1;

//            #if (defined(__CHIP_VERSION_F23) || defined(__CHIP_VERSION_F51) || defined(__CHIP_VERSION_F50)) //a31 temp change to this, because of vertical screen
//            #elif (defined(__CHIP_VERSION_F33))
//                if(TEST_FORCE_VDEC_YV12_OUTPUT)
//                {
//                    LOGI("test_force_vdec_yv12_output");
                    //outputSetting |= CEDARX_OUTPUT_SETTING_MODE_PLANNER;
//                }
//            #else
//                #error "Unknown chip type!"
//            #endif
			}
			else {
				LOGE("fatal error! use render GPU 1");
                mRenderToDE = 0;
				//outputSetting |= CEDARX_OUTPUT_SETTING_MODE_PLANNER;
			}
		}
	}
	else {
//		outputSetting |= CEDARX_OUTPUT_SETTING_MODE_PLANNER;
//		LOGI("use render GPU 2");
//        mRenderToDE = 0;
		LOGI("default use HwLayer");
        mRenderToDE = 1;
	}

	//outputSetting |= CEDARX_OUTPUT_SETTING_MODE_PLANNER;
	mExtendMember->mPlaybackNotifySend = 0;
	//mExtendMember->mOutputSetting = outputSetting;
	//mPlayer->control(mPlayer, CDX_CMD_SET_VIDEO_OUTPUT_SETTING, outputSetting, 0);

    
    //0: no rotate, 1: 90 degree (clock wise), 2: 180, 3: 270, 4: horizon flip, 5: vertical flig;
	if(mInitRotation)
	{
        if(mRenderToDE)
        {
            LOGD("init rotate[%d] in prepareAsync()!", mInitRotation);
            mPlayer->control(mPlayer, CDX_CMD_SET_VIDEO_ROTATION, mInitRotation, 0);
        }
	}

//	mMaxOutputWidth = 720;
//	mMaxOutputHeight = 576;

	//scale down in decoder when using GPU
#if 0 //* chenxiaochuan. We need not to limite the picture size because pixel format is transformed by decoder.
	if(outputSetting & CEDARX_OUTPUT_SETTING_MODE_PLANNER) {
		mMaxOutputWidth  = (mMaxOutputWidth > 1280 || mMaxOutputWidth <= 0) ? 1280 : mMaxOutputWidth;
		mMaxOutputHeight = (mMaxOutputHeight > 720 || mMaxOutputHeight <= 0) ? 720 : mMaxOutputHeight;
	}
#endif

	if(mMaxOutputWidth && mMaxOutputHeight) {
		LOGV("Max ouput size %dX%d", mMaxOutputWidth, mMaxOutputHeight);
		mPlayer->control(mPlayer, CDX_CMD_SET_VIDEO_MAXWIDTH, mMaxOutputWidth, 0);
		mPlayer->control(mPlayer, CDX_CMD_SET_VIDEO_MAXHEIGHT, mMaxOutputHeight, 0);
	}

#if 0
	disable_media_type |= CDX_MEDIA_TYPE_DISABLE_MPG | CDX_MEDIA_TYPE_DISABLE_TS | CDX_MEDIA_TYPE_DISABLE_ASF;
	disable_media_type |= CDX_CODEC_TYPE_DISABLE_MPEG2 | CDX_CODEC_TYPE_DISABLE_VC1;
	disable_media_type |= CDX_CODEC_TYPE_DISABLE_WMA;
	mPlayer->control(mPlayer, CDX_CMD_DISABLE_MEDIA_TYPE, disable_media_type, 0);
#endif

	//mPlayer->control(mPlayer, CDX_SET_THIRDPART_STREAM, CEDARX_THIRDPART_STREAM_USER0, 0);
	//mPlayer->control(mPlayer, CDX_SET_THIRDPART_STREAM, CEDARX_THIRDPART_STREAM_USER0, CDX_MEDIA_FILE_FMT_AVI);
	//int32_t     encrypt_enable;
    //CDX_MEDIA_FILE_FORMAT  encrypt_file_format;
	if (PARAM_KEY_ENCRYPT_ENTIRE_FILE_TYPE == mExtendMember->encrypt_type)
	{
        mPlayer->control(mPlayer, CDX_SET_THIRDPART_STREAM, CEDARX_THIRDPART_STREAM_USER0, mExtendMember->encrypt_file_format);
    }
    else if (PARAM_KEY_ENCRYPT_PART_FILE_TYPE == mExtendMember->encrypt_type)
    {
        mPlayer->control(mPlayer, CDX_SET_THIRDPART_STREAM, CEDARX_THIRDPART_STREAM_USER1, mExtendMember->encrypt_file_format);
    }
    else 
    {
        mExtendMember->encrypt_type = PARAM_KEY_ENCRYPT_FILE_TYPE_DISABLE;
        mExtendMember->encrypt_file_format = CDX_MEDIA_FILE_FMT_UNKOWN;
        mPlayer->control(mPlayer, CDX_SET_THIRDPART_STREAM, CEDARX_THIRDPART_STREAM_NONE, mExtendMember->encrypt_file_format);
    }

    LOGD("play vps[%d] before CDX CMD_PREPARE_ASYNC!", mVpsspeed);
    mPlayer->control(mPlayer, CDX_CMD_SET_VPS, mVpsspeed, 0);
	return (mPlayer->control(mPlayer, CDX_CMD_PREPARE_ASYNC, 0, 0) == 0 ? OK : UNKNOWN_ERROR);
}

status_t CedarXPlayer::prepare() {
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	status_t ret;

	Mutex::Autolock autoLock(mLock);
	LOGV("prepare");
	_3d_mode 							= CEDARV_3D_MODE_NONE;
//	display_3d_mode 					= CEDARX_DISPLAY_3D_MODE_2D;
//	anaglagh_en						= 0;
//	anaglagh_type						= 0;
//	wait_anaglagh_display_change 		= 0;

	ret = prepare_l();
	//getInputDimensionType();

	return ret;
}

status_t CedarXPlayer::prepare_l() {
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	int ret;
	if (mFlags & PREPARED) {
	    return OK;
	}

	mIsAsyncPrepare = false;
	ret = mPlayer->control(mPlayer, CDX_CMD_PREPARE, 0, 0);
	if(ret != 0){
		return UNKNOWN_ERROR;
	}

	finishAsyncPrepare_l(0);

	return OK;
}

void CedarXPlayer::abortPrepare(status_t err) {
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	CHECK(err != OK);

	if (mIsAsyncPrepare) {
		notifyListener_l(MEDIA_ERROR, MEDIA_ERROR_UNKNOWN, err);
	}

	mPrepareResult = err;
	mFlags &= ~(PREPARING | PREPARE_CANCELLED);
}

status_t CedarXPlayer::suspend() {
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	LOGD("suspend start");

	if (mFlags & SUSPENDING)
		return OK;

	SuspensionState *state = &mSuspensionState;
	getPosition(&state->mPositionUs);

	//Mutex::Autolock autoLock(mLock);

	state->mFlags = mFlags & (PLAYING | AUTO_LOOPING | LOOPING | AT_EOS);
    state->mUri = mUri;
    state->mUriHeaders = mUriHeaders;
	mFlags |= SUSPENDING;

	if(isCedarXInitialized){
		mPlayer->control(mPlayer, CDX_CMD_STOP_ASYNC, 0, 0);
		CDXPlayer_Destroy(mPlayer);
		mPlayer = NULL;
		isCedarXInitialized = false;
	}

	pause_l(true);

	{
		Mutex::Autolock autoLock(mLockNativeWindow);
		mVideoRenderer.clear();
		mVideoRenderer = NULL;
	}

	if(mAudioPlayer){
		delete mAudioPlayer;
		mAudioPlayer = NULL;
	}
	mPlayerState = PLAYER_STATE_SUSPEND;
	LOGD("suspend end");

	return OK;
}

status_t CedarXPlayer::resume() {
    ALOGD("(f:%s, l:%d) resume start", __FUNCTION__, __LINE__);
    //Mutex::Autolock autoLock(mLock);
    SuspensionState *state = &mSuspensionState;
    status_t err;
    if (mSourceType != SOURCETYPE_URL){
        LOGW("NOT support setdatasouce non-uri currently");
        return UNKNOWN_ERROR;
    }

    if(mPlayer == NULL){
    	CDXPlayer_Create((void**)&mPlayer);
    	mPlayer->control(mPlayer, CDX_CMD_REGISTER_CALLBACK, (unsigned int)&CedarXPlayerCallbackWrapper, (unsigned int)this);
    	isCedarXInitialized = true;
    }

    //mPlayer->control(mPlayer, CDX_CMD_SET_STATE, CDX_STATE_UNKOWN, 0);

    err = setDataSource(state->mUri, &state->mUriHeaders);
	mPlayer->control(mPlayer, CDX_CMD_SET_NETWORK_ENGINE, CEDARX_NETWORK_ENGINE_SFT, 0);
	//mPlayer->control(mPlayer, CDX_CMD_SET_VIDEO_OUTPUT_SETTING, mExtendMember->mOutputSetting, 0);

    mFlags = state->mFlags & (AUTO_LOOPING | LOOPING | AT_EOS);

    mFlags |= RESTORE_CONTROL_PARA;

    if (state->mFlags & PLAYING) {
        play_l(CDX_CMD_TAG_START_ASYNC);
    }
    mFlags &= ~SUSPENDING;
    //state->mPositionUs = 0;
    mPlayerState = PLAYER_STATE_RESUME;

    LOGD("resume end");

	return OK;
}
/*******************************************************************************
Function name: android.CedarXPlayer.setVps
Description: 
    1.set variable play speed.
Parameters: 
    
Return: 
    
Time: 2013/1/12
*******************************************************************************/
int CedarXPlayer::setVps(int vpsspeed)
{
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    mVpsspeed = vpsspeed;
    if(isPlaying())
    {
        LOGD("state is playing, change vps to[%d]", mVpsspeed);
        mPlayer->control(mPlayer, CDX_CMD_SET_VPS, mVpsspeed, 0);
    }
    return OK;
}
//status_t CedarXPlayer::setScreen(int screen) {
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	mScreenID = screen;
//	LOGV("CedarX will setScreen to:%d", screen);
//	if(mVideoRenderer != NULL && !(mFlags & SUSPENDING)){
//#if (CEDARX_ANDROID_VERSION == 6 && CEDARX_ADAPTER_VERSION == 4)
//        LOGW("CedarX setScreen to:%d", screen);
//        return mVideoRenderer->control(VIDEORENDER_CMD_SETSCREEN, screen);
//#elif (CEDARX_ANDROID_VERSION >= 6)
//        return OK; //no need to setscreen from android4.0 v1.5 version
//#else
//    #error "Unknown chip type!"
//#endif
		//return pCedarXPlayerAdapter->CedarXPlayerAdapterIoCtrl(CEDARXPLAYERADAPTER_CMD_SETSCREEN_SPECIALPROCESS, screen, NULL);
//	}
//	return UNKNOWN_ERROR;
//}

/*
status_t CedarXPlayer::set3DMode(int rotate_flag)
{
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	VirtualVideo3DInfo  virtual_3d_info;
	if(rotate_flag)
	{
		//ALOGD("SET3DMODE==========================90");
		virtual_3d_info.width           =mDisplayHeight;
		virtual_3d_info.height          =mDisplayWidth;
		mVideo3dInfo.width		 =virtual_3d_info.width;
		mVideo3dInfo.height		 =virtual_3d_info.height;
//		notifyListener_l(MEDIA_SET_VIDEO_SIZE, mVideoHeight, mVideoWidth);
	}else
	{
		//ALOGD("SET3DMODE--------------------------0");
		virtual_3d_info.width           =mDisplayWidth          ;
		virtual_3d_info.height          =mDisplayHeight        ;
		mVideo3dInfo.width		 =virtual_3d_info.width;
		mVideo3dInfo.height		 =virtual_3d_info.height;
	}
	virtual_3d_info.format          =mVideo3dInfo.format         ;
	virtual_3d_info.src_mode        =mVideo3dInfo.src_mode       ;
	virtual_3d_info.display_mode    =mVideo3dInfo.display_mode   ;
//	virtual_3d_info.is_mode_changed =mVideo3dInfo.is_mode_changed;

	mVideoRenderer->control(VIDEORENDER_CMD_SET3DMODE, (int)&virtual_3d_info);
	
	android_native_rect_t crop;
	crop.left = 0;
	crop.right = virtual_3d_info.width;
	crop.top = 0;
	crop.bottom = virtual_3d_info.height;
	
	return mVideoRenderer->control(VIDEORENDER_CMD_SET_CROP, (int)&crop);
}
*/

//status_t CedarXPlayer::set3DMode(int source3dMode, int displayMode)
status_t CedarXPlayer::set3DMode(int source3dMode)
{
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	//video3Dinfo_t _3d_info;
    VirtualVideo3DInfo  virtual_3d_info;
	virtual_3d_info.width 	 			= mDisplayWidth;
	virtual_3d_info.height	 			= mDisplayHeight;
	virtual_3d_info.format	 			= (VirtualHWCRenderFormat)mDisplayFormat;

	//* set source 3d mode.
	if(source3dMode == CEDARV_3D_MODE_DOUBLE_STREAM)
		virtual_3d_info.src_mode = VIRTUAL_HWC_3D_SRC_MODE_FP;
	else if(source3dMode == CEDARV_3D_MODE_SIDE_BY_SIDE)
		virtual_3d_info.src_mode = VIRTUAL_HWC_3D_SRC_MODE_SSH;
	else if(source3dMode == CEDARV_3D_MODE_TOP_TO_BOTTOM)
		virtual_3d_info.src_mode = VIRTUAL_HWC_3D_SRC_MODE_TB;
	else if(source3dMode == CEDARV_3D_MODE_LINE_INTERLEAVE)
		virtual_3d_info.src_mode = VIRTUAL_HWC_3D_SRC_MODE_LI;
	else
		virtual_3d_info.src_mode = VIRTUAL_HWC_3D_SRC_MODE_NORMAL;

	//* set display 3d mode.
//	if(displayMode == CEDARX_DISPLAY_3D_MODE_ANAGLAGH) {
//		if(source3dMode == CEDARV_3D_MODE_SIDE_BY_SIDE) {
//			virtual_3d_info.width = mDisplayWidth/2;//frm_inf->width /=2;
//			virtual_3d_info.width = (virtual_3d_info.width + 0xF) & 0xFFFFFFF0;
//		}
//		else if(source3dMode == CEDARV_3D_MODE_TOP_TO_BOTTOM) {
//			virtual_3d_info.height = mDisplayHeight/2; //frm_inf->height /=2;
//			virtual_3d_info.height = (virtual_3d_info.height + 0xF) & 0xFFFFFFF0;
//		}
//
//		if(virtual_3d_info.format != VIRTUAL_HWC_FORMAT_RGBA_8888)
//			virtual_3d_info.format = VIRTUAL_HWC_FORMAT_RGBA_8888;		//* force pixel format to be RGBA8888.
//
//		virtual_3d_info.display_mode = VIRTUAL_HWC_3D_OUT_MODE_ANAGLAGH;
//	}
//	else if(displayMode == CEDARX_DISPLAY_3D_MODE_3D)
//		virtual_3d_info.display_mode = VIRTUAL_HWC_3D_OUT_MODE_HDMI_3D_1080P24_FP;
//	else if(displayMode == CEDARX_DISPLAY_3D_MODE_2D)
//		virtual_3d_info.display_mode = VIRTUAL_HWC_3D_OUT_MODE_ORIGINAL;
//	else
	{
		virtual_3d_info.display_mode = VIRTUAL_HWC_3D_OUT_MODE_2D;
	}
		
	mVideo3dInfo.width           =virtual_3d_info.width           ;
	mVideo3dInfo.height          =virtual_3d_info.height          ;
	mVideo3dInfo.format          =virtual_3d_info.format          ;
	mVideo3dInfo.src_mode        =virtual_3d_info.src_mode        ;
	mVideo3dInfo.display_mode    =virtual_3d_info.display_mode    ;
//	mVideo3dInfo.is_mode_changed =virtual_3d_info.is_mode_changed ;
	if(mVideoRenderer != NULL) {
		ALOGV("set hdmi, virtual_src_mode = %d, virtual_dst_mode = %d", virtual_3d_info.src_mode, virtual_3d_info.display_mode);
		return mVideoRenderer->control(VIDEORENDER_CMD_SET3DMODE, (int)&virtual_3d_info);
	}

	return UNKNOWN_ERROR;
}

int CedarXPlayer::getMeidaPlayerState() {
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    return mPlayerState;
}

//int CedarXPlayer::getSubCount()
//{
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	int tmp;
//
//	if(mPlayer == NULL){
//		return -1;
//	}
//
//	mPlayer->control(mPlayer, CDX_CMD_GETSUBCOUNT, (unsigned int)&tmp, 0);
//
//	LOGV("getSubCount:%d",tmp);
//
//    return tmp;
//}
//
//int CedarXPlayer::getSubList(MediaPlayer_SubInfo *infoList, int count)
//{
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	if(mPlayer == NULL){
//		return -1;
//	}
//
//	if(mPlayer->control(mPlayer, CDX_CMD_GETSUBLIST, (unsigned int)infoList, count) == 0){
//		return count;
//	}
//
//	return -1;
//}
//
//int CedarXPlayer::getCurSub()
//{
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	int tmp;
//
//	if(mPlayer == NULL){
//		return -1;
//	}
//
//	mPlayer->control(mPlayer, CDX_CMD_GETCURSUB, (unsigned int)&tmp, 0);
//    return tmp;
//};
//
status_t CedarXPlayer::switchSub_l(int index)
{
    LOGV("(f:%s, l:%d) index=%d", __FUNCTION__, __LINE__, index);
	if(mPlayer == NULL){
		return -1;
	} else if(mSubTrackIndex == index) {
		return 0;
	}

	mSubTrackIndex = index;
	return mPlayer->control(mPlayer, CDX_CMD_SWITCHSUB, index, 0);
}

status_t CedarXPlayer::setSubGate_l(bool showSub)
{
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	if(mPlayer == NULL){
		return -1;
	}

	mSubtitleParameter.mSubtitleGate = showSub;
	return mPlayer->control(mPlayer, CDX_CMD_SETSUBGATE, showSub, 0);
}

bool CedarXPlayer::getSubGate_l()
{
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	int tmp;

	if(mPlayer == NULL){
		return false;
	}

	mPlayer->control(mPlayer, CDX_CMD_GETSUBGATE, (unsigned int)&tmp, 0);
	return tmp;
};
//
//status_t CedarXPlayer::setSubColor(int color)
//{
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	if(mPlayer == NULL){
//		return -1;
//	}
//
//	mSubtitleParameter.mSubtitleColor = color;
//	return mPlayer->control(mPlayer, CDX_CMD_SETSUBCOLOR, color, 0);
//}
//
//int CedarXPlayer::getSubColor()
//{
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	int tmp;
//
//	if(mPlayer == NULL){
//		return -1;
//	}
//
//	mPlayer->control(mPlayer, CDX_CMD_GETSUBCOLOR, (unsigned int)&tmp, 0);
//	return tmp;
//}
//
//status_t CedarXPlayer::setSubFrameColor(int color)
//{
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	if(mPlayer == NULL){
//		return -1;
//	}
//
//	mSubtitleParameter.mSubtitleFrameColor = color;
//    return mPlayer->control(mPlayer, CDX_CMD_SETSUBFRAMECOLOR, color, 0);
//}
//
//int CedarXPlayer::getSubFrameColor()
//{
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	int tmp;
//
//	if(mPlayer == NULL){
//		return -1;
//	}
//
//	mPlayer->control(mPlayer, CDX_CMD_GETSUBFRAMECOLOR, (unsigned int)&tmp, 0);
//	return tmp;
//}
//
//status_t CedarXPlayer::setSubFontSize(int size)
//{
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	if(mPlayer == NULL){
//		return -1;
//	}
//
//	mSubtitleParameter.mSubtitleFontSize = size;
//	return mPlayer->control(mPlayer, CDX_CMD_SETSUBFONTSIZE, size, 0);
//}
//
//int CedarXPlayer::getSubFontSize()
//{
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	int tmp;
//
//	if(mPlayer == NULL){
//		return -1;
//	}
//
//	mPlayer->control(mPlayer, CDX_CMD_GETSUBFONTSIZE, (unsigned int)&tmp, 0);
//	return tmp;
//}

status_t CedarXPlayer::setSubCharset(const char *charset)
{
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	if(mPlayer == NULL){
		return -1;
	}

	mSubtitleParameter.mSubtitleCharset = charset;
    return mPlayer->control(mPlayer, CDX_CMD_SETSUBCHARSET, (unsigned int)charset, 0);
}

status_t CedarXPlayer::getSubCharset(char *charset)
{
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	if(mPlayer == NULL){
		return -1;
	}

    return mPlayer->control(mPlayer, CDX_CMD_GETSUBCHARSET, (unsigned int)charset, 0);
}

//status_t CedarXPlayer::setSubPosition(int percent)
//{
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	if(mPlayer == NULL){
//		return -1;
//	}
//
//	mSubtitleParameter.mSubtitlePosition = percent;
//	return mPlayer->control(mPlayer, CDX_CMD_SETSUBPOSITION, percent, 0);
//}
//
//int CedarXPlayer::getSubPosition()
//{
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	int tmp;
//
//	if(mPlayer == NULL){
//		return -1;
//	}
//
//	mPlayer->control(mPlayer, CDX_CMD_GETSUBPOSITION, (unsigned int)&tmp, 0);
//	return tmp;
//}

status_t CedarXPlayer::setSubDelay(int time)
{
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	if(mPlayer == NULL){
		return -1;
	}

	mSubtitleParameter.mSubtitleDelay = time;
	return mPlayer->control(mPlayer, CDX_CMD_SETSUBDELAY, time, 0);
}

int CedarXPlayer::getSubDelay()
{
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	int tmp;

	if(mPlayer == NULL){
		return -1;
	}

	mPlayer->control(mPlayer, CDX_CMD_GETSUBDELAY, (unsigned int)&tmp, 0);
	return tmp;
}

//int CedarXPlayer::getTrackCount()
//{
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	int tmp;
//
//	if(mPlayer == NULL){
//		return -1;
//	}
//
//	mPlayer->control(mPlayer, CDX_CMD_GETTRACKCOUNT, (unsigned int)&tmp, 0);
//	return tmp;
//}
//
//int CedarXPlayer::getTrackList(MediaPlayer_TrackInfo *infoList, int count)
//{
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	if(mPlayer == NULL){
//		return -1;
//	}
//
//	if(mPlayer->control(mPlayer, CDX_CMD_GETTRACKLIST, (unsigned int)infoList, count) == 0){
//		return count;
//	}
//
//	return -1;
//}
//
//int CedarXPlayer::getCurTrack()
//{
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	int tmp;
//
//	if(mPlayer == NULL){
//		return -1;
//	}
//
//	mPlayer->control(mPlayer, CDX_CMD_GETCURTRACK, (unsigned int)&tmp, 0);
//	return tmp;
//}
//
status_t CedarXPlayer::switchTrack_l(int index)
{
    LOGV("(f:%s, l:%d) index=0x%x", __FUNCTION__, __LINE__, index);
	if(mPlayer == NULL){
		return -1;
	} else if(mAudioTrackIndex == index) {
		return 0;
	}

	mAudioTrackIndex = index;
	return mPlayer->control(mPlayer, CDX_CMD_SWITCHTRACK, index, 0);
}
//
//status_t CedarXPlayer::setInputDimensionType(int type)
//{
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	if(mPlayer == NULL)
//		return -1;
//
//	_3d_mode = (cedarv_3d_mode_e)type;
//	if(mPlayer->control(mPlayer, CDX_CMD_SET_PICTURE_3D_MODE, _3d_mode, 0) != 0)
//		return -1;
//
	//* the 3d mode you set may be invalid, get the valid 3d mode from mPlayer.
//	mPlayer->control(mPlayer, CDX_CMD_GET_PICTURE_3D_MODE, (unsigned int)&_3d_mode, 0);
//
//	LOGV("set 3d source mode to %d, current_3d_mode = %d", type, _3d_mode);
//
//	return 0;
//}
//
//int CedarXPlayer::getInputDimensionType()
//{
//    /*
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	cedarv_3d_mode_e tmp;
//
//	if(mPlayer == NULL)
//		return -1;
//
//	pre_3d_mode = _3d_mode;
//	mPlayer->control(mPlayer, CDX_CMD_GET_SOURCE_3D_MODE, (unsigned int)&tmp, 0);
//
//	if((unsigned int)tmp != _3d_mode)
//	{
//		_3d_mode = tmp;
//		LOGV("set _3d_mode to be %d when getting source mode", _3d_mode);
//	}
//
//	LOGV("_3d_mode = %d", _3d_mode);
//
//	return (int)_3d_mode;
//	*/
//	ALOGW("(f:%s, l:%d) fatal error! not support ", __FUNCTION__, __LINE__);
//	return UNKNOWN_ERROR;
//}
//
//status_t CedarXPlayer::setOutputDimensionType(int type)
//{
//    /*
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	status_t ret;
//	if(mPlayer == NULL)
//		return -1;
//
//	ret = mPlayer->control(mPlayer, CDX_CMD_SET_DISPLAY_MODE, type, 0);
//
//	LOGV("set output display mode to be %d, current display mode = %d", type, display_3d_mode);
//	if(display_3d_mode != CEDARX_DISPLAY_3D_MODE_ANAGLAGH && type != CEDARX_DISPLAY_3D_MODE_ANAGLAGH)
//	{
//		display_3d_mode = type;
//
//		set3DMode(_3d_mode, display_3d_mode);
//	}
//	else
//	{
		//* for switching on or off the anaglagh display, setting for display device(Overlay) will be
		//* done when next picture be render.
//		if(type == CEDARX_DISPLAY_3D_MODE_ANAGLAGH && display_3d_mode != CEDARX_DISPLAY_3D_MODE_ANAGLAGH)
//		{
			//* switch on anaglagh display.
//			if(anaglagh_en)
//				wait_anaglagh_display_change = 1;
//		}
//		else if(type != CEDARX_DISPLAY_3D_MODE_ANAGLAGH && display_3d_mode == CEDARX_DISPLAY_3D_MODE_ANAGLAGH)
//		{
			//* switch off anaglagh display.
//			display_type_tmp_save = type;
//			wait_anaglagh_display_change = 1;
//		}
//		else
//		{
//			if(_3d_mode != pre_3d_mode)
//			{
//				display_type_tmp_save = type;
//				wait_anaglagh_display_change = 1;
//			}
//		}
//	}
//
//	return ret;
//	*/
//	ALOGW("(f:%s, l:%d) fatal error! not support ", __FUNCTION__, __LINE__);
//	return UNKNOWN_ERROR;
//}
//
//int CedarXPlayer::getOutputDimensionType()
//{
//    /*
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	if(mPlayer == NULL)
//		return -1;
//
//	LOGV("current display 3d mode = %d", display_3d_mode);
//	return (int)display_3d_mode;
//	*/
//	ALOGW("(f:%s, l:%d) fatal error! not support ", __FUNCTION__, __LINE__);
//	return UNKNOWN_ERROR;
//}
//
//status_t CedarXPlayer::setAnaglaghType(int type)
//{
//    /*
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	int ret;
//	if(mPlayer == NULL)
//		return -1;
//
//	anaglagh_type = (cedarv_anaglath_trans_mode_e)type;
//	ret = mPlayer->control(mPlayer, CDX_CMD_SET_ANAGLAGH_TYPE, type, 0);
//	if(ret == 0)
//		anaglagh_en = 1;
//	else
//		anaglagh_en = 0;
//
//	return ret;
//    */
//    ALOGW("(f:%s, l:%d) fatal error! not support ", __FUNCTION__, __LINE__);
//    return UNKNOWN_ERROR;
//}
//
//int CedarXPlayer::getAnaglaghType()
//{
//    /*
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	if(mPlayer == NULL)
//		return -1;
//
//	return (int)anaglagh_type;
//    */
//    ALOGW("(f:%s, l:%d) fatal error! not support ", __FUNCTION__, __LINE__);
//    return UNKNOWN_ERROR;
//}
//
//status_t CedarXPlayer::getVideoEncode(char *encode)
//{
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//    return -1;
//}
//
//int CedarXPlayer::getVideoFrameRate()
//{
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//    return -1;
//}
//
//status_t CedarXPlayer::getAudioEncode(char *encode)
//{
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//    return -1;
//}
//
//int CedarXPlayer::getAudioBitRate()
//{
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//    return -1;
//}
//
//int CedarXPlayer::getAudioSampleRate()
//{
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//    return -1;
//}

status_t CedarXPlayer::enableScaleMode(bool enable, int width, int height)
{
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	if(mPlayer == NULL){
		return -1;
	}

	if(enable) {
		mMaxOutputWidth = width;
		mMaxOutputHeight = height;
	}

	return 0;
}

//status_t CedarXPlayer::setVppGate(bool enableVpp)
//{
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	mVppGate = enableVpp;
//	if(mVideoRenderer != NULL && !(mFlags & SUSPENDING)){
//		return mVideoRenderer->control(VIDEORENDER_CMD_VPPON, enableVpp);
//	}
//	return UNKNOWN_ERROR;
//}
//
//status_t CedarXPlayer::setLumaSharp(int value)
//{
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	mLumaSharp = value;
//	if(mVideoRenderer != NULL && !(mFlags & SUSPENDING)){
//		return mVideoRenderer->control(VIDEORENDER_CMD_SETLUMASHARP, value);
//	}
//	return UNKNOWN_ERROR;
//}
//
//status_t CedarXPlayer::setChromaSharp(int value)
//{
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	mChromaSharp = value;
//	if(mVideoRenderer != NULL && !(mFlags & SUSPENDING)){
//		return mVideoRenderer->control(VIDEORENDER_CMD_SETCHROMASHARP, value);
//	}
//	return UNKNOWN_ERROR;
//}
//
//status_t CedarXPlayer::setWhiteExtend(int value)
//{
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	mWhiteExtend = value;
//	if(mVideoRenderer != NULL && !(mFlags & SUSPENDING)){
//		return mVideoRenderer->control(VIDEORENDER_CMD_SETWHITEEXTEN, value);
//	}
//	return UNKNOWN_ERROR;
//}
//
//status_t CedarXPlayer::setBlackExtend(int value)
//{
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	mBlackExtend = value;
//	if(mVideoRenderer != NULL && !(mFlags & SUSPENDING)){
//		return mVideoRenderer->control(VIDEORENDER_CMD_SETBLACKEXTEN, value);
//	}
//	return UNKNOWN_ERROR;
//}

status_t CedarXPlayer::setChannelMuteMode(int muteMode)
{
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	if(mPlayer == NULL){
		return -1;
	}

	mPlayer->control(mPlayer, CDX_CMD_SET_AUDIOCHANNEL_MUTE, muteMode, 0);
	return OK;
}

int CedarXPlayer::getChannelMuteMode()
{
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	if(mPlayer == NULL){
		return -1;
	}

	int mute;
	mPlayer->control(mPlayer, CDX_CMD_GET_AUDIOCHANNEL_MUTE, (unsigned int)&mute, 0);
	return mute;
}

//status_t CedarXPlayer::extensionControl(int command, int para0, int para1)
//{
//    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
//	return OK;
//}
//
status_t CedarXPlayer::generalInterface(int cmd, int int1, int int2, int int3, void *p)
{
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	if (mPlayer == NULL) {
		return -1;
	}

	switch (cmd) {

	case MEDIAPLAYER_CMD_SET_BD_FOLDER_PLAY_MODE:
		mPlayer->control(mPlayer, CDX_CMD_PLAY_BD_FILE, int1, 0);
		break;

	case MEDIAPLAYER_CMD_GET_BD_FOLDER_PLAY_MODE:
		mPlayer->control(mPlayer, CDX_CMD_IS_PLAY_BD_FILE, (unsigned int)p, 0);
		break;

//    case MEDIAPLAYER_CMD_SET_PRESENTANTION_SCREEN:
//        mPlayer->control(mPlayer, CDX_CMD_SET_PRESENT_SCREEN, int1, 0);
//        break;
//
//    case MEDIAPLAYER_CMD_GET_PRESENTANTION_SCREEN:
//        mPlayer->control(mPlayer, CDX_CMD_GET_PRESENT_SCREEN, (unsigned int)p, 0);
//        break;

	case MEDIAPLAYER_CMD_SET_STREAMING_TYPE:
		mPlayer->control(mPlayer, CDX_CMD_SET_STREAMING_TYPE, int1, 0);
		break;

//	case MEDIAPLAYER_CMD_QUERY_HWLAYER_RENDER:
//		*((int*)p) = mExtendMember->mUseHardwareLayer;
//		break;
		
	case MEDIAPLAYER_CMD_IS_PLAYINGVIDEO:
//		if(0/*mAwesomePlayer*/) {
//			*((int*)p) = mAwesomePlayer->isPlaying();
//		}else{
			*((int*)p) = (mFlags & PLAYING) || (mFlags & CACHE_UNDERRUN);
//		}
		break;

//#ifdef MEDIAPLAYER_CMD_SET_ROTATION
//	case MEDIAPLAYER_CMD_SET_ROTATION:
//	{
//        LOGD("call CedarXPlayer dynamic MEDIAPLAYER_CMD_SET_ROTATION! anti-clock rotate=%d", int1);
//		if(!DYNAMIC_ROTATION_ENABLE)
//		{
//			return OK;
//		}
//        int nCurDynamicRotation;
//        switch(int1)
//        {
//            case 0: //VideoRotateAngle_0:
//            {
//                nCurDynamicRotation = 0;
//                break;
//            }
//            case 1: //VideoRotateAngle_90:
//            {
//                nCurDynamicRotation = 1;
//                break;
//            }
//            case 2: //VideoRotateAngle_180:
//            {
//                nCurDynamicRotation = 2;
//                break;
//            }
//            case 3: //VideoRotateAngle_270:
//            {
//                nCurDynamicRotation = 3;
//                break;
//            }
//            default:
//            {
//                LOGW("wrong param rotation:%d, keep rotation:%d",int1, mDynamicRotation);
//                nCurDynamicRotation = mDynamicRotation;
//                break;
//            }
//        }
//        if(0 == mRenderToDE)
//        {
//            LOGD("dynamic rotate, GPU render, cedarx do nothing! mDynamicRotation=%d", mDynamicRotation);
//            mDynamicRotation = nCurDynamicRotation;
//            return OK;
//        }
//        
//        if(nCurDynamicRotation != mDynamicRotation)
//        {
//            mDynamicRotation = nCurDynamicRotation;
//            if(0 == mRenderToDE)
//            {
//                LOGD("dynamic rotate, GPU render, cedarx do nothing!");
//            }
//            else
//            {
//                LOGD("dynamic rotate, hw render, cedarx vdeclib rotate!");
//                if(mVideoRenderer!=NULL)
//                {
//                    LOGD("(f:%s, l:%d), dynamic rotate, hw render, close screen", __FUNCTION__, __LINE__);
//                    mVideoRenderer->control(VIDEORENDER_CMD_SHOW, 0);
//                    LOGD("(f:%s, l:%d), delay sometime to wait hw", __FUNCTION__, __LINE__);
//                    usleep(50*1000);
//                }
//                mPlayer->control(mPlayer, CDX_CMD_SET_DYNAMIC_ROTATE, nCurDynamicRotation, 0);
//            }
//        }
//		break;
//	}
//#endif
#ifdef MEDIAPLAYER_CMD_SET_INIT_ROTATION
    case MEDIAPLAYER_CMD_SET_INIT_ROTATION:
	{
        ALOGD("call CedarXPlayer init MEDIAPLAYER_CMD_SET_INIT_ROTATION! anti-clock rotate=%d", int1);
        int nCurInitRotation;
        switch(int1)
        {
            case 0: //VideoRotateAngle_0:
            {
                nCurInitRotation = 0;
                break;
            }
            case 1: //VideoRotateAngle_90:
            {
                nCurInitRotation = 1;
                break;
            }
            case 2: //VideoRotateAngle_180:
            {
                nCurInitRotation = 2;
                break;
            }
            case 3: //VideoRotateAngle_270:
            {
                nCurInitRotation = 3;
                break;
            }
            default:
            {
                ALOGW("wrong param init rotation:%d, keep init rotation:%d",int1, mInitRotation);
                nCurInitRotation = mInitRotation;
                break;
            }
        }
        if(nCurInitRotation != mInitRotation)
        {
            mInitRotation = nCurInitRotation;
            ALOGD("init rotate=%d, cedarx will decide whether rotate in prepareAsync()!", mInitRotation);
        }
		break;
	}
#endif

	default:
		break;
	}

	return OK;
}

uint32_t CedarXPlayer::flags() const {
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	if(mCanSeek) {
		return MediaExtractor::CAN_PAUSE | MediaExtractor::CAN_SEEK  | MediaExtractor::CAN_SEEK_FORWARD  | MediaExtractor::CAN_SEEK_BACKWARD;
	}
	else {
		return MediaExtractor::CAN_PAUSE;
	}
}

int CedarXPlayer::nativeSuspend()
{
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	if (mFlags & PLAYING) {
		LOGV("nativeSuspend may fail, I'am still playing");
		return -1;
	}

//	if(mPlayer != NULL){
//		LOGV("I'm force to quit!");
//		mPlayer->control(mPlayer, CDX_CMD_STOP_ASYNC, 0, 0);
//	}

	suspend();

	mFlags |= NATIVE_SUSPENDING;

	return 0;
}

status_t CedarXPlayer::setVideoScalingMode(int32_t mode) {
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    Mutex::Autolock lock(mLock);
    return setVideoScalingMode_l(mode);
}

status_t CedarXPlayer::setVideoScalingMode_l(int32_t mode) {
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	LOGV("mVideoScalingMode %d, mode %d", mVideoScalingMode, mode);
	if(mVideoScalingMode == mode) {
		return OK;
	}

    mVideoScalingMode = mode;
    //if (mNativeWindow != NULL) 
    if (mVideoLayerId >= 0) 
    {
//        status_t err = native_window_set_scaling_mode(
//                mNativeWindow.get(), mVideoScalingMode);
//        if (err != OK) {
//            LOGW("Failed to set scaling mode: %d", err);
//        }
        ALOGE("(f:%s, l:%d) not support window videoScaling mode!", __FUNCTION__, __LINE__);
    }
    return OK;
}

#if (CEDARX_ANDROID_VERSION >= 7)
/*
 * add by weihongqiang
 * the three functions below are useless at moment.
 */
enum {
    // These keys must be in sync with the keys in TimedText.java
    KEY_DISPLAY_FLAGS                 = 1, // int
    KEY_STYLE_FLAGS                   = 2, // int
    KEY_BACKGROUND_COLOR_RGBA         = 3, // int
    KEY_HIGHLIGHT_COLOR_RGBA          = 4, // int
    KEY_SCROLL_DELAY                  = 5, // int
    KEY_WRAP_TEXT                     = 6, // int
    KEY_START_TIME                    = 7, // int
    KEY_STRUCT_BLINKING_TEXT_LIST     = 8, // List<CharPos>
    KEY_STRUCT_FONT_LIST              = 9, // List<Font>
    KEY_STRUCT_HIGHLIGHT_LIST         = 10, // List<CharPos>
    KEY_STRUCT_HYPER_TEXT_LIST        = 11, // List<HyperText>
    KEY_STRUCT_KARAOKE_LIST           = 12, // List<Karaoke>
    KEY_STRUCT_STYLE_LIST             = 13, // List<Style>
    KEY_STRUCT_TEXT_POS               = 14, // TextPos
    KEY_STRUCT_JUSTIFICATION          = 15, // Justification
    KEY_STRUCT_TEXT                   = 16, // Text

    KEY_STRUCT_AWEXTEND_BMP           = 50, // bmp subtitle such as idxsub and pgs.
    KEY_STRUCT_AWEXTEND_PIXEL_FORMAT  = 51,  //PIXEL_FORMAT_RGBA_8888
    KEY_STRUCT_AWEXTEND_PICWIDTH      = 52, // bmp subtitle item's width
    KEY_STRUCT_AWEXTEND_PICHEIGHT     = 53, // bmp subtitle item's height
    KEY_STRUCT_AWEXTEND_SUBDISPPOS    = 54, // text subtitle's position, SUB_DISPPOS_BOT_LEFT
    KEY_STRUCT_AWEXTEND_SCREENRECT    = 55, // text subtitle's position need a whole area as a ref.
    KEY_STRUCT_AWEXTEND_HIDESUB       = 56, // when multi subtitle show the same time, such as ssa, we need to tell app which subtitle need to hide.

    KEY_GLOBAL_SETTING                = 101,
    KEY_LOCAL_SETTING                 = 102,
    KEY_START_CHAR                    = 103,
    KEY_END_CHAR                      = 104,
    KEY_FONT_ID                       = 105,
    KEY_FONT_SIZE                     = 106,
    KEY_TEXT_COLOR_RGBA               = 107,
};

size_t CedarXPlayer::countTracks() const {
	//Do not change the order of track.
    return mMediaInfo.mVideoStreamCount
    		+ mMediaInfo.mAudioStreamCount + mMediaInfo.mSubtitleStreamCount;
}

#if 0
int convertToUTF16LE(char *pDesString, int nDesBufSize, const char *pSrcString)
{
    const char* enc = NULL;
    const char* desCharset = NULL;
    int nDesTextLen;
    enc = "UTF-8";
    desCharset = "UTF-16LE";
    if (enc) 
    {
        UErrorCode status = U_ZERO_ERROR;

        UConverter *conv = ucnv_open(enc, &status);
        if (U_FAILURE(status)) 
        {
            LOGW("could not create UConverter for %s\n", enc);
            
            return -1;
        }
        
        UConverter *desConv = ucnv_open(desCharset, &status);
        if (U_FAILURE(status)) 
        {
            LOGW("could not create UConverter for [%s]\n", desCharset);
            
            ucnv_close(conv);
            return -1;
        }

        // first we need to untangle the utf8 and convert it back to the original bytes
        // since we are reducing the length of the string, we can do this in place
        const char* src = (const char*)pSrcString;
        //LOGV("CedarXTimedText::convertUniCode src = %s\n",src);
        int len = strlen((char *)src);
		//LOGV("CedarXTimedText::convertUniCode len = %d\n",len);
        // now convert from native encoding to UTF-8
        int targetLength = len * 3 + 1;
        if(targetLength > nDesBufSize)
        {
            LOGW("(f:%s, l:%d) fatal error! targetLength[%d]>nDesBufSize[%d], need cut!", __FUNCTION__, __LINE__, targetLength, nDesBufSize);
            targetLength = nDesBufSize;
        }
        memset(pDesString,0,nDesBufSize);
        char* target = (char*)&pDesString[0];
        
		//LOGV("(f:%s, l:%d), target[%p], targetLength[%d], src[%p], len[%d]", __FUNCTION__, __LINE__, target, targetLength, src, len);
        ucnv_convertEx(desConv, conv, &target, (const char*)target + targetLength,&src, (const char*)src + len, NULL, NULL, NULL, NULL, true, true, &status);
        if (U_FAILURE(status)) 
        {
            LOGW("ucnv_convertEx failed: %d\n", status);
            
            memset(pDesString,0,nDesBufSize);
        } 
        nDesTextLen = target - (char*)pDesString;
        //LOGV("(f:%s, l:%d)_2, target[%p], targetLength[%d], src[%p]", __FUNCTION__, __LINE__, target, (int)(target - (char*)subTextBuf), src, (int)(src - (char*)sub_info->subTextBuf));
        //LOGD("CedarXTimedText::convertUniCode src = %s,target = %s\n", pSrcString, pDesString);
//        for(int i=0; i<strlen(pSrcString);i++ )
//        {
//            LOGD("src[0x%x]", pSrcString[i]);
//        }
//        for(int i=0; i<nDesTextLen;i++ )
//        {
//            LOGD("[0x%x]", pDesString[i]);
//        }
//	        int targetlen2 = strlen((char *)subTextBuf);
//          LOGV("(f:%s, l:%d), targetLength2[%d], nDesTextLen[%d]", __FUNCTION__, __LINE__, targetlen2, nDesTextLen);
        
        ucnv_close(conv);
        ucnv_close(desConv);
    }
    return 0;
}
#endif

status_t CedarXPlayer::getTrackInfo(Parcel *reply) const {
    Mutex::Autolock autoLock(mLock);
    size_t trackCount = countTracks();
    LOGV("tack count %u, %d:%d:%d", trackCount, mMediaInfo.mVideoStreamCount,
    		mMediaInfo.mAudioStreamCount, mMediaInfo.mSubtitleStreamCount);
    reply->writeInt32(trackCount);
    const char *lang;
    for(size_t i = 0; i < mMediaInfo.mVideoStreamCount; ++i) {
    	reply->writeInt32(2);
    	reply->writeInt32(MEDIA_TRACK_TYPE_VIDEO);
    	lang = "und";
    	reply->writeString16(String16(lang));
    }

    for(size_t i = 0; i < mMediaInfo.mAudioStreamCount; ++i) {
    	reply->writeInt32(2);
    	reply->writeInt32(MEDIA_TRACK_TYPE_AUDIO);
    	lang = mMediaInfo.mAudioInfo[i].mLang;
    	if(!strlen(lang)) {
    		lang = "und";
    	}
    	reply->writeString16(String16(lang));
    }

    for(size_t i = 0; i < mMediaInfo.mSubtitleStreamCount; ++i) {
    	reply->writeInt32(2);
    	reply->writeInt32(MEDIA_TRACK_TYPE_TIMEDTEXT);
    	lang = mMediaInfo.mSubtitleInfo[i].mLang;
        //LOGD("(f:%s, l:%d) i=%d, lang_len=%d, lang[%s]", __FUNCTION__, __LINE__, i, strlen(lang), lang);
    	if(!strlen(lang)) {
    		lang = "und";
    	}
        char string16Array[1024];
        const char16_t* pDesString16 = (const char16_t*)string16Array;
        //convertToUTF16LE(string16Array, 1024, lang);
    	reply->writeString16(String16(lang));
    	//reply->writeString16(String16(pDesString16));
    }

    return OK;
}

status_t CedarXPlayer::selectTrack(size_t trackIndex, bool select) {
    ATRACE_CALL();
    ALOGV("selectTrack: trackIndex = %d and select=%d", trackIndex, select);
    Mutex::Autolock autoLock(mLock);
    size_t trackCount = countTracks();

    if (trackIndex >= trackCount) {
        ALOGE("Track index (%d) is out of range [0, %d)", trackIndex, trackCount);
        return ERROR_OUT_OF_RANGE;
    }
    ssize_t videoTrackIndex = (ssize_t)trackIndex;
    ssize_t audioTrackIndex = (ssize_t)(videoTrackIndex - mMediaInfo.mVideoStreamCount);
    ssize_t subTrackIndex   = (ssize_t)(audioTrackIndex - mMediaInfo.mAudioStreamCount);

    status_t err = OK;
    if(videoTrackIndex < mMediaInfo.mVideoStreamCount) {
    	//Video track. ignore.
    } else if(audioTrackIndex < mMediaInfo.mAudioStreamCount) {
    	if(!select) {
    		err =  ERROR_UNSUPPORTED;
    	} else {
    		err = switchTrack_l(audioTrackIndex);
    	}
    } else if(subTrackIndex < mMediaInfo.mSubtitleStreamCount) {
    	if(!select) {
    		if(getSubGate_l()) {
    			err = setSubGate_l(select);
    		}
    	} else {
    		if(!getSubGate_l()) {
    			setSubGate_l(true);
    		}
    		switchSub_l(subTrackIndex);
    	}
    } else {
    	err =  ERROR_UNSUPPORTED;
    }

    return err;
}


status_t CedarXPlayer::invoke(const Parcel &request, Parcel *reply) {
	ALOGV("invoke");
    ATRACE_CALL();
    if (NULL == reply) {
        return android::BAD_VALUE;
    }
    int32_t methodId;
    status_t ret = request.readInt32(&methodId);
    if (ret != android::OK) {
        return ret;
    }
    switch(methodId) {
        case INVOKE_ID_SET_VIDEO_SCALING_MODE:
        {
            int mode = request.readInt32();
            return setVideoScalingMode(mode);
        }

        case INVOKE_ID_GET_TRACK_INFO:
        {
            return getTrackInfo(reply);
        }
        case INVOKE_ID_ADD_EXTERNAL_SOURCE:
        {
        	//update track info.
        	mPlayer->control(mPlayer, CDX_CMD_GET_MEDIAINFO, (unsigned int)&mMediaInfo, 0);
            return OK;
        }
        case INVOKE_ID_ADD_EXTERNAL_SOURCE_FD:
        {
        	ALOGV("add external source fd");
        	Mutex::Autolock autoLock(mLock);

        	CedarXExternFdDesc extFd;
        	//Must dup a new.
        	extFd.fd 		= dup(request.readFileDescriptor());
            extFd.offset 	= request.readInt64();
            extFd.length  	= request.readInt64();
            extFd.cur_offset = extFd.offset ;
            String8 mimeType(request.readString16());
            mPlayer->control(mPlayer, CDX_CMD_ADD_EXTERNAL_SOURCE_FD,
            		(unsigned int)&extFd, (unsigned int)mimeType.string());
        	//update track info after new external source added.
        	mPlayer->control(mPlayer, CDX_CMD_GET_MEDIAINFO, (unsigned int)&mMediaInfo, 0);
            return OK;

        }
        case INVOKE_ID_SELECT_TRACK:
        {
            int trackIndex = request.readInt32();
            return selectTrack(trackIndex, true /* select */);
        }
        case INVOKE_ID_UNSELECT_TRACK:
        {
            int trackIndex = request.readInt32();
            return selectTrack(trackIndex, false /* select */);
        }

        default:
        {
            return ERROR_UNSUPPORTED;
        }
    }
    // It will not reach here.
    return OK;
}
#endif

#if 0

static int g_dummy_render_frame_id_last = -1;
static int g_dummy_render_frame_id_curr = -1;

int CedarXPlayer::StagefrightVideoRenderInit(int width, int height, int format, void *frame_info)
{
	g_dummy_render_frame_id_last = -1;
	g_dummy_render_frame_id_curr = -1;
	return 0;
}

void CedarXPlayer::StagefrightVideoRenderData(void *frame_info, int frame_id)
{
	g_dummy_render_frame_id_last = g_dummy_render_frame_id_curr;
	g_dummy_render_frame_id_curr = frame_id;
	return ;
}

int CedarXPlayer::StagefrightVideoRenderGetFrameID()
{
	return g_dummy_render_frame_id_last;
}

void CedarXPlayer::StagefrightVideoRenderExit()
{
}

#else

int CedarXPlayer::StagefrightVideoRenderInit(int width, int height, int format, int nRenderMode)
{
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    int nGUIInitRotation;
	//VideoPicture* frm_inf;
	android_native_rect_t crop; //android_native_rect_t struct is defined in system\core\include\system\window.h

    nGUIInitRotation  = (mInitRotation + mMediaInfo.mVideoInfo[0].mFileSelfRotation)%4;
    //LOGD("(f:%s, l:%d) mInitRotation=%d, mFileSelfRotation=%d, nGUIInitRotation=%d", __FUNCTION__, __LINE__, mInitRotation, mMediaInfo.mVideoInfo[0].mFileSelfRotation, nGUIInitRotation);
    if(mMediaInfo.mVideoInfo[0].mFileSelfRotation!=0)
    {
        LOGD("(f:%s, l:%d) Be careful. mFileSelfRotation[%d]", __FUNCTION__, __LINE__, mMediaInfo.mVideoInfo[0].mFileSelfRotation);
    }
	mDisplayWidth 	= width;
	mDisplayHeight 	= height;
//    if(mDynamicRotation)
//    {
//        LOGD("(f:%s, l:%d) mDynamicRotation=%d", __FUNCTION__, __LINE__, mDynamicRotation);
//    }
	mFirstFrame 	= 1;

    if(format == PIXEL_FORMAT_YUV_MB32_422)
	    mDisplayFormat = (int32_t)VIRTUAL_HWC_FORMAT_MBYUV422;
	else if (format == PIXEL_FORMAT_YUV_MB32_420)
		mDisplayFormat = (int32_t)VIRTUAL_HWC_FORMAT_MBYUV420;
	else if(format == PIXEL_FORMAT_YV12)
	    mDisplayFormat = (int32_t)VIRTUAL_HWC_FORMAT_YUV420PLANAR;//(int32_t)HAL_PIXEL_FORMAT_YV12;
	else if(format == PIXEL_FORMAT_NV21)
        mDisplayFormat = (int32_t)VIRTUAL_HWC_FORMAT_YUV420VUC;//(int32_t)HAL_PIXEL_FORMAT_YV12;
	else
    {
        LOGW("(f:%s, l:%d) fatal error! width=%d, height=%d, format=%x\n", __FUNCTION__, __LINE__, width, height,format);
	    mDisplayFormat = VIRTUAL_HWC_FORMAT_MBYUV420;    //* format should be CEDARV_PIXEL_FORMAT_MB_UV_COMBINED_YUV420
	    
	}
    //* other case like 'CEDARV_PIXEL_FORMAT_PLANNER_YUV420', 'CEDARV_PIXEL_FORMAT_RGB888'
	//* are ignored here. There are currently not used yet. 
	//* To Be Done.
//    if(format == CEDARV_PIXEL_FORMAT_PLANNER_YVU420)
//    {
//        mDisplayFormat = VIRTUAL_HWC_FORMAT_YUV420PLANAR;   //HWC_FORMAT_YUV420PLANAR, (int32_t)HAL_PIXEL_FORMAT_YV12
//    }
//    else
//    {
//        LOGW("(f:%s, l:%d) fatal error! width=%d, height=%d, format=0x%x, pixelformat should be CEDARV_PIXEL_FORMAT_PLANNER_YVU420", 
//            __FUNCTION__, __LINE__, width, height, format);
//    }

	ALOGD("video render size:%dx%d, format:0x%x", width, height, format);

	//if (mNativeWindow == NULL)
	if (mVideoLayerId < 0)
	    return -1;

    int nAppVideoWidth  = width; //notifyListener_l_MEDIA_SET_VIDEO_SIZE will notify app the video size, should be the same to GPU_buffersize when use GPU render. and keep the same as original picture.
	int nAppVideoHeight = height;
    if(mRenderToDE) // if set to hwlayer, we should consider vdeclib rotate
    {
        //if(1 == mDynamicRotation || 3 == mDynamicRotation)
        if(1 == mInitRotation || 3 == mInitRotation)
        {
            nAppVideoWidth = height;
			nAppVideoHeight = width;
        }
    }
    else
    {
		if(1 == nGUIInitRotation || 3 == nGUIInitRotation)	  //in gui_request_frame_mode, only use initRotation.
		{
			LOGD("GUI_mode_for_android42:init rotate[%d] in StagefrightVideoRenderInit()!", nGUIInitRotation);
			nAppVideoWidth = height;
			nAppVideoHeight = width;
		}

#if (defined(__CHIP_VERSION_F81))
#elif (defined(__CHIP_VERSION_F23) || (defined(__CHIP_VERSION_F51))|| defined(__CHIP_VERSION_F50))
  #if (1 == ADAPT_A10_GPU_RENDER)
        int nGpuBufWidth, nGpuBufHeight;
        nGpuBufWidth = (nAppVideoWidth + 15) & ~15;
        nGpuBufHeight = nAppVideoHeight;
        //A10's GPU has a bug, we can avoid it
        if(nGpuBufHeight%8 != 0)
        {
            if((nGpuBufWidth*nGpuBufHeight)%256 != 0)
            { 
                nGpuBufHeight = (nGpuBufHeight+7)&~7;
                LOGW("(f:%s, l:%d) the video height to tell app, change from [%d] to [%d]", __FUNCTION__, __LINE__, nAppVideoHeight, nGpuBufHeight);
            }
        }
        nAppVideoHeight = nGpuBufHeight;
        //nAppVideoWidth = nGpuBufWidth;
  #endif
#elif (defined(__CHIP_VERSION_F33) )
#else
        #error "Unknown chip type!"
#endif
    }
    

	if(mVideoWidth!=nAppVideoWidth ||  mVideoHeight!=nAppVideoHeight)
	{
        mVideoWidth = nAppVideoWidth;
		mVideoHeight = nAppVideoHeight;

		LOGD("(f:%s, l:%d) nAppVideoWidth[%d], nAppVideoHeight[%d], notifyListener_l(MEDIA_SET_VIDEO_SIZE_)", __FUNCTION__, __LINE__, nAppVideoWidth, nAppVideoHeight);
		notifyListener_l(MEDIA_SET_VIDEO_SIZE, nAppVideoWidth, nAppVideoHeight);
	}
	//frm_inf = (VideoPicture *)frame_info;

	sp<MetaData> meta = new MetaData;
	//meta->setInt32(kKeyScreenID, mScreenID);
    meta->setInt32(kKeyColorFormat, mDisplayFormat);
    meta->setInt32(kKeyWidth, mDisplayWidth);
    meta->setInt32(kKeyHeight, mDisplayHeight);
    meta->setInt32(kKeyRotation, nGUIInitRotation);
 	meta->setInt32(kKeyIsDRM, mIsDRMMedia);

    LOGV("StagefrightVideoRenderInit mScreenID:%d",mScreenID);

    mVideoRenderer.clear();

    // Must ensure that mVideoRenderer's destructor is actually executed
    // before creating a new one.
    IPCThreadState::self()->flushCommands();
    if(nRenderMode == VideoRender_HW)//HWC_FORMAT_YUV420PLANAR, HAL_PIXEL_FORMAT_YV12)
    {
		mRenderToDE = 1;
        LOGD("(f:%s, l:%d) mDisplayFormat[0x%x], new CedarXDirectHwRenderer", __FUNCTION__, __LINE__, mDisplayFormat);
    	mVideoRenderer = new CedarXDirectHwRenderer(mVideoLayerId, meta);

    	//set3DMode(_3d_mode, display_3d_mode);
    	set3DMode(_3d_mode);
    }
    else
    {
    	//if (0 == mNativeWindow->perform(mNativeWindow.get(), NATIVE_WINDOW_GETPARAMETER,NATIVE_WINDOW_CMD_GET_SURFACE_TEXTURE_TYPE, 0)
        //    || !mExtendMember->mUseHardwareLayer || TEST_FORCE_GPU_RENDER)
        if(1)
    	{
        	//mLocalRenderFrameIDCurr = -1;
			mRenderToDE = 0;
            LOGD("(f:%s, l:%d) mDisplayFormat[0x%x], new CedarXLocalRenderer", __FUNCTION__, __LINE__, mDisplayFormat);
    		mVideoRenderer = new CedarXLocalRenderer(mVideoLayerId, meta);
    	}
//    	else
//    	{
//			mRenderToDE = 1;
//            LOGD("(f:%s, l:%d) mDisplayFormat[0x%x], new CedarXDirectHwRenderer", __FUNCTION__, __LINE__, mDisplayFormat);
//    		mVideoRenderer = new CedarXDirectHwRenderer(mVideoLayerId, meta);
//    	}
        //set3DMode(_3d_mode, display_3d_mode);
        set3DMode(_3d_mode);
    }
    setVideoScalingMode_l(mVideoScalingMode);

//	if(frm_inf->rotate_angle != 0)
//    {
//		crop.top = frm_inf->top_offset;
//		crop.left = frm_inf->left_offset;
//		crop.right = frm_inf->display_width + frm_inf->left_offset;
//		crop.bottom = frm_inf->display_height + frm_inf->top_offset;
//
//		mVideoRenderer->control(VIDEORENDER_CMD_SET_CROP, (int)&crop);
//    }

    return 0;
}

void CedarXPlayer::StagefrightVideoRenderExit()
{
    LOGV("(f:%s, l:%d)", __FUNCTION__, __LINE__);
	if(mVideoRenderer != NULL)
	{
		if(mDisplayFormat != VIRTUAL_HWC_FORMAT_YUV420PLANAR)//HWC_FORMAT_YUV420PLANAR, HAL_PIXEL_FORMAT_YV12)
		{
			mVideoRenderer->control(VIDEORENDER_CMD_SHOW, 0);
            LOGD("(f:%s, l:%d), delay sometime to wait hw", __FUNCTION__, __LINE__);
            usleep(50*1000);
		}
        else
        {
            if(mRenderToDE)
            {
                LOGD("(f:%s, l:%d), render to de, close screen", __FUNCTION__, __LINE__);
                mVideoRenderer->control(VIDEORENDER_CMD_SHOW, 0);
                LOGD("(f:%s, l:%d), delay sometime to wait hw", __FUNCTION__, __LINE__);
                //usleep(50*1000);
            }
        }
	}
}

void CedarXPlayer::StagefrightVideoRenderData(void *frame_info, int frame_id)
{
	Mutex::Autolock autoLock(mLockNativeWindow);

_render_again:
	if(mVideoRenderer != NULL)
	{
		VideoPicture *frm_inf = (VideoPicture *) frame_info;
        unsigned int v3d_width = frm_inf->nWidth;
        unsigned int v3d_height = frm_inf->nHeight;
        if(mVideo3dInfo.display_mode == VIRTUAL_HWC_3D_OUT_MODE_ANAGLAGH)
        {
            if(mVideo3dInfo.src_mode == VIRTUAL_HWC_3D_SRC_MODE_SSH)
            {
                v3d_width = ((frm_inf->nWidth/2 + 0xF) & 0xFFFFFFF0);
            }
            else if(mVideo3dInfo.src_mode == VIRTUAL_HWC_3D_SRC_MODE_TB)
            {
                v3d_height = ((frm_inf->nHeight/2 + 0xF) & 0xFFFFFFF0);
            }
        }

//		if(DYNAMIC_ROTATION_ENABLE)
//		{
//			if(frm_inf->display_height != mVideo3dInfo.height)
//			{
//				if(frm_inf->display_height != (uint32_t)mDisplayHeight)
//					set3DMode(1);
//				else
//					set3DMode(0);
//			}
//		}

#if 1
		if(v3d_width * v3d_height != mVideo3dInfo.width * mVideo3dInfo.height)
		{
            ALOGW("resolution has changed! frm_inf->display_width[%d],frm_inf->display_height[%d],mVideo3dInfo.width[%d],mVideo3dInfo.height[%d]",
                v3d_width, v3d_height, mVideo3dInfo.width, mVideo3dInfo.height);
//			StagefrightVideoRenderExit();
			StagefrightVideoRenderInit(frm_inf->nWidth, frm_inf->nHeight, frm_inf->ePixelFormat, VideoRender_HW);
		}else if((int32_t)frm_inf->nHeight != mDisplayHeight)
		{
            ALOGD("rotate happens?frm_inf->display_height[%d] != mDisplayHeight[%d]", frm_inf->nHeight, mDisplayHeight);
            //dy_rotate_flag = 1;
			StagefrightVideoRenderExit();
			StagefrightVideoRenderInit(frm_inf->nWidth, frm_inf->nHeight, frm_inf->ePixelFormat, VideoRender_HW);
            ALOGD("(f:%s, l:%d), delay sometime to wait hw", __FUNCTION__, __LINE__);
            //usleep(50*1000);
		}
#endif
        //LOGD("(f:%s, l:%d) mDisplayFormat=[0x%x], frm_inf->pixel_format=[0x%x]", __FUNCTION__, __LINE__, mDisplayFormat, frm_inf->pixel_format);
		if(mDisplayFormat != VIRTUAL_HWC_FORMAT_YUV420PLANAR)//HAL_PIXEL_FORMAT_YV12)
		{
		}
        else
        {
           ALOGW("(f:%s, l:%d) why mDisplayFormat[0x%x] == VIRTUAL_HWC_FORMAT_YUV420PLANAR", __FUNCTION__, __LINE__, mDisplayFormat);
        }
		{
			//libhwclayerpara_t overlay_para;
            Virtuallibhwclayerpara virtual_overlay_para;

			memset(&virtual_overlay_para, 0, sizeof(Virtuallibhwclayerpara));

//			if(wait_anaglagh_display_change)
//			{
//				LOGV("+++++++++++++ display 3d mode == %d", display_3d_mode);
//				if(display_3d_mode != CEDARX_DISPLAY_3D_MODE_ANAGLAGH)
//				{
					//* switch on anaglagh display.
//					if(anaglagh_type == (uint32_t)frm_inf->anaglath_transform_mode)
//					{
//						display_3d_mode = CEDARX_DISPLAY_3D_MODE_ANAGLAGH;
//						set3DMode(_3d_mode, display_3d_mode);
//						wait_anaglagh_display_change = 0;
//					}
//				}
//				else
//				{
					//* switch off anaglagh display.
//					display_3d_mode = display_type_tmp_save;
//					set3DMode(_3d_mode, display_3d_mode);
//					wait_anaglagh_display_change = 0;
//				}
//			}
            virtual_overlay_para.source3dMode   = (cedarv_3d_mode_e)_3d_mode;
            //virtual_overlay_para.displayMode    = (cedarx_display_3d_mode_e)display_3d_mode;
            virtual_overlay_para.pixel_format   = (EPIXELFORMAT)frm_inf->ePixelFormat;
            
			virtual_overlay_para.bProgressiveSrc = frm_inf->bIsProgressive;
			virtual_overlay_para.bTopFieldFirst = frm_inf->bTopFieldFirst;
#if 1 //deinterlace support
			virtual_overlay_para.flag_addr              = (unsigned long)frm_inf->pMafData;
			virtual_overlay_para.flag_stride            = frm_inf->nMafFlagStride;
			virtual_overlay_para.maf_valid              = frm_inf->bMafValid;
			virtual_overlay_para.pre_frame_valid        = frm_inf->bPreFrmValid;
#endif
            virtual_overlay_para.top_y  = (unsigned int)MemAdapterGetPhysicAddressCpu(frm_inf->pData0);
            virtual_overlay_para.top_u  = (unsigned int)MemAdapterGetPhysicAddressCpu(frm_inf->pData1);
            virtual_overlay_para.top_v  = (unsigned int)NULL;
            virtual_overlay_para.top_y2	= (unsigned int)MemAdapterGetPhysicAddressCpu(frm_inf->pData2);
			virtual_overlay_para.top_u2	= (unsigned int)MemAdapterGetPhysicAddressCpu(frm_inf->pData3);
            virtual_overlay_para.top_v2	= (unsigned int)NULL;

			virtual_overlay_para.number = frame_id;
			mVideoRenderer->render(&virtual_overlay_para, 0);
			//LOGV("render frame id:%d",frame_id);
			if(mFirstFrame)
			{
				mVideoRenderer->control(VIDEORENDER_CMD_SHOW, 1);
				mFirstFrame = 0;
#if (CEDARX_ANDROID_VERSION >= 8)
				//add by weihongqiang.
				notifyListener_l(MEDIA_INFO, MEDIA_INFO_RENDERING_START);
#endif
			}
		}
	}
	else
	{
		//if (mDisplayFormat == HAL_PIXEL_FORMAT_YV12)
		//{
		//	mLocalRenderFrameIDCurr = frame_id;
		//}
        ALOGD("(f:%s, l:%d) mVideoRenderer=NULL, mNativeWindow=[%d]", __FUNCTION__, __LINE__, mVideoLayerId);
		//if (mNativeWindow != NULL)
		if (mVideoLayerId >= 0)
		{
			sp<MetaData> meta = new MetaData;
			//meta->setInt32(kKeyScreenID, mScreenID);
		    meta->setInt32(kKeyColorFormat, mDisplayFormat);
		    meta->setInt32(kKeyWidth, mDisplayWidth);
		    meta->setInt32(kKeyHeight, mDisplayHeight);
		    meta->setInt32(kKeyIsDRM, mIsDRMMedia);
		    ALOGD("reinit mVideoRenderer, mDisplayFormat=[0x%x]", mDisplayFormat);
		    // Must ensure that mVideoRenderer's destructor is actually executed
		    // before creating a new one.
		    IPCThreadState::self()->flushCommands();

		    setVideoScalingMode_l(mVideoScalingMode);

		    if(mDisplayFormat != VIRTUAL_HWC_FORMAT_YUV420PLANAR)//HAL_PIXEL_FORMAT_YV12)
		    {
		    }
            else
            {
                ALOGW("(f:%s, l:%d) why mDisplayFormat[0x%x] == VIRTUAL_HWC_FORMAT_YUV420PLANAR", __FUNCTION__, __LINE__, mDisplayFormat);
            }
            
		    {
				mRenderToDE = 1;
	    		mFirstFrame = 1;
		    	mVideoRenderer = new CedarXDirectHwRenderer(mVideoLayerId, meta);
		    	//set3DMode(_3d_mode, display_3d_mode);
		    	set3DMode(_3d_mode);
		    }
//		    else
//		    {
//            #if (defined(__CHIP_VERSION_F23) || defined(__CHIP_VERSION_F51) || defined(__CHIP_VERSION_F33) || defined(__CHIP_VERSION_F50))//a31 temp change to this, because of vertical screen
//                mRenderToDE = 0;
//		        mVideoRenderer = new CedarXLocalRenderer(mVideoLayerId, meta);
//            #elif (defined(__CHIP_VERSION_F33))
//                if (0 == mNativeWindow->perform(mNativeWindow.get(), NATIVE_WINDOW_GETPARAMETER,NATIVE_WINDOW_CMD_GET_SURFACE_TEXTURE_TYPE, 0)
//                    || TEST_FORCE_GPU_RENDER)
//                if(0)
//		    	{
//                    LOGD("(f:%s, l:%d) reinit mVideoRenderer, use GPU", __FUNCTION__, __LINE__);
//					mRenderToDE = 0;
//		    		mVideoRenderer = new CedarXLocalRenderer(mVideoLayerId, meta);
//		    	}
//		    	else
//		    	{
//                    LOGD("(f:%s, l:%d) reinit mVideoRenderer, use DirectHwRender!", __FUNCTION__, __LINE__);
//		    		mRenderToDE = 1;
//		    		mFirstFrame = 1;
//		    		mVideoRenderer = new CedarXDirectHwRenderer(mVideoLayerId, meta);
//		    	}
//            #else
//                #error "Unknown chip type!"
//            #endif
//				set3DMode(_3d_mode, display_3d_mode);
//		    }

		    goto _render_again;
		}
	}
}

int CedarXPlayer::StagefrightVideoRenderGetFrameID()
{
	int ret = -1;

	if(mRenderToDE)
	{
		Mutex::Autolock autoLock(mLockNativeWindow);
		if(mVideoRenderer != NULL)
			ret = mVideoRenderer->control(VIDEORENDER_CMD_GETCURFRAMEPARA, 0);
		//LOGV("get disp frame id:%d",ret);
	}
	else
	{
		//ret = mLocalRenderFrameIDCurr;
		ALOGE("(f:%s, l:%d) fatal error! why call this?", __FUNCTION__, __LINE__);
	}

	//LOGV("get disp frame id:%d",ret);

	return ret;
}

/*******************************************************************************
Function name: android.CedarXPlayer.StagefrightVideoRenderDequeueFrame
Description: 
    
Parameters: 
    
Return: 
    ret = 0: ok
    ret = -1: error
    ret = 10: mNativeWindow = NULL;
Time: 2013/5/31
*******************************************************************************/
int CedarXPlayer::StagefrightVideoRenderDequeueFrame(void *frame_info, int frame_id, ANativeWindowBufferCedarXWrapper *pANativeWindowBuffer)
{
    int ret = -1;
    Mutex::Autolock autoLock(mLockNativeWindow);

_render_again:
	if(mVideoRenderer != NULL)
	{
		VideoPicture *frm_inf = (VideoPicture *) frame_info;


#if 1
		if(frm_inf->nWidth * frm_inf->nHeight != mVideo3dInfo.width * mVideo3dInfo.height)
		{
            ALOGW("resolution has changed! frm_inf->display_width[%d],frm_inf->display_height[%d],mVideo3dInfo.width[%d],mVideo3dInfo.height[%d]",
                frm_inf->nWidth, frm_inf->nHeight, mVideo3dInfo.width, mVideo3dInfo.height);
//			StagefrightVideoRenderExit();
			StagefrightVideoRenderInit(frm_inf->nWidth, frm_inf->nHeight, frm_inf->ePixelFormat, VideoRender_SW);
		}else if((int32_t)frm_inf->nHeight != mDisplayHeight)
		{
            ALOGD("rotate happens?frm_inf->display_height[%d] != mDisplayHeight[%d]", frm_inf->nHeight, mDisplayHeight);
            //dy_rotate_flag = 1;
			StagefrightVideoRenderExit();
			StagefrightVideoRenderInit(frm_inf->nWidth, frm_inf->nHeight, frm_inf->ePixelFormat, VideoRender_SW);
            LOGD("(f:%s, l:%d), delay sometime to wait hw", __FUNCTION__, __LINE__);
            //usleep(50*1000);
		}
#endif
        //LOGD("(f:%s, l:%d) mDisplayFormat=[0x%x], frm_inf->pixel_format=[0x%x]", __FUNCTION__, __LINE__, mDisplayFormat, frm_inf->pixel_format);
		{
            ret = mVideoRenderer->dequeueFrame(pANativeWindowBuffer);
			if(mFirstFrame) {
			#if (CEDARX_ANDROID_VERSION >= 8)
				//add by weihongqiang.
				notifyListener_l(MEDIA_INFO, MEDIA_INFO_RENDERING_START);
			#endif
				mFirstFrame = 0;
			}
            //LOGV("render frame id:%d",frame_id);
            //mLocalRenderFrameIDCurr = frame_id;
		}
	}
	else
	{
		//if (mDisplayFormat == HAL_PIXEL_FORMAT_YV12)
		//{
		//	mLocalRenderFrameIDCurr = frame_id;
		//}
        LOGD("(f:%s, l:%d) mVideoRenderer=NULL, mNativeWindow=[%d]", __FUNCTION__, __LINE__, mVideoLayerId);
		//if (mNativeWindow != NULL)
		if (mVideoLayerId >= 0)
		{
            int nGUIInitRotation;
            nGUIInitRotation  = (mInitRotation + mMediaInfo.mVideoInfo[0].mFileSelfRotation)%4;
            //LOGD("(f:%s, l:%d) mInitRotation=%d, mFileSelfRotation=%d, nGUIInitRotation=%d", __FUNCTION__, __LINE__, mInitRotation, mMediaInfo.mVideoInfo[0].mFileSelfRotation, nGUIInitRotation);
            if(mMediaInfo.mVideoInfo[0].mFileSelfRotation!=0)
            {
                LOGD("(f:%s, l:%d) Be careful. mFileSelfRotation[%d]", __FUNCTION__, __LINE__, mMediaInfo.mVideoInfo[0].mFileSelfRotation);
            }
			sp<MetaData> meta = new MetaData;
			//meta->setInt32(kKeyScreenID, mScreenID);
		    meta->setInt32(kKeyColorFormat, mDisplayFormat);
		    meta->setInt32(kKeyWidth, mDisplayWidth);
		    meta->setInt32(kKeyHeight, mDisplayHeight);
            meta->setInt32(kKeyRotation, nGUIInitRotation);
            meta->setInt32(kKeyIsDRM, mIsDRMMedia);
		    LOGD("reinit mVideoRenderer, mDisplayFormat=[0x%x]", mDisplayFormat);
		    // Must ensure that mVideoRenderer's destructor is actually executed
		    // before creating a new one.
		    IPCThreadState::self()->flushCommands();

		    if(mDisplayFormat != VIRTUAL_HWC_FORMAT_YUV420PLANAR)//HWC_FORMAT_YUV420PLANAR, HAL_PIXEL_FORMAT_YV12)
		    {
//				mRenderToDE = 1;
//	    		mFirstFrame = 1;
//		    	mVideoRenderer = new CedarXDirectHwRenderer(mVideoLayerId, meta);
//		    	set3DMode(_3d_mode, display_3d_mode);
                ALOGE("(f:%s, l:%d) fatal error! when dequeueFrame, must yv12", __FUNCTION__, __LINE__);
		    }
		    else
		    {
                mRenderToDE = 0;
		        mVideoRenderer = new CedarXLocalRenderer(mVideoLayerId, meta);
				//set3DMode(_3d_mode, display_3d_mode);
				set3DMode(_3d_mode);
		    }
            setVideoScalingMode_l(mVideoScalingMode);
		    goto _render_again;
		}
        else
        {
            LOGV("(f:%s, l:%d) mNativeWindow=NULL", __FUNCTION__, __LINE__);
            ret = 10;
        }
	}
    return ret;
}

int CedarXPlayer::StagefrightVideoRenderEnqueueFrame(ANativeWindowBufferCedarXWrapper *pANativeWindowBuffer)
{
    int ret;
    if(mVideoRenderer == NULL)
    {
        LOGW("(f:%s, l:%d) Be careful! mVideoRenderer==NULL", __FUNCTION__, __LINE__);
        return 10;  //10 indicate NativeWindow is NULL, consider enqueue success.
    }
    ret = mVideoRenderer->enqueueFrame(pANativeWindowBuffer);
    return ret;
}

#endif


// Parse the SRT text sample, and store the timing and text sample in a Parcel.
// The Parcel will be sent to MediaPlayer.java through event, and will be
// parsed in TimedText.java.
int extractTextLocalDescriptions(CedarXTimedText *pCedarXTimedText, Parcel *parcel)
{
    // In the absence of any bits set in flags, the text
    // is plain. Otherwise, 1: bold, 2: italic, 4: underline
    int nStyleFlags = 0;
    parcel->writeInt32(KEY_LOCAL_SETTING);
    parcel->writeInt32(KEY_START_TIME);
    parcel->writeInt32(pCedarXTimedText->startTime);

    parcel->writeInt32(KEY_STRUCT_TEXT);
    // write the size of the text sample
    parcel->writeInt32(pCedarXTimedText->subTextLen);
    // write the text sample as a byte array
    parcel->writeInt32(pCedarXTimedText->subTextLen);
    parcel->write(pCedarXTimedText->subTextBuf, pCedarXTimedText->subTextLen);


    if(pCedarXTimedText->subDispPos != SUB_DISPPOS_DEFAULT)
    {
        parcel->writeInt32(KEY_STRUCT_AWEXTEND_SUBDISPPOS);
        //write text subtitle's position area.
        parcel->writeInt32(pCedarXTimedText->subDispPos);
        
        parcel->writeInt32(KEY_STRUCT_AWEXTEND_SCREENRECT);
        //write text subtitle's whole screen rect.
        parcel->writeInt32(0);
        parcel->writeInt32(0);
        parcel->writeInt32(pCedarXTimedText->nScreenHeight);
        parcel->writeInt32(pCedarXTimedText->nScreenWidth);

        parcel->writeInt32(KEY_STRUCT_TEXT_POS);
        //write text subtitle's position Rect.
        parcel->writeInt32(pCedarXTimedText->starty);
        parcel->writeInt32(pCedarXTimedText->startx);
        parcel->writeInt32(pCedarXTimedText->endy);
        parcel->writeInt32(pCedarXTimedText->endx);
    }
    
    if(pCedarXTimedText->subHasFontInfFlag)
    {
        parcel->writeInt32(KEY_STRUCT_STYLE_LIST);
        parcel->writeInt32(KEY_FONT_ID);
        parcel->writeInt32((int)pCedarXTimedText->fontStyle);
        parcel->writeInt32(KEY_FONT_SIZE);
        parcel->writeInt32((int)pCedarXTimedText->fontSize);
        parcel->writeInt32(KEY_TEXT_COLOR_RGBA);
        parcel->writeInt32((int)pCedarXTimedText->primaryColor);
        parcel->writeInt32(KEY_STYLE_FLAGS);
        nStyleFlags = (pCedarXTimedText->subStyle&0x7);
        parcel->writeInt32(nStyleFlags);
    }

    if(pCedarXTimedText->getSubShowFlag() == 0) //SUB_SHOW_INVALID
    {
        parcel->writeInt32(KEY_STRUCT_AWEXTEND_HIDESUB);
        //nofity app to hide this subtitle
        parcel->writeInt32(1);
    }
    return OK;
}

/*******************************************************************************
Function name: android.extractBmpLocalDescriptions
Description: 
    we extend android subtitle interface for bmp subtitle such as idxsub and pgs.
Parameters: 
    
Return: 
    
Time: 2013/6/18
*******************************************************************************/
int extractBmpLocalDescriptions(CedarXTimedText *pCedarXTimedText, Parcel *parcel)
{
    int bufsize;
    parcel->writeInt32(KEY_LOCAL_SETTING);
    parcel->writeInt32(KEY_START_TIME);
    parcel->writeInt32(pCedarXTimedText->startTime);

    parcel->writeInt32(KEY_STRUCT_AWEXTEND_BMP);
    // write the pixel format of bmp subtitle
    parcel->writeInt32(KEY_STRUCT_AWEXTEND_PIXEL_FORMAT);
    parcel->writeInt32(pCedarXTimedText->mSubPicPixelFormat);
    // write the width of bmp subtitle
    parcel->writeInt32(KEY_STRUCT_AWEXTEND_PICWIDTH);
    parcel->writeInt32(pCedarXTimedText->subPicWidth);
    // write the height of bmp subtitle
    parcel->writeInt32(KEY_STRUCT_AWEXTEND_PICHEIGHT);
    parcel->writeInt32(pCedarXTimedText->subPicHeight);
    // write the size of the text sample
    bufsize = pCedarXTimedText->subPicWidth*pCedarXTimedText->subPicHeight*4;   //we know format is PIXEL_FORMAT_RGBA_8888, so 4 byte every pixel.
    if(bufsize > SUB_FRAME_BUFSIZE)
    {
        LOGW("(f:%s, l:%d) fatal error! bufsize[%d], SUB_FRAME_BUFSIZE[%d]", __FUNCTION__, __LINE__, bufsize, SUB_FRAME_BUFSIZE);
        bufsize = SUB_FRAME_BUFSIZE;
    }
    parcel->writeInt32(bufsize);
    // write the argb buffer as an int array
    parcel->writeInt32(bufsize/4);
    //LOGD("(f:%s, l:%d) bmp_length_int=%d", __FUNCTION__, __LINE__, bufsize/4);
    parcel->write(pCedarXTimedText->subBitmapBuf, bufsize);

    return OK;
}

int CedarXPlayer::StagefrightAudioRenderInit(int samplerate, int channels, int format)
{
	//initial audio playback
	if (mAudioPlayer == NULL) {
		if (mAudioSink != NULL) {
			mAudioPlayer = new CedarXAudioPlayer(mAudioSink, this);
			//mAudioPlayer->setSource(mAudioSource);
			LOGV("set audio format: (%d : %d)", samplerate, channels);
			mAudioPlayer->setFormat(samplerate, channels);

			status_t err = mAudioPlayer->start(true /* sourceAlreadyStarted */);

			if (err != OK) {
				delete mAudioPlayer;
				mAudioPlayer = NULL;

				//mFlags &= ~(PLAYING);

				return err;
			}
			//Set whether suppress audio data, since audioPlayer
			//may have not initialized when calling setHDMIState at start.
			mAudioPlayer->setSuppressData(mHDMIPlugged && mIsDRMMedia);
			mWaitAudioPlayback = 1;
		}
	} else {
		mAudioPlayer->resume();
	}

	return 0;
}

void CedarXPlayer::StagefrightAudioRenderExit(int immed)
{
	if(mAudioPlayer){
		delete mAudioPlayer;
		mAudioPlayer = NULL;
	}
}

int CedarXPlayer::StagefrightAudioRenderData(void* data, int len)
{
	if(mAudioPlayer == NULL)
		return 0;

	return mAudioPlayer->render(data,len);
}

int CedarXPlayer::StagefrightAudioRenderGetSpace(void)
{
	if(mAudioPlayer == NULL)
		return 0;

	return mAudioPlayer->getSpace();
}

int CedarXPlayer::StagefrightAudioRenderGetDelay(void)
{
	if(mAudioPlayer == NULL)
		return 0;

	return mAudioPlayer->getLatency();
}

int CedarXPlayer::StagefrightAudioRenderFlushCache(void)
{
	if(mAudioPlayer == NULL)
		return 0;

	return mAudioPlayer->seekTo(0);
}

int CedarXPlayer::StagefrightAudioRenderPause(void)
{
	if(mAudioPlayer == NULL)
		return 0;

	mAudioPlayer->pause();
	return 0;
}


int CedarXPlayer::getMaxCacheSize()
{
	int arg[5];
	mPlayer->control(mPlayer, CDX_CMD_GET_CACHE_PARAMS, (unsigned int)arg, 0);
	return arg[0];
}

int CedarXPlayer::getMinCacheSize()
{
	int arg[5];
	mPlayer->control(mPlayer, CDX_CMD_GET_CACHE_PARAMS, (unsigned int)arg, 0);
	return arg[2];
}

int CedarXPlayer::getStartPlayCacheSize()
{
	int arg[5];
	mPlayer->control(mPlayer, CDX_CMD_GET_CACHE_PARAMS, (unsigned int)arg, 0);
	return arg[1];
}


int CedarXPlayer::getCachedDataSize()
{
	int cachedDataSize;
	mPlayer->control(mPlayer, CDX_CMD_GET_CURRETN_CACHE_SIZE, (unsigned int)&cachedDataSize, 0);
	return cachedDataSize;
}


int CedarXPlayer::getCachedDataDuration()
{
	int bitrate;
	int cachedDataSize;
	float tmp;

	mPlayer->control(mPlayer, CDX_CMD_GET_CURRENT_BITRATE, (unsigned int)&bitrate, 0);
	mPlayer->control(mPlayer, CDX_CMD_GET_CURRETN_CACHE_SIZE, (unsigned int)&cachedDataSize, 0);

	if(bitrate <= 0)
		return 0;
	else
	{
		tmp = (float)cachedDataSize*8.0*1000.0;
		tmp /= (float)bitrate;
		return (int)tmp;
	}
}


int CedarXPlayer::getStreamBitrate()
{
	int bitrate;
	mPlayer->control(mPlayer, CDX_CMD_GET_CURRENT_BITRATE, (unsigned int)&bitrate, 0);
	return bitrate;
}


int CedarXPlayer::getCacheSize(int *nCacheSize)
{
	*nCacheSize = getCachedDataSize();
	if(*nCacheSize > 0)
	{
		return *nCacheSize;
	}
	return 0;
}

int CedarXPlayer::getCacheDuration(int *nCacheDuration)
{
	*nCacheDuration = getCachedDataDuration();
	if(*nCacheDuration > 0)
	{
		return *nCacheDuration;
	}
	return 0;
}


bool CedarXPlayer::setCacheSize(int nMinCacheSize,int nStartCacheSize,int nMaxCacheSize)
{
	return setCacheParams(nMaxCacheSize, nStartCacheSize, nMinCacheSize, 0, 0);
}

bool CedarXPlayer::setCacheParams(int nMaxCacheSize, int nStartPlaySize, int nMinCacheSize, int nCacheTime, bool bUseDefaultPolicy)
{
	int arg[5];
	arg[0] = nMaxCacheSize;
	arg[1] = nStartPlaySize;
	arg[2] = nMinCacheSize;
	arg[3] = nCacheTime;
	arg[4] = bUseDefaultPolicy;

	if(mPlayer->control(mPlayer, CDX_CMD_SET_CACHE_PARAMS, (unsigned int)arg, 0) == 0)
		return true;
	else
		return false;
}


void CedarXPlayer::getCacheParams(int* pMaxCacheSize, int* pStartPlaySize, int* pMinCacheSize, int* pCacheTime, bool* pUseDefaultPolicy)
{
	int arg[5];
	mPlayer->control(mPlayer, CDX_CMD_GET_CACHE_PARAMS, (unsigned int)arg, 0);
	*pMaxCacheSize     = arg[0];
	*pStartPlaySize    = arg[1];
	*pMinCacheSize     = arg[2];
	*pCacheTime        = arg[3];
	*pUseDefaultPolicy = arg[4];
	return;
}

void CedarXPlayer::setHDMIState(bool state) {
	LOGV("setHDMIState");
	mHDMIPlugged = state;
	if(mAudioPlayer) {
		mAudioPlayer->setSuppressData(mHDMIPlugged && mIsDRMMedia);
	}
}

//static
void CedarXPlayer::HDMINotify(void* cookie, bool state)
{
	LOGV("HDMINotify");
	CedarXPlayer * player = static_cast<CedarXPlayer *>(cookie);
	if(player) {
		player->setHDMIState(state);
	}
}

int CedarXPlayer::CedarXPlayerCallback(int event, void *info)
{
	int ret = 0;
	int *para = (int*)info;

	//LOGV("----------CedarXPlayerCallback event:%d info:%p\n", event, info);

	switch (event) {
	case CDX_EVENT_PLAYBACK_COMPLETE:
        ALOGV("(f:%s, l:%d) receive CDX_EVENT_PLAYBACK_COMPLETE, mFlags[0x%x]", __FUNCTION__, __LINE__, mFlags);
		mFlags &= ~PLAYING;
		mFlags |= AT_EOS;
		if (mAudioPlayer != NULL) {
			mAudioPlayer->setPlaybackEos(true);
		}
		stop_l(); //for gallery
		break;

    case CDX_EVENT_VDEC_OUT_OF_MEMORY:
        LOGD("vdec_out_of_memory, cedarxPlayer know it and notify to upper!");
        notifyListener_l(MEDIA_ERROR, MEDIA_ERROR_OUT_OF_MEMORY);
        //notifyListener_l(MEDIA_ERROR, 900);
        mFlags &= ~PLAYING;
		mFlags |= AT_EOS;
		stop_l(); //for gallery
		break;

	case CDX_EVENT_VIDEORENDERINIT:
	    StagefrightVideoRenderInit(para[0], para[1], para[2], para[3]);
		break;

	case CDX_EVENT_VIDEORENDERDATA:
		StagefrightVideoRenderData((void*)para[0],para[1]);
		break;

	case CDX_EVENT_VIDEORENDEREXIT:
		StagefrightVideoRenderExit();
		break;

	case CDX_EVENT_VIDEORENDERGETDISPID:
		*((int*)para) = StagefrightVideoRenderGetFrameID();
		break;
    case CDX_EVENT_VIDEORENDER_DEQUEUEFRAME:
        ret = StagefrightVideoRenderDequeueFrame((void*)para[0], para[1], (ANativeWindowBufferCedarXWrapper*)para[2]);
        break;
    case CDX_EVENT_VIDEORENDER_ENQUEUEFRAME:
        ret = StagefrightVideoRenderEnqueueFrame((ANativeWindowBufferCedarXWrapper*)para);
        break;
	case CDX_EVENT_AUDIORENDERINIT:
		StagefrightAudioRenderInit(para[0], para[1], para[2]);
		break;

	case CDX_EVENT_AUDIORENDEREXIT:
		StagefrightAudioRenderExit(0);
		break;

	case CDX_EVENT_AUDIORENDERDATA:
		ret = StagefrightAudioRenderData((void*)para[0],para[1]);
		break;

	case CDX_EVENT_AUDIORENDERPAUSE:
		ret = StagefrightAudioRenderPause();
		break;

	case CDX_EVENT_AUDIORENDERGETSPACE:
		ret = StagefrightAudioRenderGetSpace();
		break;

	case CDX_EVENT_AUDIORENDERGETDELAY:
		ret = StagefrightAudioRenderGetDelay();
		break;

	case CDX_EVENT_AUDIORENDERFLUSHCACHE:
		ret = StagefrightAudioRenderFlushCache();
		break;

	case CDX_MEDIA_INFO_BUFFERING_START:
		LOGV("MEDIA_INFO_BUFFERING_START");
		notifyListener_l(MEDIA_INFO, MEDIA_INFO_BUFFERING_START);
		break;

	case CDX_MEDIA_INFO_BUFFERING_END:
		LOGV("MEDIA_INFO_BUFFERING_END ...");
		notifyListener_l(MEDIA_INFO, MEDIA_INFO_BUFFERING_END);
		break;

	case CDX_MEDIA_BUFFERING_UPDATE:
	{
		LOGV("buffering percentage %d", (int)para);
		notifyListener_l(MEDIA_BUFFERING_UPDATE, (int)para);
		break;
	}

	case CDX_MEDIA_WHOLE_BUFFERING_UPDATE:
		notifyListener_l(MEDIA_BUFFERING_UPDATE, (int)para);
		break;

	case CDX_EVENT_FATAL_ERROR:
        notifyListener_l(MEDIA_ERROR, MEDIA_ERROR_UNKNOWN, (int)para);
		break;

	case CDX_EVENT_PREPARED:
		finishAsyncPrepare_l((int)para);
		break;

	case CDX_EVENT_SEEK_COMPLETE:
		finishSeek_l(0);
		break;

	case CDX_EVENT_NATIVE_SUSPEND:
		LOGV("receive CDX_EVENT_NATIVE_SUSPEND");
		ret = nativeSuspend();
		break;
		
    case CDX_EVENT_NETWORK_STREAM_INFO:
        notifyListener_l(MEDIA_INFO, MEDIA_INFO_METADATA_UPDATE, (int)para);
		break;
	case CDA_EVENT_AUDIORAWPLAY:
    {
        int64_t token = IPCThreadState::self()->clearCallingIdentity();
	
    	static int raw_data_test = 0;
    	String8 raw1 = String8("raw_data_output=1");
    	String8 raw0 = String8("raw_data_output=0");
    	const sp<IAudioFlinger>& af = AudioSystem::get_audio_flinger();
		LOGV("***line:%d,func:%s",__LINE__,__FUNCTION__);
        if (af == 0) 
        {
        	LOGE("[star]............ PERMISSION_DENIED");
        }
        else
        {
    	
        	if (para[0])
        	{
        		LOGV("[star]............ to set raw data output");
            	af->setParameters(0, raw1);
        	}
        	else
        	{
        		LOGV("[star]............ to set not raw data output");
            	af->setParameters(0, raw0);
        	}
        }
    	
        IPCThreadState::self()->restoreCallingIdentity(token);
    }
        break;

	case CDX_EVENT_IS_ENCRYPTED_MEDIA:
		if(ENABLE_HDCP) {
			mIsDRMMedia = !!(*para);
		}
		LOGV("is encrypted media %d", mIsDRMMedia);
		break;

#if (CEDARX_ANDROID_VERSION >= 7)
	case CDX_EVENT_TIMED_TEXT:
	{
		if (mListener == NULL)
        {
            LOGE("fatal error! why mListener==NULL!");
            ret = 0;
            break;
		}
        sp<MediaPlayerBase> listener = mListener.promote();
        if (listener == NULL) 
        {
            LOGE("%s: fatal error! Listener is NULL.", __FUNCTION__);
            ret = 0;
            break;
        }

        CedarXTimedText *pCedarXTimedText = (CedarXTimedText*)para;
        if(pCedarXTimedText)    //show or hide
        {
            Parcel parcel;
            if (pCedarXTimedText->subMode == 0) //SUB_MODE_TEXT
            {
//                LOGD("eric_sub:show subtitle:subMode[%d], startx[%d], starty[%d], endx[%d], endy[%d], subDispPos[0x%x],\
//                \n startTime[%d], endTime[%d], subTextLen[%d], encodingType[%d],\
//                \n subHasFontInfFlag[%d], fontStyle[%d], fontSize[%d], primaryColor[0x%x], secondaryColor[0x%x], subStyle[%d]\
//                \n mSubShowFlag[%d]",
//                  pCedarXTimedText->subMode, pCedarXTimedText->startx, pCedarXTimedText->starty, pCedarXTimedText->endx, pCedarXTimedText->endy, pCedarXTimedText->subDispPos, 
//                  pCedarXTimedText->startTime, pCedarXTimedText->endTime, pCedarXTimedText->subTextLen, pCedarXTimedText->encodingType,
//                  pCedarXTimedText->subHasFontInfFlag, pCedarXTimedText->fontStyle, pCedarXTimedText->fontSize, pCedarXTimedText->primaryColor, pCedarXTimedText->secondaryColor, pCedarXTimedText->subStyle,
//                  pCedarXTimedText->getSubShowFlag()); 
//                LOGD("eric_sub:subTextBuf:[%s]", pCedarXTimedText->subTextBuf);
                extractTextLocalDescriptions(pCedarXTimedText, &parcel);
                listener->sendEvent(MEDIA_TIMED_TEXT, 0, 0, &parcel);
                ret = 0;
            }
            else if (pCedarXTimedText->subMode == 1)    //SUB_MODE_BITMAP
            {
//                LOGD("eric_sub:SUB_MODE_BITMAP, subMode[%d], startTime[%d], endTime[%d], subBitmapBuf[%p], subPicWidth[%d], subPicHeight[%d], encodingType[%d]",
//                    pCedarXTimedText->subMode, pCedarXTimedText->startTime, pCedarXTimedText->endTime, 
//                    pCedarXTimedText->subBitmapBuf, pCedarXTimedText->subPicWidth, pCedarXTimedText->subPicHeight, pCedarXTimedText->encodingType); 
                extractBmpLocalDescriptions(pCedarXTimedText, &parcel);
                listener->sendEvent(MEDIA_TIMED_TEXT, 0, 0, &parcel);
                ret = 0;
            }
            else
            {
                LOGW("fatal error! subMode=%d, check code!", pCedarXTimedText->subMode);
                ret = 0;
            }
        }
        else    //hide all
        {
           //LOGD("eric_sub:CDX_EVENT_TIMED_TEXT_FOR_PLAYER hide subtitle"); 
           listener->sendEvent(MEDIA_TIMED_TEXT);
           ret = 0;
        }
        break;
    }
#endif
//	case CDX_MEDIA_INFO_SRC_3D_MODE:
//		{
//			cdx_3d_mode_e tmp_3d_mode;
//			tmp_3d_mode = *((cdx_3d_mode_e *)info);
//			LOGV("source 3d mode get from parser is %d", tmp_3d_mode);
//			notifyListener_l(MEDIA_INFO_SRC_3D_MODE, tmp_3d_mode);
//		}
//		break;

	default:
		break;
	}

	return ret;
}

extern "C" int CedarXPlayerCallbackWrapper(void *cookie, int event, void *info) //cookie:CedarXPlayer*;
{
	return ((android::CedarXPlayer *)cookie)->CedarXPlayerCallback(event, info);
}

} // namespace android
