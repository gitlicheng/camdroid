#ifndef __CDR_RTSP_SERVER_H__
#define __CDR_RTSP_SERVER_H__

#include <platform.h>

#define SESSION_VIDEO_H264       0
#define SESSION_AUDIO_AMR        1

typedef struct __rtsp_data {
	int len;
	long long pts;
	char* frame;
} __attribute__((packed, aligned(1)))rtsp_data;


typedef int (*RtspStatusCB)(void *cookie, int status, void * argv[]);

#ifdef APP_WIFI
void *rtsp_server_init();
void* rtsp_create_session(void* handle, char* sessionName, int streamType);
int get_rtsp_url(void *handle, void *p_session, char *url, char *ipaddr);
int send_rtsp_data(void* session, int cmd, void* data, int len);
int rtsp_server_start(void *handler);
int set_status_callback(void *arg, void *p_session, void *cookie, RtspStatusCB callback);

void destroy_rtsp_server(void* handle);
#else
#define rtsp_server_init() 0
#define rtsp_create_session(x,y,z) 0
#define get_rtsp_url(x,y,z,zz) 0
#define send_rtsp_data(x,y,z,zz) 0
#define rtsp_server_start(z) 0
#define set_status_callback(x,y,z,zz) 0
#define destroy_rtsp_server(x,y,z,zz) 0
#endif

#endif
