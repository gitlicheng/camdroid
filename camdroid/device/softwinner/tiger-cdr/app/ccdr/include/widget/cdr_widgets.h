/*
 * 自定义的一些控件
 * */

#ifndef __CDR_WIDGETS_H__
#define __CDR_WIDGETS_H__

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>
#include <minigui/ctrl/listbox.h>

#include <utils/Vector.h>
#include <utils/String8.h>

#include "cdr.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 控件类在系统中的名称 */
#define CTRL_CDRMenuList		"CDRMenuList"
#define CTRL_CDRPROGRESSBAR		"CDRPROGRESSBAR"


/* ********** define static draw style ********** */
#define SS_VCENTER		0x00000040


/* this flags is defines for MenuList, MenuList is inherit form LISTBOX
 * LBS_USEICON + IMGFLAG_BITMAP to dislay image at the end of the line
 *		the LISTBOXITEMINFO->hIcon store the BITMAP address
 * LBS_USETEXT + CMFLAG_TEXT to display text at the end of the line
 *		the LISTBOXITEMINFO->hIcon store the Text string address
 * Remind: can not use the IMGFLAG_BITMAP and CMFLAG_TEXT and CMFLAG_WITHCHECKBOX
 *			at the same time
 * */
/****** LBS_ is used by user when create the listbox  ******/
#define LBS_HAVE_VALUE		0x00001000		/* stead of the LBS_CHECKBOX */
#define LBS_USEBITMAP		0x00004000		/* stead of the LBS_AUTOCHECK*/

/* CMFLAG is used by the user in the LISTBOXITEMINFO */
#define CMFLAG_WITHCHECKBOX	0x00200000	/*add check box for a item*/
/****  value mask **********/
#define VMFLAG_IMAGE		0x00010000		/* LBIF_VALUE_IMAGE dwValueImage */
#define VMFLAG_STRING		0x00020000		/* LBIF_VALUE_STRING dwValueString */
/********** image mask *****************/
#define IMGFLAG_IMAGE		0x00040000


/* if create the menu list use the LBS_USEBITMAP LBS_HAVE_VALUE
 * then the lParam use the PMENULISTITEMINFO
 * */
#define MAX_VALUE_COUNT	3
typedef struct __MLFlags
{
	DWORD imageFlag;
	DWORD valueFlag[MAX_VALUE_COUNT];
	unsigned int valueCount;	/* the value's count */
}MLFlags;

typedef struct __MENULISTITEMINFO
{
    /** Item string */
    char* string;

    DWORD   cmFlag;     /* check mark flag */

	MLFlags flagsEX;

    /** Handle to the Bitmap (or pointer to bitmap object) of the item */
	DWORD	hBmpImage;
	/*  address of the value image1 or value string1*/
	DWORD	hValue[MAX_VALUE_COUNT];
} MENULISTITEMINFO;

typedef MENULISTITEMINFO* PMENULISTITEMINFO;

typedef struct{
	gal_pixel normalBgc;
	gal_pixel normalFgc;
	gal_pixel linec;
	gal_pixel normalStringc;
	gal_pixel normalValuec;
	gal_pixel selectedStringc;
	gal_pixel selectedValuec;
	gal_pixel scrollbarc;
	int itemHeight;
	void* context;
}menuListAttr_t;

typedef struct {
	CDR_RECT rect;
	android::String8 title;
	android::Vector < android::String8 > contents;
	int selectedIndex;
	BITMAP BmpChoice;
	gal_pixel lincTitle;
	menuListAttr_t menuListAttr;
	PLOGFONT pLogFont;
	unsigned int menu_index;
}subMenuData_t;


typedef struct {
	gal_pixel bgcWidget;
	gal_pixel fgcWidget;
}ProgressBarData_t;

typedef struct {
  unsigned char sec;			/* Seconds.	[0-60] (1 leap second) */
  unsigned char min;			/* Minutes.	[0-59] */
}PGBTime_t;

