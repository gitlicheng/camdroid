LOCAL_PATH := $(call my-dir)
#
#include $(CLEAR_VARS)
#
#include $(call all-makefiles-under,$(LOCAL_PATH))

include $(CLEAR_VARS)

MY_WEBRTC_ROOT_PATH := $(LOCAL_PATH)/../../../../..

include $(MY_WEBRTC_ROOT_PATH)/android-webrtc.mk

CODEC_PATH := $(MY_WEBRTC_ROOT_PATH)/../codecs

LOCAL_ARM_MODE := arm
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE := libwebrtc_amrnb
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := amrnb_interface.c

# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include \
    $(MY_WEBRTC_ROOT_PATH)/include \
    $(MY_WEBRTC_ROOT_PATH)/src \
    $(CODEC_PATH)/libamrnb/common/include \
    $(CODEC_PATH)/libamrnb/dec/src \
    $(CODEC_PATH)/libamrnb/enc/src \
    $(CODEC_PATH)/libplc

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport

#LOCAL_WHOLE_STATIC_LIBRARIES += \
#	libstagefright_amrnb_common \
#	libstagefright_amrnbdec \
#	libstagefright_amrnbenc

#LOCAL_LDFLAGS += $(CODEC_PATH)/libplc/libplc.a

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)
