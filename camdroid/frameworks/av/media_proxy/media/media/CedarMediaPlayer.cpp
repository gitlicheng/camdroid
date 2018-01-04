/*
 * Copyright (C) 2006 The Android Open Source Project
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
#define LOG_TAG "CedarMediaPlayer"
#include <unistd.h>
#include <fcntl.h>
#include <utils/Log.h>

//#include "jni.h"
//#include "JNIHelp.h"
//#include "android_runtime/AndroidRuntime.h"

#include <cutils/properties.h>
#include <utils/Vector.h>

//#include <gui/SurfaceTexture.h>
//#include <gui/Surface.h>
#include <camera/Camera.h>
#include <binder/IMemory.h>
#include <binder/IServiceManager.h>

#include <CedarMediaPlayer.h>
#include <MediaCallbackDispatcher.h>


namespace android {

const char CedarMediaPlayer::IMEDIA_PLAYER[] = "android.media.IMediaPlayer";

void CedarMediaPlayer::EventHandler::handleMessage(const MediaCallbackMessage &msg)
{
	if (mCedarMP->mNativeContext == 0)
	{
		ALOGW("mediaplayer went away with unhandled events");
		return;
	}

	switch (msg.what) {
		case MEDIA_PREPARED:
			ALOGV("MEDIA_PREPARED");
			if (mCedarMP->mOnPreparedListener != NULL)
				mCedarMP->mOnPreparedListener->onPrepared(mCedarMP);
			return;

		case MEDIA_PLAYBACK_COMPLETE:
			ALOGV("MEDIA_PLAYBACK_COMPLETE");
			if (mCedarMP->mOnCompletionListener != NULL)
				mCedarMP->mOnCompletionListener->onCompletion(mCedarMP);
			//stayAwake(false);
			return;

		case MEDIA_BUFFERING_UPDATE:
			ALOGV("MEDIA_BUFFERING_UPDATE");
			if (mCedarMP->mOnBufferingUpdateListener != NULL)
				mCedarMP->mOnBufferingUpdateListener->onBufferingUpdate(mCedarMP, msg.arg1);
			return;

		case MEDIA_SEEK_COMPLETE:
			ALOGV("MEDIA_SEEK_COMPLETE");
			if (mCedarMP->mOnSeekCompleteListener != NULL)
				mCedarMP->mOnSeekCompleteListener->onSeekComplete(mCedarMP);
			return;

		case MEDIA_SET_VIDEO_SIZE:
			ALOGV("MEDIA_SET_VIDEO_SIZE");
			if (mCedarMP->mOnVideoSizeChangedListener != NULL)
				mCedarMP->mOnVideoSizeChangedListener->onVideoSizeChanged(mCedarMP, msg.arg1, msg.arg2);
			return;

		case MEDIA_ERROR:
			{
				ALOGE("Error (%d,%d)",  msg.arg1, msg.arg2);
				bool error_was_handled = false;
				if (mCedarMP->mOnErrorListener != NULL) {
					error_was_handled = mCedarMP->mOnErrorListener->onError(mCedarMP, msg.arg1, msg.arg2);
				}
				if (mCedarMP->mOnCompletionListener != NULL && !error_was_handled) {
					mCedarMP->mOnCompletionListener->onCompletion(mCedarMP);
				}
				//stayAwake(false);
                return;
			}

		case MEDIA_INFO:
			ALOGV("MEDIA_INFO");
			if (msg.arg1 != MEDIA_INFO_VIDEO_TRACK_LAGGING) {
				ALOGI("Info (%d,%d)", msg.arg1, msg.arg2);
			}
			if (mCedarMP->mOnInfoListener != NULL) {
				mCedarMP->mOnInfoListener->onInfo(mCedarMP, msg.arg1, msg.arg2);
			}
			// No real default action so far.
			return;

		case MEDIA_NOP: // interface test message - ignore
			break;
#if 0
		case MEDIA_TIMED_TEXT:
			if (mOnTimedTextListener == NULL)
				return;
			if (msg.obj == NULL) {
				mOnTimedTextListener.onTimedText(mCedarMP, NULL);
			} else {
				if (msg.obj instanceof Parcel) {
					Parcel parcel = (Parcel)msg.obj;
					TimedText text = new TimedText(parcel);
					parcel.recycle();
					mOnTimedTextListener.onTimedText(mCedarMP, text);
				}
			}
			return;

		/*Start by Bevis. Detect http data source from other application.*/
		case MEDIA_SOURCE_DETECTED:
			if (mDlnaSourceDetector != NULL && msg.obj != NULL){
				if(msg.obj instanceof Parcel){
					Parcel parcel = (Parcel)msg.obj;
					//parcel.setDataPosition(0);
					String url = parcel.readString();
					Log.d(TAG, "######MEDIA_SOURCE_DETECTED! url = " + url);
					mDlnaSourceDetector.onSourceDetected(url);
				}
			}
			return;
			/*End by Bevis. Detect http data source from other application. */	
