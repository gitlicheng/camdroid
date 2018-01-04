
#define LOG_TAG "AdasCars"

#include <fcntl.h>
#include "AdasCars.h"

static int screen_w, screen_h;
void getScreenInfo(int *swidth, int *sheight)
{
	char *env = NULL;
	char mode[32]={0};
	if ((env=getenv("MG_DEFAULTMODE")) != NULL) {
		strncpy(mode, env, strlen(env)+1);
	}
	char *pW = NULL;
	char *pH = NULL;
	pW = strstr(mode, "x");
	*pW = '\0';
	*swidth = atoi(mode);
	*pW = 'x';
	pH = strstr(mode, "-");
	*pW = '\0';
	*sheight = atoi(pW+1);
	*pW = '-';

}

AdasCars::AdasCars(){
	mAdasTipsMode = 2;
	mCurBuf = 1;
	getScreenInfo(&screen_w, &screen_h);
	ALOGD("*** screen: screen_w:%d,screen_h:%d",screen_w,screen_h);
	mem_init();
	layer_car_init();
	mNeedDrawPic = false;
	mAvMedia = new AVMediaPlayer();
	mDrawAlign = 0;
	//getScreenInfo(&SCREEN_HEIGHT, &SCREEN_WIDTH);
}
	
AdasCars::~AdasCars(){
	mem_exit();
	if(mAvMedia)
		delete mAvMedia;
	close(mDisp_fd);
}

void AdasCars::initWarnImage(HDC hdc){
	char *filepath;
	int ret;
	for(int i =0;i<WARN_IMAGE_NUMBER;i++){
		switch(i){
			case CRASH_WARN:
				filepath=(char *)"/system/res/others/crash_warn.jpg";
				break;
			case LEFT_WARN:
				filepath=(char *)"/system/res/others/left_warn.jpg";
				break;
			case RIGHT_WARN:
				filepath=(char *)"/system/res/others/right_warn.jpg";
				break;
			case LEFT_CRASH_WARN:
				filepath=(char *)"/system/res/others/left_crash_warn.jpg";
				break;
			case RIGHT_CRASH_WARN:
				filepath=(char *)"/system/res/others/right_crash_warn.jpg";
				break;
			default:
				filepath=NULL;
				break;	
		}
		if(filepath)
			ret = LoadBitmapFromFile(hdc, &warnImage[i], filepath);
		if(ret!= ERR_BMP_OK){
			ALOGD("load the  %d %d warn bitmap error",i, ret);
		}	
	}


}

void AdasCars::initDistImage(HDC hdc)
{
	char *filepath= (char *)"/system/res/others/dist_";
	char fpath[64]={0};
	int ret;
	for(int i =0;i<DIST_IMAGE_NUMBER;i++){
		sprintf(fpath,"%s%d.png",filepath,i+1);
		//ALOGD("<***fpath=%s*****filepath=%s**>",fpath,filepath);
		ret = LoadBitmapFromFile(hdc, &distImage[i], fpath);
		if(ret!= ERR_BMP_OK){
			ALOGD("load the %d Dist bitmap error",ret);	
		}
	}
}

#ifdef USE_ALIGN_IMAGE
void AdasCars::initAlignImage(HDC hdc)
{
	char *filepath= (char *)"/system/res/others/adas_calibration_line.png";
	int ret = LoadBitmapFromFile(hdc, &mAlignImage, filepath);
	if(ret!= ERR_BMP_OK){
		ALOGD("load the adas_calibration_line bitmap error");
	}
}

void AdasCars::showAlignImage(HDC hdc)
{
	FillBoxWithBitmap(hdc, 0, 0, screen_w,screen_h-30, &mAlignImage);
}
#endif

void AdasCars::showDistImage(HDC hdc,int x,int y,int w,int h,int ds,bool showflag)
{
	if(ds>DIST_IMAGE_NUMBER)
		return ;
	BITMAP *Image;
	int wi = w*4/5;
	int he = h*4/5;
	if (wi < 28) {
		wi = 28;
		he = 12;
	}else{
		wi = 52;
		he = 24;
	}
	if (x + wi > screen_w){
		wi = screen_w-x;
	}
	Image = &distImage[ds-1];
	if(y-he<0){
		FillBoxWithBitmap(hdc,x,0,wi,he,Image);
	}else{
		FillBoxWithBitmap(hdc,x,y-he,wi,he,Image);
	}
	
}

