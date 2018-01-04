/*
**
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

#include <arpa/inet.h>
#include <stdint.h>
#include <sys/types.h>

#include <binder/Parcel.h>

#include <media/IMediaPlayer.h>
#include <media/IStreamSource.h>

//#include <gui/ISurfaceTexture.h>
#include <utils/String8.h>

namespace android {

enum {
    DISCONNECT = IBinder::FIRST_CALL_TRANSACTION,
    SET_DATA_SOURCE_URL,
    SET_DATA_SOURCE_FD,
    SET_DATA_SOURCE_STREAM,
    PREPARE_ASYNC,
    START,
    STOP,
    IS_PLAYING,
    PAUSE,
    SEEK_TO,
    GET_CURRENT_POSITION,
    GET_DURATION,
    RESET,
    SET_AUDIO_STREAM_TYPE,
    SET_LOOPING,
    SET_VOLUME,
    INVOKE,
    SET_METADATA_FILTER,
    GET_METADATA,
    SET_AUX_EFFECT_SEND_LEVEL,
    ATTACH_AUX_EFFECT,
    SET_VIDEO_SURFACETEXTURE,
    SET_PARAMETER,
    GET_PARAMETER,
    SET_RETRANSMIT_ENDPOINT,
    GET_RETRANSMIT_ENDPOINT,
    SET_NEXT_PLAYER,
    /* add by Gary. start {{----------------------------------- */
    /* 2011-9-15 10:51:10 */
    /* expend interfaces about subtitle, track and so on */
//    GET_SUB_COUNT,
//    GET_SUB_LIST,
//    GET_CUR_SUB,
//    SWITCH_SUB,
//    SET_SUB_GATE,
//    GET_SUB_GATE,
//    SET_SUB_COLOR,
//    GET_SUB_COLOR,
//    SET_SUB_FRAME_COLOR,
//    GET_SUB_FRAME_COLOR,
//    SET_SUB_FONT_SIZE,
//    GET_SUB_FONT_SIZE,
    SET_SUB_CHARSET,
    GET_SUB_CHARSET,
//    SET_SUB_POSITION,
//    GET_SUB_POSITION,
    SET_SUB_DELAY,
    GET_SUB_DELAY,
//    GET_TRACK_COUNT,
//    GET_TRACK_LIST,
//    GET_CUR_TRACK,
//    SWITCH_TRACK,
//    SET_INPUT_DIMENSION_TYPE,   
//    GET_INPUT_DIMENSION_TYPE, 
//    SET_OUTPUT_DIMENSION_TYPE,  
//    GET_OUTPUT_DIMENSION_TYPE,
//    SET_ANAGLAGH_TYPE,   
//    GET_ANAGLAGH_TYPE, 
//    GET_VIDEO_ENCODE,                   
//    GET_VIDEO_FRAME_RATE,                            
//    GET_AUDIO_ENCODE,                   
//    GET_AUDIO_BIT_RATE,
//    GET_AUDIO_SAMPLE_RATE,
    /* add by Gary. end   -----------------------------------}} */
    
    /* add by Gary. start {{----------------------------------- */
    /* 2011-11-14 */
    /* support scale mode */
    ENABLE_SCALE_MODE,
    /* add by Gary. end   -----------------------------------}} */

    /* add by Gary. start {{----------------------------------- */
    /* 2012-03-07 */
    /* set audio channel mute */
    SET_CHANNEL_MUTE_MODE,
    GET_CHANNEL_MUTE_MODE,
    /* add by Gary. end   -----------------------------------}} */

    /* add by Gary. start {{----------------------------------- */
    /* 2012-4-24 */
    /* add two general interfaces for expansibility */
    GENERAL_INTERFACE,
    /* add by Gary. end   -----------------------------------}} */

    SET_DATA_SOURCE_STREAM2,
};

class BpMediaPlayer: public BpInterface<IMediaPlayer>
{
public:
    BpMediaPlayer(const sp<IBinder>& impl)
        : BpInterface<IMediaPlayer>(impl)
    {
    }

