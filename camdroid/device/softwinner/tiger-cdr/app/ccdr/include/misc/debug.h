#ifndef __DEBUG_H__
#define __DEBUG_H__

#define SYS_LOG

#ifdef SYS_LOG
extern "C"
{
	void klog_write(int level, const char *fmt, ...);
	void klog_init(void);
	void klog_close(void);
}

#define KLOG_ERROR(tag,x...)   klog_write(3, "<3>" tag ": " x)
#define sys_log(x...)   KLOG_ERROR("¡ï¡¤newcdr¡¤¡ï", x)
#define sys_log_init() klog_init()
#else
#define sys_log(x...)
#define sys_log_init()
#endif

/* 
 * if USE_ANDROID_DEBUG is set to 1, then use the android log mechanism
 * if USE_ANDROID_DEBUG is set to 0, then ues the fprint instead
 * */
#define USE_ANDROID_DEBUG	1

/* if DB_LOG_LEVEL set to 1, then all the debug info is included */
#define DB_LOG_LEVEL	1
#undef	DB_MSG
#undef	DB_DEBUG
#undef	DB_INFO
#undef	DB_WARN
#undef	DB_ERROR

#if DB_LOG_LEVEL	== 5
#define DB_ERROR
#elif DB_LOG_LEVEL	== 4
#define DB_WARN
#define DB_ERROR
#elif DB_LOG_LEVEL	== 3
#define DB_INFO
#define DB_WARN
#define DB_ERROR
#elif DB_LOG_LEVEL == 2
#define DB_DEBUG
#define DB_INFO
#define DB_WARN
#define DB_ERROR
#elif DB_LOG_LEVEL == 1
#define DB_MSG
#define DB_DEBUG
#define DB_INFO
#define DB_WARN
#define DB_ERROR
#endif

#if DB_LOG_LEVEL > 0
#undef LOG_NDEBUG
#undef NDEBUG
#else
#define LOG_NDEBUG
#define NDEBUG
#endif

#if USE_ANDROID_DEBUG
#include <cutils/log.h>
#endif

#if USE_ANDROID_DEBUG

#ifdef DB_ERROR
#define db_error(fmt, arg...)	\
	do { \
		ALOG(LOG_ERROR, LOG_TAG, "<line[%04d] %s()> " fmt, __LINE__, __FUNCTION__, ##arg); \
	} while(0)
#else
#define db_error(fmt, arg...)
#endif

#ifdef DB_WARN
#define db_warn(fmt, arg...) \
	do { \
		ALOG(LOG_WARN, LOG_TAG, "<line[%04d] %s()> " fmt, __LINE__, __FUNCTION__, ##arg); \
	} while(0)
#else
#define db_warm(fmt, arg...)
#endif

#ifdef DB_INFO
#define db_info(fmt, arg...)	\
	do { \
		ALOG(LOG_INFO, LOG_TAG, "<line[%04d] %s()> " fmt, __LINE__, __FUNCTION__, ##arg); \
	} while(0)
#else
#define db_info(fmt, arg...)
#endif

#ifdef DB_DEBUG
#define db_debug(fmt, arg...)	\
	do { \
		ALOG(LOG_DEBUG, LOG_TAG, "<line[%04d] %s()> " fmt, __LINE__, __FUNCTION__, ##arg); \
	} while(0)
#else
#define db_debug(fmt, arg...)
#endif

#ifdef DB_MSG
#define db_msg(fmt, arg...)	\
	do { \
		ALOG(LOG_VERBOSE, LOG_TAG, "<line[%04d] %s()> " fmt, __LINE__, __FUNCTION__, ##arg); \
	} while(0)
#else
#define db_msg(fmt, arg...)
#endif

#else

#ifdef DB_ERROR
#define db_error(fmt, ...) \
	do { fprintf(stderr, LOG_TAG "(error): line[%04d] %s() ", __LINE__, __FUNCTION__); fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)
#else
#define db_error(fmt, ...)
#endif /* DB_ERROR */

#ifdef DB_WARN
#define db_warn(fmt, ...) \
	do { fprintf(stdout, LOG_TAG "(warn): line[%04d] %s()", __LINE__, __FUNCTION__); fprintf(stdout, fmt, ##__VA_ARGS__); } while (0)
#else
#define db_warn(fmt, ...)
#endif /* DB_WARN */

#ifdef DB_INFO
#define db_info(fmt, ...) \
	do { fprintf(stdout, LOG_TAG "(info): line[%04d] %s()", __LINE__, __FUNCTION__); fprintf(stdout, fmt, ##__VA_ARGS__); } while (0)
#else
#define db_info(fmt, ...)
#endif /* DB_INFO */

#ifdef DB_DEBUG
#define db_debug(fmt, ...) \
	do { fprintf(stdout, LOG_TAG "(debug): "); fprintf(stdout, fmt, ##__VA_ARGS__); } while (0)
#else
#define db_debug(fmt, ...)
#endif /* DB_DEBUG */

#ifdef DB_MSG
#define db_msg(fmt, ...) \
	do { fprintf(stdout, LOG_TAG "(msg): line:[%04d] %s() ", __LINE__, __FUNCTION__); fprintf(stdout, fmt, ##__VA_ARGS__); } while (0)
#else
#define db_msg(fmt, ...)
#endif /* DB_MSG */

#endif


#endif		/* __DEBUG_H__ */
