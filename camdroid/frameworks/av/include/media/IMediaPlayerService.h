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

#ifndef ANDROID_IMEDIAPLAYERSERVICE_H
#define ANDROID_IMEDIAPLAYERSERVICE_H

#include <utils/Errors.h>  // for status_t
#include <utils/KeyedVector.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <system/audio.h>

#include <media/IMediaPlayerClient.h>
#include <media/IMediaPlayer.h>
#include <media/IMediaMetadataRetriever.h>
#include <media/IMediaVideoResizer.h>
#include <media/IMediaServerCaller.h>

namespace android {

struct ICrypto;
struct IHDCP;
class IMediaRecorder;
//class IOMX;
class IRemoteDisplay;
class IRemoteDisplayClient;
struct IStreamSource;

/* add by Gary. start {{----------------------------------- */
/**
*  screen name
*/
//#define MASTER_SCREEN        0
//#define SLAVE_SCREEN         1
/* add by Gary. end   -----------------------------------}} */

class IMediaPlayerService: public IInterface
{
public:
    DECLARE_META_INTERFACE(MediaPlayerService);

    virtual sp<IMediaRecorder> createMediaRecorder(pid_t pid) = 0;
    virtual sp<IMediaMetadataRetriever> createMetadataRetriever(pid_t pid) = 0;
    virtual sp<IMediaServerCaller> createMediaServerCaller(pid_t pid) = 0;
    virtual sp<IMediaPlayer> create(pid_t pid, const sp<IMediaPlayerClient>& client, int audioSessionId = 0) = 0;
    virtual sp<IMediaVideoResizer> createMediaVideoResizer(pid_t pid) = 0;

    virtual sp<IMemory>         decode(const char* url, uint32_t *pSampleRate, int* pNumChannels, audio_format_t* pFormat) = 0;
    virtual sp<IMemory>         decode(int fd, int64_t offset, int64_t length, uint32_t *pSampleRate, int* pNumChannels, audio_format_t* pFormat) = 0;
    //virtual sp<IOMX>            getOMX() = 0;
    virtual sp<ICrypto>         makeCrypto() = 0;
    virtual sp<IHDCP>           makeHDCP() = 0;

    // Connects to a remote display.
    // 'iface' specifies the address of the local interface on which to listen for
    // a connection from the remote display as an ip address and port number
    // of the form "x.x.x.x:y".  The media server should call back into the provided remote
    // display client when display connection, disconnection or errors occur.
    // The assumption is that at most one remote display will be connected to the
    // provided interface at a time.
    virtual sp<IRemoteDisplay> listenForRemoteDisplay(const sp<IRemoteDisplayClient>& client,
            const String8& iface) = 0;

    // codecs and audio devices usage tracking for the battery app
    enum BatteryDataBits {
        // tracking audio codec
        kBatteryDataTrackAudio          = 0x1,
        // tracking video codec
        kBatteryDataTrackVideo          = 0x2,
        // codec is started, otherwise codec is paused
        kBatteryDataCodecStarted        = 0x4,
        // tracking decoder (for media player),
        // otherwise tracking encoder (for media recorder)
        kBatteryDataTrackDecoder        = 0x8,
        // start to play an audio on an audio device
        kBatteryDataAudioFlingerStart   = 0x10,
        // stop/pause the audio playback
        kBatteryDataAudioFlingerStop    = 0x20,
        // audio is rounted to speaker
        kBatteryDataSpeakerOn           = 0x40,
        // audio is rounted to devices other than speaker
        kBatteryDataOtherAudioDeviceOn  = 0x80,
    };

    virtual void addBatteryData(uint32_t params) = 0;
    virtual status_t pullBatteryData(Parcel* reply) = 0;
    /* add by Gary. start {{----------------------------------- */
//    virtual status_t        setScreen(int screen) = 0;
//    virtual status_t        getScreen(int *screen) = 0;
    virtual status_t        isPlayingVideo(int *playing) = 0;
    /* add by Gary. end   -----------------------------------}} */

    /* add by Gary. start {{----------------------------------- */
    /* 2011-11-14 */
    /* support adjusting colors while playing video */
//    virtual status_t        setVppGate(bool enableVpp) = 0;
//    virtual bool            getVppGate() = 0;
//    virtual status_t        setLumaSharp(int value) = 0;
//    virtual int             getLumaSharp() = 0;
//    virtual status_t        setChromaSharp(int value) = 0;
//    virtual int             getChromaSharp() = 0;
//    virtual status_t        setWhiteExtend(int value) = 0;
//    virtual int             getWhiteExtend() = 0;
//    virtual status_t        setBlackExtend(int value) = 0;
//    virtual int             getBlackExtend() = 0;
    /* add by Gary. end   -----------------------------------}} */

    /* add by Gary. start {{----------------------------------- */
    /* 2012-03-12 */
    /* add the global interfaces to control the subtitle gate  */
//    virtual status_t        setGlobalSubGate(bool showSub) = 0;
//    virtual bool            getGlobalSubGate() = 0;
    /* add by Gary. end   -----------------------------------}} */

    /* add by Gary. start {{----------------------------------- */
    /* 2012-4-24 */
    /* add two general interfaces for expansibility */
//    virtual status_t        generalGlobalInterface(int cmd, int int1, int int2, int int3, void *p) = 0;
    /* add by Gary. end   -----------------------------------}} */
};

// ----------------------------------------------------------------------------

class BnMediaPlayerService: public BnInterface<IMediaPlayerService>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};

}; // namespace android

#endif // ANDROID_IMEDIAPLAYERSERVICE_H
