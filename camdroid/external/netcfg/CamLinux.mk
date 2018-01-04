LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= netcfg.c
LOCAL_MODULE:= netcfg
LOCAL_SHARED_LIBRARIES := libc libnetutils
LOCAL_MODULE_TAGS := eng

include $(BUILD_EXECUTABLE)
