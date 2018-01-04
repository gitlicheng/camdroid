
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include "cdr_widgets.h"
#include "cdr_message.h"
#include "cdr_res.h"
#include "keyEvent.h"
#include "include/ctrlclass.h"

#undef LOG_TAG
#define LOG_TAG "PopUpMenu.cpp"
#include "debug.h"
#include "ResourceManager.h"

#define POPMENU_USB_OFFSET_X  5
#define POPMENU_USB_OFFSET_Y  10

typedef struct tagBUTTONDATA
{
	DWORD status;           /* button flags */
	DWORD data;             /* bitmap or icon of butotn. */
} BUTTONDATA;
typedef BUTTONDATA* PBUTTONDATA;
 
#define VBORDER 10
#define HBORDER 5
#define BTN_WIDTH_BMP       20 
#define BTN_HEIGHT_BMP      20 
#define BTN_INTER_BMPTEXT   2

#define BUTTON_STATUS(pctrl)   (((PBUTTONDATA)((pctrl)->dwAddData2))->status)

#define BUTTON_GET_POSE(pctrl)  \
	(BUTTON_STATUS(pctrl) & BST_POSE_MASK)

#define BUTTON_SET_POSE(pctrl, status) \
	do { \
		BUTTON_STATUS(pctrl) &= ~BST_POSE_MASK; \
		BUTTON_STATUS(pctrl) |= status; \
	}while(0)

#define BUTTON_TYPE(pctrl)     ((pctrl)->dwStyle & BS_TYPEMASK)

#define BUTTON_IS_PUSHBTN(pctrl) \
	(BUTTON_TYPE(pctrl) == BS_PUSHBUTTON || \
	 BUTTON_TYPE(pctrl) == BS_DEFPUSHBUTTON)

#define BUTTON_IS_AUTO(pctrl) \
	(BUTTON_TYPE(pctrl) == BS_AUTO3STATE || \
	 BUTTON_TYPE(pctrl) == BS_AUTOCHECKBOX || \
	 BUTTON_TYPE(pctrl) == BS_AUTORADIOBUTTON)


/*btnGetRects:
 *      get the client rect(draw body of button), the content rect
 *      (draw contern), and bitmap rect(draw a little icon)
 */
static void btnGetRects (PCONTROL pctrl, RECT* prcClient, RECT* prcContent, RECT* prcBitmap)
{
	GetClientRect ((HWND)pctrl, prcClient);

	if (BUTTON_IS_PUSHBTN(pctrl) || (pctrl->dwStyle & BS_PUSHLIKE))
	{
		SetRect (prcContent, (prcClient->left   + BTN_WIDTH_BORDER),
				(prcClient->top    + BTN_WIDTH_BORDER),
				(prcClient->right  - BTN_WIDTH_BORDER),
				(prcClient->bottom - BTN_WIDTH_BORDER));

		if (BUTTON_GET_POSE(pctrl) == BST_PUSHED)
		{
			prcContent->left ++;
			prcContent->top ++;
			prcContent->right ++;
			prcContent->bottom++;
		}

		SetRectEmpty (prcBitmap);
		return;
	}

	if (pctrl->dwStyle & BS_LEFTTEXT) {
		SetRect (prcContent, prcClient->left + 1,
				prcClient->top + 1,
				prcClient->right - BTN_WIDTH_BMP - BTN_INTER_BMPTEXT,
				prcClient->bottom - 1);
		SetRect (prcBitmap, prcClient->right - BTN_WIDTH_BMP,
				prcClient->top + 1,
				prcClient->right - 1,
				prcClient->bottom - 1);
	}
	else {
		SetRect (prcContent, prcClient->left + BTN_WIDTH_BMP + BTN_INTER_BMPTEXT,
				prcClient->top + 1,
				prcClient->right - 1,
				prcClient->bottom - 1);
		SetRect (prcBitmap, prcClient->left + 1,
				prcClient->top + 1,
				prcClient->left + BTN_WIDTH_BMP,
				prcClient->bottom - 1);
	}

}


