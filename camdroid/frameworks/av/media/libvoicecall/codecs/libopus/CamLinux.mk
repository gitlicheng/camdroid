LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

include $(LOCAL_PATH)/opus_sources.mk
include $(LOCAL_PATH)/silk_sources.mk
include $(LOCAL_PATH)/celt_sources.mk


LOCAL_SRC_FILES := $(OPUS_SOURCES) $(SILK_SOURCES) $(SILK_SOURCES_FIXED) $(CELT_SOURCES)

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/include \
		$(LOCAL_PATH)/silk \
		$(LOCAL_PATH)/celt \
		$(LOCAL_PATH)/silk/fixed \

LOCAL_CFLAGS := -D__EMX__ -DOPUS_BUILD -DDISABLE_FLOAT_API -DFIXED_POINT -DUSE_ALLOCA -DHAVE_LRINT -DHAVE_LRINTF -O3 -fno-math-errno
#LOCAL_CFLAGS := -DHAVE_CONFIG_H

LOCAL_MODULE := libopus

include $(BUILD_STATIC_LIBRARY)
