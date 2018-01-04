LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
	frameworks/av/media/CedarX-Projects/CedarX/libcodecs/CODEC/VIDEO/DECODER \

LOCAL_SRC_FILES := \
    jpegDecoder.cpp

LOCAL_SHARED_LIBRARIES := \
	libutils \
    libvdecoder \

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE:= libjpegdecoder

include $(BUILD_SHARED_LIBRARY)

