
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

LOCAL_SHARED_LIBRARIES:= \
    libbinder \
    libutils \
    libcutils \
    libcamera_client \
    libui \
    libhwdisp \
    libcedarxbase \
    libvdecoder \
    libMemAdapter \
	libmedia_proxy 
	
#    libcedarxosal \
#    libcedarv

    
# cedarx libraries
LOCAL_SHARED_LIBRARIES += \
	libvencoder

#	libion_alloc \
#	libjpgenc \
	
LOCAL_C_INCLUDES += 								\
	frameworks/base/core/jni/android/graphics 		\
	frameworks/include/media/openmax				\
	hardware/libhardware/include/hardware			\
	frameworks/include								\
	frameworks/av/media/CedarX-Projects/CedarX/include	\
	frameworks/av/media/CedarX-Projects/CedarX/include/include_system	\
	frameworks/av/media/CedarX-Projects/CedarX/include/include_camera \
	frameworks/av/media/CedarX-Projects/CedarX/include/include_cedarv \
	frameworks/av/media/CedarX-Projects/CedarX/libcodecs/CODEC/VIDEO/DECODER \
	frameworks/av/media/CedarX-Projects/CedarX/libcodecs/CODEC/VIDEO/ENCODER \
	frameworks/av/media/CedarX-Projects/CedarX/libcodecs/MEMORY \
	frameworks/av/display \
	frameworks/include/include_adas \
	frameworks/av/media_proxy/media/media \
	$(TOP)/frameworks/include/include_media/media \
	$(TOP)/frameworks/include/include_gui \
	$(TARGET_HARDWARE_INCLUDE) \

LOCAL_SRC_FILES := \
	HALCameraFactory.cpp \
	PreviewWindow.cpp \
	CallbackNotifier.cpp \
	CCameraConfig.cpp \
	BufferListManager.cpp \
	OSAL_Mutex.c \
	OSAL_Queue.c \
	scaler.c \
	LibveDecoder.c \
	sonix/sonix_xu_ctrls.c \
	sonix/SonixUsbCameraDevice.cpp \
	sonix/v4l2uvc.c \
	FormatConvert.c \
	tvin/TVDecoder.cpp \
	uvc-gadget.c \
	jpegEncode/CameraJpegEncode.cpp \



# choose hal for new driver or old
SUPPORT_NEW_DRIVER := Y

ifeq ($(SUPPORT_NEW_DRIVER),Y)
LOCAL_CFLAGS += -DSUPPORT_NEW_DRIVER
LOCAL_SRC_FILES += \
	CameraHardware2.cpp \
	V4L2CameraDevice2.cpp
else
LOCAL_SRC_FILES += \
	CameraHardware.cpp \
	V4L2CameraDevice.cpp
endif

ifneq ($(filter nuclear%,$(TARGET_DEVICE)),)
LOCAL_CFLAGS += -D__SUN5I__
endif

#ifneq ($(filter crane%,$(TARGET_DEVICE)),)
#LOCAL_CFLAGS += -D__SUN4I__
#endif

ifneq ($(filter polaris%,$(TARGET_DEVICE)),)
LOCAL_CFLAGS += -D__SUN6I__
endif

ifneq ($(filter polaris%,$(TARGET_DEVICE)),)
LOCAL_CFLAGS += -D__SUNXI__
endif

ifeq ($(BOARD_HAVE_FACE_DETECT),true)
LOCAL_SHARED_LIBRARIES += libfd
LOCAL_C_INCLUDES += frameworks/include/include_fd
LOCAL_CFLAGS += -DFACE_DETECTION
endif

ifeq ($(BOARD_HAVE_WATER_MARK),true)
LOCAL_SHARED_LIBRARIES += libwater_mark
LOCAL_C_INCLUDES += frameworks/include/include_watermark
LOCAL_CFLAGS += -DWATERMARK_ENABLE
ifeq ($(BOARD_TARGET_DEVICE),tiger_cdr)
    LOCAL_CFLAGS += -DNO_PREVIEW_WATERMARK
endif
ifeq ($(BOARD_TARGET_DEVICE),crane_sdv)
    LOCAL_CFLAGS += -DNO_PREVIEW_WATERMARK
endif
endif

ifeq ($(BOARD_HAVE_MOTION_DETECT),true)
LOCAL_SHARED_LIBRARIES += libawmd
LOCAL_C_INCLUDES += frameworks/include/include_awmd
LOCAL_SRC_FILES += motionDetect/MotionDetect.cpp
LOCAL_CFLAGS += -DMOTION_DETECTION_ENABLE
ifeq ($(BOARD_TARGET_DEVICE),tiger_cdr)
    LOCAL_CFLAGS += -DMOTION_DETECTION_1M_MEMORY
endif

ifeq ($(BOARD_TARGET_DEVICE),tiger_standard)
    LOCAL_CFLAGS += -DMOTION_DETECTION_1M_MEMORY
endif

endif

ifeq ($(BOARD_HAVE_ADAS),true)
LOCAL_SHARED_LIBRARIES += libAdas
LOCAL_SRC_FILES += adas/CameraAdas.cpp \
				   adas/AdasCars.cpp \
				   adas/AVMediaPlayer.cpp \
				   adas/SemaphoreMutex.cpp
LOCAL_C_INCLUDES += adas
LOCAL_CFLAGS += -DADAS_ENABLE
LOCAL_LDFLAGS += -L$(TOP)/frameworks/prebuilts \
	   -lminigui_ths
endif

ifeq ($(BOARD_HAVE_QRDECODE),true)
LOCAL_SHARED_LIBRARIES += libqrdecode
LOCAL_SRC_FILES += qrdecode/CameraQrDecode.cpp
LOCAL_C_INCLUDES += $(TOP)/external/qrdecode/include
LOCAL_CFLAGS += -DQRDECODE_ENABLE
endif

ifeq ($(BOARD_TARGET_DEVICE),crane_ipc)
LOCAL_CFLAGS += -DTARGET_IPC_DEVICE
endif

#LOCAL_MODULE := camera.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE := camera.default

LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
