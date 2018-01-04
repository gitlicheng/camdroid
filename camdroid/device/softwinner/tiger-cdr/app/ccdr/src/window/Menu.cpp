
#include "windows.h"
#include "MenuInheritors.h"
#include "coolIndicator.h"
#undef LOG_TAG
#define	 LOG_TAG "Menu.cpp"
#include "debug.h"
#include "MenuSystem.h"


#define IDC_MENULIST			400
#define MENU_LIST_ITEM_HEIGHT	49

using namespace android;

static int formatTf =0,isFormatTimer=0;
int MenuProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
	Menu* menu;
	//db_msg("message is %s, wParam: 0x%x, lParam: 0x%lx\n", Message2Str(message), wParam, lParam);
	switch(message) {
	case MSG_CREATE:
		/* for switch ui Bug: ui from others to RecordPreview will not refresh */
		RECT rect;
		HWND hMainWin;
		HDC secDC, MainWindowDC;

		hMainWin = GetMainWindowHandle(hWnd);
		GetClientRect(hMainWin, &rect);
		secDC = GetSecondaryDC(hMainWin);
		MainWindowDC = GetDC(hMainWin);

		SetMemDCAlpha(secDC, 0, 0);
		BitBlt(secDC, 0, 0, RECTW(rect), RECTH(rect), secDC, 0, 0, 0);

		ReleaseDC(MainWindowDC);
		break;
	case MSG_FONTCHANGED:
		menu = (Menu*)GetWindowAdditionalData(hWnd);
		menu->onFontChanged();
		break;
	case MSG_SHOWWINDOW:
		if(wParam == SW_SHOWNORMAL) {
			menu = (Menu*)GetWindowAdditionalData(hWnd);
			menu->focusCurrentMenuObj();
			ShowWindow(GetDlgItem(hWnd, ID_MENU_LIST_INDICATOR), SW_SHOWNORMAL);
		} else {
			ShowWindow(GetDlgItem(hWnd, ID_MENU_LIST_INDICATOR), SW_HIDE);
		}
		break;
	case MWM_CHANGE_FROM_WINDOW:
		menu = (Menu*)GetWindowAdditionalData(hWnd);
		menu->onChangeFromOtherWindow(wParam);
		break;
	case MLM_NEW_SELECTED:
		menu = (Menu*)GetWindowAdditionalData(hWnd);
		menu->onNewSelected(wParam, lParam);
		break;
	case MSG_CDR_KEY:
		menu = (Menu*)GetWindowAdditionalData(hWnd);
		return menu->keyProc(wParam, lParam);
	case MSG_TIMER:
		if(wParam == FORMATTF_TIMER){
			formatTf++;
			if(formatTf == 5){
				db_msg("******formatTf:%d*******",formatTf);
				BroadcastMessage(MSG_CLOSE_TIP_LABEL, 0, 0);
				KillTimer(hWnd,FORMATTF_TIMER); 
				isFormatTimer = 0;
			}
		}
		break;
	case MSG_FORMAT_TFDIALOG:
		menu = (Menu*)GetWindowAdditionalData(hWnd);
		menu->formatTfCardOutSide(wParam);
		break;
		
	case STBM_MOUNT_TFCARD:{
		db_msg("******[debug_jaosn]:this  message is STBM_MOUNT_TFCARD *******\n");
		db_msg("******[debug_jaosn]:this  message is wParam :%d*******\n",wParam);
		bool isChecked;
		menu = (Menu*)GetWindowAdditionalData(hWnd);
		if(!FILE_EXSIST(MOUNT_PATH"full_img.fex")){
			db_msg("/mnt/extsd/full_img.fex not exist");
			isChecked = false;
			menu->hand_message(MENU_INDEX_FIRMWARE,isChecked);
		}else{
			db_msg("/mnt/extsd/full_img.fex is  1 exist");
			isChecked = true;
			menu->hand_message(MENU_INDEX_FIRMWARE,isChecked);
		}
		break;
	}
	case MSG_CARD_SPEED_DISPLY:
		menu = (Menu*)GetWindowAdditionalData(hWnd);
		TestResult *test_result ; 
		test_result = (TestResult *)malloc(sizeof(struct TestResult));
		memset(test_result, 0, sizeof(struct TestResult));
		test_result = (TestResult *)wParam;
		db_msg("---------read_speed = %2.1lfM/s, write_speed = %2.1lfM/s\n------\n",test_result->mReadSpeed/1024,test_result->mWriteSpeed/1024);
		menu->Card_speed_disp(test_result);
		free(test_result);
		break;
	case MSG_CARD_DISPLY_LABLE:
		menu = (Menu*)GetWindowAdditionalData(hWnd);
		menu->Card_speed_disp_lable();
		break;
	case MSG_FORMAT_TFCARD_NOW:
	{
		menu = (Menu*)GetWindowAdditionalData(hWnd);
		MenuSystem* menuSystem = NULL;
		menuSystem = (MenuSystem*)menu->getMenuObj(MENU_OBJ_SYSTEM);
		menuSystem->ShowFormattingTip(false);
	}
		break;
	case MSG_FORMAT_TFCARD_FINISH:
	{
		BroadcastMessage(MSG_CLOSE_TIP_LABEL, 0, 0);
		ShowTipLabel(hWnd, LABEL_FORMAT_TFCARD_FINISH, false,3000,true);
		#ifdef FATFS
		StorageManager * sm = StorageManager::getInstance();
		sm->setFATFSMountValue(true);
		#endif
	}
		break;
	default:
		break;
	}

	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

static int MenuObjsProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
	WNDPROC proc = NULL;
	int ret;
	Menu::MenuObj* menuObj = (Menu::MenuObj*)(((menuListAttr_t*)GetWindowAdditionalData(hWnd))->context);

	if(menuObj != NULL) {
		ret = menuObj->menuObjWindowProc(hWnd, message, wParam, lParam);
		if(ret == WANT_SYSTEM_PROCESS) {
			proc = menuObj->getOldProc();
			if(proc != NULL) {
				return (*proc)(hWnd, message, wParam, lParam);
			}
		}
	}

	return 0;
}

