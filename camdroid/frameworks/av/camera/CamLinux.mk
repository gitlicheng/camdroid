LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	Camera.cpp \
	CameraParameters.cpp \
	ICamera.cpp \
	ICameraClient.cpp \
	ICameraService.cpp \
	ICameraRecordingProxy.cpp \
	ICameraRecordingProxyListener.cpp

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libbinder \
	libhardware \

LOCAL_C_INCLUDES += 								\
	frameworks/include								\
	frameworks/include/include_adas					\
	frameworks/av/media/CedarX-Projects/CedarX/include/include_camera \
	device/softwinner/common/hardware/include 

LOCAL_MODULE:= libcamera_client

include $(BUILD_SHARED_LIBRARY)
