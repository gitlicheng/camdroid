#ifndef __DBG_LOG_H__
#define __DBG_LOG_H__

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef ANDROID
#include <cutils/log.h>
#undef DBG_TAG
#define DBG_TAG	"transfer"
#endif

#undef DEBUG
#define DEBUG

#ifdef DEBUG

#if 0
#define dbg_print(...)	\
do {						\
	ALOG(LOG_DEBUG, DBG_TAG, __VA_ARGS__);		\
} while(0)

#else						

#define dbg_print(...)	\
do {						\
	printf(__VA_ARGS__);	\
} while(0)

#endif

#else

#define dbg_print(...)	\
do {						\
} while(0)

#endif

#endif
