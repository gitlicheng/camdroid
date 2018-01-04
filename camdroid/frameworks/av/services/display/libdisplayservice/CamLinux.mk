LOCAL_PATH:= $(call my-dir)

#
# libdisplayservice
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=               \
    DisplayService.cpp \
    DisplayClient.cpp 
	
LOCAL_SHARED_LIBRARIES:= \
    libutils \
    libbinder \
    libcutils \
    libdisplay_client \
    libhwdisp

LOCAL_C_INCLUDES += \
    frameworks/av/display

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE:= libdisplayservice

include $(BUILD_SHARED_LIBRARY)
