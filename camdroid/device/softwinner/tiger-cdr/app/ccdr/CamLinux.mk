ifeq ($(TARGET_PRODUCT), tiger_cdr)
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

#ifneq ($(filter crane-cdr%,$(TARGET_DEVICE)),)
LOCAL_LDFLAGS += -L$(TOP)/frameworks/prebuilts \
-lminigui_ths
#-lrtspserver \
-lNATClient2 \
-lhttpsrv \

#strict compile option
#LOCAL_CFLAGS += -Werror  -Wall -Wno-unused-parameter -Wno-reorder
compile_date=$(shell date "+%Y-%m-%d %H:%M:%S")
#compile_version=$(addprefix $(addsuffix $(compile_date), "), ")
LOCAL_CFLAGS += -DCOMPILE_VERSION="\"$(USER)@$(TARGET_PRODUCT) $(compile_date)\""

LOCAL_CFLAGS += -D$(TARGET_PRODUCT)

ifeq ($(TARGET_PRODUCT), tiger_cdr)
	LOCAL_CFLAGS += -DNORMAL_STANDBY
endif

ifeq ($(TARGET_PRODUCT), tiger_cdr_standard)
	LOCAL_CFLAGS += -DNORMAL_STANDBY
endif

LOCAL_SHARED_LIBRARIES := \
	libmedia_proxy \
	libcutils \
	libutils \
	libbinder \
	libdatabase \
	libstorage \
	libhardware \
	libhardware_legacy \
	libcpuinfo \
	libmedia \
	libMemAdapter \
	libstandby \
	libcamera_client \
#	libnetutils

ifeq ($(CEDARX_FILE_SYSTEM),DIRECT_FATFS)
LOCAL_SHARED_LIBRARIES += libFatFs
LOCAL_CFLAGS += -DFATFS
endif
SRC_TAG := src/camera  \
	src/display \
	src/event \
	src/misc \
	src/power \
	src/storage  \
	src/widget \
	src/window \
	src/debug
#	src/ConnectivityManager
#	src/server
INC_TAG := include

define inc_sub_dir
$(shell cd $(LOCAL_PATH);find $(1)/* -type d)
endef

SRC_SUB_DIRS := $(call sub_dir, $(SRC_TAG))
INC_SUB_DIRS := $(addprefix $(LOCAL_PATH)/,$(call inc_sub_dir, $(INC_TAG)))

SRC_FILES := $(call all-cpp-files-under, $(SRC_TAG))
LOCAL_SRC_FILES := $(SRC_FILES)
LOCAL_C_INCLUDES := $(INC_SUB_DIRS) \
	$(TOP)/external/stlport/stlport \
	$(TOP)/frameworks/include/binder \
	$(TOP)/frameworks/include/include_gui \
	$(TOP)/frameworks/include/include_media/media \
	$(TOP)/frameworks/av/display \
	$(TOP)/frameworks/include \
	$(TOP)/frameworks/include/include_media/display \
	$(TOP)/frameworks/include/include_database \
	$(TOP)/frameworks/include/include_battery \
	$(TOP)/frameworks/include/include_storage \
	$(TOP)/frameworks/include/include_rtsp \
	$(TOP)/frameworks/include/include_http \
	$(TOP)/frameworks/include/standby \
	$(TOP)/frameworks/include/include_adas \
	$(TOP)/device/softwinner/common/hardware/camera/tvin \
	$(TOP)/device/softwinner/common/hardware/include \
	$(TOP)/external/fatfs
#	$(LOCAL_PATH)/include/window \
	$(LOCAL_PATH)/include/camera \
	$(LOCAL_PATH)/include/misc \
	$(LOCAL_PATH)/include/display \
	$(LOCAL_PATH)/include/storage \
	$(LOCAL_PATH)/include/event \
	$(LOCAL_PATH)/include/power \
	
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := ccdr

include $(BUILD_EXECUTABLE)
endif
#endif ifeq ($(TARGET_PRODUCT), tiger_cdr)
