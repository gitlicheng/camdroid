/*
 * Copyright (C) 2009 The Android Open Source Project
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

#ifndef CEDARADECODER_H_
#define CEDARADECODER_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef long long CDA_S64;
#define CDA_TRUE  1
#define CDA_FALSE 0

enum CDA_ERROR_TYPE{
	CDA_ERROR_NOMEM = -2,
	CDA_ERROR = -1,
	CDA_OK = 0
};

enum CDA_STATE_TYPE{
	CDA_STATE_INVALID = 0,
	CDA_STATE_IDLE,
	CDA_STATE_PAUSE,
	CDA_STATE_EXCUTING
};

enum CDA_COMMANDS{
	CDA_CMD_UNKOWN = 0,
	CDA_CMD_PREPARE,
	CDA_CMD_PAUSE,
	CDA_CMD_PLAY,
	CDA_CMD_SEEK,
	CDA_CMD_STOP,
	CDA_CMD_GET_POSITION,
	CDA_CMD_GET_DURATION,

	CDA_CMD_REGISTER_CALLBACK,
	CDA_SET_DATASOURCE_URL,
	CDA_SET_DATASOURCE_FD,
};

enum CDA_EVENT_TYPE{
	CDA_EVENT_UNKOWN = 0,
	CDA_EVENT_PLAYBACK_COMPLETE,
	CDA_EVENT_AUDIORENDERINIT,
	CDA_EVENT_AUDIORENDEREXIT,
	CDA_EVENT_AUDIORENDERDATA,
	CDA_EVENT_AUDIORENDERGETSPACE,
	CDA_EVENT_AUDIORENDERGETDELAY,
	CDA_EVENT_AUDIORAWSPDIFPLAY,
};

typedef struct CedarAFdDesc{
	int	fd;   //SetDataSource FD
	long long offset;
	long long length;
}CedarAFdDesc;

typedef struct CDADecoder
{
	void *context;
	int  (*control)(void *player, int cmd, unsigned int para0, unsigned int para1);
}CDADecoder;

typedef int (*CedarACallbackType)(void *cookie, int event, void *p_event_data);

typedef struct CedarADecoderCallbackType{
	void *cookie;
	CedarACallbackType callback;
}CedarADecoderCallbackType;

int CDADecoder_Create(void **inst);
void CDADecoder_Destroy(void *inst);

#ifdef __cplusplus
}
#endif

#endif

