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

LOCAL_MODULE := libril_audio

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/
#SRC_FILES := $(call find-subdir-subdir-files, bp_devices/mu509/, *.c, .h)
#SRC_FILES := $(call find-subdir-files, *.c)

#$(warning adf$(SRC_FILES))

LOCAL_SRC_FILES := bp.c  \
			bp_devices.c \
			bp_utils.c \
			bp_devices/mu509/mu509.c  \
			bp_devices/oviphone_em55/em55.c \
			bp_devices/demo/demo.c \
			bp_devices/usi6276/usi6276.c

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/
	
LOCAL_SHARED_LIBRARIES += liblog libcutils libdl
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


#$(addsuffix /java, \	    ethernet \	 )

#	external/tinyalsa/include \
#	system/media/audio_utils/include \
#	system/media/audio_effects/include \


