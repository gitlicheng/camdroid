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

// Proxy for media player implementations

//#define LOG_NDEBUG 0
#define LOG_TAG "MediaPlayerService"
#include <utils/Log.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <unistd.h>

#include <string.h>

#include <cutils/atomic.h>
#include <cutils/properties.h> // for property_get

#include <utils/misc.h>

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/MemoryHeapBase.h>
#include <binder/MemoryBase.h>
//#include <gui/SurfaceTextureClient.h>
#include <utils/Errors.h>  // for status_t
#include <utils/String8.h>
#include <utils/SystemClock.h>
#include <utils/Vector.h>

#include <media/IRemoteDisplay.h>
#include <media/IRemoteDisplayClient.h>
#include <media/MediaPlayerInterface.h>
#include <media/mediarecorder.h>
#include <media/MediaMetadataRetrieverInterface.h>
#include <media/Metadata.h>
#include <media/AudioTrack.h>
#include <media/MemoryLeakTrackUtil.h>
#include <media/stagefright/MediaErrors.h>
#include <media/mediavideoresizer.h>

#include <system/audio.h>

#include <private/android_filesystem_config.h>

#include "ActivityManager.h"
#include "MediaRecorderClient.h"
#include "MediaPlayerService.h"
#include "MetadataRetrieverClient.h"
#include "MediaPlayerFactory.h"
#include "MediaVideoResizerClient.h"
#include "MediaServerCallerClient.h"

#include "MidiFile.h"
#include "TestPlayerStub.h"
//#include "StagefrightPlayer.h"
//#include "nuplayer/NuPlayerDriver.h"

#include "CedarPlayer.h"
#include "CedarAPlayerWrapper.h"
#include "SimpleMediaFormatProbe.h"

//#include <OMX.h>

#include "Crypto.h"
#include "HDCP.h"
//#include "RemoteDisplay.h"

/* add by Gary. start {{----------------------------------- */
/* save the screen info */
#define PROP_SCREEN_KEY             "mediasw.sft.screen"
#define PROP_MASTER_SCREEN          "master"
#define PROP_SLAVE_SCREEN           "slave"
#define PROP_SCREEN_DEFAULT_VALUE   PROP_MASTER_SCREEN
/* add by Gary. end   -----------------------------------}} */

/* add by Gary. start {{----------------------------------- */
/* 2011-11-14 */
/* support adjusting colors while playing video */
#define PROP_VPP_GATE_KEY           "mediasw.sft.vpp_gate"
#define PROP_ENABLE_VPP             "enable vpp"
#define PROP_DISABLE_VPP            "disable vpp"
#define PROP_VPP_GATE_DEFAULT_VALUE PROP_DISABLE_VPP

#define PROP_LUMA_SHARP_KEY           "mediasw.sft.luma_sharp"
#define PROP_LUMA_SHARP_DEFAULT_VALUE PROP_DISABLE_VPP

#define PROP_CHROMA_SHARP_KEY           "mediasw.sft.chroma_sharp"
#define PROP_CHROMA_SHARP_DEFAULT_VALUE PROP_DISABLE_VPP

#define PROP_WHITE_EXTEND_KEY           "mediasw.sft.white_extend"
#define PROP_WHITE_EXTEND_DEFAULT_VALUE PROP_DISABLE_VPP

#define PROP_BLACK_EXTEND_KEY           "mediasw.sft.black_extend"
#define PROP_BLACK_EXTEND_DEFAULT_VALUE PROP_DISABLE_VPP
/* add by Gary. end   -----------------------------------}} */

/* add by Gary. start {{----------------------------------- */
/* 2012-03-12 */
/* add the global interfaces to control the subtitle gate  */
#define PROP_GLOBAL_SUB_GATE_KEY           "persist.mediasw.sft.sub_gate"
#define PROP_ENABLE_GLOBAL_SUB             "enable global sub"
#define PROP_DISABLE_GLOBAL_SUB            "disable global sub"
#define PROP_GLOBAL_SUB_GATE_DEFAULT_VALUE PROP_ENABLE_GLOBAL_SUB
/* add by Gary. end   -----------------------------------}} */

namespace {
using android::media::Metadata;
using android::status_t;
using android::OK;
using android::BAD_VALUE;
using android::NOT_ENOUGH_DATA;
using android::Parcel;

// Max number of entries in the filter.
const int kMaxFilterSize = 64;  // I pulled that out of thin air.

// FIXME: Move all the metadata related function in the Metadata.cpp


// Unmarshall a filter from a Parcel.
// Filter format in a parcel:
//
//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                       number of entries (n)                   |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                       metadata type 1                         |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                       metadata type 2                         |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  ....
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                       metadata type n                         |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// @param p Parcel that should start with a filter.
// @param[out] filter On exit contains the list of metadata type to be
//                    filtered.
// @param[out] status On exit contains the status code to be returned.
// @return true if the parcel starts with a valid filter.
bool unmarshallFilter(const Parcel& p,
                      Metadata::Filter *filter,
                      status_t *status)
{
    int32_t val;
    if (p.readInt32(&val) != OK)
    {
        ALOGE("Failed to read filter's length");
        *status = NOT_ENOUGH_DATA;
        return false;
    }

    if( val > kMaxFilterSize || val < 0)
    {
        ALOGE("Invalid filter len %d", val);
        *status = BAD_VALUE;
        return false;
    }

    const size_t num = val;

    filter->clear();
    filter->setCapacity(num);

    size_t size = num * sizeof(Metadata::Type);


    if (p.dataAvail() < size)
    {
        ALOGE("Filter too short expected %d but got %d", size, p.dataAvail());
        *status = NOT_ENOUGH_DATA;
        return false;
    }

    const Metadata::Type *data =
            static_cast<const Metadata::Type*>(p.readInplace(size));

    if (NULL == data)
    {
        ALOGE("Filter had no data");
        *status = BAD_VALUE;
        return false;
    }

    // TODO: The stl impl of vector would be more efficient here
    // because it degenerates into a memcpy on pod types. Try to
    // replace later or use stl::set.
    for (size_t i = 0; i < num; ++i)
    {
        filter->add(*data);
        ++data;
    }
    *status = OK;
    return true;
}

// @param filter Of metadata type.
// @param val To be searched.
// @return true if a match was found.
bool findMetadata(const Metadata::Filter& filter, const int32_t val)
{
    // Deal with empty and ANY right away
    if (filter.isEmpty()) return false;
    if (filter[0] == Metadata::kAny) return true;

    return filter.indexOf(val) >= 0;
}

}  // anonymous namespace


namespace android {

static bool checkPermission(const char* permissionString) {
#ifndef HAVE_ANDROID_OS
    return true;
#endif
    if (getpid() == IPCThreadState::self()->getCallingPid()) return true;
    bool ok = checkCallingPermission(String16(permissionString));
    if (!ok) ALOGE("Request requires %s", permissionString);
    return ok;
}

// TODO: Find real cause of Audio/Video delay in PV framework and remove this workaround
/* static */ int MediaPlayerService::AudioOutput::mMinBufferCount = 4;
/* static */ bool MediaPlayerService::AudioOutput::mIsOnEmulator = false;
///* static */ int MediaPlayerService::mHdmiPlugged = 0;

void MediaPlayerService::instantiate() {
    defaultServiceManager()->addService(
            String16("media.player"), new MediaPlayerService());
}

MediaPlayerService::MediaPlayerService()
{
    ALOGV("MediaPlayerService created");
    mNextConnId = 1;

    mBatteryAudio.refCount = 0;
    for (int i = 0; i < NUM_AUDIO_DEVICES; i++) {
        mBatteryAudio.deviceOn[i] = 0;
        mBatteryAudio.lastTime[i] = 0;
        mBatteryAudio.totalTime[i] = 0;
    }
    // speaker is on by default
    mBatteryAudio.deviceOn[SPEAKER] = 1;

    MediaPlayerFactory::registerBuiltinFactories();

    /*add by weihongqiang*/
    //
	char wfd_value[PROPERTY_VALUE_MAX];
	if (property_get("wfd.enable", wfd_value, "0")
			&& (!strcmp(wfd_value, "1") || !strcasecmp(wfd_value, "true"))) {
		if(property_set("wfd.enable", "0")) {
			ALOGI("set property wfd.enable failed");
		}
	}

    /**/
    /* add by Gary. start {{----------------------------------- */
    char prop_value[PROPERTY_VALUE_MAX];
    property_get(PROP_SCREEN_KEY, prop_value, PROP_SCREEN_DEFAULT_VALUE);
    ALOGV("prop_value = %s", prop_value);
//    String8 value( prop_value );
//    if(value == PROP_MASTER_SCREEN)
//        mScreen = MASTER_SCREEN;
//    else
//        mScreen = SLAVE_SCREEN;
    /* add by Gary. end   -----------------------------------}} */
    
    /* add by Gary. start {{----------------------------------- */
    /* 2012-03-12 */
    /* add the global interfaces to control the subtitle gate  */
    property_get(PROP_GLOBAL_SUB_GATE_KEY, prop_value, PROP_GLOBAL_SUB_GATE_DEFAULT_VALUE);
    ALOGV("prop_value of PROP_GLOBAL_SUB_GATE_KEY = %s", prop_value);
//    String8 global_sub_value(prop_value);
//    if(global_sub_value == PROP_ENABLE_GLOBAL_SUB)
//        mGlobalSubGate = true;
//    else
//        mGlobalSubGate = false;
    /* add by Gary. end   -----------------------------------}} */  
    /*Add by eric_wang. record hdmi state, 20130318 */
    //mHdmiPlugged = 0;
    /*Add by eric_wang. record hdmi state, 20130318, end ---------- */
}

MediaPlayerService::~MediaPlayerService()
{
    ALOGV("MediaPlayerService destroyed");
}

sp<IMediaRecorder> MediaPlayerService::createMediaRecorder(pid_t pid)
{
    sp<MediaRecorderClient> recorder = new MediaRecorderClient(this, pid);
    wp<MediaRecorderClient> w = recorder;
    Mutex::Autolock lock(mLock);
    mMediaRecorderClients.add(w);
    ALOGV("Create new media recorder client from pid %d", pid);
    return recorder;
}

void MediaPlayerService::removeMediaRecorderClient(wp<MediaRecorderClient> client)
{
    Mutex::Autolock lock(mLock);
    mMediaRecorderClients.remove(client);
    ALOGV("Delete media recorder client");
}

sp<IMediaMetadataRetriever> MediaPlayerService::createMetadataRetriever(pid_t pid)
{
    sp<MetadataRetrieverClient> retriever = new MetadataRetrieverClient(pid);
    ALOGV("Create new media retriever from pid %d", pid);
    return retriever;
}

sp<IMediaServerCaller> MediaPlayerService::createMediaServerCaller(pid_t pid)
{
    sp<MediaServerCallerClient> caller = new MediaServerCallerClient(pid);
    ALOGV("Create new media server caller from pid %d", pid);
    return caller;
}

sp<IMediaPlayer> MediaPlayerService::create(pid_t pid, const sp<IMediaPlayerClient>& client,
        int audioSessionId)
{
    ALOGV("MediaPlayerService::create");
    int32_t connId = android_atomic_inc(&mNextConnId);

    sp<Client> c = new Client(
            this, pid, connId, client, audioSessionId,
            IPCThreadState::self()->getCallingUid());

    ALOGV("Create new client(%d) from pid %d, uid %d, ", connId, pid,
         IPCThreadState::self()->getCallingUid());
    /* add by Gary. start {{----------------------------------- */
//    c->setScreen(mScreen);
    /* add by Gary. end   -----------------------------------}} */
//    c->setSubGate(mGlobalSubGate);  // 2012-03-12, add the global interfaces to control the subtitle gate

    wp<Client> w = c;
    {
        Mutex::Autolock lock(mLock);
        mClients.add(w);
    }
    return c;
}

//sp<IOMX> MediaPlayerService::getOMX() {
//    Mutex::Autolock autoLock(mLock);
//
//    if (mOMX.get() == NULL) {
//        mOMX = new OMX;
//    }
//
//    return mOMX;
//}


sp<IMediaVideoResizer> MediaPlayerService::createMediaVideoResizer(pid_t pid)
{
    sp<MediaVideoResizerClient> resizer = new MediaVideoResizerClient(this, pid);
    wp<MediaVideoResizerClient> w = resizer;
    Mutex::Autolock lock(mLock);
    mMediaVideoResizerClients.add(w);
    ALOGV("Create new video video resizer client from pid %d", pid);
    return resizer;
}

void MediaPlayerService::removeMediaVideoResizerClient(wp<MediaVideoResizerClient> client)
{
    Mutex::Autolock lock(mLock);
    mMediaVideoResizerClients.remove(client);
    ALOGV("Delete video resizer client");
}


sp<ICrypto> MediaPlayerService::makeCrypto() {
    return new Crypto;
}

sp<IHDCP> MediaPlayerService::makeHDCP() {
    return new HDCP;
}

sp<IRemoteDisplay> MediaPlayerService::listenForRemoteDisplay(
        const sp<IRemoteDisplayClient>& client, const String8& iface) {
//    if (!checkPermission("android.permission.CONTROL_WIFI_DISPLAY")) {
//        return NULL;
//    }
//
//    return new RemoteDisplay(client, iface.string());

    ALOGE("(f:%s, l:%d) fatal error! don't support RemoteDisplay ", __FUNCTION__, __LINE__);
    return NULL;
}

status_t MediaPlayerService::AudioCache::dump(int fd, const Vector<String16>& args) const
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;

