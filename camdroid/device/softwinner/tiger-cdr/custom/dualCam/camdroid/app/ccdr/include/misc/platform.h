#ifndef _PLATFORM_H
#define _PLATFORM_H

#define PLATFORM_1 1

#define APP_ADAS

//#define PLAY_LOOP

/*if you want open wifi function please also add the flow line in the Menu.h(include/window/Menu.h) file 
#define MENU_LIST_COUNT  22
*/
//#define APP_WIFI

#define ALLKEY_AWAKE_SCREEN
#define HWDECODE_JPEG

//#define FATFS
#ifdef FATFS
	#define STORAGE_PATH "0:/"
	#define STORAGE_POINT "0:"
#else
	#define STORAGE_PATH "/mnt/extsd/"
	#define STORAGE_POINT "/mnt/extsd"
#endif

#define TS_MUXER

#define UVC_CAR_BACK

#define DA280
#ifdef DA280
	#define THRESHOLD_HIGH_VALUE   17
	#define THRESHOLD_MIDDLE_VALUE 19
	#define THRESHOLD_LOW_VALUE    21
#else	//DA380
	#define THRESHOLD_HIGH_VALUE   11
	#define THRESHOLD_MIDDLE_VALUE 15
	#define THRESHOLD_LOW_VALUE    19
#endif

#endif //_PLATFORM_H
