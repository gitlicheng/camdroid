#ifndef SONIX_DEBUG
#define SONIX_DEBUG
#include<android/log.h>

#define LOG    "................................SONIX_CAMERA"

#define DBG_ENABLE 1


#ifdef __cplusplus
extern "C" {
#endif

#if  DBG_ENABLE
	 #define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG,__VA_ARGS__)
	 #define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG,__VA_ARGS__) 
	 #define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG,__VA_ARGS__) 
	 #define LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG,__VA_ARGS__)    
	 #define LOGF(...)  __android_log_print(ANDROID_LOG_FATAL,LOG,__VA_ARGS__) 
	 #define LOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE,LOG,__VA_ARGS__) 
#else
	 #define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG,__VA_ARGS__)
	 #define LOGD(...)
	 #define LOGI(...)
	 #define LOGW(...)
	 #define LOGF(...)
	 #define LOGV(...)
#endif

#ifdef __cplusplus
}
#endif

#endif


