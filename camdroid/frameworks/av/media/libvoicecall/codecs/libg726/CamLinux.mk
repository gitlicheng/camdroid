LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	g72x.c \
	g711.c \
	g726_16.c \
	g726_24.c \
	g726_32.c \
	g726_40.c \
	codec_g726.c

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/include

LOCAL_MODULE := libg726

include $(BUILD_STATIC_LIBRARY)
#include $(BUILD_SHARED_LIBRARY)