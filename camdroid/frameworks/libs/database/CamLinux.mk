# Copyright (C) 2009 The Android Open Source Project
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

# we have the common sources, plus some device-specific stuff
sources := \
DBCon.cpp \
DBCtrl.cpp 
	
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libdatabase
#LOCAL_LDFLAGS += -L$(TOP)/frameworks/prebuilts -lsqlite


LOCAL_SHARED_LIBRARIES := \
	libutils \
	libsqlite

LOCAL_SRC_FILES := $(sources)
include $(BUILD_SHARED_LIBRARY)
