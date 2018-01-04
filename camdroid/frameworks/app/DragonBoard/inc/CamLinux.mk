LOCAL_PATH := $(call my-dir)

############################################
include $(CLEAR_VARS)

LOCAL_MODULE := parser
LOCAL_SRC_FILES := parser.c

LOCAL_C_INCLUDES := $(TOP)/DragonBoard/inc
LOCAL_SHARED_LIBRARIES := libutils
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
