
#include "MenuInheritors.h"
#include "MenuSystem.h"
#undef LOG_TAG
#define LOG_TAG "MenuSystem"
#include "debug.h"

using namespace android;

int MenuSystem::menuObjWindowProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
	//db_msg("message code: [%d], %s, wParam=%d, lParam=0x%lx\n", message, Message2Str(message), wParam, lParam);

	return WANT_SYSTEM_PROCESS;
}


int MenuSystem::keyProc(int keyCode, int isLongPress)
{
	switch(keyCode) {
	case CDR_KEY_OK:
		{
			unsigned int selectedItem;
			int newSel;
			bool isModified, isChecked;
			ResourceManager* rm = ResourceManager::getInstance();
			if(isLongPress == SHORT_PRESS) {
				selectedItem = SendMessage(mHwnd, LB_GETCURSEL, 0, 0);
				db_msg("selectedItem is %d\n", selectedItem);
				switch(selectedItem) {
				case MENU_INDEX_PARK:		case MENU_INDEX_GSENSOR:
				case MENU_INDEX_VOICEVOL:	case MENU_INDEX_LIGHT_FREQ:
				case MENU_INDEX_LANG:		case MENU_INDEX_SS:		
				case MENU_INDEX_DELAY_SHUTDOWN:  case MENU_INDEX_TFCARD_INFO:
					if( ( newSel = ShowSubMenu(selectedItem, isModified) ) < 0) {
						db_error("show submenu %d failed\n", selectedItem);
						break;
					}
					if(isModified == true)
					HandleSubMenuChange(selectedItem, newSel);
					break;
				case MENU_INDEX_RECORD_SOUND:		case MENU_INDEX_KEYTONE:
				case MENU_INDEX_STANDBY_MODE:
#ifdef APP_VERSION
				case MENU_INDEX_WIFI:
				case MENU_INDEX_ALIGNLINE:
#endif

					if(getCheckBoxStatus(selectedItem, isChecked) < 0) {
						db_error("get check box status failed\n");
						break;
					}
					HandleSubMenuChange(selectedItem, isChecked);
					break;
				case MENU_INDEX_FIRMWARE:{
					if(getCheckBoxStatus(selectedItem, isChecked) < 0) {
						db_error("get check box status failed\n");
						break;
					}
					db_msg("---ischeck:%d--",isChecked);
					bool flag= rm->getResBoolValue(ID_MENU_LIST_FIRMWARE);
					db_msg("---flag:%d--",flag);
					if(flag == true)
					{
						db_msg("@@@@@@@@@@@@@isChecked is true@@@@@@@@@\n");
						//SendMessage(rm->getHwnd(WINDOWID_MAIN), MSG_OTA_UPDATE, 0, 0);
						card_ota_upgrade();
						break;
					}
					}
					break;
				case MENU_INDEX_DATE:
					ShowDateDialog();
					break;
				case MENU_INDEX_FORMAT:
				{
					#ifdef ASYNC_FORMAT
					if(!isFormat){
						int ret = formatTfCard(NULL, true);
					}else{
						ShowTipLabel(mHwnd, LABEL_FORMATING_TFCARD, true,2000,true);
					}
					#else
					int ret = formatTfCard(NULL, true);
					#ifdef FATFS
						StorageManager * sm = StorageManager::getInstance();
						sm->setFATFSMountValue(true);
					#endif
					#endif
				}
					break;
				case MENU_INDEX_FRESET:
					factoryReset();
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

MenuSystem::MenuSystem(class Menu* parent, AvaliableMenu menuObjId) : 
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
	isFormat = false;
}

MenuSystem::~MenuSystem()
{
	db_info("MenuSystem Destructor\n");
}

// --------------------- private implements --------------------
// must be called from the MenuCloudDog Constructor
void MenuSystem::initMenuObjParams()
{
	db_msg("initMenuObjParams\n");

	mMenuResourceID = menuResourceID;
	mHaveCheckBox	= haveCheckBox;
	mHaveSubMenu	= haveSubMenu;
	mHaveValueString	= haveValueString;
	mSubMenuContent0Cmd = subMenuContent0Cmd;
	mMlFlags = mlFlags;
}

int MenuSystem::getMessageBoxData(unsigned int menuIndex, MessageBox_t &messageBoxData)
{
	const char* ptr;
	ResourceManager* rm;
	enum LANG_STRINGS titleId, textId;

	switch(menuIndex) {
	case MENU_INDEX_FORMAT:
		titleId = LANG_LABEL_SUBMENU_FORMAT_TITLE;
		textId = LANG_LABEL_SUBMENU_FORMAT_TEXT;
		break;
	case MENU_INDEX_FRESET:
		titleId = LANG_LABEL_SUBMENU_FRESET_TITLE;
		textId = LANG_LABEL_SUBMENU_FRESET_TEXT;
		break;
	case MENU_INDEX_FIRMWARE:
		titleId = LANG_LABEL_TIPS;
		textId = LANG_LABEL_OTA_UPDATE;
		break;
	default:
		db_error("invalid menuIndex: %d\n", menuIndex);
		return -1;
	}

	rm = ResourceManager::getInstance();

	ptr = rm->getLabel(titleId);
	if(ptr == NULL) {
		db_error("get title failed, titleId is %d\n", menuIndex);
		return -1;
	}
	messageBoxData.title = ptr;
	ptr = rm->getLabel(textId);
	if(ptr == NULL) {
		db_error("get title failed, textId is %d\n", menuIndex);
		return -1;
	}
	messageBoxData.text = ptr;

	return mParentContext->getCommonMessageBoxData(messageBoxData);
}

static void formatCallback(HWND hDlg, void* data)
{
	MenuSystem* menuSystem;

	menuSystem = (MenuSystem*)data;
	db_msg("formatMessageBoxCallback\n");

	menuSystem->doFormatTFCard();
	db_msg("formatMessageBoxCallback finish\n");
	EndDialog(hDlg, IDC_BUTTON_OK);
}




int MenuSystem::get_Card_Speed_Data_Box(unsigned int menuIndex, MessageBox_t &messageBoxData,TestResult *test_result)
{
	char *ptr;
	static char  str[128]={0};
	ResourceManager* rm;
	enum LANG_STRINGS titleId, textId;
	static char buff[50]={0};
	memset(buff,0,sizeof(buff));	
	memset(str,0,sizeof(str));
	db_msg("[debug_jaosn]:the avali is :%2.1f  the total is :%2.1f \n",test_result->mReadSpeed,test_result->mWriteSpeed);
	sprintf(buff, "%2.1lfM/s/%2.1lfM/s", test_result->mReadSpeed/(float)1024,test_result->mWriteSpeed/(float)1024);	
	db_msg("----the buff is %s-------",buff);
	switch(menuIndex) {
	case MENU_INDEX_TFCARD_INFO:
		titleId = LANG_LABEL_SUBMENU_TF_CARD_CONTENT2;
		textId = LANG_LABEL_CARD_READ_WRITE;
		break;
	default:
		db_error("invalid menuIndex: %d\n", menuIndex);
		return -1;
	}

	rm = ResourceManager::getInstance();

	ptr = (char *)rm->getLabel(titleId);
	if(ptr == NULL) {
		db_error("get title failed, titleId is %d\n", menuIndex);
		return -1;
	}
	messageBoxData.title = ptr;
	ptr = (char *)rm->getLabel(textId);
	if(ptr == NULL) {
		db_error("get title failed, textId is %d\n", menuIndex);
		return -1;
	}
	strcpy(str,ptr);
	strcat(str,buff);
	messageBoxData.text = str;
	return mParentContext->getCommonMessageBoxData(messageBoxData);
}

int MenuSystem::ShowFormattingTip(bool alphaEffect)
{
	tipLabelData_t tipLabelData;
	ResourceManager* rm;
	int retval;

	rm = ResourceManager::getInstance();
	memset(&tipLabelData, 0, sizeof(tipLabelData));
	if(getTipLabelData(&tipLabelData) < 0) {
		db_error("get TipLabel data failed\n");	
		return -1;
	}

	tipLabelData.title = rm->getLabel(LANG_LABEL_TIPS);
	if(!tipLabelData.title) {
		db_error("get FormattingTip titile failed\n");
		return -1;
	}

	tipLabelData.text = rm->getLabel(LANG_LABEL_SUBMENU_FORMATTING_TEXT);
	if(!tipLabelData.text) {
		db_error("get LANG_LABEL_LOW_POWER_TEXT failed\n");
		return -1;
	}

	tipLabelData.timeoutMs = TIME_INFINITE;
	tipLabelData.disableKeyEvent = true;
	tipLabelData.callback = formatCallback;
	tipLabelData.callbackData = this;

	if(alphaEffect)
		mParentContext->showAlphaEffect();

	retval = showTipLabel(mHwnd, &tipLabelData);

	if(alphaEffect)
		mParentContext->cancelAlphaEffect();

	return retval;
}
// +++++++++++++++++++++ End of private implements +++++++++++++++++++

// ----------------------------- public interface --------------------
int MenuSystem::HandleSubMenuChange(unsigned int menuIndex, int newSel)
{
	ResourceManager* rm;

	rm = ResourceManager::getInstance();

	switch(menuIndex) {
	case MENU_INDEX_PARK:	
	case MENU_INDEX_DELAY_SHUTDOWN:
	case MENU_INDEX_GSENSOR:	
	case MENU_INDEX_VOICEVOL:
	case MENU_INDEX_LIGHT_FREQ:
	case MENU_INDEX_LANG:
	case MENU_INDEX_SS:	
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

	HWND hMenuList;
	MENULISTITEMINFO mlii;
	hMenuList = mHwnd;

	if(menuIndex != MENU_INDEX_LANG) {
		if(SendMessage(hMenuList, LB_GETITEMDATA, menuIndex, (LPARAM)&mlii) != LB_OKAY) {
			db_error("get item info failed, menuIndex is %d\n", menuIndex);
			return -1;
		}

		if(getFirstValueStrings(&mlii, menuIndex) < 0) {
			db_error("get first value strings failed\n");
			return -1;
		}
		db_msg("xxxxxxxx\n");
		SendMessage(hMenuList, LB_SETITEMDATA, menuIndex, (LPARAM)&mlii);
		db_msg("xxxxxxxx\n");
	}

	return 0;
}

int MenuSystem::HandleSubMenuChange(unsigned int menuIndex, bool newValue)
{
	ResourceManager* rm;

	db_msg("menuObj %d, set menuIndex %s\n", mMenuObjId, newValue ? "true" : "false");
	rm = ResourceManager::getInstance();
	switch(menuIndex) {
	case MENU_INDEX_RECORD_SOUND:
	case MENU_INDEX_KEYTONE:
	case MENU_INDEX_STANDBY_MODE:
#ifdef APP_VERSION
	case MENU_INDEX_WIFI:
	case MENU_INDEX_ALIGNLINE:
#endif
    case MENU_INDEX_FIRMWARE:
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

int MenuSystem::ShowDateDialog(void)
{
	int retval;
	ResourceManager* rm;
	dateSettingData_t configData;

	rm = ResourceManager::getInstance();

	configData.title = rm->getLabel(LANG_LABEL_DATE_TITLE);
	configData.year = rm->getLabel(LANG_LABEL_DATE_YEAR);
	configData.month = rm->getLabel(LANG_LABEL_DATE_MONTH);
	configData.day = NULL ;//rm->getLabel(LANG_LABEL_DATE_DAY);
	configData.hour = rm->getLabel(LANG_LABEL_DATE_HOUR);
	configData.minute = rm->getLabel(LANG_LABEL_DATE_MINUTE);
	configData.second = NULL ;//rm->getLabel(LANG_LABEL_DATE_SECOND);
	if(configData.title == NULL)
		return -1;
	if(configData.year == NULL)
		return -1;
	if(configData.month == NULL)
		return -1;
	if(configData.day == NULL)
		//return -1;
	if(configData.hour == NULL)
		return -1;
	if(configData.minute == NULL)
		return -1;
	if(configData.second == NULL)
		//return -1;

	retval = rm->getResRect(ID_MENU_LIST_DATE, configData.rect);
	if(retval < 0) {
		db_error("get date rect failed\n");
		return -1;
	}

	configData.titleRectH = rm->getResIntValue(ID_MENU_LIST_DATE, INTVAL_TITLEHEIGHT);
	configData.hBorder	= rm->getResIntValue(ID_MENU_LIST_DATE, INTVAL_HBORDER);
	configData.yearW	= rm->getResIntValue(ID_MENU_LIST_DATE, INTVAL_YEARWIDTH);
	configData.numberW	= rm->getResIntValue(ID_MENU_LIST_DATE, INTVAL_NUMBERWIDTH);
	configData.dateLabelW = rm->getResIntValue(ID_MENU_LIST_DATE, INTVAL_LABELWIDTH);
	configData.boxH	= rm->getResIntValue(ID_MENU_LIST_DATE, INTVAL_BOXHEIGHT);
	if(configData.titleRectH < 0)
		return -1;
	if(configData.hBorder < 0)
		return -1;
	if(configData.yearW < 0)
		return -1;
	if(configData.numberW < 0)
		return -1;
	if(configData.dateLabelW < 0)
		return -1;
	if(configData.boxH < 0)
		return -1;

	configData.bgc_widget = rm->getResColor(ID_MENU_LIST_DATE, COLOR_BGC);
	configData.fgc_label = rm->getResColor(ID_MENU_LIST_DATE, COLOR_FGC_LABEL);
	configData.fgc_number = rm->getResColor(ID_MENU_LIST_DATE, COLOR_FGC_NUMBER);
	configData.linec_title = rm->getResColor(ID_MENU_LIST_DATE, COLOR_LINEC_TITLE);
	configData.borderc_selected = rm->getResColor(ID_MENU_LIST_DATE, COLOR_BORDERC_SELECTED);
	configData.borderc_normal = rm->getResColor(ID_MENU_LIST_DATE, COLOR_BORDERC_NORMAL);
	configData.pLogFont = rm->getLogFont();

	mParentContext->showAlphaEffect();
	retval = dateSettingDialog(mHwnd, &configData);
	mParentContext->cancelAlphaEffect();
	return retval;
}

void MenuSystem::finish(int what, int extra)
{
	CdrMain* cdrMain = mParentContext->getCdrMain();

	db_error("-----formatfinish!|n");
	cdrMain->forbidRecord(FORBID_NORMAL);
	#ifdef ASYNC_FORMAT
	ResourceManager * rm = ResourceManager::getInstance();
	if(cdrMain->getCurWindowId()!= WINDOWID_MENU){
		SendMessage(rm->getHwnd(WINDOWID_MENU), MSG_FORMAT_TFCARD_FINISH, 0, 0);
	}
	isFormat = false;
	#endif
}

// if messageBox data is NULL, thell use the default messageBox data
int MenuSystem::formatTfCard(MessageBox_t* data, bool alphaEffect)
{
	StorageManager* sm;
	MessageBox_t messageBoxData;
	int retval;

	sm = StorageManager::getInstance();

	if (!sm->isInsert()) {
		mParentContext->showAlphaEffect();
		ShowTipLabel(mHwnd, LABEL_NO_TFCARD);
		mParentContext->cancelAlphaEffect();
		return -1;
	}

	memset(&messageBoxData, 0, sizeof(MessageBox_t));
	if(data == NULL) {
		getMessageBoxData(MENU_INDEX_FORMAT, messageBoxData);
		retval = mParentContext->ShowMessageBox(&messageBoxData, alphaEffect);
	} else {
		retval = mParentContext->ShowMessageBox(data, alphaEffect);
	}

	if(retval < 0) {
		db_error("ShowMessageBox err, retval is %d\n", retval);
		return -1;
	}
	
	if(retval == IDC_BUTTON_OK) {
		if(sm->isInsert() == false) {
			db_msg("tfcard not mount\n");
			ShowTipLabel(mHwnd, LABEL_NO_TFCARD);
			return -1;
		}
		#ifdef ASYNC_FORMAT
		ResourceManager * rm = ResourceManager::getInstance();
		isFormat = true;
		SendNotifyMessage(rm->getHwnd(WINDOWID_MENU), MSG_FORMAT_TFCARD_NOW, 0, 0);
		#else
		ShowFormattingTip(alphaEffect);
		#endif
		return 0;
	}

	return -1;
}

void MenuSystem::card_ota_upgrade()
{
	StorageManager* sm;
	MessageBox_t messageBoxData;
	int retval;

	sm = StorageManager::getInstance();

	memset(&messageBoxData, 0, sizeof(MessageBox_t));
	getMessageBoxData(MENU_INDEX_FIRMWARE, messageBoxData);
	retval = mParentContext->ShowMessageBox(&messageBoxData, true);
	if(retval < 0) {
		db_error("ShowMessageBox err, retval is %d\n", retval);
		return;
	}
	
	if(retval == IDC_BUTTON_OK) {
		//doFactoryReset();
		 //otaUpdate();
		 ResourceManager *rm = ResourceManager::getInstance();
		SendMessage(rm->getHwnd(WINDOWID_MAIN), MSG_OTA_UPDATE, 0, 0);
	}
}

void MenuSystem::factoryReset()
{
	StorageManager* sm;
	MessageBox_t messageBoxData;
	int retval;

	sm = StorageManager::getInstance();

	memset(&messageBoxData, 0, sizeof(MessageBox_t));
	getMessageBoxData(MENU_INDEX_FRESET, messageBoxData);
	retval = mParentContext->ShowMessageBox(&messageBoxData, true);
	if(retval < 0) {
		db_error("ShowMessageBox err, retval is %d\n", retval);
		return;
	}
	
	if(retval == IDC_BUTTON_OK) {
		doFactoryReset();
	}
}


int MenuSystem::card_speed_test(TestResult *test_result)
{
	StorageManager* sm;
	int retval;
	sm = StorageManager::getInstance();

	if (!sm->isInsert()) {
		mParentContext->showAlphaEffect();
		ShowTipLabel(mHwnd, LABEL_NO_TFCARD);
		mParentContext->cancelAlphaEffect();
		return -1;
	}

	static char buff[20]={0};
	memset(buff,0,sizeof(buff));
	db_msg("[debug_jaosn]:the avali is :%2.1f  the total is :%2.1f \n",test_result->mReadSpeed,test_result->mWriteSpeed);
	sprintf(buff, "%2.1lfM/s/%2.1lfM/s", test_result->mReadSpeed/(float)1024,test_result->mWriteSpeed/(float)1024);	
	ShowTipLabel(mHwnd, (enum labelIndex)LABEL_SUBMENU_TF_CARD_CONTENT2, TRUE, 3000,false, buff);
	return 0;
}

void MenuSystem::doFormatTFCard(void)
{
	StorageManager *sm = StorageManager::getInstance();
	CdrMain* cdrMain = mParentContext->getCdrMain();

	db_msg("do format TF card\n");
	sm->format(this);
	cdrMain->forbidRecord(FORBID_FORMAT);
	#ifndef ASYNC_FORMAT
	finish(0, 0);
	#endif
}

void MenuSystem::doFactoryReset(void)
{
	db_msg("do factory reset\n");
	ResourceManager *rm = ResourceManager::getInstance();
	rm->resetResource();
	//resetDateTime();
	updateMenuTexts();
	updateSwitchIcons();
}
