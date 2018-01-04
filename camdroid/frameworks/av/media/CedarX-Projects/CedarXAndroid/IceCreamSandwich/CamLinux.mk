LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

#include frameworks/${AV_BASE_PATH}/media/libstagefright/codecs/common/Config.mk
include $(LOCAL_PATH)/../../Config.mk

LOCAL_SRC_FILES:=                         \
		CedarXAudioPlayer.cpp			  \
        CedarXMetadataRetriever.cpp		  \
        CedarXMetaData.cpp				  \
        CedarXRecorder.cpp				  \
		CedarXPlayer.cpp				  \
		CedarXNativeRenderer.cpp		\
		CedarXSoftwareRenderer.cpp		  \

#virtual_hwcomposer.cpp

LOCAL_C_INCLUDES:= \
	$(JNI_H_INCLUDE) \
	$(TOP)/frameworks/${AV_BASE_PATH}/include/media/stagefright \
    $(CEDARX_TOP)/include \
    $(CEDARX_TOP)/libutil \
    $(CEDARX_TOP)/include/include_cedarv \
    $(CEDARX_TOP)/include/include_audio \
    ${CEDARX_TOP}/include/include_camera \
    ${CEDARX_TOP}/include/include_omx \
    ${CEDARX_TOP}/include/include_subtitle_render \
    ${CEDARX_TOP}/include/include_stream \
    ${CEDARX_TOP}/libcodecs/CODEC/VIDEO/DECODER \
    ${CEDARX_TOP}/libcodecs/CODEC/VIDEO/ENCODER \
    ${CEDARX_TOP}/libcodecs/MEMORY \
    ${CEDARX_TOP}/../ \
    $(TOP)/frameworks/${AV_BASE_PATH}/media/libstagefright/include \
    $(TOP)/frameworks/${AV_BASE_PATH} \
    $(TOP)/frameworks/${AV_BASE_PATH}/include \
    $(TOP)/external/openssl/include \
    $(TOP)/external/icu4c/common \
    $(TOP)/frameworks/av/display \
    $(TOP)/device/softwinner/common/hardware/include 
    
ifeq ($(PLATFORM_VERSION),4.1.1)
    LOCAL_C_INCLUDES += $(TOP)/frameworks/include/media/hardware
endif
ifeq ($(PLATFORM_VERSION),4.2.1)
    LOCAL_C_INCLUDES += $(TOP)/frameworks/include/media/hardware
endif
ifeq ($(PLATFORM_VERSION),4.2.2)
	LOCAL_C_INCLUDES += $(TOP)/frameworks/include/media/hardware
endif


LOCAL_SHARED_LIBRARIES := \
	libbinder         \
	libmedia          \
	libutils          \
	libcutils         \
	libcamera_client \
	libstagefright_foundation \
	libhwdisp \
	libstagefright_color_conversion

#libcedarxosal	  \
# libicuuc
# libion_alloc \


# libui             \
# libgui			  \
# libskia    \

ifeq ($(PLATFORM_VERSION),2.3.4)
   LOCAL_SHARED_LIBRARIES += libsurfaceflinger_client
endif
ifeq ($(PLATFORM_VERSION),4.0.1)
   LOCAL_SHARED_LIBRARIES += libsurfaceflinger_client
endif
ifeq ($(PLATFORM_VERSION),4.0.3)
   LOCAL_SHARED_LIBRARIES += libsurfaceflinger_client
endif
ifeq ($(PLATFORM_VERSION),4.0.4)
   LOCAL_SHARED_LIBRARIES += libsurfaceflinger_client
endif
#ifneq ($(PLATFORM_VERSION),4.1.1)
#LOCAL_SHARED_LIBRARIES += libsurfaceflinger_client
#endif
#ifneq ($(PLATFORM_VERSION),4.2.1)
#LOCAL_SHARED_LIBRARIES += libsurfaceflinger_client
#endif

ifeq ($(CEDARX_DEBUG_FRAMEWORK),Y)
LOCAL_STATIC_LIBRARIES += \
	libcedarxplayer \
	libcedarxcomponents \
	libcedarxdemuxers \
	libcedarxrender \
	libsub \
	libmuxers \
	libmp4_muxer \
	libmp3_muxer \
	libaac_muxer \
	libraw_muxer \
	libmpeg2ts_muxer \
	libcedarxdrmsniffer

# libcedarxsftdemux \
# libcedarxwvmdemux \
#libjpgenc	\
#libuserdemux \
#libawts_muxer \


#LOCAL_STATIC_LIBRARIES += \
#	libcedarxstream \

# LOCAL_STATIC_LIBRARIES += \
#	libcedarx_rtsp
	
else
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarxplayer.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarxcomponents.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarxdemuxers.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarxrender.a	\
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libsub.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libmuxers.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libmp4_muxer.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libmp3_muxer.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libaac_muxer.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libraw_muxer.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libmpeg2ts_muxer.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarxdrmsniffer.a \

# $(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libiconv.a \
# $(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarxsftdemux.a \
# $(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarxwvmdemux.a \
# $(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libjpgenc.a	\
# $(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libuserdemux.a \
# $(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libawts_muxer.a \

#LOCAL_LDFLAGS += \
#	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarxstream.a 

LOCAL_LDFLAGS += \


# $(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarx_rtsp.a

endif

#audio postprocess lib
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libPostProcessOBJ.a \

#$(CEDARX_TOP)/../CedarAndroidLib/$(CEDARX_PREBUILD_LIB_PATH)/libGetAudio_format.a

ifeq ($(CEDARX_DEBUG_DEMUXER),Y)
LOCAL_STATIC_LIBRARIES += \
	libdemux_mov \
	libdemux_ts 
else
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libdemux_mov.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libdemux_ts.a 
endif

LOCAL_SHARED_LIBRARIES += libthirdpartstream libcedarxstream
ifeq ($(CEDARX_DEBUG_FRAMEWORK),Y)
LOCAL_SHARED_LIBRARIES += libcedarxbase
# libcedarxsftstream libswdrm
else
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarxbase.so \

#	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libthirdpartstream.so \
#	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarxstream.so \

# $(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarxosal.so \
# $(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarxsftstream.so	
# $(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libswdrm.so \

endif

ifeq ($(CEDARX_DEBUG_CEDARV),Y)
LOCAL_SHARED_LIBRARIES += libvdecoder libMemAdapter libVE libvencoder
else
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libvdecoder.so \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libMemAdapter.so \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libVE.so \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libvencoder.so
endif

LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/$(CEDARX_PREBUILD_LIB_PATH)/libaacenc.a \
	$(CEDARX_TOP)/../CedarAndroidLib/$(CEDARX_PREBUILD_LIB_PATH)/libmp3enc.a

ifeq ($(CEDARX_ENABLE_MEMWATCH),Y)
LOCAL_STATIC_LIBRARIES += libmemwatch
endif

LOCAL_SHARED_LIBRARIES += libstagefright_foundation
# libstagefright libdrmframework

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

ifeq ($(CEDARX_ANDROID_VERSION),3)
LOCAL_CFLAGS += -D__ANDROID_VERSION_2_3
else
LOCAL_CFLAGS += -D__ANDROID_VERSION_2_3_4
endif
LOCAL_CFLAGS += $(CEDARX_EXT_CFLAGS)
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= libCedarX

include $(BUILD_SHARED_LIBRARY)

#include $(call all-makefiles-under,$(LOCAL_PATH))