#endif

		default:
			ALOGE("Unknown message type %d", msg.what);
			return;
	}
}

void CedarMediaPlayer::CedarMediaPlayerListener::notify(int msg, int ext1, int ext2, const Parcel *obj)
{
	if ( mEventHandler != NULL) {
		MediaCallbackMessage message;
		message.what = msg;
		message.arg1 = ext1;
		message.arg2 = ext2;
		mEventHandler->post(message);
	}
}

CedarMediaPlayer::CedarMediaPlayer()
{
	sp<MediaPlayer> mp = new MediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed to new MediaPlayer");
		return;
	}
	mOnErrorListener = NULL;
	mOnCompletionListener = NULL;
	mOnPreparedListener = NULL;
	mOnBufferingUpdateListener = NULL;
	mOnSeekCompleteListener = NULL;
	mOnVideoSizeChangedListener = NULL;
	mOnTimedTextListener = NULL;
	mOnInfoListener = NULL;
    mNativeContext = 0;
	mEventHandler = new EventHandler(this);
	sp<CedarMediaPlayerListener> listener = new CedarMediaPlayerListener(mEventHandler);
	mp->setListener(listener);
	setMediaPlayer(mp);
}

CedarMediaPlayer::~CedarMediaPlayer()
{
}

sp<MediaPlayer> CedarMediaPlayer::setMediaPlayer(const sp<MediaPlayer>& player)
{
	Mutex::Autolock l(sLock);
	sp<MediaPlayer> old = (MediaPlayer*)mNativeContext;
	if (player.get()) {
		player->incStrong(this);
	}
	if (old != 0) {
		old->decStrong(this);
	}
	mNativeContext = (int)player.get();
	return old;
}

sp<MediaPlayer> CedarMediaPlayer::getMediaPlayer()
{
	Mutex::Autolock l(sLock);
	return sp<MediaPlayer>((MediaPlayer*)mNativeContext);
}

void CedarMediaPlayer::process_media_player_call(status_t opStatus, const char *message)
{
   if (opStatus != (status_t) OK && message != NULL) {
       ALOGE("%s, opStatus=%d", message, opStatus);
       //sp<MediaPlayer> mp = getMediaPlayer();
       //if (mp != 0) mp->notify(MEDIA_ERROR, opStatus, 0);
   } else if ( opStatus == (status_t) INVALID_OPERATION ) {
		ALOGE("INVALID_OPERATION");
   } else if ( opStatus == (status_t) PERMISSION_DENIED ) {
		ALOGE("PERMISSION_DENIED");
   }
}
void CedarMediaPlayer::invoke(Parcel *pRequest, Parcel *pReply)
{
	sp<MediaPlayer> media_player = getMediaPlayer();
	if (media_player == NULL ) {
		ALOGE("<invoke> media_player=NULL");
		return;
	}

	int ret = media_player->invoke(*pRequest, pReply);
	pReply->setDataPosition(0);
	if (ret != 0) {
		ALOGE("<invoke>Failed code %d", ret);
	}
}