void AdasCars::unLoadImage()
{
	int i ;
	for(i=0;i<WARN_IMAGE_NUMBER;i++){
		UnloadBitmap(&warnImage[i]);
	}
	for(i=0;i<DIST_IMAGE_NUMBER;i++){
		UnloadBitmap(&distImage[i]);
	}
#ifdef USE_ALIGN_IMAGE
	UnloadBitmap(&mAlignImage);
#endif

}

void AdasCars::showWarnBitmap(int warn,HDC hdc)
{
	BITMAP *Image;
	Image=&warnImage[warn];
	FillBoxWithBitmap(hdc,0,0,screen_w,screen_h,Image);
}

static int convertCoordinate(int dispVal, int imgVal, int val) {
    	return (int)(1.0 * dispVal * val / imgVal);
}

void AdasCars::update(int idx)
{
//	ALOGD("%s %d\n", __FUNCTION__, __LINE__);
	MemAdapterFlushCache(mBuf[idx].addr_vir,screen_w*screen_h*4);
	
	disp_layer_config car_layer_config;
	
	car_layer_config.channel = 2;
	car_layer_config.layer_id = 1;
	layer_get_para(&car_layer_config);
	
	car_layer_config.info.fb.addr[0] = (unsigned long long)mBuf[idx].addr_phy;
	layer_set_para(&car_layer_config);
	
	mem_set(idx, false);
}

void AdasCars::setAdasAudioVol(float val)
{
	mAdasAudioVol= val;
	ALOGD("***********mAdasAudioVol:%f",mAdasAudioVol);
}

void AdasCars::adasSetTipsMode(int mode)
{
	mAdasTipsMode = mode;
	ALOGD("  ****mAdasTipsMode:%d*** ", mAdasTipsMode);
}

