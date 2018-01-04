LOCAL_PATH := $(call my-dir)

#############################################
#include $(CLEAR_VARS)
#LOCAL_SRC_FILES := mixer.c pcm.c
#LOCAL_MODULE := libtinyalsa
#LOCAL_SHARED_LIBRARIES := libcutils libutils
#LOCAL_MODULE_TAGS := optional
#LOCAL_PRELINK_MODULE := false
#include $(BUILD_SHARED_LIBRARY)
#
#############################################
include $(CLEAR_VARS)
LOCAL_SRC_FILES := mic_spk_tester.c
LOCAL_C_INCLUDES :=  $(TOP)/frameworks/app/DragonBoard/inc
LOCAL_MODULE := mictester
LOCAL_SHARED_LIBRARIES := libtinyalsa libutils
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

#############################################
#include $(CLEAR_VARS)
#LOCAL_C_INCLUDES:= external/tinyalsa/include
#LOCAL_SRC_FILES:= tinycap.c
#LOCAL_MODULE := tinycap
#LOCAL_SHARED_LIBRARIES:= libcutils libutils libtinyalsa
#LOCAL_MODULE_TAGS := optional eng
#
#include $(BUILD_EXECUTABLE)
#
##############################################
#include $(CLEAR_VARS)
#LOCAL_MODULE := tinymix
#LOCAL_SRC_FILES:= tinymix.c
#LOCAL_C_INCLUDES := $(TOP)/app/DragonBoard/inc
#LOCAL_SHARED_LIBRARIES:= libtinyalsa
#LOCAL_MODULE_TAGS := optional eng
#
#include $(BUILD_EXECUTABLE)
