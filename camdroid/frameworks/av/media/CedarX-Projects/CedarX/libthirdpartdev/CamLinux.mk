LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../Config.mk
LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
	thirdpart_stream.c

LOCAL_C_INCLUDES := \
		${CEDARX_TOP}/include \
		${CEDARX_TOP}/include/include_stream \
		${CEDARX_TOP}/include/include_thirdpartdev

LOCAL_MODULE_TAGS := optional
 
LOCAL_CFLAGS += $(CEDARX_EXT_CFLAGS)
LOCAL_SHARED_LIBRARIES := \
        libutils          \
        libcutils         
LOCAL_MODULE:= libthirdpartstream

include $(BUILD_SHARED_LIBRARY)

