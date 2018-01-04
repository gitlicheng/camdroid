LOCAL_PATH:= $(call my-dir)

SOURCES := \
	src/Client.cpp \
	src/Extractor.c \
	src/gnu_getopt.c \
	src/gnu_getopt_long.c \
	src/Launch.cpp \
	src/List.cpp \
	src/Listener.cpp \
	src/Locale.c \
	src/main.cpp \
	src/PerfSocket.cpp \
	src/ReportCSV.c \
	src/ReportDefault.c \
	src/Reporter.c \
	src/Server.cpp \
	src/service.c \
	src/Settings.cpp \
	src/SocketAddr.c \
	src/sockets.c \
	src/stdio.c \
	src/tcp_window_size.c 
	
COMPAT := \
	compat/error.c \
	compat/inet_ntop.c \
	compat/signal.c \
	compat/string.c \
	compat/gettimeofday.c \
	compat/inet_pton.c \
	compat/snprintf.c \
	compat/Thread.c \
	compat/delay.cpp 

include $(CLEAR_VARS)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include/
LOCAL_SRC_FILES := $(SOURCES) $(COMPAT)
LOCAL_CFLAGS := -DHAVE_CONFIG_H -UAF_INET6 -Wno-error=format-security
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := optional 
LOCAL_MODULE := iperf
include $(BUILD_EXECUTABLE)
