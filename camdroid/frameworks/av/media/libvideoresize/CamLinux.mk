LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

CEDARX_TOP := $(TOP)/frameworks/av/media/CedarX-Projects/CedarX

LOCAL_C_INCLUDES:= \
	$(CEDARX_TOP)/include \
	$(CEDARX_TOP)/include/include_demux \
	$(CEDARX_TOP)/include/include_cedarv \
	$(CEDARX_TOP)/include/include_vencoder \
	$(CEDARX_TOP)/include/include_audio \
	${CEDARX_TOP}/include/include_camera \
	${CEDARX_TOP}/include/include_subtitle_render \
	${CEDARX_TOP}/libcodecs/CODEC/VIDEO/DECODER \
	${CEDARX_TOP}/libcodecs/CODEC/VIDEO/ENCODER \
	${CEDARX_TOP}/libcodecs/MEMORY \
	frameworks/include/include_parse


LOCAL_SRC_FILES := \
    VideoResizer.cpp \
    VideoResizerDemuxer.cpp \
    VideoResizerDecoder.cpp \
    VideoResizerEncoder.cpp \
    VideoResizerMuxer.cpp

LOCAL_SHARED_LIBRARIES := \
	libutils \
	libbinder \
	libCedarX \
	libcedarxbase \
	libvdecoder \
	libvencoder\
	libMemAdapter

#	libaw_h264enc
#	libcedarv \
#	libcedarxosal \
	
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE:= libvideoresizer

include $(BUILD_SHARED_LIBRARY)