    result.append(" AudioCache\n");
    if (mHeap != 0) {
        snprintf(buffer, 255, "  heap base(%p), size(%d), flags(%d), device(%s)\n",
                mHeap->getBase(), mHeap->getSize(), mHeap->getFlags(), mHeap->getDevice());
        result.append(buffer);
    }
    snprintf(buffer, 255, "  msec per frame(%f), channel count(%d), format(%d), frame count(%ld)\n",
            mMsecsPerFrame, mChannelCount, mFormat, mFrameCount);
    result.append(buffer);
    snprintf(buffer, 255, "  sample rate(%d), size(%d), error(%d), command complete(%s)\n",
            mSampleRate, mSize, mError, mCommandComplete?"true":"false");
    result.append(buffer);
    ::write(fd, result.string(), result.size());
    return NO_ERROR;
}

status_t MediaPlayerService::AudioOutput::dump(int fd, const Vector<String16>& args) const
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;

    result.append(" AudioOutput\n");
    snprintf(buffer, 255, "  stream type(%d), left - right volume(%f, %f)\n",
            mStreamType, mLeftVolume, mRightVolume);
    result.append(buffer);
    snprintf(buffer, 255, "  msec per frame(%f), latency (%d)\n",
            mMsecsPerFrame, (mTrack != 0) ? mTrack->latency() : -1);
    result.append(buffer);
    snprintf(buffer, 255, "  aux effect id(%d), send level (%f)\n",
            mAuxEffectId, mSendLevel);
    result.append(buffer);

    ::write(fd, result.string(), result.size());
    if (mTrack != 0) {
        mTrack->dump(fd, args);
    }
    return NO_ERROR;
}

status_t MediaPlayerService::Client::dump(int fd, const Vector<String16>& args) const
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    result.append(" Client\n");
    snprintf(buffer, 255, "  pid(%d), connId(%d), status(%d), looping(%s)\n",
            mPid, mConnId, mStatus, mLoop?"true": "false");
    result.append(buffer);
    write(fd, result.string(), result.size());
    if (mPlayer != NULL) {
        mPlayer->dump(fd, args);
    }
    if (mAudioOutput != 0) {
        mAudioOutput->dump(fd, args);
    }
    write(fd, "\n", 1);
    return NO_ERROR;
}

status_t MediaPlayerService::dump(int fd, const Vector<String16>& args)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    if (checkCallingPermission(String16("android.permission.DUMP")) == false) {
        snprintf(buffer, SIZE, "Permission Denial: "
                "can't dump MediaPlayerService from pid=%d, uid=%d\n",
                IPCThreadState::self()->getCallingPid(),
                IPCThreadState::self()->getCallingUid());
        result.append(buffer);
    } else {
        Mutex::Autolock lock(mLock);
        for (int i = 0, n = mClients.size(); i < n; ++i) {
            sp<Client> c = mClients[i].promote();
            if (c != 0) c->dump(fd, args);
        }
        if (mMediaRecorderClients.size() == 0) {
                result.append(" No media recorder client\n\n");
        } else {
            for (int i = 0, n = mMediaRecorderClients.size(); i < n; ++i) {
                sp<MediaRecorderClient> c = mMediaRecorderClients[i].promote();
                if (c != 0) {
                    snprintf(buffer, 255, " MediaRecorderClient pid(%d)\n", c->mPid);
                    result.append(buffer);
                    write(fd, result.string(), result.size());
                    result = "\n";
                    c->dump(fd, args);
                }
            }
        }

        result.append(" Files opened and/or mapped:\n");
        snprintf(buffer, SIZE, "/proc/%d/maps", gettid());
        FILE *f = fopen(buffer, "r");
        if (f) {
            while (!feof(f)) {
                fgets(buffer, SIZE, f);
                if (strstr(buffer, " /storage/") ||
                    strstr(buffer, " /system/sounds/") ||
                    strstr(buffer, " /data/") ||
                    strstr(buffer, " /system/media/")) {
                    result.append("  ");
                    result.append(buffer);
                }
            }
            fclose(f);
        } else {
            result.append("couldn't open ");
            result.append(buffer);
            result.append("\n");
        }

        snprintf(buffer, SIZE, "/proc/%d/fd", gettid());
        DIR *d = opendir(buffer);
        if (d) {
            struct dirent *ent;
            while((ent = readdir(d)) != NULL) {
                if (strcmp(ent->d_name,".") && strcmp(ent->d_name,"..")) {
                    snprintf(buffer, SIZE, "/proc/%d/fd/%s", gettid(), ent->d_name);
                    struct stat s;
                    if (lstat(buffer, &s) == 0) {
                        if ((s.st_mode & S_IFMT) == S_IFLNK) {
                            char linkto[256];
                            int len = readlink(buffer, linkto, sizeof(linkto));
                            if(len > 0) {
                                if(len > 255) {
                                    linkto[252] = '.';
                                    linkto[253] = '.';
                                    linkto[254] = '.';
                                    linkto[255] = 0;
                                } else {
                                    linkto[len] = 0;
                                }
                                if (strstr(linkto, "/storage/") == linkto ||
                                    strstr(linkto, "/system/sounds/") == linkto ||
                                    strstr(linkto, "/data/") == linkto ||
                                    strstr(linkto, "/system/media/") == linkto) {
                                    result.append("  ");
                                    result.append(buffer);
                                    result.append(" -> ");
                                    result.append(linkto);
                                    result.append("\n");
                                }
                            }
                        } else {
                            result.append("  unexpected type for ");
                            result.append(buffer);
                            result.append("\n");
                        }
                    }
                }
            }
            closedir(d);
        } else {
            result.append("couldn't open ");
            result.append(buffer);
            result.append("\n");
        }

        bool dumpMem = false;
        for (size_t i = 0; i < args.size(); i++) {
            if (args[i] == String16("-m")) {
                dumpMem = true;
            }
        }
        if (dumpMem) {
            dumpMemoryAddresses(fd);
        }
    }
    write(fd, result.string(), result.size());
    return NO_ERROR;
}

void MediaPlayerService::removeClient(wp<Client> client)
{
    Mutex::Autolock lock(mLock);
    mClients.remove(client);
}

MediaPlayerService::Client::Client(
        const sp<MediaPlayerService>& service, pid_t pid,
        int32_t connId, const sp<IMediaPlayerClient>& client,
        int audioSessionId, uid_t uid)
{
    ALOGV("Client(%d) constructor", connId);
    mPid = pid;
    mConnId = connId;
    mService = service;
    mClient = client;
    mLoop = false;
    mStatus = NO_INIT;
    mAudioSessionId = audioSessionId;
    mUID = uid;
    mRetransmitEndpointValid = false;
    mConnectedVideoLayerId = -1;

#if CALLBACK_ANTAGONIZER
    ALOGD("create Antagonizer");
    mAntagonizer = new Antagonizer(notify, this);
#endif
}

MediaPlayerService::Client::~Client()
{
    ALOGV("Client(%d) destructor pid = %d", mConnId, mPid);
    mAudioOutput.clear();
    wp<Client> client(this);
    disconnect();
    mService->removeClient(client);
}

void MediaPlayerService::Client::disconnect()
{
    ALOGV("disconnect(%d) from pid %d", mConnId, mPid);
    // grab local reference and clear main reference to prevent future
    // access to object
    sp<MediaPlayerBase> p;
    {
        Mutex::Autolock l(mLock);
        p = mPlayer;
    }
    mClient.clear();

    mPlayer.clear();

    // clear the notification to prevent callbacks to dead client
    // and reset the player. We assume the player will serialize
    // access to itself if necessary.
    if (p != 0) {
        p->setNotifyCallback(0, 0);
#if CALLBACK_ANTAGONIZER
        ALOGD("kill Antagonizer");
        mAntagonizer->kill();
#endif
        p->reset();
    }

    disconnectNativeWindow();

    IPCThreadState::self()->flushCommands();
}

sp<MediaPlayerBase> MediaPlayerService::Client::createPlayer(player_type playerType)
{
    // determine if we have the right player type
    sp<MediaPlayerBase> p = mPlayer;
    if ((p != NULL) && (p->playerType() != playerType)) {
        ALOGV("delete player");
        p.clear();
    }
    if (p == NULL) {
        p = MediaPlayerFactory::createPlayer(playerType, this, notify);
    }

    if (p != NULL) {
        p->setUID(mUID);
    }

    return p;
}

sp<MediaPlayerBase> MediaPlayerService::Client::setDataSource_pre(
        player_type playerType)
{
    // create the right type of player
    sp<MediaPlayerBase> p = createPlayer(playerType);
    if (p == NULL) {
        return p;
    }

    if (!p->hardwareOutput()) {
        mAudioOutput = new AudioOutput(mAudioSessionId);
        static_cast<MediaPlayerInterface*>(p.get())->setAudioSink(mAudioOutput);
    }

    return p;
}

void MediaPlayerService::Client::setDataSource_post(
        const sp<MediaPlayerBase>& p,
        status_t status)
{
    mStatus = status;
    if (mStatus != OK) {
        ALOGE("  error: %d", mStatus);
        return;
    }

    // Set the re-transmission endpoint if one was chosen.
    if (mRetransmitEndpointValid) {
        mStatus = p->setRetransmitEndpoint(&mRetransmitEndpoint);
        if (mStatus != NO_ERROR) {
            ALOGE("setRetransmitEndpoint error: %d", mStatus);
        }
    }

    if (mStatus == OK) {
        mPlayer = p;
    }
}

