LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/libechocancel \
	$(TOP)/frameworks/include/include_media/media \
	$(LOCAL_PATH)/libwebrtc_neteq/include \
	$(LOCAL_PATH)/libwebrtc_neteq/src \
	$(LOCAL_PATH)/codecs/libamrnb/common/include \
	$(LOCAL_PATH)/codecs/libamrnb/enc/src \
	$(LOCAL_PATH)/codecs/libamrnb/dec/src \
	$(LOCAL_PATH)/codecs/libopus/include \
	$(LOCAL_PATH)/codecs/libg726/include \
	$(LOCAL_PATH)/codecs/libplc

LOCAL_SRC_FILES := \
	SoundRecorder.cpp \
	SoundPlayer.cpp \
	AudioCodec.cpp \
	echocancel.c \
	BufferManager.cpp \

CONFIG_CODEC := AMR
PLAY_USE_WEBRTC_NETEQ := false

ifeq ($(findstring AMR,$(CONFIG_CODEC)),AMR)
LOCAL_SRC_FILES += codecs/AmrCodec.cpp
LOCAL_STATIC_LIBRARIES += libamrnb
LOCAL_CFLAGS += -DUSE_CODEC_AMR
endif

ifeq ($(findstring OPUS,$(CONFIG_CODEC)),OPUS)
LOCAL_SRC_FILES += codecs/OpusCodec.cpp
LOCAL_CFLAGS += -DUSE_CODEC_OPUS
LOCAL_STATIC_LIBRARIES += libopus
endif

ifeq ($(findstring G726,$(CONFIG_CODEC)),G726)
LOCAL_SRC_FILES += codecs/G726Codec.cpp
LOCAL_CFLAGS += -DUSE_CODEC_G726
LOCAL_STATIC_LIBRARIES += libg726
endif

ifeq ($(findstring SPEEX,$(CONFIG_CODEC)),SPEEX)
LOCAL_SRC_FILES += codecs/SpeexCodec.cpp
LOCAL_CFLAGS += -DUSE_CODEC_SPEEX
LOCAL_STATIC_LIBRARIES += libspeex-codec
endif

LOCAL_LDFLAGS += $(LOCAL_PATH)/libechocancel/libechocancel.a
LOCAL_LDFLAGS += $(LOCAL_PATH)/codecs/libplc/libplc.a

ifeq ($(PLAY_USE_WEBRTC_NETEQ), true)
LOCAL_SRC_FILES += libwebrtc_neteq/src/WebrtcNeteq_Interface.c
LOCAL_CFLAGS += -DPLAY_USE_WEBRTC_NETEQ
#LOCAL_STATIC_LIBRARIES += libwebrtc_neteq
LOCAL_STATIC_LIBRARIES += \
	libwebrtc_neteq_internal \
	libwebrtc_amrnb \
	libwebrtc_cng \
	libwebrtc_opus \
	libwebrtc_g726 \
	libwebrtc_spl \
	libwebrtc_spl_neon
endif

LOCAL_SHARED_LIBRARIES += libutils libmedia

LOCAL_SHARED_LIBRARIES += \
    libcutils \
    libdl \
    libstlport

LOCAL_MODULE := libvoicecall
include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
