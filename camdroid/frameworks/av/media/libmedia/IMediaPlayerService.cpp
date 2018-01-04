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

#include <stdint.h>
#include <sys/types.h>

#include <binder/Parcel.h>
#include <binder/IMemory.h>
#include <media/ICrypto.h>
#include <media/IHDCP.h>
#include <media/IMediaPlayerService.h>
#include <media/IMediaRecorder.h>
//#include <media/IOMX.h>
#include <media/IRemoteDisplay.h>
#include <media/IRemoteDisplayClient.h>
#include <media/IStreamSource.h>

#include <utils/Errors.h>  // for status_t
#include <utils/String8.h>

namespace android {

enum {
    CREATE = IBinder::FIRST_CALL_TRANSACTION,
    DECODE_URL,
    DECODE_FD,
    CREATE_MEDIA_RECORDER,
    CREATE_METADATA_RETRIEVER,
    CREATE_MEDIA_SERVER_CALLER,
    GET_OMX,
    MAKE_CRYPTO,
    MAKE_HDCP,
    ADD_BATTERY_DATA,
    PULL_BATTERY_DATA,
    LISTEN_FOR_REMOTE_DISPLAY,
    /* add by Gary. start {{----------------------------------- */
//    SET_SCREEN,
//    GET_SCREEN,
    IS_PLAYING_VIDEO
    /* add by Gary. end   -----------------------------------}} */

    /* add by Gary. start {{----------------------------------- */
    /* 2011-11-14 */
    /* support adjusting colors while playing video */
    ,
//    SET_VPP_GATE,
//    GET_VPP_GATE,
//    SET_LUMA_SHARP,
//    GET_LUMA_SHARP,
//    SET_CHROMA_SHARP,
//    GET_CHROMA_SHARP,
//    SET_WHITE_EXTEND,
//    GET_WHITE_EXTEND,
//    SET_BLACK_EXTEND,
//    GET_BLACK_EXTEND
    /* add by Gary. end   -----------------------------------}} */
    /* add by Gary. start {{----------------------------------- */
    /* 2012-03-12 */
    /* add the global interfaces to control the subtitle gate  */
//    ,
//    SET_GLOBAL_SUB_GATE,
//    GET_GLOBAL_SUB_GATE,
    /* add by Gary. end   -----------------------------------}} */
    /* add by Gary. start {{----------------------------------- */
    /* 2012-4-24 */
    /* add two general interfaces for expansibility */
//    GENERAL_GLOBAL_INTERFACE,
    /* add by Gary. end   -----------------------------------}} */
    CREATE_VIDEO_RESIZER,
};

class BpMediaPlayerService: public BpInterface<IMediaPlayerService>
{
public:
    BpMediaPlayerService(const sp<IBinder>& impl)
        : BpInterface<IMediaPlayerService>(impl)
    {
    }

    virtual sp<IMediaMetadataRetriever> createMetadataRetriever(pid_t pid)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
        data.writeInt32(pid);
        remote()->transact(CREATE_METADATA_RETRIEVER, data, &reply);
        return interface_cast<IMediaMetadataRetriever>(reply.readStrongBinder());
    }

    sp<IMediaServerCaller> createMediaServerCaller(pid_t pid)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
        data.writeInt32(pid);
        remote()->transact(CREATE_MEDIA_SERVER_CALLER, data, &reply);
        return interface_cast<IMediaServerCaller>(reply.readStrongBinder());
    }

    virtual sp<IMediaPlayer> create(
            pid_t pid, const sp<IMediaPlayerClient>& client, int audioSessionId) {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
        data.writeInt32(pid);
        data.writeStrongBinder(client->asBinder());
        data.writeInt32(audioSessionId);

        remote()->transact(CREATE, data, &reply);
        return interface_cast<IMediaPlayer>(reply.readStrongBinder());
    }

    virtual sp<IMediaRecorder> createMediaRecorder(pid_t pid)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
        data.writeInt32(pid);
        remote()->transact(CREATE_MEDIA_RECORDER, data, &reply);
        return interface_cast<IMediaRecorder>(reply.readStrongBinder());
    }

    virtual sp<IMediaVideoResizer> createMediaVideoResizer(pid_t pid)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
        data.writeInt32(pid);
        remote()->transact(CREATE_VIDEO_RESIZER, data, &reply);
        return interface_cast<IMediaVideoResizer>(reply.readStrongBinder());
    }

    virtual sp<IMemory> decode(const char* url, uint32_t *pSampleRate, int* pNumChannels, audio_format_t* pFormat)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
        data.writeCString(url);
        remote()->transact(DECODE_URL, data, &reply);
        *pSampleRate = uint32_t(reply.readInt32());
        *pNumChannels = reply.readInt32();
        *pFormat = (audio_format_t) reply.readInt32();
        return interface_cast<IMemory>(reply.readStrongBinder());
    }

    virtual sp<IMemory> decode(int fd, int64_t offset, int64_t length, uint32_t *pSampleRate, int* pNumChannels, audio_format_t* pFormat)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
        data.writeFileDescriptor(fd);
        data.writeInt64(offset);
        data.writeInt64(length);
        remote()->transact(DECODE_FD, data, &reply);
        *pSampleRate = uint32_t(reply.readInt32());
        *pNumChannels = reply.readInt32();
        *pFormat = (audio_format_t) reply.readInt32();
        return interface_cast<IMemory>(reply.readStrongBinder());
    }

