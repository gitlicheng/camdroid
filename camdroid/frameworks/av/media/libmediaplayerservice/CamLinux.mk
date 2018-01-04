LOCAL_PATH:= $(call my-dir)

###########################################

include $(CLEAR_VARS)
LOCAL_MODULE := librotation

LOCAL_MODULE_TAGS := optional 

LOCAL_SRC_FILES := rotation.cpp 

LOCAL_SHARED_LIBRARIES := libutils libbinder

include $(BUILD_STATIC_LIBRARY)
###########################################

#
# libmediaplayerservice
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=               \
    ActivityManager.cpp         \
    Crypto.cpp                  \
    HDCP.cpp                    \
    MediaPlayerFactory.cpp      \
    MediaPlayerService.cpp      \
    MediaRecorderClient.cpp     \
    MetadataRetrieverClient.cpp \
    CedarPlayer.cpp       		\
    TestPlayerStub.cpp          \
    CedarAPlayerWrapper.cpp		\
    SimpleMediaFormatProbe.cpp	\
    MovAvInfoDetect.cpp         \
    MediaVideoResizerClient.cpp \
    MediaServerCallerClient.cpp \

#    MidiFile.cpp                \
#    MidiMetadataRetriever.cpp   \
#	ThumbnailPlayer/tplayer.cpp \
#    ThumbnailPlayer/avtimer.cpp


# RemoteDisplay.cpp           \
# StagefrightPlayer.cpp		\
# StagefrightRecorder.cpp	 \



LOCAL_SHARED_LIBRARIES :=       \
    libbinder                   \
    libcamera_client            \
    libcutils                   \
    libdl                       \
    libmedia                    \
    libmedia_native             \
    libCedarX           	    \
    libCedarA           	    \
    libcedarxbase                   \
    libstagefright_foundation   \
    libutils                    \
    libvideoresizer \
    libjpegdecoder \
    libstagefright_color_conversion \
    libMemAdapter \
    libcedarxstream 

#    libvorbisidec               \
#    libsonivox                  \
# libgui                      \
# libui							\
# libstagefright				\
# libstagefright_omx          \
# libstagefright_wfd          \
# libcedarxosal					\
# libcedarv						\


LOCAL_STATIC_LIBRARIES :=       \
    librotation


# libstagefright_nuplayer     \
# libstagefright_rtsp         \

LOCAL_C_INCLUDES :=                                                 \
    $(call include-path-for, graphics corecg)                       \
    $(TOP)/frameworks/av/media/CedarX-Projects \
    $(TOP)/frameworks/av/media/CedarX-Projects/CedarXAndroid/IceCreamSandwich \
	$(TOP)/frameworks/av/media/CedarX-Projects/CedarX/include/include_audio \
	$(TOP)/frameworks/av/media/CedarX-Projects/CedarX/include/include_cedarv \
	$(TOP)/frameworks/av/media/CedarX-Projects/CedarX/libcodecs/CODEC/VIDEO/DECODER \
	$(TOP)/frameworks/av/media/CedarX-Projects/CedarX/libcodecs/CODEC/VIDEO/ENCODER \
    $(TOP)/frameworks/av/media/CedarX-Projects/CedarX/include/include_camera \
    $(TOP)/frameworks/av/media/CedarX-Projects/CedarX/include/include_demux \
    $(TOP)/frameworks/av/media/CedarX-Projects/CedarX/include/include_stream \
    $(TOP)/frameworks/av/media/CedarX-Projects/CedarX/include/include_vencoder \
	$(TOP)/frameworks/av/media/CedarX-Projects/CedarX/include \
	$(TOP)/frameworks/av/media/CedarX-Projects/CedarA \
	$(TOP)/frameworks/av/media/CedarX-Projects/CedarA/include \
    $(TOP)/frameworks/av/media/libstagefright/include               \
    $(TOP)/frameworks/av/media/libstagefright/rtsp                  \
    $(TOP)/frameworks/av/media/libstagefright/wifi-display          \
    $(TOP)/frameworks/include/media/openmax                  \
    $(TOP)/external/tremolo/Tremolo                                 \
    $(TOP)/frameworks/av/media/libvideoresize \
    $(TOP)/frameworks/av/media/libjpegdecoder \
    $(TOP)/device/softwinner/common/hardware/include \
	$(TOP)/frameworks/av/media/CedarX-Projects/CedarX/libcodecs/MEMORY \
	$(TOP)/external/fatfs \

LOCAL_CFLAGS +=-DCEDARX_ANDROID_VERSION=9

LOCAL_MODULE:= libmediaplayerservice

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
