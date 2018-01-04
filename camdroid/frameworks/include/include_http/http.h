#ifndef __HTTP_H__
#define __HTTP_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <netinet/tcp.h>
#include "dbg_log.h"
#include "thread_pool.h"

#ifndef off64_t
typedef long long off64_t;
#endif 

#define BUF_LEN		16384
#define HTTP_MAX_URL	1024

//#undef USE_SENDFILE
//#define USE_SENDFILE
#define NET_TCP_CORK
#define MAX_THREAD 50

struct path_table {
	char path[256];
	int used_count;
	int file_id;
};

typedef struct chunk {
	const char *mem;
	off_t mem_len;
	struct chunk *next;
} chunk_t;

typedef struct file_info {
	int file_fd;
	off64_t file_len;

	off64_t start_off;
	off64_t len_off;

	time_t last_modified;	/* 最后修改时间 */
	int is_dir;				/* 是否是目录 */
	int is_readable;		/* 文件是否可读 */
	char content_typ[10];
} file_info_t;

typedef struct mime_typ {
	const char *ext;
	int ext_len;
	const char *value;
} mime_typ_t;

#if 0
struct virt_map {
	char *id;
	char *truepath;
	struct virt_map *next;
};
#endif

typedef struct keymap {
	int key;
	const char *value;
} keymap_t;

typedef enum {
	HTTP_METHOD_GET,
	HTTP_METHOD_POST,
	HTTP_METHOD_PUT,
	HTTP_METHOD_DELETE,
	HTTP_METHOD_HEAD,
	HTTP_METHOD_UNC
} method_t;

typedef enum {
	HTTP_VERSION_1_0,
	HTTP_VERSION_1_1,
	HTTP_VERSION_UNC
} http_version_t;

typedef enum {
	HTTP_STATUS_200,
	HTTP_STATUS_206,
	HTTP_STATUS_400,
	HTTP_STATUS_404,
	HTTP_STATUS_414,
	HTTP_STATUS_505,
	HTTP_STATUS_507,
	HTTP_STATUS_UNC
} http_status_t;

static keymap_t http_method[] = {
	{HTTP_METHOD_GET, "GET"},
	{HTTP_METHOD_POST,"POST"},
	{HTTP_METHOD_PUT, "PUT"},
	{HTTP_METHOD_DELETE, "DELETE"},
	{HTTP_METHOD_HEAD, "HEAD"},
	{HTTP_METHOD_UNC, NULL}
};

static keymap_t status_array[] = {
	{HTTP_STATUS_200, "200 OK"},
	{HTTP_STATUS_206, "206 Partial Content"},
	{HTTP_STATUS_400, "400 Bad Request"},
	{HTTP_STATUS_404, "404 Not Found"},
	{HTTP_STATUS_414, "414 Request-URI Too Long"},
	{HTTP_STATUS_505, "505 HTTP Version Not Supported"},
	{HTTP_STATUS_507, "507 Insufficient Storage"},
	{HTTP_STATUS_UNC, NULL},
};

static mime_typ_t mime_tab[] = {
	{"txt", 3, "txt/plain"},
	{"ms", 2, "application/x-troff-ms"},
	{"ms", 2, "application/x-troff-ms"},
	{"mv", 2, "video/x-sgi-movie"},
	{"qt", 2, "video/quicktime"},
	{"ts", 2, "video/mp2t"},
	{"3gp", 3, "video/3gp"},
	{"asf", 3, "video/x-ms-asf"},
	{"avi", 3, "video/x-msvideo"},
	{"flv", 3, "video/x-flv"},
	{"vob", 3, "video/mpeg"},
	{"mkv", 3, "video/x-matroska"},
	{"mov", 3, "video/quicktime"},
	{"mp2", 3, "audio/mpeg"},
	{"mp3", 3, "audio/mpeg"},
	{"mp4", 3, "video/mp4"},
	{"mpe", 3, "video/mpeg"},
	{"3gp", 3, "video/mpeg"},
	{"msh", 3, "model/mesh"},
	{"mpg", 3, "video/mpeg"},
	{"dat", 3, "video/mpeg"},
	{"mpeg", 4, "video/mpeg"},
	{"mpga", 4, "audio/mpeg"},
	{"m2ts", 4, "video/mp2t"},
	{"movie", 5, "video/x-sgi-movie"},
	{"png", 3, "image/png"},
	{"bmp", 3, "image/bmp"},
	{"gif", 3, "image/gif"},
	{"jpe", 3, "image/jpeg"},
	{"jpg", 3, "image/jpeg"},
	{"jpeg", 4, "image/jpeg"},
	{"jfif", 4, "image/jpeg"},
	{"ra", 2, "audio/x-realaudio"},
	{"rmvb", 4, "appllication/vnd.rn-realmedia"},
	{"rm", 2, "appllication/vnd.rn-realmedia"},
	{"mp2", 3, "audio/mpeg"},
	{"mp3", 3, "audio/mpeg"},
	{"ram", 3, "audio/x-pn-realaudio"},
	{"wav", 3, "audio/x-wav"},
	{"wax", 3, "audio/x-ms-wax"},
	{"wma", 3, "audio/x-ms-wma"},
	{"wmv", 3, "video/x-ms-wmv"},
	{"mpga", 4, "audio/mpeg"},
	{NULL, 0, NULL},
};

typedef struct http_req {
	char buffer[2048];
	method_t method;
	http_version_t http_ver;

	size_t content_len;
	size_t auth_len;

	char *uri;		/* request-uri */
	char *host;		/* the host string */
	char *user_agent;
	char *content_type;	
	char *range;	/* client request range */
	char *refer;
	char *location;
	char *cookie;
	char *tran_co;
	char *status;
	char *connection;	/* client connection infomation close or keep-alive */

	off64_t range_first;
	off64_t range_end;

	int req_end;
	int	used_pos;
} http_req_t;

typedef struct http_res {
	char buffer[1024];
	int status;

	time_t last_modified;

	off64_t content_len;
	chunk_t *head_chunk;

	file_info_t file;
} http_res_t;

typedef enum {
	CONN_STATE_ERR = -1,
	CONN_STATE_PREPARE,
	CONN_STATE_READ_REQ,
	CONN_STATE_READ_POST,
	CONN_STATE_PARSE,
	CONN_STATE_REQUEST,
	CONN_STATE_RESP,
	CONN_STATE_CLOSE
} conn_state_t;

typedef void (*GetDataOrEnd)(void * cookies, char *uri, int flag);
typedef void (*StartWrite)(void * cookies, char * uri, char * header, int socket);
typedef void (*BufferSize)(void * cookies, char * uri, int *size);

typedef struct http_srv {
	int srv_fd;
	char ip_addr[128];
	short port;
	struct sockaddr_in local_addr;
	
	pthread_t th_id;	/* accept thread */
	pthread_mutex_t mutex;
	pthread_cond_t cond_var; 
	int kill_pipe[2];

	struct th_pool *thread_pool;
	void * cookies; //IPC应用层需求
	GetDataOrEnd srv_getDataOrEnd; //IPC应用层需求
	StartWrite   srv_startWrite;   //IPC应用层需求
	BufferSize   srv_bufferSize;   //IPC应用层需求
} http_srv_t;

typedef struct conn {
	int	cli_fd;
	struct sockaddr_in dst_addr;
	int keepalive;

	http_req_t *con_req;
	http_res_t *con_res;

	int con_stat;

	struct http_srv *srv_ptr;
} conn_t;

#ifdef __cplusplus
extern "C" 
{
#endif


#ifdef __cplusplus
}
#endif

#endif //__HTTP_H__

