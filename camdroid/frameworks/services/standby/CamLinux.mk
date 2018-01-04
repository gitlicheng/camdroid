LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	IStandBy.cpp \
	StandbyService.cpp


LOCAL_SHARED_LIBRARIES := \
	libutils \
	libbinder

LOCAL_MODULE:= libstandby

LOCAL_C_INCLUDES := \
    $(TOP)/frameworks/include/standby 

LOCAL_MODULE_TAGS := optional 

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES :=  \
        main_standby.cpp

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := standbyservice

LOCAL_SHARED_LIBRARIES := \
    libmedia_proxy \
    libcutils                   \
    libutils                    \
        libstandby \
        libbinder

LOCAL_C_INCLUDES := \
        $(TOP)/frameworks/include/binder \
        $(TOP)/frameworks/include/standby

include $(BUILD_EXECUTABLE)