Menu::Menu(CdrMain* pCdrMain) : 
	mCdrMain(pCdrMain),
	mCurMenuObjId(MENU_OBJ_SYSTEM),
	mHwndIndicator(HWND_INVALID),
	mSubMenu(false)
{
	CDR_RECT rect;
	ResourceManager* rm;

	db_info("Menu Constructor\n");

	rm = ResourceManager::getInstance();
	rm->getResRect(ID_MENU_LIST, rect);

	mHwnd = CreateWindowEx(WINDOW_MENU, "",
			WS_NONE,
			WS_EX_USEPARENTFONT,
			WINDOWID_MENU,
			rect.x, rect.y, rect.w, rect.h,
			mCdrMain->getHwnd(), (DWORD)this);
	if(mHwnd == HWND_INVALID) {
		db_error("create menu window failed\n");
		return;
	}
	ShowWindow(mHwnd, SW_HIDE);

	// ------------------ create menu indicator -----------------
	rm->getResRect(ID_MENU_LIST_INDICATOR, rect);
	mHwndIndicator = CreateWindowEx (CTRL_COOL_INDICATOR, "",
		WS_CHILD | WS_VISIBLE | CBS_BMP_CUSTOM, 
		WS_EX_USEPARENTFONT,
		ID_MENU_LIST_INDICATOR,
		rect.x, rect.y, rect.w, rect.h,
		mHwnd,
		MAKELONG(42, 28));
	if(mHwndIndicator == HWND_INVALID ) {
		db_error("create menu Indicator failed\n");
		return;
	}
	SetWindowBkColor(mHwndIndicator, rm->getResColor(ID_MENU_LIST_INDICATOR, COLOR_BGC)); //UI@coolbox_backcolor

	RECT rc;
	GetClientRect(mHwndIndicator, &rc);
	db_msg("menuIndicator: left: %d, top: %d, right: %d, bottom: %d\n", rc.left, rc.top, rc.right, rc.bottom);

	if(createMenuObjs(mHwnd) < 0) {
		db_error("create subWidgets failed\n");
		mHwnd = HWND_INVALID;
		return;
	}

	rm->setHwnd(WINDOWID_MENU, mHwnd);
}

Menu::~Menu()
{
	BITMAP* pBmp = NULL;
	enum AvaliableMenu menuObjId;
	ssize_t index = 0;
	for (unsigned int i = 0; i < mAvailableMenuObjIds.size(); i++) {
		menuObjId = mAvailableMenuObjIds.itemAt(i);
		index = mMenuObjIcons.indexOfKey(menuObjId);
		if(index >= 0) {
			pBmp = mMenuObjIcons.valueAt(index);
		}
		if(pBmp != NULL) {
			free(pBmp);
		}
    }
}

// ------------------ Menu private implements --------------------
int Menu::createMenuObjs(HWND hWnd)
{
	ResourceManager* rm;
	class MenuObj *menuObj = NULL;
	BITMAP* pBmp;

	mMenuObjList.clear();
	mMenuObjIcons.clear();
	mMenuObjHintIds.clear();

	rm = ResourceManager::getInstance();
	db_msg("createMenuObjs Menu hWnd is 0x%x\n", hWnd);

	menuObj = new MenuSystem(this, MENU_OBJ_SYSTEM);
	if(menuObj->getHwnd() == HWND_INVALID) {
		db_error("create MenuSystem fail\n");
		return -1;
	} else {
		mMenuObjList.add(MENU_OBJ_SYSTEM, menuObj);
	}

	menuObj = new MenuRecordPreview(this, MENU_OBJ_RECORDPREVIEW);
	if(menuObj->getHwnd() == HWND_INVALID) {
		db_error("create MenuRecordPreview fail\n");
		return -1;
	} else {
		mMenuObjList.add(MENU_OBJ_RECORDPREVIEW, menuObj);
	}

	menuObj = new MenuPhoto(this, MENU_OBJ_PHOTO);
	if(menuObj->getHwnd() == HWND_INVALID) {
		db_error("create MenuPhoto fail\n");
		return -1;
	} else {
		mMenuObjList.add(MENU_OBJ_PHOTO, menuObj);
	}

	pBmp = (PBITMAP)malloc(sizeof(BITMAP));
	rm->getResBmp2(ID_MENU_LIST_INDICATOR, 3, *pBmp);
	mMenuObjIcons.add(MENU_OBJ_SYSTEM, pBmp);
	mMenuObjHintIds.add(MENU_OBJ_SYSTEM, LANG_LABEL_SYSTEM_MENU);
	
	pBmp = (PBITMAP)malloc(sizeof(BITMAP));
	rm->getResBmp2(ID_MENU_LIST_INDICATOR, 0, *pBmp);
	mMenuObjIcons.add(MENU_OBJ_RECORDPREVIEW, pBmp);
	mMenuObjHintIds.add(MENU_OBJ_RECORDPREVIEW, LANG_LABEL_RECORDPREVIEW_MENU);

	pBmp = (PBITMAP)malloc(sizeof(BITMAP));
	rm->getResBmp2(ID_MENU_LIST_INDICATOR, 1, *pBmp);
	mMenuObjIcons.add(MENU_OBJ_PHOTO, pBmp);
	mMenuObjHintIds.add(MENU_OBJ_PHOTO, LANG_LABEL_PHOTO_MENU);

	return 0;
}

Menu::MenuObj* Menu::getCurrentMenuObj(void)
{
	return getMenuObj(mCurMenuObjId);
}

Menu::MenuObj* Menu::getMenuObj(enum AvaliableMenu menuObjId)
{
	MenuObj* obj = NULL;
	
	ssize_t index = mMenuObjList.indexOfKey(menuObjId);
	if(index >= 0) {
		obj = mMenuObjList.valueAt(index);
	}

	db_msg("index is %ld, current MenuObj is 0x%p\n", index, obj);
	return obj;
}

int Menu::hideMenuObj(enum AvaliableMenu menuObjId)
{
	HWND hWndMenuObj;
	class MenuObj* menuObj = NULL;

	db_msg("hideMenuObj id %d\n", menuObjId);

	menuObj = getMenuObj(menuObjId);
	if(menuObj == NULL) {
		db_error("get menuObj failed\n");
		return -1;
	}

	hWndMenuObj = menuObj->getHwnd();

	if(hWndMenuObj && hWndMenuObj != HWND_INVALID) {
		ShowWindow(hWndMenuObj, SW_HIDE);
		return 0;
	}

	return -1;
}

