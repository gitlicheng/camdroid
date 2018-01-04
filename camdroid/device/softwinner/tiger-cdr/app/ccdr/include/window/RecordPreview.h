#ifndef _RECORD_PREVIEW_H
#define _RECORD_PREVIEW_H

#include <time.h>
#include "windows.h"
#include "keyEvent.h"
#include "cdr_message.h"
#include "ResourceManager.h"
#include "camera.h"
#include "Dialog.h"

#define BIG_RECT_W 320
#define BIG_RECT_H 240
#define BIG_RECT_X 0
#define BIG_RECT_Y 0


#define SML_RECT_W (BIG_RECT_W*2/5)
#define SML_RECT_H (SML_RECT_W*BIG_RECT_H/BIG_RECT_W)
#define SML_RECT_X (BIG_RECT_W - SML_RECT_W)
#define SML_RECT_Y (30)


#define LOCAL2SESSION(idx) (idx+CAM_CNT)
#define SESSION2LOCAL(idx) (idx-CAM_CNT)
#define CAM_OTHER(idx) (1-idx)

#define Max_Capacity_Support			(4*1024)		//MB 	

#define MAX_FILE_SIZE			3750	//MB
#define MIN_FILE_SPCE		50		//MB

enum eRecordTimeLength {
	RTL_0 = 1,
	RTL_1 = 2,
	RTL_2 = 5,
	RTL_3 = 10,
	RTL_AVAILABLE=0xff,  
};

#endif