void CedarMediaPlayer::setDisplay(unsigned int hlay)
{
	ALOGV("setDisplay, hlay=%d", hlay);
	mSurfaceHolder = hlay;
}

status_t CedarMediaPlayer::setVideoScalingMode(int mode)
{
	ALOGV("setVideoScalingMode, mode=%d", mode);
	if (!isVideoScalingModeSupported(mode)) {
		ALOGE("Scaling mode %d is not supported", mode);
		return INVALID_OPERATION;
	}

	Parcel request, reply;
	request.writeInterfaceToken(String16(IMEDIA_PLAYER));
	request.writeInt32(INVOKE_ID_SET_VIDEO_SCALE_MODE);
	request.writeInt32(mode);
	invoke(&request, &reply);

	return NO_ERROR;
}

status_t CedarMediaPlayer::setDataSource(String8 path)
{
	ALOGV("setDataSource, path=%s", path.string());
	return setDataSource(path, NULL);
}

status_t CedarMediaPlayer::setDataSource(String8 path, KeyedVector<String8, String8> *headers)
{
#if 0
	String8 *keys = NULL;
	String8 *values = NULL;

	if (headers != NULL) {
		int size = headers.size();
		keys = new String8[size];
		values = new String8[size];

		for (int i = 0; i < size; ++i) {
			keys[i] = headers->keyAt(i);
			values[i] = headers->valueAt(i);
		}
	}
	setDataSource(path, keys, values);
	delete[] keys;
	delete[] values;
#else
	status_t opStatus;

	if (access(path.string(), F_OK) == 0) {
		int fd = open(path.string(), O_RDONLY);
		if (fd < 0) {
			ALOGE("Failed to open file %s(%s)", path.string(), strerror(errno));
			return UNKNOWN_ERROR;
		}
		opStatus = setDataSource(fd);
		close(fd);
	} else {
		sp<MediaPlayer> mp = getMediaPlayer();
		if (mp == NULL) {
			ALOGE("Failed getMediaPlayer");
			return NO_INIT;
		}

		if (headers == NULL || headers->size() <= 0) {
			opStatus = mp->setDataSource(path.string(), NULL);
		} else {
			opStatus = mp->setDataSource(path.string(), headers);
		}
        process_media_player_call(opStatus, "setDataSource failed." );
	}
	return opStatus;
#endif
}

status_t CedarMediaPlayer::setDataSource(int fd)
{
	return setDataSource(fd, 0, 0x7ffffffffffffffL);
}

status_t CedarMediaPlayer::setDataSource(int fd, int64_t offset, int64_t length)
{
	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed getMediaPlayer");
		return NO_INIT;
	}
    status_t opStatus = mp->setDataSource(fd, offset, length);
    process_media_player_call(opStatus, "setDataSourceFD failed." );
    return opStatus;
}

status_t CedarMediaPlayer::prepare()
{
	ALOGV("prepare");
	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed getMediaPlayer");
		return NO_INIT;
	}

	mp->setVideoSurfaceTexture(mSurfaceHolder);
    status_t opStatus = mp->prepare();
    process_media_player_call(opStatus, "Prepare failed." );
    return opStatus;
}

status_t CedarMediaPlayer::prepareAsync()
{
	ALOGV("prepareAsync");
	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed getMediaPlayer");
		return NO_INIT;
	}

	mp->setVideoSurfaceTexture(mSurfaceHolder);
	return mp->prepareAsync();
}

status_t CedarMediaPlayer::start()
{
    ALOGV("start");
	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed getMediaPlayer");
		return NO_INIT;
	}
    status_t opStatus = mp->start();
    process_media_player_call(opStatus, "start failed.");
    return opStatus;
}