int Menu::showMenuObj(enum AvaliableMenu menuObjId)
{
	HWND hWndMenuObj;
	class MenuObj* menuObj = NULL;

	db_msg("showMenuObj id %d\n", menuObjId);

	menuObj = getMenuObj(menuObjId);
	if(menuObj == NULL) {
		db_error("get menuObj failed\n");
		return -1;
	}

	hWndMenuObj = menuObj->getHwnd();

	if(hWndMenuObj && hWndMenuObj != HWND_INVALID) {
		ShowWindow(hWndMenuObj, SW_SHOWNORMAL);
		SetFocusChild(hWndMenuObj);
		return 0;
	}

	return -1;
}

// 1. set the menuObjId
// 2. hide the old Menu Obj and show the new Menu Obj
// 3. show the newMenuObj
// 4. update new MenuObj
void Menu::setMenuObjId(enum AvaliableMenu menuObjId)
{
	enum AvaliableMenu oldMenuObjId = mCurMenuObjId;

	if(menuObjId < MENU_OBJ_END) {
		if(mCurMenuObjId == menuObjId) {
			showMenuObj(mCurMenuObjId);
			updateMenu(mCurMenuObjId);
		} else {
			if( (hideMenuObj(oldMenuObjId) == 0) && (showMenuObj(menuObjId) == 0) ) {
				mCurMenuObjId = menuObjId;
				updateMenu(mCurMenuObjId);
			}
		}
	}
}

void Menu::updateMenu(enum AvaliableMenu menuObjId)
{
	class MenuObj* menuObj = NULL;

	db_msg("updateMenuObj %d\n", menuObjId);

	menuObj = getCurrentMenuObj();

	if(menuObj != NULL) {
		menuObj->updateMenu();
	}
}

// if return value < 0, then no menuObj avaliable to switch
int Menu::switch2nextMenuObj(void)
{
	size_t nextMenuObjIndex = 0;

	if(mMenuObjList.size() <= 1)
		return -1;

	// find next menuObjId from the available MenuObjId list
	for(unsigned int i = 0; i < mAvailableMenuObjIds.size(); i++) {
		if(mAvailableMenuObjIds.itemAt(i) == mCurMenuObjId) {
			nextMenuObjIndex = i + 1;
			if(nextMenuObjIndex >= mAvailableMenuObjIds.size())
				nextMenuObjIndex = 0;
			
			break;
		}
	}

	if(nextMenuObjIndex < MENU_OBJ_END) {
		setMenuObjId(mAvailableMenuObjIds.itemAt(nextMenuObjIndex));
		SendMessage(mHwndIndicator, CBM_SWITCH2NEXT, 0, 0);
		return 0;
	}

	return -1;
}

void Menu::initMenuIndicator(void)
{
	ssize_t index;
	COOL_INDICATOR_ITEMINFO item;
	AvaliableMenu menuObjId;
	ResourceManager* rm;
	PBITMAP pBmp = NULL;
	const char* hint = NULL;

	rm = ResourceManager::getInstance();
    item.ItemType = TYPE_BMPITEM;
    item.dwAddData = 0;
	item.Caption = NULL;
	item.sliderColor = rm->getResColor(ID_MENU_LIST_INDICATOR, COLOR_SCROLLBARC);

    for (unsigned int i = 0; i < mAvailableMenuObjIds.size(); i++) {
		menuObjId = mAvailableMenuObjIds.itemAt(i);
		index = mMenuObjIcons.indexOfKey(menuObjId);
		if(index >= 0) {
			pBmp = mMenuObjIcons.valueAt(index);
		}
		index = mMenuObjHintIds.indexOfKey(menuObjId);
		if(index >= 0) {
			hint = rm->getLabel(mMenuObjHintIds.valueAt(index));
		}

		if(pBmp != NULL && hint != NULL) {
			item.Bmp = pBmp;
			item.id = i;
			item.ItemHint = hint;
			SendMessage (mHwndIndicator, CBM_ADDITEM, 0, (LPARAM)&item);
		}
    }
}

void Menu::clearMenuIndicator(void)
{
	SendMessage(mHwndIndicator, CBM_CLEARITEM, 0, 0);
}

// ++++++++++++++ End of  Menu private implements ----------------

// ------------------ Menu Public interfaces ---------------------
int Menu::keyProc(int keyCode, int isLongPress)
{
	switch(keyCode) {
	case CDR_KEY_MENU:
		{
			ResourceManager* rm;	
			rm = ResourceManager::getInstance();
			if( isLongPress == SHORT_PRESS ){
				if((switch2nextMenuObj() < 0) ){
					rm->syncConfigureToDisk();
					clearMenuIndicator();
					return WINDOWID_RECORDPREVIEW;
				}
			} else {
				rm->syncConfigureToDisk();
				clearMenuIndicator();
				return WINDOWID_RECORDPREVIEW;
			}
		}
	default:
		{
			unsigned int ret;
			class MenuObj* menuObj = NULL;
			menuObj = getCurrentMenuObj();

			if(menuObj != NULL) {
				ret = menuObj->keyProc(keyCode, isLongPress);
				if (mCurMenuObjId != MENU_OBJ_SYSTEM && keyCode == CDR_KEY_MENU) {
					ResourceManager* rm;
					rm = ResourceManager::getInstance();
					rm->syncConfigureToDisk();
					clearMenuIndicator();
					return WINDOWID_RECORDPREVIEW;
				} else {
					return ret;
				}
			}
		}
		break;
	}

	return WINDOWID_MENU;
}

void Menu::onFontChanged()
{
	MenuObj* menuObj = NULL;

	menuObj = getCurrentMenuObj();
	if(menuObj != NULL) {
		menuObj->updateMenuTexts();
	}

	updateWindowFont();
}

int Menu::storeData()
{
	ResourceManager* rm;	
	rm = ResourceManager::getInstance();
	rm->syncConfigureToDisk();
	clearMenuIndicator();
	return 0;
}

void Menu::onNewSelected(int oldSel, int newSel)
{
	MenuObj* menuObj = NULL;

	db_msg("onNewSelected\n");
	menuObj = getCurrentMenuObj();
	if(menuObj != NULL)
		menuObj->updateNewSelected(oldSel, newSel);
}

