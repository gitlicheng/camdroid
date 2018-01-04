/*
 * Copyrightm (C) 2010 The Android Open Source Project
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

#include <stdint.h>

#ifndef __AUDIO_CODEC_H__
#define __AUDIO_CODEC_H__

typedef struct Codec_Params_t {
    char                name[16];
    int                 sampleRate;
    int                 channels;
    //int                 bitRate;
    int                 mode;
}CodecParams;

class AudioEncoder
{
public:
    const char *name;
    // Needed by destruction through base class pointers.
    virtual ~AudioEncoder() {}
    // Returns sampleCount or non-positive value if unsupported.
    virtual int set(int sampleRate, const int channels, const int mode) = 0;
    // Returns the length of payload in bytes.
    virtual int encode(void *payload, int16_t *samples) = 0;
	// Returns the encoder output max size
	virtual int getEncodedFrameMaxSize() = 0;
};

AudioEncoder *newAudioEncoder(const char *codecName);



class AudioDecoder
{
public:
    const char *name;
    // Needed by destruction through base class pointers.
    virtual ~AudioDecoder() {}
    // Returns sampleCount or non-positive value if unsupported.
    virtual int set(int sampleRate, const int channels, const int mode) = 0;
    // Returns the number of decoded samples.
    virtual int decode(int16_t *samples, void *payload, int length) = 0;
    //Returns non-positive value if decoder PLC failed
    virtual int decodePLC(int16_t *samples, int16_t number_of_lost_frames) = 0;
    // Returns the per packet max size for decoder
	virtual int getEncodedFrameMaxSize() = 0;
};

AudioDecoder *newAudioDecoder(const char *codecName);

#endif