//    virtual sp<IOMX> getOMX() {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
//        remote()->transact(GET_OMX, data, &reply);
        //return interface_cast<IOMX>(reply.readStrongBinder());
//        ALOGE("(f:%s, l:%d) not support OMX now!", __FUNCTION__, __LINE__);
//        return NULL;
//    }

    virtual sp<ICrypto> makeCrypto() {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
        remote()->transact(MAKE_CRYPTO, data, &reply);
        return interface_cast<ICrypto>(reply.readStrongBinder());
    }

    virtual sp<IHDCP> makeHDCP() {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
        remote()->transact(MAKE_HDCP, data, &reply);
        return interface_cast<IHDCP>(reply.readStrongBinder());
    }

    virtual void addBatteryData(uint32_t params) {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
        data.writeInt32(params);
        remote()->transact(ADD_BATTERY_DATA, data, &reply);
    }

    virtual status_t pullBatteryData(Parcel* reply) {
        Parcel data;
        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
        return remote()->transact(PULL_BATTERY_DATA, data, reply);
    }
    
    virtual sp<IRemoteDisplay> listenForRemoteDisplay(const sp<IRemoteDisplayClient>& client,
            const String8& iface)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
        data.writeStrongBinder(client->asBinder());
        data.writeString8(iface);
        remote()->transact(LISTEN_FOR_REMOTE_DISPLAY, data, &reply);
        return interface_cast<IRemoteDisplay>(reply.readStrongBinder());
    }

    /* add by Gary. start {{----------------------------------- */
//    status_t setScreen(int screen)
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
//        data.writeInt32(screen);
//        remote()->transact(SET_SCREEN, data, &reply);
//        return reply.readInt32();
//    }
//
//    status_t getScreen(int *screen)
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
//        remote()->transact(GET_SCREEN, data, &reply);
//        *screen = reply.readInt32();
//        return reply.readInt32();
//    }
    
    status_t isPlayingVideo(int *ret)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
        remote()->transact(IS_PLAYING_VIDEO, data, &reply);
        *ret = reply.readInt32();
        return reply.readInt32();
    }
    /* add by Gary. end   -----------------------------------}} */

    /* add by Gary. start {{----------------------------------- */
    /* 2011-11-14 */
    /* support adjusting colors while playing video */
