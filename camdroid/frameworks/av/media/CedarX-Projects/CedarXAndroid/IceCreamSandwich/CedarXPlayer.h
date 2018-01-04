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

#ifndef CEDARX_PLAYER_H_

#define CEDARX_PLAYER_H_

#include <media/MediaPlayerInterface.h>
#include <media/stagefright/DataSource.h>
//#include <media/stagefright/OMXClient.h>
#include <media/stagefright/TimeSource.h>
#include <utils/threads.h>
//#include <hardware/hwcomposer.h>
//#include "CedarXNativeRenderer.h"

#include <media/stagefright/MediaBuffer.h>
//#include <media/stagefright/MediaClock.h>
#include <media/mediaplayerinfo.h>

#include <CDX_PlayerAPI.h>

#include <include_sft/NuPlayerSource.h>

//#include <HDMIListerner.h>

#include "CedarXAudioPlayer.h"

namespace android {

struct CedarXAudioPlayer;
struct DataSource;
struct MediaBuffer;
struct MediaExtractor;
struct MediaSource;
struct NuCachedSource2;
struct ISurfaceTexture;

struct ALooper;
struct AwesomePlayer;
struct CedarXPlayerAdapter;

struct CedarXRenderer : public RefBase {
    CedarXRenderer() {}

    virtual void render(const void *data, size_t size) = 0;
    //virtual void render(MediaBuffer *buffer) = 0;
    virtual int control(int cmd, int para) = 0;
    virtual int dequeueFrame(ANativeWindowBufferCedarXWrapper *pObject) = 0;
    virtual int enqueueFrame(ANativeWindowBufferCedarXWrapper *pObject) = 0;
    
private:
    CedarXRenderer(const CedarXRenderer &);
    CedarXRenderer &operator=(const CedarXRenderer &);
};

typedef struct tag_VirtualVideo3DInfo
{
	unsigned int        width;
	unsigned int        height;
	//unsigned int        format; // HWC_FORMAT_MBYUV422(hwcomposer.h) or HAL_PIXEL_FORMAT_YV12(graphics.h).
	int  format; // VirtualHWCRenderFormat, virtual to HWC_FORMAT_MBYUV422(hwcomposer.h) or HAL_PIXEL_FORMAT_YV12(graphics.h).
	int     src_mode;   //VirtualHWC3DSrcMode, virtual to HWC_3D_SRC_MODE_TB
	int   display_mode;   //VirtualHWCDisplayMode, virtual to HWC_3D_OUT_MODE_ANAGLAGH(hwcomposer.h)
}VirtualVideo3DInfo;    //virtual to video3Dinfo_t

//for setParameter()
typedef enum tag_KeyofSetParameter{
    PARAM_KEY_ENCRYPT_FILE_TYPE_DISABLE          = 0x00,
    PARAM_KEY_ENCRYPT_ENTIRE_FILE_TYPE           = 0x01,
    PARAM_KEY_ENCRYPT_PART_FILE_TYPE             = 0x02,
    PARAM_KEY_ENABLE_BOOTANIMATION               = 100,

    //add by weihongqiang for IPTV.
    PARAM_KEY_SET_AV_SYNC			             = 0x100,
    PARAM_KEY_ENABLE_KEEP_FRAME				     = 0x101,
    PARAM_KEY_CLEAR_BUFFER				     	 = 0x102,
    //audio channel.
    PARAM_KEY_SWITCH_CHANNEL		     	 	 = 0x103,
    //for IPTV end.
    PARAM_KEY_,
}KeyofSetParameter;

enum {
    PLAYING             = 0x01,
    LOOPING             = 0x02,
    FIRST_FRAME         = 0x04,
    PREPARING           = 0x08,
    PREPARED            = 0x10,
    AT_EOS              = 0x20,
    PREPARE_CANCELLED   = 0x40,
    CACHE_UNDERRUN      = 0x80,
    AUDIO_AT_EOS        = 0x100,
    VIDEO_AT_EOS        = 0x200,
    AUTO_LOOPING        = 0x400,
    WAIT_TO_PLAY        = 0x800,
    WAIT_VO_EXIT        = 0x1000,
    CEDARX_LIB_INIT     = 0x2000,
    SUSPENDING          = 0x4000,
    PAUSING				= 0x8000,
    RESTORE_CONTROL_PARA= 0x10000,
    NATIVE_SUSPENDING   = 0x20000,
};

enum {
	SOURCETYPE_URL = 0,
	SOURCETYPE_FD ,
	SOURCETYPE_SFT_STREAM
};

typedef struct CedarXPlayerExtendMember_{
	int64_t mLastGetPositionTimeUs;
	int64_t mLastPositionUs;
	//int32_t mOutputSetting;
	int32_t mUseHardwareLayer;
	int32_t mPlaybackNotifySend;

    //for thirdpart_fread of encrypt video file
    int32_t     encrypt_type;         // 0: disable; 1: encrypt entire file; 2: encrypt part file;
    uint32_t    encrypt_file_format;  //

    //for Bootanimation;
    uint32_t    bootanimation_enable;
}CedarXPlayerExtendMember;

struct CedarXPlayer { //don't touch this struct any more, you can extend members in CedarXPlayerExtendMember
    CedarXPlayer();
    ~CedarXPlayer();

