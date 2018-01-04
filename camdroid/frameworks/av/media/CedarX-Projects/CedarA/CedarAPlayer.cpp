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
#define LOG_TAG "CedarAPlayer"
#include <utils/Log.h>
 
#include <dlfcn.h>

#include <binder/IPCThreadState.h>
#include <CedarAAudioPlayer.h>
#include <media/stagefright/DataSource.h>
#include <media/stagefright/FileSource.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaExtractor.h>

#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>

#if (CEDARX_ANDROID_VERSION < 7)
	#include <media/stagefright/MediaDebug.h>
	#if (CEDARX_ANDROID_VERSION < 6)
	#include <media/stagefright/VideoRenderer.h>
	#include <surfaceflinger/ISurface.h>
	#else
//	#include <surfaceflinger/Surface.h>
//	#include <gui/ISurfaceTexture.h>
//	#include <gui/SurfaceTextureClient.h>
//	#include <surfaceflinger/ISurfaceComposer.h>
	#endif
#else
//#include <gui/ISurfaceTexture.h>
//#include <gui/SurfaceTextureClient.h>
#include <media/stagefright/foundation/ADebug.h>
#endif

#include <media/stagefright/foundation/ALooper.h>
//#include <OMX_IVCommon.h>

#include "CedarAPlayer.h"

