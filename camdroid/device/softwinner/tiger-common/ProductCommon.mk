# full pluto product config
$(call inherit-product, $(SRC_TARGET_DIR)/product/CamLinuxFeatures.mk)
#$(call inherit-product, device/softwinner/common/sw-common.mk)

PRODUCT_PACKAGES += \
	audio.primary.default \
 audio.r_submix.default \
 audio.a2dp.default \
 audio.usb.default

PRODUCT_PACKAGES += \
	libcedarxbase \
	libcedarxosal \
	libcedarv \
	libcedarv_base \
	libcedarv_adapter \
	libve \
	libaw_audio \
	libaw_audioa \
	libswdrm \
	libstagefright_soft_cedar_h264dec \
	libfacedetection \
	libthirdpartstream \
	libcedarxsftstream \
	libsunxi_alloc \
	libsrec_jni \
	libjpgenc \
	libstagefrighthw \
	libOmxCore \
	libOmxVdec \
	libOmxVenc \
	libaw_h264enc \
	libI420colorconvert \
	libion_alloc

PRODUCT_PACKAGES += \
	StorageTest \
	BatteryTest \


PRODUCT_COPY_FILES += \
	device/softwinner/tiger-common/media_codecs.xml:system/etc/media_codecs.xml \
	device/softwinner/common/hardware/audio/audio_policy.conf:system/etc/audio_policy.conf \
	device/softwinner/common/hardware/audio/phone_volume.conf:system/etc/phone_volume.conf

	
#exdroid HAL
PRODUCT_PACKAGES += \
   camera.default \

# init.rc, kernel
PRODUCT_COPY_FILES += \
	device/softwinner/tiger-common/init.rc:root/init.rc \
