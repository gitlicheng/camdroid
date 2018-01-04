#ifndef ADAS_CARS_H
#define ADAS_CARS_H


#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

#include<math.h>
#include<Adas.h>
#include<Adas_interface.h>
#include "AVMediaPlayer.h"
#include <cutils/log.h>

#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <sunxi_display2.h>

#include <cutils/log.h>


#define CAR_CNT 5

typedef struct lAdas_buffer
{
	unsigned long *addr_vir;
	unsigned long *addr_phy;
	HDC dc;
	bool ugly;
}lAdas_buffer;

#define LAYER_ADAS_BUF_CNT 2

#define ADAS_WARN_ONLY
#define USE_ALIGN_IMAGE

//#define SCREEN_WIDTH  320
//#define SCREEN_HEIGHT  240

#define EXTRA_HEIGHT  30
#define SUB_WIDTH  640
#define SUB_HEIGHT 360
#define PI 3.1415926

#define CAR_NUMBER  8
#define DIST_IMAGE_NUMBER 45
#define WARN_IMAGE_NUMBER 5

enum {
	CRASH_WARN=0,
	LEFT_WARN,
	RIGHT_WARN,
	LEFT_CRASH_WARN,
	RIGHT_CRASH_WARN,
	NO_WARN
};


void setAdasMemeryOx(unsigned long *addr_ir);
int layer_config(__DISP_t cmd, disp_layer_config *pinfo);
int layer_set_para(disp_layer_config *pinfo);
int layer_get_para(disp_layer_config *pinfo);

extern BLOCKHEAP __mg_FreeClipRectList;

extern "C" {
extern int MemAdapterOpen(void);
extern void MemAdapterClose(void);
extern void* MemAdapterPalloc(int nSize);
extern void  MemAdapterPfree(void* pMem);
extern void  MemAdapterFlushCache(void* pMem, int nSize);
extern void* MemAdapterGetPhysicAddress(void* pVirtualAddress);
extern void* MemAdapterGetVirtualAddress(void* pPhysicAddress);
extern void* MemAdapterGetPhysicAddressCpu(void* pVirtualAddress);
extern void* MemAdapterGetVirtualAddressCpu(void* pCpuPhysicAddress);
}
static int mDisp_fd;
static float mAdasAudioVol = 0.6;

class AdasCars{

public:
	AdasCars();
	~AdasCars();
	void drawCars(ADASOUT_IF mAd_if);
	void drawRoadLine(HDC hdc,int x1,int y1,int x2,int y2);	
	void drawLines(HDC hdc,int x1,int y1,int x2,int y2);
	void initWarnImage(HDC hdc);
	void initDistImage(HDC hdc);
#ifdef USE_ALIGN_IMAGE
	void initAlignImage(HDC hdc);
	void showAlignImage(HDC hdc);
#endif
	void showWarnBitmap(int warn,HDC hdc);
	void showDistImage(HDC hdc,int x,int y,int w,int h,int ds,bool showflag);
	void unLoadImage();
	static void drawALignLine(HDC hdc);
	//get free buf
	int mem_get();
	void mem_set(int idx, bool ugly);
	void mem_init();
	void mem_exit();
	void layer_car_init();
	void update(int idx);
	static void setAdasAudioVol(float val);
	void  adasSetTipsMode(int mode);
private:
	ADASOUT_IF mAdas_if;
	int mCarLines[32];
	BITMAP	warnImage[WARN_IMAGE_NUMBER];
	bool mNeedDrawPic;
	BITMAP distImage[DIST_IMAGE_NUMBER];
	bool mInitWarnImage;
	AVMediaPlayer * mAvMedia;
	lAdas_buffer mBuf[LAYER_ADAS_BUF_CNT];
	int mCurBuf;
	int mDrawAlign;
	int mAdasTipsMode;   // 1: simple hint ,  2: global function hint
#ifdef USE_ALIGN_IMAGE
	BITMAP	mAlignImage;
#endif	
};
#endif //ADAS_CARS_H