static void change_status_from_pose (PCONTROL pctrl, int new_pose, BOOL noti_pose_changing, BOOL change_valid, DWORD context)
{
	static int nt_code[3][3] = {
		/*normal->nochange, hilite, pressed*/
		{          0,  BN_HILITE, BN_PUSHED},
		/*hilite->normal, nochange, pressed*/
		{BN_UNHILITE,          0, BN_PUSHED},
		/*pressed->normal, hilite, nochange*/
		{BN_UNPUSHED, BN_UNPUSHED,        0},
	};

	DWORD dwStyle = pctrl->dwStyle;
	int old_pose = BUTTON_GET_POSE(pctrl);
 
	RECT rcClient;
	RECT rcContent;
	RECT rcBitmap;

	const WINDOW_ELEMENT_RENDERER* win_rdr;
	win_rdr = GetWindowInfo((HWND)pctrl)->we_rdr;

	/*make sure it can be changed, and avoid changing more than once*/
	MG_CHECK (old_pose!= BST_DISABLE && old_pose != new_pose);

	/*set new pose*/
	BUTTON_SET_POSE(pctrl, new_pose);
	if ((dwStyle & BS_NOTIFY) && noti_pose_changing)
	{
		if (new_pose != BST_DISABLE)
			NotifyParent ((HWND)pctrl, pctrl->id, 
					nt_code[old_pose][new_pose]);
		/*disable the button*/
		else
		{
			/*before to disable, firstly to normal*/
			switch (old_pose)
			{
			case BST_PUSHED:
				NotifyParent ((HWND)pctrl, pctrl->id, BN_UNPUSHED);
			case BST_HILITE:
				NotifyParent ((HWND)pctrl, pctrl->id, BN_UNHILITE);
			case BST_NORMAL:
				NotifyParent ((HWND)pctrl, pctrl->id, BN_DISABLE);
				break;
			default:
				assert (0);
				break;
			}
		}
	}

	/*complete a click if the click if valid
	 * (mouse lbutton up in button )*/
	if (old_pose == BST_PUSHED && new_pose != BST_DISABLE
			&& change_valid)
	{
		NotifyParentEx((HWND)pctrl, pctrl->id, BN_CLICKED, context);
	}

	/* DK: When the radio or check button state changes do not erase the background, 
	 * and the prospects for direct rendering */
	btnGetRects (pctrl, &rcClient, &rcContent, &rcBitmap);
	if (BUTTON_IS_PUSHBTN(pctrl) || (pctrl->dwStyle & BS_PUSHLIKE)) 
		InvalidateRect((HWND)pctrl, NULL, TRUE);
	else 
		InvalidateRect((HWND)pctrl, &rcBitmap, FALSE);
}


static int btnGetTextFmt (int dwStyle)
{
	UINT dt_fmt;
	if (dwStyle & BS_MULTLINE)
		dt_fmt = DT_WORDBREAK;
	else
		dt_fmt = DT_SINGLELINE;

	if ((dwStyle & BS_TYPEMASK) == BS_PUSHBUTTON
			|| (dwStyle & BS_TYPEMASK) == BS_DEFPUSHBUTTON
			|| (dwStyle & BS_PUSHLIKE))
		dt_fmt |= DT_CENTER | DT_VCENTER;
	else {
		if ((dwStyle & BS_ALIGNMASK) == BS_RIGHT)
			dt_fmt = DT_WORDBREAK | DT_RIGHT;
		else if ((dwStyle & BS_ALIGNMASK) == BS_CENTER)
			dt_fmt = DT_WORDBREAK | DT_CENTER;
		else dt_fmt = DT_WORDBREAK | DT_LEFT;

		if ((dwStyle & BS_ALIGNMASK) == BS_TOP)
			dt_fmt |= DT_SINGLELINE | DT_TOP;
		else if ((dwStyle & BS_ALIGNMASK) == BS_BOTTOM)
			dt_fmt |= DT_SINGLELINE | DT_BOTTOM;
		else dt_fmt |= DT_SINGLELINE | DT_VCENTER;
	}

	return dt_fmt;
}