/* pop up menu */
#define POPMENU_MAX_ITEMS	10
#define POPMENUS_V			0x01	/* 垂直分布 */
#define POPMENUS_H			0x02
#define POPMENUS_VCENTER	0x04
#define POPMENUS_MASK		0x0F
typedef struct {
	unsigned int style;
	unsigned int item_nrs;
	int id;		/* items' start id */
	const char *name[POPMENU_MAX_ITEMS];
	void (*callback[POPMENU_MAX_ITEMS])(HWND hWnd, int id, int nc, DWORD context);	
	DWORD context;

	CDR_RECT rect;
	unsigned int item_width;
	unsigned int item_height;

	/* 
	 * 0: use the end_key to stop the dialog when end_key key_down
	 * 1: use the end_key to stop the dialog when end_key key_up
	 * */
	unsigned char flag_end_key;	
	unsigned int push_key;		/* the key value to push the button */
	unsigned int table_key;
	unsigned int end_key;		/* key value to end the dialog */

	gal_pixel bgc_widget;
	gal_pixel bgc_item_focus;
	gal_pixel bgc_item_normal;
	gal_pixel mainc_3dbox;
	BITMAP bmp;
	PLOGFONT pLogFont;
	BITMAP pc_usb_bmp;
}PopUpMenuInfo_t;


#define MESSAGEBOX_MAX_BUTTONS	3
#define IDC_BUTTON_START	100
#define IDC_BUTTON_OK		100
#define IDC_BUTTON_CANCEL	101
#define IDC_BUTTON_ELSE		102
#define MB_OK                   0x00000000
#define MB_OKCANCEL             0x00000001
#define MB_HAVE_TITLE			0x00000002
#define MB_HAVE_TEXT			0x00000004
typedef struct {
	/* 
	 * 0: use the end_key to stop the dialog when end_key key_down
	 * 1: use the end_key to stop the dialog when end_key key_up
	 * */
	unsigned char flag_end_key;
	/* Only support one of the MB_OK MB_OKCANCEL*/
	unsigned int dwStyle;
	const char* title;
	const char* text;
	PLOGFONT pLogFont;	

	CDR_RECT rect;
	gal_pixel bgcWidget;
	gal_pixel fgcWidget;
	gal_pixel linecTitle;
	gal_pixel linecItem;
	const char* buttonStr[MESSAGEBOX_MAX_BUTTONS];
	void (*confirmCallback)(HWND hText, void* data);
	void* confirmData;		/* pass to the confirmCallback */
	const char* msg4ConfirmCallback;
}MessageBox_t;

typedef struct {
	const char* title;
	const char* year;
	const char* month;
	const char* day;
	const char* hour;
	const char* minute;
	const char* second;
	CDR_RECT rect;
	int titleRectH;
	int hBorder;
	int yearW;
	int numberW;
	int dateLabelW;
	int boxH;

	gal_pixel bgc_widget;
	gal_pixel fgc_label;
	gal_pixel fgc_number;
	gal_pixel linec_title;
	gal_pixel borderc_selected;
	gal_pixel borderc_normal;
	PLOGFONT pLogFont;
}dateSettingData_t;

typedef struct {
	CDR_RECT rect;	
	unsigned int titleHeight;
	const char* title;
	const char* text;
	PLOGFONT pLogFont;
	gal_pixel bgc_widget;
	gal_pixel fgc_widget;
	gal_pixel linec_title;
	bool disableKeyEvent;
	void (*callback)(HWND hText, void* data);
	void* callbackData;
	/* 
	 * false:  end the label when anykey down, if disableKeyEvent is not set
	 * true:   end the label when anykey up, if disableKeyEvent is not set
	 * */
	bool hasicon;
	BITMAP bitImage;
	bool endLabelKeyUp;	
	unsigned int timeoutMs;
}tipLabelData_t;

typedef struct
{
	DWORD   style;
    DWORD   exStyle;
	HWND	hParent;
}XCreateParams;

extern int MenuListCallback(HWND hWnd, int message, WPARAM wParam, LPARAM lParam);
extern int RegisterCDRControls(void);
extern void UnRegisterCDRControls(void);

extern int showSubMenu(HWND hWnd, subMenuData_t* subMenuData);

extern int showMessageBox(HWND hParentWnd, MessageBox_t *info);

/*
 * if do nothing return 0;
 * if press buttion1, then return the buttion1's id after do some works
 * if error accoured, return -1
 * */
extern int PopUpMenu(HWND hWnd, PopUpMenuInfo_t *info);

extern int dateSettingDialog(HWND hParent, dateSettingData_t* configData);

extern int showTipLabel(HWND hParent, tipLabelData_t* info, int full_screen=0);

extern HWND CreateCdrToolTipWin (HWND hParentWnd, int x, int y, int timeout_ms, const char* text, ...);
extern void ResetCdrToolTipWin (HWND hwnd, int x, int y, const char* text, ...);
extern void DestroyCdrToolTipWin (HWND hwnd);


#ifdef __cplusplus
}  /* end of extern "C" */
#endif


#endif