    // disconnect from media player service
    void disconnect()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(DISCONNECT, data, &reply);
    }

    status_t setDataSource(const char* url,
            const KeyedVector<String8, String8>* headers)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeCString(url);
        if (headers == NULL) {
            data.writeInt32(0);
        } else {
            // serialize the headers
            data.writeInt32(headers->size());
            for (size_t i = 0; i < headers->size(); ++i) {
                data.writeString8(headers->keyAt(i));
                data.writeString8(headers->valueAt(i));
            }
        }
        remote()->transact(SET_DATA_SOURCE_URL, data, &reply);
        return reply.readInt32();
    }

    status_t setDataSource(int fd, int64_t offset, int64_t length) {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeFileDescriptor(fd);
        data.writeInt64(offset);
        data.writeInt64(length);
        remote()->transact(SET_DATA_SOURCE_FD, data, &reply);
        return reply.readInt32();
    }

    status_t setDataSource(const sp<IStreamSource> &source) {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeStrongBinder(source->asBinder());
        remote()->transact(SET_DATA_SOURCE_STREAM, data, &reply);
        return reply.readInt32();
    }

    status_t setDataSource(const sp<IStreamSource> &source, int type) {
		Parcel data, reply;
		data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
		data.writeStrongBinder(source->asBinder());
		data.writeInt32(type);
		remote()->transact(SET_DATA_SOURCE_STREAM2, data, &reply);
		return reply.readInt32();
	}

    // pass the buffered ISurfaceTexture to the media player service
//    status_t setVideoSurfaceTexture(const sp<ISurfaceTexture>& surfaceTexture)
    status_t setVideoSurfaceTexture(const unsigned int hlay)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        sp<IBinder> b(surfaceTexture->asBinder());
//        data.writeStrongBinder(b);
        data.writeInt32(hlay);
        remote()->transact(SET_VIDEO_SURFACETEXTURE, data, &reply);
        return reply.readInt32();
    }

    status_t prepareAsync()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(PREPARE_ASYNC, data, &reply);
        return reply.readInt32();
    }

    status_t start()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(START, data, &reply);
        return reply.readInt32();
    }

    status_t stop()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(STOP, data, &reply);
        return reply.readInt32();
    }

    status_t isPlaying(bool* state)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(IS_PLAYING, data, &reply);
        *state = reply.readInt32();
        return reply.readInt32();
    }

    status_t pause()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(PAUSE, data, &reply);
        return reply.readInt32();
    }

    status_t seekTo(int msec)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(msec);
        remote()->transact(SEEK_TO, data, &reply);
        return reply.readInt32();
    }

    status_t getCurrentPosition(int* msec)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(GET_CURRENT_POSITION, data, &reply);
        *msec = reply.readInt32();
        return reply.readInt32();
    }

    status_t getDuration(int* msec)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(GET_DURATION, data, &reply);
        *msec = reply.readInt32();
        return reply.readInt32();
    }

    status_t reset()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(RESET, data, &reply);
        return reply.readInt32();
    }

    status_t setAudioStreamType(audio_stream_type_t stream)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32((int32_t) stream);
        remote()->transact(SET_AUDIO_STREAM_TYPE, data, &reply);
        return reply.readInt32();
    }

    status_t setLooping(int loop)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(loop);
        remote()->transact(SET_LOOPING, data, &reply);
        return reply.readInt32();
    }

    status_t setVolume(float leftVolume, float rightVolume)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeFloat(leftVolume);
        data.writeFloat(rightVolume);
        remote()->transact(SET_VOLUME, data, &reply);
        return reply.readInt32();
    }

    status_t invoke(const Parcel& request, Parcel *reply)
    {
        // Avoid doing any extra copy. The interface descriptor should
        // have been set by MediaPlayer.java.
        return remote()->transact(INVOKE, request, reply);
    }

    status_t setMetadataFilter(const Parcel& request)
    {
        Parcel reply;
        // Avoid doing any extra copy of the request. The interface
        // descriptor should have been set by MediaPlayer.java.
        remote()->transact(SET_METADATA_FILTER, request, &reply);
        return reply.readInt32();
    }

    status_t getMetadata(bool update_only, bool apply_filter, Parcel *reply)
    {
        Parcel request;
        request.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        // TODO: Burning 2 ints for 2 boolean. Should probably use flags in an int here.
        request.writeInt32(update_only);
        request.writeInt32(apply_filter);
        remote()->transact(GET_METADATA, request, reply);
        return reply->readInt32();
    }

    status_t setAuxEffectSendLevel(float level)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeFloat(level);
        remote()->transact(SET_AUX_EFFECT_SEND_LEVEL, data, &reply);
        return reply.readInt32();
    }

    status_t attachAuxEffect(int effectId)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(effectId);
        remote()->transact(ATTACH_AUX_EFFECT, data, &reply);
        return reply.readInt32();
    }

    status_t setParameter(int key, const Parcel& request)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(key);
        if (request.dataSize() > 0) {
            data.appendFrom(const_cast<Parcel *>(&request), 0, request.dataSize());
        }
        remote()->transact(SET_PARAMETER, data, &reply);
        return reply.readInt32();
    }

    status_t getParameter(int key, Parcel *reply)
    {
        Parcel data;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(key);
        return remote()->transact(GET_PARAMETER, data, reply);
    }

    status_t setRetransmitEndpoint(const struct sockaddr_in* endpoint)
    {
        Parcel data, reply;
        status_t err;

        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        if (NULL != endpoint) {
            data.writeInt32(sizeof(*endpoint));
            data.write(endpoint, sizeof(*endpoint));
        } else {
            data.writeInt32(0);
        }

        err = remote()->transact(SET_RETRANSMIT_ENDPOINT, data, &reply);
        if (OK != err) {
            return err;
        }
        return reply.readInt32();
    }

    status_t setNextPlayer(const sp<IMediaPlayer>& player) {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        sp<IBinder> b(player->asBinder());
        data.writeStrongBinder(b);
        remote()->transact(SET_NEXT_PLAYER, data, &reply);
        return reply.readInt32();
    }

    status_t getRetransmitEndpoint(struct sockaddr_in* endpoint)
    {
        Parcel data, reply;
        status_t err;

        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        err = remote()->transact(GET_RETRANSMIT_ENDPOINT, data, &reply);

        if ((OK != err) || (OK != (err = reply.readInt32()))) {
            return err;
        }

        data.read(endpoint, sizeof(*endpoint));

        return err;
    }

    /* add by Gary. start {{----------------------------------- */
    /* 2011-9-14 14:27:12 */
    /* expend interfaces about subtitle, track and so on */