static void paint_content_focus(HDC hdc, PCONTROL pctrl, RECT* prc_cont)
{
	DWORD dt_fmt = 0;
 	BOOL is_get_fmt = FALSE;
	gal_pixel old_pixel;
	gal_pixel text_pixel;
	gal_pixel frame_pixel;
	RECT focus_rc;
	int status, type;
	const WINDOW_ELEMENT_RENDERER* win_rdr;

	status = BUTTON_STATUS (pctrl);
	type = BUTTON_TYPE (pctrl);
	win_rdr = GetWindowInfo((HWND)pctrl)->we_rdr;

	/*draw button content*/
	switch (pctrl->dwStyle & BS_CONTENTMASK)
	{
	case BS_BITMAP:
		break;
	case BS_ICON:
		break;
	default:
		if(BUTTON_STATUS(pctrl) & BST_FOCUS) {
			text_pixel = DWORD2PIXEL(HDC_SCREEN, 0xFF000000);	//UI@Popmenu_Focus_char_color
		} else {
			text_pixel = DWORD2PIXEL(HDC_SCREEN, 0xFF292929);   //UI@Popmenu_Unfocus_char_color
		}

		old_pixel = SetTextColor(hdc, text_pixel);
		dt_fmt = btnGetTextFmt(pctrl->dwStyle);
		is_get_fmt = TRUE;
		SetBkMode (hdc, BM_TRANSPARENT);
		DrawText (hdc, pctrl->spCaption, -1, prc_cont, dt_fmt);
		SetTextColor(hdc, old_pixel);

		/*disable draw text*/
		if ((BUTTON_GET_POSE(pctrl) == BST_DISABLE) 
				| (pctrl->dwStyle & WS_DISABLED))
			win_rdr->disabled_text_out ((HWND)pctrl, hdc, pctrl->spCaption, 
					prc_cont, dt_fmt);
	}

	/*draw focus frame*/
	if (BUTTON_STATUS(pctrl) & BST_FOCUS)
	{
		focus_rc = *prc_cont;

		if (!BUTTON_IS_PUSHBTN(pctrl) && 
				!(pctrl->dwStyle & BS_PUSHLIKE) && 
				is_get_fmt)
		{
			dt_fmt |= DT_CALCRECT;
			DrawText (hdc, pctrl->spCaption, -1, &focus_rc, dt_fmt);
		}
		frame_pixel = DWORD2PIXEL(HDC_SCREEN, 0xFF050505);
//		win_rdr->draw_focus_frame(hdc, &focus_rc, frame_pixel);	//UI@Button_FocusFrame
	}
}

static void paint_push_btn (HDC hdc, PCONTROL pctrl)
{
	const WINDOW_ELEMENT_RENDERER* win_rdr;
	DWORD main_color;

	RECT rcClient;
	RECT rcContent;
	RECT rcBitmap;
	win_rdr = GetWindowInfo((HWND)pctrl)->we_rdr;
	main_color = GetWindowElementAttr((HWND)pctrl, WE_MAINC_THREED_BODY);	/* 按钮边框的颜色  */

	btnGetRects (pctrl, &rcClient, &rcContent, &rcBitmap);
	win_rdr->draw_push_button((HWND)pctrl, hdc, &rcClient, main_color, 0xFFFFFFFF, BUTTON_STATUS(pctrl));

	paint_content_focus(hdc, pctrl, &rcContent);
}

static WNDPROC button_default_callback;
static int button_callback(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
	PopUpMenuInfo_t *info;
	PCONTROL    pctrl;

	switch(message)	{
	case MSG_PAINT:
		{
			HDC hdc = BeginPaint (hWnd);

			pctrl   = gui_Control(hWnd);
			SelectFont (hdc, GetWindowFont(hWnd));

			if (BUTTON_IS_PUSHBTN(pctrl) || (pctrl->dwStyle & BS_PUSHLIKE)) {
				paint_push_btn(hdc, pctrl);
			}

			EndPaint (hWnd, hdc);
			return 0;
		}
		break;
	case MSG_SETFOCUS:
		{
			info = (PopUpMenuInfo_t *)GetWindowAdditionalData(GetParent(hWnd));
			SetWindowBkColor(hWnd, info->bgc_item_focus);	//@UI_Focus_backcolor
		}
		break;
	case MSG_KILLFOCUS:
		{
			info = (PopUpMenuInfo_t *)GetWindowAdditionalData(GetParent(hWnd));
			SetWindowBkColor(hWnd, info->bgc_item_normal);	//@UI_unFocus_backcolor
		}
		break;
	case MSG_KEYDOWN:
	case MSG_KEYUP:
		return 0;
	default:
		break;
	}

	return (*button_default_callback)(hWnd, message, wParam, lParam);
}

/*
 * color:
 * [playback_preview_menu]
 *	bgc_widget=0xC8292929
 *	bgc_item_focus=0xFF7D29E2
 *	bgc_item_normal=0xFFFFCE42
 *	mainc_3dbox=0xFF881d65
 *	
 *	Dialog Backgroud:	use the bgc_widget
 *	button focused:		use the bgc_item_focus
 *	button normal:		use the bgc_item_normal
 *	buttion efftec:		use the mainc_3dbox
 * */