void Menu::onChangeFromOtherWindow(unsigned int fromId)
{
	enum AvaliableMenu menuObjId;
	// mAvailableMenuObjIds is used to determine which menuObj can be display in Menu
	mAvailableMenuObjIds.clear();

	switch(fromId) {
	case WINDOWID_RECORDPREVIEW:
		{
			if(mCdrMain->getCurrentRPWindowState() == STATUS_PHOTOGRAPH) {
				menuObjId = MENU_OBJ_PHOTO;
			} else {
				menuObjId = MENU_OBJ_RECORDPREVIEW;
			}
			mAvailableMenuObjIds.add(menuObjId);
		}
		break;
	default:
		menuObjId = MENU_OBJ_SYSTEM;
		break;
	}

	mAvailableMenuObjIds.add(MENU_OBJ_SYSTEM);

	initMenuIndicator();

	setMenuObjId(menuObjId);
}

void Menu::showAlphaEffect(void)
{
	RECT rect;
	HWND hMainWin;
	HDC secDC, MainWindowDC;

	hMainWin = GetMainWindowHandle(mHwnd);
	GetClientRect(hMainWin, &rect);
	secDC = GetSecondaryDC(hMainWin);
	MainWindowDC = GetDC(hMainWin);

	db_msg("rect.left is %d\n", rect.left);
	db_msg("rect.top is %d\n", rect.top);
	db_msg("rect.right is %d\n", rect.right);
	db_msg("rect.bottom is %d\n", rect.bottom);

	SetMemDCAlpha(secDC, MEMDC_FLAG_SRCALPHA, 180);
	ShowWindow(mHwnd, SW_HIDE);
	ShowWindow(mCdrMain->getWindowHandle(WINDOWID_STATUSBAR), SW_HIDE);

	BitBlt(secDC, 0, 0, RECTW(rect), RECTH(rect), MainWindowDC, 0, 0, 0);

	ReleaseDC(MainWindowDC);
	setSubMenuFlag(true);
}

void Menu::cancelAlphaEffect(void)
{
	RECT rect;
	HWND hMainWin;
	HDC secDC, MainWindowDC;

	hMainWin = GetMainWindowHandle(mHwnd);
	GetClientRect(hMainWin, &rect);
	secDC = GetSecondaryDC(hMainWin);
	MainWindowDC = GetDC(hMainWin);

	SetMemDCAlpha(secDC, 0, 0);
	BitBlt(secDC, 0, 0, RECTW(rect), RECTH(rect), secDC, 0, 0, 0);
	ShowWindow(mHwnd, SW_SHOWNORMAL);
	ShowWindow(mCdrMain->getWindowHandle(WINDOWID_STATUSBAR), SW_SHOWNORMAL);
	SetSecondaryDC(mHwnd, secDC, ON_UPDSECDC_DEFAULT);

	ReleaseDC(MainWindowDC);
	setSubMenuFlag(false);	//check whether to shutdown
}

// get MessageBox Data, except title and text
int Menu::getCommonMessageBoxData(MessageBox_t &messageBoxData)
{
	const char* ptr;
	ResourceManager* rm;

	db_msg("getMessageBoxData\n");
	rm = ResourceManager::getInstance();

	messageBoxData.dwStyle = MB_OKCANCEL | MB_HAVE_TITLE | MB_HAVE_TEXT;
	messageBoxData.flag_end_key = 1; /* end the dialog when endkey keyup */

	ptr = rm->getLabel(LANG_LABEL_SUBMENU_OK);
	if(ptr == NULL) {
		db_error("get ok string failed\n");
		return -1;
	}
	messageBoxData.buttonStr[0] = ptr;

	ptr = rm->getLabel(LANG_LABEL_SUBMENU_CANCEL);
	if(ptr == NULL) {
		db_error("get ok string failed\n");
		return -1;
	}
	messageBoxData.buttonStr[1] = ptr;

	messageBoxData.pLogFont = rm->getLogFont();

	rm->getResRect(ID_MENU_LIST_MB, messageBoxData.rect);
	messageBoxData.fgcWidget = rm->getResColor(ID_MENU_LIST_MB, COLOR_FGC);
	messageBoxData.bgcWidget = rm->getResColor(ID_MENU_LIST_MB, COLOR_BGC);
	messageBoxData.linecTitle = rm->getResColor(ID_MENU_LIST_MB, COLOR_LINEC_TITLE);
	messageBoxData.linecItem = rm->getResColor(ID_MENU_LIST_MB, COLOR_LINEC_ITEM);

	return 0;
}

int Menu::ShowMessageBox(MessageBox_t* data, bool alphaEffect)
{
	int retval;
	if(alphaEffect) {
		showAlphaEffect();
	}

	retval = showMessageBox(mHwnd, data);

	if(alphaEffect) {
		cancelAlphaEffect();
	}

	db_msg("retval is %d\n", retval);
	return retval;
}
// ++++++++++++++++++++++++ End of Menu Public interfaces ++++++++++++++++++++++++

// ===========================================================================================
Menu::MenuObj::MenuObj(class Menu* parent, unsigned int menulistCount, AvaliableMenu menuObjId) : 
	mParentContext(parent),
	mHwnd(HWND_INVALID),
	mOldProc(NULL),
	mMenuObjId(menuObjId),
	mItemImageArray(NULL),
	mValue1ImageArray(NULL),
	mMenuListCount(menulistCount),
	mMenuResourceID(NULL),
	mHaveCheckBox(NULL),
	mHaveSubMenu(NULL),
	mHaveValueString(NULL),
	mSubMenuContent0Cmd(NULL),
	mMlFlags(NULL)
{
	db_info("MenuObj Constructor, parent is 0x%p, menulist count is %d, menuObjId is %d\n", parent, mMenuListCount, mMenuObjId);
	mItemImageArray = new BITMAP[mMenuListCount];
	memset(mItemImageArray, 0, sizeof(BITMAP) * mMenuListCount);

	mValue1ImageArray = new BITMAP[mMenuListCount];
	memset(mValue1ImageArray, 0, sizeof(BITMAP) * mMenuListCount);

	HWND hParent;
	RECT parentRect, indicatorRect;

	memset(&mMenuListAttr, 0, sizeof(menuListAttr_t));
	hParent = mParentContext->getHwnd();
	
	GetClientRect(hParent, &parentRect);
	GetClientRect( GetDlgItem(hParent, ID_MENU_LIST_INDICATOR), &indicatorRect);
	getMenuListAttr(mMenuListAttr);
	mMenuListAttr.context = this;
	db_msg("context is 0x%p, this is 0x%p\n", mMenuListAttr.context, this);

	int x, y, w, h;
	x = 0;
	y = RECTH(indicatorRect);
	w = RECTW(parentRect);
	h = RECTH(parentRect) - RECTH(indicatorRect);
	mHwnd = CreateWindowEx(CTRL_CDRMenuList, "", 
			WS_CHILD  | WS_VSCROLL | LBS_USEBITMAP | LBS_HAVE_VALUE,
			WS_EX_NONE | WS_EX_USEPARENTFONT,
			IDC_MENULIST + mMenuObjId,
			x, y, w, h,
			hParent, (DWORD)&mMenuListAttr);
	if(mHwnd == HWND_INVALID) {
		db_error("create Menu List failed\n");
		return;
	}
	mOldProc = SetWindowCallbackProc(mHwnd, MenuObjsProc);
	db_msg("xxxxxxxxxxxxxxx\n");
}

