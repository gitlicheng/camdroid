#ifndef __RTSP_SERVER_INTERFACE_H__
#define __RTSP_SERVER_INTERFACE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef DEBUG_WITH_PLAYER
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <memory.h>
#include <string.h>

#include <fcntl.h>
#include <errno.h>
#include <dlfcn.h>
#endif

#define RTSP_SOURCE_START_STREAM 1
#define RTSP_SOURCE_STOP_STREAM 0

#define SESSION_MSG_DATA_AVAILABLE 100		//Stream data available

typedef int (*RtspServerStatusCallback)(void *cookie, int status, void * argv[]);
typedef void (*NofityRTSPStatusFunc)(void * pSession, int status); // for nat
//typedef long long (*RtspServerGetPTS)(void *cookie);
typedef int (*RtspServerGetStreamData)(void *cookie, unsigned char *buffer, unsigned int size, long long *pts);

void *rtspServerInit();			//Init RTSP environment
void rtspServerDestroy(void *handle);		//Destroy RTSP environment
void *rtspServerCreateSession(void *handle, const char *session_name);
void *rtspServerCreateAudioSession(void *handle, const char *session_name);
void *rtspServerCreateAMRAudioSession(void *handle, const char *session_name);
int rtspServerExitSession(void *handle, void *p_session);
int rtspServerGetUrl(void *handle, void *p_session, char *url, char *ipAddr); // 修改，添加ipAddr参数
int rtspServerSetStatusCallback(void *handle, void *p_session, void *cookie, RtspServerStatusCallback callback);
int rtspServerSetNotifyNatCallback(void *p_session, NofityRTSPStatusFunc callback);
int rtspServerSetStreamDataCallback(void *handle, void *p_session, void *cookie, RtspServerGetStreamData callback);
int rtspServerStart(void *handle);			//Start RTSP message loop
int rtspServerStop(void *handle);			//Stop RTSP message loop
int rtspServerSendNotify(void *handle, void *p_session, int msg, int arg);
//void resetRTSPServerTimeScheduler();
char IsRTSPHasSendPlayRespond();
void SetRTSPHasSendPlayRespond(char ch);

void setMediaSessionDest(void *p_session, char * ip, int port, int destport, int dst_rtcp_port);
void setSessionBitRateAdapt(void *p_session, int istrue);

void setSessionStreamMode(void *p_session, int mode, int width , int height, int bitrate, int framrate, int iframeinter);

void setRtspADTSInfo(void *p_session, unsigned sampleFrequency, unsigned numberChannel, unsigned profile);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //__RTSP_SERVER_INTERFACE_H__