status_t CedarMediaPlayer::stop()
{
    ALOGV("stop");
	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed getMediaPlayer");
		return NO_INIT;
	}
    status_t opStatus = mp->stop();
    process_media_player_call(opStatus, "stop failed.");
    return opStatus;
}

status_t CedarMediaPlayer::pause()
{
    ALOGV("pause");
	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed getMediaPlayer");
		return NO_INIT;
	}
    status_t opStatus = mp->pause();
    process_media_player_call(opStatus, "pause failed.");
    return opStatus;
}

void CedarMediaPlayer::setScreenOnWhilePlaying(bool screenOn)
{
    ALOGV("setScreenOnWhilePlaying");
}

void CedarMediaPlayer::updateSurfaceScreenOn()
{
    ALOGV("updateSurfaceScreenOn");
}

status_t CedarMediaPlayer::getVideoWidth(int *width)
{
	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed getMediaPlayer");
		return NO_INIT;
	}
	return mp->getVideoWidth(width);
}

status_t CedarMediaPlayer::getVideoHeight(int *height)
{
	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed getMediaPlayer");
		return NO_INIT;
	}
	return mp->getVideoHeight(height);
}

bool CedarMediaPlayer::isPlaying()
{
	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed getMediaPlayer");
		return false;
	}

	const bool is_playing = mp->isPlaying();
	ALOGV("isPlaying: %d", is_playing);
	return is_playing;
}

status_t CedarMediaPlayer::seekTo(int msec)
{
	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed getMediaPlayer");
		return NO_INIT;
	}

	ALOGV("seekTo: %d(msec)", msec);
    status_t opStatus = mp->seekTo(msec);
    process_media_player_call(opStatus, "seekTo failed");
    return opStatus;
}

int CedarMediaPlayer::getCurrentPosition()
{
	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed getMediaPlayer");
                return -1;
	}

    int msec = -1;
    process_media_player_call(mp->getCurrentPosition(&msec), "getCurrentPosition failed.");
    ALOGV("getCurrentPosition: %d (msec)", msec);
    return msec;
}

int CedarMediaPlayer::getDuration()
{
	sp<MediaPlayer> mp = getMediaPlayer();
    if (mp == NULL) {
    	ALOGE("Failed getMediaPlayer");
        return -1;
    }

    int msec;
    process_media_player_call(mp->getDuration(&msec), "getDuration failed.");
    ALOGV("getDuration: %d (msec)", msec);
    return msec;
}

status_t CedarMediaPlayer::getMetadata(const bool update_only, const bool apply_filter, CedarMetadata *pMetadata)
{
	ALOGV("getMetadata");
	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed getMediaPlayer");
		return NO_INIT;
	}
	Parcel reply;
	if (mp->getMetadata(update_only, apply_filter, &reply) != OK) {
		ALOGE("getMetadata Failed");
		return UNKNOWN_ERROR;
	}
	if (!pMetadata->parse(reply)) {
		return UNKNOWN_ERROR;
	}
	return NO_ERROR;
}
	
status_t CedarMediaPlayer::setMetadataFilter(SortedVector<int> allow, SortedVector<int> block)
{
	ALOGV("setMetadataFilter");
	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed getMediaPlayer");
		return NO_INIT;
	}
	Parcel request;
	request.writeInterfaceToken(String16(IMEDIA_PLAYER));
	size_t capacity = request.dataSize() + 4 * (1 + allow.size() + 1 + block.size());

	if (request.dataCapacity() < capacity) {
		request.setDataCapacity(capacity);
	}
	request.writeInt32(allow.size());
	for (size_t i = 0; i < allow.size(); ++i) {
		request.writeInt32(i);
	}
	request.writeInt32(block.size());
	for (size_t i = 0; i < block.size(); ++i) {
		request.writeInt32(i);
	}

	return mp->setMetadataFilter(request);
}