Menu::MenuObj::~MenuObj()
{
	db_info("MenuObj Destructor\n");
	if(mItemImageArray != NULL) {
		for(unsigned int i = 0; i < mMenuListCount; i++) {
			if(mItemImageArray[i].bmBits != NULL) {
				UnloadBitmap(&mItemImageArray[i]);
			}
		}
		delete []mItemImageArray;
	}
	if(mValue1ImageArray != NULL) {
		for(unsigned int i = 0; i < mMenuListCount; i++) {
			if(mValue1ImageArray[i].bmBits != NULL) {
				UnloadBitmap(&mValue1ImageArray[i]);
			}
		}
		delete []mValue1ImageArray;
	}

	if(mUnfoldImageLight.bmBits != NULL) {
		UnloadBitmap(&mUnfoldImageLight);
	}

	if(mUnfoldImageDark.bmBits != NULL) {
		UnloadBitmap(&mUnfoldImageDark);
	}
}

// -------------------------- MenuObj public interfaces ----------------------------
int Menu::MenuObj::getMenuListAttr(menuListAttr_t &attr)
{
	ResourceManager* rm;

	rm = ResourceManager::getInstance();

	attr.normalBgc = rm->getResColor(ID_MENU_LIST, COLOR_BGC);
	attr.normalFgc = rm->getResColor(ID_MENU_LIST, COLOR_FGC);
	attr.linec = rm->getResColor(ID_MENU_LIST, COLOR_LINEC_ITEM);
	attr.normalStringc = rm->getResColor(ID_MENU_LIST, COLOR_STRINGC_NORMAL);
	attr.normalValuec = rm->getResColor(ID_MENU_LIST, COLOR_VALUEC_NORMAL);
	attr.selectedStringc = rm->getResColor(ID_MENU_LIST, COLOR_STRINGC_SELECTED);
	attr.selectedValuec = rm->getResColor(ID_MENU_LIST, COLOR_VALUEC_SELECTED);
	attr.scrollbarc = rm->getResColor(ID_MENU_LIST, COLOR_SCROLLBARC);

	attr.itemHeight = MENU_LIST_ITEM_HEIGHT;

	return 0;
}

int Menu::MenuObj::getItemImages(MENULISTITEMINFO *mlii)
{
	int retval;
	ResourceManager* rm;

	rm = ResourceManager::getInstance();

	if(mMenuResourceID == NULL) {
		db_error("mMenuResourceID is not initialized\n");
	}
	/* ++++++++++ load the itemImage bmp ++++++++++ */
	for(unsigned int i = 0; i < mMenuListCount; i++) {
		retval = rm->getResBmp(mMenuResourceID[i], BMPTYPE_UNSELECTED, mItemImageArray[i]);	
		if(retval < 0) {
			db_error("get bmp failed, i is %d, resID is %d\n", i, mMenuResourceID[i]);
			return -1;
		}
		mlii[i].hBmpImage = (DWORD)&mItemImageArray[i];
	}
	/* ---------- load the itemImageArray bmp ---------- */

	return 0;
}

int Menu::MenuObj::getItemStrings(MENULISTITEMINFO* mlii)
{
	ResourceManager* rm;

	rm = ResourceManager::getInstance();
	for(unsigned int i = 0; i < mMenuListCount; i++) {
		mlii[i].string = (char*)rm->getResMenuItemString(mMenuResourceID[i]);
		if(mlii[i].string == NULL) {
			db_error("get the %d label string failed\n", i);
			return -1;
		}
	}

	return 0;
}

int Menu::MenuObj::getFirstValueStrings(MENULISTITEMINFO* mlii, int menuIndex)
{
	const char* ptr;
	ResourceManager* rm;

	rm = ResourceManager::getInstance();

	db_msg("*******mMenuListCount:%d**********",mMenuListCount);
	if(menuIndex == -1) {
		for(unsigned int i = 0; i < mMenuListCount ; i++) {
			if(mHaveValueString[i] == 1) {
				ptr = rm->getResSubMenuItemString(mMenuResourceID[i], -1);
				if(ptr == NULL) {
					db_error("get ResSubMenuString %d failed\n", i);
					return -1;
				}
				mlii[ i ].hValue[0] = (DWORD)ptr;
			}
		}
	} else if(menuIndex >=0 && menuIndex < (int)mMenuListCount ){
		if(mHaveSubMenu[menuIndex]) {
			ptr = rm->getResSubMenuItemString(mMenuResourceID[menuIndex], -1);
			if(ptr == NULL) {
				db_error("get ResSubMenuString %d failed\n", menuIndex);
				return -1;
			}
			mlii->hValue[0] = (DWORD)ptr;
		}
	}

	if(mParentContext->getCurMenuObjId() == MENU_OBJ_SYSTEM){
		aw_cdr_tf_capacity capacity;
		StorageManager *sm = StorageManager::getInstance();
		if(sm->isMount()){
			static char capitly_buff[30]={0};
			memset(capitly_buff,0,sizeof(capitly_buff));
			sm->getCapacity(&(capacity.remain), &(capacity.total));
			sprintf(capitly_buff, "%1.1f/%1.1fG", capacity.remain/(float)1024,capacity.total/(float)1024);
			db_msg("~~~~~~~capitly_buff is %s~~~~~~~\n", capitly_buff);
			mlii[MENU_INDEX_TFCARD_INFO].hValue[0] = (DWORD)capitly_buff;
		}else{
			mlii[MENU_INDEX_TFCARD_INFO].hValue[0] = (DWORD)"0.0/0.0G";
		}
	}
	return 0;
}

