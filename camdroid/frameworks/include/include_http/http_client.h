#ifndef __HTTP_CLT_H__
#define __HTTP_CLT_H__

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <errno.h> 
#include <unistd.h> 
#include <netinet/in.h>
#include <limits.h> 
#include <netdb.h> 
#include <arpa/inet.h> 
#include <ctype.h>  

#ifdef __cplusplus
extern "C" 
{
#endif

/* path：文件保存路径，包含文件名  URL：下载文件的完整URL*/
int downloadFile(char *path, char *URL);

/*uri: 该字符串就是下载文件的完整URL， flag：宏DOWNLOAD_START值为1，宏DOWNLOAD_END值为0*/
typedef void (*DownloadCommand)(char *uri, int flag);
void registerDownloadCommand(DownloadCommand _downloadCommand);

#ifdef __cplusplus
}
#endif


#endif//__HTTP_CLT_H__