//    status_t setVppGate(bool enableVpp)
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
//        data.writeInt32(enableVpp);
//        remote()->transact(SET_VPP_GATE, data, &reply);
//        return reply.readInt32();
//    }
//
//    bool getVppGate()
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
//        remote()->transact(GET_VPP_GATE, data, &reply);
//        return reply.readInt32();
//    }
//
//    status_t setLumaSharp(int value)
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
//        data.writeInt32(value);
//        remote()->transact(SET_LUMA_SHARP, data, &reply);
//        return reply.readInt32();
//    }
//
//    int getLumaSharp()
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
//        remote()->transact(GET_LUMA_SHARP, data, &reply);
//        return reply.readInt32();
//    }
//    
//    status_t setChromaSharp(int value)
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
//        data.writeInt32(value);
//        remote()->transact(SET_CHROMA_SHARP, data, &reply);
//        return reply.readInt32();
//    }
//
//    int getChromaSharp()
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
//        remote()->transact(GET_CHROMA_SHARP, data, &reply);
//        return reply.readInt32();
//    }
//    
//    status_t setWhiteExtend(int value)
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
//        data.writeInt32(value);
//        remote()->transact(SET_WHITE_EXTEND, data, &reply);
//        return reply.readInt32();
//    }
//
//    int getWhiteExtend()
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
//        remote()->transact(GET_WHITE_EXTEND, data, &reply);
//        return reply.readInt32();
//    }
//    
//    status_t setBlackExtend(int value)
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
//        data.writeInt32(value);
//        remote()->transact(SET_BLACK_EXTEND, data, &reply);
//        return reply.readInt32();
//    }
//
//    int getBlackExtend()
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
//        remote()->transact(GET_BLACK_EXTEND, data, &reply);
//        return reply.readInt32();
//    }
    
    /* add by Gary. end   -----------------------------------}} */

    /* add by Gary. start {{----------------------------------- */
    /* 2012-03-12 */
    /* add the global interfaces to control the subtitle gate  */
//    status_t setGlobalSubGate(bool showSub)
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
//        data.writeInt32(showSub);
//        remote()->transact(SET_GLOBAL_SUB_GATE, data, &reply);
//        return reply.readInt32();
//    }
//
//    bool getGlobalSubGate()
//    {
//        Parcel data, reply;
//        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
//        remote()->transact(GET_GLOBAL_SUB_GATE, data, &reply);
//        return reply.readInt32();
//    }    
    /* add by Gary. end   -----------------------------------}} */

    /* add by Gary. start {{----------------------------------- */
    /* 2012-4-24 */
    /* add two general interfaces for expansibility */
//    status_t generalGlobalInterface(int cmd, int int1, int int2, int int3, void *p)
//    {
//        Parcel data, reply;
//        status_t ret;
//        data.writeInterfaceToken(IMediaPlayerService::getInterfaceDescriptor());
//        
//        data.writeInt32(cmd);                               // the first input value MUST be always the command.
//        switch(cmd){
//            case MEDIAPLAYER_GLOBAL_CMD_TEST:{
//                data.writeInt32(int1);
//                remote()->transact(GENERAL_GLOBAL_INTERFACE, data, &reply);
//                ret = reply.readInt32();
//                *((int *)p) = reply.readInt32();
//            }break;
//			case MEDIAPLAYER_CMD_IS_ROTATABLE:{
//                remote()->transact(GENERAL_GLOBAL_INTERFACE, data, &reply);
//                ret = reply.readInt32();
//                *((int *)p) = reply.readInt32();
//			}break;
//			case MEDIAPLAYER_CMD_SET_ROTATION:{
//                data.writeInt32(int1);
//                remote()->transact(GENERAL_GLOBAL_INTERFACE, data, &reply);
//                ret = reply.readInt32();
//			}break;
//            case MEDIAPLAYER_CMD_SET_HDMISTATE:{
//                data.writeInt32(int1);
//                remote()->transact(GENERAL_GLOBAL_INTERFACE, data, &reply);
//                ret = reply.readInt32();
//			}break;
//            default:
//                return BAD_VALUE;
//        }
//        return ret;
//    }
    /* add by Gary. end   -----------------------------------}} */ 
};

IMPLEMENT_META_INTERFACE(MediaPlayerService, "android.media.IMediaPlayerService");

// ----------------------------------------------------------------------