int Menu::MenuObj::getFirstValueImages(MENULISTITEMINFO* mlii, int menuIndex)
{
	int retval;
	int current;
	ResourceManager* rm;
	HWND hMenuList;

	db_msg("************************MenuObj %d, getFirstValueImages menuIndex is %d\n", mMenuObjId, menuIndex);
	rm = ResourceManager::getInstance();

	if(menuIndex == -1) {
		for(unsigned int i = 0; i < mMenuListCount; i++) {
			if(mHaveCheckBox[i] == 1) {
				retval = rm->getResBmpSubMenuCheckbox( mMenuResourceID[i], false, mValue1ImageArray[i] );
				if(retval < 0) {
					db_error("get first value images failed, i is %d\n", i);
					return -1;
				}
				if(i==MENU_INDEX_FIRMWARE)
				{
					db_error("--------this is for test i is %d ---------\n", i);
					mlii[i].hValue[1] = (DWORD)&mValue1ImageArray[i];
				}else
				{
					mlii[i].hValue[0] = (DWORD)&mValue1ImageArray[i];
				}
				
			}
		}
	} else if(menuIndex >= 0 && menuIndex < (int)mMenuListCount ) {
		if(mHwnd && mHwnd != HWND_INVALID) {
			if(mHaveCheckBox[menuIndex] == 1) {
				hMenuList = mHwnd;
				current = SendMessage(hMenuList, LB_GETCURSEL, 0, 0);
				rm->unloadBitMap(mValue1ImageArray[menuIndex]);
				if(current == menuIndex) {
					retval = rm->getResBmpSubMenuCheckbox( mMenuResourceID[menuIndex], true, mValue1ImageArray[menuIndex] );
				} else
					retval = rm->getResBmpSubMenuCheckbox( mMenuResourceID[menuIndex], false, mValue1ImageArray[menuIndex] );
				if(retval < 0) {
					db_error("get first value images failed, menuIndex is %d\n", menuIndex);
					return -1;
				}
			}
		}
	}

	return 0;
}

int Menu::MenuObj::getSecondValueImages(MENULISTITEMINFO* mlii, MLFlags* mlFlags)
{
	int retval;
	ResourceManager* rm;

	rm = ResourceManager::getInstance();

	retval = rm->getResBmp(ID_MENU_LIST_UNFOLD_PIC, BMPTYPE_UNSELECTED, mUnfoldImageLight);
	if(retval < 0) {
		db_error("get secondValue image failed\n");
		return -1;
	}
	retval = rm->getResBmp(ID_MENU_LIST_UNFOLD_PIC, BMPTYPE_SELECTED, mUnfoldImageDark);
	if(retval < 0) {
		db_error("get secondValue image failed\n");
		return -1;
	}

	for(unsigned int i = 0; i < mMenuListCount; i++) {

		if (i==MENU_INDEX_FIRMWARE)
		{
				return 0;	
		}
		
		if(mlFlags[i].valueCount == 2) {
			mlii[i].hValue[1] = (DWORD)&mUnfoldImageLight;
		}
	}

	return 0;
}


/*
 * ----------------------------------------------------------------------------------
 * |   Image   |     itemString    |      first value			 |  second value    |
 * ----------------------------------------------------------------------------------
 * | itemImage | Example: 录像质量 | (sting or image) (optional) | image (optional) |
 * ----------------------------------------------------------------------------------
 * */
int Menu::MenuObj::createMenuListContents(HWND hMenuList)
{
	MENULISTITEMINFO menuListII[mMenuListCount];

	memset(&menuListII, 0, sizeof(menuListII));

	if(getItemImages(menuListII) < 0) {
		db_error("get item images failed\n");
		return -1;
	}
	if(getItemStrings(menuListII) < 0) {
		db_error("get item strings failed\n");
		return -1;
	}

	if(getFirstValueStrings(menuListII, -1) < 0) {
		db_error("get first value strings failed\n");
		return -1;
	}
	if(getFirstValueImages(menuListII, -1) < 0) {
		db_error("get first value images failed\n");
		return -1;
	}

	if(getSecondValueImages(menuListII, mMlFlags) < 0) {
		db_error("get second value images failed\n");
		return -1;
	}

	for(unsigned int i = 0; i < mMenuListCount; i++) {
		memcpy( &menuListII[i].flagsEX, &mMlFlags[i], sizeof(MLFlags) );
	}

	db_msg("xxxxxxxxxxxxxxxxxx\n");
	SendMessage(mHwnd, LB_MULTIADDITEM, mMenuListCount, (LPARAM)menuListII);
	db_msg("xxxxxxxxxxxxxxxxxx\n");
	SendMessage(mHwnd, MLM_HILIGHTED_SPACE, 8, 1);
	int screenW, screenH;
	getScreenInfo(&screenW,&screenH);
	SendMessage(mHwnd, LB_SETITEMHEIGHT, 0, (int)(screenH/6));
	db_msg("height is %d\n", SendMessage(mHwnd, LB_GETITEMHEIGHT, 0, 0));

	return 0;
}

int Menu::MenuObj::updateMenuTexts(void)
{
	int itemCount;
	HWND hMenuList;
	MENULISTITEMINFO menuListII[mMenuListCount];

	hMenuList = mHwnd;
	itemCount = SendMessage(hMenuList, LB_GETCOUNT, 0, 0);
	if(itemCount < 0) {
		db_error("get menu counts failed\n");
		return -1;
	}

	memset(menuListII, 0, sizeof(menuListII));
	for(int count = 0; count < itemCount; count++) {
		if( SendMessage(hMenuList, LB_GETITEMDATA, count, (LPARAM)&menuListII[count]) != LB_OKAY) {
			db_msg("get the %d item data failed\n", count);
			return -1;
		}
	}

	if(getItemStrings(menuListII) < 0) {
		db_error("get item strings failed\n");
		return -1;
	}
	if(getFirstValueStrings(menuListII, -1) < 0) {
		db_error("get item value strings failed\n");
		return -1;
	}

	db_msg("xxxxxxxxxx\n");
	for(int count = 0; count < itemCount; count++) {
		SendMessage(hMenuList, LB_SETITEMDATA, count, (LPARAM)&menuListII[count] );
		db_msg("xxxxxxxxxx\n");
	}

	return 0;
}

