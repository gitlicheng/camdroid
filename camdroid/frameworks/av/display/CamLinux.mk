###############################################
###########   libdisplay_client   #############
###############################################
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	Display.cpp \
	IDisplay.cpp \
	IDisplayClient.cpp \
	IDisplayService.cpp \

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libbinder

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE:= libdisplay_client

include $(BUILD_SHARED_LIBRARY)

###############################################
###########       libhwdisp       #############
###############################################
include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := \
	libcutils \

LOCAL_SRC_FILES:= \
	hwdisplay.cpp \

LOCAL_C_INCLUDES := \
	$(TOP)/frameworks/include/include_media/media \

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libhwdisp

include $(BUILD_SHARED_LIBRARY)