status_t MediaPlayerService::Client::setDataSource(
        const char *url, const KeyedVector<String8, String8> *headers)
{
    ALOGV("setDataSource(%s)", url);
    if (url == NULL)
        return UNKNOWN_ERROR;
    
    if ((strncmp(url, "http://", 7) == 0) ||
        (strncmp(url, "https://", 8) == 0) ||
        (strncmp(url, "rtsp://", 7) == 0)) {
        if (!checkPermission("android.permission.INTERNET")) {
            return PERMISSION_DENIED;
        }
    }

    if (strncmp(url, "content://", 10) == 0) {
        // get a filedescriptor for the content Uri and
        // pass it to the setDataSource(fd) method

        String16 url16(url);
        int fd = android::openContentProviderFile(url16);
        if (fd < 0)
        {
            ALOGE("Couldn't open fd for %s", url);
            return UNKNOWN_ERROR;
        }
        setDataSource(fd, 0, 0x7fffffffffLL); // this sets mStatus
        close(fd);
        return mStatus;
    } else {
        player_type playerType = MediaPlayerFactory::getPlayerType(this, url);

        sp<MediaPlayerBase> p = setDataSource_pre(playerType);
        if (p == NULL) {
            return NO_INIT;
        }

        /* add by Gary. start {{----------------------------------- */
        /* 2011-9-28 16:28:24 */
        /* save properties before creating the real player */
//        p->setSubGate(mSubGate);
//        p->setSubColor(mSubColor);
//        p->setSubFrameColor(mSubFrameColor);
//        p->setSubPosition(mSubPosition);
        p->setSubDelay(mSubDelay);
//        p->setSubFontSize(mSubFontSize);
        p->setSubCharset(mSubCharset);
//		p->switchSub(mSubIndex);
//		p->switchTrack(mTrackIndex);
        p->setChannelMuteMode(mMuteMode); // 2012-03-07, set audio channel mute
        /* add by Gary. end   -----------------------------------}} */
      
        setDataSource_post(p, p->setDataSource(url, headers));
        return mStatus;
    }
}

status_t MediaPlayerService::Client::setDataSource(int fd, int64_t offset, int64_t length)
{
    ALOGV("setDataSource fd=%d, offset=%lld, length=%lld", fd, offset, length);
    struct stat sb;
    int ret = fstat(fd, &sb);
    if (ret != 0) {
        ALOGE("fstat(%d) failed: %d, %s", fd, ret, strerror(errno));
        return UNKNOWN_ERROR;
    }

    ALOGV("st_dev  = %llu", sb.st_dev);
    ALOGV("st_mode = %u", sb.st_mode);
    ALOGV("st_uid  = %lu", sb.st_uid);
    ALOGV("st_gid  = %lu", sb.st_gid);
    ALOGV("st_size = %llu", sb.st_size);

    if (offset >= sb.st_size) {
        ALOGE("offset error");
        ::close(fd);
        return UNKNOWN_ERROR;
    }
    if (offset + length > sb.st_size) {
        length = sb.st_size - offset;
        ALOGV("calculated length = %lld", length);
    }

    player_type playerType = MediaPlayerFactory::getPlayerType(this,
                                                               fd,
                                                               offset,
                                                               length,
                                                               true
                                                               );
    
    sp<MediaPlayerBase> p = setDataSource_pre(playerType);
    if (p == NULL) {
        return NO_INIT;
    }

    // now set data source
    setDataSource_post(p, p->setDataSource(fd, offset, length));
    return mStatus;
}

status_t MediaPlayerService::Client::setDataSource(
        const sp<IStreamSource> &source) {
    
    // create the right type of player
    player_type playerType = MediaPlayerFactory::getPlayerType(this, source);
    sp<MediaPlayerBase> p = setDataSource_pre(playerType);
    if (p == NULL) {
        return NO_INIT;
    }

    /* add by Gary. start {{----------------------------------- */
    /* 2011-9-28 16:28:24 */
    /* save properties before creating the real player */
//    p->setSubGate(mSubGate);
//    p->setSubColor(mSubColor);
//    p->setSubFrameColor(mSubFrameColor);
//    p->setSubPosition(mSubPosition);
    p->setSubDelay(mSubDelay);
//    p->setSubFontSize(mSubFontSize);
    p->setSubCharset(mSubCharset);
//	p->switchSub(mSubIndex);
//	p->switchTrack(mTrackIndex);
    p->setChannelMuteMode(mMuteMode); // 2012-03-07, set audio channel mute
    /* add by Gary. end   -----------------------------------}} */

    // now set data source
    setDataSource_post(p, p->setDataSource(source));
    return mStatus;
}

status_t MediaPlayerService::Client::setDataSource(
        const sp<IStreamSource> &source, int type) {

	sp<MediaPlayerBase> p = setDataSource_pre(CEDARX_PLAYER);
	if (p == NULL) {
		return NO_INIT;
	}

    /* add by Gary. start {{----------------------------------- */
    /* 2011-9-28 16:28:24 */
    /* save properties before creating the real player */
//    p->setSubGate(mSubGate);
//    p->setSubColor(mSubColor);
//    p->setSubFrameColor(mSubFrameColor);
//    p->setSubPosition(mSubPosition);
    p->setSubDelay(mSubDelay);
//    p->setSubFontSize(mSubFontSize);
    p->setSubCharset(mSubCharset);
//	p->switchSub(mSubIndex);
//	p->switchTrack(mTrackIndex);
    p->setChannelMuteMode(mMuteMode); // 2012-03-07, set audio channel mute
    /* add by Gary. end   -----------------------------------}} */

    setDataSource_post(p, p->setDataSource(source));

    generalInterface(MEDIAPLAYER_CMD_SET_STREAMING_TYPE, type, 0, 0, NULL);

    return mStatus;
}

void MediaPlayerService::Client::disconnectNativeWindow() {
//    if (mConnectedWindow != NULL) {
//        status_t err = native_window_api_disconnect(mConnectedWindow.get(),
//                NATIVE_WINDOW_API_MEDIA);
//
//        if (err != OK) {
//            ALOGW("native_window_api_disconnect returned an error: %s (%d)",
//                    strerror(-err), err);
//        }
//    }
//    mConnectedWindow.clear();
    if(mConnectedVideoLayerId != (unsigned int)-1)
    {
        ALOGE("(f:%s, l:%d) simulate disconnectNativeWindow", __FUNCTION__, __LINE__);
        mConnectedVideoLayerId = -1;
    }
}

//status_t MediaPlayerService::Client::setVideoSurfaceTexture(
//        const sp<ISurfaceTexture>& surfaceTexture)
status_t MediaPlayerService::Client::setVideoSurfaceTexture(
        const unsigned int hlay)
{
    ALOGV("[%d] setVideoSurfaceTexture(%p)", mConnId, surfaceTexture.get());
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) return UNKNOWN_ERROR;

    if(mConnectedVideoLayerId == hlay)
    {
        ALOGW("(f:%s, l:%d) video layer id is same[%d], reutrn OK", __FUNCTION__, __LINE__, hlay);
        return OK;
    }
    /* don't need this block now.
    sp<IBinder> binder(surfaceTexture == NULL ? NULL :
            surfaceTexture->asBinder());
    if (mConnectedWindowBinder == binder) {
        return OK;
    }

    
    sp<ANativeWindow> anw;
    if (surfaceTexture != NULL) {
        anw = new SurfaceTextureClient(surfaceTexture);
        native_window_api_disconnect(anw.get(), NATIVE_WINDOW_API_MEDIA);
        status_t err = native_window_api_connect(anw.get(),
                NATIVE_WINDOW_API_MEDIA);

        if (err != OK) {
            ALOGE("setVideoSurfaceTexture failed: %d", err);
            // Note that we must do the reset before disconnecting from the ANW.
            // Otherwise queue/dequeue calls could be made on the disconnected
            // ANW, which may result in errors.
            reset();

            disconnectNativeWindow();

            return err;
        }
    }
    */

    // Note that we must set the player's new SurfaceTexture before
    // disconnecting the old one.  Otherwise queue/dequeue calls could be made
    // on the disconnected ANW, which may result in errors.
    //status_t err = p->setVideoSurfaceTexture(surfaceTexture);
    status_t err = p->setVideoSurfaceTexture(hlay);

    disconnectNativeWindow();

    //mConnectedWindow = anw;
    mConnectedVideoLayerId = hlay;

    if (err == OK) {
        //mConnectedWindowBinder = binder;
    } else {
        disconnectNativeWindow();
    }

    return err;
}

status_t MediaPlayerService::Client::invoke(const Parcel& request,
                                            Parcel *reply)
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == NULL) return UNKNOWN_ERROR;
    return p->invoke(request, reply);
}

// This call doesn't need to access the native player.
status_t MediaPlayerService::Client::setMetadataFilter(const Parcel& filter)
{
    status_t status;
    media::Metadata::Filter allow, drop;

    if (unmarshallFilter(filter, &allow, &status) &&
        unmarshallFilter(filter, &drop, &status)) {
        Mutex::Autolock lock(mLock);

        mMetadataAllow = allow;
        mMetadataDrop = drop;
    }
    return status;
}

status_t MediaPlayerService::Client::getMetadata(
        bool update_only, bool apply_filter, Parcel *reply)
{
    sp<MediaPlayerBase> player = getPlayer();
    if (player == 0) return UNKNOWN_ERROR;

    status_t status;
    // Placeholder for the return code, updated by the caller.
    reply->writeInt32(-1);

    media::Metadata::Filter ids;

    // We don't block notifications while we fetch the data. We clear
    // mMetadataUpdated first so we don't lose notifications happening
    // during the rest of this call.
    {
        Mutex::Autolock lock(mLock);
        if (update_only) {
            ids = mMetadataUpdated;
        }
        mMetadataUpdated.clear();
    }

    media::Metadata metadata(reply);

    metadata.appendHeader();
    status = player->getMetadata(ids, reply);

    if (status != OK) {
        metadata.resetParcel();
        ALOGE("getMetadata failed %d", status);
        return status;
    }

    // FIXME: Implement filtering on the result. Not critical since
    // filtering takes place on the update notifications already. This
    // would be when all the metadata are fetch and a filter is set.

    // Everything is fine, update the metadata length.
    metadata.updateLength();
    return OK;
}

extern int getRotation();

status_t MediaPlayerService::Client::prepareAsync()
{
    ALOGV("[%d] prepareAsync", mConnId);
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) return UNKNOWN_ERROR;

    status_t ret = p->prepareAsync();
	
	
#if CALLBACK_ANTAGONIZER
    ALOGD("start Antagonizer");
    if (ret == NO_ERROR) mAntagonizer->start();
#endif
    return ret;
}

status_t MediaPlayerService::Client::start()
{
    ALOGV("[%d] start", mConnId);
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) return UNKNOWN_ERROR;
    p->setLooping(mLoop);
    return p->start();
}

status_t MediaPlayerService::Client::stop()
{
    status_t ret;
    ALOGV("[%d] stop", mConnId);
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) return UNKNOWN_ERROR;
    //return p->stop();
    ret = p->stop();
    disconnectNativeWindow();
    IPCThreadState::self()->flushCommands();
    return ret;
}

status_t MediaPlayerService::Client::pause()
{
    ALOGV("[%d] pause", mConnId);
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) return UNKNOWN_ERROR;
    return p->pause();
}

status_t MediaPlayerService::Client::isPlaying(bool* state)
{
    *state = false;
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) return UNKNOWN_ERROR;
    *state = p->isPlaying();
    ALOGV("[%d] isPlaying: %d", mConnId, *state);
    return NO_ERROR;
}

