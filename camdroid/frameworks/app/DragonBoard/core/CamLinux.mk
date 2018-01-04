LOCAL_PATH := $(call my-dir)

#########################################################
#include $(CLEAR_VARS)
#
#LOCAL_MODULE := dragonboard
#LOCAL_SRC_FILES := core/dragonboard.c
#
#LOCAL_C_INCLUDES := $(TOP)/frameworks/include/include_gui $(LOCAL_PATH)/inc
#LOCAL_LDFLAGS := -L$(TOP)/frameworks/prebuilts -lminigui_ths
#
#include $(BUILD_EXECUTABLE)

#########################################################
#include $(CLEAR_VARS)
#
#LOCAL_MODULE := dialogbox
#LOCAL_SRC_FILES := dialogbox.c
#
#LOCAL_C_INCLUDES := $(TOP)/frameworks/include/include_gui $(LOCAL_PATH)/inc
#LOCAL_LDFLAGS := -L$(TOP)/frameworks/prebuilts -lminigui_ths
#
#include $(BUILD_EXECUTABLE)

#########################################################
include $(CLEAR_VARS)

LOCAL_MODULE := dragonboard
LOCAL_SRC_FILES := dragonboard.cpp
LOCAL_C_INCLUDES := $(TOP)/frameworks/include/include_gui \
		$(TOP)/frameworks/include/include_media/media \
		$(TOP)/frameworks/av/display \
		$(TOP)/frameworks/include/include_media/display \
		$(TOP)/device/softwinner/common/hardware/include \
		$(TOP)/frameworks/app/DragonBoard/inc
LOCAL_SHARED_LIBRARIES := libmedia_proxy libcutils libutils libbinder
LOCAL_LDFLAGS := -L$(TOP)/frameworks/prebuilts -lminigui_ths
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
