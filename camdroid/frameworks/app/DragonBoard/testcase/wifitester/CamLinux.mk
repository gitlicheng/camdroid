LOCAL_PATH := $(call my-dir)

############################################
include $(CLEAR_VARS)

ifeq ($(BOARD_WIFI_VENDOR), realtek)

LOCAL_MODULE := wifitester
LOCAL_SRC_FILES := wifitester.c

LOCAL_C_INCLUDES :=  $(TOP)/frameworks/app/DragonBoard/inc
LOCAL_SHARED_LIBRARIES := libcutils libhardware libhardware_legacy
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

endif

ifeq ($(BOARD_WIFI_VENDOR), broadcom)

LOCAL_MODULE := wifitester
LOCAL_SRC_FILES := wifitester.c

LOCAL_C_INCLUDES :=  $(TOP)/frameworks/app/DragonBoard/inc
LOCAL_SHARED_LIBRARIES := libcutils libhardware libhardware_legacy
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

endif

ifeq ($(BOARD_WIFI_VENDOR), realtek)
legacy_modules += wifi
endif
ifeq ($(BOARD_WIFI_VENDOR), broadcom)
legacy_modules += wifi
endif
