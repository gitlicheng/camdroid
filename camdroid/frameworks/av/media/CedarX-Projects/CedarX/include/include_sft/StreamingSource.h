/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef STREAMING_SOURCE_H_

#define STREAMING_SOURCE_H_

#include "NuPlayer.h"
#include "NuPlayerSource.h"
#include "NuPlayerStreamListener.h"

namespace android {

struct ABuffer;
struct ATSParser;

struct StreamingSource : public Source
{
    StreamingSource(const sp<IStreamSource> &source);

    virtual void start(int numBuffer, int bufferSize);

    virtual void stop();
    virtual void clearBuffer();
    virtual status_t feedMoreTSData() {return OK;};

    virtual sp<MetaData> getFormat(bool audio) { return NULL;};
    virtual status_t dequeueAccessUnit(bool audio, sp<ABuffer> *accessUnit) {return OK;};
    virtual int dequeueAccessData(char *accessData, int size);
protected:
    virtual ~StreamingSource();

private:
    sp<IStreamSource> mSource;
    status_t mFinalResult;
    sp<NuPlayerStreamListener> mStreamListener;
    Mutex mLock;
    DISALLOW_EVIL_CONSTRUCTORS(StreamingSource);
};

}  // namespace android

#endif  // STREAMING_SOURCE_H_
