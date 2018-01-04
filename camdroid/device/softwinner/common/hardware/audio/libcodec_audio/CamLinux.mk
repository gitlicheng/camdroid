# Copyright (C) 2011 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libcodec_audio
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/
LOCAL_SRC_FILES := codec.c \
			 pcm.c mixer.c \
			codec_devices.c \
			codec_utils.c volume_conf.c\
			wakelock.cpp \
			record.c wav.c \
			codec_devices/planV3/plan_v3.c \
		
#			codec_devices/pad/pad.c \
#			codec_devices/planOne/plan_one.c \
#			codec_devices/planOne/manage.c	\
#			codec_devices/planTwo/plan_two.c \
#			codec_devices/planTwo/manage.c	\


LOCAL_C_INCLUDES += \
	system/media/audio_utils/include \
	frameworks/native/include \
	$(LOCAL_PATH)/
	
LOCAL_SHARED_LIBRARIES += liblog libcutils libutils libdl libaudioutils libbinder libpowermanager

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)





