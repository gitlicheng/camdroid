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

ifeq ($(SW_BOARD_USR_WIFI),AP6181)
   include hardware/broadcom/wlan/bcmdhd/firmware/ap6181/device-bcm.mk
endif

ifeq ($(SW_BOARD_USR_WIFI),AP6210)
   include hardware/broadcom/wlan/bcmdhd/firmware/ap6210/device-bcm.mk
endif

ifeq ($(SW_BOARD_USR_WIFI),AP6330)
   include hardware/broadcom/wlan/bcmdhd/firmware/ap6330/device-bcm.mk
endif

ifeq ($(SW_BOARD_USR_WIFI), AP6335)
   include hardware/broadcom/wlan/bcmdhd/firmware/ap6335/device-bcm.mk
endif

ifeq ($(SW_BOARD_USR_WIFI), AP6212)
   include hardware/broadcom/wlan/bcmdhd/firmware/ap6212/device-bcm.mk
endif
