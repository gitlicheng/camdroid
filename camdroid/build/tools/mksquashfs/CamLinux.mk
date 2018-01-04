LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)

LOCAL_PREBUILT_EXECUTABLES := mksquashfs 

include $(BUILD_HOST_PREBUILT)

#################################################################
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := libltdl.so.7.2.2 
include $(BUILD_HOST_PREBUILT)

#################################################################
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := liblzma.a
include $(BUILD_HOST_PREBUILT)

#################################################################
include $(CLEAR_VARS)
#LOCAL_PREBUILT_LIBS := liblzma.la
include $(BUILD_HOST_PREBUILT)

#################################################################
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := liblzma.so
include $(BUILD_HOST_PREBUILT)

#################################################################
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := liblzma.so.5
include $(BUILD_HOST_PREBUILT)

#################################################################
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := liblzma.so.5.0.0
include $(BUILD_HOST_PREBUILT)

#################################################################
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := liblzo2.a
include $(BUILD_HOST_PREBUILT)

#################################################################
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := liblzo2.la
include $(BUILD_HOST_PREBUILT)

#################################################################
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := liblzo2.so.2
include $(BUILD_HOST_PREBUILT)

#################################################################
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := liblzo2.so.2.0.0
include $(BUILD_HOST_PREBUILT)

#################################################################
include $(CLEAR_VARS)
#LOCAL_PREBUILT_LIBS := libz.a
include $(BUILD_HOST_PREBUILT)

#################################################################
include $(CLEAR_VARS)
#LOCAL_PREBUILT_LIBS := libz.so
include $(BUILD_HOST_PREBUILT)

#################################################################
include $(CLEAR_VARS)
#LOCAL_PREBUILT_LIBS := libz.so.1
include $(BUILD_HOST_PREBUILT)

#################################################################
include $(CLEAR_VARS)
#LOCAL_PREBUILT_LIBS := libz.so.1.2.5
include $(BUILD_HOST_PREBUILT)

#################################################################
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := libfakeroot-0.so
include $(BUILD_HOST_PREBUILT)

#################################################################
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := libfakeroot.a
include $(BUILD_HOST_PREBUILT)

#################################################################
include $(CLEAR_VARS)
#LOCAL_PREBUILT_LIBS := libfakeroot.la
include $(BUILD_HOST_PREBUILT)

#################################################################
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := libfakeroot.so
include $(BUILD_HOST_PREBUILT)

#################################################################
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := libltdl.a
include $(BUILD_HOST_PREBUILT)

#################################################################
include $(CLEAR_VARS)
#LOCAL_PREBUILT_LIBS := libltdl.la
include $(BUILD_HOST_PREBUILT)

#################################################################
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := libltdl.so
include $(BUILD_HOST_PREBUILT)

#################################################################
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := libltdl.so.7
include $(BUILD_HOST_PREBUILT)

#################################################################
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := pkgconfig/zlib.pc
include $(BUILD_HOST_PREBUILT)

#################################################################
include $(CLEAR_VARS)
##LOCAL_PREBUILT_LIBS := pkgconfig/liblzma.pc
include $(BUILD_HOST_PREBUILT)


