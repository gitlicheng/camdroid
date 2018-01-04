
#include <string.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include "include/ctrlclass.h"

#include "cdr_widgets.h"
#include "cdr_message.h"
#include "misc.h"
#include "keyEvent.h"
#include "StorageManager.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "showMessageBox.cpp"
#include "debug.h"

#define IDC_TITLE		650
#define IDC_TEXT		651
#define TIMER_CONFIRM_CALLBACK	200

typedef struct {
	unsigned int dwStyle;
	unsigned int IDCSelected;
	unsigned char flag_end_key;
	gal_pixel linecTitle;
	gal_pixel linecItem;
	unsigned int buttonNRs;
	void (*confirmCallback)(HWND hDlg, void* data);
	void* confirmData;
	const char* msg4ConfirmCallback;
	int id;
}MBPrivData;

static gal_pixel gp_hilite_bgc;
static gal_pixel gp_hilite_fgc;
static gal_pixel gp_normal_bgc;
static gal_pixel gp_normal_fgc;

#define BUTTON_SELECTED	 0
#define BUTTON_UNSELECTED	 1

static void drawButton(HWND hDlg, HDC IDCButton, unsigned int flag)
{
	HWND hWnd;
	RECT rect;
	MBPrivData* privData;
	DWORD old_bkmode;
	gal_pixel old_color;
	HDC hdc;
	char* text;

	privData = (MBPrivData*)GetWindowAdditionalData(hDlg);
	hWnd = GetDlgItem(hDlg, IDCButton);
	GetClientRect(hWnd, &rect);

	hdc = GetDC(hWnd);

	if(flag == BUTTON_SELECTED) {
		old_color = SetBrushColor(hdc, gp_hilite_bgc);
		old_bkmode = SetBkMode(hdc, BM_TRANSPARENT);
		FillBox(hdc, rect.left, rect.top, RECTW(rect), RECTH(rect));
		SetTextColor(hdc, gp_hilite_fgc);
		text = (char*)GetWindowAdditionalData2(hWnd);
		DrawText(hdc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		SetBrushColor(hdc, old_color);
		SetBkMode(hdc, old_bkmode);
	} else if(flag == BUTTON_UNSELECTED) {
		old_color = SetBrushColor(hdc, gp_normal_bgc);
		old_bkmode = SetBkMode(hdc, BM_TRANSPARENT);
		FillBox(hdc, rect.left, rect.top, RECTW(rect), RECTH(rect));
		SetTextColor (hdc, gp_normal_fgc);
		text = (char*)GetWindowAdditionalData2(hWnd);
		DrawText(hdc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		SetBrushColor(hdc, old_color);
		SetBkMode(hdc, old_bkmode);
	}

	ReleaseDC(hdc);
}

static BOOL timerCallback(HWND hDlg, int id, DWORD data)
{
	MBPrivData* privData;
	privData = (MBPrivData *)GetWindowAdditionalData(hDlg);

	if(id == TIMER_CONFIRM_CALLBACK) {
		db_msg("confirmCallback\n");
		(*privData->confirmCallback)(hDlg , privData->confirmData);
		db_msg("confirmCallback\n");
	}

	return FALSE;
}

static int msgBoxProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam )
{
	MBPrivData* privData;
	switch(message) {
	case MSG_FONTCHANGED:
		{
			HWND hChild;	
			hChild = GetNextChild(hDlg, 0);
			while( hChild && (hChild != HWND_INVALID) )
			{
				SetWindowFont(hChild, GetWindowFont(hDlg));
				hChild = GetNextChild(hDlg, hChild);
			}
		}
		break;
	case MSG_INITDIALOG:
		{
			unsigned int dwStyle;
			MessageBox_t* info = (MessageBox_t *)lParam;
			if(info == NULL) {
				db_error("invalid info\n");	
				return -1;
			}
			dwStyle = info->dwStyle & MB_TYPEMASK;

			privData = (MBPrivData*)malloc(sizeof(MBPrivData));
			privData->dwStyle = dwStyle;
			if(privData->dwStyle & MB_OKCANCEL){
				privData->IDCSelected = IDC_BUTTON_START + 1;
				privData->buttonNRs = 2;
			} else {
				privData->IDCSelected = IDC_BUTTON_START;
				privData->buttonNRs = 1;
			}
			db_msg("buttonNRs is %d\n", privData->buttonNRs);

			privData->flag_end_key = info->flag_end_key;
			privData->linecTitle = info->linecTitle;
			privData->linecItem = info->linecItem;
			privData->confirmCallback = info->confirmCallback;
			privData->confirmData = info->confirmData;
			privData->msg4ConfirmCallback = info->msg4ConfirmCallback;
			SetWindowAdditionalData(hDlg, (DWORD)privData);

			SetWindowFont(hDlg, info->pLogFont);

			/******* the hilight is the global attibute *******/
			gp_hilite_bgc = GetWindowElementPixel(hDlg, WE_BGC_HIGHLIGHT_ITEM);
			gp_hilite_fgc = GetWindowElementPixel(hDlg, WE_FGC_HIGHLIGHT_ITEM);

			gp_normal_bgc = info->bgcWidget;
			gp_normal_fgc = info->fgcWidget;
			SetWindowBkColor(hDlg, gp_normal_bgc);

			SetWindowElementAttr(GetDlgItem(hDlg, IDC_TITLE), WE_FGC_WINDOW, Pixel2DWORD(HDC_SCREEN, gp_normal_fgc));
			SetWindowElementAttr(GetDlgItem(hDlg, IDC_TEXT), WE_FGC_WINDOW, Pixel2DWORD(HDC_SCREEN, gp_normal_fgc));
			SetWindowText(GetDlgItem(hDlg, IDC_TITLE), info->title);
			SetWindowText(GetDlgItem(hDlg, IDC_TEXT), info->text);

			for(unsigned int i = 0; i < privData->buttonNRs; i++) {
				SetWindowElementAttr( GetDlgItem(hDlg, IDC_BUTTON_START + i), WE_FGC_WINDOW, Pixel2DWORD(HDC_SCREEN, gp_normal_fgc));
				SetWindowAdditionalData2( GetDlgItem(hDlg, IDC_BUTTON_START + i), (DWORD)info->buttonStr[i]);
			}
		}
		break;
	case MSG_KEYUP:
		{
			privData = (MBPrivData *)GetWindowAdditionalData(hDlg);
			switch(wParam) {
			case CDR_KEY_OK:
				if(privData->flag_end_key == 1) {
					db_msg("hDlg is %x\n", hDlg);
					if( (privData->IDCSelected == IDC_BUTTON_START) && privData->confirmCallback) {
						if(privData->msg4ConfirmCallback) {
							SetWindowText(GetDlgItem(hDlg, IDC_TEXT), privData->msg4ConfirmCallback);
							SetTimerEx(hDlg, TIMER_CONFIRM_CALLBACK, 5, timerCallback);
							SetNullFocus(hDlg);
						} else {
							db_msg("confirmCallback\n");
							(*privData->confirmCallback)(hDlg , privData->confirmData);
							db_msg("confirmCallback\n");
						}
					} else {
						EndDialog(hDlg, privData->IDCSelected);
					}
				}
				break;
			case CDR_KEY_MENU:
			case CDR_KEY_MODE:
				if(privData->flag_end_key == 1)
					EndDialog(hDlg, IDC_BUTTON_CANCEL);
				break;
			case CDR_KEY_LEFT:
				privData->IDCSelected--;
				if(privData->IDCSelected < IDC_BUTTON_START) {
					privData->IDCSelected = IDC_BUTTON_START + privData->buttonNRs - 1;
				}
				InvalidateRect(hDlg, NULL, TRUE);	
				break;
			case CDR_KEY_RIGHT:
				privData->IDCSelected++;
				if(privData->IDCSelected > (IDC_BUTTON_START + privData->buttonNRs - 1) ) {
					privData->IDCSelected = IDC_BUTTON_START;
				}
				InvalidateRect(hDlg, NULL, TRUE);	
				break;
			default:
				break;
			}
		}
		break;
	case MSG_KEYDOWN:
		{
			RECT rect;
			GetClientRect(hDlg, &rect);
			privData = (MBPrivData *)GetWindowAdditionalData(hDlg);
			switch(wParam) {
			case CDR_KEY_OK:
				if(privData->flag_end_key == 0) {
					db_msg("hDlg is %x\n", hDlg);
					if( (privData->IDCSelected == IDC_BUTTON_START) && privData->confirmCallback ) {
						if(privData->msg4ConfirmCallback) {
							SetWindowText(GetDlgItem(hDlg, IDC_TEXT), privData->msg4ConfirmCallback);
							SetTimerEx(hDlg, TIMER_CONFIRM_CALLBACK, 5, timerCallback);
							SetNullFocus(hDlg);
						} else {
							db_msg("confirmCallback\n");
							(*privData->confirmCallback)(hDlg , privData->confirmData);
							db_msg("confirmCallback\n");
						}
					} else {
						EndDialog(hDlg, privData->IDCSelected);	
					}
				}
				break;
			case CDR_KEY_MENU:
			case CDR_KEY_MODE:
				if(privData->flag_end_key == 0) {
					EndDialog(hDlg, 0);	
				}
				break;

			default:
				break;
			}
		}
		break;
	case MSG_PAINT:
		{
			RECT rect, rect1;
			HDC hdc = BeginPaint(hDlg);
			privData = (MBPrivData *)GetWindowAdditionalData(hDlg);

			GetClientRect(GetDlgItem(hDlg, IDC_TITLE), &rect);
			GetClientRect(hDlg, &rect1);
			/**** draw the line below the title *******/
			SetPenColor(hdc, privData->linecTitle );
			Line2(hdc, 1, RECTH(rect) + 1, RECTW(rect) - 2, RECTH(rect) + 1);

			/**** draw the line above the button *******/
			SetPenColor(hdc, privData->linecItem );
			Line2(hdc, 1, RECTH(rect1) * 3 / 4 - 2, RECTW(rect1) - 2, RECTH(rect1) * 3 / 4 - 2);

			unsigned int i;
			for(i = 0; i < privData->buttonNRs; i++) {
				if(IDC_BUTTON_START + i == privData->IDCSelected) {
					drawButton(hDlg, IDC_BUTTON_START + i, BUTTON_SELECTED);
				} else {
					drawButton(hDlg, IDC_BUTTON_START + i, BUTTON_UNSELECTED);
				}
			}
			EndPaint(hDlg, hdc);
		}
		break;
	case MSG_DESTROY: {
		db_msg("msg destroy");
		privData = (MBPrivData *)GetWindowAdditionalData(hDlg);
		if(privData)
			free(privData);
		}
		break;
	case MSG_CLOSE_TIP_LABEL:
		EndDialog(hDlg, 0);
		break;
	default:
		break;
	}

	return DefaultDialogProc (hDlg, message, wParam, lParam);
}


/*********** 1 Static + 2 Button *************/
int showMessageBox (HWND hParentWnd, MessageBox_t *info)
{
	CDR_RECT rect;
	unsigned int dwStyle;
	int	 retval;

	DLGTEMPLATE MsgBoxDlg;
	CTRLDATA *CtrlData = NULL;
	unsigned int controlNrs;

	dwStyle = info->dwStyle & MB_TYPEMASK;
	rect = info->rect;

	controlNrs = 3;
	if(dwStyle & MB_OKCANCEL){
		controlNrs = 4;
		db_msg("button dwStyle is MB_OKCANCEL\n");
	}

	CtrlData = (PCTRLDATA)malloc(sizeof(CTRLDATA) * controlNrs);
	memset(CtrlData, 0, sizeof(CTRLDATA) * controlNrs);
	memset(&MsgBoxDlg, 0, sizeof(MsgBoxDlg));
	MsgBoxDlg.dwStyle = WS_NONE;
	MsgBoxDlg.dwExStyle = WS_EX_USEPARENTFONT;

	db_msg("rect.x is %d\n", rect.x);
	db_msg("rect.y is %d\n", rect.y);
	db_msg("rect.w is %d\n", rect.w);
	db_msg("rect.h is %d\n", rect.h);

	CtrlData[0].class_name = CTRL_STATIC;
	CtrlData[0].dwStyle = SS_CENTER | SS_VCENTER;
	CtrlData[0].dwExStyle = WS_EX_USEPARENTFONT | WS_EX_TRANSPARENT;
	CtrlData[0].id = IDC_TITLE;
	CtrlData[0].caption = "";
	if(dwStyle & MB_HAVE_TITLE) {
		CtrlData[0].dwStyle |= WS_VISIBLE;
		/* Title occupy 1/4 the height*/
		db_msg("messageBox have title\n");
		CtrlData[0].x = 0;
		CtrlData[0].y = 0;
		CtrlData[0].w = rect.w;
		CtrlData[0].h = rect.h / 4;
	} else {
		CtrlData[0].x = 0;
		CtrlData[0].y = 0;
		CtrlData[0].w = rect.w;
		CtrlData[0].h = rect.h / 8;
		db_msg("messageBox do not have title\n");
		db_msg("CtrlData[0].h is %d\n", CtrlData[0].h);
	}

	CtrlData[1].class_name = CTRL_STATIC;
	CtrlData[1].dwStyle = SS_LEFT | SS_CENTER | SS_VCENTER;
	CtrlData[1].dwExStyle = WS_EX_USEPARENTFONT | WS_EX_TRANSPARENT;
	CtrlData[1].id = IDC_TEXT;
	CtrlData[1].caption = "";
	if(dwStyle & MB_HAVE_TEXT) {
		CtrlData[1].dwStyle |= WS_VISIBLE;
		/******* text *********/
		CtrlData[1].x = rect.w / 16;
		CtrlData[1].y = CtrlData[0].h + CtrlData[0].y + 20;
		CtrlData[1].w = rect.w - CtrlData[1].x;
		CtrlData[1].h = rect.h / 2 - 5;
		db_msg("messageBox have text\n");
	} else {
		/******* text *********/
		CtrlData[1].x = rect.w / 16;
		CtrlData[1].y = CtrlData[0].h + CtrlData[0].y + 20;
		CtrlData[1].w = rect.w - CtrlData[1].x;
		CtrlData[1].h = rect.h / 8;
		db_msg("messageBox do not have text\n");
		db_msg("CtrlData[1].h is %d\n", CtrlData[1].h);
	}
	/* -------------------------------------- */

	db_msg("CtrlData[2] is Button OK\n");
	CtrlData[2].class_name = CTRL_STATIC;
	CtrlData[2].dwStyle = SS_CENTER | SS_VCENTER | WS_VISIBLE;
	CtrlData[2].dwExStyle = WS_EX_USEPARENTFONT | WS_EX_TRANSPARENT;
	CtrlData[2].id = IDC_BUTTON_START;
	CtrlData[2].caption = "";

	if(dwStyle & MB_OKCANCEL) {
		db_msg("CtrlData[3] is Button Cancel\n");
		CtrlData[3].class_name = CTRL_STATIC;
		CtrlData[3].dwStyle = SS_CENTER | SS_VCENTER | WS_VISIBLE;
		CtrlData[3].dwExStyle = WS_EX_USEPARENTFONT | WS_EX_TRANSPARENT;
		CtrlData[3].id = IDC_BUTTON_START + 1;
		CtrlData[3].caption = "";
	}

	if( (dwStyle & MB_HAVE_TEXT) || (dwStyle & MB_HAVE_TITLE) ) {
		if(dwStyle & MB_OKCANCEL) {
			/********* button ok **********/
			db_msg("MB_OKCANCEL rect\n");
			CtrlData[2].x = 1;
			CtrlData[2].y = rect.h * 3 / 4 + 1;
			CtrlData[2].w = rect.w / 2 - 2;
			CtrlData[2].h = rect.h / 4 - 2;

			/********* button CANCEL **********/
			CtrlData[3].x = rect.w / 2 + 1;
			CtrlData[3].y = rect.h * 3 / 4 + 1;
			CtrlData[3].w = rect.w / 2 - 2;
			CtrlData[3].h = rect.h / 4 - 2;
		} else {
			/********* button ok **********/
			db_msg("MB_OK rect\n");
			CtrlData[2].x = rect.w / 4;
			CtrlData[2].y = rect.h * 3 / 4 + 1;
			CtrlData[2].w = rect.w / 2;
			CtrlData[2].h = rect.h / 4 - 2;
		}
	} else {
		if(dwStyle & MB_OKCANCEL) {
			db_msg("MB_OKCANCEL rect\n");
			CtrlData[2].x = 1;
			CtrlData[2].y = CtrlData[1].y + CtrlData[1].h + 2;
			CtrlData[2].w = rect.w / 2 - 2;
			CtrlData[2].h = rect.h - (CtrlData[2].y + 8);
			db_msg("button ok: y is %d, h is %d\n", CtrlData[2].y, CtrlData[2].h);

			/********* button CANCEL **********/
			CtrlData[3].x = rect.w / 2 + 1;
			CtrlData[3].y = CtrlData[1].y + CtrlData[1].h + 2;
			CtrlData[3].w = rect.w / 2 - 2;
			CtrlData[3].h = rect.h - (CtrlData[3].y + 8);
		} else {
			db_msg("MB_OK rect\n");
			CtrlData[2].x = rect.w / 4;
			CtrlData[2].y = CtrlData[1].y + CtrlData[1].h + 5;
			CtrlData[2].w = rect.w / 2;
			CtrlData[2].h = rect.h - CtrlData[2].y - 5;
		}
	}

	MsgBoxDlg.x = rect.x;
	MsgBoxDlg.y = rect.y;
	MsgBoxDlg.w = rect.w;
	MsgBoxDlg.h = rect.h;
	MsgBoxDlg.caption = "";
	MsgBoxDlg.controlnr = controlNrs;
	MsgBoxDlg.controls = CtrlData;

	retval = DialogBoxIndirectParam(&MsgBoxDlg, hParentWnd, msgBoxProc, (LPARAM)info);
	db_msg("retval is %d\n", retval);
	free(CtrlData);
	return retval;
}