static int DialogProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
	switch(message) {
	case MSG_INITDIALOG:
		{
			unsigned int count;
			PopUpMenuInfo_t *info = (PopUpMenuInfo_t *)lParam;
			if(info == NULL) {
				db_error("info is NULL\n");	
				EndDialog(hDlg, -1);
			}
			db_msg("hDlg is 0x%x\n", hDlg);
			SetWindowAdditionalData(hDlg, (DWORD)lParam);
			SetWindowFont(hDlg, info->pLogFont);

			SetWindowBkColor(hDlg, info->bgc_widget);	//UI@PopMenu_background_color

			SetFocusChild(GetDlgItem(hDlg, info->id));
			for(count = 0; count < info->item_nrs; count++) {
				if(info->callback[count]) {
					SetNotificationCallback(GetDlgItem(hDlg, info->id + count), info->callback[count]);
				}
				button_default_callback = SetWindowCallbackProc(GetDlgItem(hDlg, info->id + count), button_callback);

				//SetWindowElementAttr(GetDlgItem(hDlg, info->id + count), WE_MAINC_THREED_BODY, Pixel2DWORD(HDC_SCREEN, info->mainc_3dbox));

				if(count == 0)
					SetWindowBkColor(GetDlgItem(hDlg, info->id + count), info->bgc_item_focus);	//@UI_Focus_backcolor_init
				else 
					SetWindowBkColor(GetDlgItem(hDlg, info->id + count), info->bgc_item_normal); //@UI_Unfocus_backcolor_init
			}
		}
		break;
	case MSG_FONTCHANGED:
		{
			PLOGFONT pLogFont;	
			HWND hChild;

			pLogFont = GetWindowFont(hDlg);
			if(!pLogFont) {
				db_error("invalid logFont\n");
				break;
			}
			hChild = GetNextChild(hDlg, 0);
			while( hChild && (hChild != HWND_INVALID) )
			{
				SetWindowFont(hChild, pLogFont);
				hChild = GetNextChild(hDlg, hChild);
			}
		}
		break;
	case MSG_KEYDOWN:
		{
			PCONTROL    pCtrl;
			int id;
			PopUpMenuInfo_t *info;

			info = (PopUpMenuInfo_t *)GetWindowAdditionalData(hDlg);
			id = GetDlgCtrlID(GetFocusChild(hDlg));
			if(id == -1)
				break;		
			pCtrl   = gui_Control(GetDlgItem(hDlg, id));

			if(wParam == CDR_KEY_OK) {
				change_status_from_pose(pCtrl, BST_PUSHED, TRUE, TRUE, info->context);
				return 0;
			} 
		}
		break;
	case MSG_KEYUP:
		{
			PCONTROL pCtrl;
			int id;
			PopUpMenuInfo_t *info;

			info = (PopUpMenuInfo_t *)GetWindowAdditionalData(hDlg);
			id = GetDlgCtrlID(GetFocusChild(hDlg));
			if(id == -1)
				break;
			pCtrl   = gui_Control(GetDlgItem(hDlg, id));

			if(wParam == CDR_KEY_OK) {
				change_status_from_pose(pCtrl, BST_NORMAL, TRUE, TRUE, info->context);
				return 0;
			} else if(wParam == CDR_KEY_LEFT) {
				HWND hNewFocus, hCurFocus;
				hCurFocus = GetFocusChild (hDlg);
				hNewFocus = GetNextDlgTabItem (hDlg, hCurFocus, TRUE);
				if (hNewFocus != hCurFocus) {
					SetNullFocus (hCurFocus);
					SetFocus (hNewFocus);
				}
			} else if(wParam == CDR_KEY_RIGHT) {
				HWND hNewFocus, hCurFocus;
				hCurFocus = GetFocusChild (hDlg);
				hNewFocus = GetNextDlgTabItem (hDlg, hCurFocus, FALSE);
				if (hNewFocus != hCurFocus) {
					SetNullFocus (hCurFocus);
					SetFocus (hNewFocus);
				}
			}
		}
		break;
	case MSG_CLOSE_TIP_LABEL: {
			EndDialog(hDlg, IDCANCEL);
		}
		break;
	case MSG_UPDATE_BG: {
			PopUpMenuInfo_t *info = (PopUpMenuInfo_t *)GetWindowAdditionalData(hDlg);
			RECT rect;
			PBITMAP pBmp = &info->pc_usb_bmp;
			HDC hdc = GetDC(hDlg);
			LoadBitmapFromFile(HDC_SCREEN, pBmp, (char*)wParam);
			GetWindowRect(hDlg, &rect);
			FillBoxWithBitmap(hdc,0,0,RECTW(rect),RECTH(rect),pBmp);
		}
		break;
	case MSG_CLOSE_CONNECT2PC_DIALOG:
		{
			PopUpMenuInfo_t *info = (PopUpMenuInfo_t *)GetWindowAdditionalData(hDlg);
			PBITMAP pBmp = &info->pc_usb_bmp;
			if (pBmp->bmBits) {
				UnloadBitmap(pBmp);
			}
			db_msg("recieved MSG_CLOSE_CONNECT2PC_DIALOG\n");	
			EndDialog(hDlg, 0);
		}
		break;
	default:
		break;
	}

	return DefaultDialogProc(hDlg, message, wParam, lParam);	
}


