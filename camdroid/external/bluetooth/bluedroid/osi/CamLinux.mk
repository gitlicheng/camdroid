LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include

LOCAL_SRC_FILES := \
    ./src/alarm.c \
    ./src/config.c \
    ./src/fixed_queue.c \
    ./src/list.c \
    ./src/reactor.c \
    ./src/semaphore.c \
    ./src/thread.c

LOCAL_CFLAGS := -std=c99 -Wall -Werror
LOCAL_MODULE := libosi
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libc liblog
LOCAL_MODULE_CLASS := STATIC_LIBRARIES

include $(BUILD_STATIC_LIBRARY)
