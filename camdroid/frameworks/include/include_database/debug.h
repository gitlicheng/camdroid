/*************************************************************************
  > File Name: debug.h
  > Description: 
		for debug options
  > Author: CaoDan
  > Mail: 464114320@qq.com 
  > Created Time: 2014年03月28日 星期五 11:43:51
 ************************************************************************/

#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifdef __cplusplus
extern "C" {
#endif


#define PRJ_NAME "cdr"

#define DB_LOG_LEVEL	4

#if DB_LOG_LEVEL	== 1
#define DB_ERROR
#elif DB_LOG_LEVEL	== 2
#define DB_ERROR
#define DB_WARN
#elif DB_LOG_LEVEL	== 3
#define DB_ERROR
#define DB_WARN
#define DB_MSG
#elif DB_LOG_LEVEL == 4
#define DB_ERROR
#define DB_WARN
#define DB_MSG
#define DB_DEBUG
#elif DB_LOG_LEVEL == 5
#define DB_ERROR
#define DB_WARN
#define DB_MSG
#define DB_DEBUG
#define DB_DUMP
#endif

#ifdef DB_ERROR
#define db_error(fmt, ...) \
	do { fprintf(stderr, PRJ_NAME "(error): line[%4d] %s() ", __LINE__, __FUNCTION__); fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)
#else
#define db_error(fmt, ...)
#endif /* DB_ERROR */

#ifdef DB_WARN
#define db_warn(fmt, ...) \
	do { fprintf(stdout, PRJ_NAME "(warn): "); fprintf(stdout, fmt, ##__VA_ARGS__); } while (0)
#else
#define db_warn(fmt, ...)
#endif /* DB_WARN */

#ifdef DB_MSG
#define db_msg(fmt, ...) \
	do { fprintf(stdout, PRJ_NAME "(msg): line:[%4d] %s() ", __LINE__, __FUNCTION__); fprintf(stdout, fmt, ##__VA_ARGS__); } while (0)
#else
#define db_msg(fmt, ...)
#endif /* DB_MSG */

#ifdef DB_DEBUG
#define db_debug(fmt, ...) \
	do { fprintf(stdout, PRJ_NAME "(debug): "); fprintf(stdout, fmt, ##__VA_ARGS__); } while (0)
#else
#define db_debug(fmt, ...)
#endif /* DB_DEBUG */

#ifdef DB_DUMP
#define db_dump(fmt, ...) \
	do { fprintf(stdout, PRJ_NAME "(dump): "); fprintf(stdout, fmt, ##__VA_ARGS__); } while (0)
#else
#define db_dump(fmt, ...)
#endif


#ifdef __cplusplus
}  /* end of extern "C" */
#endif



#endif		/* __DEBUG_H__ */

