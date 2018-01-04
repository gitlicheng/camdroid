#include <stdlib.h>
#include <string.h>

#include "cdr_widgets.h"

#undef LOG_TAG
#define LOG_TAG "RegisterWidgets.cpp"
#include "debug.h"

typedef int (*Proc) (HWND, int, WPARAM, LPARAM);
extern int MenuListCallback(HWND hwnd, int message, WPARAM wParam, LPARAM lParam);
extern int CdrProgressBarProc (HWND hWnd, int message, WPARAM wParam, LPARAM lParam);
extern BOOL RegisterCoolIndicatorControl (void);

/*
 * register widgets
 * On success return 0
 * On fail return -1
 * */
int RegisterCDRControls(void)
{
	WNDCLASS WndClass;

	WndClass.spClassName = CTRL_CDRMenuList;
	WndClass.dwStyle     = WS_NONE;
	WndClass.dwExStyle   = WS_EX_NONE;
	WndClass.hCursor     = GetSystemCursor (0);
	WndClass.iBkColor    = COLOR_lightgray;
	WndClass.WinProc = MenuListCallback;

	if(RegisterWindowClass(&WndClass) == FALSE) {
		db_error("register MenuList failed\n");
		return -1;
	}

	WndClass.spClassName = CTRL_CDRPROGRESSBAR;
	WndClass.dwStyle     = WS_NONE;
	WndClass.dwExStyle   = WS_EX_NONE;
	WndClass.hCursor     = GetSystemCursor (0);
	WndClass.iBkColor    = RGBA2Pixel(HDC_SCREEN, 0x00, 0x00, 0xff, 0x10);
	WndClass.WinProc = CdrProgressBarProc;

	if(RegisterWindowClass(&WndClass) == FALSE) {
		db_error("register MenuList failed\n");
		return -1;
	}

	RegisterCoolIndicatorControl();

	return 0;
}

void UnRegisterCDRControls(void)
{	
	UnregisterWindowClass(CTRL_CDRPROGRESSBAR);
	UnregisterWindowClass(CTRL_CDRMenuList);
}

