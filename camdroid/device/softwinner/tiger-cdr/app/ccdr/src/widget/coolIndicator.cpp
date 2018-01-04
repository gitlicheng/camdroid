
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include "include/cliprect.h"
#include "include/internals.h"
#include "include/ctrlclass.h"
#include "coolIndicator.h"
#include "cdr_widgets.h"
#include "cdr_message.h"

#undef LOG_TAG
#define LOG_TAG "coolIndicator"
#include "debug.h"

#define ITEMBARWIDTH    8

#define LEN_HINT    50
#define LEN_TITLE   10

static BOOL enable_hint = TRUE;

typedef struct cool_indicatorCTRL
{
    int    	nCount;
    int 	ItemWidth;
    int		ItemHeight;
    PBITMAP BackBmp;
    struct 	cool_indicator_ItemData* head;
    struct 	cool_indicator_ItemData* tail;
    int 	iSel; 				    // the current select item
    HWND    hToolTip;
}COOL_INDICATOR_CTRL;
typedef COOL_INDICATOR_CTRL* PCOOL_INDICATOR_CTRL;

typedef struct cool_indicator_ItemData
{
      RECT      RcTitle;            // title and where clicked
      int       hintx, hinty;       // position of hint box
      int       id;                 // id
      int	    ItemType;	
      BOOL	    Disable;
      PBITMAP   Bmp;
	  gal_pixel sliderColor;
      char      Hint [LEN_HINT + 1];
      char      Caption[LEN_TITLE + 1];
      struct    cool_indicator_ItemData* next; //page linked list next
} COOL_INDICATOR_ITEMDATA;
typedef  COOL_INDICATOR_ITEMDATA* PCOOL_INDICATOR_ITEMDATA;

static int CoolIndicatorCtrlProc (HWND hWnd, int uMsg, WPARAM wParam, LPARAM lParam);

BOOL RegisterCoolIndicatorControl (void)
{
    WNDCLASS WndClass;
        
    WndClass.spClassName = CTRL_COOL_INDICATOR;
    WndClass.dwStyle     = WS_NONE;
    WndClass.dwExStyle   = WS_EX_NONE;
    WndClass.hCursor     = GetSystemCursor (IDC_ARROW);
    WndClass.iBkColor    = GetWindowElementPixel (HWND_DESKTOP, WE_MAINC_THREED_BODY);
    WndClass.WinProc     = CoolIndicatorCtrlProc;

	if(RegisterWindowClass(&WndClass) == FALSE) {
		db_error("register Cool Indicator failed\n");
		return FALSE;
	}

	return TRUE;
}

static COOL_INDICATOR_ITEMDATA* GetCurSel (PCOOL_INDICATOR_CTRL pdata)
{
	COOL_INDICATOR_ITEMDATA*  tmpdata;
	int count=0;

    tmpdata = pdata->head;
    while (tmpdata) { 
		if(count == pdata->iSel)
			return tmpdata;		
        tmpdata = tmpdata->next;
		count++;
	}

    return NULL;         
}

static int enable_item (PCOOL_INDICATOR_CTRL TbarData, int id, BOOL flag)
{
    COOL_INDICATOR_ITEMDATA*  tmpdata;
   
    tmpdata = TbarData->head;
    while (tmpdata) { 
        if (tmpdata->id == id) {
            tmpdata->Disable = !flag;    
            return 0;
        }
        tmpdata = tmpdata->next;
    }

    return -1;
}

