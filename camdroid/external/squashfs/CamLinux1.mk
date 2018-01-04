###############################################
#          Compression build options          #
###############################################
#
#
############# Building gzip support ###########
#
# Gzip support is by default enabled, and the compression type default
# (COMP_DEFAULT) is gzip.
#
# If you don't want/need gzip support then comment out the GZIP SUPPORT line
# below, and change COMP_DEFAULT to one of the compression types you have
# selected.
#
# Obviously, you must select at least one of the available gzip, lzma, lzo
# compression types.
#
GZIP_SUPPORT = 1

########### Building XZ support #############
#
# LZMA2 compression.
#
# XZ Utils liblzma (http://tukaani.org/xz/) is supported
#
# To build using XZ Utils liblzma - install the library and uncomment
# the XZ_SUPPORT line below.
#
#XZ_SUPPORT = 1


############ Building LZO support ##############
#
# The LZO library (http://www.oberhumer.com/opensource/lzo/) is supported.
#
# To build using the LZO library - install the library and uncomment the
# LZO_SUPPORT line below. If needed, uncomment and set LZO_DIR to the
# installation prefix.
#
#LZO_SUPPORT = 1
#LZO_DIR = /usr/local

########### Building LZMA support #############
#
# LZMA1 compression.
#
# LZMA1 compression is deprecated, and the newer and better XZ (LZMA2)
# compression should be used in preference.
#
# Both XZ Utils liblzma (http://tukaani.org/xz/) and LZMA SDK
# (http://www.7-zip.org/sdk.html) are supported
#
# To build using XZ Utils liblzma - install the library and uncomment
# the LZMA_XZ_SUPPORT line below.
#
# To build using the LZMA SDK (4.65 used in development, other versions may
# work) - download and unpack it, uncomment and set LZMA_DIR to unpacked source,
# and uncomment the LZMA_SUPPORT line below.
#
#LZMA_XZ_SUPPORT = 1
#LZMA_SUPPORT = 1
#LZMA_DIR = ../../../../LZMA/lzma465

######## Specifying default compression ########
#
# The next line specifies which compression algorithm is used by default
# in Mksquashfs.  Obviously the compression algorithm must have been
# selected to be built
#
COMP_DEFAULT = gzip

###############################################
#  Extended attribute (XATTRs) build options  #
###############################################
#
# Building XATTR support for Mksquashfs and Unsquashfs
#
# If your C library or build/target environment doesn't support XATTRs then
# comment out the next line to build Mksquashfs and Unsquashfs without XATTR
# support
XATTR_SUPPORT = 1

# Select whether you wish xattrs to be stored by Mksquashfs and extracted
# by Unsquashfs by default.  If selected users can disable xattr support by
# using the -no-xattrs option
#
# If unselected, Mksquashfs/Unsquashfs won't store and extract xattrs by
# default.  Users can enable xattrs by using the -xattrs option.
XATTR_DEFAULT = 1

LOCAL_PATH:= $(call my-dir)

#INCLUDEDIR = -I.

#
# Common definitions for host and device.
#

MKSRCFILES := mksquashfs.c read_fs.c sort.c swap.c pseudo.c compressor.c 
UNSRCFILES := unsquashfs.c unsquash-1.c unsquash-2.c unsquash-3.c unsquash-4.c swap.c compressor.c

# This is necessary to guarantee that the FDLIBM functions are in
# "IEEE spirit", i.e. to guarantee that the IEEE 754 core functions
# are used.
CFLAGS += -O2
CFLAGS += $(EXTRA_CFLAGS) -D_FILE_OFFSET_BITS=64 \
	-D_LARGEFILE_SOURCE -D_GNU_SOURCE -DCOMP_DEFAULT=\"$(COMP_DEFAULT)\" \
	-Wall
	
	
ifeq ($(GZIP_SUPPORT),1)
CFLAGS += -DGZIP_SUPPORT
MKSRCFILES += gzip_wrapper.c
UNSRCFILES += gzip_wrapper.c
#LIBS += -lz
COMPRESSORS += gzip
endif


#
# At least one compressor must have been selected
#
ifndef COMPRESSORS
$(error "No compressor selected! Select one or more of GZIP, LZMA, XZ or LZO!")
endif

#
# COMP_DEFAULT must be a selected compressor
#
ifeq (, $(findstring $(COMP_DEFAULT), $(COMPRESSORS)))
$(error "COMP_DEFAULT isn't selected to be built!")
endif


#
# Build for the target (device).
#

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng

LOCAL_SRC_FILES:= $(MKSRCFILES)
#LOCAL_CFLAGS += $(CFLAGS)

ifneq ($(filter $(TARGET_ARCH),arm x86),)
    # When __LITTLE_ENDIAN is set, the source will compile for
    # little endian cpus.
    LOCAL_CFLAGS += "-D__LITTLE_ENDIAN"
endif

#LOCAL_CFLAGS += -lz -lm -lpthread

LOCAL_SHARED_LIBRARIES := libc libz libm

LOCAL_MODULE := mksquashfs

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= $(UNSRCFILES)
LOCAL_CFLAGS := $(CFLAGS)

ifneq ($(filter $(TARGET_ARCH),arm x86),)
    # When __LITTLE_ENDIAN is set, the source will compile for
    # little endian cpus.
    LOCAL_CFLAGS += "-D__LITTLE_ENDIAN"
endif

LOCAL_SHARED_LIBRARIES := libz

LOCAL_MODULE := unsquashfs

include $(BUILD_EXECUTABLE)

