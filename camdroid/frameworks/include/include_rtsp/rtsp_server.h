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

typedef int (*RtspServerStatusCallback)(void *cookie, int status, void *argv[]);
//typedef long long (*RtspServerGetPTS)(void *cookie);
typedef int (*RtspServerGetStreamData)(void *cookie, unsigned char *buffer, unsigned int size, long long *pts);

void *rtspServerInit();			//Init RTSP environment
void rtspServerDestroy(void *handle);		//Destroy RTSP environment
void *rtspServerCreateSession(void *handle, const char *session_name);
int rtspServerExitSession(void *handle, void *p_session);
int rtspServerGetUrl(void *handle, void *p_session, char *url, char *ipAddr); // 修改，添加ipAddr参数
int rtspServerSetStatusCallback(void *handle, void *p_session, void *cookie, RtspServerStatusCallback callback);
//int rtspServerSetPTSCallback(void *p_session, void *cookie, RtspServerGetPTS callback);
int rtspServerSetStreamDataCallback(void *handle, void *p_session, void *cookie, RtspServerGetStreamData callback);
int rtspServerStart(void *handle);			//Start RTSP message loop
int rtspServerStop(void *handle);			//Stop RTSP message loop
int rtspServerSendNotify(void *handle, void *p_session, int msg, int arg);
//void resetRTSPServerTimeScheduler();


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //__RTSP_SERVER_INTERFACE_H__

