# BoardConfig.mk
#
# Product-specific compile-time definitions.
#

include device/softwinner/tiger-common/BoardConfigCommon.mk


# image related
TARGET_NO_BOOTLOADER := true
TARGET_NO_RECOVERY := false
TARGET_NO_KERNEL := false

INSTALLED_KERNEL_TARGET := kernel
BOARD_KERNEL_BASE := 0x40000000
BOARD_KERNEL_CMDLINE := 
#TARGET_USERIMAGES_USE_EXT4 := true
BOARD_FLASH_BLOCK_SIZE := 4096
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 536870912
#BOARD_USERDATAIMAGE_PARTITION_SIZE := 1073741824

# recovery stuff
TARGET_RECOVERY_PIXEL_FORMAT := "BGRA_8888"
TARGET_RECOVERY_UI_LIB := librecovery_ui_nvr_standard
#TARGET_RECOVERY_UPDATER_LIBS :=
#add by huangtingjin ir driver
SW_BOARD_IR_RECOVERY := true

# wifi and bt configuration

# 1. Wifi Configuration
# BOARD_WIFI_VENDOR := realtek
#BOARD_WIFI_VENDOR := broadcom

# 1.1 realtek wifi support
ifeq ($(BOARD_WIFI_VENDOR), realtek)
    WPA_SUPPLICANT_VERSION := VER_0_8_X
    BOARD_WPA_SUPPLICANT_DRIVER := NL80211
    BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_rtl
    BOARD_HOSTAPD_DRIVER        := NL80211
    BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_rtl
    
    #SW_BOARD_USR_WIFI := rtl8192cu
    #BOARD_WLAN_DEVICE := rtl8192cu

#    SW_BOARD_USR_WIFI := rtl8188eu
#    BOARD_WLAN_DEVICE := rtl8188eu

    SW_BOARD_USR_WIFI := rtl8189es
    BOARD_WLAN_DEVICE := rtl8189es

    #SW_BOARD_USR_WIFI := rtl8723as
    #BOARD_WLAN_DEVICE := rtl8723as

    #SW_BOARD_USR_WIFI := rtl8723au
    #BOARD_WLAN_DEVICE := rtl8723au
endif

# 1.2 broadcom wifi support
ifeq ($(BOARD_WIFI_VENDOR), broadcom)
    BOARD_WPA_SUPPLICANT_DRIVER := NL80211
    WPA_SUPPLICANT_VERSION      := VER_0_8_X
    BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd
    BOARD_HOSTAPD_DRIVER        := NL80211
    BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_bcmdhd
    BOARD_WLAN_DEVICE           := bcmdhd
    WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/bcmdhd/parameters/firmware_path"

    #SW_BOARD_USR_WIFI := bcm40181
    #WIFI_DRIVER_FW_PATH_STA    := "/system/vendor/modules/fw_bcm40181a2.bin"
    #WIFI_DRIVER_FW_PATH_P2P    := "/system/vendor/modules/fw_bcm40181a2_p2p.bin"
    #WIFI_DRIVER_FW_PATH_AP     := "/system/vendor/modules/fw_bcm40181a2_apsta.bin"

    #SW_BOARD_USR_WIFI := bcm40183
    #WIFI_DRIVER_FW_PATH_STA    := "/system/vendor/modules/fw_bcm40183b2.bin"
    #WIFI_DRIVER_FW_PATH_P2P    := "/system/vendor/modules/fw_bcm40183b2_p2p.bin"
    #WIFI_DRIVER_FW_PATH_AP     := "/system/vendor/modules/fw_bcm40183b2_apsta.bin"

    SW_BOARD_USR_WIFI := AP6210
    WIFI_DRIVER_FW_PATH_STA    := "/system/vendor/modules/fw_bcm40181a2.bin"
    WIFI_DRIVER_FW_PATH_P2P    := "/system/vendor/modules/fw_bcm40181a2_p2p.bin"
    WIFI_DRIVER_FW_PATH_AP     := "/system/vendor/modules/fw_bcm40181a2_apsta.bin"
endif

# 2. Bluetooth Configuration
# make sure BOARD_HAVE_BLUETOOTH is true for every bt vendor
#BOARD_HAVE_BLUETOOTH := true
#BOARD_HAVE_BLUETOOTH_BCM := true
#SW_BOARD_HAVE_BLUETOOTH_RTK := true
#SW_BOARD_HAVE_BLUETOOTH_NAME := bcm40183
#SW_BOARD_HAVE_BLUETOOTH_NAME := ap6210
#BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := device/softwinner/nvr-publicboard/bluetooth/

BOARD_HAVE_WATER_MARK          := true
BOARD_HAVE_MOTION_DETECT       := true
#BOARD_HAVE_ADAS                := true
#BOARD_HAVE_QRDECODE            := true

#BOARD_RECORD_SUPPORT_MULTI_SAMPLERATE  := true

#CedarX config
# CEDARX_WRITE_FILE_METHOD value: FS_THREAD_CACHE, FS_SIMPLE_CACHE, FS_NO_CACHE
CEDARX_WRITE_FILE_METHOD := FS_SIMPLE_CACHE
# CEDARX_FILE_SYSTEM value: LINUX_VFS, DIRECT_FATFS
CEDARX_FILE_SYSTEM := DIRECT_IO

BOARD_TARGET_DEVICE             :=tiger_cdr

BOARD_CONFIG_V4L2_VIDEO_CAP_BUF_NUM := 5