status_t MediaPlayerService::Client::getCurrentPosition(int *msec)
{
    ALOGV("getCurrentPosition");
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) return UNKNOWN_ERROR;
    status_t ret = p->getCurrentPosition(msec);
    if (ret == NO_ERROR) {
        ALOGV("[%d] getCurrentPosition = %d", mConnId, *msec);
    } else {
        ALOGE("getCurrentPosition returned %d", ret);
    }
    return ret;
}

status_t MediaPlayerService::Client::getDuration(int *msec)
{
    ALOGV("getDuration");
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) return UNKNOWN_ERROR;
    status_t ret = p->getDuration(msec);
    if (ret == NO_ERROR) {
        ALOGV("[%d] getDuration = %d", mConnId, *msec);
    } else {
        ALOGE("getDuration returned %d", ret);
    }
    return ret;
}

status_t MediaPlayerService::Client::setNextPlayer(const sp<IMediaPlayer>& player) {
    ALOGV("setNextPlayer");
    Mutex::Autolock l(mLock);
    sp<Client> c = static_cast<Client*>(player.get());
    mNextClient = c;

    if (c != NULL) {
        if (mAudioOutput != NULL) {
            mAudioOutput->setNextOutput(c->mAudioOutput);
            if(mMsg == MEDIA_PLAYBACK_COMPLETE)//lszhang add
    		{
    			mMsg = MEDIA_NOP;
    			mAudioOutput->switchToNextOutput();
    	        mNextClient->start();
    	        mNextClient->mClient->notify(MEDIA_INFO, MEDIA_INFO_STARTED_AS_NEXT, 0, 0);
    		}
        } else if ((mPlayer != NULL) && !mPlayer->hardwareOutput()) {
            ALOGE("no current audio output");
        }

        if ((mPlayer != NULL) && (mNextClient->getPlayer() != NULL)) {
            mPlayer->setNextPlayer(mNextClient->getPlayer());
        }
    }

    return OK;
}

status_t MediaPlayerService::Client::seekTo(int msec)
{
    ALOGV("[%d] seekTo(%d)", mConnId, msec);
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) return UNKNOWN_ERROR;
    return p->seekTo(msec);
}

status_t MediaPlayerService::Client::reset()
{
    ALOGV("[%d] reset", mConnId);
    mRetransmitEndpointValid = false;
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) return UNKNOWN_ERROR;
    return p->reset();
}

status_t MediaPlayerService::Client::setAudioStreamType(audio_stream_type_t type)
{
    ALOGV("[%d] setAudioStreamType(%d)", mConnId, type);
    // TODO: for hardware output, call player instead
    Mutex::Autolock l(mLock);
    if (mAudioOutput != 0) mAudioOutput->setAudioStreamType(type);
    return NO_ERROR;
}

status_t MediaPlayerService::Client::setLooping(int loop)
{
    ALOGV("[%d] setLooping(%d)", mConnId, loop);
    mLoop = loop;
    sp<MediaPlayerBase> p = getPlayer();
    if (p != 0) return p->setLooping(loop);
    return NO_ERROR;
}

status_t MediaPlayerService::Client::setVolume(float leftVolume, float rightVolume)
{
    ALOGV("[%d] setVolume(%f, %f)", mConnId, leftVolume, rightVolume);

    // for hardware output, call player instead
    sp<MediaPlayerBase> p = getPlayer();
    {
      Mutex::Autolock l(mLock);
      if (p != 0 && p->hardwareOutput()) {
          MediaPlayerHWInterface* hwp =
                  reinterpret_cast<MediaPlayerHWInterface*>(p.get());
          return hwp->setVolume(leftVolume, rightVolume);
      } else {
          if (mAudioOutput != 0) mAudioOutput->setVolume(leftVolume, rightVolume);
          return NO_ERROR;
      }
    }

    return NO_ERROR;
}

status_t MediaPlayerService::Client::setAuxEffectSendLevel(float level)
{
    ALOGV("[%d] setAuxEffectSendLevel(%f)", mConnId, level);
    Mutex::Autolock l(mLock);
    if (mAudioOutput != 0) return mAudioOutput->setAuxEffectSendLevel(level);
    return NO_ERROR;
}

status_t MediaPlayerService::Client::attachAuxEffect(int effectId)
{
    ALOGV("[%d] attachAuxEffect(%d)", mConnId, effectId);
    Mutex::Autolock l(mLock);
    if (mAudioOutput != 0) return mAudioOutput->attachAuxEffect(effectId);
    return NO_ERROR;
}

status_t MediaPlayerService::Client::setParameter(int key, const Parcel &request) {
    ALOGV("[%d] setParameter(%d)", mConnId, key);
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) return UNKNOWN_ERROR;
    return p->setParameter(key, request);
}

status_t MediaPlayerService::Client::getParameter(int key, Parcel *reply) {
    ALOGV("[%d] getParameter(%d)", mConnId, key);
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) return UNKNOWN_ERROR;
    return p->getParameter(key, reply);
}

/* add by Gary. start {{----------------------------------- */
//status_t MediaPlayerService::setScreen(int screen)
//{
//    ALOGV("setScreen(%d)", screen);
//    if( screen != MASTER_SCREEN && screen != SLAVE_SCREEN )
//        return BAD_VALUE;
//    if( screen == mScreen )
//        return OK;
//        
//    status_t ret = OK;
//    for (int i = 0, n = mClients.size(); i < n; ++i) {
//        sp<Client> c = mClients[i].promote();
//        if (c != 0) {
//            status_t temp = c->setScreen(screen);
//            if( temp != OK )
//                ret = temp;
//        }
//    }
//    
//    mScreen = screen;
//    
//    if(mScreen == MASTER_SCREEN)
//        property_set(PROP_SCREEN_KEY, PROP_MASTER_SCREEN);
//    else
//        property_set(PROP_SCREEN_KEY, PROP_SLAVE_SCREEN);
//    char prop_value[PROPERTY_VALUE_MAX];
//    property_get(PROP_SCREEN_KEY, prop_value, "no screen");
//    ALOGV("prop_value = %s", prop_value);
//        
//    return ret;
//}
//
//status_t MediaPlayerService::getScreen(int *screen)
//{
//    ALOGV("get Screen");
//    if( screen == NULL )
//        return BAD_VALUE;
//        
//    *screen = mScreen;
//    return OK;
//}
//
//status_t MediaPlayerService::Client::setScreen(int screen)
//{
//    ALOGV("[%d] setScreen(%d)", mConnId, screen);
//    mScreen = screen;
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return UNKNOWN_ERROR;
//    return p->setScreen(screen);
//}

status_t MediaPlayerService::isPlayingVideo(int *playing)
{
    status_t ret = OK;
    *playing = 0;
    for (int i = 0, n = mClients.size(); i < n; ++i) {
        sp<Client> c = mClients[i].promote();
        if (c != 0) {
            c->isPlayingVideo(playing);
            if( *playing == 1 )
                return OK;
        }
    }
    
    return ret;
}

status_t MediaPlayerService::Client::isPlayingVideo(int *playing)
{
	ALOGV("Client::isPlayingVideo");
	sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) {
    	*playing = 0;
        return OK;
    }

    *playing =  (p->getMeidaPlayerState() != PLAYER_STATE_SUSPEND);

    if (*playing) {
    	generalInterface(MEDIAPLAYER_CMD_IS_PLAYINGVIDEO, 0, 0, 0, playing);
    }

    ALOGV("isPlayingVideo:%d",*playing);

    return OK;
}

/* add by Gary. end   -----------------------------------}} */

/* add by Gary. start {{----------------------------------- */
/* 2011-9-15 15:41:36 */
/* expend interfaces about subtitle, track and so on */
//int MediaPlayerService::Client::getSubCount()
//{
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return -1;
//    return p->getSubCount();
//}
//
//
//int MediaPlayerService::Client::getSubList(MediaPlayer_SubInfo *infoList, int count)
//{
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return -1;
//    return p->getSubList((MediaPlayer_SubInfo *)infoList, count);
//}
//
//int MediaPlayerService::Client::getCurSub()
//{
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return -1;
//    return p->getCurSub();
//}
//
//status_t MediaPlayerService::Client::switchSub(int index)
//{
//    mSubIndex = index;
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return UNKNOWN_ERROR;
//    return p->switchSub(index);
//}
//
//status_t MediaPlayerService::Client::setSubGate(bool showSub)
//{
//    ALOGV("MediaPlayerService::Client::setSubGate(): showSub = %d", showSub);
//    mSubGate = showSub;
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return OK;
//    return p->setSubGate(showSub);
//}
//
//bool MediaPlayerService::Client::getSubGate()
//{
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return true;
//    return p->getSubGate();
//}
//
//status_t MediaPlayerService::Client::setSubColor(int color)
//{
//    mSubColor = color;
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return OK;
//    return p->setSubColor(color);
//}
//
//int MediaPlayerService::Client::getSubColor()
//{
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return 0xFFFFFFFF;
//    return p->getSubColor();
//}
//
//status_t MediaPlayerService::Client::setSubFrameColor(int color)
//{
//    mSubFrameColor = color;
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return OK;
//    return p->setSubFrameColor(color);
//}
//
//int MediaPlayerService::Client::getSubFrameColor()
//{
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return 0xFFFFFFFF;
//    return p->getSubFrameColor();
//}
//
//status_t MediaPlayerService::Client::setSubFontSize(int size)
//{
//    mSubFontSize = size;
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return OK;
//    return p->setSubFontSize(size);
//}
//
//int MediaPlayerService::Client::getSubFontSize()
//{
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return -1;
//    return p->getSubFontSize();
//}

status_t MediaPlayerService::Client::setSubCharset(const char *charset)
{
    strcpy(mSubCharset, charset);
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return OK;
    return p->setSubCharset(charset);
}

status_t MediaPlayerService::Client::getSubCharset(char *charset)
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return UNKNOWN_ERROR;
    return p->getSubCharset(charset);
}

//status_t MediaPlayerService::Client::setSubPosition(int percent)
//{
//    mSubPosition = percent;
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return OK;
//    return p->setSubPosition(percent);
//}
//
//int MediaPlayerService::Client::getSubPosition()
//{
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return -1;
//    return p->getSubPosition();
//}

status_t MediaPlayerService::Client::setSubDelay(int time)
{
    mSubDelay = time;
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return OK;
    return p->setSubDelay(time);
}

int MediaPlayerService::Client::getSubDelay()
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return 0;
    return p->getSubDelay();
}

