//#define LOG_NDEBUG 0
#define LOG_TAG "BufferManager"
#include <utils/Log.h>
#include <stdio.h>
#include <stdlib.h>
#include "BufferManager.h"

BufferManager::BufferManager()
{
    mStatus = false;
}

BufferManager::BufferManager(uint32_t bufferCount, uint32_t bufferSize)
{
    mStatus = false;
    bufferAlloc(bufferCount, bufferSize);
}

BufferManager::~BufferManager()
{
    bufferDeAlloc();
}

status_t BufferManager::initCheck() const
{
    return mStatus;
}

bool BufferManager::isAudioBufferQueueEmpty()
{
    Mutex::Autolock autoLock(mLock);
    return mFilledQueue.empty();
}

long BufferManager::getFirstAudioBufferPts()
{
    Mutex::Autolock autoLock(mLock);

    if (!mFilledQueue.empty()) {
        AudioBuffer *buffer = *(mFilledQueue.begin());
        return buffer->pts;
    } else {
        return -1;
    }
}

AudioBuffer *BufferManager::getFirstAudioBuffer()
{
    Mutex::Autolock autoLock(mLock);

    if (!mFilledQueue.empty()) {
        AudioBuffer *buffer = *(mFilledQueue.begin());
        ALOGV("getFirstAudioBuffer: buffer(%p), size(%d)", buffer, mFilledQueue.size());
        mFilledQueue.erase(mFilledQueue.begin());
        return buffer;
    } else {
        ALOGV("FilledQueue is empty, EmptyQueue size is %d", mEmptyQueue.size());
        return NULL;
    }
}

status_t BufferManager::pushAudioBuffer(AudioBuffer *buffer)
{
    Mutex::Autolock autoLock(mLock);
    ALOGV("pushAudioBuffer: buffer(%p), size(%d)", buffer, mFilledQueue.size());
    mFilledQueue.push_back(buffer);
    return OK;
}

AudioBuffer *BufferManager::getFirstEmptyBuffer()
{
    Mutex::Autolock autoLock(mLock);
    AudioBuffer *buffer;
    if (!mEmptyQueue.empty()) {
        buffer = *(mEmptyQueue.begin());
        ALOGV("getFirstEmptyBuffer: buffer(%p), size(%d)", buffer, mEmptyQueue.size());
        mEmptyQueue.erase(mEmptyQueue.begin());
    } else {
        ALOGV("EmptyQueue is empty, FilledQueue size is %d", mFilledQueue.size());
        buffer = *(mFilledQueue.begin());
        mFilledQueue.erase(mFilledQueue.begin());
        //memset((char *)buffer, 0x00, mBufferSize);
    }
    return buffer;
}

status_t BufferManager::pushEmptyBuffer(AudioBuffer *buffer)
{
    Mutex::Autolock autoLock(mLock);
    ALOGV("pushEmptyBuffer: buffer(%p), size(%d)", buffer, mEmptyQueue.size());
    //memset((char *)buffer, 0x00, mBufferSize);
    mEmptyQueue.push_back(buffer);
    return OK;
}

status_t BufferManager::bufferAlloc(uint32_t bufferCount, uint32_t bufferSize)
{
    Mutex::Autolock autoLock(mLock);

    mBufferSize = sizeof(AudioBuffer) + bufferSize;
    for (uint32_t i = 0; i < bufferCount; i++) {
        AudioBuffer *buffer = (AudioBuffer *)malloc(mBufferSize);
        ALOGV("bufferAlloc: malloc buffer(%p).", buffer);
        if (!buffer) {
            ALOGE("bufferAlloc: malloc buffer failed.");
            bufferDeAlloc_l();
            return -ENOMEM;
        }
        memset((char *)buffer, 0x0, mBufferSize);
        mEmptyQueue.push_back(buffer);
        mFilledQueue.clear();
        mBufPool.push_back(buffer);
    }

    mStatus = true;

    return OK;
}

void BufferManager::bufferDeAlloc_l()
{
    while (!mBufPool.empty()) {
        AudioBuffer *buffer = *(mBufPool.begin());
        ALOGV("Removing buffer(%p) from Buffer Pool", buffer);
        mBufPool.erase(mBufPool.begin());
        free(buffer);
    }

    mEmptyQueue.clear();
    mFilledQueue.clear();
}

void BufferManager::bufferDeAlloc()
{
    Mutex::Autolock autoLock(mLock);
    bufferDeAlloc_l();
}

status_t BufferManager::bufferFlush()
{
    Mutex::Autolock autoLock(mLock);

    mEmptyQueue.clear();
    mFilledQueue.clear();

    List<AudioBuffer *>::iterator it = mBufPool.begin();
    for (; it!=mBufPool.end(); ++it) {
        memset((char *)(*it), 0x00, mBufferSize);
        mEmptyQueue.push_back(*it);
    }

    return OK;
}
