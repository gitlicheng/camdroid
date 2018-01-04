
#include "MenuInheritors.h"
#include "MenuRecordPreview.h"
#include "licensePlateWM.h"
#undef LOG_TAG
#define LOG_TAG "MenuSystem"
#include "debug.h"

using namespace android;

int MenuRecordPreview::menuObjWindowProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
	//db_msg("message code: [%d], %s, wParam=%d, lParam=0x%lx\n", message, Message2Str(message), wParam, lParam);

	return WANT_SYSTEM_PROCESS;
}


int MenuRecordPreview::keyProc(int keyCode, int isLongPress)
{
	switch(keyCode) {
	case CDR_KEY_OK:
		{
			unsigned int selectedItem;
			int newSel;
			bool isModified, isChecked;
			if(isLongPress == SHORT_PRESS) {
				selectedItem = SendMessage(mHwnd, LB_GETCURSEL, 0, 0);
				db_msg("selectedItem is %d\n", selectedItem);
				switch(selectedItem) {
				case MENU_INDEX_VIDEO_RESOLUTION:	case MENU_INDEX_VIDEO_BITRATE:
				case MENU_INDEX_VTL:				case MENU_INDEX_WB:
				case MENU_INDEX_CONTRAST:			case MENU_INDEX_EXPOSURE:
					{
						if( ( newSel = ShowSubMenu(selectedItem, isModified) ) < 0) {
							db_error("show submenu %d failed\n", selectedItem);
							break;
						}
						if(isModified == true)
							HandleSubMenuChange(selectedItem, newSel);
					}
					break;
				case MENU_INDEX_POR:			case MENU_INDEX_TWM:	
				case MENU_INDEX_AWMD:
#ifdef APP_VERSION
				case MENU_INDEX_SMARTALGORITHM:
#endif
					{
						if(getCheckBoxStatus(selectedItem, isChecked) < 0) {
							db_error("get check box status failed\n");
							break;
						}
						HandleSubMenuChange(selectedItem, isChecked);
					}
					break;
				case MENU_INDEX_LICENSE_PLATE_WM:
					showLicensePlateWM();
					break;
				default:
					break;
				}
			}
		}
	default:
		break;
	}

	return WINDOWID_MENU;
}

MenuRecordPreview::MenuRecordPreview(class Menu* parent, AvaliableMenu menuObjId) : 
	MenuObj(parent, MENU_LIST_COUNT, menuObjId)
{
	db_info("MenuSystem Constructor\n");

	if(mHwnd != HWND_INVALID) {
		initMenuObjParams();
		createMenuListContents(mHwnd);
	} else {
		db_msg("MenuSystem window not created\n");
		return;
	}
}

MenuRecordPreview::~MenuRecordPreview()
{
	db_info("MenuSystem Destructor\n");
}

// must be called from the MenuCloudDog Constructor
void MenuRecordPreview::initMenuObjParams()
{
	db_msg("initMenuObjParams\n");

	mMenuResourceID = menuResourceID;
	mHaveCheckBox	= haveCheckBox;
	mHaveSubMenu	= haveSubMenu;
	mHaveValueString	= haveValueString;
	mSubMenuContent0Cmd = subMenuContent0Cmd;
	mMlFlags = mlFlags;
}

int MenuRecordPreview::HandleSubMenuChange(unsigned int menuIndex, int newSel)
{
	ResourceManager* rm;

	rm = ResourceManager::getInstance();

	switch(menuIndex) {
	case MENU_INDEX_VIDEO_RESOLUTION:
	case MENU_INDEX_VIDEO_BITRATE:
	case MENU_INDEX_VTL:
	case MENU_INDEX_WB:	
	case MENU_INDEX_CONTRAST:	
	case MENU_INDEX_EXPOSURE:	
		if(haveSubMenu[menuIndex] != 1) {
			db_error("invalid menuIndex %d\n", menuIndex);
			return -1;
		}
		if(rm->setResIntValue(menuResourceID[menuIndex], INTVAL_SUBMENU_INDEX, newSel) < 0) {
			db_error("set %d to %d failed\n", menuIndex, newSel);	
			return -1;
		}
		break;
	default:
		db_error("invalid menuIndex %d\n", menuIndex);
		return -1;
	}

	MENULISTITEMINFO mlii;

	if(SendMessage(mHwnd, LB_GETITEMDATA, menuIndex, (LPARAM)&mlii) != LB_OKAY) {
		db_error("get item info failed, menuIndex is %d\n", menuIndex);
		return -1;
	}

	if(getFirstValueStrings(&mlii, menuIndex) < 0) {
		db_error("get first value strings failed\n");
		return -1;
	}
	db_msg("xxxxxxxx\n");
	SendMessage(mHwnd, LB_SETITEMDATA, menuIndex, (LPARAM)&mlii);
	db_msg("xxxxxxxx\n");

	return 0;
}

