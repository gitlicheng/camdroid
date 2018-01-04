LOCAL_PATH := $(call my-dir)

ifneq ($(BOARD_HAVE_BLUETOOTH_BCM),)

include $(CLEAR_VARS)

ifeq ($(SW_BOARD_HAVE_BLUETOOTH_NAME), ap6210)
    LOCAL_CFLAGS += -DUSE_AP6210_BT_MODULE    
endif

ifeq ($(SW_BOARD_HAVE_BLUETOOTH_NAME), ap6212)
    LOCAL_CFLAGS += -DUSE_AP6212_BT_MODULE    
endif

BDROID_DIR := $(TOP_DIR)external/bluetooth/bluedroid

LOCAL_SRC_FILES := \
        src/bt_vendor_brcm.c \
        src/hardware.c \
        src/userial_vendor.c \
        src/upio.c \
        src/conf.c

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/include \
        $(BDROID_DIR)/hci/include

LOCAL_SHARED_LIBRARIES := \
        libcutils

LOCAL_MODULE := libbt-vendor
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_OWNER := broadcom
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)

include $(LOCAL_PATH)/vnd_buildcfg.mk

include $(BUILD_SHARED_LIBRARY)


ifeq ($(SW_BOARD_HAVE_BLUETOOTH_NAME), ap6210)    
    include $(LOCAL_PATH)/conf/ap6210/CamLinux.mk
endif

ifeq ($(SW_BOARD_HAVE_BLUETOOTH_NAME), ap6212)    
    include $(LOCAL_PATH)/conf/ap6212/CamLinux.mk
endif

ifeq ($(SW_BOARD_HAVE_BLUETOOTH_NAME), ap6330)
    include $(LOCAL_PATH)/conf/ap6330/CamLinux.mk
endif

endif # BOARD_HAVE_BLUETOOTH_BCM
