#
# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

#
# This file should set PRODUCT_MAKEFILES to a list of product makefiles
# to expose to the build system.  LOCAL_DIR will already be set to
# the directory containing this file.
# PRODUCT_MAKEFILES is set up in AndroidProducts.mks.
# Format of PRODUCT_MAKEFILES:
# <product_name>:<path_to_the_product_makefile>
# If the <product_name> is the same as the base file name (without dir
# and the .mk suffix) of the product makefile, "<product_name>:" can be
# omitted.
#
# This file may not rely on the value of any variable other than
# LOCAL_DIR; do not use any conditionals, and do not look up the
# value of any variable that isn't set in this file or in a file that
# it includes.
#

# Unbundled apps will be built with the most generic product config.
$(call inherit-product, $(SRC_TARGET_DIR)/product/base.mk)

ifeq ($(strip $(filter all,$(CAMLINUX_PRODUCT_FEATURE))),all)
$(call inherit-product, $(SRC_TARGET_DIR)/product/external.mk) 
$(call inherit-product, $(SRC_TARGET_DIR)/product/audio.mk) 
$(call inherit-product, $(SRC_TARGET_DIR)/product/media.mk) 
$(call inherit-product, $(SRC_TARGET_DIR)/product/videoencoder.mk) 
$(call inherit-product, $(SRC_TARGET_DIR)/product/debug.mk)  
$(call inherit-product, $(SRC_TARGET_DIR)/product/storage.mk) 
$(call inherit-product, $(SRC_TARGET_DIR)/product/voicecall.mk)
else
ifeq ($(strip $(filter external,$(CAMLINUX_PRODUCT_FEATURE))),external)
$(call inherit-product, $(SRC_TARGET_DIR)/product/external.mk) 
endif

ifeq ($(strip $(filter audio,$(CAMLINUX_PRODUCT_FEATURE))),audio)
$(call inherit-product, $(SRC_TARGET_DIR)/product/audio.mk) 
endif

ifeq ($(strip $(filter media,$(CAMLINUX_PRODUCT_FEATURE))),media)
$(call inherit-product, $(SRC_TARGET_DIR)/product/media.mk) 
endif

ifeq ($(strip $(filter mediarecoder,$(CAMLINUX_PRODUCT_FEATURE))),mediarecoder)
$(call inherit-product, $(SRC_TARGET_DIR)/product/mediarecoder.mk) 
endif

ifeq ($(strip $(filter wifi,$(CAMLINUX_PRODUCT_FEATURE))),wifi)
$(call inherit-product, $(SRC_TARGET_DIR)/product/wifi.mk) 
endif

ifeq ($(strip $(filter onvif,$(CAMLINUX_PRODUCT_FEATURE))),onvif)
$(call inherit-product, $(SRC_TARGET_DIR)/product/onvif.mk) 
endif

ifeq ($(strip $(filter videoencoder,$(CAMLINUX_PRODUCT_FEATURE))),videoencoder)
$(call inherit-product, $(SRC_TARGET_DIR)/product/videoencoder.mk) 
endif

ifeq ($(strip $(filter webserver,$(CAMLINUX_PRODUCT_FEATURE))),webserver)
$(call inherit-product, $(SRC_TARGET_DIR)/product/webserver.mk) 
endif

ifeq ($(strip $(filter webkit,$(CAMLINUX_PRODUCT_FEATURE))),webkit)
$(call inherit-product, $(SRC_TARGET_DIR)/product/webkit.mk) 
endif

ifeq ($(strip $(filter debug,$(CAMLINUX_PRODUCT_FEATURE))),debug)
$(call inherit-product, $(SRC_TARGET_DIR)/product/debug.mk)  
endif

ifeq ($(strip $(filter storage,$(CAMLINUX_PRODUCT_FEATURE))),storage)
$(call inherit-product, $(SRC_TARGET_DIR)/product/storage.mk) 
endif

ifeq ($(strip $(filter ipcam,$(CAMLINUX_PRODUCT_FEATURE))),ipcam)
$(call inherit-product, $(SRC_TARGET_DIR)/product/ipcam.mk) 
endif

ifeq ($(strip $(filter voicecall,$(CAMLINUX_PRODUCT_FEATURE))),voicecall)
$(call inherit-product, $(SRC_TARGET_DIR)/product/voicecall.mk)
endif

ifeq ($(strip $(filter rtsp,$(CAMLINUX_PRODUCT_FEATURE))),rtsp)
$(call inherit-product, $(SRC_TARGET_DIR)/product/rtsp.mk)
endif

ifeq ($(strip $(filter http,$(CAMLINUX_PRODUCT_FEATURE))),http)
$(call inherit-product, $(SRC_TARGET_DIR)/product/http.mk)
endif

ifeq ($(strip $(filter nat,$(CAMLINUX_PRODUCT_FEATURE))),nat)
$(call inherit-product, $(SRC_TARGET_DIR)/product/nat.mk)
endif

ifeq ($(strip $(filter smtpclt,$(CAMLINUX_PRODUCT_FEATURE))),smtpclt)
$(call inherit-product, $(SRC_TARGET_DIR)/product/smtpclt.mk)
endif

ifeq ($(strip $(filter adb,$(CAMLINUX_PRODUCT_FEATURE))),adb)
$(call inherit-product, $(SRC_TARGET_DIR)/product/adb.mk)
endif

ifeq ($(strip $(filter txdevicesdk,$(CAMLINUX_PRODUCT_FEATURE))),txdevicesdk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/txdevicesdk.mk)
endif

ifeq ($(strip $(filter qqipc,$(CAMLINUX_PRODUCT_FEATURE))),qqipc)
$(call inherit-product, $(SRC_TARGET_DIR)/product/qqipc.mk)
endif

ifeq ($(strip $(filter IPCamera2.0,$(CAMLINUX_PRODUCT_FEATURE))),IPCamera2.0)
$(call inherit-product, $(SRC_TARGET_DIR)/product/IPCamera2.0.mk)
endif

ifeq ($(strip $(filter wifitest,$(CAMLINUX_PRODUCT_FEATURE))),wifitest)
$(call inherit-product, $(SRC_TARGET_DIR)/product/wifitest.mk)
endif

endif