static void ShowCurItemHint (HWND hWnd, PCOOL_INDICATOR_CTRL cb)
{
	COOL_INDICATOR_ITEMDATA* tmpdata;
    PCOOL_INDICATOR_CTRL TbarData = (PCOOL_INDICATOR_CTRL) GetWindowAdditionalData2(hWnd);
	int x, y;

	tmpdata = GetCurSel(cb);
	if(tmpdata == NULL)
		return;

	x = tmpdata->hintx;
	y = tmpdata->hinty;

    if (tmpdata->Hint [0] == '\0')
        return;

    ClientToScreen (hWnd, &x, &y);
    if (g_rcScr.bottom - y < y - g_rcScr.top) {
        y -= (GetSysCharHeight () + (2 << 1) + cb->ItemHeight);
    }

    if (TbarData->hToolTip == 0) {
        TbarData->hToolTip = CreateCdrToolTipWin (hWnd, x, y, 1000, tmpdata->Hint);
		SetWindowFont(TbarData->hToolTip, GetWindowFont(hWnd));
    }
    else {
        ResetCdrToolTipWin (TbarData->hToolTip, x, y, tmpdata->Hint);
    }
}
/*
 *                       
 *         
 *                                  ¨x¨x¨x¨x¨x¨x¨xtop_line(dark)
 *                                   ---------top_line(light)
 *                                ¨† ¨‡        ¨‡¨†
 *        left_line(dark+light)   ¨† ¨‡        ¨‡¨†right_line(light+dark)
 *                                ¨† ¨‡        ¨‡¨†
 *    base_line_left ¨{¨{¨{¨{¨{¨{¨{                ¨{¨{¨{¨{¨{¨{¨{ base_line_right
 */
static void draw_hilight_box (HWND hWnd, HDC hdc, COOL_INDICATOR_ITEMDATA* item)
{
    int  l,r,t,b; 
    WINDOWINFO *info = (WINDOWINFO*)GetWindowInfo (hWnd);
    DWORD color;
    DWORD mainc = GetWindowElementAttr (hWnd, WE_FGC_THREED_BODY);	//UI@hilight_box_color
    
	if(item == NULL)
		return;

    l = item->RcTitle.left;
    t = item->RcTitle.top + 2;
    r = item->RcTitle.right + 1;
    b = item->RcTitle.bottom + 1;

    color = info->we_rdr->calc_3dbox_color (mainc, LFRDR_3DBOX_COLOR_DARKER);
    SetPenColor (hdc, RGBA2Pixel (hdc, GetRValue (color), GetGValue (color), GetBValue (color), GetAValue (color)));
    MoveTo (hdc, l, t);
    LineTo (hdc, l, b);	//left_line_dark
    MoveTo (hdc, r, t);
    LineTo (hdc, r, b);	//right_line_dark

	MoveTo (hdc, l, t);
	LineTo (hdc, r, t);	//top_line_dark

    color = info->we_rdr->calc_3dbox_color (mainc, LFRDR_3DBOX_COLOR_LIGHTER);
    SetPenColor (hdc, RGBA2Pixel (hdc, GetRValue (color), GetGValue (color), GetBValue (color), GetAValue (color)));
    MoveTo (hdc, l + 1, t);
    LineTo (hdc, l + 1, b);	//left_line_light
    MoveTo (hdc, r - 1, t);
    LineTo (hdc, r - 1, b);	//right_line_light

	MoveTo (hdc, l, t+1);
	LineTo (hdc, r, t+1);	//top_line_light

	mainc = GetWindowElementPixelEx(hWnd, hdc, WE_FGC_THREED_BODY);	//to avoid hdc use the color as ARGB
	SetBrushColor(hdc, mainc);	//base line color

	RECT rP;
	GetWindowRect(GetParent(hWnd), &rP);
	int x, y, w, h;
	x = 0;
	y = item->RcTitle.bottom + 2;
	w = item->RcTitle.left + 2;
	h = 4;
	FillBox(hdc, x, y, w, h);	//base_line_left
	x = item->RcTitle.right + 1;
	y = item->RcTitle.bottom + 2;
	w = RECTW(rP) - x;
	h = 4;
	FillBox(hdc, x, y, w, h);	//base_line_right
}

