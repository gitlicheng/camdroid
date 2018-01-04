LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE := libamrnb
LOCAL_MODULE_TAGS := optional

LOCAL_WHOLE_STATIC_LIBRARIES += \
	libstagefright_amrnb_common \
	libstagefright_amrnbdec \
	libstagefright_amrnbenc

include $(BUILD_STATIC_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
