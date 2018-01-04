#ifndef _BUFFER_MANAGER_H
#define _BUFFER_MANAGER_H
#include <utils/Errors.h>
#include <utils/List.h>
#include <utils/threads.h>

using namespace android;

typedef struct AudioBuffer {
    long            pts;
    int             size;
    char            data[0];
}AudioBuffer;

class BufferManager  {
public:
    BufferManager();
    BufferManager(uint32_t bufferCount, uint32_t bufferSize);
    ~BufferManager();
    status_t initCheck() const;

    bool isAudioBufferQueueEmpty();
    long getFirstAudioBufferPts();
    AudioBuffer *getFirstAudioBuffer();
    status_t pushAudioBuffer(AudioBuffer *buffer);
    AudioBuffer *getFirstEmptyBuffer();
    status_t pushEmptyBuffer(AudioBuffer *buffer);

    status_t bufferAlloc(uint32_t bufferCount, uint32_t bufferSize);
    void bufferDeAlloc();
    status_t bufferFlush();

private:
    void bufferDeAlloc_l();

    List<AudioBuffer *>     mEmptyQueue;
    List<AudioBuffer *>     mFilledQueue;
    List<AudioBuffer *>     mBufPool;

    uint32_t                mBufferSize;
    status_t                mStatus;
    Mutex                   mLock;
};


#endif //_BUFFER_MANAGER_H
