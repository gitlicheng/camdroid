LOCAL_PATH:= $(call my-dir)

#
# libcameraservice
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=               \
    CameraService.cpp \
    CameraClient.cpp  \

LOCAL_SHARED_LIBRARIES:= \
    libutils \
    libbinder \
    libcutils \
    libmedia \
    libmedia_native \
    libcamera_client \
    libhardware \
    libsync \
    libcamera_metadata \
    libjpeg

LOCAL_C_INCLUDES += \
    system/media/camera/include \
    frameworks/av/display \
    external/jpeg \
    device/softwinner/common/hardware/include 

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE:= libcameraservice

include $(BUILD_SHARED_LIBRARY)
