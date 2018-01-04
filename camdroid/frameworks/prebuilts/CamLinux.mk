# Copyright (C) 2010 The Android Open Source Project
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

#######################

ifeq ($(strip $(BOARD_HAVE_ADAS)),true)
    include $(CLEAR_VARS)
    LOCAL_MODULE_TAGS := optional
    LOCAL_PREBUILT_LIBS := libAdas.so 
    include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(BOARD_HAVE_ADAS)),true)
    include $(CLEAR_VARS)
    LOCAL_MODULE_TAGS := optional
    LOCAL_PREBUILT_LIBS := libGsensor.so 
    include $(BUILD_MULTI_PREBUILT)
endif

#######################

ifeq ($(strip $(BOARD_HAVE_FACE_DETECT)),true)
    include $(CLEAR_VARS)
    LOCAL_MODULE_TAGS := optional
    LOCAL_PREBUILT_LIBS := libfd.so 
    include $(BUILD_MULTI_PREBUILT)
endif

#######################

ifeq ($(strip $(BOARD_HAVE_MOTION_DETECT)),true)
    include $(CLEAR_VARS)
    LOCAL_MODULE := libawmd
    LOCAL_MODULE_SUFFIX := .so
    LOCAL_MODULE_TAGS := optional
    LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
ifeq ($(BOARD_TARGET_DEVICE),crane_cdr)
    LOCAL_SRC_FILES := libawmd_mini.so
else ifeq ($(BOARD_TARGET_DEVICE),crane_cdr_8M)
    LOCAL_SRC_FILES := libawmd_mini.so
else ifeq ($(BOARD_TARGET_DEVICE),tiger_cdr)
    LOCAL_SRC_FILES := libawmd_1m.so	
else ifeq ($(BOARD_TARGET_DEVICE),tiger_standard)
    LOCAL_SRC_FILES := libawmd_1m.so		
else
    LOCAL_SRC_FILES := libawmd.so
endif
    LOCAL_MODULE_CLASS := SHARED_LIBRARIES
    include $(BUILD_PREBUILT)
endif

#######################
ifeq ($(strip $(BOARD_HAVE_PEOPLE_COUNT)),true)
    include $(CLEAR_VARS)
    LOCAL_MODULE_TAGS := optional
    LOCAL_PREBUILT_LIBS := libawapc.so 
    include $(BUILD_MULTI_PREBUILT)
endif

#######################

include $(CLEAR_VARS)
LOCAL_MODULE := libminigui_ths-3.0.so.12
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_SRC_FILES := libminigui_ths.so
LOCAL_MODULE_CLASS := lib
include $(BUILD_PREBUILT)

#######################
#######################

include $(CLEAR_VARS)
LOCAL_MODULE := libcvbsdisp.so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := lib
include $(BUILD_PREBUILT)

#######################

include $(CLEAR_VARS)
LOCAL_MODULE := libdhcp.so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := lib
include $(BUILD_PREBUILT)

#######################

include $(CLEAR_VARS)
LOCAL_MODULE := libhttpclt.so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := lib
include $(BUILD_PREBUILT)

#######################

include $(CLEAR_VARS)
LOCAL_MODULE := libhttpsrv.so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := lib
include $(BUILD_PREBUILT)

#########################
#######BDVideoHD#########
#########################
include $(CLEAR_VARS)
LOCAL_MODULE := libNATClient.so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := lib
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libNATClient2.so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := lib
include $(BUILD_PREBUILT)
#######################

include $(CLEAR_VARS)
LOCAL_MODULE := libonvifipc.so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := lib
include $(BUILD_PREBUILT)

#######################

include $(CLEAR_VARS)
LOCAL_MODULE := librtspserver.so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := lib
include $(BUILD_PREBUILT)

#######################

#include $(CLEAR_VARS)
#LOCAL_MODULE := libssl.so
#LOCAL_MODULE_TAGS := optional
#LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
#LOCAL_SRC_FILES := $(LOCAL_MODULE)
#LOCAL_MODULE_CLASS := lib
#include $(BUILD_PREBUILT)

#######################

include $(CLEAR_VARS)
LOCAL_MODULE := libupnp.so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := lib
include $(BUILD_PREBUILT)

#######################

#include $(CLEAR_VARS)
#LOCAL_MODULE := libvdecoder.so
#LOCAL_MODULE_TAGS := optional
#LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
#LOCAL_SRC_FILES := $(LOCAL_MODULE)
#LOCAL_MODULE_CLASS := lib
#include $(BUILD_PREBUILT)

#######################

#include $(CLEAR_VARS)
#LOCAL_MODULE := libvecore.so
#LOCAL_MODULE_TAGS := optional
#LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
#LOCAL_SRC_FILES := $(LOCAL_MODULE)
#LOCAL_MODULE_CLASS := lib
#include $(BUILD_PREBUILT)

#######################

include $(CLEAR_VARS)
LOCAL_MODULE := libgnustl_shared.so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := lib
include $(BUILD_PREBUILT)

#######################

include $(CLEAR_VARS)
LOCAL_MODULE := libtxdevicesdk.so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := lib
include $(BUILD_PREBUILT)

#######################

include $(CLEAR_VARS)
#LOCAL_MODULE := libvencoder.so
#LOCAL_MODULE_TAGS := optional
#LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
#LOCAL_SRC_FILES := $(LOCAL_MODULE)
#LOCAL_MODULE_CLASS := lib
#include $(BUILD_PREBUILT)

#########################
#######BDVideoHD#########
#########################



#######################

#include $(CLEAR_VARS)
#LOCAL_MODULE := libve.so
#LOCAL_MODULE_TAGS := optional
#LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
#LOCAL_SRC_FILES := $(LOCAL_MODULE)
#LOCAL_MODULE_CLASS := lib
#include $(BUILD_PREBUILT)

#######################
include $(CLEAR_VARS)
LOCAL_MODULE := libsmtpclt.so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := lib
include $(BUILD_PREBUILT)

#######################
include $(CLEAR_VARS)
LOCAL_MODULE := libawapc.so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := lib
include $(BUILD_PREBUILT)

#######################
include $(CLEAR_VARS)
LOCAL_MODULE := libaw_net.so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := lib
include $(BUILD_PREBUILT)

#######################
include $(CLEAR_VARS)
LOCAL_MODULE := libsmartlink.so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := lib
include $(BUILD_PREBUILT)



#######################
include $(CLEAR_VARS)
LOCAL_MODULE := libRDTAPIs.so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := lib
include $(BUILD_PREBUILT)


#######################
include $(CLEAR_VARS)
LOCAL_MODULE := libIOTCAPIs.so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := lib
include $(BUILD_PREBUILT)

#######################
include $(CLEAR_VARS)
LOCAL_MODULE := libAVAPIs.so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := lib
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := memtester 
LOCAL_MODULE := memtester
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_SRC_FILES := mtop
LOCAL_MODULE := mtop
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
