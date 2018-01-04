# pluto fpga product config

$(call inherit-product, device/softwinner/tiger-common/ProductCommon.mk)
$(call inherit-product-if-exists, device/softwinner/tiger-cdr/modules/modules.mk)
$(call inherit-product-if-exists, device/softwinner/tiger-cdr/rootfs/rootfs.mk)

PRODUCT_PACKAGES += \
	fs_mgr \
	make_ext4fs \
	libzipfile \
	libunz \
	standbyservice

PRODUCT_PACKAGES += \
	libminigui_ths-3.0.so.12 \
	libpng \
	startup_music \
	cdrUpdate \
	sensors.default \
	ccdr \
	fatTest

# for dragonboard
#PRODUCT_PACKAGES += \
	dragonboard \
	parser \
	ddrtester \
	mictester \
	nortester \
	rtctester \
	acctester \
	tftester \
	videotester \
	wifitester
	
PRODUCT_COPY_FILES += \
	device/softwinner/tiger-cdr/kernel:kernel \
	device/softwinner/tiger-cdr/init.sun8i.rc:root/init.sun8i.rc \
	device/softwinner/tiger-cdr/init.sun8i.usb.rc:root/init.sun8i.usb.rc \
	device/softwinner/tiger-cdr/ueventd.sun8i.rc:root/ueventd.sun8i.rc \
	device/softwinner/tiger-cdr/configs/camera.cfg:system/etc/camera.cfg \
	device/softwinner/tiger-cdr/configs/sunxi-keyboard.kl:system/etc/sunxi-keyboard.kl \
	device/softwinner/tiger-cdr/configs/MiniGUI.cfg:system/etc/MiniGUI.cfg \
	device/softwinner/tiger-cdr/configs/gsensor.cfg:system/etc/gsensor.cfg \
	device/softwinner/tiger-cdr/configs/media_profiles.xml:system/etc/media_profiles.xml \
	device/softwinner/tiger-cdr/vold.fstab:system/etc/vold.fstab

PRODUCT_COPY_FILES += \
	device/softwinner/tiger-cdr/modules/modules/videobuf-core.ko:/system/vendor/modules/videobuf-core.ko \
	device/softwinner/tiger-cdr/modules/modules/videobuf-dma-contig.ko:/system/vendor/modules/videobuf-dma-contig.ko \
	device/softwinner/tiger-cdr/modules/modules/cci.ko:/system/vendor/modules/cci.ko \
	device/softwinner/tiger-cdr/modules/modules/vfe_os.ko:/system/vendor/modules/vfe_os.ko \
	device/softwinner/tiger-cdr/modules/modules/vfe_subdev.ko:/system/vendor/modules/vfe_subdev.ko \
	device/softwinner/tiger-cdr/modules/modules/ar0330_mipi.ko:/system/vendor/modules/ar0330_mipi.ko \
	device/softwinner/tiger-cdr/modules/modules/vfe_v4l2.ko:/system/vendor/modules/vfe_v4l2.ko \
	device/softwinner/tiger-cdr/modules/modules/da380.ko:/system/vendor/modules/da380.ko \
	device/softwinner/tiger-cdr/modules/modules/sw-device.ko:/system/vendor/modules/sw-device.ko \
	device/softwinner/tiger-cdr/modules/modules/fatfs.ko:/system/vendor/modules/fatfs.ko \
	device/softwinner/tiger-cdr/modules/modules/uvcvideo.ko:/system/vendor/modules/uvcvideo.ko

generate_font := $(shell device/softwinner/tiger-cdr/sxf_maker.sh)

src_font_files := $(shell ls device/softwinner/tiger-cdr/res/font)
src_menu_files := $(shell ls device/softwinner/tiger-cdr/res/menu)
src_others_files := $(shell ls device/softwinner/tiger-cdr/res/others)
src_topbar_files := $(shell ls device/softwinner/tiger-cdr/res/topbar)
src_cfg_files := $(shell ls device/softwinner/tiger-cdr/res/cfg)
src_lang_files := $(shell ls device/softwinner/tiger-cdr/res/lang)
src_data_files := $(shell ls device/softwinner/tiger-cdr/res/data)

PRODUCT_COPY_FILES += $(foreach file, $(src_lang_files), \
	device/softwinner/tiger-cdr/res/lang/$(file):system/res/lang/$(file))
PRODUCT_COPY_FILES += $(foreach file, $(src_font_files), \
	device/softwinner/tiger-cdr/res/font/$(file):system/res/font/$(file))
PRODUCT_COPY_FILES += $(foreach file, $(src_background_files), \
	device/softwinner/tiger-cdr/res/background/$(file):system/res/background/$(file)) 
PRODUCT_COPY_FILES += $(foreach file, $(src_menu_files), \
	device/softwinner/tiger-cdr/res/menu/$(file):system/res/menu/$(file))
PRODUCT_COPY_FILES += $(foreach file, $(src_others_files), \
	device/softwinner/tiger-cdr/res/others/$(file):system/res/others/$(file))
PRODUCT_COPY_FILES += $(foreach file, $(src_topbar_files), \
	device/softwinner/tiger-cdr/res/topbar/$(file):system/res/topbar/$(file))
PRODUCT_COPY_FILES += $(foreach file, $(src_cfg_files), \
	device/softwinner/tiger-cdr/res/cfg/$(file):system/res/cfg/$(file))
PRODUCT_COPY_FILES += $(foreach file, $(src_data_files), \
	device/softwinner/tiger-cdr/res/data/$(file):system/res/cfg/$(file))

PRODUCT_COPY_FILES += \
	$(call find-copy-subdir-files,*.bmp,device/softwinner/tiger-cdr/watermark,system/watermark)

#PRODUCT_COPY_FILES += \
#	$(call find-copy-subdir-files,*,$(LOCAL_PATH)/hawkview,system/etc/hawkview)
	
PRODUCT_PROPERTY_OVERRIDES += \
	sys.usb.config=mass_storage,adb \
	ro.font.scale=1.0 \
	ro.hwa.force=true \
	rw.logger=0 \
	ro.sys.bootfast=true\
	ro.aw.sensordiscard=8
	
CAMLINUX_PRODUCT_FEATURE := external media  audio  storage videoencoder debug  camera  adb

PRODUCT_CHARACTERISTICS := cdr

# Overrides
PRODUCT_BRAND  := softwinners
PRODUCT_NAME   :=tiger_cdr
PRODUCT_DEVICE := tiger-cdr
PRODUCT_MODEL  := SoftwinerEvb

