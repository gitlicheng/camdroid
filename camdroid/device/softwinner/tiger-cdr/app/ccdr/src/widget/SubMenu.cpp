#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>
#include <minigui/ctrl/listbox.h>

#include <cutils/atomic.h>

#include "cdr_res.h"
#include "misc.h"
#include "cdr_widgets.h"
#include "keyEvent.h"
#include "cdr_message.h"
#include "windows.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "SubMenu.cpp"
#include "debug.h"

#define IDC_SUB_TITLE			410
#define IDC_SUB_LIST_BOX		411
#define TF_CARD_INF				6

static void *card_send_message(void *ptx)
{
	ResourceManager* rm = ResourceManager::getInstance();
	TestResult *test_result ; 
	TestInfo* test_info ;
	bool flag = false;
	test_result = (TestResult *)malloc(sizeof(struct TestResult));
	memset(test_result, 0, sizeof(struct TestResult));
	char *path = MOUNT_PATH;
#ifdef FATFS
	path = MOUNT_POINT;
#endif
	flag = Get_Card_Speed(path,test_result,test_info,NULL);
	if (flag == true)
	{
		SendMessage(rm->getHwnd(WINDOWID_MENU), MSG_CARD_DISPLY_LABLE, 0, 0);
		SendNotifyMessage(rm->getHwnd(WINDOWID_MENU), MSG_CARD_SPEED_DISPLY, (WPARAM)test_result, 0);
		db_msg("---------read_speed = %2.1lfM/s, write_speed = %2.1lfM/s\n------\n",test_result->mReadSpeed/1024,test_result->mWriteSpeed/1024);
	}
	return NULL;
	
}

static int SubMenuProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
	subMenuData_t* subMenuData;
	HDC hdc;

