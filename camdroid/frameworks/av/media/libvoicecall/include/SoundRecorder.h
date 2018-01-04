#ifndef _SOUND_RECORDER_H
#define _SOUND_RECORDER_H

#include <utils/Log.h>
#include <utils/threads.h>
#include <utils/Errors.h>

#include <media/AudioRecord.h>
#include <media/AudioSystem.h>

#include "AudioCodec.h"
#include "echocancel.h"

#include <utils/List.h>
#include "BufferManager.h"

using namespace android;

#define REC_BUF_COUNT   8

typedef enum aec_state {
    AEC_DISABLED = 0,
    AEC_ENABLED  = 1,
} AEC_STATE;
/*
typedef struct Encoder_Params_t {
    char                name[16];
    int                 sampleRate;
    int                 channels;
    int                 bitRate;
    int                 mode;
}EncoderParams;
*/
class AudioEncoder;

class SoundRecorder {
public:
    SoundRecorder();
    ~SoundRecorder();

    typedef void (*callback_t)(void * user);
    int rtspGetAudioData(unsigned char *data, unsigned int size, long long *pts);
    status_t setAecEnabled(bool enabled);
    bool isAecEnabled();
    status_t init(void *aec, CodecParams *params, void *user, callback_t cbf);
    status_t start();
    status_t stop();
    status_t exit();

    bool audioRecordThread();

private:
    status_t createAudioRecorder(int sampleRate, int channels);
    status_t setAecEnabled_l(bool enabled);
    status_t audioEncoderInit(CodecParams *params);
    status_t audioEncoderExit();
    status_t stop_l();

    status_t                mInitCheck;
    bool                    mStarted;
    Mutex                   mLock;
    callback_t              mCbf;
    void*                   mUserData;

    sp<AudioRecord>         mAudioRecord;

    int                     mSampleRate;
    int                     mChannels;

    BufferManager *         mBufferManager;

    long long               mPts;

    AudioEncoder *          mEncoder;
    int                     mSampleCount;
    int                     mAecFrameCnt;

    void *                  mAec;
    AEC_STATE               mAecState;

    char*                   mRecBuffer;
    char*                   mRecEcBuffer;
    unsigned int            mRecBufSize;

    class RecordThread : public Thread {
    public:
        RecordThread(SoundRecorder *recorder) : Thread(false), mRecorder(recorder) {}

        bool start()
        {
            if (run("SoundRecorder", ANDROID_PRIORITY_AUDIO) != NO_ERROR) {
                ALOGE("cannot start SoundRecorder thread");
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
            return mRecorder->audioRecordThread();
        };

        SoundRecorder *     mRecorder;
    };

    sp<RecordThread>        mRecordThread;
};

#endif

