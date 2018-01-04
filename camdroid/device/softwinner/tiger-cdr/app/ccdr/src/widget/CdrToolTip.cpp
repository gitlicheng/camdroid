
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

#include "cdr_widgets.h"

#define _ID_TIMER   100

static int CdrToolTipWinProc (HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
    static int iBorder;
    switch (message) {
    case MSG_CREATE:
        {
            const WINDOWINFO *info;
            int timeout = (int)GetWindowAdditionalData (hWnd);
            if (timeout >= 10)
                SetTimer (hWnd, _ID_TIMER, timeout / 10);

            info = GetWindowInfo (hWnd);
            iBorder = 
                info->we_rdr->calc_we_area (hWnd, LFRDR_METRICS_BORDER, NULL);
            SetWindowFont (hWnd, 
                (PLOGFONT)GetWindowElementAttr(hWnd, WE_FONT_TOOLTIP));
        }
        break;

    case MSG_TIMER:
		{
            KillTimer (hWnd, _ID_TIMER);
            if(IsWindowVisible(hWnd)) {
				ShowWindow (hWnd, SW_HIDE);
			}
		}
        break;

    case MSG_PAINT:
        {
            HDC hdc;
            const char* text;
            
            hdc = BeginPaint (hWnd);
 
            text = GetWindowCaption (hWnd);
            SetBkMode (hdc, BM_TRANSPARENT);

            SetTextColor (hdc, GetWindowElementPixel(hWnd, WE_FGC_TOOLTIP));
            TabbedTextOut (hdc, iBorder, iBorder, text);
 
            EndPaint (hWnd, hdc);
            return 0;
        }

    case MSG_DESTROY:
        {
            KillTimer (hWnd, _ID_TIMER);
            return 0;
        }

    case MSG_SETTEXT:
        {
            int timeout = (int)GetWindowAdditionalData (hWnd);
            if (timeout >= 10) {
                KillTimer (hWnd, _ID_TIMER);
                SetTimer (hWnd, _ID_TIMER, timeout / 10);
            }

            break;
        }
    }
 
    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

/* ToolTip Win without border or frame. */
HWND CreateCdrToolTipWin (HWND hParentWnd, int x, int y, int timeout_ms, const char* text, ...)
{
    HWND hwnd;
    char* buf = NULL;
    MAINWINCREATE CreateInfo;
    SIZE text_size;

    if (strchr (text, '%')) {
        va_list args;
        int size = 0;
        int i = 0;

        va_start(args, text);
        do {
            size += 1000;
            if (buf) free(buf);
            buf = (char*)malloc(size);
            i = vsnprintf(buf, size, text, args);
        } while (i == size);
        va_end(args);
    }
        
    SelectFont (HDC_SCREEN, 
            (PLOGFONT)GetWindowElementAttr(hParentWnd, WE_FONT_TOOLTIP));
    GetTabbedTextExtent (HDC_SCREEN, buf ? buf : text, -1, &text_size);

    if (x + text_size.cx > g_rcScr.right)
        x = g_rcScr.right - text_size.cx;
    if (y + text_size.cy > g_rcScr.bottom)
        y = g_rcScr.bottom - text_size.cy;

    CreateInfo.dwStyle = WS_VISIBLE;
    CreateInfo.dwExStyle = WS_EX_TOOLWINDOW | WS_EX_USEPARENTRDR;
    CreateInfo.spCaption = buf ? buf : text;
    CreateInfo.hMenu = 0;
    CreateInfo.hCursor = GetSystemCursor (IDC_ARROW);
    CreateInfo.hIcon = 0;
    CreateInfo.MainWindowProc = CdrToolTipWinProc;
    CreateInfo.lx = x;
    CreateInfo.ty = y;
    CreateInfo.rx = CreateInfo.lx + text_size.cx;
    CreateInfo.by = CreateInfo.ty + text_size.cy;
    CreateInfo.iBkColor = GetWindowElementPixel (hParentWnd, WE_BGC_TOOLTIP);
    CreateInfo.dwAddData = (DWORD) timeout_ms;
    CreateInfo.hHosting = hParentWnd;

    hwnd = CreateMainWindow (&CreateInfo);

    if (buf)
        free (buf);

    return hwnd;
}

void ResetCdrToolTipWin (HWND hwnd, int x, int y, const char* text, ...)
{
    char* buf = NULL;
    SIZE text_size;

    if (strchr (text, '%')) {
        va_list args;
        int size = 0;
        int i = 0;

        va_start(args, text);
        do {
            size += 1000;
            if (buf) free(buf);
            buf = (char*)malloc(size);
            i = vsnprintf(buf, size, text, args);
        } while (i == size);
        va_end(args);
    }

    SelectFont (HDC_SCREEN, 
            (PLOGFONT)GetWindowElementAttr(hwnd, WE_FONT_TOOLTIP));
    GetTabbedTextExtent (HDC_SCREEN, buf ? buf : text, -1, &text_size);

    SetWindowCaption (hwnd, buf ? buf : text);
    if (buf) free (buf);

    if (x + text_size.cx > g_rcScr.right)
        x = g_rcScr.right - text_size.cx;
    if (y + text_size.cy > g_rcScr.bottom)
        y = g_rcScr.bottom - text_size.cy;

    MoveWindow (hwnd, x, y, text_size.cx, text_size.cy, TRUE);
    ShowWindow (hwnd, SW_SHOWNORMAL);
}

void DestroyCdrToolTipWin (HWND hwnd)
{
    DestroyMainWindow (hwnd);
    MainWindowThreadCleanup (hwnd);
}
