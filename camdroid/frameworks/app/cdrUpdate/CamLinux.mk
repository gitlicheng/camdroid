
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := \
        libcutils 

LOCAL_SRC_FILES := \
	OtaUpdate.cpp \
	OtaMain.cpp

LOCAL_LDFLAGS += -L$(TOP)/frameworks/prebuilts \
   -lminigui_ths

LOCAL_C_INCLUDES += \
    $(TOP)/frameworks/include/include_gui
ifeq ($(CEDARX_FILE_SYSTEM),DIRECT_FATFS)
LOCAL_C_INCLUDES += \
	$(TOP)/external/fatfs
endif
	
ifeq ($(CEDARX_FILE_SYSTEM),DIRECT_FATFS)
LOCAL_SHARED_LIBRARIES += libFatFs
LOCAL_CFLAGS += -DFATFS
endif

LOCAL_CFLAGS += -D$(TARGET_PRODUCT)   

LOCAL_MODULE := cdrUpdate
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

