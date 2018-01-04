/*
**
** Copyright 2009, The Android Open Source Project
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

#ifndef ANDROID_CEDARTPLAYER_H
#define ANDROID_CEDARTPLAYER_H

#include <media/MediaPlayerInterface.h>

namespace android {

struct CedarXPlayer;

class CedarPlayer : public MediaPlayerInterface {
public:
    CedarPlayer();
    virtual ~CedarPlayer();

    virtual status_t initCheck();

    virtual status_t setUID(uid_t uid);

    virtual status_t setDataSource(
            const char *url, const KeyedVector<String8, String8> *headers);

    virtual status_t setDataSource(int fd, int64_t offset, int64_t length);

    virtual status_t setDataSource(const sp<IStreamSource> &source);

    virtual status_t setVideoSurface(const unsigned int hlay);
//    virtual status_t setVideoSurfaceTexture(
//            const sp<ISurfaceTexture> &surfaceTexture);
    virtual status_t setVideoSurfaceTexture(
            const unsigned int hlay);
    virtual status_t prepare();
    virtual status_t prepareAsync();
    virtual status_t start();
    virtual status_t stop();
    virtual status_t pause();
    virtual bool isPlaying();
    virtual status_t seekTo(int msec);
    virtual status_t getCurrentPosition(int *msec);
    virtual status_t getDuration(int *msec);
    virtual status_t reset();
    virtual status_t setLooping(int loop);
    virtual player_type playerType();
    virtual status_t invoke(const Parcel &request, Parcel *reply);
    virtual void setAudioSink(const sp<AudioSink> &audioSink);
    virtual status_t setParameter(int key, const Parcel &request);
    virtual status_t getParameter(int key, Parcel *reply);
//    virtual status_t setScreen(int screen);
    virtual int getMeidaPlayerState();

    virtual status_t getMetadata(
            const media::Metadata::Filter& ids, Parcel *records);

    //virtual status_t dump(int fd, const Vector<String16> &args) const;
//    virtual int getSubCount();
//    virtual int getSubList(MediaPlayer_SubInfo *infoList, int count);
//    virtual int getCurSub();
//    virtual status_t switchSub(int index);
//    virtual status_t setSubGate(bool showSub);
//    virtual bool getSubGate();
//    virtual status_t setSubColor(int color);
//    virtual int getSubColor();
//    virtual status_t setSubFrameColor(int color);
//    virtual int getSubFrameColor();
//    virtual status_t setSubFontSize(int size);
//    virtual int getSubFontSize();
    virtual status_t setSubCharset(const char *charset);
    virtual status_t getSubCharset(char *charset);
//    virtual status_t setSubPosition(int percent);
//    virtual int getSubPosition();
    virtual status_t setSubDelay(int time);
    virtual int getSubDelay();
//    virtual int getTrackCount();
//    virtual int getTrackList(MediaPlayer_TrackInfo *infoList, int count);
//    virtual int getCurTrack();
//    virtual status_t switchTrack(int index);
//    virtual status_t setInputDimensionType(int type);
//    virtual int getInputDimensionType();
//    virtual status_t setOutputDimensionType(int type);
//    virtual int getOutputDimensionType();
//    virtual status_t setAnaglaghType(int type);
//    virtual int getAnaglaghType();
//    virtual status_t getVideoEncode(char *encode);
//    virtual int getVideoFrameRate();
//    virtual status_t getAudioEncode(char *encode);
//    virtual int getAudioBitRate();
//    virtual int getAudioSampleRate();

    virtual status_t        enableScaleMode(bool enable, int width, int height);
//    virtual status_t        setVppGate(bool enableVpp);
//    virtual status_t        setLumaSharp(int value);
//    virtual status_t        setChromaSharp(int value);
//    virtual status_t        setWhiteExtend(int value);
//    virtual status_t        setBlackExtend(int value);
//    virtual status_t 		extensionControl(int command, int para0, int para1);
    virtual status_t 		generalInterface(int cmd, int int1, int int2, int int3, void *p);
    //don't add any extension interface in future!!

private:
    CedarXPlayer *mPlayer;

    CedarPlayer(const CedarPlayer &);
    CedarPlayer &operator=(const CedarPlayer &);
};

}  // namespace android

#endif  // ANDROID_CedarPlayer_H
