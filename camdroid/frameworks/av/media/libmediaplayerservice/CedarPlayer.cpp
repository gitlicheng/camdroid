//#define ALOG_NDEBUG 0
#define ALOG_TAG "CedarPlayerWrapper"
#include <utils/Log.h>

#include "CedarPlayer.h"

#include <CedarXPlayer.h>

#include <media/Metadata.h>
#include <media/stagefright/MediaExtractor.h>

namespace android {

CedarPlayer::CedarPlayer()
    : mPlayer(new CedarXPlayer) {
    ALOGV("CedarPlayer");

    mPlayer->setListener(this);
}

CedarPlayer::~CedarPlayer() {
    ALOGV("~CedarPlayer");
    reset();

    delete mPlayer;
    mPlayer = NULL;
}

status_t CedarPlayer::initCheck() {
    ALOGV("initCheck");
    return OK;
}

status_t CedarPlayer::setUID(uid_t uid) {
    mPlayer->setUID(uid);

    return OK;
}

status_t CedarPlayer::setDataSource(
        const char *url, const KeyedVector<String8, String8> *headers) {
    ALOGI("setDataSource('%s')", url);
    return mPlayer->setDataSource(url, headers);
}

// Warning: The filedescriptor passed into this method will only be valid until
// the method returns, if you want to keep it, dup it!
status_t CedarPlayer::setDataSource(int fd, int64_t offset, int64_t length) {
    ALOGV("setDataSource(%d, %lld, %lld)", fd, offset, length);
    return mPlayer->setDataSource(dup(fd), offset, length);
}

status_t CedarPlayer::setDataSource(const sp<IStreamSource> &source) {
    return mPlayer->setDataSource(source);
}

status_t CedarPlayer::setParameter(int key, const Parcel &request) {
    ALOGV("setParameter");
    return mPlayer->setParameter(key, request);
}

status_t CedarPlayer::getParameter(int key, Parcel *reply) {
    ALOGV("getParameter");
    return mPlayer->getParameter(key, reply);
}

//status_t CedarPlayer::setVideoSurface(const sp<Surface> &surface) 
status_t CedarPlayer::setVideoSurface(const unsigned int hlay) 
{
    ALOGV("setVideoSurface");

    return mPlayer->setSurface(hlay);
}

//status_t CedarPlayer::setVideoSurfaceTexture(
//        const sp<ISurfaceTexture> &surfaceTexture) 
status_t CedarPlayer::setVideoSurfaceTexture(
        const unsigned int hlay) 
{
    ALOGV("setVideoSurfaceTexture");

    return mPlayer->setSurfaceTexture(hlay);
}

status_t CedarPlayer::prepare() {
    return mPlayer->prepare();
}

status_t CedarPlayer::prepareAsync() {
    return mPlayer->prepareAsync();
}

status_t CedarPlayer::start() {
    ALOGV("start");

    return mPlayer->play();
}

status_t CedarPlayer::stop() {
    ALOGV("stop");

    return mPlayer->stop();  // what's the difference?
}

status_t CedarPlayer::pause() {
    ALOGV("pause");

    return mPlayer->pause();
}

bool CedarPlayer::isPlaying() {
    ALOGV("isPlaying");
    return mPlayer->isPlaying();
}

status_t CedarPlayer::seekTo(int msec) {
    ALOGV("seekTo");

    status_t err = mPlayer->seekTo((int64_t)msec);

    return err;
}

status_t CedarPlayer::getCurrentPosition(int *msec) {
    ALOGV("getCurrentPosition");

    int64_t positionUs;
    status_t err = mPlayer->getPosition(&positionUs);

    if (err != OK) {
        return err;
    }

    *msec = (positionUs + 500) / 1000;

    return OK;
}

status_t CedarPlayer::getDuration(int *msec) {
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

status_t CedarPlayer::reset() {
    ALOGV("reset");

    mPlayer->reset();

    return OK;
}

status_t CedarPlayer::setLooping(int loop) {
    ALOGV("setLooping");

    return mPlayer->setLooping(loop);
}

player_type CedarPlayer::playerType() {
    ALOGV("playerType");
    return CEDARX_PLAYER;
}

//status_t CedarPlayer::setScreen(int screen) {
//    ALOGV("setScreen");
//    return mPlayer->setScreen(screen);
//}

int CedarPlayer::getMeidaPlayerState() {
    ALOGV("getMeidaPlayerState");
    return mPlayer->getMeidaPlayerState();
}

status_t CedarPlayer::invoke(const Parcel &request, Parcel *reply) {
    ALOGV("invoke()");
    return mPlayer->invoke(request, reply);
}

void CedarPlayer::setAudioSink(const sp<AudioSink> &audioSink) {
    MediaPlayerInterface::setAudioSink(audioSink);

    mPlayer->setAudioSink(audioSink);
}

status_t CedarPlayer::getMetadata(
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

//int CedarPlayer::getSubCount()
//{
//	return mPlayer->getSubCount();
//}
//
//int CedarPlayer::getSubList(MediaPlayer_SubInfo *infoList, int count)
//{
//    return mPlayer->getSubList(infoList,count);
//}
//
//int CedarPlayer::getCurSub()
//{
//	return mPlayer->getCurSub();
//}
//
//status_t CedarPlayer::switchSub(int index)
//{
//	return mPlayer->switchSub(index);
//}
//
//status_t CedarPlayer::setSubGate(bool showSub)
//{
//	return mPlayer->setSubGate(showSub);
//}
//
//bool CedarPlayer::getSubGate()
//{
//	return mPlayer->getSubGate();
//}
//
//status_t CedarPlayer::setSubColor(int color)
//{
//	return mPlayer->setSubColor(color);
//}
//
//int CedarPlayer::getSubColor()
//{
//    return mPlayer->getSubColor();
//};
//
//status_t CedarPlayer::setSubFrameColor(int color)
//{
//	return mPlayer->setSubFrameColor(color);
//}
//
//int CedarPlayer::getSubFrameColor()
//{
//    return mPlayer->getSubFrameColor();
//}
//
//status_t CedarPlayer::setSubFontSize(int size)
//{
//    return mPlayer->setSubFontSize(size);
//}
//
//int CedarPlayer::getSubFontSize()
//{
//    return mPlayer->getSubFontSize();
//}

status_t CedarPlayer::setSubCharset(const char *charset)
{
    return mPlayer->setSubCharset(charset);
}

status_t CedarPlayer::getSubCharset(char *charset)
{
    return mPlayer->getSubCharset(charset);
}

//status_t CedarPlayer::setSubPosition(int percent)
//{
//    return mPlayer->setSubPosition(percent);
//}
//
//int CedarPlayer::getSubPosition()
//{
//    return mPlayer->getSubPosition();
//}

status_t CedarPlayer::setSubDelay(int time)
{
    return mPlayer->setSubDelay(time);
}

int CedarPlayer::getSubDelay()
{
    return mPlayer->getSubDelay();
}

//int CedarPlayer::getTrackCount()
//{
//    return mPlayer->getTrackCount();
//}
//
//int CedarPlayer::getTrackList(MediaPlayer_TrackInfo *infoList, int count)
//{
//    return mPlayer->getTrackList(infoList, count);
//}
//
//int CedarPlayer::getCurTrack()
//{
//    return mPlayer->getCurTrack();
//}
//
//status_t CedarPlayer::switchTrack(int index)
//{
//    return mPlayer->switchTrack(index);
//}
//
//status_t CedarPlayer::setInputDimensionType(int type)
//{
//	return mPlayer->setInputDimensionType(type);
//}
//
//int CedarPlayer::getInputDimensionType()
//{
//	return mPlayer->getInputDimensionType();
//}
//
//status_t CedarPlayer::setOutputDimensionType(int type)
//{
//	return mPlayer->setOutputDimensionType(type);
//}
//
//int CedarPlayer::getOutputDimensionType()
//{
//	return mPlayer->getOutputDimensionType();
//}
//
//status_t CedarPlayer::setAnaglaghType(int type)
//{
//	return mPlayer->setAnaglaghType(type);
//}
//
//int CedarPlayer::getAnaglaghType()
//{
//	return mPlayer->getAnaglaghType();
//}
//
//status_t CedarPlayer::getVideoEncode(char *encode)
//{
//    return -1;
//}
//
//int CedarPlayer::getVideoFrameRate()
//{
//    return -1;
//}
//
//status_t CedarPlayer::getAudioEncode(char *encode)
//{
//    return -1;
//}
//
//int CedarPlayer::getAudioBitRate()
//{
//    return -1;
//}
//
//int CedarPlayer::getAudioSampleRate()
//{
//    return -1;
//}

status_t CedarPlayer::enableScaleMode(bool enable, int width, int height)
{
	return mPlayer->enableScaleMode(enable, width, height);
}

//status_t CedarPlayer::setVppGate(bool enableVpp)
//{
//	return mPlayer->setVppGate(enableVpp);
//}
//
//status_t CedarPlayer::setLumaSharp(int value)
//{
//	return mPlayer->setLumaSharp(value);
//}
//
//status_t CedarPlayer::setChromaSharp(int value)
//{
//	return mPlayer->setChromaSharp(value);
//}
//
//status_t CedarPlayer::setWhiteExtend(int value)
//{
//	return mPlayer->setWhiteExtend(value);
//}
//
//status_t CedarPlayer::setBlackExtend(int value)
//{
//	return mPlayer->setBlackExtend(value);
//}
//
//status_t CedarPlayer::extensionControl(int command, int para0, int para1)
//{
//	return mPlayer->extensionControl(command, para0, para1);
//}
//
status_t CedarPlayer::generalInterface(int cmd, int int1, int int2, int int3, void *p)
{
	return mPlayer->generalInterface(cmd, int1, int2, int3, p);
}

}  // namespace android