status_t CedarMediaPlayer::setNextMediaPlayer(CedarMediaPlayer *pNext)
{
	ALOGV("setNextMediaPlayer");
	sp<MediaPlayer> thisplayer = getMediaPlayer();
	if (thisplayer == NULL) {
		ALOGE("This player not initialized");
		return NO_INIT;
	}
	sp<MediaPlayer> nextplayer = (pNext == NULL) ? NULL : pNext->getMediaPlayer();
	if (nextplayer == NULL && pNext != NULL) {
		ALOGE("That player not initialized");
		return NO_INIT;
	}
	if (nextplayer == thisplayer) {
		ALOGE("Next player can't be self");
		return INVALID_OPERATION;
	}
    status_t opStatus = thisplayer->setNextMediaPlayer(nextplayer);
    process_media_player_call(opStatus, "setNextMediaPlayer failed.");
    return opStatus;
}

void CedarMediaPlayer::release()
{
	ALOGV("release");
	//stayAwake(false);
	//updateSurfaceScreenOn();
	mOnPreparedListener = NULL;
	mOnBufferingUpdateListener = NULL;
	mOnCompletionListener = NULL;
	mOnSeekCompleteListener = NULL;
	mOnErrorListener = NULL;
	mOnInfoListener = NULL;
	mOnVideoSizeChangedListener = NULL;
	mOnTimedTextListener = NULL;
	
	//decVideoSurfaceRef();
	sp<MediaPlayer> mp = setMediaPlayer(0);
	if (mp != NULL) {
		mp->setListener(0);
		mp->disconnect();
	}
}

status_t CedarMediaPlayer::reset()
{
    ALOGV("reset");
	//stayAwake(false);
	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed getMediaPlayer");
		return NO_INIT;
	}
    status_t opStatus = mp->reset();
    process_media_player_call(opStatus, "reset failed.");
    return opStatus;
	//mEventHandler.removeCallbacksAndMessages(null);
}

status_t CedarMediaPlayer::setAudioStreamType(int streamtype)
{
	ALOGV("setAudioStreamType: %d", streamtype);
	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed getMediaPlayer");
		return NO_INIT;
	}
    status_t opStatus = mp->setAudioStreamType((audio_stream_type_t) streamtype);
    process_media_player_call(opStatus, "setAudioStreamType failed.");
    return opStatus;
}

status_t CedarMediaPlayer::setLooping(bool looping)
{
	ALOGV("setLooping: %d", looping);
	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed getMediaPlayer");
		return NO_INIT;
	}
    status_t opStatus = mp->setLooping(looping);
    process_media_player_call(opStatus, "setLooping failed.");
    return opStatus;
}

bool CedarMediaPlayer::isLooping()
{
	ALOGV("isLooping");
	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed getMediaPlayer");
		return false;
	}
	return mp->isLooping();
}

status_t CedarMediaPlayer::setVolume(float leftVolume, float rightVolume)
{
	ALOGV("setVolume: left %f  right %f", leftVolume, rightVolume);
	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed getMediaPlayer");
		return NO_INIT;
	}
    status_t opStatus = mp->setVolume(leftVolume, rightVolume);
    process_media_player_call(opStatus, "setVolume failed.");
    return opStatus;
}

status_t CedarMediaPlayer::setAudioSessionId(int sessionId)
{
	ALOGV("set_session_id(): %d", sessionId);
	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed getMediaPlayer");
		return NO_INIT;
	}
    status_t opStatus = mp->setAudioSessionId(sessionId);
    process_media_player_call(opStatus, "setAudioSessionId failed.");
    return opStatus;
}

int CedarMediaPlayer::getAudioSessionId()
{
	ALOGV("get_session_id()");
	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed getMediaPlayer");
		return -1;
	}
	return mp->getAudioSessionId();
}

