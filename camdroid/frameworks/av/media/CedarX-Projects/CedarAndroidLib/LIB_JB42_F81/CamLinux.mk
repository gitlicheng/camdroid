LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../Config.mk

LOCAL_PREBUILT_LIBS := libMemAdapter.so libvdecoder.so libVE.so
#libcedarxsftstream.so libthirdpartstream.so
LOCAL_PREBUILT_LIBS += libvencoder.so
# libfacedetection.so
ifeq ($(CEDARX_DEBUG_ENABLE),N)
LOCAL_PREBUILT_LIBS += libcedarxbase.so libaw_audio.so libaw_audioa.so 
# libstagefright_soft_cedar_h264dec.so libswdrm.so
#LOCAL_PREBUILT_LIBS += libswa1.so libswa2.so
endif
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)

