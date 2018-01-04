#include "MenuInheritors.h"
#include "MenuPhoto.h"
#undef LOG_TAG
#define LOG_TAG "MenuPhoto"
#include "debug.h"

using namespace android;

int MenuPhoto::menuObjWindowProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
	//db_msg("message code: [%d], %s, wParam=%d, lParam=0x%lx\n", message, Message2Str(message), wParam, lParam);

	return WANT_SYSTEM_PROCESS;
}


int MenuPhoto::keyProc(int keyCode, int isLongPress)
{
	switch(keyCode) {
	case CDR_KEY_OK:
		{
			unsigned int selectedItem;
			int newSel;
			if(isLongPress == SHORT_PRESS) {
				selectedItem = SendMessage(mHwnd, LB_GETCURSEL, 0, 0);
				db_msg("selectedItem is %d\n", selectedItem);
				switch(selectedItem) {
				case MENU_INDEX_PHOTO_RESOLUTION:		case MENU_INDEX_PHOTO_COMPRESSION_QUALITY:
				case MENU_INDEX_WB:						case MENU_INDEX_EXPOSURE:
					{
						bool isModified;
						if( ( newSel = ShowSubMenu(selectedItem, isModified) ) < 0) {
							db_error("show submenu %d failed\n", selectedItem);
							break;
						}
						if(isModified == true)
							HandleSubMenuChange(selectedItem, newSel);
					}
					break;
				case MENU_INDEX_PHOTO_WM:
					{
						bool isChecked;
						if(getCheckBoxStatus(selectedItem, isChecked) < 0) {
							db_error("get check box status failed\n");
							break;
						}
						HandleSubMenuChange(selectedItem, isChecked);
					}
					break;
				default:
					break;
				}
			}
		}
		break;
	default:
		break;
	}
	return WINDOWID_MENU;
}

MenuPhoto::MenuPhoto(class Menu* parent, AvaliableMenu menuObjId) : 
	MenuObj(parent, MENU_LIST_COUNT, menuObjId)
{
	db_info("MenuPhoto Constructor\n");

	if(mHwnd != HWND_INVALID) {
		initMenuObjParams();
		createMenuListContents(mHwnd);
	} else {
		db_msg("MenuPhoto window not created\n");
		return;
	}
}

MenuPhoto::~MenuPhoto()
{
	db_info("MenuPhoto Destructor\n");
}


// must be called from the MenuCloudDog Constructor
void MenuPhoto::initMenuObjParams()
{
	db_msg("initMenuObjParams\n");

	mMenuResourceID = menuResourceID;
	mHaveCheckBox	= haveCheckBox;
	mHaveSubMenu	= haveSubMenu;
	mHaveValueString	= haveValueString;
	mSubMenuContent0Cmd = subMenuContent0Cmd;
	mMlFlags = mlFlags;
}

int MenuPhoto::HandleSubMenuChange(unsigned int menuIndex, int newSel)
{
	ResourceManager* rm;

	rm = ResourceManager::getInstance();

	switch(menuIndex) {
	
	case MENU_INDEX_PHOTO_RESOLUTION:	
	case MENU_INDEX_PHOTO_COMPRESSION_QUALITY:	
	case MENU_INDEX_WB:	
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

int MenuPhoto::HandleSubMenuChange(unsigned int menuIndex, bool newValue)
{
	ResourceManager* rm;

	db_msg("menuObj %d, set menuIndex %s\n", mMenuObjId, newValue ? "true" : "false");
	rm = ResourceManager::getInstance();

	switch(menuIndex) {
	case MENU_INDEX_PHOTO_WM:
		{
			if(haveCheckBox[menuIndex] != 1) {
				db_error("invalid menuIndex %d\n", menuIndex);
				return -1;
			}
			if(rm->setResBoolValue(menuResourceID[menuIndex], newValue) < 0) {
				db_error("set %d to %d failed\n", menuIndex, newValue);
				return -1;
			}
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