status_t BnMediaPlayerService::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch (code) {
        case CREATE: {
            CHECK_INTERFACE(IMediaPlayerService, data, reply);
            pid_t pid = data.readInt32();
            sp<IMediaPlayerClient> client =
                interface_cast<IMediaPlayerClient>(data.readStrongBinder());
            int audioSessionId = data.readInt32();
            sp<IMediaPlayer> player = create(pid, client, audioSessionId);
            reply->writeStrongBinder(player->asBinder());
            return NO_ERROR;
        } break;
        case DECODE_URL: {
            CHECK_INTERFACE(IMediaPlayerService, data, reply);
            const char* url = data.readCString();
            uint32_t sampleRate;
            int numChannels;
            audio_format_t format;
            sp<IMemory> player = decode(url, &sampleRate, &numChannels, &format);
            reply->writeInt32(sampleRate);
            reply->writeInt32(numChannels);
            reply->writeInt32((int32_t) format);
            reply->writeStrongBinder(player->asBinder());
            return NO_ERROR;
        } break;
        case DECODE_FD: {
            CHECK_INTERFACE(IMediaPlayerService, data, reply);
            int fd = dup(data.readFileDescriptor());
            int64_t offset = data.readInt64();
            int64_t length = data.readInt64();
            uint32_t sampleRate;
            int numChannels;
            audio_format_t format;
            sp<IMemory> player = decode(fd, offset, length, &sampleRate, &numChannels, &format);
            reply->writeInt32(sampleRate);
            reply->writeInt32(numChannels);
            reply->writeInt32((int32_t) format);
            reply->writeStrongBinder(player->asBinder());
            return NO_ERROR;
        } break;
        case CREATE_MEDIA_RECORDER: {
            CHECK_INTERFACE(IMediaPlayerService, data, reply);
            pid_t pid = data.readInt32();
            sp<IMediaRecorder> recorder = createMediaRecorder(pid);
            reply->writeStrongBinder(recorder->asBinder());
            return NO_ERROR;
        } break;
        case CREATE_METADATA_RETRIEVER: {
            CHECK_INTERFACE(IMediaPlayerService, data, reply);
            pid_t pid = data.readInt32();
            sp<IMediaMetadataRetriever> retriever = createMetadataRetriever(pid);
            reply->writeStrongBinder(retriever->asBinder());
            return NO_ERROR;
        } break;
        case CREATE_VIDEO_RESIZER: {
            CHECK_INTERFACE(IMediaPlayerService, data, reply);
            pid_t pid = data.readInt32();
            sp<IMediaVideoResizer> resizer = createMediaVideoResizer(pid);
            reply->writeStrongBinder(resizer->asBinder());
            return NO_ERROR;
        } break;
        case CREATE_MEDIA_SERVER_CALLER: {
            CHECK_INTERFACE(IMediaPlayerService, data, reply);
            pid_t pid = data.readInt32();
            sp<IMediaServerCaller> caller = createMediaServerCaller(pid);
            reply->writeStrongBinder(caller->asBinder());
            return NO_ERROR;
        } break;
        case GET_OMX: {
            CHECK_INTERFACE(IMediaPlayerService, data, reply);
            ALOGE("(f:%s, l:%d) not support OMX now!", __FUNCTION__, __LINE__);
//            sp<IOMX> omx = getOMX();
//            reply->writeStrongBinder(omx->asBinder());
            return NO_ERROR;
        } break;
        case MAKE_CRYPTO: {
            CHECK_INTERFACE(IMediaPlayerService, data, reply);
            sp<ICrypto> crypto = makeCrypto();
            reply->writeStrongBinder(crypto->asBinder());
            return NO_ERROR;
        } break;
        case MAKE_HDCP: {
            CHECK_INTERFACE(IMediaPlayerService, data, reply);
            sp<IHDCP> hdcp = makeHDCP();
            reply->writeStrongBinder(hdcp->asBinder());
            return NO_ERROR;
        } break;
        case ADD_BATTERY_DATA: {
            CHECK_INTERFACE(IMediaPlayerService, data, reply);
            uint32_t params = data.readInt32();
            addBatteryData(params);
            return NO_ERROR;
        } break;
        case PULL_BATTERY_DATA: {
            CHECK_INTERFACE(IMediaPlayerService, data, reply);
            pullBatteryData(reply);
            return NO_ERROR;
        } break;
        case LISTEN_FOR_REMOTE_DISPLAY: {
            CHECK_INTERFACE(IMediaPlayerService, data, reply);
            sp<IRemoteDisplayClient> client(
                    interface_cast<IRemoteDisplayClient>(data.readStrongBinder()));
            String8 iface(data.readString8());
            sp<IRemoteDisplay> display(listenForRemoteDisplay(client, iface));
            reply->writeStrongBinder(display->asBinder());
            return NO_ERROR;
        } break;
        /* add by Gary. start {{----------------------------------- */
//        case SET_SCREEN: {
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(setScreen(data.readInt32()));
//            return NO_ERROR;
//        } break;
//        case GET_SCREEN: {
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            int screen;
//            status_t ret = getScreen(&screen);
//            reply->writeInt32(screen);
//            reply->writeInt32(ret);
//            return NO_ERROR;
//        } break;
        case IS_PLAYING_VIDEO: {
            CHECK_INTERFACE(IMediaPlayer, data, reply);
            int playing;
            status_t ret = isPlayingVideo(&playing);
            reply->writeInt32(playing);
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        /* add by Gary. end   -----------------------------------}} */

        /* add by Gary. start {{----------------------------------- */
        /* 2011-11-14 */
        /* support adjusting colors while playing video */
//        case SET_VPP_GATE: {
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(setVppGate(data.readInt32()));
//            return NO_ERROR;
//        } break;
//        case GET_VPP_GATE: {
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(getVppGate());
//            return NO_ERROR;
//        } break;
//        case SET_LUMA_SHARP: {
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(setLumaSharp(data.readInt32()));
//            return NO_ERROR;
//        } break;
//        case GET_LUMA_SHARP: {
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(getLumaSharp());
//            return NO_ERROR;
//        } break;
//        case SET_CHROMA_SHARP: {
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(setChromaSharp(data.readInt32()));
//            return NO_ERROR;
//        } break;
//        case GET_CHROMA_SHARP: {
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(getChromaSharp());
//            return NO_ERROR;
//        } break;
//        case SET_WHITE_EXTEND: {
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(setWhiteExtend(data.readInt32()));
//            return NO_ERROR;
//        } break;
//        case GET_WHITE_EXTEND: {
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(getWhiteExtend());
//            return NO_ERROR;
//        } break;
//        case SET_BLACK_EXTEND: {
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(setBlackExtend(data.readInt32()));
//            return NO_ERROR;
//        } break;
//        case GET_BLACK_EXTEND: {
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(getBlackExtend());
//            return NO_ERROR;
//        } break;
        /* add by Gary. end   -----------------------------------}} */
        /* add by Gary. start {{----------------------------------- */
        /* 2012-03-12 */
        /* add the global interfaces to control the subtitle gate  */
//        case SET_GLOBAL_SUB_GATE: {
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(setGlobalSubGate(data.readInt32()));
//            return NO_ERROR;
//        } break;
//        case GET_GLOBAL_SUB_GATE: {
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            reply->writeInt32(getGlobalSubGate());
//            return NO_ERROR;
//        } break;
        /* add by Gary. end   -----------------------------------}} */
        /* add by Gary. end   -----------------------------------}} */
        /* add by Gary. start {{----------------------------------- */
        /* 2012-4-24 */
        /* add two general interfaces for expansibility */
//        case GENERAL_GLOBAL_INTERFACE: {      
//            CHECK_INTERFACE(IMediaPlayer, data, reply);
//            int cmd;
//            int int1 = 0;
//            int int2 = 0;
//            int int3 = 0;
//            void *p  = NULL;
//            status_t ret;
//            
//            cmd = data.readInt32();
//            switch(cmd){
//                case MEDIAPLAYER_GLOBAL_CMD_TEST:{
//                    int1 = data.readInt32();
//                    int data = 2;
//                    p = &data;
//                    ret = generalGlobalInterface(cmd, int1, int2, int3, p);
//                    reply->writeInt32(ret);
//                    reply->writeInt32(data);
//                }break;
//                case MEDIAPLAYER_CMD_IS_ROTATABLE:{
//                    int rotatable;
//                    p = &rotatable;
//                    ret = generalGlobalInterface(cmd, int1, int2, int3, p);
//                    reply->writeInt32(ret);
//                    reply->writeInt32(rotatable);
//                }break;
//				case MEDIAPLAYER_CMD_SET_ROTATION:{
//                    int1 = data.readInt32();
//                    ret = generalGlobalInterface(cmd, int1, int2, int3, p);
//                    reply->writeInt32(ret);
//				}break;
//                case MEDIAPLAYER_CMD_SET_HDMISTATE:{
//                    int1 = data.readInt32();
//                    ret = generalGlobalInterface(cmd, int1, int2, int3, p);
//                    reply->writeInt32(ret);
//				}break;
//                default:
//                    return BAD_VALUE;
//            }
//            return NO_ERROR;
//        } break;
        /* add by Gary. end   -----------------------------------}} */
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

// ----------------------------------------------------------------------------

}; // namespace android
