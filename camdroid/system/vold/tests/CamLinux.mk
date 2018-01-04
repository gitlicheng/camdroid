LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
        mysu.c \
    		
LOCAL_SHARED_LIBRARIES:= \
	libutils \
	libcutils \

LOCAL_ARM_MODE := arm	
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE:= mytest

include $(BUILD_EXECUTABLE)
