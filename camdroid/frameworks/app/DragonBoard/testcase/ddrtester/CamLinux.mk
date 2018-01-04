LOCAL_PATH := $(call my-dir)

############################################
include $(CLEAR_VARS)

LOCAL_MODULE := ddrtester
LOCAL_SRC_FILES := ddrtester.c

LOCAL_C_INCLUDES := $(TOP)/frameworks/app/DragonBoard/inc
LOCAL_SHARED_LIBRARIES := libcutils libhardware
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