//int MediaPlayerService::Client::getTrackCount()
//{
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return -1;
//    return p->getTrackCount();
//}
//
//int MediaPlayerService::Client::getTrackList(MediaPlayer_TrackInfo *infoList, int count)
//{
//    sp<MediaPlayerBase> p = getPlayer();
//
//    if (p == 0) 
//        return -1;
//    return p->getTrackList((MediaPlayer_TrackInfo *)infoList, count);
//}
//
//int MediaPlayerService::Client::getCurTrack()
//{
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return -1;
//    return p->getCurTrack();
//}
//
//status_t MediaPlayerService::Client::switchTrack(int index)
//{
//    mTrackIndex = index;
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return UNKNOWN_ERROR;
//    return p->switchTrack(index);
//}
//
//status_t MediaPlayerService::Client::setInputDimensionType(int type)
//{
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return UNKNOWN_ERROR;
//    return p->setInputDimensionType(type);
//}
//
//int MediaPlayerService::Client::getInputDimensionType()
//{
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return -1;
//    return p->getInputDimensionType();
//}
//
//status_t MediaPlayerService::Client::setOutputDimensionType(int type)
//{
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return UNKNOWN_ERROR;
//    return p->setOutputDimensionType(type);
//}
//
//int MediaPlayerService::Client::getOutputDimensionType()
//{
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return -1;
//    return p->getOutputDimensionType();
//}
//
//status_t MediaPlayerService::Client::setAnaglaghType(int type)
//{
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return UNKNOWN_ERROR;
//    return p->setAnaglaghType(type);
//}
//
//int MediaPlayerService::Client::getAnaglaghType()
//{
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return -1;
//    return p->getAnaglaghType();
//}
//
//status_t MediaPlayerService::Client::getVideoEncode(char *encode)
//{
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0)
//        return UNKNOWN_ERROR;
//    return p->getVideoEncode(encode);
//}
//
//int MediaPlayerService::Client::getVideoFrameRate()
//{
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0)
//        return UNKNOWN_ERROR;
//    return p->getVideoFrameRate();
//}
//
//status_t MediaPlayerService::Client::getAudioEncode(char *encode)
//{
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0)
//        return UNKNOWN_ERROR;
//    return p->getAudioEncode(encode);
//}
//
//int MediaPlayerService::Client::getAudioBitRate()
//{
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0)
//        return UNKNOWN_ERROR;
//    return p->getAudioBitRate();
//}
//
//int MediaPlayerService::Client::getAudioSampleRate()
//{
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0)
//        return UNKNOWN_ERROR;
//    return p->getAudioSampleRate();
//}

/* add by Gary. end   -----------------------------------}} */


/* add by Gary. start {{----------------------------------- */
/* 2011-11-14 */
/* support scale mode */
status_t MediaPlayerService::Client::enableScaleMode(bool enable, int width, int height)
{
    mEnableScaleMode = enable;
    mScaleWidth = width;
    mScaleHeight = height;
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return UNKNOWN_ERROR;
    return p->enableScaleMode(enable, width, height);
}
/* add by Gary. end   -----------------------------------}} */

/* add by Gary. start {{----------------------------------- */
/* 2011-11-14 */
/* support adjusting colors while playing video */
//status_t MediaPlayerService::setVppGate(bool enableVpp)
//{
//    if( enableVpp == mVppGate )
//        return OK;
//        
//    status_t ret = OK;
//    for (int i = 0, n = mClients.size(); i < n; ++i) {
//        sp<Client> c = mClients[i].promote();
//        if (c != 0) {
//            status_t temp = c->setVppGate(enableVpp);
//            if( temp != OK )
//                ret = temp;
//        }
//    }
//    
//    mVppGate = enableVpp;
//    
//    if(mVppGate)
//        property_set(PROP_VPP_GATE_KEY, PROP_ENABLE_VPP);
//    else
//        property_set(PROP_VPP_GATE_KEY, PROP_DISABLE_VPP);
//    char prop_value[PROPERTY_VALUE_MAX];
//    property_get(PROP_VPP_GATE_KEY, prop_value, "no enableVpp");
//    ALOGV("prop_value of PROP_VPP_GATE_KEY = %s", prop_value);
//        
//    return ret;
//}
//
//bool MediaPlayerService::getVppGate()
//{
//    return mVppGate;
//}
//
//status_t MediaPlayerService::Client::setVppGate(bool enableVpp)
//{
//    mVppGate = enableVpp;
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return UNKNOWN_ERROR;
//    return p->setVppGate(enableVpp);
//}
//
//status_t MediaPlayerService::setLumaSharp(int value)
//{
//    if(value < 0)
//        return BAD_VALUE;
//        
//    value %= 5;
//    
//    if( value == mLumaSharp )
//        return OK;
//        
//    status_t ret = OK;
//    for (int i = 0, n = mClients.size(); i < n; ++i) {
//        sp<Client> c = mClients[i].promote();
//        if (c != 0) {
//            status_t temp = c->setLumaSharp(value);
//            if( temp != OK )
//                ret = temp;
//        }
//    }
//    
//    mLumaSharp = value;
//    
//    char prop_value[2] = "\0";
//    prop_value[0] = "01234"[value];    
//    property_set(PROP_LUMA_SHARP_KEY, prop_value);
//    
//    char prop_value2[PROPERTY_VALUE_MAX];
//    property_get(PROP_LUMA_SHARP_KEY, prop_value2, "no proper LUMA_SHARP");
//    ALOGV("prop_value of PROP_LUMA_SHARP_KEY = %s", prop_value2);
//        
//    return ret;
//}
//
//int MediaPlayerService::getLumaSharp()
//{
//    return mLumaSharp;
//}
//
//status_t MediaPlayerService::Client::setLumaSharp(int value)
//{
//    mLumaSharp = value;
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return UNKNOWN_ERROR;
//    return p->setLumaSharp(value);
//}
//
//status_t MediaPlayerService::setChromaSharp(int value)
//{
//    if(value < 0)
//        return BAD_VALUE;
//        
//    value %= 5;
//    
//    if( value == mChromaSharp )
//        return OK;
//        
//    status_t ret = OK;
//    for (int i = 0, n = mClients.size(); i < n; ++i) {
//        sp<Client> c = mClients[i].promote();
//        if (c != 0) {
//            status_t temp = c->setChromaSharp(value);
//            if( temp != OK )
//                ret = temp;
//        }
//    }
//    
//    mChromaSharp = value;
//    
//    char prop_value[2] = "\0";
//    prop_value[0] = "01234"[value];    
//    property_set(PROP_CHROMA_SHARP_KEY, prop_value);
//    
//    char prop_value2[PROPERTY_VALUE_MAX];
//    property_get(PROP_CHROMA_SHARP_KEY, prop_value2, "no proper CHROMA_SHARP");
//    ALOGV("prop_value of PROP_CHROMA_SHARP_KEY = %s", prop_value2);
//        
//    return ret;
//}
//
//int MediaPlayerService::getChromaSharp()
//{
//    return mChromaSharp;
//}
//
//status_t MediaPlayerService::Client::setChromaSharp(int value)
//{
//    mChromaSharp = value;
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return UNKNOWN_ERROR;
//    return p->setChromaSharp(value);
//}
//
//status_t MediaPlayerService::setWhiteExtend(int value)
//{
//    if(value < 0)
//        return BAD_VALUE;
//        
//    value %= 5;
//    
//    if( value == mWhiteExtend )
//        return OK;
//        
//    status_t ret = OK;
//    for (int i = 0, n = mClients.size(); i < n; ++i) {
//        sp<Client> c = mClients[i].promote();
//        if (c != 0) {
//            status_t temp = c->setWhiteExtend(value);
//            if( temp != OK )
//                ret = temp;
//        }
//    }
//    
//    mWhiteExtend = value;
//    
//    char prop_value[2] = "\0";
//    prop_value[0] = "01234"[value];    
//    property_set(PROP_WHITE_EXTEND_KEY, prop_value);
//    
//    char prop_value2[PROPERTY_VALUE_MAX];
//    property_get(PROP_WHITE_EXTEND_KEY, prop_value2, "no proper WHITE_EXTEND");
//    ALOGV("prop_value of PROP_WHITE_EXTEND_KEY = %s", prop_value2);
//        
//    return ret;
//}
//
//int MediaPlayerService::getWhiteExtend()
//{
//    return mWhiteExtend;
//}
//
//status_t MediaPlayerService::Client::setWhiteExtend(int value)
//{
//    mWhiteExtend = value;
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return UNKNOWN_ERROR;
//    return p->setWhiteExtend(value);
//}
//
//status_t MediaPlayerService::setBlackExtend(int value)
//{
//    if(value < 0)
//        return BAD_VALUE;
//        
//    value %= 5;
//    
//    if( value == mBlackExtend )
//        return OK;
//        
//    status_t ret = OK;
//    for (int i = 0, n = mClients.size(); i < n; ++i) {
//        sp<Client> c = mClients[i].promote();
//        if (c != 0) {
//            status_t temp = c->setBlackExtend(value);
//            if( temp != OK )
//                ret = temp;
//        }
//    }
//    
//    mBlackExtend = value;
//    
//    char prop_value[2] = "\0";
//    prop_value[0] = "01234"[value];
//    property_set(PROP_BLACK_EXTEND_KEY, prop_value);
//    
//    char prop_value2[PROPERTY_VALUE_MAX];
//    property_get(PROP_BLACK_EXTEND_KEY, prop_value2, "no proper BLACK_EXTEND");
//    ALOGV("prop_value of PROP_BLACK_EXTEND_KEY = %s", prop_value2);
//        
//    return ret;
//}
//
//int MediaPlayerService::getBlackExtend()
//{
//    return mBlackExtend;
//}
//
//status_t MediaPlayerService::Client::setBlackExtend(int value)
//{
//    mBlackExtend = value;
//    sp<MediaPlayerBase> p = getPlayer();
//    if (p == 0) 
//        return UNKNOWN_ERROR;
//    return p->setBlackExtend(value);
//}

/* add by Gary. end   -----------------------------------}} */

/* add by Gary. start {{----------------------------------- */
/* 2012-03-07 */
/* set audio channel mute */
status_t MediaPlayerService::Client::setChannelMuteMode(int muteMode)
{
    mMuteMode = muteMode;
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return OK;
    return p->setChannelMuteMode(muteMode);
}

int MediaPlayerService::Client::getChannelMuteMode()
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return -1;
    return p->getChannelMuteMode();
}
/* add by Gary. end   -----------------------------------}} */

/* add by Gary. start {{----------------------------------- */
/* 2012-03-12 */
/* add the global interfaces to control the subtitle gate  */
//status_t MediaPlayerService::setGlobalSubGate(bool showSub)
//{
//    ALOGV("MediaPlayerService::setGlobalSubGate(): enable = %d", showSub);
//    if( showSub == mGlobalSubGate )
//        return OK;
//        
//    status_t ret = OK;
//    for (int i = 0, n = mClients.size(); i < n; ++i) {
//        sp<Client> c = mClients[i].promote();
//        if (c != 0) {
//            status_t temp = c->setSubGate(showSub);
//            if( temp != OK )
//                ret = temp;
//        }
//    }
//    
//    mGlobalSubGate = showSub;
//    
//    if(mGlobalSubGate)
//        property_set(PROP_GLOBAL_SUB_GATE_KEY, PROP_ENABLE_GLOBAL_SUB);
//    else
//        property_set(PROP_GLOBAL_SUB_GATE_KEY, PROP_DISABLE_GLOBAL_SUB);
//    char prop_value[PROPERTY_VALUE_MAX];
//    property_get(PROP_GLOBAL_SUB_GATE_KEY, prop_value, "no showSub");
//
//    return ret;
//}
//
//bool MediaPlayerService::getGlobalSubGate()
//{
//    return mGlobalSubGate;
//}
/* add by Gary. end   -----------------------------------}} */

