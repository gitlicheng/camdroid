//#define LOG_NDEBUG 0
#define LOG_TAG "MediaCallbackDispatcher"
#include <utils/threads.h>
#include <utils/Log.h>

#include "MediaCallbackDispatcher.h"

namespace android {

bool MediaCallbackDispatcherThread::threadLoop() {
    return mDispatcher->loop();
}


MediaCallbackDispatcher::MediaCallbackDispatcher()
    : mDone(false) {
    mThread = new MediaCallbackDispatcherThread(this);
    mThread->run("MediaCallbackDisp", PRIORITY_DEFAULT);
}

MediaCallbackDispatcher::~MediaCallbackDispatcher() {
    {
        Mutex::Autolock autoLock(mLock);

        mDone = true;
        mQueueChanged.signal();
    }

    // A join on self can happen if the last ref to CallbackDispatcher
    // is released within the CallbackDispatcherThread loop
    status_t status = mThread->join();
    if (status != WOULD_BLOCK) {
        // Other than join to self, the only other error return codes are
        // whatever readyToRun() returns, and we don't override that
        if(status != (status_t)NO_ERROR)
        {
            ALOGE("(f:%s, l:%d) status[0x%x]!=NO_ERROR", __FUNCTION__, __LINE__, status);
        }
    }
}

void MediaCallbackDispatcher::post(const MediaCallbackMessage &msg) {
    Mutex::Autolock autoLock(mLock);

    mQueue.push_back(msg);
    mQueueChanged.signal();
}

void MediaCallbackDispatcher::dispatch(const MediaCallbackMessage &msg) {
    handleMessage(msg);
}

bool MediaCallbackDispatcher::loop() {
    for (;;) {
        MediaCallbackMessage msg;

        {
            Mutex::Autolock autoLock(mLock);
            while (!mDone && mQueue.empty()) {
                mQueueChanged.wait(mLock);
            }

            if (mDone) {
                break;
            }

            msg = *mQueue.begin();
            mQueue.erase(mQueue.begin());
        }

        dispatch(msg);
    }

    return false;
}

void MediaCallbackDispatcher::removeCallbacksAndMessages(MediaCallbackMessage* token)
{
    ALOGW("(f:%s, l:%d) sorry, don't implement this function now. If necessary, we will implement it", __FUNCTION__, __LINE__);
}

};