//	db_msg("message is %s\n", Message2Str(message));

	switch(message) {
	case MSG_INITDIALOG:
		{
			int count,ret;
			HWND hMenuList;
			MENULISTITEMINFO mlii;
			aw_cdr_tf_capacity capacity;
			unsigned long int SOS_file_size;
			StorageManager *sm = StorageManager::getInstance();

			subMenuData = (subMenuData_t*)lParam;
			if(subMenuData == NULL) {
				db_error("invalid subMenuData\n");
				EndDialog(hDlg, -3);
			}

			SetWindowAdditionalData(hDlg, (DWORD)lParam);
			hMenuList = GetDlgItem(hDlg, IDC_SUB_LIST_BOX);

			SetWindowFont(hDlg, subMenuData->pLogFont);
			SetWindowBkColor(hDlg, subMenuData->menuListAttr.normalBgc);
			SetWindowElementAttr(GetDlgItem(hDlg, IDC_SUB_TITLE), WE_FGC_WINDOW, PIXEL2DWORD(HDC_SCREEN, subMenuData->menuListAttr.normalFgc));

			/* set the MENULIST item Height */
			int screenW, screenH;
			getScreenInfo(&screenW,&screenH);
			SendMessage(hMenuList, LB_SETITEMHEIGHT, 0, (int)(screenH/6));	/* set the item height */
			SendMessage(hMenuList, MLM_HILIGHTED_SPACE, 8, 8);
			SetFocusChild(hMenuList);

			/* set submenu title */
			SetWindowText(GetDlgItem(hDlg, IDC_SUB_TITLE), subMenuData->title.string());

			/* add item to sub menu list */
			count = subMenuData->contents.size();
			db_msg("count is %d\n", count);
			if(subMenuData->selectedIndex >= count || subMenuData->selectedIndex < 0) {
				db_msg("invalid menu current value\n");
				subMenuData->selectedIndex = 0;
			}

			if(subMenuData->menu_index == TF_CARD_INF)
			{
				db_msg("[debug_jason]:this is for test subMenuData->menu_index is %d\n",subMenuData->menu_index);
				static char capitly_buff[30]={0};
				static char sos_buff[20]={0};
				memset(capitly_buff,0,sizeof(capitly_buff));
				memset(sos_buff,0,sizeof(sos_buff));
				if(sm->isMount()){
					SOS_file_size = sm->getSoS_fileCapitly();
					sprintf(sos_buff, "%.2f/G", SOS_file_size/(float)1024/1024/1024);
					ret = sm->getCapacity(&(capacity.remain), &(capacity.total));
					db_msg("[debug_jaosn]:the avali is :%lld  the total is :%lld ,SOS_file_size:%ld",capacity.remain,capacity.total,SOS_file_size);
					sprintf(capitly_buff, "%1.1f/%1.1fG", capacity.remain/(float)1024,capacity.total/(float)1024);	
					db_msg("capitly_buff is %s;sos_buff is %s\n", capitly_buff,sos_buff);
				}
				for(int i = 0; i < count; i++)
				{
					memset(&mlii, 0, sizeof(mlii));
					mlii.string = (char*)subMenuData->contents.itemAt(i).string();
					if(i == 0) 
					{
						/* set the current item chioce image */
						mlii.flagsEX.imageFlag = 0;
						mlii.flagsEX.valueCount = 1;			
						mlii.flagsEX.valueFlag[0] = VMFLAG_STRING;
						//char *buff1 = "abcded";
						mlii.hValue[0] = (DWORD)capitly_buff;
					}else if(i==1)
					{
						mlii.flagsEX.imageFlag = 0;
						mlii.flagsEX.valueCount = 1;			
						mlii.flagsEX.valueFlag[0] = VMFLAG_STRING;
						mlii.hValue[0] = (DWORD)sos_buff;
					}
					SendMessage(hMenuList, LB_ADDSTRING, 0, (LPARAM)&mlii);
				}
			}else
			{
				for(int i = 0; i < count; i++)
				{
					memset(&mlii, 0, sizeof(mlii));
					mlii.string = (char*)subMenuData->contents.itemAt(i).string();
					if(i == subMenuData->selectedIndex) 
					{
						/* set the current item chioce image */
						mlii.flagsEX.imageFlag = 0;
						mlii.flagsEX.valueCount = 1;			
						mlii.flagsEX.valueFlag[0] = VMFLAG_IMAGE;
						mlii.hValue[0] = (DWORD)&subMenuData->BmpChoice;
					}
					SendMessage(hMenuList, LB_ADDSTRING, 0, (LPARAM)&mlii);
			   	}
			
			}

			/* set the current selected item */
			db_msg("current is %d\n", subMenuData->selectedIndex);
			SendMessage(hMenuList, LB_SETCURSEL, subMenuData->selectedIndex, 0);
		}
		break;
	case MSG_FONTCHANGED:
		{
			PLOGFONT pLogFont;

			pLogFont = GetWindowFont(hDlg);
			if(pLogFont == NULL)
				break;
			db_msg("type: %s, family: %s, charset: %s\n", pLogFont->type, pLogFont->family, pLogFont->charset);
			SetWindowFont(GetDlgItem(hDlg, IDC_SUB_TITLE), GetWindowFont(hDlg));
			SetWindowFont(GetDlgItem(hDlg, IDC_SUB_LIST_BOX), GetWindowFont(hDlg));
		}
		break;
	case MSG_KEYUP:
		{
			int index;
			bool flag = false;
			subMenuData = (subMenuData_t*)GetWindowAdditionalData(hDlg);
			ResourceManager* rm = ResourceManager::getInstance();
			StorageManager *sm = StorageManager::getInstance();
			if(wParam == CDR_KEY_OK) {
				db_msg("press key ok\n");
				index = SendMessage(GetDlgItem(hDlg, IDC_SUB_LIST_BOX), LB_GETCURSEL, 0, 0);
				if(subMenuData->menu_index == TF_CARD_INF)
				{
					db_msg("[debug_jaosn]:the index is :%d",index);
					switch(index)
					{
						case 2:
						{
							if(!sm->isInsert())
							{
								EndDialog(hDlg, -2);
								ShowTipLabel(rm->getHwnd(WINDOWID_MENU), LABEL_NO_TFCARD);
								break;
							}
							EndDialog(hDlg, -2);
							pthread_t thread_id = 0;
							int err = pthread_create(&thread_id, NULL, card_send_message, NULL);
							if(err || !thread_id) {
								db_msg("Create card_send_message  thread error.");
								return -1;
							}
							ShowTipLabel(rm->getHwnd(WINDOWID_MENU), LABEL_SPEED_TFCARD);
							break;
						}
						default :
						   	break;
					}
				}
				EndDialog(hDlg, index);
			} else if (wParam == CDR_KEY_POWER) {
#if 0
				int retval;
				native_sock_msg_t msg;	

				memset(&msg, 0, sizeof(native_sock_msg_t));
				msg.id = hDlg;	
				msg.messageid = MWM_CHANGE_SS;

				retval = send_sockmsg2server(SOCK_SERVER_NAME, &msg);
				if(retval == sizeof(native_sock_msg_t)) {
					db_msg("send message successd, retval is %d\n", retval);	
				} else {
					db_msg("send message failed, retval is %d\n", retval);	
				}
#endif
			} else if (wParam == CDR_KEY_MODE || wParam == CDR_KEY_MENU) {
				EndDialog(hDlg, -2);
			}
		}
		break;
	case MSG_KEYDOWN:
		{
#if 0
			int retval;
			native_sock_msg_t msg;	

			memset(&msg, 0, sizeof(native_sock_msg_t));
			msg.id = hDlg;	
			msg.messageid = MWM_WAKEUP_SS;

			retval = send_sockmsg2server(SOCK_SERVER_NAME, &msg);
			if(retval == sizeof(native_sock_msg_t)) {
				db_msg("send message successd, retval is %d\n", retval);	
			} else {
				db_msg("send message failed, retval is %d\n", retval);	
			}
#endif
		}
		break;
	case MSG_PAINT:
		{
			RECT rect;
			hdc = BeginPaint(hDlg);

			subMenuData = (subMenuData_t*)GetWindowAdditionalData(hDlg);
			GetClientRect(GetDlgItem(hDlg, IDC_SUB_TITLE), &rect);
			SetPenColor(hdc, subMenuData->lincTitle);
			Line2(hdc, 0, RECTH(rect) + 2, RECTW(rect), RECTH(rect) + 2);

			EndPaint(hDlg, hdc);
		}
		break;
	case MSG_ERASEBKGND:
		break;
	case MSG_CLOSE_TIP_LABEL:
		EndDialog(hDlg, IDCANCEL);
		break;
	case MSG_CLOSE:
		{
			subMenuData = (subMenuData_t*)GetWindowAdditionalData(hDlg);
			if(subMenuData->BmpChoice.bmBits) {
				UnloadBitmap(&subMenuData->BmpChoice);
			}

			EndDialog(hDlg, IDCANCEL);
		}
		break;
	default:
		break;
	}

	return DefaultDialogProc(hDlg, message, wParam, lParam);
}

