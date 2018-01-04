LOCAL_PATH:= $(call my-dir)

########################################
include $(CLEAR_VARS)

LOCAL_MODULE := videotester
LOCAL_SRC_FILES := videotester.cpp

LOCAL_C_INCLUDES := \
	$(TOP)/frameworks/include/include_media/media \
	$(TOP)/frameworks/av/display \
	$(TOP)/frameworks/include/include_media/display \
	$(TOP)/device/softwinner/common/hardware/include \
	 $(TOP)/frameworks/app/DragonBoard/inc
LOCAL_SHARED_LIBRARIES := libmedia_proxy libcutils libutils libbinder

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