namespace android {

extern "C" int CedarAPlayerCallbackWrapper(void *cookie, int event, void *info);

CedarAPlayer::CedarAPlayer() :
	mQueueStarted(false),mAudioPlayer(NULL), mFlags(0), mExtractorFlags(0),
	mSuspensionPositionUs(0){

	LOGV("Construction");
	reset_l();

	CDADecoder_Create((void**)&mPlayer);
	mPlayer->control(mPlayer, CDA_CMD_REGISTER_CALLBACK, (unsigned int)&CedarAPlayerCallbackWrapper, (unsigned int)this);
	isCedarAInitialized = true;
}

CedarAPlayer::~CedarAPlayer() {

	if(isCedarAInitialized){
		//mPlayer->control(mPlayer, CDA_CMD_STOP, 0, 0);
		CDADecoder_Destroy(mPlayer);
		mPlayer = NULL;
		isCedarAInitialized = false;
	}

	if (mAudioPlayer) {
		LOGV("delete mAudioPlayer");
		delete mAudioPlayer;
		mAudioPlayer = NULL;
	}

	LOGV("Deconstruction %x",mFlags);
}

#if (CEDARX_ANDROID_VERSION >= 6)
void CedarAPlayer::setUID(uid_t uid) {
    LOGV("CedarXPlayer running on behalf of uid %d", uid);

    mUID = uid;
    mUIDValid = true;
}
#endif

void CedarAPlayer::setListener(const wp<MediaPlayerBase> &listener) {
	Mutex::Autolock autoLock(mLock);
	mListener = listener;
}

status_t CedarAPlayer::setDataSource(const char *uri, const KeyedVector<
		String8, String8> *headers) {
	//Mutex::Autolock autoLock(mLock);
	LOGV("CedarAPlayer::setDataSource (%s)", uri);
	return mPlayer->control(mPlayer, CDA_SET_DATASOURCE_URL, (unsigned int)uri, 0);
}

status_t CedarAPlayer::setDataSource(int fd, int64_t offset, int64_t length) {
	//Mutex::Autolock autoLock(mLock);
	LOGV("CedarAPlayer::setDataSource fd");
	CedarAFdDesc ext_fd_desc;
	ext_fd_desc.fd = fd;
	ext_fd_desc.offset = offset;
	ext_fd_desc.length = length;
	return mPlayer->control(mPlayer, CDA_SET_DATASOURCE_FD, (unsigned int)&ext_fd_desc, 0);
}

#if (CEDARX_ANDROID_VERSION >= 6)
status_t CedarAPlayer::setDataSource(const sp<IStreamSource> &source) {
    return INVALID_OPERATION;
}

status_t CedarAPlayer::setParameter(int key, const Parcel &request)
{
	return ERROR_UNSUPPORTED;
}

status_t CedarAPlayer::getParameter(int key, Parcel *reply) {
	return ERROR_UNSUPPORTED;
}
#endif

void CedarAPlayer::reset() {
	//Mutex::Autolock autoLock(mLock);
	LOGV("RESET ???????????????, context: %p",this);

	if(mPlayer != NULL){
		//mPlayer->control(mPlayer, CDA_CMD_RESET, 0, 0);
		if(isCedarAInitialized){
            ALOGD("(f:%s, l:%d) call CDADecoder_Destroy when reset()", __FUNCTION__, __LINE__);
    		//mPlayer->control(mPlayer, CDA_CMD_STOP, 0, 0);
    		CDADecoder_Destroy(mPlayer);
    		mPlayer = NULL;
    		isCedarAInitialized = false;
	    }
	}

	reset_l();
}

void CedarAPlayer::reset_l() {
	LOGV("reset_l");

	mDurationUs = 0;
	mFlags = 0;

	mSeeking = false;
	mSeekNotificationSent = false;
	mSeekTimeUs = 0;
	mLastValidPosition = 0;

	mBitrate = -1;
}

void CedarAPlayer::notifyListener_l(int msg, int ext1, int ext2) {
	if (mListener != NULL) {
		sp<MediaPlayerBase> listener = mListener.promote();

		if (listener != NULL) {
			listener->sendEvent(msg, ext1, ext2);
		}
	}
}

status_t CedarAPlayer::play() {
	LOGV("CedarAPlayer::play()");
	status_t ret = OK;

	//Mutex::Autolock autoLock(mLock);

	mFlags &= ~CACHE_UNDERRUN;

	ret = play_l(CDA_CMD_PLAY);

	LOGV("CedarAPlayer::play() end");
	return ret;
}

status_t CedarAPlayer::play_l(int command) {
	LOGV("CedarAPlayer::play_l()");
	if (mFlags & PLAYING) {
		return OK;
	}

	mFlags |= PLAYING;

	if(mAudioPlayer){
		mAudioPlayer->resume();
		mAudioPlayer->setPlaybackEos(false);
	}

	//if (!(mFlags & PAUSING)) {
	if(1){
		LOGV("CedarAPlayer::play_l start cedara...");
		mPlayer->control(mPlayer, CDA_CMD_PLAY, (unsigned int)&mSuspensionPositionUs, 0);
	}

	mFlags &= ~PAUSING;
	mFlags &= ~AT_EOS;

	return OK;
}

status_t CedarAPlayer::stop() {
	LOGV("CedarAPlayer::stop");

	//mPlayer->control(mPlayer, CDA_CMD_STOP, 0, 0);
	stop_l();

	return OK;
}

status_t CedarAPlayer::stop_l() {
	LOGV("stop() status:%x", mFlags & PLAYING);

	LOGV("MEDIA_PLAYBACK_COMPLETE");
	notifyListener_l(MEDIA_PLAYBACK_COMPLETE);

	pause_l(true);

	mFlags &= ~SUSPENDING;
	LOGV("stop finish 1...");

	return OK;
}

status_t CedarAPlayer::pause() {
	//Mutex::Autolock autoLock(mLock);
	mFlags &= ~CACHE_UNDERRUN;
	//mPlayer->control(mPlayer, CDA_CMD_PAUSE, 0, 0);

	return pause_l(false);
}

status_t CedarAPlayer::pause_l(bool at_eos) {
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
			mAudioPlayer->pause();
		}
	}
    mPlayer->control(mPlayer, CDA_CMD_PAUSE, 0, 0);

	mFlags &= ~PLAYING;
	mFlags |= PAUSING;

	return OK;
}

bool CedarAPlayer::isPlaying() const {
	return (mFlags & PLAYING) || (mFlags & CACHE_UNDERRUN);
}

#if (CEDARX_ANDROID_VERSION < 6)
void CedarAPlayer::setISurface(const sp<ISurface> &isurface) {
	//Mutex::Autolock autoLock(mLock);
	mISurface = isurface;
}
#else

#if (CEDARX_ANDROID_VERSION < 7)
status_t CedarAPlayer::setSurface(const sp<Surface> &surface) {
    //Mutex::Autolock autoLock(mLock);

    //mSurface = surface;
    return OK;
}
#endif

status_t CedarAPlayer::setSurfaceTexture(const unsigned int hlay) {
    //Mutex::Autolock autoLock(mLock);
    return OK;
}
#endif

void CedarAPlayer::setAudioSink(const sp<MediaPlayerBase::AudioSink> &audioSink) {
	//Mutex::Autolock autoLock(mLock);
	mAudioSink = audioSink;
}