/* ********** hWnd 是Menu List的句柄 ************** 
 * 返回值是用户选中的item在ListBox中的序号
 * */
int showSubMenu(HWND hWnd, subMenuData_t* subMenuData)
{
	CDR_RECT rect;
	DLGTEMPLATE DlgSubMenu;
	CTRLDATA CtrlSubMenu[2];
	int screenW,screenH;

	memset(&DlgSubMenu, 0, sizeof(DlgSubMenu));
	memset(CtrlSubMenu, 0, sizeof(CtrlSubMenu));

	rect = subMenuData->rect;
	getScreenInfo(&screenW, &screenH);
	CtrlSubMenu[0].class_name = CTRL_STATIC;
	CtrlSubMenu[0].dwStyle = WS_CHILD | WS_VISIBLE | SS_CENTER | SS_VCENTER;
	CtrlSubMenu[0].dwExStyle = WS_EX_TRANSPARENT;
	CtrlSubMenu[0].caption = "";
	CtrlSubMenu[0].x = 0;
	CtrlSubMenu[0].y = 0;
	CtrlSubMenu[0].w = rect.w;
	CtrlSubMenu[0].h = screenH/8;
	CtrlSubMenu[0].id = IDC_SUB_TITLE;

	CtrlSubMenu[1].class_name = CTRL_CDRMenuList;
	CtrlSubMenu[1].dwStyle = WS_CHILD| WS_VISIBLE | WS_VSCROLL | LBS_USEBITMAP | LBS_HAVE_VALUE;
	CtrlSubMenu[1].dwExStyle = WS_EX_TRANSPARENT;
	CtrlSubMenu[1].caption = "";
	CtrlSubMenu[1].x = 0;
	CtrlSubMenu[1].y = CtrlSubMenu[0].h;
	CtrlSubMenu[1].w = rect.w;
	CtrlSubMenu[1].h = rect.h - CtrlSubMenu[0].h;
	CtrlSubMenu[1].id = IDC_SUB_LIST_BOX;
	CtrlSubMenu[1].dwAddData = (DWORD)&subMenuData->menuListAttr;

	DlgSubMenu.dwStyle = WS_VISIBLE | WS_CHILD;
	DlgSubMenu.dwExStyle = WS_EX_TRANSPARENT;
	DlgSubMenu.x = rect.x;
	DlgSubMenu.y = rect.y;
	DlgSubMenu.w = rect.w;
	DlgSubMenu.h = rect.h;
	DlgSubMenu.caption = "";
	DlgSubMenu.controlnr = 2;
	DlgSubMenu.controls = (PCTRLDATA)CtrlSubMenu;

	return DialogBoxIndirectParam(&DlgSubMenu, hWnd, SubMenuProc, (DWORD)subMenuData);
}