    void setListener(const wp<MediaPlayerBase> &listener);
    void setUID(uid_t uid);

    status_t setDataSource(
            const char *uri,
            const KeyedVector<String8, String8> *headers = NULL);

    status_t setDataSource(int fd, int64_t offset, int64_t length);
    status_t setDataSource(const sp<IStreamSource> &source);

    void reset();

    status_t prepare();
    status_t prepare_l();
    status_t prepareAsync();
    status_t prepareAsync_l();

    status_t play();
    status_t pause();
    status_t stop_l();
    status_t stop();

    bool isPlaying() const;

    status_t setSurface(const int hlay);
    status_t setSurfaceTexture(const int hlay);
    void setAudioSink(const sp<MediaPlayerBase::AudioSink> &audioSink);
    status_t setLooping(bool shouldLoop);

    status_t getDuration(int64_t *durationUs);
    status_t getPosition(int64_t *positionUs);

    status_t setParameter(int key, const Parcel &request);
    status_t getParameter(int key, Parcel *reply);
    status_t setCacheStatCollectFreq(const Parcel &request);
    status_t seekTo(int64_t timeUs);

    status_t getVideoDimensions(int32_t *width, int32_t *height) const;

    status_t suspend();
    status_t resume();
    int setVps(int vpsspeed);    //set play speed,  aux = -40~100,=0-normal; <0-slow; >0-fast, so speed scope: (100-40)% ~ (100+100)%, ret = OK
//#ifndef __CHIP_VERSION_F20
//    status_t setScreen(int screen);
    status_t set3DMode(int source3dMode);
    //status_t set3DMode(int rotate_flag);
    int    	 getMeidaPlayerState();
    status_t setSubCharset(const char *charset);
    status_t getSubCharset(char *charset);
    status_t setSubDelay(int time);
    int      getSubDelay();

    status_t enableScaleMode(bool enable, int width, int height);
//    status_t setVppGate(bool enableVpp);
//    status_t setLumaSharp(int value);
//    status_t setChromaSharp(int value);
//    status_t setWhiteExtend(int value);
//    status_t setBlackExtend(int value);
    status_t setChannelMuteMode(int muteMode);
    int getChannelMuteMode();
//    status_t extensionControl(int command, int para0, int para1);
    status_t generalInterface(int cmd, int int1, int int2, int int3, void *p);
//#endif
    // This is a mask of MediaExtractor::Flags.
    uint32_t flags() const;

    void postAudioEOS();
    void postAudioSeekComplete();
#if (CEDARX_ANDROID_VERSION >= 7)
    //added by weihongqiang.
    status_t invoke(const Parcel &request, Parcel *reply);
#endif

    int CedarXPlayerCallback(int event, void *info);


    int getMaxCacheSize();
    int getMinCacheSize();
    int getStartPlayCacheSize();
    int getCachedDataSize();
    int getCachedDataDuration();
    int getStreamBitrate();

    // add by yaosen 2013-1-22
	int getCacheSize(int *nCacheSize);
	int getCacheDuration(int *nCacheDuration);
    bool setCacheSize(int nMinCacheSize,int nStartCacheSize,int nMaxCacheSize);
    bool setCacheParams(int nMaxCacheSize, int nStartPlaySize, int nMinCacheSize, int nCacheTime, bool bUseDefaultPolicy);
    void getCacheParams(int* pMaxCacheSize, int* pStartPlaySize, int* pMinCacheSize, int* pCacheTime, bool* pUseDefaultPolicy);

private:
    friend struct CedarXEvent;

    //* for cache policy of network stream playing.
    int mMaxCacheSize;
    int mStartPlayCacheSize;
    int mMinCacheSize;
    int mCacheTime;
    int mUseDefautlCachePolicy;

    mutable Mutex mLock;
    mutable Mutex mLockNativeWindow;
    Mutex mMiscStateLock;