status_t CedarMediaPlayer::attachAuxEffect(int effectId)
{
	ALOGV("attachAuxEffect(): %d", effectId);
	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed getMediaPlayer");
		return NO_INIT;
	}
    status_t opStatus = mp->attachAuxEffect(effectId);
    process_media_player_call(opStatus, "attachAuxEffect failed");
    return opStatus;
}

status_t CedarMediaPlayer::setAuxEffectSendLevel(float level)
{
	ALOGV("setAuxEffectSendLevel: level %f", level);
	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed getMediaPlayer");
		return NO_INIT;
	}
    status_t opStatus = mp->setAuxEffectSendLevel(level);
    process_media_player_call(opStatus, "setAuxEffectSendLevel failed.");
    return opStatus;
}

int CedarMediaPlayer::TrackInfo::getTrackType()
{
	return mTrackType;
}

const String8* CedarMediaPlayer::TrackInfo::getLanguage()
{
	return &mLanguage;
}

CedarMediaPlayer::TrackInfo::TrackInfo(Parcel& in) :
	mTrackType(in.readInt32()),
	mLanguage(in.readString8())
{
}

int CedarMediaPlayer::TrackInfo::describeContents()
{
	return 0;
}

void CedarMediaPlayer::TrackInfo::writeToParcel(Parcel& dest, int flags)
{
	dest.writeInt32(mTrackType);
	dest.writeString8(mLanguage);
}

void CedarMediaPlayer::getTrackInfo(Vector<sp<TrackInfo> > &trackInfo)
{
	Parcel request;
	Parcel reply;
	request.writeInterfaceToken(String16(IMEDIA_PLAYER));
	request.writeInt32(INVOKE_ID_GET_TRACK_INFO);
	invoke(&request, &reply);

	int n = reply.readInt32();
	if (n < 0) {
		return;
	}
	for (int i = 0; i < n; ++i) {
		if (reply.readInt32() != 0) {
			sp<TrackInfo> ti = new TrackInfo(reply);
			trackInfo.push(ti);
		}
	}
}


/* Do not change these values without updating their counterparts
 * in include/media/stagefright/MediaDefs.h and media/libstagefright/MediaDefs.cpp!
 */
/**
 * MIME type for SubRip (SRT) container. Used in addTimedTextSource APIs.
 */
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_SUBRIP[] = "application/x-subrip";

/* Do not change these values without updating their counterparts
 * in media/CedarX-Projects/CedarX/libutil/CDX_MediaDefs.h
 * and media/CedarX-Projects/CedarX/libutil/CDX_MediaDefs.cpp
 */    
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_IDXSUB[]   = "application/idx-sub";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_SUB[] 	  = "application/sub";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_SMI[]	  = "text/smi";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_RT[]	      = "text/rt";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_TXT[]  = "text/txt";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_SSA[]  = "text/ssa";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_AQT[]  = "text/aqt";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_JSS[]  = "text/jss";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_JS[]	  = "text/js";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_ASS[]  = "text/ass";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_VSF[]  = "text/vsf";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_TTS[]  = "text/tts";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_STL[]  = "text/stl";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_ZEG[]  = "text/zeg";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_OVR[]  = "text/ovr";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_DKS[]  = "text/dks";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_LRC[]  = "text/lrc";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_PAN[]  = "text/pan";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_SBT[]  = "text/sbt";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_VKT[]  = "text/vkt";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_PJS[]  = "text/pjs";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_MPL[]  = "text/mpl";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_SCR[]  = "text/scr";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_PSB[]  = "text/psb";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_ASC[]  = "text/asc";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_RTF[]  = "text/rtf";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_S2K[]  = "text/s2k";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_SST[]  = "text/sst";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_SON[]  = "text/son";
const char CedarMediaPlayer::MEDIA_MIMETYPE_TEXT_SSTS[] = "text/ssts";
    
