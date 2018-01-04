LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../Config.mk
LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
	cedarx_stream.c \
	cedarx_outstream.c \
	cedarx_outstream_http.c \
	cedarx_outstream_external.c \
	cedarx_stream_file.c \


# LOCAL_SRC_FILES += \
#     sft_httplive_stream.c \
#     sft_httplive_stream_wrapper.cpp
#     cedarx_stream_drm_file.c 

    
# LOCAL_SRC_FILES += \
# 	sft_streaming_source.c \
# 	NuPlayerStreamListener.cpp \
# 	StreamingSource.cpp

LOCAL_C_INCLUDES := \
		${CEDARX_TOP}/include \
		${CEDARX_TOP}/include/include_stream \
		${CEDARX_TOP}/include/include_demux \
		${CEDARX_TOP}/include/include_sft \
		${CEDARX_TOP}/include/include_cedarv \
		${CEDARX_TOP}/include/include_thirdpartdev \
		${CEDARX_TOP}/include/include_base \
		${CEDARX_TOP}/include/include_omx \
		${CEDARX_TOP}/overlay/httplive \
		${CEDARX_TOP}/libutil \
		${CEDARX_TOP}/../ \
		$(TOP)/external/fatfs \
		

LOCAL_C_INCLUDES += \
    $(TOP)/frameworks/${AV_BASE_PATH}/media/libstagefright/include \
    $(TOP)/frameworks/${AV_BASE_PATH}/media/libstagefright/mpeg2ts \
    $(TOP)/frameworks/${AV_BASE_PATH}/media/libstagefright/rtsp    \

LOCAL_SHARED_LIBRARIES := \
        libthirdpartstream \
        libcedarxbase \
        libutils \

# libcutils         \
# libbinder		   \

ifeq ($(CEDARX_FILE_SYSTEM),DIRECT_FATFS)
LOCAL_SHARED_LIBRARIES += libFatFs
endif

LOCAL_MODULE_TAGS := optional
 
LOCAL_CFLAGS += $(CEDARX_EXT_CFLAGS)

LOCAL_MODULE:= libcedarxstream

include $(BUILD_SHARED_LIBRARY)


######################################################

#include $(CLEAR_VARS)
#
#include $(LOCAL_PATH)/../../Config.mk
#LOCAL_ARM_MODE := arm
#
#LOCAL_SRC_FILES := \
#	awdrm.c
#		
#LOCAL_C_INCLUDES := \
#		${CEDARX_TOP}/include \


#LOCAL_MODULE_TAGS := optional
 
#LOCAL_CFLAGS += $(CEDARX_EXT_CFLAGS)

#LOCAL_LDFLAGS += \
#	$(CEDARX_TOP)/../CedarAndroidLib/libawdrm.a 

#LOCAL_PRELINK_MODULE := false

#LOCAL_MODULE:= libswdrm

#include $(BUILD_SHARED_LIBRARY)
