LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:=                     \
        ColorConverter.cpp

LOCAL_C_INCLUDES := \
        $(TOP)/frameworks/include/media/openmax \
        $(TOP)/hardware/msm7k

LOCAL_SHARED_LIBRARIES := \
	libutils          \
	libcutils         \

LOCAL_MODULE:= libstagefright_color_conversion

include $(BUILD_SHARED_LIBRARY)
