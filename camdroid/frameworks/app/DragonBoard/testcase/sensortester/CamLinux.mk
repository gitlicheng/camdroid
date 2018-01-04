LOCAL_PATH := $(call my-dir)

############################################
include $(CLEAR_VARS)

LOCAL_MODULE := acctester
LOCAL_SRC_FILES := sensortester.c

LOCAL_C_INCLUDES :=  $(TOP)/frameworks/app/DragonBoard/inc
LOCAL_SHARED_LIBRARIES := libcutils libhardware
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

############################################
#include $(CLEAR_VARS)
#
#LOCAL_MODULE := server
#LOCAL_SRC_FILES := server.c
#
#LOCAL_C_INCLUDES := $(TOP)/app/DragonBoard/inc
#LOCAL_MODULE_TAGS := optional
#
#include $(BUILD_EXECUTABLE)