int Menu::MenuObj::updateSwitchIcons(void)
{
	int itemCount;
	HWND hMenuList;
	MENULISTITEMINFO menuListII[mMenuListCount];

	hMenuList = mHwnd;
	itemCount = SendMessage(hMenuList, LB_GETCOUNT, 0, 0);
	if(itemCount < 0) {
		db_error("get menu counts failed\n");
		return -1;
	}

	memset(menuListII, 0, sizeof(menuListII));
	for(int count = 0; count < itemCount; count++) {
		if( SendMessage(hMenuList, LB_GETITEMDATA, count, (LPARAM)&menuListII[count]) != LB_OKAY) {
			db_msg("get the %d item data failed\n", count);
			return -1;
		}
	}

	db_msg("xxxxxxxxxx\n");
	for(int count = 0; count < itemCount; count++) {
		getFirstValueImages(menuListII, count);
		SendMessage(hMenuList, LB_SETITEMDATA, count, (LPARAM)&menuListII[count] );
		db_msg("xxxxxxxxxx\n");
	}

	return 0;
}

void Menu::MenuObj::updateMenu()
{
	db_msg("MenuObj %d updateMenu\n", mMenuObjId);
	if(mHwnd == 0 || mHwnd == HWND_INVALID) {
		db_error("invalid window handle\n");
		return;
	}
	SendNotifyMessage(mHwnd, LB_SETCURSEL, 0, 0);

	updateMenuTexts();
	updateSwitchIcons();
}

int Menu::MenuObj::updateNewSelected(int oldSel, int newSel)
{
	HWND hMenuList;
	MENULISTITEMINFO mlii;
	ResourceManager* rm;
	int retval;

	db_msg("xxxxxoldSel is %d, newSel is %d\n", oldSel, newSel);

	if(oldSel >= (int)mMenuListCount || newSel >= (int)mMenuListCount)
		return -1;

	if(oldSel == newSel)
		return 0;

	if(mHwnd == 0 || mHwnd == HWND_INVALID)
		return -1;

	db_msg("xxxxxxxxxxx\n");

	rm = ResourceManager::getInstance();
	hMenuList = mHwnd;

	if(oldSel >= 0) {
		if(SendMessage(hMenuList, LB_GETITEMDATA, oldSel, (LPARAM)&mlii) != LB_OKAY) {
			db_error("get item info failed\n");
			return -1;
		}
		rm->unloadBitMap(mItemImageArray[oldSel]);
		retval = rm->getResBmp(mMenuResourceID[oldSel] ,BMPTYPE_UNSELECTED, mItemImageArray[oldSel]);
		if(retval < 0) {
			db_error("get item image failed, oldSel is %d\n", oldSel);
			return -1;
		}
		mlii.hBmpImage = (DWORD)&mItemImageArray[oldSel];

		if(mHaveCheckBox[oldSel] == 1) {
			rm->unloadBitMap(mValue1ImageArray[oldSel]);
			retval = rm->getResBmpSubMenuCheckbox(mMenuResourceID[oldSel], false, mValue1ImageArray[oldSel]);
			if(retval < 0) {
				db_error("get resBmp SubMenu Check box failed, oldSel is %d\n", oldSel);
				return -1;
			}
			if(oldSel == MENU_INDEX_FIRMWARE)
			{
				db_msg("---------- this is jason_yin test the oldsel is  1 %d----------------\n",oldSel);
				mlii.hValue[1] = (DWORD)&mValue1ImageArray[oldSel];
			}else
			{
				db_msg("---------- this is jason_yin test the oldsel is  2  %d----------------\n",oldSel);
				mlii.hValue[0] = (DWORD)&mValue1ImageArray[oldSel];
			}
			
		} else if(mlii.flagsEX.valueCount == 2) {
			mlii.hValue[1] = (DWORD)&mUnfoldImageLight;
		}

		db_msg("xxxxxxxxx\n");
		SendMessage(hMenuList, LB_SETITEMDATA, oldSel, (LPARAM)&mlii);
		db_msg("xxxxxxxxx\n");
	}

	if(newSel >= 0) {
		if(SendMessage(hMenuList, LB_GETITEMDATA, newSel, (LPARAM)&mlii) != LB_OKAY) {
			db_error("get item info failed\n");
			return -1;
		}
		rm->unloadBitMap(mItemImageArray[newSel]);
		retval = rm->getResBmp(mMenuResourceID[newSel],BMPTYPE_SELECTED, mItemImageArray[newSel]);
		if(retval < 0) {
			db_error("get item image failed, oldSel is %d\n", oldSel);
			return -1;
		}
		mlii.hBmpImage = (DWORD)&mItemImageArray[newSel];

		if(mHaveCheckBox[newSel] == 1) {
			rm->unloadBitMap(mValue1ImageArray[newSel]);
			retval = rm->getResBmpSubMenuCheckbox(mMenuResourceID[newSel], true, mValue1ImageArray[newSel]);
			if(retval < 0) {
				db_error("get resBmp SubMenu Check box failed, oldSel is %d\n", oldSel);
				return -1;
			}
			if(newSel == MENU_INDEX_FIRMWARE)
			{
				db_msg("---------- this is jason_yin test the oldsel is  1 %d----------------\n",oldSel);
				mlii.hValue[1] = (DWORD)&mValue1ImageArray[newSel];
			}else
			{
				db_msg("---------- this is jason_yin test the oldsel is  2  %d----------------\n",oldSel);
				mlii.hValue[0] = (DWORD)&mValue1ImageArray[newSel];
			}
			
		} else if(mlii.flagsEX.valueCount == 2) {
			mlii.hValue[1] = (DWORD)&mUnfoldImageDark;
		}

		SendMessage(hMenuList, LB_SETITEMDATA, newSel, (LPARAM)&mlii);
	}

	return 0;
}

