
#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include "ResourceManager.h"
#include "cdr_widgets.h"
#undef LOG_TAG
#define LOG_TAG "TipLabel.cpp"
#include "debug.h"

#define IDC_TITLE	100
#define IDC_TEXT	101
#define ONE_SHOT_TIMER	200
#define TIMEOUT_TIMER	201

CTRLDATA ctrlData[2] = {
	{
		CTRL_STATIC,
		WS_VISIBLE | SS_CENTER | SS_VCENTER,
		0, 0, 0, 0,
		IDC_TITLE,
		"",
		0,
		WS_EX_NONE | WS_EX_TRANSPARENT,
		NULL, NULL
	},

	{
		CTRL_STATIC,
		WS_VISIBLE | SS_LEFT,
		0, 0, 0, 0,
		IDC_TEXT,
		"",
		0,
		WS_EX_NONE | WS_EX_TRANSPARENT,
		NULL, NULL
	}
};

static BOOL timerCallback(HWND hDlg, int id, DWORD data)
{
	tipLabelData_t* tipLabelData;

	tipLabelData = (tipLabelData_t*)GetWindowAdditionalData(hDlg);

	db_msg("timerCallback\n");
	if(id == ONE_SHOT_TIMER) {
		db_msg("one shot timer timerCallback\n");
		if(tipLabelData->callback != NULL)
			(*tipLabelData->callback)(hDlg, tipLabelData->callbackData);
		db_msg("one shot timer timerCallback\n");
	} else if(id == TIMEOUT_TIMER) {
		db_msg("timeout timer timerCallback\n");
		SendNotifyMessage(hDlg, MSG_CLOSE_TIP_LABEL, 0, 0);
	}

	return FALSE;
}


static int DialogProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
	switch(message) {
	case MSG_INITDIALOG:
		{
			tipLabelData_t* tipLabelData;

			tipLabelData = (tipLabelData_t*)lParam;
			if(!tipLabelData) {
				db_error("invalid tipLabelData\n");
				return -1;
			}
			SetWindowAdditionalData(hDlg, (DWORD)tipLabelData);

			if(tipLabelData->pLogFont) {
				SetWindowFont(hDlg, tipLabelData->pLogFont);
			}

			SetWindowBkColor(hDlg, tipLabelData->bgc_widget);
			SetWindowElementAttr(GetDlgItem(hDlg, IDC_TITLE), WE_FGC_WINDOW, PIXEL2DWORD(HDC_SCREEN, tipLabelData->fgc_widget) );
			SetWindowElementAttr(GetDlgItem(hDlg, IDC_TEXT), WE_FGC_WINDOW,  PIXEL2DWORD(HDC_SCREEN, tipLabelData->fgc_widget) );

			if(tipLabelData->callback != NULL) {
				db_msg("start timer\n");
				SetTimerEx(hDlg, ONE_SHOT_TIMER, 5, timerCallback);
			}
			if(tipLabelData->timeoutMs != TIME_INFINITE) {
				db_msg("set timeOut %d ms\n", tipLabelData->timeoutMs);
				SetTimerEx(hDlg, TIMEOUT_TIMER, tipLabelData->timeoutMs / 10, timerCallback);
			}
		}
		break;
	case MSG_PAINT:
		{
			RECT rect;
			HDC hdc; 
			tipLabelData_t* tipLabelData;

			hdc = BeginPaint(hDlg);

			tipLabelData = (tipLabelData_t*)GetWindowAdditionalData(hDlg);
			GetClientRect(GetDlgItem(hDlg, IDC_TITLE), &rect);
			SetPenColor(hdc, tipLabelData->linec_title );
			Line2(hdc, 0, RECTH(rect) + 2, RECTW(rect), RECTH(rect) + 2);

			if (tipLabelData->hasicon == true)
			{
				char *filepath=(char *)"/system/res/others/Warning.png";
				LoadBitmapFromFile(HDC_SCREEN, &tipLabelData->bitImage, filepath);
				FillBoxWithBitmap(hdc,ctrlData[1].x-45,ctrlData[1].y,32,28,&tipLabelData->bitImage);
			}
			
			EndPaint(hDlg, hdc);
		}
		break;
	case MSG_FONTCHANGED:
		{
			PLOGFONT pLogFont;		
			pLogFont = GetWindowFont(hDlg);
			if(pLogFont) {
				SetWindowFont(GetDlgItem(hDlg, IDC_TITLE), pLogFont);
				SetWindowFont(GetDlgItem(hDlg, IDC_TEXT), pLogFont);
			}
		}
		break;
	case MSG_KEYUP:
		{
			tipLabelData_t* tipLabelData;

			db_msg("key up, wParam is %d\n", wParam);
			tipLabelData = (tipLabelData_t*)GetWindowAdditionalData(hDlg);
			if(tipLabelData->disableKeyEvent == true) {
				db_msg("disable key event\n");
				break;
			}
			if(tipLabelData->endLabelKeyUp == true)
				EndDialog(hDlg, 0);
			BroadcastMessage(MSG_RECOVER_ADAS_SCREEN_FLAG,0,0);
		}
		break;
	case MSG_KEYDOWN:
		{
			tipLabelData_t* tipLabelData;

			db_msg("key down, wParam is %d\n", wParam);
			tipLabelData = (tipLabelData_t*)GetWindowAdditionalData(hDlg);
			if(tipLabelData->disableKeyEvent == true) {
				db_msg("disable key event\n");
				break;
			}
			if(tipLabelData->endLabelKeyUp == false){
				EndDialog(hDlg, 0);
				BroadcastMessage(MSG_RECOVER_ADAS_SCREEN_FLAG,0,0);
			}
		}
		break;
	case MSG_CLOSE_TIP_LABEL:
		tipLabelData_t* tipLabelData;

		tipLabelData = (tipLabelData_t*)GetWindowAdditionalData(hDlg);
		db_info("MSG_CLOSE_LOWPOWER_DIALOG\n");
		if(IsTimerInstalled(hDlg, ONE_SHOT_TIMER) == TRUE) {
			KillTimer(hDlg, ONE_SHOT_TIMER);
		}
		if(IsTimerInstalled(hDlg, TIMEOUT_TIMER) == TRUE) {
			KillTimer(hDlg, TIMEOUT_TIMER);
		}
		if (tipLabelData->bitImage.bmBits != NULL){
			UnloadBitmap(&tipLabelData->bitImage);
		}
		EndDialog(hDlg, 0);
		BroadcastMessage(MSG_RECOVER_ADAS_SCREEN_FLAG,0,0);
		break;
	}
	return DefaultDialogProc(hDlg, message, wParam, lParam);
}

