# Copyright 2006 The Android Open Source Project
LOCAL_PATH:= $(call my-dir)



include $(CLEAR_VARS)
LOCAL_SRC_FILES := iwlib.c iwpriv.c
#LOCAL_C_INCLUDES := $(KERNEL_HEADERS)
LOCAL_SHARED_LIBRARIES := libc
LOCAL_MODULE = iwpriv
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := iwlib.c iwlist.c
#LOCAL_C_INCLUDES := $(KERNEL_HEADERS)
LOCAL_SHARED_LIBRARIES := libc
LOCAL_MODULE = iwlist
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := iwlib.c iwconfig.c
#LOCAL_C_INCLUDES := $(KERNEL_HEADERS)
LOCAL_SHARED_LIBRARIES := libc
LOCAL_MODULE = iwconfig
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)