status_t CedarAPlayer::setLooping(bool shouldLoop) {
	//Mutex::Autolock autoLock(mLock);

	mFlags = mFlags & ~LOOPING;

	if (shouldLoop) {
		mFlags |= LOOPING;
	}

	return OK;
}

status_t CedarAPlayer::getDuration(int64_t *durationUs) {

	mPlayer->control(mPlayer, CDA_CMD_GET_DURATION, (unsigned int)durationUs, 0);
	*durationUs *= 1000;
	mDurationUs = *durationUs;

	return OK;
}

status_t CedarAPlayer::getPosition(int64_t *positionUs) {

	if(mFlags & AT_EOS){
		*positionUs = mDurationUs;
		return OK;
	}
	
	mPlayer->control(mPlayer, CDA_CMD_GET_POSITION, (unsigned int)positionUs, 0);

	if(*positionUs == -1){
		*positionUs = mSeekTimeUs;
	}

	return OK;
}

status_t CedarAPlayer::seekTo(int64_t timeUs) {

	int64_t currPositionUs;
	mSeekNotificationSent = false;
	LOGV("seek cmd0 to %lldms", timeUs);

	if (mSeeking) {
		LOGV( "seeking while paused or is seeking, sending SEEK_COMPLETE notification"
					" immediately.");

		notifyListener_l(MEDIA_SEEK_COMPLETE);
		mSeekNotificationSent = true;
		return OK;
	}

	getPosition(&currPositionUs);
	mSeekTimeUs = timeUs * 1000;
	mSeeking = true;

	{
		mPlayer->control(mPlayer, CDA_CMD_SEEK, (int)timeUs, (int)(currPositionUs/1000));
		if(mAudioPlayer){
			mAudioPlayer->seekTo(0);
		}
		mSeeking = false;
		if (!mSeekNotificationSent) {
			LOGV("MEDIA_SEEK_COMPLETE return");
			notifyListener_l(MEDIA_SEEK_COMPLETE);
			mSeekNotificationSent = true;
		}
	}

	LOGV("--------- seek cmd1 to %lldms end -----------", timeUs);

	return OK;
}

void CedarAPlayer::finishAsyncPrepare_l(int err){

	LOGV("finishAsyncPrepare_l");
	if(err == -1){
		LOGE("CedarAPlayer:prepare error!");
		abortPrepare(UNKNOWN_ERROR);
		return;
	}

	//mPlayer->control(mPlayer, CDA_CMD_GET_STREAM_TYPE, (unsigned int)&mStreamType, 0);
	//mPlayer->control(mPlayer, CDA_CMD_GET_MEDIAINFO, (unsigned int)&mMediaInfo, 0);

	if(mIsAsyncPrepare){
		notifyListener_l(MEDIA_PREPARED);
	}

	return;
}

void CedarAPlayer::finishSeek_l(int err){

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

	return;
}


status_t CedarAPlayer::prepare_l() {

	return mPrepareResult;
}

status_t CedarAPlayer::prepareAsync() {
	LOGV("prepare Async");
	//Mutex::Autolock autoLock(mLock);
	if (mFlags & PREPARING) {
		return UNKNOWN_ERROR; // async prepare already pending
	}
	mFlags |= PREPARING;
	mIsAsyncPrepare = true;

	if(mPlayer->control(mPlayer, CDA_CMD_PREPARE, 0, 0) != 0){
		return UNKNOWN_ERROR;
	}
	finishAsyncPrepare_l(0);

	return OK; //(//mPlayer->control(mPlayer, CDA_CMD_PREPARE_ASYNC, 0, 0) == 0 ? OK : UNKNOWN_ERROR);
}

status_t CedarAPlayer::prepare() {
	//Mutex::Autolock autoLock(mLock);
	LOGV("prepare");
	mIsAsyncPrepare = false;

	if(mPlayer->control(mPlayer, CDA_CMD_PREPARE, 0, 0) != 0){
		return UNKNOWN_ERROR;
	}

	finishAsyncPrepare_l(0);

	return OK;
}

void CedarAPlayer::abortPrepare(status_t err) {
	CHECK(err != OK);

	if (mIsAsyncPrepare) {
		notifyListener_l(MEDIA_ERROR, MEDIA_ERROR_UNKNOWN, err);
	}

	mPrepareResult = err;
	mFlags &= ~(PREPARING | PREPARE_CANCELLED);
}

