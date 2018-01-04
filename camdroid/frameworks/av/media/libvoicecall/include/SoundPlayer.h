#ifndef _SOUND_PLAYER_H
#define _SOUND_PLAYER_H
#include <utils/Log.h>
#include <utils/threads.h>
#include <utils/Errors.h>

#include <media/AudioTrack.h>
#include <media/AudioSystem.h>

#include "AudioCodec.h"

#include "echocancel.h"
#include <utils/List.h>

#include "BufferManager.h"

using namespace android;

#define ENC_BUF_COUNT   50
/*
typedef struct Decoder_Params_t {
    //char                name[16];
    const char *        name;
    int                 sampleRate;
    int                 channels;
    int                 bitRate;
    int                 mode;
}DecoderParams;
*/
class SoundPlayer {
public:
    SoundPlayer();
    ~SoundPlayer();

    status_t rtspFeedAudioData(char *audioBuf, unsigned int len, long long pts);
    status_t init(void *aec, CodecParams *params);
    status_t start();
    status_t stop();
    status_t stop_l();
    status_t exit();

    bool audioPlayThread();

private:
    int createAudioTrack(int sampleRate, int channels);
//#ifndef PLAY_USE_WEBRTC_NETEQ
    status_t audioDecoderInit(CodecParams *params);
    status_t audioDecoderExit();
//#endif
    status_t                mInitCheck;
    bool                    mStarted;
    Mutex                   mLock;

    int                     mSampleRate;
    int                     mChannels;

    AudioTrack *            mAudioTrack;
    char *                  mPcmBuffer;

    bool                    mFirstPacket;
    pthread_mutex_t         mInMutex;
    pthread_cond_t          mInCond;
    void *                  mAec;

//#ifdef PLAY_USE_WEBRTC_NETEQ
    char *                  mDecName;
    int                     mDecMode;
    void *                  mNeteqInst;
//#else
    AudioDecoder *          mDecoder;
    int                     mSampleCount;
    int                     mFrameSize;
    BufferManager *         mBufferManager;
    uint32_t                mBufferSize;
//#endif

    uint64_t                mSequenceNumber;
    int                     mDuration;

    class PlayThread : public Thread {
    public:
        PlayThread(SoundPlayer *player) : Thread(false), mPlayer(player) {}

        bool start()
        {
            if (run("SoundPlayer", ANDROID_PRIORITY_AUDIO) != NO_ERROR) {
                ALOGE("cannot start SoundPlayer thread");
                return false;
            }

            return true;
        }

        status_t stop()
        {
            return requestExitAndWait();
        }

    private:
        bool threadLoop() {
            return mPlayer->audioPlayThread();
        };

        SoundPlayer *       mPlayer;
    };

    sp<PlayThread>          mPlayThread;
};

#endif
