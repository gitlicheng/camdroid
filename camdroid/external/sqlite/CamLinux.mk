LOCAL_PATH:= $(call my-dir)
common_sqlite_flags := \
        -DNDEBUG=1 \
        -DHAVE_USLEEP=1 \
        -DSQLITE_HAVE_ISNAN \
        -DSQLITE_DEFAULT_JOURNAL_SIZE_LIMIT=1048576 \
        -DSQLITE_THREADSAFE=2 \
        -DSQLITE_TEMP_STORE=3 \
        -DSQLITE_POWERSAFE_OVERWRITE=1 \
        -DSQLITE_DEFAULT_FILE_FORMAT=4 \
        -DSQLITE_DEFAULT_AUTOVACUUM=1 \
        -DSQLITE_ENABLE_MEMORY_MANAGEMENT=1 \
        -DSQLITE_ENABLE_FTS3 \
        -DSQLITE_ENABLE_FTS3_BACKWARDS \
        -DSQLITE_ENABLE_FTS4 \
        -DSQLITE_OMIT_BUILTIN_TEST \
        -DSQLITE_OMIT_COMPILEOPTION_DIAGS \
        -DSQLITE_OMIT_LOAD_EXTENSION \
        -DSQLITE_DEFAULT_FILE_PERMISSIONS=0600
common_src_files := sqlite3.c
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(common_src_files)

ifneq ($(TARGET_ARCH),arm)
LOCAL_LDLIBS += -lpthread -ldl
endif

LOCAL_CFLAGS += $(common_sqlite_flags) -DUSE_PREAD64 -Dfdatasync=fdatasync \
                                -DHAVE_MALLOC_USABLE_SIZE
LOCAL_SHARED_LIBRARIES := libdl
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE:= libsqlite
LOCAL_C_INCLUDES += $(call include-path-for, system-core)/cutils
LOCAL_SHARED_LIBRARIES += liblog \
            libutils \
            liblog

# include android specific methods
#LOCAL_WHOLE_STATIC_LIBRARIES := libsqlite3_android


include $(BUILD_SHARED_LIBRARY)
