LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include $(LOCAL_PATH)/../Config.mk

ifeq ($(CEDARX_ANDROID_VERSION),4)
CEDARA_VERSION_TAG = _
else
CEDARA_VERSION_TAG = _$(CEDARX_ANDROID_CODE)_
endif

LOCAL_SRC_FILES:=                         \
		CedarARender.cpp \
        CedarAPlayer.cpp				  


LOCAL_C_INCLUDES:= \
	$(JNI_H_INCLUDE) \
	$(LOCAL_PATH)/include \
	${CEDARX_TOP}/ \
	${CEDARX_TOP}/libcodecs/include \
	$(TOP)/frameworks/${AV_BASE_PATH}/include/media/stagefright \
    $(TOP)/frameworks/${AV_BASE_PATH}/include/media/stagefright/openmax

LOCAL_SHARED_LIBRARIES := \
        libbinder         \
        libmedia          \
        libutils          \
        libcutils         \
        libCedarX         \
        libcedarxstream


ifneq ($(CEDARX_DEBUG_ENABLE),Y)
LOCAL_LDFLAGS += \
	$(LOCAL_PATH)/../CedarAndroidLib/LIB$(CEDARA_VERSION_TAG)$(CEDARX_CHIP_VERSION)/libcedara_decoder.a
endif

LOCAL_LDFLAGS += \
	$(LOCAL_PATH)/../CedarAndroidLib/LIB$(CEDARA_VERSION_TAG)$(CEDARX_CHIP_VERSION)/libGetAudio_format.a \
	$(LOCAL_PATH)/../CedarAndroidLib/LIB$(CEDARA_VERSION_TAG)$(CEDARX_CHIP_VERSION)/libaacenc.a

ifeq ($(CEDARX_DEBUG_ENABLE),Y)
LOCAL_STATIC_LIBRARIES += \
	libcedara_decoder
endif

ifeq ($(CEDARX_ENABLE_MEMWATCH),Y)
LOCAL_STATIC_LIBRARIES += libmemwatch
endif

ifeq ($(TARGET_OS)-$(TARGET_SIMULATOR),linux-true)
        LOCAL_LDLIBS += -lpthread -ldl
        LOCAL_SHARED_LIBRARIES += libdvm
        LOCAL_CPPFLAGS += -DANDROID_SIMULATOR
endif

ifneq ($(TARGET_SIMULATOR),true)
LOCAL_SHARED_LIBRARIES += libdl
endif

ifeq ($(TARGET_OS)-$(TARGET_SIMULATOR),linux-true)
        LOCAL_LDLIBS += -lpthread
endif

LOCAL_CFLAGS += -Wno-multichar 

LOCAL_CFLAGS += $(CEDARX_EXT_CFLAGS)
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= libCedarA

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))


















ifneq ($(CEDARX_DEBUG_ENABLE),N)

include $(CLEAR_VARS)


include $(LOCAL_PATH)/../../Config.mk
ifeq ($(CEDARX_ANDROID_VERSION),4)
CEDARA_VERSION_TAG = _
else
CEDARA_VERSION_TAG = _$(CEDARX_ANDROID_CODE)_
LOCAL_CFLAGS += -D__ENABLE_AC3DTSRAW
endif

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
	CedarADecoder.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../include \
	${CEDARX_TOP}/libutil \
	${CEDARX_TOP}/libcodecs/include \
	${CEDARX_TOP}/include \
	${CEDARX_TOP}/include/include_base

#LOCAL_SHARED_LIBRARIES := \
        libCedarX

LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libutils libcutils

LOCAL_PRELINK_MODULE := false

LOCAL_CFLAGS += $(CEDARX_EXT_CFLAGS)   -D__ENABLE_AC3DTS

ifeq ($(CEDARX_DEBUG_FRAMEWORK),Y)
LOCAL_SHARED_LIBRARIES += libcedarxbase 
else
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB$(CEDARA_VERSION_TAG)$(CEDARX_CHIP_VERSION)/libcedarxbase.so
endif

#ifeq ($(CEDARX_DEBUG_CEDARV),Y)
#LOCAL_SHARED_LIBRARIES += libcedarxosal 
#else
#LOCAL_LDFLAGS += \
#	$(CEDARX_TOP)/../CedarAndroidLib/LIB$(CEDARA_VERSION_TAG)$(CEDARX_CHIP_VERSION)/libcedarxosal.so
#endif

LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/$(CEDARX_PREBUILD_LIB_PATH)/libac3.a \
	$(CEDARX_TOP)/../CedarAndroidLib/$(CEDARX_PREBUILD_LIB_PATH)/libdts.a 
ifeq ($(CEDARX_ANDROID_VERSION),4)

else
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/$(CEDARX_PREBUILD_LIB_PATH)/libac3_raw.a \
	$(CEDARX_TOP)/../CedarAndroidLib/$(CEDARX_PREBUILD_LIB_PATH)/libdts_raw.a

endif	

LOCAL_MODULE:= libswa2

include $(BUILD_SHARED_LIBRARY)
#######################################################################################

include $(CLEAR_VARS)


include $(LOCAL_PATH)/../../Config.mk
ifeq ($(CEDARX_ANDROID_VERSION),4)
CEDARA_VERSION_TAG = _
else
CEDARA_VERSION_TAG = _$(CEDARX_ANDROID_CODE)_
endif

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
	CedarADecoder.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../include \
	${CEDARX_TOP}/libutil \
	${CEDARX_TOP}/libcodecs/include \
	${CEDARX_TOP}/include \
	${CEDARX_TOP}/include/include_base

#LOCAL_SHARED_LIBRARIES := \
#        libCedarX

LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libutils libcutils

LOCAL_PRELINK_MODULE := false

LOCAL_CFLAGS += $(CEDARX_EXT_CFLAGS)   -D__ENABLE_OTHER

ifeq ($(CEDARX_DEBUG_FRAMEWORK),Y)
LOCAL_SHARED_LIBRARIES += libcedarxbase 
else
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB$(CEDARA_VERSION_TAG)$(CEDARX_CHIP_VERSION)/libcedarxbase.so
endif

#ifeq ($(CEDARX_DEBUG_CEDARV),Y)
#LOCAL_SHARED_LIBRARIES += libcedarxosal 
#else
#LOCAL_LDFLAGS += \
#	$(CEDARX_TOP)/../CedarAndroidLib/LIB$(CEDARA_VERSION_TAG)$(CEDARX_CHIP_VERSION)/libcedarxosal.so
#endif

LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/$(CEDARX_PREBUILD_LIB_PATH)/libaac.a \
	$(CEDARX_TOP)/../CedarAndroidLib/$(CEDARX_PREBUILD_LIB_PATH)/libmp3.a \
	$(CEDARX_TOP)/../CedarAndroidLib/$(CEDARX_PREBUILD_LIB_PATH)/libamr.a \
	$(CEDARX_TOP)/../CedarAndroidLib/$(CEDARX_PREBUILD_LIB_PATH)/libwav.a 
LOCAL_MODULE:= libaw_audioa

include $(BUILD_SHARED_LIBRARY)

endif
