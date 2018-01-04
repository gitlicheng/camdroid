# Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

MY_WEBRTC_ROOT_PATH := $(LOCAL_PATH)/../../..

include $(MY_WEBRTC_ROOT_PATH)/android-webrtc.mk

LOCAL_ARM_MODE := arm
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE := libwebrtc_resampler
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
	resampler.cc \

#	libspeex/resample.c

# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS)

#LOCAL_CFLAGS += -DEXPORT= -DFIXED_POINT -DRESAMPLE_FORCE_FULL_SINC_TABLE
#LOCAL_CFLAGS += -O3 -fstrict-aliasing -fprefetch-loop-arrays
#
#ifeq ($(ARCH_ARM_HAVE_NEON),true)
#LOCAL_CFLAGS += -D_USE_NEON
#endif

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include \
    $(MY_WEBRTC_ROOT_PATH)/include \
    $(MY_WEBRTC_ROOT_PATH)/src \
    $(MY_WEBRTC_ROOT_PATH)/src/common_audio/signal_processing/include

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport \
	libspeexresampler

ifeq ($(TARGET_OS)-$(TARGET_SIMULATOR),linux-true)
LOCAL_LDLIBS += -ldl -lpthread
endif

ifneq ($(TARGET_SIMULATOR),true)
LOCAL_SHARED_LIBRARIES += libdl
endif

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)