//    int getSubCount()
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        remote()->transact(GET_SUB_COUNT, data, &reply);
//        return reply.readInt32();
//    }
//    
//    int getSubList(MediaPlayer_SubInfo *infoList, int count)
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        data.writeInt32(count);
//        remote()->transact(GET_SUB_LIST, data, &reply);
//        int nr = reply.readInt32();
//        if(nr > 0){
//            MediaPlayer_SubInfo *info;
//            for(int i = 0; i < nr; i++){
//                info = infoList + i;
//                info->len = reply.readInt32();
//                if(info->len >= 0)
//                    reply.read(info->name, info->len);
//                strcpy(info->charset, reply.readCString());
//                info->type = reply.readInt32();
//            }
//        }
//        return nr;
//    }
//    
//    int getCurSub()
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        remote()->transact(GET_CUR_SUB, data, &reply);
//        return reply.readInt32();
//    }
//    
//    status_t switchSub(int index)
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        data.writeInt32(index);
//        remote()->transact(SWITCH_SUB, data, &reply);
//        return reply.readInt32();
//    }
//    
//    status_t setSubGate(bool showSub)
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        int v = 0;
//        if(showSub)
//            v = 1;
//        data.writeInt32(v);
//        remote()->transact(SET_SUB_GATE, data, &reply);
//        return reply.readInt32();
//    }    
//    
//    bool getSubGate()
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        remote()->transact(GET_SUB_GATE, data, &reply);
//        int ret = reply.readInt32();
//        if(ret)
//            return true;
//        else
//            return false;
//    }
//    
//    status_t setSubColor(int color)
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        data.writeInt32(color);
//        remote()->transact(SET_SUB_COLOR, data, &reply);
//        return reply.readInt32();
//    }
//    
//    int getSubColor()
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        remote()->transact(GET_SUB_COLOR, data, &reply);
//        return reply.readInt32();
//    }
//    
//    status_t setSubFrameColor(int color)
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        data.writeInt32(color);
//        remote()->transact(SET_SUB_FRAME_COLOR, data, &reply);
//        return reply.readInt32();
//    }
//    
//    int getSubFrameColor()
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        remote()->transact(GET_SUB_FRAME_COLOR, data, &reply);
//        return reply.readInt32();
//    }
//    
//    status_t setSubFontSize(int size)
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        data.writeInt32(size);
//        remote()->transact(SET_SUB_FONT_SIZE, data, &reply);
//        return reply.readInt32();
//    }
//    
//    int getSubFontSize()
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        remote()->transact(GET_SUB_FONT_SIZE, data, &reply);
//        return reply.readInt32();
//    }
    
    status_t setSubCharset(const char *charset)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeCString(charset);
        remote()->transact(SET_SUB_CHARSET, data, &reply);
        return reply.readInt32();
    }
    
    status_t getSubCharset(char *charset)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(GET_SUB_CHARSET, data, &reply);
        status_t ret = reply.readInt32();
        if(ret == OK)
            strcpy(charset, reply.readCString());
        return ret;
    }
    
