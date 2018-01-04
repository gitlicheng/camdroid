#
# Copyright (C) 2007 The Android Open Source Project
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

PRODUCT_BRAND := generic
PRODUCT_DEVICE := generic
PRODUCT_NAME := core

PRODUCT_PACKAGES += \
    abcc \
    apache-xml \
    atrace \
    bu \
    cacerts \
    hprof-conv \
    libbcc \
    libdownmix \
    libvariablespeed \

#PRODUCT_COPY_FILES += \
#    system/core/rootdir/init.usb.rc:root/init.usb.rc \
#    system/core/rootdir/init.trace.rc:root/init.trace.rc \

# host-only dependencies
#ifeq ($(WITH_HOST_DALVIK),true)
#    PRODUCT_PACKAGES += \
#        apache-xml-hostdex \
#        bouncycastle-hostdex \
#        core-hostdex \
#        libcrypto \
#        libexpat \
#        libicui18n \
#        libicuuc \
#        libjavacore \
#        libssl \
#        libz-host \
#        dalvik \
#        zoneinfo-host.dat \
#        zoneinfo-host.idx \
#        zoneinfo-host.version
#endif
#
#ifeq ($(HAVE_SELINUX),true)
#    PRODUCT_PACKAGES += \
#        sepolicy \
#        file_contexts \
#        seapp_contexts \
#        property_contexts \
#        mac_permissions.xml
#endif

$(call inherit-product, $(SRC_TARGET_DIR)/product/base.mk)