/* add by Gary. start {{----------------------------------- */
/* 2012-4-24 */
/* add two general interfaces for expansibility */
//status_t MediaPlayerService::generalGlobalInterface(int cmd, int int1, int int2, int int3, void *p)
//{
//    switch(cmd){
//        case MEDIAPLAYER_GLOBAL_CMD_TEST:{
//            ALOGD("MEDIAPLAYER_GLOBAL_CMD_TEST: int1 = %d", int1);
//            *((int *)p) = 111;
//            ALOGD("*p = %d", *((int *)p));
//        }break;
//		case MEDIAPLAYER_CMD_IS_ROTATABLE:{
//			ALOGD("MEDIAPLAYER_CMD_IS_ROTATABLE...");
//		}break;
//		case MEDIAPLAYER_CMD_SET_ROTATION:{
//			status_t ret = OK;
//            if(1 == mHdmiPlugged)
//            {
//                ALOGD("(f:%s, l:%d)hdmi state is [%d], don't tell vdeclib rotate!", __FUNCTION__, __LINE__, mHdmiPlugged);
//                return ret;
//            }
//			for (int i = 0, n = mClients.size(); i < n; ++i) {
//				sp<Client> c = mClients[i].promote();
//				if (c != 0) {
//					status_t temp = c->generalInterface(MEDIAPLAYER_CMD_SET_ROTATION, int1, int2, int3, p);
//					if( temp != OK )
//						ret = temp;
//				}
//			}
//			return ret;
//		}break;
//        case MEDIAPLAYER_CMD_SET_HDMISTATE:{
//            ALOGD("(f:%s, l:%d)hdmi state is [%d]", __FUNCTION__, __LINE__, int1);
//            mHdmiPlugged = int1;
//        }break;
//        default:{
//            ALOGW("cmd %d is NOT defined.", cmd);
//        }break;
//    }
//    return OK;
//}
/* add by Gary. end   -----------------------------------}} */


status_t MediaPlayerService::Client::generalInterface(int cmd, int int1, int int2, int int3, void *p)
{
    sp<MediaPlayerBase> mp = getPlayer();
    if (mp == 0) 
        return UNKNOWN_ERROR;

    ALOGD("generalInterface cmd=%d int1=%d int2=%d",cmd, int1, int2);
    if(cmd == MEDIAPLAYER_CMD_RELEASE_SURFACE_BYHAND) {
        disconnectNativeWindow();
        IPCThreadState::self()->flushCommands();
	    ALOGD("RELEASE_SURFACE_BYHAND not implement now");
        return OK;
    }

    return mp->generalInterface(cmd, int1, int2, int3, p);
}

status_t MediaPlayerService::Client::setRetransmitEndpoint(
        const struct sockaddr_in* endpoint) {

    if (NULL != endpoint) {
        uint32_t a = ntohl(endpoint->sin_addr.s_addr);
        uint16_t p = ntohs(endpoint->sin_port);
        ALOGV("[%d] setRetransmitEndpoint(%u.%u.%u.%u:%hu)", mConnId,
                (a >> 24), (a >> 16) & 0xFF, (a >> 8) & 0xFF, (a & 0xFF), p);
    } else {
        ALOGV("[%d] setRetransmitEndpoint = <none>", mConnId);
    }

    sp<MediaPlayerBase> p = getPlayer();

    // Right now, the only valid time to set a retransmit endpoint is before
    // player selection has been made (since the presence or absence of a
    // retransmit endpoint is going to determine which player is selected during
    // setDataSource).
    if (p != 0) return INVALID_OPERATION;

    if (NULL != endpoint) {
        mRetransmitEndpoint = *endpoint;
        mRetransmitEndpointValid = true;
    } else {
        mRetransmitEndpointValid = false;
    }

    return NO_ERROR;
}

status_t MediaPlayerService::Client::getRetransmitEndpoint(
        struct sockaddr_in* endpoint)
{
    if (NULL == endpoint)
        return BAD_VALUE;

    sp<MediaPlayerBase> p = getPlayer();

    if (p != NULL)
        return p->getRetransmitEndpoint(endpoint);

    if (!mRetransmitEndpointValid)
        return NO_INIT;

    *endpoint = mRetransmitEndpoint;

    return NO_ERROR;
}

void MediaPlayerService::Client::notify(
        void* cookie, int msg, int ext1, int ext2, const Parcel *obj)
{
    Client* client = static_cast<Client*>(cookie);
    if (client == NULL) {
        return;
    }

    sp<IMediaPlayerClient> c;
    {
        Mutex::Autolock l(client->mLock);
        c = client->mClient;
        if (msg == MEDIA_PLAYBACK_COMPLETE && client->mNextClient != NULL) {
            if (client->mNextClient != NULL) {
				client->mMsg = MEDIA_NOP;
                
                if (client->mAudioOutput != NULL)
                    client->mAudioOutput->switchToNextOutput();
                client->mNextClient->start();
                client->mNextClient->mClient->notify(MEDIA_INFO, MEDIA_INFO_STARTED_AS_NEXT, 0, obj);
            }
            else
			{
				client->mMsg = MEDIA_PLAYBACK_COMPLETE;
			}
        }
    }

    if (MEDIA_INFO == msg &&
        MEDIA_INFO_METADATA_UPDATE == ext1) {
        const media::Metadata::Type metadata_type = ext2;

        if(client->shouldDropMetadata(metadata_type)) {
            return;
        }

        // Update the list of metadata that have changed. getMetadata
        // also access mMetadataUpdated and clears it.
        client->addNewMetadataUpdate(metadata_type);
    }

    if (c != NULL) {
        ALOGV("[%d] notify (%p, %d, %d, %d)", client->mConnId, cookie, msg, ext1, ext2);
        c->notify(msg, ext1, ext2, obj);
    }
}


bool MediaPlayerService::Client::shouldDropMetadata(media::Metadata::Type code) const
{
    Mutex::Autolock lock(mLock);

    if (findMetadata(mMetadataDrop, code)) {
        return true;
    }

    if (mMetadataAllow.isEmpty() || findMetadata(mMetadataAllow, code)) {
        return false;
    } else {
        return true;
    }
}


void MediaPlayerService::Client::addNewMetadataUpdate(media::Metadata::Type metadata_type) {
    Mutex::Autolock lock(mLock);
    if (mMetadataUpdated.indexOf(metadata_type) < 0) {
        mMetadataUpdated.add(metadata_type);
    }
}

#if CALLBACK_ANTAGONIZER
const int Antagonizer::interval = 10000; // 10 msecs

Antagonizer::Antagonizer(notify_callback_f cb, void* client) :
    mExit(false), mActive(false), mClient(client), mCb(cb)
{
    createThread(callbackThread, this);
}

void Antagonizer::kill()
{
    Mutex::Autolock _l(mLock);
    mActive = false;
    mExit = true;
    mCondition.wait(mLock);
}

int Antagonizer::callbackThread(void* user)
{
    ALOGD("Antagonizer started");
    Antagonizer* p = reinterpret_cast<Antagonizer*>(user);
    while (!p->mExit) {
        if (p->mActive) {
            ALOGV("send event");
            p->mCb(p->mClient, 0, 0, 0);
        }
        usleep(interval);
    }
    Mutex::Autolock _l(p->mLock);
    p->mCondition.signal();
    ALOGD("Antagonizer stopped");
    return 0;
}
#endif

static size_t kDefaultHeapSize = 1024 * 1024; // 1MB

sp<IMemory> MediaPlayerService::decode(const char* url, uint32_t *pSampleRate, int* pNumChannels, audio_format_t* pFormat)
{
    ALOGV("decode(%s)", url);
    sp<MemoryBase> mem;
    sp<MediaPlayerBase> player;

    // Protect our precious, precious DRMd ringtones by only allowing
    // decoding of http, but not filesystem paths or content Uris.
    // If the application wants to decode those, it should open a
    // filedescriptor for them and use that.
    if (url != NULL && strncmp(url, "http://", 7) != 0) {
        ALOGD("Can't decode %s by path, use filedescriptor instead", url);
        return mem;
    }

    player_type playerType =
        MediaPlayerFactory::getPlayerType(NULL /* client */, url);
    ALOGV("player type = %d", playerType);

    // create the right type of player
    sp<AudioCache> cache = new AudioCache(url);
    player = MediaPlayerFactory::createPlayer(playerType, cache.get(), cache->notify);
    if (player == NULL) goto Exit;
    if (player->hardwareOutput()) goto Exit;

    static_cast<MediaPlayerInterface*>(player.get())->setAudioSink(cache);

    // set data source
    if (player->setDataSource(url) != NO_ERROR) goto Exit;

    ALOGV("prepare");
    player->prepareAsync();

    ALOGV("wait for prepare");
    if (cache->wait() != NO_ERROR) goto Exit;

    ALOGV("start");
    player->start();

    ALOGV("wait for playback complete");
    cache->wait();
    // in case of error, return what was successfully decoded.
    if (cache->size() == 0) {
        goto Exit;
    }

    mem = new MemoryBase(cache->getHeap(), 0, cache->size());
    *pSampleRate = cache->sampleRate();
    *pNumChannels = cache->channelCount();
    *pFormat = cache->format();
    ALOGV("return memory @ %p, sampleRate=%u, channelCount = %d, format = %d", mem->pointer(), *pSampleRate, *pNumChannels, *pFormat);

Exit:
    if (player != 0) player->reset();
    return mem;
}

sp<IMemory> MediaPlayerService::decode(int fd, int64_t offset, int64_t length, uint32_t *pSampleRate, int* pNumChannels, audio_format_t* pFormat)
{
    ALOGV("decode(%d, %lld, %lld)", fd, offset, length);
    sp<MemoryBase> mem;
    sp<MediaPlayerBase> player;

    player_type playerType = MediaPlayerFactory::getPlayerType(NULL /* client */,
                                                               fd,
                                                               offset,
                                                               length,
                                                               false);
    ALOGV("player type = %d", playerType);

    // create the right type of player
    sp<AudioCache> cache = new AudioCache("decode_fd");
    player = MediaPlayerFactory::createPlayer(playerType, cache.get(), cache->notify);
    if (player == NULL) goto Exit;
    if (player->hardwareOutput()) goto Exit;

    static_cast<MediaPlayerInterface*>(player.get())->setAudioSink(cache);

    // set data source
    if (player->setDataSource(fd, offset, length) != NO_ERROR) goto Exit;

    ALOGV("prepare");
    player->prepareAsync();

    ALOGV("wait for prepare");
    if (cache->wait() != NO_ERROR) goto Exit;

    ALOGV("start");
    player->start();

    ALOGV("wait for playback complete");
    cache->wait();
    // in case of error, return what was successfully decoded.
    if (cache->size() == 0) {
        goto Exit;
    }

    mem = new MemoryBase(cache->getHeap(), 0, cache->size());
    *pSampleRate = cache->sampleRate();
    *pNumChannels = cache->channelCount();
    *pFormat = cache->format();
    ALOGV("return memory @ %p, sampleRate=%u, channelCount = %d, format = %d", mem->pointer(), *pSampleRate, *pNumChannels, *pFormat);

Exit:
    if (player != 0) player->reset();
    ::close(fd);
    return mem;
}


#undef LOG_TAG
#define LOG_TAG "AudioSink"
MediaPlayerService::AudioOutput::AudioOutput(int sessionId)
    : mCallback(NULL),
      mCallbackCookie(NULL),
      mCallbackData(NULL),
      mBytesWritten(0),
      mSessionId(sessionId),
      mFlags(AUDIO_OUTPUT_FLAG_NONE) {
    ALOGV("AudioOutput(%d)", sessionId);
    mTrack = 0;
    mRecycledTrack = 0;
    mStreamType = AUDIO_STREAM_MUSIC;
    mLeftVolume = 1.0;
    mRightVolume = 1.0;
    mPlaybackRatePermille = 1000;
    mSampleRateHz = 0;
    mMsecsPerFrame = 0;
    mAuxEffectId = 0;
    mSendLevel = 0.0;
    setMinBufferCount();
}