static void DrawCoolBox (HWND hWnd, HDC hdc, PCOOL_INDICATOR_CTRL pdata)
{
	db_msg("DrawCoolBox\n");
    COOL_INDICATOR_ITEMDATA* tmpdata;
    RECT rc;
    int l,t;
    WINDOWINFO *info = (WINDOWINFO*)GetWindowInfo (hWnd);
    DWORD color;
    DWORD mainc = GetWindowElementAttr (hWnd, WE_MAINC_THREED_BODY);
  
    GetClientRect (hWnd, &rc);

    if (pdata->BackBmp) {
        FillBoxWithBitmap (hdc, 0, 0, rc.right, rc.bottom, pdata->BackBmp);
    }

//    color = info->we_rdr->calc_3dbox_color (mainc, LFRDR_3DBOX_COLOR_DARKEST);
//    SetPenColor (hdc, RGBA2Pixel (hdc, GetRValue (color), GetGValue (color), GetBValue (color), GetAValue (color)));
//    MoveTo (hdc, 0, rc.bottom - 2);
//    color = info->we_rdr->calc_3dbox_color (mainc, LFRDR_3DBOX_COLOR_LIGHTEST);
//    SetPenColor (hdc, RGBA2Pixel (hdc, GetRValue (color), GetGValue (color), GetBValue (color), GetAValue (color)));
//    MoveTo (hdc, 0, 3);
//    LineTo (hdc, rc.right, 3);
//    MoveTo (hdc, 0, 4);
//    LineTo (hdc, rc.right, 4);
//
//    MoveTo (hdc, 0, rc.bottom - 1);
//    LineTo (hdc, rc.right, rc.bottom - 1);

    tmpdata = pdata->head;
    while (tmpdata) {
        l = tmpdata->RcTitle.left;
        t = tmpdata->RcTitle.top;
      
        switch (tmpdata->ItemType) {
        case TYPE_BARITEM:
			{
				WINDOWINFO *info = (WINDOWINFO*)GetWindowInfo (hWnd);
				RECT rcTmp;
				rcTmp.left = l + 2;
				rcTmp.top = 4;
				rcTmp.right = l + 4;
				rcTmp.bottom = rc.bottom - 4;

				info->we_rdr->draw_3dbox (hdc, &rcTmp, 
					GetWindowElementAttr (hWnd, WE_MAINC_THREED_BODY),
					LFRDR_BTN_STATUS_PRESSED);
			}
            break;

        case TYPE_BMPITEM:
			{
				FillBoxWithBitmap (hdc, l + 4, t + 4, pdata->ItemWidth, pdata->ItemHeight, tmpdata->Bmp);
			}
            break;

        case TYPE_TEXTITEM:
			{
				SIZE size;
				int h;
				WINDOWINFO *info;
				RECT rc;

				if (tmpdata->Caption == NULL || tmpdata->Caption [0] == '\0')
					break;

				GetTextExtent (hdc, tmpdata->Caption, -1, &size);
				h = (pdata->ItemHeight - size.cy) / 2;

				SetBkMode (hdc, BM_TRANSPARENT);
				if (tmpdata->Disable) {
					info = (WINDOWINFO*)GetWindowInfo (hWnd);
					rc.left = l + 2;
					rc.top = t + h + 2;
					rc.right = rc.left + size.cx;
					rc.bottom = rc.top + size.cy;
					info->we_rdr->disabled_text_out (hWnd, hdc,
						tmpdata->Caption, &rc, DT_SINGLELINE);
				}
				else {
					SetBkColor (hdc, GetWindowBkColor (hWnd));
					SetTextColor (hdc, PIXEL_black);
					TextOut (hdc, l+2, t + h + 2, tmpdata->Caption);
				}
			}
            break;

        default:
            break;
        }

        tmpdata = tmpdata->next;
    }

	db_msg("xxxxxxxxxxx\n");
    draw_hilight_box (hWnd, hdc, GetCurSel(pdata));

//if(enable_hint == TRUE)
	//	ShowCurItemHint (hWnd, pdata);
}

static void set_item_rect (HWND hwnd, PCOOL_INDICATOR_CTRL TbarData, COOL_INDICATOR_ITEMDATA* ptemp)
{
    SIZE size;
    int w;

    if (TbarData->tail == NULL) 
        ptemp->RcTitle.left = 2;
    else
        ptemp->RcTitle.left = TbarData->tail->RcTitle.right;

    switch (ptemp->ItemType) {
    case TYPE_BARITEM:
        w = ITEMBARWIDTH;
        break;

    case TYPE_BMPITEM:
        w = TbarData->ItemWidth + 4;
        break;

    case TYPE_TEXTITEM:
        if (strlen (ptemp->Caption)) {
            HDC hdc;
            hdc = GetClientDC (hwnd);
            GetTextExtent (hdc, ptemp->Caption, -1, &size);
            ReleaseDC (hdc);
        }
        w = size.cx + 4;
        break;

    default:
        w = 0;
        break;
    }

    ptemp->RcTitle.right = ptemp->RcTitle.left + w;
    ptemp->RcTitle.top = 2;
    ptemp->RcTitle.bottom = ptemp->RcTitle.top + TbarData->ItemHeight + 4;
  
    ptemp->hintx = ptemp->RcTitle.left;
    ptemp->hinty = ptemp->RcTitle.bottom + 8;
}