int Menu::MenuObj::getSubMenuData(unsigned int subMenuIndex, subMenuData_t &subMenuData)
{
	int retval;
	int contentCount;
	const char* ptr;
	ResourceManager* rm;

	if(mHaveSubMenu[subMenuIndex] != 1) {
		db_error("invalid index %d\n", subMenuIndex);
		return -1;
	}

	rm = ResourceManager::getInstance();

	retval = rm->getResRect(ID_SUBMENU, subMenuData.rect);
	if(retval < 0) {
		db_error("get subMenu rect failed\n");
		return -1;
	}
	retval = rm->getResBmp(ID_SUBMENU_CHOICE_PIC, BMPTYPE_BASE, subMenuData.BmpChoice);
	if(retval < 0) {
		db_error("get subMenu choice bmp failed\n");
		return -1;
	}

	getMenuListAttr(subMenuData.menuListAttr);
	subMenuData.lincTitle = rm->getResColor(ID_SUBMENU, COLOR_LINEC_TITLE);
	subMenuData.pLogFont = rm->getLogFont();

	/* current submenu index */
	retval = rm->getResIntValue(mMenuResourceID[subMenuIndex], INTVAL_SUBMENU_INDEX );
	if(retval < 0) {
		db_error("get res submenu index failed\n");
		return -1;
	}
	subMenuData.selectedIndex = retval;

	/* submenu count */
	contentCount = rm->getResIntValue(mMenuResourceID[subMenuIndex], INTVAL_SUBMENU_COUNT);

	/* submenu title */
	ptr = rm->getResSubMenuTitle(mMenuResourceID[subMenuIndex]);
	if(ptr == NULL) {
		db_error("get subMenu title failed\n");
		return -1;
	}
	subMenuData.title = ptr;

	/* submenu contents */
	for(int i = 0; i < contentCount; i++) {
		ptr = rm->getResSubMenuItemString(mMenuResourceID[subMenuIndex], i);
		if(ptr == NULL) {
			db_error("get submenu content %d failed", i);
			return -1;
		}
		subMenuData.contents.add(String8(ptr));
	}

	return 0;
}

int Menu::MenuObj::ShowSubMenu(unsigned int menuIndex, bool &isModified)
{
	int retval;
	int oldSel;
	subMenuData_t subMenuData;

	if(getSubMenuData(menuIndex, subMenuData) < 0) {
		db_error("get submenu data failed\n");
		return -1;
	}
	oldSel = subMenuData.selectedIndex;
    subMenuData.menu_index = menuIndex;
	mParentContext->showAlphaEffect();
	retval = showSubMenu(mHwnd, &subMenuData);
	db_msg("retval = %d\n", retval);
	mParentContext->cancelAlphaEffect();
	if(retval >= 0) {
		if(retval != oldSel) {
			db_msg("change from %d to %d\n", oldSel, retval);
			isModified = true;
		}
	}

	return retval;
}

int Menu::MenuObj::getCheckBoxStatus(unsigned int menuIndex, bool &isChecked)
{
	ResourceManager* rm;
	bool curStatus;

	rm = ResourceManager::getInstance();
	if(menuIndex >= mMenuListCount )
		return -1;
	if(mHaveCheckBox[menuIndex] != 1) {
		db_error("menuIndex: %d, don not have checkbox\n", menuIndex);
		return -1;
	}

	curStatus = rm->getResBoolValue(mMenuResourceID[menuIndex]);

	isChecked = ( (curStatus == true) ? false : true );

	return 0;
}
// ++++++++++++++++++++++++++ End of MenuObj public interfaces +++++++++++++++++++++++++

CdrMain * Menu::getCdrMain()
{
	if(mCdrMain)
		return mCdrMain;
	return NULL;
}


int Menu::hand_message(unsigned int menuIndex,bool newSel)
{
	MenuSystem* menuSystem = NULL;
	menuSystem = (MenuSystem*)getMenuObj(MENU_OBJ_SYSTEM);
	menuSystem->HandleSubMenuChange(menuIndex, newSel);
	return 0;
}

void Menu::Card_speed_disp(TestResult *test_result )
{
	MenuSystem* menuSystem = NULL;
	menuSystem = (MenuSystem*)getMenuObj(MENU_OBJ_SYSTEM);
	menuSystem->card_speed_test(test_result);
}

int Menu::Card_speed_disp_lable(void)
{
	CloseTipLabel();
	return 0;
}


void Menu::formatTfCardOutSide(bool param)
{
	int retval;
	MenuSystem* menuSystem = NULL;
	MessageBox_t messageBoxData;
	ResourceManager* rm;

	rm = ResourceManager::getInstance();
	formatTf = 0;
	if(isFormatTimer == 0){
		SetTimer(mHwnd,FORMATTF_TIMER,100);
		isFormatTimer = -1;
	}

	if(param){
		SendMessage(mCdrMain->getWindowHandle(WINDOWID_RECORDPREVIEW),MSG_ADAS_SCREEN,0,0);
	}

	BroadcastMessage(MSG_CLOSE_TIP_LABEL, 0, 0);

	menuSystem = (MenuSystem*)getMenuObj(MENU_OBJ_SYSTEM);
	if(menuSystem != NULL) {
		memset(&messageBoxData, 0, sizeof(MessageBox_t));
		getCommonMessageBoxData(messageBoxData);
		messageBoxData.title = rm->getLabel(LANG_LABEL_SUBMENU_FORMAT_TITLE);
		messageBoxData.text = rm->getLabel(LANG_LABEL_FORMAT_SDCARD);
		retval = menuSystem->formatTfCard(&messageBoxData, false);
		if(retval == 0) {
			mCdrMain->setNeedCheckSD(false);
		}
	}

	if(param && ! mCdrMain->isShuttingdown() ) {
		SendMessage(mCdrMain->getWindowHandle(WINDOWID_RECORDPREVIEW),MSG_ADAS_SCREEN,1,0);
	}
}

void Menu::focusCurrentMenuObj(void)
{
	class MenuObj* menuObj;

	menuObj = getCurrentMenuObj();
	if(menuObj == NULL) {
		return;
	}

	SetFocusChild(menuObj->getHwnd());
}

int Menu::getCurMenuObjId()
{
	return mCurMenuObjId;
}

int Menu::releaseResource()
{
	return 0;
}

void Menu::setSubMenuFlag(bool flag)
{
	if (!flag) {
		mCdrMain->checkShutdown();
	}
	mSubMenu = flag;
}

bool Menu::getSubMenuFlag()
{
	return mSubMenu;
}