    CDXPlayer *mPlayer;
    //AwesomePlayer *mAwesomePlayer;
    int mStreamType;

    bool mQueueStarted;
    wp<MediaPlayerBase> mListener;
    bool mUIDValid;
    uid_t mUID;

    //sp<Surface> mSurface;
    //sp<ANativeWindow> mNativeWindow;
    int mVideoLayerId;
    sp<MediaPlayerBase::AudioSink> mAudioSink;

    String8 mUri;
    int mSourceType;    //SOURCETYPE_URL
    KeyedVector<String8, String8> mUriHeaders;

    sp<CedarXRenderer> mVideoRenderer;
    bool mVideoRendererIsPreview;

    sp<Source> mSftSource;

    CedarXMediaInformations mMediaInfo;
    CedarXAudioPlayer *mAudioPlayer;
    int64_t mDurationUs;

    uint32_t mFlags;
    uint32_t mExtractorFlags;
    bool isCedarXInitialized;
    int32_t mDisableXXXX;

    uint32_t	_3d_mode;           //cedarv_3d_mode_e
    //uint32_t	pre_3d_mode;		//* for source 3d mode changing when displaying at anaglagh mode.
    //uint32_t	display_3d_mode;    //cedarx_display_3d_mode_e
    //uint32_t	display_type_tmp_save;
    //uint32_t    anaglagh_type;
    //uint32_t	anaglagh_en;
    //uint32_t	wait_anaglagh_display_change;

    int32_t mVideoWidth, mVideoHeight, mFirstFrame; //mVideoWidth:the video width told to app
    int32_t mCanSeek;
    int32_t mDisplayWidth, mDisplayHeight, mDisplayFormat;  //mDisplayWidth=vdeclib_display_width, mDisplayHeight=vdeclib_display_height,so when vdeclib rotate, we should careful! 
                                                            //mDisplayFormat:destination format to send out. VirtualHWCRenderFormat(not HWC_FORMAT_MBYUV422 or HAL_PIXEL_FORMAT_YV12)
    int32_t mRenderToDE;
    //int32_t mLocalRenderFrameIDCurr;
    int64_t mTimeSourceDeltaUs;
    int64_t mVideoTimeUs;


//    video3Dinfo_t mVideo3dInfo;//mickey
    VirtualVideo3DInfo mVideo3dInfo;
    int  mTagPlay; //0: none 1:first TagePlay 2: Seeding TagPlay
    bool mSeeking;
    bool mSeekNotificationSent;
    int64_t mSeekTimeUs;    //unit:us
    int64_t mLastValidPosition;

    int64_t mBitrate;  // total bitrate of the file (in bps) or -1 if unknown.

    bool mIsAsyncPrepare;
    status_t mPrepareResult;
    status_t mStreamDoneStatus;

    int     mPlayerState;

    void postVideoEvent_l(int64_t delayUs = -1);
    void postBufferingEvent_l();
    void postStreamDoneEvent_l(status_t status);
    void postCheckAudioStatusEvent_l();
    status_t play_l(int command);

    bool mWaitAudioPlayback;

    sp<ALooper> mLooper;

    int64_t mSuspensionPositionUs;

    struct SuspensionState {
        String8 mUri;
        KeyedVector<String8, String8> mUriHeaders;
        sp<DataSource> mFileSource;

        uint32_t mFlags;
        int64_t mPositionUs;
    } mSuspensionState;

    /*user defined parameters*/
//    int32_t mScreenID;
//    int32_t mVppGate;
//    int32_t mLumaSharp;
//    int32_t mChromaSharp;
//    int32_t mWhiteExtend;
//    int32_t mBlackExtend;
    uint8_t mAudioTrackIndex;
    uint8_t mSubTrackIndex;
    int32_t mMaxOutputWidth;
    int32_t mMaxOutputHeight;

    int32_t mVideoScalingMode;

    struct SubtitleParameter {
//		int32_t mSubtitleFontSize;
//		int32_t mSubtitleColor;
//		int32_t mSubtitleFrameColor;
		String8 mSubtitleCharset;
		int32_t mSubtitleGate;
		int32_t mSubtitleDelay;
		int32_t mSubtitleIndex;
//		int32_t mSubtitlePosition;
    }mSubtitleParameter;

    CedarXPlayerExtendMember *mExtendMember;

//    status_t setDataSource_l(
//            const char *uri,
//            const KeyedVector<String8, String8> *headers = NULL);
//
//    status_t setDataSource_l(const sp<DataSource> &dataSource);
//    status_t setDataSource_l(const sp<MediaExtractor> &extractor);
    void reset_l();
    void partial_reset_l();
    status_t seekTo_l(int64_t timeUs);
    status_t pause_l(bool at_eos = false);
    void initRenderer_l();
    void seekAudioIfNecessary_l();

