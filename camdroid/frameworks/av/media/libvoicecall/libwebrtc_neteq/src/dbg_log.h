/*
 * Author: yewenliang@allwinnertech.com
 * Copyright (C) 2014 Allwinner Technology Co., Ltd.
 */
#ifndef _DEBUG_LOG_H
#define _DEBUG_LOG_H

#if 0
#include <android/log.h>

#ifndef LOG_NDEBUG
#define LOG_NDEBUG 1
#endif

#ifndef LOG_TAG
#define LOG_TAG NULL
#endif

#ifdef _ANDROID_LOG_H
#ifndef ALOGV
#if LOG_NDEBUG
#define ALOGV(...)   ((void)0)
#else
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#endif
#endif
#ifndef ALOGI
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#endif
#ifndef ALOGD
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#endif
#ifndef ALOGW
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#endif
#ifndef ALOGE
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#endif
#else
#define ALOGV(...)
#define ALOGI(...)
#define ALOGD(...)
#define ALOGW(...)
#define ALOGE(...)
#endif

#else
#include <utils/Log.h>
#endif

#endif //_DEBUG_LOG_H
