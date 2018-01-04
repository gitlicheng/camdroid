# Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

WEBRTC_ROOT_PATH := $(call my-dir)
# voice
include $(WEBRTC_ROOT_PATH)/src/common_audio/signal_processing/Android.mk
#include $(WEBRTC_ROOT_PATH)/src/system_wrappers/source/Android.mk
include $(WEBRTC_ROOT_PATH)/src/modules/audio_coding/neteq/Android.mk
include $(WEBRTC_ROOT_PATH)/src/modules/audio_coding/codecs/cng/Android.mk
include $(WEBRTC_ROOT_PATH)/src/modules/audio_coding/codecs/amrnb/Android.mk
include $(WEBRTC_ROOT_PATH)/src/modules/audio_coding/codecs/opus/Android.mk
include $(WEBRTC_ROOT_PATH)/src/modules/audio_coding/codecs/g726/Android.mk

## build .so
#LOCAL_PATH := $(WEBRTC_ROOT_PATH)
#
#include $(CLEAR_VARS)
#include $(LOCAL_PATH)/android-webrtc.mk
#
#LOCAL_ARM_MODE := arm
#LOCAL_MODULE_CLASS := STATIC_LIBRARIES
#LOCAL_MODULE := libwebrtc_neteq
#LOCAL_MODULE_TAGS := optional
#LOCAL_SRC_FILES := src/WebrtcNeteq_Interface.c
#
#LOCAL_C_INCLUDES := $(LOCAL_PATH)/src $(LOCAL_PATH)/include
#
##LOCAL_STATIC_LIBRARIES
#LOCAL_WHOLE_STATIC_LIBRARIES := \
#	libwebrtc_neteq_internal \
#	libwebrtc_amrnb \
#	libwebrtc_cng \
#	libwebrtc_opus \
#	libwebrtc_g726 \
#	libwebrtc_spl
#
## Add Neon libraries.
#ifeq ($(WEBRTC_BUILD_NEON_LIBS),true)
##LOCAL_STATIC_LIBRARIES
#LOCAL_WHOLE_STATIC_LIBRARIES += \
#    libwebrtc_spl_neon
#endif
#
#LOCAL_SHARED_LIBRARIES := \
#    libcutils \
#    libdl \
#    libstlport
#
##LOCAL_STATIC_LIBRARIES
##LOCAL_WHOLE_STATIC_LIBRARIES += \
##	libstagefright_amrnbdec \
##	libstagefright_amrnb_common \
##	libopus \
##	libg726
#
##now only support one codec
#CONFIG_CODEC := AMR OPUS G726
#
#ifeq ($(findstring AMR,$(CONFIG_CODEC)),AMR)
#LOCAL_CFLAGS += -DUSE_CODEC_AMR
#endif
#ifeq ($(findstring OPUS,$(CONFIG_CODEC)),OPUS)
#LOCAL_CFLAGS += -DUSE_CODEC_OPUS
#endif
#ifeq ($(findstring G726,$(CONFIG_CODEC)),G726)
#LOCAL_CFLAGS += -DUSE_CODEC_G726
#endif
#
##LOCAL_LDFLAGS += -Wl,--whole-archive $(LOCAL_PATH)/../libcodecs/libplc/libplc.a
##LOCAL_LDFLAGS += $(LOCAL_PATH)/../libcodecs/libplc/libplc.a
#
#LOCAL_PRELINK_MODULE := false
#
#ifndef NDK_ROOT
#include external/stlport/libstlport.mk
#endif
#
##include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)