void AdasCars::drawCars(ADASOUT_IF mAd_if){
	int free_idx = 	mem_get();
	bool ugly = false;
//	ALOGD("%s %d free_idx%d\n", __FUNCTION__, __LINE__,free_idx);
	if (free_idx < 0) {
		return ;
	}
	setAdasMemeryOx(mBuf[free_idx].addr_vir);
	HDC hdc = mBuf[free_idx].dc;
	if(hdc == HDC_INVALID){
		ALOGD("<***HDC_INVALID***>");
		return ;
	}
	
	int i,dist,j;
	bool carwarn=false;
	int lwarn = 0, rwarn = 0;
	ADASOUT *adas_out = mAd_if.adasOut;
	int subW = mAd_if.subWidth;
	int subH = mAd_if.subHeight;

	if(mDrawAlign < 2*60*10){
#ifdef USE_ALIGN_IMAGE
		showAlignImage(hdc);
#else
		drawALignLine(hdc);
#endif
		ugly = true;
		mDrawAlign ++;
	}
		
	mNeedDrawPic = false;	
	for (i = 0; i < adas_out->cars.Num && i < CAR_CNT; ++i) {
		if (adas_out->cars.carP[i].idx.x + adas_out->cars.carP[i].idx.width > subW ||
			adas_out->cars.carP[i].idx.y + adas_out->cars.carP[i].idx.height > subH) {
			
				break;  //³¬³ö·¶Î§
		}
		ugly = true;
		int x = convertCoordinate(screen_w, subW, adas_out->cars.carP[i].idx.x);
		int w = convertCoordinate(screen_w, subW, adas_out->cars.carP[i].idx.width);
		int y = convertCoordinate(screen_h, subH, adas_out->cars.carP[i].idx.y);
		y -= EXTRA_HEIGHT;	//status bar
		int h = convertCoordinate(screen_h, subH, adas_out->cars.carP[i].idx.height);
		int len = (int)(w * 1.0 / 6);
		mCarLines[0] = x;
		mCarLines[1] = y;
		mCarLines[2] = x + len;
		mCarLines[3] = y;
		mCarLines[4] = x;
		mCarLines[5] = y;
		mCarLines[6] = x;
		mCarLines[7] = y + len;
	
		mCarLines[8] = x + w - len;
		mCarLines[9] = y;
		mCarLines[10] = x + w;
		mCarLines[11] = y;
		mCarLines[12] = x + w;
		mCarLines[13] = y;
		mCarLines[14] = x + w;
		mCarLines[15] = y + len;
	
		mCarLines[16] = x + w - len;
		mCarLines[17] = y + h;
		mCarLines[18] = x + w;
		mCarLines[19] = y + h;
		mCarLines[20] = x + w;
		mCarLines[21] = y + h - len;
		mCarLines[22] = x + w;
		mCarLines[23] = y + h;
		
		mCarLines[24] = x;
		mCarLines[25] = y + h;
		mCarLines[26] = x + len;
		mCarLines[27] = y + h;
		mCarLines[28] = x;
		mCarLines[29] = y + h - len;
		mCarLines[30] = x;
		mCarLines[31] = y + h;

		if(adas_out->cars.carP[i].isWarn != 0 ){
			carwarn=true;
			mNeedDrawPic=true;
		}
		
		if (adas_out->cars.carP[i].color == 1) { //yellow
			SetPenColor(hdc, RGBA2Pixel(hdc, 0xff, 0xff, 0x00, 0xff));
		} else if (adas_out->cars.carP[i].color == 2){	//red
			SetPenColor(hdc, RGBA2Pixel(hdc, 0xff, 0x00, 0x00, 0xff));
		} else {	//0
			SetPenColor(hdc, RGBA2Pixel(hdc, 0xff, 0xff, 0xff, 0xff));
		}
		for(j =3;j<32;j+=4){
			drawLines(hdc,mCarLines[j-3],mCarLines[j-2],mCarLines[j-1],mCarLines[j]);
		}	
		dist = (int)(adas_out->cars.carP[i].dist + 0.5);
		//int text_x = (w > 24) ? (w-24)/2+x : x;
		//int text_y = (h > 8) ? (h-8)/2+y : y;
		showDistImage(hdc, x, y,w,h,dist,true);
	}


	if (adas_out->lane.isDisp) {
		 
		SetPenColor(hdc, RGBA2Pixel(hdc, 0x00, 0xff, 0x00, 0xff));
		int lx0,ly0,lx1,ly1,rx0,ry0,rx1,ry1;
		lx0 = adas_out->lane.ltIdxs[0].x;
		ly0 = adas_out->lane.ltIdxs[0].y;
		lx1 = adas_out->lane.ltIdxs[1].x;
		ly1 = adas_out->lane.ltIdxs[1].y;
		rx0 = adas_out->lane.rtIdxs[0].x;
		ry0 = adas_out->lane.rtIdxs[0].y;
		rx1 = adas_out->lane.rtIdxs[1].x;
		ry1 = adas_out->lane.rtIdxs[1].y;


		lx0 = convertCoordinate(screen_w, subW, lx0);
		ly0 = convertCoordinate(screen_h, subH, ly0) - EXTRA_HEIGHT;
		lx1 = convertCoordinate(screen_w, subW, lx1);
		ly1 = convertCoordinate(screen_h, subH, ly1) - EXTRA_HEIGHT;

		rx0 = convertCoordinate(screen_w, subW, rx0);
		ry0 = convertCoordinate(screen_h, subH, ry0) - EXTRA_HEIGHT;
		rx1 = convertCoordinate(screen_w, subW, rx1);
		ry1 = convertCoordinate(screen_h, subH, ry1) - EXTRA_HEIGHT;

		drawRoadLine(hdc,lx0,ly0,lx1,ly1);
		drawRoadLine(hdc,rx0,ry0,rx1,ry1);

		if(adas_out->lane.ltWarn != 0){
			lwarn = adas_out->lane.ltWarn;
			mNeedDrawPic=true;
		}else{
			if(adas_out->lane.rtWarn !=0){
				rwarn = adas_out->lane.rtWarn;
				mNeedDrawPic=true;
	
			}
		}
		ugly = true;
	}
	
	if(lwarn != 0 && carwarn){
		showWarnBitmap(LEFT_CRASH_WARN,hdc);
		ugly = true;
		if(!mAvMedia->isPlaying())
			mAvMedia->startPlay("/system/res/others/car_warn.mp3",mAdasAudioVol);
	} else if (rwarn != 0 && carwarn){
		showWarnBitmap(RIGHT_CRASH_WARN,hdc);
		ugly = true;
		if(!mAvMedia->isPlaying())
			mAvMedia->startPlay("/system/res/others/car_warn.mp3",mAdasAudioVol);
	} else if(lwarn > 0 ){
		//showWarnBitmap(LEFT_WARN,hdc);
		ugly = true;
		if(mAdasTipsMode == 1){
			if(!mAvMedia->isPlaying() && (lwarn == 255)){
				showWarnBitmap(LEFT_WARN,hdc);
				mAvMedia->startPlay("/system/res/others/left_line_warn.mp3",mAdasAudioVol);
			}
		}else if(mAdasTipsMode == 2){
			if(!mAvMedia->isPlaying()){
				if(lwarn == 255){
					showWarnBitmap(LEFT_WARN,hdc);
					mAvMedia->startPlay("/system/res/others/left_line_warn.mp3",mAdasAudioVol);
				}else if(lwarn == 128)
					mAvMedia->startPlay("/system/res/others/line_warn.mp3",mAdasAudioVol);
			}
		}
	} else if(rwarn > 0){
		//showWarnBitmap(RIGHT_WARN,hdc);
		ugly = true;
		if(mAdasTipsMode == 1){
			if(!mAvMedia->isPlaying() && (rwarn == 255)){
				showWarnBitmap(RIGHT_WARN,hdc);
				mAvMedia->startPlay("/system/res/others/right_line_warn.mp3",mAdasAudioVol);
			}
		}else if(mAdasTipsMode == 2){
			if(!mAvMedia->isPlaying()){
				if(rwarn == 255){
					showWarnBitmap(RIGHT_WARN,hdc);
					mAvMedia->startPlay("/system/res/others/right_line_warn.mp3",mAdasAudioVol);
				}else if(rwarn == 128)
					mAvMedia->startPlay("/system/res/others/line_warn.mp3",mAdasAudioVol);
			}			
		}

	} else if(carwarn){
		showWarnBitmap(CRASH_WARN,hdc);
		ugly = true;
		if(!mAvMedia->isPlaying())
			mAvMedia->startPlay("/system/res/others/car_warn.mp3",mAdasAudioVol);
	}
	if (ugly) {
		update(free_idx);
	}
}
	