//    status_t setSubPosition(int percent)
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        data.writeInt32(percent);
//        remote()->transact(SET_SUB_POSITION, data, &reply);
//        return reply.readInt32();
//    }
//    
//    int getSubPosition()
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        remote()->transact(GET_SUB_POSITION, data, &reply);
//        return reply.readInt32();
//    }
    
    status_t setSubDelay(int time)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(time);
        remote()->transact(SET_SUB_DELAY, data, &reply);
        return reply.readInt32();
    }
    
    int getSubDelay()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(GET_SUB_DELAY, data, &reply);
        return reply.readInt32();
    }
    
//    int getTrackCount()
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        remote()->transact(GET_TRACK_COUNT, data, &reply);
//        return reply.readInt32();
//    }
//    
//    int getTrackList(MediaPlayer_TrackInfo *infoList, int count)
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        data.writeInt32(count);
//        remote()->transact(GET_TRACK_LIST, data, &reply);
//        int nr = reply.readInt32();
//        if(nr > 0){
//            MediaPlayer_TrackInfo *info;
//            for(int i = 0; i < nr; i++){
//                info = infoList + i;
//                info->len = reply.readInt32();
//                if(info->len >= 0)
//                    reply.read(info->name, info->len);
//                strcpy(info->charset, reply.readCString());
//            }
//        }
//        return nr;
//    }
//    
//    int getCurTrack()
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        remote()->transact(GET_CUR_TRACK, data, &reply);
//        return reply.readInt32();
//    }
//    
//    status_t switchTrack(int index)
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        data.writeInt32(index);
//        remote()->transact(SWITCH_TRACK, data, &reply);
//        return reply.readInt32();
//    }
//    
//    status_t setInputDimensionType(int type)
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        data.writeInt32(type);
//        remote()->transact(SET_INPUT_DIMENSION_TYPE, data, &reply);
//        return reply.readInt32();
//    }
//    
//    int getInputDimensionType()
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        remote()->transact(GET_INPUT_DIMENSION_TYPE, data, &reply);
//        return reply.readInt32();
//    }
//    
//    status_t setOutputDimensionType(int type)
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        data.writeInt32(type);
//        remote()->transact(SET_OUTPUT_DIMENSION_TYPE, data, &reply);
//        return reply.readInt32();
//    }
//    
//    int getOutputDimensionType()
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        remote()->transact(GET_OUTPUT_DIMENSION_TYPE, data, &reply);
//        return reply.readInt32();
//    }
//    
//    status_t setAnaglaghType(int type)
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        data.writeInt32(type);
//        remote()->transact(SET_ANAGLAGH_TYPE, data, &reply);
//        return reply.readInt32();
//    }
//    
//    int getAnaglaghType()
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        remote()->transact(GET_ANAGLAGH_TYPE, data, &reply);
//        return reply.readInt32();
//    }
//    
//    status_t getVideoEncode(char *encode)
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        remote()->transact(GET_VIDEO_ENCODE, data, &reply);
//        status_t ret = reply.readInt32();
//        if(ret == OK){
//            const char *temp = reply.readCString();
//            strcpy(encode, temp);
//        }
//        return ret;
//    }
//    
//    int getVideoFrameRate()
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        remote()->transact(GET_VIDEO_FRAME_RATE, data, &reply);
//        return reply.readInt32();
//    }
//    
//    status_t getAudioEncode(char *encode)
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        remote()->transact(GET_AUDIO_ENCODE, data, &reply);
//        status_t ret = reply.readInt32();
//        if(ret == OK){
//            const char *temp = reply.readCString();
//            strcpy(encode, temp);
//        }
//        return ret;
//    }
//    
//    int getAudioBitRate()
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        remote()->transact(GET_AUDIO_BIT_RATE, data, &reply);
//        return reply.readInt32();
//    }
//    
//    int getAudioSampleRate()
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
//        remote()->transact(GET_AUDIO_SAMPLE_RATE, data, &reply);
//        return reply.readInt32();
//    }

    /* add by Gary. end   -----------------------------------}} */

    /* add by Gary. start {{----------------------------------- */
    /* 2011-11-14 */
    /* support scale mode */
    status_t enableScaleMode(bool enable, int width, int height)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(enable);
        data.writeInt32(width);
        data.writeInt32(height);
        remote()->transact(ENABLE_SCALE_MODE, data, &reply);
        return reply.readInt32();
    }
    /* add by Gary. end   -----------------------------------}} */    

    /* add by Gary. start {{----------------------------------- */
    /* 2012-03-07 */
    /* set audio channel mute */
    status_t setChannelMuteMode(int muteMode)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(muteMode);
        remote()->transact(SET_CHANNEL_MUTE_MODE, data, &reply);
        return reply.readInt32();
    }
    
    int getChannelMuteMode()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        remote()->transact(GET_CHANNEL_MUTE_MODE, data, &reply);
        return reply.readInt32();
    }
    /* add by Gary. end   -----------------------------------}} */
    
    /* add by Gary. start {{----------------------------------- */
    /* 2012-4-24 */
    /* add two general interfaces for expansibility */
    status_t generalInterface(int cmd, int int1, int int2, int int3, void *p)
    {
        Parcel data, reply;
        status_t ret;
        data.writeInterfaceToken(IMediaPlayer::getInterfaceDescriptor());
        data.writeInt32(cmd);
        data.writeInt32(int1);
        data.writeInt32(int2);
        data.writeInt32(int3);
        remote()->transact(GENERAL_INTERFACE, data, &reply);
        if (p != NULL) {
            *((int *)p) = reply.readInt32();
        } else {
            reply.readInt32();
        }
        return reply.readInt32();
    }
    /* add by Gary. end   -----------------------------------}} */
};