int showTipLabel(HWND hParent, tipLabelData_t* info, int full_screen)
{
	DLGTEMPLATE dlg;
	CDR_RECT rect;

	if(info == NULL) {
		db_error("invalid info\n");
		return -1;
	}
	memset(&dlg, 0, sizeof(dlg));

	rect = info->rect;

	if (full_screen) {
		rect.x = 0;
		rect.y = 0;
		getScreenInfo(&rect.w,&rect.h);
		db_msg(" rect(%d %d %d %d)", rect.x, rect.y, rect.w, rect.h);
	}
	ctrlData[0].x = 0;
	ctrlData[0].y = 0;
	ctrlData[0].w = rect.w;
	ctrlData[0].h = info->titleHeight;
	ctrlData[0].caption = info->title;

	if (info->hasicon == true){

		ctrlData[1].x = 60;
		ctrlData[1].w = rect.w - 60;
		ctrlData[1].h = rect.h - 10 - ctrlData[0].h;
		ctrlData[1].y = ctrlData[0].h +(ctrlData[1].h/4);
		ctrlData[1].caption = info->text;
		ctrlData[1].dwStyle = WS_VISIBLE | SS_LEFT;
	}
	else{
		ctrlData[1].x = 10;
		ctrlData[1].w = rect.w - 20;
		ctrlData[1].h = rect.h - 10 - ctrlData[0].h;
		ctrlData[1].y = ctrlData[0].h +(ctrlData[1].h/4);
		ctrlData[1].caption = info->text;
		ctrlData[1].dwStyle = WS_VISIBLE | SS_CENTER;
	}
	ResourceManager	*rm = ResourceManager::getInstance();
	if(!strcmp(info->title,rm->getLabel(LANG_LABEL_IMPACT_NUM))){
			ctrlData[1].y = ctrlData[0].h +10;
#ifdef PLATFORM_0
			ctrlData[1].h = rect.h - ctrlData[1].y;
#endif
	}

	dlg.dwStyle = WS_VISIBLE;
	dlg.dwExStyle = WS_EX_NONE;
	dlg.x = rect.x;
	dlg.y = rect.y;
	dlg.w = rect.w;
	dlg.h =	rect.h;
	dlg.caption = "";
	dlg.hIcon = 0;
	dlg.hMenu = 0;
	dlg.controlnr = 2;
	dlg.controls = ctrlData;
	dlg.dwAddData = 0;

	return DialogBoxIndirectParam(&dlg, hParent, DialogProc, (LPARAM)info);
}