bool CedarMediaPlayer::availableMimeTypeForExternalSource(String8 &mimeType)
{
	if (mimeType == MEDIA_MIMETYPE_TEXT_SUBRIP ||
		mimeType == MEDIA_MIMETYPE_TEXT_IDXSUB ||
		mimeType == MEDIA_MIMETYPE_TEXT_SUB ||
		mimeType == MEDIA_MIMETYPE_TEXT_SMI ||
		mimeType == MEDIA_MIMETYPE_TEXT_RT ||
		mimeType == MEDIA_MIMETYPE_TEXT_TXT ||
		mimeType == MEDIA_MIMETYPE_TEXT_SSA ||
		mimeType == MEDIA_MIMETYPE_TEXT_AQT ||
		mimeType == MEDIA_MIMETYPE_TEXT_JSS ||
		mimeType == MEDIA_MIMETYPE_TEXT_JS ||
		mimeType == MEDIA_MIMETYPE_TEXT_ASS ||
		mimeType == MEDIA_MIMETYPE_TEXT_VSF ||
		mimeType == MEDIA_MIMETYPE_TEXT_TTS ||
		mimeType == MEDIA_MIMETYPE_TEXT_STL ||
		mimeType == MEDIA_MIMETYPE_TEXT_ZEG ||
		mimeType == MEDIA_MIMETYPE_TEXT_OVR ||
		mimeType == MEDIA_MIMETYPE_TEXT_DKS ||
		mimeType == MEDIA_MIMETYPE_TEXT_LRC ||
		mimeType == MEDIA_MIMETYPE_TEXT_PAN ||
		mimeType == MEDIA_MIMETYPE_TEXT_SBT ||
		mimeType == MEDIA_MIMETYPE_TEXT_VKT ||
		mimeType == MEDIA_MIMETYPE_TEXT_PJS ||
		mimeType == MEDIA_MIMETYPE_TEXT_MPL ||
		mimeType == MEDIA_MIMETYPE_TEXT_SCR ||
		mimeType == MEDIA_MIMETYPE_TEXT_PSB ||
		mimeType == MEDIA_MIMETYPE_TEXT_ASC ||
		mimeType == MEDIA_MIMETYPE_TEXT_RTF ||
		mimeType == MEDIA_MIMETYPE_TEXT_S2K ||
		mimeType == MEDIA_MIMETYPE_TEXT_SST ||
		mimeType == MEDIA_MIMETYPE_TEXT_SON ||
		mimeType == MEDIA_MIMETYPE_TEXT_SSTS) {
		return true;
	}
	return false;
}

void CedarMediaPlayer::addTimedTextSource(String8 path, String8 mimeType)
{
	if (!availableMimeTypeForExternalSource(mimeType)) {
		ALOGE("Illegal mimeType for timed text source: %s", mimeType.string());
		return;
	}

	int fd = open(path.string(), O_RDONLY);
	if (fd < 0) {
		ALOGE("Failed to open file %s", path.string());
		return;
	}
	addTimedTextSource(fd, mimeType);
	close(fd);
}

void CedarMediaPlayer::addTimedTextSource(int fd, String8 mimeType)
{
	addTimedTextSource(fd, 0, 0x7ffffffffffffffL, mimeType);
}

void CedarMediaPlayer::addTimedTextSource(int fd, int64_t offset, int64_t length, String8 mimeType)
{
	if (!availableMimeTypeForExternalSource(mimeType)) {
		ALOGE("Illegal mimeType for timed text source: %s", mimeType.string());
		return;
	}
	Parcel request, reply;
	request.writeInterfaceToken(String16(IMEDIA_PLAYER));
	request.writeInt32(INVOKE_ID_ADD_EXTERNAL_SOURCE_FD);
	request.writeFileDescriptor(fd);
	request.writeInt64(offset);
	request.writeInt64(length);
	request.writeString8(mimeType);
	invoke(&request, &reply);
}

