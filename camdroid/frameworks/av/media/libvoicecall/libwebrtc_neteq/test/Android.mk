# Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../android-webrtc.mk

LOCAL_ARM_MODE := arm
#LOCAL_MODULE_CLASS := EXECUTABLE
LOCAL_MODULE := neteq_test
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc

LOCAL_SRC_FILES := \
	audio_loop.cc \
	rtp_generator.cc \
	neteq_speed_test.cc

# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS) \
    '-DNETEQ_VOICEENGINE_CODECS'

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/ \
    $(LOCAL_PATH)/../include \
    $(LOCAL_PATH)/../src \
    $(LOCAL_PATH)/../src/modules/audio_coding/neteq \
    $(LOCAL_PATH)/../src/modules/audio_coding/neteq/interface \
	

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport

LOCAL_SHARED_LIBRARIES += libwebrtc_neteq
	
ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif

include $(BUILD_EXECUTABLE)

#opus test
include $(CLEAR_VARS)

include $(LOCAL_PATH)/../android-webrtc.mk

LOCAL_ARM_MODE := arm
#LOCAL_MODULE_CLASS := EXECUTABLE
LOCAL_MODULE := opus_test
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc

LOCAL_SRC_FILES := \
	opus_test.cc

# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/ \
    $(LOCAL_PATH)/../include \
    $(LOCAL_PATH)/../src \
    $(LOCAL_PATH)/../src/modules/audio_coding/neteq \
    $(LOCAL_PATH)/../src/modules/audio_coding/neteq/interface \
	
LOCAL_STATIC_LIBRARIES += libwebrtc_opus libopus

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport \

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif

include $(BUILD_EXECUTABLE)