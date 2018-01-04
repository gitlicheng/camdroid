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

#ifndef CEDARX_METADATA_RETRIEVER_H_

#define CEDARX_METADATA_RETRIEVER_H_

#include <media/MediaMetadataRetrieverInterface.h>

//#include <media/stagefright/OMXClient.h>
#include <utils/KeyedVector.h>
#include <CDX_PlayerAPI.h>
#include <GetAudio_format.h>
#include <CDX_Common.h>

namespace android
{

struct CedarXMetadataRetriever : public MediaMetadataRetrieverInterface
{
    CedarXMetadataRetriever();
    virtual ~CedarXMetadataRetriever();

    virtual status_t setDataSource(const char *url, const KeyedVector<String8, String8> *headers);

    virtual status_t setDataSource(int fd, int64_t offset, int64_t length);

#ifdef __ANDROID_VERSION_2_3_1
    virtual VideoFrame *captureFrame();
#else
    virtual VideoFrame *getFrameAtTime(int64_t timeUs, int option);
#endif
    virtual MediaAlbumArt *extractAlbumArt();
    virtual const char *extractMetadata(int keyCode);
    virtual sp<IMemory> getStreamAtTime(int64_t timeUs);

private:
    CDXRetriever*             mRetriever;
    bool                      bCDXMetaRetriverInit;
    bool                      mParsedMetaData;
    KeyedVector<int, String8> mMetaData;
    MediaAlbumArt*            mAlbumArt;

    void parseMetaData();

    CedarXMetadataRetriever(const CedarXMetadataRetriever &);
    CedarXMetadataRetriever &operator=(const CedarXMetadataRetriever &);
};


};  // namespace android

#endif  // STAGEFRIGHT_METADATA_RETRIEVER_H_
