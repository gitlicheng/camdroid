ifeq ($(BOARD_HAVE_WATER_MARK),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    water_mark.c \
	water_mark_interface.c
LOCAL_SHARED_LIBRARIES := \
	libcutils \

LOCAL_MODULE:= libwater_mark

include $(BUILD_SHARED_LIBRARY)

endif

