LOCAL_PATH:= $(call my-dir)

#
# libmediaplayerservice
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=   \
    core/hardware/HerbCamera.cpp \
    core/hardware/CedarDisplay.cpp \
    media/media/HerbMediaRecorder.cpp \
    media/media/HerbCamcorderProfile.cpp \
    media/media/MediaCallbackDispatcher.cpp \
    media/media/CedarMediaPlayer.cpp \
    media/media/CedarMetadata.cpp \
    media/media/HerbMediaMetadataRetriever.cpp \
    media/media/CedarMediaVideoResizer.cpp \
    media/media/CedarMediaServerCaller.cpp


LOCAL_SHARED_LIBRARIES := \
	libcamera_client \
	libdisplay_client \
	libmedia \
	libcutils \
	libutils \
	libbinder

LOCAL_STATIC_LIBRARIES :=

LOCAL_C_INCLUDES := \
    $(TOP)/frameworks/include/include_media/media \
    $(TOP)/frameworks/include/include_media/display \
    $(TOP)/device/softwinner/common/hardware/include 

#ifeq ($(BOARD_HAVE_ADAS),true)
#LOCAL_CFLAGS += -DADAS_ENABLE
#endif

# LOCAL_CFLAGS +=-DCEDARX_ANDROID_VERSION=9
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE:= libmedia_proxy

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
