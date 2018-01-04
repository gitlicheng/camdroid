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

#include <strings.h>

#include "AudioCodec.h"

#ifdef USE_CODEC_AAC
extern AudioEncoder *newAacEncoder();
extern AudioDecoder *newAacDecoder();
#endif
#ifdef USE_CODEC_AMR
extern AudioEncoder *newAmrEncoder();
extern AudioDecoder *newAmrDecoder();
#endif
#ifdef USE_CODEC_OPUS
extern AudioEncoder *newOpusEncoder();
//extern AudioDecoder *newOpusDecoder();
#endif
#ifdef USE_CODEC_G726
extern AudioEncoder *newG726Encoder();
extern AudioDecoder *newG726Decoder();
#endif

struct AudioCodecType {
    const char *name;
    AudioEncoder *(*createEncoder)();
    AudioDecoder *(*createDecoder)();
} gAudioCodecTypes[] = {
#ifdef USE_CODEC_AAC
    {"AAC", newAacEncoder, newAacDecoder},
#endif
#ifdef USE_CODEC_AMR
    {"AMR", newAmrEncoder, newAmrDecoder},
#endif
#ifdef USE_CODEC_OPUS
    {"OPUS", newOpusEncoder, NULL},
#endif
#ifdef USE_CODEC_G726
    {"G726", newG726Encoder, newG726Decoder},
#endif
    //{"GSM-EFR", newGsmEfrEncoder, newGsmEfrDecoder},
    {NULL, NULL, NULL},
};

AudioEncoder *newAudioEncoder(const char *encName)
{
    AudioCodecType *type = gAudioCodecTypes;
    while (type->name != NULL) {
        if (strcasecmp(encName, type->name) == 0) {
            AudioEncoder *encoder = type->createEncoder();
            encoder->name = type->name;
            return encoder;
        }
        ++type;
    }
    return NULL;
}

AudioDecoder *newAudioDecoder(const char *decName)
{
    AudioCodecType *type = gAudioCodecTypes;
    while (type->name != NULL) {
        if (strcasecmp(decName, type->name) == 0) {
            AudioDecoder *decoder = type->createDecoder();
            decoder->name = type->name;
            return decoder;
        }
        ++type;
    }
    return NULL;
}

