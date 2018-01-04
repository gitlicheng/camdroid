/*******************************************************************************
--                                                                            --
--                    CedarX Multimedia Framework                             --
--                                                                            --
--          the Multimedia Framework for Linux/Android System                 --
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                         Softwinner Products.                               --
--                                                                            --
--                   (C) COPYRIGHT 2011 SOFTWINNER PRODUCTS                   --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
*******************************************************************************/

#define LOG_NDEBUG 0
#define LOG_TAG "cedarx_outstream_http"
#include <CDX_Debug.h>

#include "cedarx_stream.h"
//#include "cedarx_stream_http.h"
#include <tsemaphore.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <linux/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

//extern long long gettimeofday_curr();
static long long last_time_pos,curr_time_pos;
int trans_bytes;

#define BROCASET_ADD	239.255.255.255
#define BROCASET_PROT	10000
#define DEFAULT_PORT	9090

static int create_socket(char *ip, short port)
{
	int ret, sock_fd, count = 0, opt = 1;
	struct sockaddr_in sc_addr;
	char buffer[1024];
	size_t size;

	sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	if(sock_fd < 0){
		LOGE("ERROR: create a socket failed\n");
		return -1;
	}

	if(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
		LOGE("ERROR: setsocketopt is failed\n");
	}

//	int snd_size = 4*1024;
//	if(setsockopt(sock_fd, SOL_SOCKET, SO_SNDBUF, &snd_size, sizeof(snd_size)) < 0){
//		LOGV("ERROR: setsocketopt is failed\n");
//	}

	/* Disable the Nagle (TCP No Delay) algorithm */
	int flag = 1;
	ret = setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag) );
	if (ret == -1) {
	  printf("Couldn't setsockopt(TCP_NODELAY)\n");
	  exit(-1);
	}

//	ret = setsockopt(sock_fd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos_t));
//	if (ret == -1) {
//	  printf("Couldn't setsockopt(IP_TOS)\n");
//	}

	memset(&sc_addr, 0, sizeof(struct sockaddr_in));
	sc_addr.sin_family = AF_INET;
	sc_addr.sin_addr.s_addr = INADDR_ANY;
	//sc_addr.sin_addr.s_addr = inet_addr(local_ip);
	sc_addr.sin_port = htons(port);

	if(bind(sock_fd, (struct sockaddr*)&sc_addr, sizeof(struct sockaddr)) < 0){
		LOGE("ERROR: bind failed %d\n",errno);
		close(sock_fd);
		return -1;
	}

	// set server addr
	sc_addr.sin_addr.s_addr = inet_addr(ip);
	sc_addr.sin_port = htons(port);

	LOGV("connect start");
	ret = connect(sock_fd, (struct sockaddr *)&sc_addr, sizeof(struct sockaddr));
	if(ret < 0){
		LOGE("ERROR: connecting is failed\n");
		return ret;
	}
	LOGV("connect end");

	return sock_fd;
}

static void destroy_socket(int fd)
{
	close(fd);
}

static cdx_off_t cdx_tell_stream_http(struct cdx_stream_info *stream)
{
	return 0;
}

#define STATISTIC_INTERVAL_SECONDS 2

static int cdx_write_stream_http(const void *ptr, size_t size, size_t nmemb, struct cdx_stream_info *stm_info)
{
	int wbytes, totalbytes, offset = 0;
	int ret;

	ret = totalbytes = size*nmemb;
	//LOGV("cdx_write_stream_http");
	while (totalbytes > 0) {
		//wbytes = write(stm_info->fd_desc.fd, (void*)((int)ptr + offset), totalbytes);
		wbytes = send(stm_info->fd_desc.fd, (void*)((int)ptr + offset), totalbytes, 0);
		if(wbytes < 0) {
			ret = -1;
			break;
		}
		totalbytes -= wbytes;
		offset += wbytes;
	}

    curr_time_pos = (long long)CDX_GetSysTimeUsMonotonic(); //gettimeofday_curr()
	if(last_time_pos == -1 || curr_time_pos - last_time_pos > STATISTIC_INTERVAL_SECONDS*1000*1000){
		LOGV("trans_bytes:%d trans rate: %d kbps", trans_bytes, trans_bytes * 8 / ((curr_time_pos - last_time_pos) / 1000) );
		trans_bytes = 0;
		last_time_pos = curr_time_pos;
	}

	//LOGD(">>>> total trans bytes:%d, c:%d",trans_bytes,size*nmemb);
	trans_bytes += size*nmemb;

	return ret;
}

static int destory_outstream_handle_http(struct cdx_stream_info *stm_info)
{
	destroy_socket(stm_info->fd_desc.fd);
	return 0;
}

int create_outstream_handle_http(struct cdx_stream_info *stm_info, CedarXDataSourceDesc *datasource_desc)
{
	char *ip_addr;

	if (!strncasecmp(datasource_desc->source_url, "http://", 7)) {
		ip_addr = datasource_desc->source_url + 7;
	}
	else {
		LOGE("source_url error");
		return -1;
	}

	last_time_pos = -1;
	trans_bytes = 0;

	stm_info->fd_desc.fd = create_socket("192.168.43.1", DEFAULT_PORT);
	LOGD("url:%s fd_desc.fd:%d",datasource_desc->source_url,stm_info->fd_desc.fd);
	//stm_info->seek  = cdx_seek_stream_http ;
	stm_info->tell  = cdx_tell_stream_http ;
	//stm_info->read  = cdx_read_stream_http ;
	stm_info->write = cdx_write_stream_http;
	//stm_info->getsize = cdx_get_stream_size_http;
	stm_info->destory = destory_outstream_handle_http;

	return 0;
}