/*
 * if do nothing return 0;
 * if press buttion1, then return the buttion1's id after do some works
 * if error accoured, return -1
 * */
int PopUpMenu(HWND hWnd, PopUpMenuInfo_t *info)
{

	DLGTEMPLATE dlg;
	PCTRLDATA ctrldata;
	unsigned int count;
	int tmp;
	int retval;
	unsigned int dwStyle;
    ResourceManager* rm;
	BITMAP bmpIcon[3]={0};
	if(info->item_nrs > POPMENU_MAX_ITEMS) {
		db_error("the max items is %d\n", POPMENU_MAX_ITEMS);	
		return -1;
	}
	ctrldata = (PCTRLDATA)malloc(info->item_nrs *2* sizeof(CTRLDATA));
	memset(ctrldata, 0, info->item_nrs *2* sizeof(CTRLDATA));

	dwStyle = info->style & POPMENUS_MASK;
	if(dwStyle & POPMENUS_V) {
		tmp = info->item_height * info->item_nrs;
		tmp += (2 + info->item_nrs - 1) * VBORDER;
		if(info->rect.h < tmp) {
			db_error("the height is not enough to place %d buttons\n", info->item_nrs);	
			free(ctrldata);
			return -1;
		}
		tmp = info->item_width + HBORDER * 2;
		if(info->rect.w < tmp) {
			db_error("the width is not enough to place the button\n");
			free(ctrldata);
			return -1;
		}
	} else if(dwStyle & POPMENUS_H) {
		tmp = info->item_height + VBORDER * 2;	
		if(info->rect.h < tmp) {
			db_error("the height is not enough to place %d buttons\n", info->item_nrs);	
			free(ctrldata);
			return -1;
		}
		tmp = info->item_width * info->item_nrs;
		tmp += (2 + info->item_nrs - 1) * HBORDER;
		if(info->rect.w < tmp) {
			db_error("the width is not enough to place the button\n");
			free(ctrldata);
			return -1;
		}
	} else {
		db_error("invalid window style 0x%x\n", info->style);	
	}

	for(count = 0; count < info->item_nrs; count++) {
		ctrldata[count].class_name = CTRL_BUTTON;
		if(count != 0) {
			ctrldata[count].dwStyle = WS_VISIBLE | WS_TABSTOP;	//UI@Popmenu_push_style :BS_PUSHBUTTON
		}
		ctrldata[count].id = info->id + count;
		ctrldata[count].caption = info->name[count];
		if(dwStyle & POPMENUS_V) {
			if(dwStyle & POPMENUS_VCENTER) {
				ctrldata[count].x = info->rect.w / 2 -info->item_width / 2;
				tmp = (count * info->item_height) + (count + 1) * VBORDER;
				tmp += (info->rect.h - (info->item_nrs * info->item_height + (info->item_nrs + 1) * VBORDER)) / 2;
				ctrldata[count].y = tmp;
			} else {
				ctrldata[count].x = info->rect.w / 2 - info->item_width/2;
				ctrldata[count].y = (count * info->item_height) + (count + 1) * VBORDER;
			}
		} else {
			ctrldata[count].x =	(count * info->item_width) + (count + 1) * HBORDER;
			ctrldata[count].y = info->rect.h / 2 - info->item_height / 2;
		}
		ctrldata[count].w = info->item_width;
		ctrldata[count].h = info->item_height;
	}
   #if 1
			rm = ResourceManager::getInstance();
			for (count = 0; count < info->item_nrs; count++)
			{
				retval = rm->getResBmp(ID_CONNECT2PC, (BmpType)count, bmpIcon[count]);
				if(retval < 0) {
					db_error("get current playback icon bmp failed\n");
					return -1;
				}
				//_error("count=%d, bmpIcon.bmWidth=%d bmpIcon.bmHeight=%d\n", count, bmpIcon[].bmWidth, bmpIcon.bmHeight);
				//_error("bmpIcon =%x\n", &bmpIcon);
				ctrldata[info->item_nrs+count].class_name = CTRL_STATIC;
				ctrldata[info->item_nrs+count].dwStyle = WS_VISIBLE | WS_CHILD | SS_BITMAP | SS_CENTERIMAGE;
				ctrldata[info->item_nrs+count].dwExStyle = WS_EX_TRANSPARENT;
	
				if(dwStyle & POPMENUS_V) {
				if(dwStyle & POPMENUS_VCENTER) {
				ctrldata[info->item_nrs+count].x = info->rect.w / 2 -info->item_width / 2 +10;
				tmp = (count * info->item_height) + (count + 1) * VBORDER;
				tmp += (info->rect.h - (info->item_nrs * info->item_height + (info->item_nrs + 1) * VBORDER)) / 2;
				ctrldata[info->item_nrs+count].y = tmp +(info->item_height - bmpIcon[count].bmHeight)/2;
			} else {
				ctrldata[info->item_nrs+count].x = info->rect.w / 2 - info->item_width/2 +10;
				ctrldata[info->item_nrs+count].y = (count * info->item_height) + (count + 1) * VBORDER+(info->item_height - bmpIcon[count].bmHeight)/2;
			}
		} else {
			ctrldata[info->item_nrs+count].x =(count * info->item_width) + (count + 1) * HBORDER + 10;
			ctrldata[info->item_nrs+count].y = info->rect.h / 2 - info->item_height / 2 + (info->item_height - bmpIcon[count].bmHeight)/2;
			}
			//	ctrldata[info->item_nrs+count].x = 32;
			//	ctrldata[info->item_nrs+count].y = count*(bmpIcon[count].bmHeight+20)+28;
				ctrldata[info->item_nrs+count].w = bmpIcon[count].bmWidth;
				ctrldata[info->item_nrs+count].h = bmpIcon[count].bmHeight;
				ctrldata[info->item_nrs+count].id =ID_CONNECT2PC+(count+1)*40 ;
				ctrldata[info->item_nrs+count].caption ="" ;
				ctrldata[info->item_nrs+count].dwAddData = (DWORD)&bmpIcon[count] ;
		
				/*retWnd[count] = CreateWindowEx(CTRL_STATIC, "",
						WS_VISIBLE | WS_CHILD | SS_BITMAP | SS_CENTERIMAGE,
						WS_EX_NONE,
						ID_CONNECT2PC+count*40,
						10, 0+count*bmpIcon.bmHeight+10, bmpIcon.bmWidth, bmpIcon.bmHeight,
						hWnd, (DWORD)&bmpIcon);
		
				if(retWnd[count] == HWND_INVALID) {
					db_error(" icon label failed\n");
					return -1;
				}*/
			}
#endif
	ctrldata[0].dwStyle = WS_VISIBLE | WS_TABSTOP | WS_GROUP;	//UI@Popmenu_push_style :BS_PUSHBUTTON

	dlg.dwStyle = NULL;
	dlg.dwExStyle = WS_EX_NONE;
	dlg.x = info->rect.x;
	dlg.y = info->rect.y;
	dlg.w = info->rect.w;
	dlg.h = info->rect.h;
	dlg.caption = "";
	dlg.hIcon = 0;
	dlg.hMenu = 0;
	dlg.controlnr = info->item_nrs*2;
	dlg.controls = ctrldata;
	dlg.dwAddData = 0;
	db_msg("xxxxxxxxxxxxxx\n");
	retval = DialogBoxIndirectParam(&dlg, hWnd, DialogProc, (LPARAM)info);
	free(ctrldata);
	return retval;
}