static int CoolIndicatorCtrlProc (HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
    HDC              hdc;
    PCOOL_INDICATOR_CTRL TbarData;
        
	//db_msg("message is %s, wParam: 0x%x, lParam: 0x%lx\n", Message2Str(message), wParam, lParam);
    switch (message) {
    case MSG_CREATE:
		{
			DWORD data; 
            DWORD dwStyle;

			//db_info("Cool Indicator ctrl proc\n");
            if ((TbarData = (COOL_INDICATOR_CTRL*) calloc (1, sizeof (COOL_INDICATOR_CTRL))) == NULL)
                return 1;

            TbarData->nCount = 0;
            TbarData->head = TbarData->tail = NULL;
            TbarData->BackBmp = NULL;
            TbarData->iSel = 0;
            TbarData->hToolTip = 0;

            //ExcludeWindowStyle (hWnd, WS_BORDER);

            dwStyle = GetWindowStyle (hWnd);
            if (dwStyle & CBS_BMP_CUSTOM) {
                data = GetWindowAdditionalData (hWnd);
                TbarData->ItemWidth = LOWORD (data);
                TbarData->ItemHeight = HIWORD (data);
				int screenW,screenH;
				getScreenInfo(&screenW,&screenH);
                TbarData->ItemHeight = 	screenH/8;
            }
            else {
                TbarData->ItemWidth = 16;
                TbarData->ItemHeight = 16;
            }

            SetWindowAdditionalData2 (hWnd, (DWORD)TbarData);
		}
		break;

	case MSG_DESTROY:
        { 
            COOL_INDICATOR_ITEMDATA* unloaddata, *tmp;
            TbarData = (PCOOL_INDICATOR_CTRL) GetWindowAdditionalData2(hWnd);
            if (TbarData->hToolTip != 0) {
                DestroyCdrToolTipWin(TbarData->hToolTip);
                TbarData->hToolTip = 0;
            }

            if (TbarData->BackBmp) {
                UnloadBitmap (TbarData->BackBmp);
                free (TbarData->BackBmp);
            }

            unloaddata = TbarData->head;
            while (unloaddata) {
                tmp = unloaddata->next;
                free (unloaddata);
                unloaddata = tmp;
            }

            free (TbarData);
        }
        break;
	
	case MSG_SHOWWINDOW:
		{
			if(wParam == SW_SHOWNORMAL) {
				enable_hint = TRUE;
			} else {
				enable_hint = FALSE;
			}
		}
		break;

	case MSG_SIZECHANGING:
        {
            const RECT* rcExpect = (const RECT*)wParam;
            RECT* rcResult = (RECT*)lParam;

            TbarData = (PCOOL_INDICATOR_CTRL) GetWindowAdditionalData2(hWnd);

            rcResult->left = rcExpect->left;
            rcResult->top = rcExpect->top;
            rcResult->right = rcExpect->right;
            rcResult->bottom = rcExpect->top + TbarData->ItemHeight + 14;
            return 0;
        }
		break;

	case MSG_SIZECHANGED:
		{
			RECT* rcWin = (RECT*)wParam;
			RECT* rcClient = (RECT*)lParam;
			*rcClient = *rcWin;
			return 1;
		}
		break;

	case MSG_FONTCHANGED:
		{
            TbarData = (PCOOL_INDICATOR_CTRL) GetWindowAdditionalData2(hWnd);

			PLOGFONT logfont = GetWindowFont(hWnd);
			//db_msg("MSG_FONTCHANGED, type :%s, familyt: %s, charset: %s\n", logfont->type, logfont->family, logfont->charset);
			if(TbarData != NULL && TbarData->hToolTip != 0) {
				SetWindowFont(TbarData->hToolTip, logfont );
			}
		}
		break;

    case MSG_NCPAINT:
        return 0;

	case MSG_PAINT:	//todo: why MSG_PAINT is received early than MSG_CREATE?
        {
            TbarData = (PCOOL_INDICATOR_CTRL) GetWindowAdditionalData2(hWnd);
			if (TbarData == NULL) {
				break;
			}
            hdc = BeginPaint (hWnd);

			PLOGFONT logfont = GetWindowFont(hWnd);

			SelectFont(hdc, logfont );
            DrawCoolBox (hWnd, hdc, TbarData);
            EndPaint (hWnd, hdc);
            return 0;
        }
		break;

	case CBM_ADDITEM:
        {
            COOL_INDICATOR_ITEMINFO* TbarInfo = NULL;
            COOL_INDICATOR_ITEMDATA* ptemp;
            RECT rc;

            TbarData = (PCOOL_INDICATOR_CTRL) GetWindowAdditionalData2 (hWnd);
            TbarInfo = (COOL_INDICATOR_ITEMINFO*) lParam;

            if (!(ptemp = (COOL_INDICATOR_ITEMDATA*)calloc (1, sizeof (COOL_INDICATOR_ITEMDATA)))) {
                return -1;
            }

            GetClientRect (hWnd, &rc);

			//db_msg("add item id is %d\n", TbarInfo->id);
            ptemp->id = TbarInfo->id;
            ptemp->Disable = 0;
            ptemp->ItemType = TbarInfo->ItemType;
			ptemp->sliderColor = TbarInfo->sliderColor;

           // if (TbarInfo->ItemHint) {
            //    strncpy (ptemp->Hint, TbarInfo->ItemHint, LEN_HINT);
            //    ptemp->Hint [LEN_HINT] = '\0';
            //}
           // else
                ptemp->Hint [0] = '\0';

            if (TbarInfo->Caption) {
                strncpy (ptemp->Caption, TbarInfo->Caption, LEN_TITLE);
                ptemp->Caption [LEN_TITLE] = '\0';
            }
            else
                ptemp->Caption [0] = '\0';
             
            ptemp->Bmp = TbarInfo->Bmp;

            set_item_rect (hWnd, TbarData, ptemp); 

            ptemp->next = NULL;
            if (TbarData->nCount == 0) {
                TbarData->head = TbarData->tail = ptemp;
            }
            else if (TbarData->nCount > 0) { 
                TbarData->tail->next = ptemp;
                TbarData->tail = ptemp;
            }
            TbarData->nCount++;

            InvalidateRect (hWnd, NULL, TRUE);
            return 0;
        }
		break;

	case CBM_CLEARITEM:
		{
            COOL_INDICATOR_ITEMINFO* TbarInfo = NULL;
            COOL_INDICATOR_ITEMDATA *ptemp1, *ptemp2;

            TbarData = (PCOOL_INDICATOR_CTRL) GetWindowAdditionalData2 (hWnd);
            TbarInfo = (COOL_INDICATOR_ITEMINFO*) lParam;

			ptemp1 = TbarData->head;
            while (ptemp1) {
                ptemp2 = ptemp1->next;
				ptemp1->next = NULL;
                free (ptemp1);
                ptemp1 = ptemp2;
            }
			TbarData->head = TbarData->tail = NULL;
            TbarData->nCount = 0;
			TbarData->iSel = 0;
		}
		break;

	case CBM_SWITCH2NEXT:
		{
			TbarData = (PCOOL_INDICATOR_CTRL) GetWindowAdditionalData2 (hWnd);

			TbarData->iSel++;
			if(TbarData->iSel >= TbarData->nCount) {
				TbarData->iSel = 0;
			}
			db_info("switch to next, iSel is %d\n", TbarData->iSel);
            InvalidateRect (hWnd, NULL, TRUE);
		}
		break;

	case CBM_ENABLE:
		{
            TbarData = (PCOOL_INDICATOR_CTRL)GetWindowAdditionalData2 (hWnd);
            if (enable_item (TbarData, wParam, lParam))
                return -1;

            InvalidateRect (hWnd, NULL, TRUE);
            return 0;
		}
		break;
	default:
		break;
	}

    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}


