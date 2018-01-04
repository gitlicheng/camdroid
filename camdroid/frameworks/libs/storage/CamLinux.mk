###############################################
###########       libstorage      #############
###############################################
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := StorageMonitor.cpp

LOCAL_MODULE := libstorage
LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libbinder \

include $(BUILD_SHARED_LIBRARY)