IMPLEMENT_META_INTERFACE(MediaPlayer, "android.media.IMediaPlayer");

// ----------------------------------------------------------------------

status_t BnMediaPlayer::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch (code) {
        case DISCONNECT: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            disconnect();
            return NO_ERROR;
        } break;
        case SET_DATA_SOURCE_URL: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            const char* url = data.readCString();
            KeyedVector<String8, String8> headers;
            int32_t numHeaders = data.readInt32();
            for (int i = 0; i < numHeaders; ++i) {
                String8 key = data.readString8();
                String8 value = data.readString8();
                headers.add(key, value);
            }
            reply->writeInt32(setDataSource(url, numHeaders > 0 ? &headers : NULL));
            return NO_ERROR;
        } break;
        case SET_DATA_SOURCE_FD: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            int fd = data.readFileDescriptor();
            int64_t offset = data.readInt64();
            int64_t length = data.readInt64();
            reply->writeInt32(setDataSource(fd, offset, length));
            return NO_ERROR;
        }
        case SET_DATA_SOURCE_STREAM: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            sp<IStreamSource> source =
                interface_cast<IStreamSource>(data.readStrongBinder());
            reply->writeInt32(setDataSource(source));
            return NO_ERROR;
        }
        case SET_DATA_SOURCE_STREAM2: {
			CHECK_INTERFACE(IMediaPlayer, data, reply);
			sp<IStreamSource> source =
				interface_cast<IStreamSource>(data.readStrongBinder());
			int32_t type = data.readInt32();
			reply->writeInt32(setDataSource(source, type));
			return NO_ERROR;
		}
        case SET_VIDEO_SURFACETEXTURE: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            sp<ISurfaceTexture> surfaceTexture =
