/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef ANDROID_IMEDIAPLAYER_H
#define ANDROID_IMEDIAPLAYER_H

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include "mediaplayerinfo.h"
#include <utils/KeyedVector.h>
#include <system/audio.h>

// Fwd decl to make sure everyone agrees that the scope of struct sockaddr_in is
// global, and not in android::
struct sockaddr_in;

namespace android {

class Parcel;
class Surface;
class IStreamSource;
class ISurfaceTexture;

class IMediaPlayer: public IInterface
{
public:
    DECLARE_META_INTERFACE(MediaPlayer);

    virtual void            disconnect() = 0;

    virtual status_t        setDataSource(const char *url,
                                    const KeyedVector<String8, String8>* headers) = 0;
    virtual status_t        setDataSource(int fd, int64_t offset, int64_t length) = 0;
    virtual status_t        setDataSource(const sp<IStreamSource>& source) = 0;
    virtual status_t        setVideoSurfaceTexture(
                                    const unsigned int hlay) = 0;
    virtual status_t        prepareAsync() = 0;
    virtual status_t        start() = 0;
    virtual status_t        stop() = 0;
    virtual status_t        pause() = 0;
    virtual status_t        isPlaying(bool* state) = 0;
    virtual status_t        seekTo(int msec) = 0;
    virtual status_t        getCurrentPosition(int* msec) = 0;
    virtual status_t        getDuration(int* msec) = 0;
    virtual status_t        reset() = 0;
    virtual status_t        setAudioStreamType(audio_stream_type_t type) = 0;
    virtual status_t        setLooping(int loop) = 0;
    virtual status_t        setVolume(float leftVolume, float rightVolume) = 0;
    virtual status_t        setAuxEffectSendLevel(float level) = 0;
    virtual status_t        attachAuxEffect(int effectId) = 0;
    virtual status_t        setParameter(int key, const Parcel& request) = 0;
    virtual status_t        getParameter(int key, Parcel* reply) = 0;
    virtual status_t        setRetransmitEndpoint(const struct sockaddr_in* endpoint) = 0;
    virtual status_t        getRetransmitEndpoint(struct sockaddr_in* endpoint) = 0;
    virtual status_t        setNextPlayer(const sp<IMediaPlayer>& next) = 0;

    // Invoke a generic method on the player by using opaque parcels
    // for the request and reply.
    // @param request Parcel that must start with the media player
    // interface token.
    // @param[out] reply Parcel to hold the reply data. Cannot be null.
    // @return OK if the invocation was made successfully.
    virtual status_t        invoke(const Parcel& request, Parcel *reply) = 0;

    // Set a new metadata filter.
    // @param filter A set of allow and drop rules serialized in a Parcel.
    // @return OK if the invocation was made successfully.
    virtual status_t        setMetadataFilter(const Parcel& filter) = 0;

    // Retrieve a set of metadata.
    // @param update_only Include only the metadata that have changed
    //                    since the last invocation of getMetadata.
    //                    The set is built using the unfiltered
    //                    notifications the native player sent to the
    //                    MediaPlayerService during that period of
    //                    time. If false, all the metadatas are considered.
    // @param apply_filter If true, once the metadata set has been built based
    //                     on the value update_only, the current filter is
    //                     applied.
    // @param[out] metadata On exit contains a set (possibly empty) of metadata.
    //                      Valid only if the call returned OK.
    // @return OK if the invocation was made successfully.
    virtual status_t        getMetadata(bool update_only,
                                        bool apply_filter,
                                        Parcel *metadata) = 0;
    virtual status_t        setDataSource(const sp<IStreamSource>& source, int type) = 0;
    /* add by Gary. start {{----------------------------------- */
    /* 2011-9-15 10:25:10 */
    /* expend interfaces about subtitle, track and so on */
//    virtual int             getSubCount() = 0;
//    virtual int             getSubList(MediaPlayer_SubInfo *infoList, int count) = 0;
//    virtual int             getCurSub() = 0;
//    virtual status_t        switchSub(int index) = 0;
//    virtual status_t        setSubGate(bool showSub) = 0;
//    virtual bool            getSubGate() = 0;
//    virtual status_t        setSubColor(int color) = 0;
//    virtual int             getSubColor() = 0;
//    virtual status_t        setSubFrameColor(int color) = 0;
//    virtual int             getSubFrameColor() = 0;
//    virtual status_t        setSubFontSize(int size) = 0;
//    virtual int             getSubFontSize() = 0;
    virtual status_t        setSubCharset(const char *charset) = 0;
    virtual status_t        getSubCharset(char *charset) = 0;
//    virtual status_t        setSubPosition(int percent) = 0;
//    virtual int             getSubPosition() = 0;
    virtual status_t        setSubDelay(int time) = 0;
    virtual int             getSubDelay() = 0;
//    virtual int             getTrackCount() = 0;
//    virtual int             getTrackList(MediaPlayer_TrackInfo *infoList, int count) = 0;
//    virtual int             getCurTrack() = 0;
//    virtual status_t        switchTrack(int index) = 0;
//    virtual status_t        setInputDimensionType(int type) = 0;
//    virtual int             getInputDimensionType() = 0;
//    virtual status_t        setOutputDimensionType(int type) = 0;
//    virtual int             getOutputDimensionType() = 0;
//    virtual status_t        setAnaglaghType(int type) = 0;
//    virtual int             getAnaglaghType() = 0;
//    virtual status_t        getVideoEncode(char *encode) = 0;
//    virtual int             getVideoFrameRate() = 0;
//    virtual status_t        getAudioEncode(char *encode) = 0;
//    virtual int             getAudioBitRate() = 0;
//    virtual int             getAudioSampleRate() = 0;
    /* add by Gary. end   -----------------------------------}} */

    /* add by Gary. start {{----------------------------------- */
    /* 2011-11-14 */
    /* support scale mode */
    virtual status_t        enableScaleMode(bool enable, int width, int height) = 0;
    /* add by Gary. end   -----------------------------------}} */

    /* add by Gary. start {{----------------------------------- */
    /* 2012-03-07 */
    /* set audio channel mute */
    virtual status_t        setChannelMuteMode(int muteMode) = 0;
    virtual int             getChannelMuteMode() = 0;
    /* add by Gary. end   -----------------------------------}} */

    /* add by Gary. start {{----------------------------------- */
    /* 2012-4-24 */
    /* add two general interfaces for expansibility */
    virtual status_t        generalInterface(int cmd, int int1, int int2, int int3, void *p) = 0;
    /* add by Gary. end   -----------------------------------}} */
};

// ----------------------------------------------------------------------------

class BnMediaPlayer: public BnInterface<IMediaPlayer>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};

}; // namespace android

#endif // ANDROID_IMEDIAPLAYER_H
