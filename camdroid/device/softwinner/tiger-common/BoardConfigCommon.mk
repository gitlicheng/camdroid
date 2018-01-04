# BoardConfig.mk
#
# Product-specific compile-time definitions.
#

# cpu stuff
TARGET_ARCH := arm
TARGET_CPU_SMP := true
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_ARCH_VARIANT := armv7-a-neon
ARCH_ARM_HAVE_TLS_REGISTER := true

TARGET_BOARD_PLATFORM := exDroid
TARGET_BOOTLOADER_BOARD_NAME := pluto

USE_OPENGL_RENDERER := true

# use our own init.rc
TARGET_PROVIDES_INIT_RC :=true

# no hardware camera
USE_CAMERA_STUB := true

#audio
HAVE_HTC_AUDIO_DRIVER := true
BOARD_USES_GENERIC_AUDIO := true

#gps 
BOARD_USES_GPS_TYPE := simulator

# Set /system/bin/sh to ash, not mksh, to make sure we can switch back.
# TARGET_SHELL := ash

# audio & camera & cedarx
CEDARX_CHIP_VERSION := F81
CEDARX_USE_SWAUDIO := Y

#widevine
BOARD_WIDEVINE_OEMCRYPTO_LEVEL := 3

# hardware module include file path
TARGET_HARDWARE_INCLUDE := $(TOP)/device/softwinner/common/hardware/include