//                    interface_cast<ISurfaceTexture>(data.readStrongBinder());
//            reply->writeInt32(setVideoSurfaceTexture(surfaceTexture));
            unsigned int hlay = data.readInt32();
            reply->writeInt32(setVideoSurfaceTexture(hlay));
            return NO_ERROR;
        } break;
        case PREPARE_ASYNC: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(prepareAsync());
            return NO_ERROR;
        } break;
        case START: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(start());
            return NO_ERROR;
        } break;
        case STOP: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(stop());
            return NO_ERROR;
        } break;
        case IS_PLAYING: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            bool state;
            status_t ret = isPlaying(&state);
            reply->writeInt32(state);
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case PAUSE: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(pause());
            return NO_ERROR;
        } break;
        case SEEK_TO: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(seekTo(data.readInt32()));
            return NO_ERROR;
        } break;
        case GET_CURRENT_POSITION: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            int msec;
            status_t ret = getCurrentPosition(&msec);
            reply->writeInt32(msec);
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case GET_DURATION: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            int msec;
            status_t ret = getDuration(&msec);
            reply->writeInt32(msec);
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case RESET: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(reset());
            return NO_ERROR;
        } break;
        case SET_AUDIO_STREAM_TYPE: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(setAudioStreamType((audio_stream_type_t) data.readInt32()));
            return NO_ERROR;
        } break;
        case SET_LOOPING: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(setLooping(data.readInt32()));
            return NO_ERROR;
        } break;
        case SET_VOLUME: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            float leftVolume = data.readFloat();
            float rightVolume = data.readFloat();
            reply->writeInt32(setVolume(leftVolume, rightVolume));
            return NO_ERROR;
        } break;
        case INVOKE: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            status_t result = invoke(data, reply);
            return result;
        } break;
        case SET_METADATA_FILTER: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(setMetadataFilter(data));
            return NO_ERROR;
        } break;
        case GET_METADATA: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            bool update_only = static_cast<bool>(data.readInt32());
            bool apply_filter = static_cast<bool>(data.readInt32());
            const status_t retcode = getMetadata(update_only, apply_filter, reply);
            reply->setDataPosition(0);
            reply->writeInt32(retcode);
            reply->setDataPosition(0);
            return NO_ERROR;
        } break;
        case SET_AUX_EFFECT_SEND_LEVEL: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(setAuxEffectSendLevel(data.readFloat()));
            return NO_ERROR;
        } break;
        case ATTACH_AUX_EFFECT: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(attachAuxEffect(data.readInt32()));
            return NO_ERROR;
        } break;
        case SET_PARAMETER: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            int key = data.readInt32();

            Parcel request;
            if (data.dataAvail() > 0) {
                request.appendFrom(
                        const_cast<Parcel *>(&data), data.dataPosition(), data.dataAvail());
            }
            request.setDataPosition(0);
            reply->writeInt32(setParameter(key, request));
            return NO_ERROR;
        } break;
        case GET_PARAMETER: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            return getParameter(data.readInt32(), reply);
        } break;
        case SET_RETRANSMIT_ENDPOINT: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);

            struct sockaddr_in endpoint;
            int amt = data.readInt32();
            if (amt == sizeof(endpoint)) {
                data.read(&endpoint, sizeof(struct sockaddr_in));
                reply->writeInt32(setRetransmitEndpoint(&endpoint));
            } else {
                reply->writeInt32(setRetransmitEndpoint(NULL));
            }

            return NO_ERROR;
        } break;
        case GET_RETRANSMIT_ENDPOINT: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);

            struct sockaddr_in endpoint;
            status_t res = getRetransmitEndpoint(&endpoint);

            reply->writeInt32(res);
            reply->write(&endpoint, sizeof(endpoint));

            return NO_ERROR;
        } break;
        case SET_NEXT_PLAYER: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(setNextPlayer(interface_cast<IMediaPlayer>(data.readStrongBinder())));

            return NO_ERROR;
        } break;
        /* add by Gary. start {{----------------------------------- */
        /* 2011-9-15 13:06:54 */
        /* expend interfaces about subtitle, track and so on */
