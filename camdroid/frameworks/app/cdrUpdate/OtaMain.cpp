
#include "OtaUpdate.h"

#define OTA_TIMER  1001
#define CDR_WIDTH  320
#define COST_TIME  120
#ifdef tiger_cdr
#define CDR_HEIGHT 480
#define Y_POS      180
#else
#define CDR_HEIGHT 240
#define Y_POS      90
#endif

HWND	 hMainWnd;

static int screen_w = CDR_WIDTH;
static int screen_h = CDR_HEIGHT;

void drawProgress(HWND hwnd,int p,bool flag)
{
	HDC hdc;
	hdc = GetDC(hwnd);
    SetPenColor(hdc, RGBA2Pixel(hdc, 0xff, 0x00, 0x00, 0xff));
	//ALOGD("****p:%d******",p);
	if(p<230){
		TextOut(hdc ,80, Y_POS-30, "OTA UPDATE NOW!");
	}else{
		UpdateWindow(hwnd,1);
		if(flag )
			TextOut(hdc ,80, Y_POS-30, "OTA UPDATE SUCCESSED!");
		else {
			TextOut(hdc ,80, Y_POS-30, "OTA UPDATE FAILED!");
		}
	}
	Rectangle(hdc,38,Y_POS,CDR_WIDTH-38,Y_POS+40);
	SetPenWidth (hdc, 36);
	LineEx (hdc, 40,Y_POS+20, 40+p,Y_POS+20);

}

static int OTAPro(HWND hWnd,int  message, WPARAM wParam, LPARAM lParam)
{	
	HDC  hdc;
	static int pos=0;	
	switch(message)
	{
		case MSG_CREATE:
		{	
			pos=0;
			SetTimer(hWnd,OTA_TIMER,100);
		}
			break;
		case MSG_TIMER:
			if(wParam == OTA_TIMER){
				pos+=1;
				//ALOGD("****pos:%d******",pos);
				if(pos<230){
					drawProgress(hWnd,pos,false);
				}else{
					drawProgress(hWnd,230,false);
				}
			}
			break;
		case MSG_UPDATE_OVER:
			{
				ALOGD("******MSG_UPDATE_OVER****wParam:%d*****",wParam);
				if(wParam){
					drawProgress(hWnd,240,true);
				}else{
					drawProgress(hWnd,235,false);
				}
			}
			break;
		case MSG_CLOSE:
			KillTimer(hWnd,OTA_TIMER);
			DestroyMainWindow(hWnd);
			PostQuitMessage(hWnd);
			break;
		default:
			return DefaultMainWinProc(hWnd,message,wParam,lParam);
	}
	return 0;
}


static void getScreenInfo(int *w, int *h)
{
	static int swidth = 0;
	static int sheight =0;
	if(swidth == 0 ){
		char *env = NULL;
		char mode[32]={0};
		if ((env=getenv("MG_DEFAULTMODE")) != NULL) {
			strncpy(mode, env, strlen(env)+1);
		} else {
			return;
		}
		char *pW = NULL;
		char *pH = NULL;
		pW = strstr(mode, "x");
		*pW = '\0';
		swidth = atoi(mode);
		*pW = 'x';
		pH = strstr(mode, "-");
		*pW = '\0';
		sheight = atoi(pW+1);
		*pW = '-';
	}
	*w = swidth;
	*h = sheight;
}

int initWindow()
{
	MAINWINCREATE CreateInfo;
	CreateInfo.dwStyle=WS_VISIBLE ;
	CreateInfo.dwExStyle = WS_EX_NONE | WS_EX_TROUNDCNS | WS_EX_BROUNDCNS;
    CreateInfo.spCaption = "OTA Update";
	CreateInfo.hMenu = 0;
    CreateInfo.hCursor = GetSystemCursor(0);
 	CreateInfo.hIcon = 0;
	CreateInfo.MainWindowProc = OTAPro;
	CreateInfo.lx = 0;
 	CreateInfo.ty = 0;
	getScreenInfo(&screen_w, &screen_h);
	CreateInfo.rx = screen_w;
	CreateInfo.by = screen_h;
	CreateInfo.iBkColor = COLOR_lightwhite;
	CreateInfo.dwAddData = 0;
	CreateInfo.hHosting = HWND_DESKTOP;
	
	hMainWnd = CreateMainWindow(&CreateInfo);
	if(hMainWnd==HWND_INVALID)
		return 0;
	return 1;
}


int MiniGUIMain(int argc, const char* argv[])
{
	MSG msg;
	if(!initWindow()){
		fprintf(stderr,"create ota window failed");
	}
	ShowWindow(hMainWnd,SW_SHOWNORMAL);
	OtaUpdate *ota = new OtaUpdate(hMainWnd);
	ota->startUpdate();
	while(GetMessage(&msg, hMainWnd)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	MainWindowThreadCleanup(hMainWnd);
	return 0;
}
