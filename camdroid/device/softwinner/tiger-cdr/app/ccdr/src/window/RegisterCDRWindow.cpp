#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include "windows.h"
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "RegisterCDRWindow.cpp"
#include "debug.h"

extern int StatusBarProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam);
extern int RecordPreviewProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam);
extern int PlayBackPreviewProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam);
extern int PlayBackProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam);
extern int MenuProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam);

typedef int (*Proc) (HWND, int, WPARAM, LPARAM);
int RegisterCDRWindows(void)
{
	int count;

	const char *windowClass[] = {
		WINDOW_STATUSBAR,
		WINDOW_RECORDPREVIEW,
		WINDOW_PLAYBACKPREVIEW,
		WINDOW_PLAYBACK,
		WINDOW_MENU
	};

	WNDCLASS WndClass = {
		NULL,
		0,
		WS_CHILD | WS_VISIBLE,
		WS_EX_USEPARENTFONT,
		GetSystemCursor(0),
		COLOR_lightgray,
		NULL,
		0
	};

	Proc windowProc[] = {
		StatusBarProc,
		RecordPreviewProc,
		PlayBackPreviewProc,
		PlayBackProc,
		MenuProc
	};

	for( count = 0; count < 5; count++) {
		WndClass.spClassName = windowClass[count];
		WndClass.WinProc = windowProc[count];
		WndClass.iBkColor = RGBA2Pixel(HDC_SCREEN, 0xFF, 0x00, 0x00, 0x00);

		if(RegisterWindowClass(&WndClass) == FALSE) {
			db_error("register %s failed\n", windowClass[count]);
			return -1;
		}
	}
	return 0;
}

void UnRegisterCDRWindows(void)
{
	UnregisterWindowClass(WINDOW_MENU);
	UnregisterWindowClass(WINDOW_PLAYBACK);
	UnregisterWindowClass(WINDOW_PLAYBACKPREVIEW);
	UnregisterWindowClass(WINDOW_RECORDPREVIEW);
	UnregisterWindowClass(WINDOW_STATUSBAR);
}
