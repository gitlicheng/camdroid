//#define LOG_NDEBUG 0
#define LOG_TAG "CedarAPlayerWrapperWrapper"
#include <utils/Log.h>

#include "CedarAPlayerWrapper.h"

#include <CedarAPlayer.h>

#include <media/Metadata.h>
#include <media/stagefright/MediaExtractor.h>

namespace android {

CedarAPlayerWrapper::CedarAPlayerWrapper()
    : mPlayer(new CedarAPlayer) {
    ALOGV("CedarAPlayerWrapper");

    mPlayer->setListener(this);
}

CedarAPlayerWrapper::~CedarAPlayerWrapper() {
    ALOGV("~CedarAPlayerWrapper");
    reset();

    delete mPlayer;
    mPlayer = NULL;
}

status_t CedarAPlayerWrapper::initCheck() {
    ALOGV("initCheck");
    return OK;
}

status_t CedarAPlayerWrapper::setUID(uid_t uid) {
    mPlayer->setUID(uid);

    return OK;
}

status_t CedarAPlayerWrapper::setDataSource(
        const char *url, const KeyedVector<String8, String8> *headers) {
    ALOGD("setDataSource('%s')", url);
    return mPlayer->setDataSource(url, headers);
}

// Warning: The filedescriptor passed into this method will only be valid until
// the method returns, if you want to keep it, dup it!
status_t CedarAPlayerWrapper::setDataSource(int fd, int64_t offset, int64_t length) {
    ALOGD("setDataSource(%d, %lld, %lld)", fd, offset, length);
    return mPlayer->setDataSource(dup(fd), offset, length);
}

status_t CedarAPlayerWrapper::setDataSource(const sp<IStreamSource> &source) {
    return mPlayer->setDataSource(source);
}

status_t CedarAPlayerWrapper::setParameter(int key, const Parcel &request) {
    ALOGV("setParameter");
    return mPlayer->setParameter(key, request);
}

status_t CedarAPlayerWrapper::getParameter(int key, Parcel *reply) {
    ALOGV("getParameter");
    return mPlayer->getParameter(key, reply);
}

//status_t CedarAPlayerWrapper::setVideoSurfaceTexture(
//        const sp<ISurfaceTexture> &surfaceTexture) 
status_t CedarAPlayerWrapper::setVideoSurfaceTexture(const unsigned int hlay) 
{
    ALOGV("setVideoSurfaceTexture");

    return mPlayer->setSurfaceTexture(hlay);
}

status_t CedarAPlayerWrapper::prepare() {
    return mPlayer->prepare();
}

status_t CedarAPlayerWrapper::prepareAsync() {
    return mPlayer->prepareAsync();
}

status_t CedarAPlayerWrapper::start() {
    ALOGV("start");

    return mPlayer->play();
}

status_t CedarAPlayerWrapper::stop() {
    ALOGV("stop");

    return mPlayer->stop();  // what's the difference?
}

status_t CedarAPlayerWrapper::pause() {
    ALOGV("pause");

    return mPlayer->pause();
}

bool CedarAPlayerWrapper::isPlaying() {
    ALOGV("isPlaying");
    return mPlayer->isPlaying();
}

status_t CedarAPlayerWrapper::seekTo(int msec) {
    ALOGV("seekTo");

    status_t err = mPlayer->seekTo((int64_t)msec);

    return err;
}

status_t CedarAPlayerWrapper::getCurrentPosition(int *msec) {
    ALOGV("getCurrentPosition");

    int64_t positionUs;
    status_t err = mPlayer->getPosition(&positionUs);

    if (err != OK) {
        return err;
    }

    *msec = (positionUs + 500) / 1000;

    return OK;
}

status_t CedarAPlayerWrapper::getDuration(int *msec) {
    ALOGV("getDuration");

    int64_t durationUs;
    status_t err = mPlayer->getDuration(&durationUs);

    if (err != OK) {
        *msec = 0;
        return OK;
    }

    *msec = (durationUs + 500) / 1000;

    return OK;
}

status_t CedarAPlayerWrapper::reset() {
    ALOGV("reset");

    mPlayer->reset();

    return OK;
}

status_t CedarAPlayerWrapper::setLooping(int loop) {
    ALOGV("setLooping");

    return mPlayer->setLooping(loop);
}

player_type CedarAPlayerWrapper::playerType() {
    ALOGV("playerType");
    return CEDARX_PLAYER;
}

//status_t CedarAPlayerWrapper::setScreen(int screen) {
//    ALOGV("setScreen");
//    return mPlayer->setScreen(screen);
//}

status_t CedarAPlayerWrapper::invoke(const Parcel &request, Parcel *reply) {
    return INVALID_OPERATION;
}

void CedarAPlayerWrapper::setAudioSink(const sp<AudioSink> &audioSink) {
    MediaPlayerInterface::setAudioSink(audioSink);

    mPlayer->setAudioSink(audioSink);
}

status_t CedarAPlayerWrapper::getMetadata(
        const media::Metadata::Filter& ids, Parcel *records) {
    using media::Metadata;

    uint32_t flags = mPlayer->flags();

    Metadata metadata(records);

    metadata.appendBool(
            Metadata::kPauseAvailable,
            flags & MediaExtractor::CAN_PAUSE);

    metadata.appendBool(
            Metadata::kSeekBackwardAvailable,
            flags & MediaExtractor::CAN_SEEK_BACKWARD);

    metadata.appendBool(
            Metadata::kSeekForwardAvailable,
            flags & MediaExtractor::CAN_SEEK_FORWARD);

    metadata.appendBool(
            Metadata::kSeekAvailable,
            flags & MediaExtractor::CAN_SEEK);

    return OK;
}

}  // namespace android