    void cancelPlayerEvents(bool keepBufferingGoing = false);

    void setAudioSource(sp<MediaSource> source);
    status_t initAudioDecoder();

    void setVideoSource(sp<MediaSource> source);
    status_t initVideoDecoder(uint32_t flags = 0);

    void onStreamDone();

    void notifyListener_l(int msg, int ext1 = 0, int ext2 = 0);

    void onBufferingUpdate();
    void onCheckAudioStatus();
    void onPrepareAsyncEvent();
    void abortPrepare(status_t err);
    void finishAsyncPrepare_l(int err);
    void finishSeek_l(int err);
    bool getCachedDuration_l(int64_t *durationUs, bool *eos);

    status_t finishSetDataSource_l();

    static bool ContinuePreparation(void *cookie);

    bool getBitrate(int64_t *bitrate);
    int nativeSuspend();

    status_t setNativeWindow_l(const int hlay);

    int StagefrightVideoRenderInit(int width, int height, int format, int nRenderMode);
    void StagefrightVideoRenderExit();
    void StagefrightVideoRenderData(void *frame_info, int frame_id);
    int StagefrightVideoRenderGetFrameID();
    int StagefrightVideoRenderDequeueFrame(void *frame_info, int frame_id, ANativeWindowBufferCedarXWrapper *pANativeWindowBuffer);
    int StagefrightVideoRenderEnqueueFrame(ANativeWindowBufferCedarXWrapper *pANativeWindowBuffer);

    int StagefrightAudioRenderInit(int samplerate, int channels, int format);
    void StagefrightAudioRenderExit(int immed);
    int StagefrightAudioRenderData(void* data, int len);
    int StagefrightAudioRenderGetSpace(void);
    int StagefrightAudioRenderGetDelay(void);
    int StagefrightAudioRenderFlushCache(void);
    int StagefrightAudioRenderPause(void);
#if (CEDARX_ANDROID_VERSION >= 7)
    //add by weihongqiang
    status_t selectTrack(size_t trackIndex, bool select);
    status_t getTrackInfo(Parcel *reply) const;
    size_t countTracks() const;
#endif
    status_t setDataSource_pre();
    status_t setDataSource_post();
    status_t setVideoScalingMode(int32_t mode);
    status_t setVideoScalingMode_l(int32_t mode);
    status_t setSubGate_l(bool showSub);
    bool getSubGate_l();
    status_t switchSub_l(int32_t index);
    status_t switchTrack_l(int32_t index);
    void 	 setHDMIState(bool state);
    static void HDMINotify(void* cookie, bool state);

    CedarXPlayer(const CedarXPlayer &);
    CedarXPlayer &operator=(const CedarXPlayer &);

    int mVpsspeed;    //set play speed,  aux = -40~100,=0-normal; <0-slow; >0-fast, so speed scope: (100-40)% ~ (100+100)%, ret = OK
    //int mDynamicRotation;   //dynamic rotate, init rotate also be consider as dynamic rotate. clock wise. 0: no rotate, 1: 90 degree (clock wise), 2: 180, 3: 270, 4: horizon flip, 5: vertical flig; reverse to mediaplayerinfo.h, VideoRotateAngle_0.
    int mInitRotation;    //record init rotate. clock wise.
    bool mIsDRMMedia;
    bool mHDMIPlugged;
    //HDMIListerner * mHDMIListener;
//    friend struct CedarXPlayerAdapter;
//    CedarXPlayerAdapter *pCedarXPlayerAdapter;
};

//typedef enum tag_CedarXPlayerAdapterCmd
//{
//    CEDARXPLAYERADAPTER_CMD_SETSCREEN_SPECIALPROCESS = 0,    //till androidv4.0 v1.4 version. we need special process when setScreen, for lcd and hdmi switch quickly. Don't need it from v1.5version
//    
//    CEDARXPLAYERADAPTER_CMD_,
//}CedarXPlayerAdapterCmd;


//struct CedarXPlayerAdapter  //base adapter
//{
//public:
//    CedarXPlayerAdapter(CedarXPlayer *player);
//    virtual ~CedarXPlayerAdapter();
//    int CedarXPlayerAdapterIoCtrl(int cmd, int aux, void *pbuffer);  //cmd = CedarXPlayerAdapterCmd
//    
//protected:
//    CedarXPlayer* const pCedarXPlayer; //CedarXPlayer pointer
//};

}  // namespace android

#endif  // AWESOME_PLAYER_H_