void CedarMediaPlayer::selectTrack(int index)
{
	selectOrDeselectTrack(index, true /* select */);
}

void CedarMediaPlayer::deselectTrack(int index)
{
	selectOrDeselectTrack(index, false /* select */);
}
void CedarMediaPlayer::selectOrDeselectTrack(int index, bool select)
{
	Parcel request, reply;
	request.writeInterfaceToken(String16(IMEDIA_PLAYER));
	request.writeInt32(select ? INVOKE_ID_SELECT_TRACK: INVOKE_ID_DESELECT_TRACK);
	request.writeInt32(index);
	invoke(&request, &reply);
}

status_t CedarMediaPlayer::pullBatteryData(Parcel *pReply)
{
	sp<IBinder> binder = defaultServiceManager()->getService(String16("media.player"));
	sp<IMediaPlayerService> service = interface_cast<IMediaPlayerService>(binder);
	if (service.get() == NULL) {
		ALOGE("cannot get MediaPlayerService");
		return UNKNOWN_ERROR;
	}

	return service->pullBatteryData(pReply);
}

void CedarMediaPlayer::setOnPreparedListener(OnPreparedListener *pListener)
{
	mOnPreparedListener = pListener;
}

void CedarMediaPlayer::setOnCompletionListener(OnCompletionListener *pListener)
{
	mOnCompletionListener = pListener;
}

void CedarMediaPlayer::setOnBufferingUpdateListener(OnBufferingUpdateListener *pListener)
{
	mOnBufferingUpdateListener = pListener;
}

void CedarMediaPlayer::setOnSeekCompleteListener(OnSeekCompleteListener *pListener)
{
	mOnSeekCompleteListener = pListener;
}

void CedarMediaPlayer::setOnVideoSizeChangedListener(OnVideoSizeChangedListener *pListener)
{
	mOnVideoSizeChangedListener = pListener;
}

void CedarMediaPlayer::setOnTimedTextListener(OnTimedTextListener *pListener)
{
	mOnTimedTextListener = pListener;
}

void CedarMediaPlayer::setOnErrorListener(OnErrorListener *pListener)
{
}

void CedarMediaPlayer::setOnInfoListener(OnInfoListener *pListener)
{
	mOnInfoListener = pListener;
}

status_t CedarMediaPlayer::setSubCharset(String8 charset)
{
	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed getMediaPlayer");
		return NO_INIT;
	}

	return mp->setSubCharset(charset.string());
}

status_t CedarMediaPlayer::getSubCharset(String8 &charset)
{
	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed getMediaPlayer");
		return NO_INIT;
	}
	const char *cs = charset.string();
	return mp->getSubCharset((char *)cs);
}

status_t CedarMediaPlayer::setSubDelay(int time)
{
	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed getMediaPlayer");
		return NO_INIT;
	}

	return mp->setSubDelay(time);
}

int CedarMediaPlayer::getSubDelay()
{
	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed getMediaPlayer");
		return -1;
	}

	return mp->getSubDelay();
}

status_t CedarMediaPlayer::enableScaleMode(bool enable, int width, int height)
{
	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("Failed getMediaPlayer");
		return NO_INIT;
	}

	return mp->enableScaleMode(enable, width, height);
}

status_t CedarMediaPlayer::setRotation(int val)
{
    if (val < 0 || val > 3) {
        ALOGE("<F:%s, L:%d> invalid value %d!", __FUNCTION__, __LINE__, val);
        return BAD_VALUE;
    }

	sp<MediaPlayer> mp = getMediaPlayer();
	if (mp == NULL) {
		ALOGE("<F:%s, L:%d> Failed getMediaPlayer", __FUNCTION__, __LINE__);
		return NO_INIT;
	}

    return mp->generalInterface(MEDIAPLAYER_CMD_SET_INIT_ROTATION, val, 0, 0, NULL);
}

};
 