status_t CedarAPlayer::suspend() {
	LOGV("suspend 0");

	if (mFlags & SUSPENDING)
		return OK;

	getPosition(&mSuspensionPositionUs);

	mFlags |= SUSPENDING;

	LOGV("suspend 2");

	return OK;
}

status_t CedarAPlayer::resume() {
	LOGV("resume");


	mSuspensionPositionUs = 0;

	return OK;
}

//status_t CedarAPlayer::setScreen(int screen) {
//	mScreenID = screen;
//	return UNKNOWN_ERROR;
//}

uint32_t CedarAPlayer::flags() const {
	return MediaExtractor::CAN_PAUSE | MediaExtractor::CAN_SEEK;//TODO:
}

int CedarAPlayer::StagefrightAudioRenderInit(int samplerate, int channels, int format)
{
	//initial audio playback
	if (mAudioPlayer == NULL) {
		if (mAudioSink != NULL) {
			mAudioPlayer = new CedarAAudioPlayer(mAudioSink, this);
			//mAudioPlayer->setSource(mAudioSource);
			LOGV("set audio format: (%d : %d)", samplerate, channels);
			mAudioPlayer->setFormat(samplerate, channels);

			status_t err = mAudioPlayer->start(true /* sourceAlreadyStarted */);

			if (err != OK) {
				delete mAudioPlayer;
				mAudioPlayer = NULL;

				mFlags &= ~(PLAYING);

				return err;
			}

			mWaitAudioPlayback = 1;
		}
	} else {
		mAudioPlayer->resume();
	}

	return 0;
}

void CedarAPlayer::StagefrightAudioRenderExit(int immed)
{
	//nothing to do;
}

int CedarAPlayer::StagefrightAudioRenderData(void* data, int len)
{
	return mAudioPlayer->render(data,len);
}

int CedarAPlayer::StagefrightAudioRenderGetSpace(void)
{
	return mAudioPlayer->getSpace();
}

int CedarAPlayer::StagefrightAudioRenderGetDelay(void)
{
	return mAudioPlayer->getLatency();
}

int CedarAPlayer::CedarAPlayerCallback(int event, void *info)
{
	int ret = 0;
	int *para = (int*)info;

	//LOGV("----------CedarAPlayerCallback event:%d info:%p\n", event, info);

	switch (event) {
	case CDA_EVENT_PLAYBACK_COMPLETE:
		mFlags &= ~PLAYING;
		mFlags |= AT_EOS;
		if (mAudioPlayer != NULL) {
			mAudioPlayer->setPlaybackEos(true);
		}
		stop_l(); //for gallery
		break;

	case CDA_EVENT_AUDIORENDERINIT:
		StagefrightAudioRenderInit(para[0], para[1], para[2]);
		break;

	case CDA_EVENT_AUDIORENDEREXIT:
		StagefrightAudioRenderExit(0);
		break;

	case CDA_EVENT_AUDIORENDERDATA:
		ret = StagefrightAudioRenderData((void*)para[0],para[1]);
		break;

	case CDA_EVENT_AUDIORENDERGETSPACE:
		ret = StagefrightAudioRenderGetSpace();
		break;

	case CDA_EVENT_AUDIORENDERGETDELAY:
		ret = StagefrightAudioRenderGetDelay();
		break;
    case CDA_EVENT_AUDIORAWSPDIFPLAY:
    {
        int64_t token = IPCThreadState::self()->clearCallingIdentity();
	
    	static int raw_data_test = 0;
    	String8 raw1 = String8("raw_data_output=1");
    	String8 raw0 = String8("raw_data_output=0");
    	const sp<IAudioFlinger>& af = AudioSystem::get_audio_flinger();
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

//	case CDA_EVENT_PREPARED:
//		finishAsyncPrepare_l((int)para);
//		break;
//
//	case CDA_EVENT_SEEK_COMPLETE:
//		finishSeek_l(0);
//		break;

	default:
		break;
	}

	return ret;
}

extern "C" int CedarAPlayerCallbackWrapper(void *cookie, int event, void *info)
{
	return ((android::CedarAPlayer *)cookie)->CedarAPlayerCallback(event, info);
}

} // namespace android

