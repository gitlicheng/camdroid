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
LOCAL_MODULE := libwebrtc_g726
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := g726_interface.c

# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include \
    $(MY_WEBRTC_ROOT_PATH)/include \
    $(MY_WEBRTC_ROOT_PATH)/src \
    $(CODEC_PATH)/libg726/include \
    $(CODEC_PATH)/libplc

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport

#LOCAL_WHOLE_STATIC_LIBRARIES += libg726

#LOCAL_LDFLAGS += $(CODEC_PATH)/libplc/libplc.a

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)