void AdasCars::drawRoadLine(HDC hdc,int x1,int y1,int x2,int y2){
	SetPenWidth (hdc, 5);
	LineEx (hdc, x1, y1, x2, y2);
}
	
void AdasCars::drawLines(HDC hdc,int x1,int y1,int x2,int y2){
	MoveTo(hdc,x1,y1);
	LineTo(hdc,x2,y2);
}

void AdasCars::drawALignLine(HDC hdc){
	float midRow = (float)55 * SUB_HEIGHT / 100;
	float midCol = (float)SUB_WIDTH / 2;
	float vLen = (float)SUB_HEIGHT -midRow - 2;
	float ang = (float)(55.0 / 180 * PI);
	float hLen = (float)((float)vLen * tan(ang));
	
	int UpX = convertCoordinate(screen_w, SUB_WIDTH, midCol);
	int UpY = convertCoordinate(screen_h, SUB_HEIGHT, midRow)- EXTRA_HEIGHT;
	int leftDnX = convertCoordinate(screen_w, SUB_WIDTH, midCol - hLen);
	int leftDnY = convertCoordinate(screen_h, SUB_HEIGHT, midRow + vLen)- EXTRA_HEIGHT;
	int rightDnX = convertCoordinate(screen_w, SUB_WIDTH, midCol + hLen);
	int rightDnY = convertCoordinate(screen_h, SUB_HEIGHT, midRow + vLen)- EXTRA_HEIGHT;		

	SetPenWidth(hdc,5);
	SetPenColor(hdc, RGBA2Pixel(hdc, 0x00, 0x00, 0xff, 0xff));
	LineEx(hdc,UpX, UpY, leftDnX, leftDnY);
	LineEx(hdc,UpX, UpY, rightDnX, rightDnY);

}

//get free buf
int AdasCars::mem_get()
{
	
	int nextBuf = 1 - mCurBuf;
	//ALOGD("%s %d mBuf[nextBuf].ugly:%d\n",__FUNCTION__, __LINE__, mBuf[nextBuf].ugly);
	if (!mBuf[nextBuf].ugly) {
		mCurBuf = 1 - mCurBuf;
		return mCurBuf;
	}
	return -1;
}

//set buf ugly
void AdasCars::mem_set(int idx, bool ugly)
{
	//ALOGD("%s %d ugly:%d\n",__FUNCTION__, __LINE__, ugly);
	mBuf[idx].ugly = ugly;	
}