//        case GET_SUB_COUNT: {      
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(getSubCount());
//            return NO_ERROR;
//        } break;
//        case GET_SUB_LIST: {       
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            int count = data.readInt32();
//            MediaPlayer_SubInfo *subList = new MediaPlayer_SubInfo[count];
//            if(subList == NULL){
//                reply->writeInt32(-1);
//                return NO_ERROR;
//            }
//            count = getSubList(subList, count);
//            if(count > 0){
//                reply->writeInt32(count);
//                MediaPlayer_SubInfo *info;
//                for(int i = 0; i < count; i++){
//                    info = subList + i;
//                    reply->writeInt32(info->len);
//                    if(info->len > 0)
//                        reply->write(info->name, info->len);
//                    reply->writeCString(info->charset);
//                    reply->writeInt32(info->type);
//                }
//            }
//            delete[] subList;
//            return NO_ERROR;
//        } break;
//        case GET_CUR_SUB: {        
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(getCurSub());
//            return NO_ERROR;
//        } break;
//        case SWITCH_SUB: {         
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(switchSub(data.readInt32()));
//            return NO_ERROR;
//        } break;
//        case SET_SUB_GATE: {       
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            int v = data.readInt32();
//            bool b = false;
//            if(v)
//                b = true;
//            reply->writeInt32(setSubGate(b));
//            return NO_ERROR;
//        } break;
//        case GET_SUB_GATE: {       
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            bool b = getSubGate();
//            int v = 0;
//            if(b)
//                v = 1;
//            reply->writeInt32(v);
//            return NO_ERROR;
//        } break;
//        case SET_SUB_COLOR: {      
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(setSubColor(data.readInt32()));
//            return NO_ERROR;
//        } break;
//        case GET_SUB_COLOR: {      
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(getSubColor());
//            return NO_ERROR;
//        } break;
//        case SET_SUB_FRAME_COLOR: {
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(setSubFrameColor(data.readInt32()));
//            return NO_ERROR;
//        } break;
//        case GET_SUB_FRAME_COLOR: {
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(getSubFrameColor());
//            return NO_ERROR;
//        } break;
//        case SET_SUB_FONT_SIZE: {  
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(setSubFontSize(data.readInt32()));
//            return NO_ERROR;
//        } break;
//        case GET_SUB_FONT_SIZE: {  
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(getSubFontSize());
//            return NO_ERROR;
//        } break;
        case SET_SUB_CHARSET: {    
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(setSubCharset(data.readCString()));
            return NO_ERROR;
        } break;
        case GET_SUB_CHARSET: {    
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            char *charset = new char[MEDIAPLAYER_NAME_LEN_MAX];
            status_t ret = getSubCharset(charset);
            reply->writeInt32(ret);
            if(ret == OK)
                reply->writeCString(charset);
            delete[] charset;
            return NO_ERROR;
        } break;
//        case SET_SUB_POSITION: {   
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(setSubPosition(data.readInt32()));
//            return NO_ERROR;
//        } break;
//        case GET_SUB_POSITION: {   
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(getSubPosition());
//            return NO_ERROR;
//        } break;
        case SET_SUB_DELAY: {      
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(setSubDelay(data.readInt32()));
            return NO_ERROR;
        } break;
        case GET_SUB_DELAY: {      
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(getSubDelay());
            return NO_ERROR;
        } break;
