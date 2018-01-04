#ifndef __HTTP_SRV_H__
#define __HTTP_SRV_H__

#include "http.h"

#ifndef WRITE_ERROR
#define WRITE_ERROR -1
#endif

#ifndef WRITE_SUCCESS
#define WRITE_SUCCESS 0
#endif

#ifndef WRITE_START
#define WRITE_START 1
#endif

#ifndef WRITE_PUASE
#define WRITE_PUASE 2
#endif

#ifdef __cplusplus
extern "C" 
{
#endif

/*
** http_srv_init()用于初始化
** http_srv_open(http_srv_t *srv)用于打开http服务,参数srv由http_srv_init函数返回获得
** http_srv_close(http_srv_t *srv)用于关闭http服务
** http_srv_release(http_srv_t *srv)用于释放资源
** getURL(http_srv_t *srv, char *path_ptr)用于获取URL，path是调用者提供的资源路径
**
*/
//int get_url(http_srv_t *srv, char *out, char *path);
http_srv_t *http_srv_init();
int http_srv_open(http_srv_t *srv, char *local_ip);
//int http_srv_open(http_srv_t *srv);
int http_srv_close(http_srv_t *srv);
void http_srv_release(http_srv_t *srv);
int http_srv_getURL(http_srv_t *srv, const char *path_ptr, const char * ip, char * url);
int http_srv_get_abspath(char *out, char *uri);

//设置写数据前的休眠值，以毫秒为单位，2015.1.5新增接口
void setWriteSleepTime(int usec);

/*
** 通知获取数据或者获取结束，获取数据时，传递整型值1，获取结束时，传递整型值0, 获取数据中止时，传递整型值2
** typedef void (*GetDataOrEnd)(void * cookies, char *uri, int flag);
*/
//这个接口弃用
void registerGetDataOrEnd(http_srv_t * srv, void * cookies, GetDataOrEnd _getDataOrEnd);






int http_srv_write(int socket, char * httpHeader, unsigned char * IPCbuffer, int buff_size); //新增接口，用于向网络写Buffer

//typedef void (*StartWrite)(void * cookies, char * uri, char * header, int socket); 通知IPC应用写数据
void registerStartWrite(http_srv_t * srv, void * cookies, StartWrite _startWrite);

//typedef void (*BufferSize)(void * cookies, char * uri, int *size);  IPC告知要写数据的长度
void registerBufferSize(http_srv_t * srv, void * cookies, BufferSize _bufferSize);






#ifdef __cplusplus
}
#endif

#endif //__HTTP_SRV_H__