void AdasCars::mem_init()
{
	
	InitFreeClipRectList (&__mg_FreeClipRectList, 1);
	ALOGD("%s %d\n", __FUNCTION__, __LINE__);
	struct fb_var_screeninfo var;
	struct fb_fix_screeninfo fix;
	unsigned char *fb_vir, *fb_phy;
	int fd_fb = open("/dev/graphics/fb0", O_RDWR);
	ioctl(fd_fb, FBIOGET_FSCREENINFO, &fix);
        ioctl(fd_fb, FBIOGET_VSCREENINFO, &var);
	int x = var.xres;
	int y = var.yres;
	int Bpp = var.bits_per_pixel / 8;
	int one_frame_size = Bpp * x * y;
	ALOGD("<**Bpp:%d one_frame_size:%d x:%d y:%d**>",Bpp,one_frame_size,x,y);
	MYBITMAP myBitmap;
	MemAdapterOpen();
	for(int i=0; i<LAYER_ADAS_BUF_CNT; i++) {
		mBuf[i].addr_vir = (unsigned long *)MemAdapterPalloc(one_frame_size);
		memset(mBuf[i].addr_vir, 0x0, one_frame_size);
		//*addr_phy = (unsigned long *)MemAdapterGetPhysicAddress((void*)addr_vir);
		mBuf[i].ugly = false;
		mBuf[i].addr_phy = (unsigned long *)MemAdapterGetPhysicAddressCpu((void*)mBuf[i].addr_vir);
		myBitmap.flags  = MYBMP_FLOW_DOWN | MYBMP_TYPE_RGB | MYBMP_ALPHA;
		myBitmap.frames = 1;
		myBitmap.depth  = 32;
		myBitmap.w      = screen_w;
		myBitmap.h      = screen_h- EXTRA_HEIGHT;
		myBitmap.pitch  = screen_w * 4;
		myBitmap.size   = screen_w*(screen_h - EXTRA_HEIGHT)*4;
		myBitmap.bits   = (BYTE*)mBuf[i].addr_vir;
		RGB pal[256] = {{0,0,0,0}};
		mBuf[i].dc =CreateMemDCFromMyBitmap (&myBitmap,pal);
		ALOGD("dc :0x%x", mBuf[i].dc);
	}
}

void AdasCars::mem_exit()
{
	unLoadImage();
	for(int i=0; i<LAYER_ADAS_BUF_CNT; i++) {
		if(mBuf[i].addr_vir) {
			if (mBuf[i].dc) {
				DeleteMemDC(mBuf[i].dc);
			}
			MemAdapterPfree(mBuf[i].addr_vir);
		}
	}
	MemAdapterClose();
	DestroyFreeClipRectList (&__mg_FreeClipRectList);
}


void setAdasMemeryOx(unsigned long *addr_vir)
{
	memset(addr_vir, 0x0, 4*screen_w*screen_h);
}

int layer_config(__DISP_t cmd, disp_layer_config *pinfo)
{
	unsigned long args[4] = {0};
	unsigned int ret = 0;
	
	args[0] = 0;
    args[1] = (unsigned long)pinfo;
    args[2] = 1;
    ret = ioctl(mDisp_fd, cmd, args);
	if(0 != ret) {
		ALOGD("fail to get para\n");
		ret = -1;
	}
	return ret;
}
int layer_set_para(disp_layer_config *pinfo)
{
	return layer_config(DISP_LAYER_SET_CONFIG, pinfo);
}

int layer_get_para(disp_layer_config *pinfo)
{
	return layer_config(DISP_LAYER_GET_CONFIG, pinfo);
}
void AdasCars::layer_car_init()
{
	ALOGD("%s %d\n", __FUNCTION__, __LINE__);
	mDisp_fd = open("/dev/disp", O_RDWR);
	disp_layer_config car_layer_config, ui_config;
	ui_config.channel = 2;
	ui_config.layer_id = 0;
	layer_get_para(&ui_config); //get ui para
	car_layer_config = ui_config;
	car_layer_config.channel = 2;
	car_layer_config.layer_id = 1;
	car_layer_config.enable = 0;
	car_layer_config.info.zorder = 11;
	int idx = mem_get();
	car_layer_config.info.fb.addr[0] = (long long unsigned int)mBuf[idx].addr_phy;
	car_layer_config.info.screen_win.x      = 0;
	car_layer_config.info.screen_win.y      = 26;
	car_layer_config.info.screen_win.width  = screen_w;
	car_layer_config.info.screen_win.height = (screen_h - EXTRA_HEIGHT);
	ui_config.info.zorder = 10;
	layer_set_para(&ui_config);
	layer_set_para(&car_layer_config);
	HDC hdc = mBuf[idx].dc;
	initWarnImage(hdc);
	initDistImage(hdc);
#ifdef USE_ALIGN_IMAGE
	initAlignImage(hdc);
#endif
}

void draw_line(unsigned long *addr_vir)
{
	ALOGD("*********%s %d\n", __FUNCTION__, __LINE__);
	int i;
	unsigned int *tmp = (unsigned int *)addr_vir;
	for(i=0; i<100; i++) {
		*(tmp+i) = 0xffff0000;
	}
}