//        case GET_TRACK_COUNT: {    
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(getTrackCount());
//            return NO_ERROR;
//        } break;
//        case GET_TRACK_LIST: {     
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            int count = data.readInt32();
//            MediaPlayer_TrackInfo *trackList = new MediaPlayer_TrackInfo[count];
//            if(trackList == NULL){
//                reply->writeInt32(-1);
//                return NO_ERROR;
//            }
//            count = getTrackList(trackList, count);
//            if(count > 0){
//                reply->writeInt32(count);
//                MediaPlayer_TrackInfo *info;
//                for(int i = 0; i < count; i++){
//                    info = trackList + i;
//                    reply->writeInt32(info->len);
//                    if(info->len > 0)
//                        reply->write(info->name, info->len);
//                    reply->writeCString(info->charset);
//                }
//            }
//            delete[] trackList;
//            return NO_ERROR;
//         } break;
//        case GET_CUR_TRACK: {
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(getCurTrack());
//            return NO_ERROR;
//        } break;
//        case SWITCH_TRACK: {
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(switchTrack(data.readInt32()));
//            return NO_ERROR;
//        } break;
//        case SET_INPUT_DIMENSION_TYPE: { 
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            int type = data.readInt32();
//            reply->writeInt32(setInputDimensionType(type));
//            return NO_ERROR;
//        } break;
//        case GET_INPUT_DIMENSION_TYPE: { 
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(getInputDimensionType());
//            return NO_ERROR;
//        } break;
//        case SET_OUTPUT_DIMENSION_TYPE: { 
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            int type = data.readInt32();
//            reply->writeInt32(setOutputDimensionType(type));
//            return NO_ERROR;
//        } break;
//        case GET_OUTPUT_DIMENSION_TYPE: { 
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(getOutputDimensionType());
//            return NO_ERROR;
//        } break;
//        case SET_ANAGLAGH_TYPE: { 
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            int type = data.readInt32();
//            reply->writeInt32(setAnaglaghType(type));
//            return NO_ERROR;
//        } break;
//        case GET_ANAGLAGH_TYPE: { 
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(getAnaglaghType());
//            return NO_ERROR;
//        } break;
//        case GET_VIDEO_ENCODE: {         
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            char *encode = new char[MEDIAPLAYER_NAME_LEN_MAX];
//            if(encode == NULL){
//                reply->writeInt32(-1);
//                return NO_ERROR;
//            }
//            status_t ret = getVideoEncode(encode);
//            reply->writeInt32(ret);
//            if(ret == OK){
//                reply->writeCString(encode);
//            }
//            delete[] encode;
//            return NO_ERROR;
//        } break;
//        case GET_VIDEO_FRAME_RATE: {     
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(getVideoFrameRate());
//            return NO_ERROR;
//        } break;
//        case GET_AUDIO_ENCODE: {         
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            char *encode = new char[MEDIAPLAYER_NAME_LEN_MAX];
//            if(encode == NULL){
//                reply->writeInt32(-1);
//                return NO_ERROR;
//            }
//            status_t ret = getAudioEncode(encode);
//            reply->writeInt32(ret);
//            if(ret == OK){
//                reply->writeCString(encode);
//            }
//            delete[] encode;
//            return NO_ERROR;
//        } break;
//        case GET_AUDIO_BIT_RATE: {       
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(getAudioBitRate());
//            return NO_ERROR;
//        } break;
//        case GET_AUDIO_SAMPLE_RATE: {     
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(getAudioSampleRate());
//            return NO_ERROR;
//        } break;
        /* add by Gary. end   -----------------------------------}} */

        /* add by Gary. start {{----------------------------------- */
        /* 2011-11-14 */
        /* support scale mode */
        case ENABLE_SCALE_MODE: { 
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            int type = data.readInt32();
            int width = data.readInt32();
            int height = data.readInt32();
            reply->writeInt32(enableScaleMode(type, width, height));
            return NO_ERROR;
        } break;
        /* add by Gary. end   -----------------------------------}} */

        /* add by Gary. start {{----------------------------------- */
        /* 2012-03-07 */
        /* set audio channel mute */
        case SET_CHANNEL_MUTE_MODE: {      
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(setChannelMuteMode(data.readInt32()));
            return NO_ERROR;
        } break;
        case GET_CHANNEL_MUTE_MODE: {      
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            reply->writeInt32(getChannelMuteMode());
            return NO_ERROR;
        } break;
        /* add by Gary. end   -----------------------------------}} */
        /* add by Gary. start {{----------------------------------- */
        /* 2012-4-24 */
        /* add two general interfaces for expansibility */
        case GENERAL_INTERFACE: {      
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            status_t ret;
            int retval = 0;
            int cmd = data.readInt32();
            int int1 = data.readInt32();
            int int2 = data.readInt32();
            int int3 = data.readInt32();
            ret = generalInterface(cmd, int1, int2, int3, &retval);
            reply->writeInt32(retval);
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        /* add by Gary. end   -----------------------------------}} */
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

// ----------------------------------------------------------------------------

}; // namespace android