int MenuRecordPreview::HandleSubMenuChange(unsigned int menuIndex, bool newValue)
{
	ResourceManager* rm;

	db_msg("menuObj %d, set menuIndex %s\n", mMenuObjId, newValue ? "true" : "false");
	rm = ResourceManager::getInstance();
	switch(menuIndex) {
	case MENU_INDEX_POR:
	case MENU_INDEX_TWM:
	case MENU_INDEX_AWMD:
#ifdef APP_VERSION
	case MENU_INDEX_SMARTALGORITHM:
#endif
		if(haveCheckBox[menuIndex] != 1) {
			db_error("invalid menuIndex %d\n", menuIndex);
			return -1;
		}
		if(rm->setResBoolValue(menuResourceID[menuIndex], newValue) < 0) {
			db_error("set %d to %d failed\n", menuIndex, newValue);
			return -1;
		}
		break;
	default:
		db_error("invalid menuIndex %d\n", menuIndex);
		return -1;
	}

	MENULISTITEMINFO mlii;
	memset(&mlii, 0, sizeof(mlii));
	if(SendMessage(mHwnd, LB_GETITEMDATA, menuIndex, (LPARAM)&mlii) != LB_OKAY) {
		db_error("get item info failed, menuIndex is %d\n", menuIndex);
		return -1;
	}
	if(getFirstValueImages(&mlii, menuIndex) < 0) {
		db_error("get first value images failed, menuIndex is %d\n", menuIndex);
		return -1;
	}

	db_msg("xxxxxxxx\n");
	SendMessage(mHwnd, LB_SETITEMDATA, menuIndex, (LPARAM)&mlii);
	db_msg("xxxxxxxx\n");

	return 0;
}

int MenuRecordPreview::showLicensePlateWM(void)
{
	int retval;
	const char* ptr;
	int len;

	ResourceManager* rm;
	LicensePlateWM_data_t dlgData;
	BITMAP bmpTitle;
	BITMAP bmpEnableChoice;
	BITMAP bmpDisableChoice;
	BITMAP bmpArrow;

	rm = ResourceManager::getInstance();
	memset(&dlgData, 0, sizeof(dlgData));
	dlgData.pLogFont = GetWindowFont(mHwnd);
	dlgData.dlgRect.x = 20;
	dlgData.dlgRect.y = 20;
	dlgData.dlgRect.w = 280;
	dlgData.dlgRect.h = 200;

	dlgData.titleText = rm->getLabel(LANG_LABEL_MENU_LICENSE_PLATE_WM);
	dlgData.enableText = "";
	dlgData.disableText = "";

	LoadBitmapFromFile(HDC_SCREEN, &bmpTitle, "system/res/menu/license_plate_wm.png");
	LoadBitmapFromFile(HDC_SCREEN, &bmpEnableChoice, "system/res/menu/Select.png");
	LoadBitmapFromFile(HDC_SCREEN, &bmpDisableChoice, "system/res/menu/unselect.png");
	LoadBitmapFromFile(HDC_SCREEN, &bmpArrow, "system/res/menu/arrow.png");
	dlgData.pBmpTitle = &bmpTitle;
	dlgData.pBmpEnableChioce = &bmpEnableChoice;
	dlgData.pBmpDisableChioce = &bmpDisableChoice;
	dlgData.pBmpArrow = &bmpArrow;

	ptr = rm->getLabel(LANG_LABEL_LICENSE_PLATE_NUMBERS_EN);
	if(ptr == NULL)
		return -1;
	len = strlen(ptr);
	strncpy(dlgData.candidateEnNumbers, ptr, (len > CANDICATE_BUFFER_LEN) ? CANDICATE_BUFFER_LEN : len );
	ptr = rm->getLabel(LANG_LABEL_LICENSE_PLATE_NUMBERS_CN);
	if(ptr == NULL)
		return -1;
	len = strlen(ptr);
	strncpy(dlgData.candidateCnNumbers, ptr, (len > CANDICATE_BUFFER_LEN) ? CANDICATE_BUFFER_LEN : len );
	dlgData.language = rm->getLanguage();
	retval = licensePlateWM_Dialog(mHwnd, &dlgData);
	UnloadBitmap(&bmpTitle);
	UnloadBitmap(&bmpEnableChoice);
	UnloadBitmap(&bmpDisableChoice);
	UnloadBitmap(&bmpArrow);

	return retval;
}