MediaPlayerService::AudioOutput::~AudioOutput()
{
    close();
    delete mRecycledTrack;
    delete mCallbackData;
}

void MediaPlayerService::AudioOutput::setMinBufferCount()
{
    char value[PROPERTY_VALUE_MAX];
    if (property_get("ro.kernel.qemu", value, 0)) {
        mIsOnEmulator = true;
        mMinBufferCount = 12;  // to prevent systematic buffer underrun for emulator
    }
}

bool MediaPlayerService::AudioOutput::isOnEmulator()
{
    setMinBufferCount();
    return mIsOnEmulator;
}

int MediaPlayerService::AudioOutput::getMinBufferCount()
{
    setMinBufferCount();
    return mMinBufferCount;
}

ssize_t MediaPlayerService::AudioOutput::bufferSize() const
{
    if (mTrack == 0) return NO_INIT;
    return mTrack->frameCount() * frameSize();
}

ssize_t MediaPlayerService::AudioOutput::frameCount() const
{
    if (mTrack == 0) return NO_INIT;
    return mTrack->frameCount();
}

ssize_t MediaPlayerService::AudioOutput::channelCount() const
{
    if (mTrack == 0) return NO_INIT;
    return mTrack->channelCount();
}

ssize_t MediaPlayerService::AudioOutput::frameSize() const
{
    if (mTrack == 0) return NO_INIT;
    return mTrack->frameSize();
}

uint32_t MediaPlayerService::AudioOutput::latency () const
{
    if (mTrack == 0) return 0;
    return mTrack->latency();
}

float MediaPlayerService::AudioOutput::msecsPerFrame() const
{
    return mMsecsPerFrame;
}

status_t MediaPlayerService::AudioOutput::getPosition(uint32_t *position) const
{
    if (mTrack == 0) return NO_INIT;
    return mTrack->getPosition(position);
}

status_t MediaPlayerService::AudioOutput::getFramesWritten(uint32_t *frameswritten) const
{
    if (mTrack == 0) return NO_INIT;
    *frameswritten = mBytesWritten / frameSize();
    return OK;
}

status_t MediaPlayerService::AudioOutput::open(
        uint32_t sampleRate, int channelCount, audio_channel_mask_t channelMask,
        audio_format_t format, int bufferCount,
        AudioCallback cb, void *cookie,
        audio_output_flags_t flags)
{
    mCallback = cb;
    mCallbackCookie = cookie;

    // Check argument "bufferCount" against the mininum buffer count
    if (bufferCount < mMinBufferCount) {
        ALOGD("bufferCount (%d) is too small and increased to %d", bufferCount, mMinBufferCount);
        bufferCount = mMinBufferCount;

    }
    ALOGV("open(%u, %d, 0x%x, %d, %d, %d)", sampleRate, channelCount, channelMask,
            format, bufferCount, mSessionId);
    int afSampleRate;
    int afFrameCount;
    uint32_t frameCount;

    if (AudioSystem::getOutputFrameCount(&afFrameCount, mStreamType) != NO_ERROR) {
        return NO_INIT;
    }
    if (AudioSystem::getOutputSamplingRate(&afSampleRate, mStreamType) != NO_ERROR) {
        return NO_INIT;
    }

    frameCount = (sampleRate*afFrameCount*bufferCount)/afSampleRate;

    if (channelMask == CHANNEL_MASK_USE_CHANNEL_ORDER) {
        channelMask = audio_channel_out_mask_from_count(channelCount);
        if (0 == channelMask) {
            ALOGE("open() error, can\'t derive mask for %d audio channels", channelCount);
            return NO_INIT;
        }
    }

    AudioTrack *t;
    CallbackData *newcbd = NULL;
    if (mCallback != NULL) {
        newcbd = new CallbackData(this);
        t = new AudioTrack(
                mStreamType,
                sampleRate,
                format,
                channelMask,
                frameCount,
                flags,
                CallbackWrapper,
                newcbd,
                0,  // notification frames
                mSessionId);
    } else {
        t = new AudioTrack(
                mStreamType,
                sampleRate,
                format,
                channelMask,
                frameCount,
                flags,
                NULL,
                NULL,
                0,
                mSessionId);
    }

    if ((t == 0) || (t->initCheck() != NO_ERROR)) {
        ALOGE("Unable to create audio track");
        delete t;
        delete newcbd;
        return NO_INIT;
    }


    if (mRecycledTrack) {
        // check if the existing track can be reused as-is, or if a new track needs to be created.

        bool reuse = true;
        if ((mCallbackData == NULL && mCallback != NULL) ||
                (mCallbackData != NULL && mCallback == NULL)) {
            // recycled track uses callbacks but the caller wants to use writes, or vice versa
            ALOGV("can't chain callback and write");
            reuse = false;
        } else if ((mRecycledTrack->getSampleRate() != sampleRate) ||
                (mRecycledTrack->channelCount() != channelCount) ||
                (mRecycledTrack->frameCount() != t->frameCount())) {
            ALOGV("samplerate, channelcount or framecount differ: %d/%d Hz, %d/%d ch, %d/%d frames",
                  mRecycledTrack->getSampleRate(), sampleRate,
                  mRecycledTrack->channelCount(), channelCount,
                  mRecycledTrack->frameCount(), t->frameCount());
            reuse = false;
        } else if (flags != mFlags) {
            ALOGV("output flags differ %08x/%08x", flags, mFlags);
            reuse = false;
        }
        if (reuse) {
            ALOGV("chaining to next output");
            close();
            mTrack = mRecycledTrack;
            mRecycledTrack = NULL;
            if (mCallbackData != NULL) {
                mCallbackData->setOutput(this);
            }
            delete t;
            delete newcbd;
            return OK;
        }

        // if we're not going to reuse the track, unblock and flush it
        if (mCallbackData != NULL) {
            mCallbackData->setOutput(NULL);
            mCallbackData->endTrackSwitch();
        }
        mRecycledTrack->flush();
        delete mRecycledTrack;
        mRecycledTrack = NULL;
        delete mCallbackData;
        mCallbackData = NULL;
        close();
    }

    mCallbackData = newcbd;
    ALOGV("setVolume");
    t->setVolume(mLeftVolume, mRightVolume);

    mSampleRateHz = sampleRate;
    mFlags = flags;
    mMsecsPerFrame = mPlaybackRatePermille / (float) sampleRate;
    uint32_t pos;
    if (t->getPosition(&pos) == OK) {
        mBytesWritten = uint64_t(pos) * t->frameSize();
    }
    mTrack = t;

    status_t res = t->setSampleRate(mPlaybackRatePermille * mSampleRateHz / 1000);
    if (res != NO_ERROR) {
        return res;
    }
    t->setAuxEffectSendLevel(mSendLevel);
    return t->attachAuxEffect(mAuxEffectId);;
}

void MediaPlayerService::AudioOutput::start()
{
    ALOGV("start");
    if (mCallbackData != NULL) {
        mCallbackData->endTrackSwitch();
    }
    if (mTrack) {
        mTrack->setVolume(mLeftVolume, mRightVolume);
        mTrack->setAuxEffectSendLevel(mSendLevel);
        mTrack->start();
    }
}

void MediaPlayerService::AudioOutput::setNextOutput(const sp<AudioOutput>& nextOutput) {
    mNextOutput = nextOutput;
}


void MediaPlayerService::AudioOutput::switchToNextOutput() {
    ALOGV("switchToNextOutput");
    if (mNextOutput != NULL) {
        if (mCallbackData != NULL) {
            mCallbackData->beginTrackSwitch();
        }
        delete mNextOutput->mCallbackData;
        mNextOutput->mCallbackData = mCallbackData;
        mCallbackData = NULL;
        mNextOutput->mRecycledTrack = mTrack;
        mTrack = NULL;
        mNextOutput->mSampleRateHz = mSampleRateHz;
        mNextOutput->mMsecsPerFrame = mMsecsPerFrame;
        mNextOutput->mBytesWritten = mBytesWritten;
        mNextOutput->mFlags = mFlags;
    }
}

ssize_t MediaPlayerService::AudioOutput::write(const void* buffer, size_t size)
{
    LOG_FATAL_IF(mCallback != NULL, "Don't call write if supplying a callback.");

    //ALOGV("write(%p, %u)", buffer, size);
    if (mTrack) {
        ssize_t ret = mTrack->write(buffer, size);
        mBytesWritten += ret;
        return ret;
    }
    return NO_INIT;
}

void MediaPlayerService::AudioOutput::stop()
{
    ALOGV("stop");
    if (mTrack) mTrack->stop();
}

void MediaPlayerService::AudioOutput::flush()
{
    ALOGV("flush");
    if (mTrack) mTrack->flush();
}

void MediaPlayerService::AudioOutput::pause()
{
    ALOGV("pause");
    if (mTrack) mTrack->pause();
}

void MediaPlayerService::AudioOutput::close()
{
    ALOGV("close");
    delete mTrack;
    mTrack = 0;
}

void MediaPlayerService::AudioOutput::setVolume(float left, float right)
{
    ALOGV("setVolume(%f, %f)", left, right);
    mLeftVolume = left;
    mRightVolume = right;
    if (mTrack) {
        mTrack->setVolume(left, right);
    }
}

status_t MediaPlayerService::AudioOutput::setPlaybackRatePermille(int32_t ratePermille)
{
    ALOGV("setPlaybackRatePermille(%d)", ratePermille);
    status_t res = NO_ERROR;
    if (mTrack) {
        res = mTrack->setSampleRate(ratePermille * mSampleRateHz / 1000);
    } else {
        res = NO_INIT;
    }
    mPlaybackRatePermille = ratePermille;
    if (mSampleRateHz != 0) {
        mMsecsPerFrame = mPlaybackRatePermille / (float) mSampleRateHz;
    }
    return res;
}

status_t MediaPlayerService::AudioOutput::setAuxEffectSendLevel(float level)
{
    ALOGV("setAuxEffectSendLevel(%f)", level);
    mSendLevel = level;
    if (mTrack) {
        return mTrack->setAuxEffectSendLevel(level);
    }
    return NO_ERROR;
}

status_t MediaPlayerService::AudioOutput::attachAuxEffect(int effectId)
{
    ALOGV("attachAuxEffect(%d)", effectId);
    mAuxEffectId = effectId;
    if (mTrack) {
        return mTrack->attachAuxEffect(effectId);
    }
    return NO_ERROR;
}

// static
void MediaPlayerService::AudioOutput::CallbackWrapper(
        int event, void *cookie, void *info) {
    //ALOGV("callbackwrapper");
    if (event != AudioTrack::EVENT_MORE_DATA) {
        return;
    }

    CallbackData *data = (CallbackData*)cookie;
    data->lock();
    AudioOutput *me = data->getOutput();
    AudioTrack::Buffer *buffer = (AudioTrack::Buffer *)info;
    if (me == NULL) {
        // no output set, likely because the track was scheduled to be reused
        // by another player, but the format turned out to be incompatible.
        data->unlock();
        buffer->size = 0;
        return;
    }

    size_t actualSize = (*me->mCallback)(
            me, buffer->raw, buffer->size, me->mCallbackCookie);

    if (actualSize == 0 && buffer->size > 0 && me->mNextOutput == NULL) {
        // We've reached EOS but the audio track is not stopped yet,
        // keep playing silence.

        memset(buffer->raw, 0, buffer->size);
        actualSize = buffer->size;
    }

    buffer->size = actualSize;
    data->unlock();
}

int MediaPlayerService::AudioOutput::getSessionId() const
{
    return mSessionId;
}

#undef LOG_TAG
#define LOG_TAG "AudioCache"
MediaPlayerService::AudioCache::AudioCache(const char* name) :
    mChannelCount(0), mFrameCount(1024), mSampleRate(0), mSize(0),
    mError(NO_ERROR), mCommandComplete(false)
{
    // create ashmem heap
    mHeap = new MemoryHeapBase(kDefaultHeapSize, 0, name);
}

uint32_t MediaPlayerService::AudioCache::latency () const
{
    return 0;
}

float MediaPlayerService::AudioCache::msecsPerFrame() const
{
    return mMsecsPerFrame;
}

status_t MediaPlayerService::AudioCache::getPosition(uint32_t *position) const
{
    if (position == 0) return BAD_VALUE;
    *position = mSize;
    return NO_ERROR;
}

status_t MediaPlayerService::AudioCache::getFramesWritten(uint32_t *written) const
{
    if (written == 0) return BAD_VALUE;
    *written = mSize;
    return NO_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

struct CallbackThread : public Thread {
    CallbackThread(const wp<MediaPlayerBase::AudioSink> &sink,
                   MediaPlayerBase::AudioSink::AudioCallback cb,
                   void *cookie);

protected:
    virtual ~CallbackThread();

    virtual bool threadLoop();

private:
    wp<MediaPlayerBase::AudioSink> mSink;
    MediaPlayerBase::AudioSink::AudioCallback mCallback;
    void *mCookie;
    void *mBuffer;
    size_t mBufferSize;

    CallbackThread(const CallbackThread &);
    CallbackThread &operator=(const CallbackThread &);
};

CallbackThread::CallbackThread(
        const wp<MediaPlayerBase::AudioSink> &sink,
        MediaPlayerBase::AudioSink::AudioCallback cb,
        void *cookie)
    : mSink(sink),
      mCallback(cb),
      mCookie(cookie),
      mBuffer(NULL),
      mBufferSize(0) {
}

CallbackThread::~CallbackThread() {
    if (mBuffer) {
        free(mBuffer);
        mBuffer = NULL;
    }
}

bool CallbackThread::threadLoop() {
    sp<MediaPlayerBase::AudioSink> sink = mSink.promote();
    if (sink == NULL) {
        return false;
    }

    if (mBuffer == NULL) {
        mBufferSize = sink->bufferSize();
        mBuffer = malloc(mBufferSize);
    }

    size_t actualSize =
        (*mCallback)(sink.get(), mBuffer, mBufferSize, mCookie);

    if (actualSize > 0) {
        sink->write(mBuffer, actualSize);
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////

status_t MediaPlayerService::AudioCache::open(
        uint32_t sampleRate, int channelCount, audio_channel_mask_t channelMask,
        audio_format_t format, int bufferCount,
        AudioCallback cb, void *cookie, audio_output_flags_t flags)
{
    ALOGV("open(%u, %d, 0x%x, %d, %d)", sampleRate, channelCount, channelMask, format, bufferCount);
    if (mHeap->getHeapID() < 0) {
        return NO_INIT;
    }

    mSampleRate = sampleRate;
    mChannelCount = (uint16_t)channelCount;
    mFormat = format;
    mMsecsPerFrame = 1.e3 / (float) sampleRate;

    if (cb != NULL) {
        mCallbackThread = new CallbackThread(this, cb, cookie);
    }
    return NO_ERROR;
}

void MediaPlayerService::AudioCache::start() {
    if (mCallbackThread != NULL) {
        mCallbackThread->run("AudioCache callback");
    }
}

void MediaPlayerService::AudioCache::stop() {
    if (mCallbackThread != NULL) {
        mCallbackThread->requestExitAndWait();
    }
}

ssize_t MediaPlayerService::AudioCache::write(const void* buffer, size_t size)
{
    ALOGV("write(%p, %u)", buffer, size);
    if ((buffer == 0) || (size == 0)) return size;

    uint8_t* p = static_cast<uint8_t*>(mHeap->getBase());
    if (p == NULL) return NO_INIT;
    p += mSize;
    ALOGV("memcpy(%p, %p, %u)", p, buffer, size);
    if (mSize + size > mHeap->getSize()) {
        ALOGE("Heap size overflow! req size: %d, max size: %d", (mSize + size), mHeap->getSize());
        size = mHeap->getSize() - mSize;
    }
    memcpy(p, buffer, size);
    mSize += size;
    return size;
}

// call with lock held
status_t MediaPlayerService::AudioCache::wait()
{
    Mutex::Autolock lock(mLock);
    while (!mCommandComplete) {
        mSignal.wait(mLock);
    }
    mCommandComplete = false;

    if (mError == NO_ERROR) {
        ALOGV("wait - success");
    } else {
        ALOGV("wait - error");
    }
    return mError;
}

void MediaPlayerService::AudioCache::notify(
        void* cookie, int msg, int ext1, int ext2, const Parcel *obj)
{
    ALOGV("notify(%p, %d, %d, %d)", cookie, msg, ext1, ext2);
    AudioCache* p = static_cast<AudioCache*>(cookie);

    // ignore buffering messages
    switch (msg)
    {
    case MEDIA_ERROR:
        ALOGE("Error %d, %d occurred", ext1, ext2);
        p->mError = ext1;
        break;
    case MEDIA_PREPARED:
        ALOGV("prepared");
        break;
    case MEDIA_PLAYBACK_COMPLETE:
        ALOGV("playback complete");
        break;
    default:
        ALOGV("ignored");
        return;
    }

    // wake up thread
    Mutex::Autolock lock(p->mLock);
    p->mCommandComplete = true;
    p->mSignal.signal();
}

int MediaPlayerService::AudioCache::getSessionId() const
{
    return 0;
}

void MediaPlayerService::addBatteryData(uint32_t params)
{
    Mutex::Autolock lock(mLock);

    int32_t time = systemTime() / 1000000L;

    // change audio output devices. This notification comes from AudioFlinger
    if ((params & kBatteryDataSpeakerOn)
            || (params & kBatteryDataOtherAudioDeviceOn)) {

        int deviceOn[NUM_AUDIO_DEVICES];
        for (int i = 0; i < NUM_AUDIO_DEVICES; i++) {
            deviceOn[i] = 0;
        }

        if ((params & kBatteryDataSpeakerOn)
                && (params & kBatteryDataOtherAudioDeviceOn)) {
            deviceOn[SPEAKER_AND_OTHER] = 1;
        } else if (params & kBatteryDataSpeakerOn) {
            deviceOn[SPEAKER] = 1;
        } else {
            deviceOn[OTHER_AUDIO_DEVICE] = 1;
        }

        for (int i = 0; i < NUM_AUDIO_DEVICES; i++) {
            if (mBatteryAudio.deviceOn[i] != deviceOn[i]){

                if (mBatteryAudio.refCount > 0) { // if playing audio
                    if (!deviceOn[i]) {
                        mBatteryAudio.lastTime[i] += time;
                        mBatteryAudio.totalTime[i] += mBatteryAudio.lastTime[i];
                        mBatteryAudio.lastTime[i] = 0;
                    } else {
                        mBatteryAudio.lastTime[i] = 0 - time;
                    }
                }

                mBatteryAudio.deviceOn[i] = deviceOn[i];
            }
        }
        return;
    }

    // an sudio stream is started
    if (params & kBatteryDataAudioFlingerStart) {
        // record the start time only if currently no other audio
        // is being played
        if (mBatteryAudio.refCount == 0) {
            for (int i = 0; i < NUM_AUDIO_DEVICES; i++) {
                if (mBatteryAudio.deviceOn[i]) {
                    mBatteryAudio.lastTime[i] -= time;
                }
            }
        }

        mBatteryAudio.refCount ++;
        return;

    } else if (params & kBatteryDataAudioFlingerStop) {
        if (mBatteryAudio.refCount <= 0) {
            ALOGW("Battery track warning: refCount is <= 0");
            return;
        }

        // record the stop time only if currently this is the only
        // audio being played
        if (mBatteryAudio.refCount == 1) {
            for (int i = 0; i < NUM_AUDIO_DEVICES; i++) {
                if (mBatteryAudio.deviceOn[i]) {
                    mBatteryAudio.lastTime[i] += time;
                    mBatteryAudio.totalTime[i] += mBatteryAudio.lastTime[i];
                    mBatteryAudio.lastTime[i] = 0;
                }
            }
        }

        mBatteryAudio.refCount --;
        return;
    }

    int uid = IPCThreadState::self()->getCallingUid();
    if (uid == AID_MEDIA) {
        return;
    }
    int index = mBatteryData.indexOfKey(uid);

    if (index < 0) { // create a new entry for this UID
        BatteryUsageInfo info;
        info.audioTotalTime = 0;
        info.videoTotalTime = 0;
        info.audioLastTime = 0;
        info.videoLastTime = 0;
        info.refCount = 0;

        if (mBatteryData.add(uid, info) == NO_MEMORY) {
            ALOGE("Battery track error: no memory for new app");
            return;
        }
    }

    BatteryUsageInfo &info = mBatteryData.editValueFor(uid);

    if (params & kBatteryDataCodecStarted) {
        if (params & kBatteryDataTrackAudio) {
            info.audioLastTime -= time;
            info.refCount ++;
        }
        if (params & kBatteryDataTrackVideo) {
            info.videoLastTime -= time;
            info.refCount ++;
        }
    } else {
        if (info.refCount == 0) {
            ALOGW("Battery track warning: refCount is already 0");
            return;
        } else if (info.refCount < 0) {
            ALOGE("Battery track error: refCount < 0");
            mBatteryData.removeItem(uid);
            return;
        }

        if (params & kBatteryDataTrackAudio) {
            info.audioLastTime += time;
            info.refCount --;
        }
        if (params & kBatteryDataTrackVideo) {
            info.videoLastTime += time;
            info.refCount --;
        }

        // no stream is being played by this UID
        if (info.refCount == 0) {
            info.audioTotalTime += info.audioLastTime;
            info.audioLastTime = 0;
            info.videoTotalTime += info.videoLastTime;
            info.videoLastTime = 0;
        }
    }
}

status_t MediaPlayerService::pullBatteryData(Parcel* reply) {
    Mutex::Autolock lock(mLock);

    // audio output devices usage
    int32_t time = systemTime() / 1000000L; //in ms
    int32_t totalTime;

    for (int i = 0; i < NUM_AUDIO_DEVICES; i++) {
        totalTime = mBatteryAudio.totalTime[i];

        if (mBatteryAudio.deviceOn[i]
            && (mBatteryAudio.lastTime[i] != 0)) {
                int32_t tmpTime = mBatteryAudio.lastTime[i] + time;
                totalTime += tmpTime;
        }

        reply->writeInt32(totalTime);
        // reset the total time
        mBatteryAudio.totalTime[i] = 0;
   }

    // codec usage
    BatteryUsageInfo info;
    int size = mBatteryData.size();

    reply->writeInt32(size);
    int i = 0;

    while (i < size) {
        info = mBatteryData.valueAt(i);

        reply->writeInt32(mBatteryData.keyAt(i)); //UID
        reply->writeInt32(info.audioTotalTime);
        reply->writeInt32(info.videoTotalTime);

        info.audioTotalTime = 0;
        info.videoTotalTime = 0;

        // remove the UID entry where no stream is being played
        if (info.refCount <= 0) {
            mBatteryData.removeItemsAt(i);
            size --;
            i --;
        }
        i++;
    }
    return NO_ERROR;
}
} // namespace android
