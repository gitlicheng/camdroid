
#include <utils/Thread.h>
#include "ResourceManager.h"
#include "cdr_message.h"
#include "Dialog.h"
#include "StorageManager.h"
#undef LOG_TAG
#define LOG_TAG "Dialog.cpp"
#include "debug.h"

using namespace android;

typedef struct {
	enum LANG_STRINGS TitleCmd;
	enum LANG_STRINGS TextCmd;
}labelStringCmd_t;

labelStringCmd_t labelStringCmd[] = {
	{LANG_LABEL_TIPS,				LANG_LABEL_NO_TFCARD},
	{LANG_LABEL_WARNING,			LANG_LABEL_TFCARD_FULL},
	{LANG_LABEL_WARNING,			LANG_LABEL_PLAYBACK_FAIL},
	{LANG_LABEL_SHUTDOWN_TITLE,		LANG_LABEL_LOW_POWER_SHUTDOWN},
	{LANG_LABEL_SHUTDOWN_TITLE,		LANG_LABEL_10S_SHUTDOWN},
	{LANG_LABEL_SHUTDOWN_TITLE,		LANG_LABEL_SHUTDOWN_NOW},
	{LANG_LABEL_SHUTDOWN_TITLE,		LANG_LABEL_30S_NOWORK_SHUTDOWN},
	{LANG_LABEL_TIPS,				LANG_LABEL_FILELIST_EMPTY},
	{LANG_LABEL_TIPS,				LANG_LABEL_WIFI_CONNECT},
	{LANG_LABEL_IMPACT_NUM,			LANG_LABEL_IMPACT_NUM},
	{LANG_LABEL_TIPS,				LANG_LABEL_OTA_UPDATE},
	{LANG_LABEL_TIPS,				LANG_LABEL_FILE_LOCKED},
	{LANG_LABEL_TIPS,				LANG_LABEL_LOCKED_FILE},
	{LANG_LABEL_TIPS,				LANG_LABEL_UNLOCK_FILE},
	{LANG_LABEL_TIPS,				LANG_LABEL_INSERTED_SDCARD},
	{LANG_LABEL_TIPS,				LANG_LABEL_EXTRACT_SDCARD},
	{LANG_LABEL_TIPS,				LANG_LABEL_CARD_SPEED_TEST},
	{LANG_LABEL_SHUTDOWN_TITLE,		LANG_LABEL_15S_SHUTDOWN},
	{LANG_LABEL_SHUTDOWN_TITLE,		LANG_LABEL_30S_SHUTDOWN},
	{LANG_LABEL_SHUTDOWN_TITLE,		LANG_LABEL_60S_SHUTDOWN},
	{LANG_LABEL_SHUTDOWN_TITLE,		LANG_LABEL_CANCEL_SHUTDOWN},
	{LANG_LABEL_TIPS,				LANG_LABEL_SOS_WARNING},
	{LANG_LABEL_SUBMENU_TF_CARD_CONTENT2,	LANG_LABEL_SUBMENU_TF_CARD_CONTENT2},
	{LANG_LABEL_TIPS,				LANG_LABEL_FORMAT_TFCARD_FINISH},
	{LANG_LABEL_TIPS,				LANG_LABEL_FORMATING_TFCARD},
	{LANG_LABEL_TIPS,				LANG_LABEL_INVALID_TFCARD},
};

int getTipLabelData(tipLabelData_t* tipLabelData)
{
	int retval;
	ResourceManager* rm;

	rm = ResourceManager::getInstance();
	retval = rm->getResRect(ID_TIPLABEL, tipLabelData->rect);
	if(retval < 0) {
		db_error("get tiplabel rect failed\n");	
		return -1;
	}
	retval = rm->getResIntValue(ID_TIPLABEL, INTVAL_TITLEHEIGHT);
	if(retval < 0) {
		db_error("get tiplabel rect failed\n");	
		return -1;
	}
	tipLabelData->titleHeight = retval;
	db_msg("titleHeight is %d\n", tipLabelData->titleHeight);

	tipLabelData->bgc_widget =	rm->getResColor(ID_TIPLABEL, COLOR_BGC);
	tipLabelData->fgc_widget	=	rm->getResColor(ID_TIPLABEL, COLOR_FGC);
	tipLabelData->linec_title =	rm->getResColor(ID_TIPLABEL, COLOR_LINEC_TITLE);

	tipLabelData->pLogFont = rm->getLogFont();

	return 0;
}

/*
 *  if endLabelKeyUp is true, then end the label while any key up
 *  if endLabelKeyUp is flase, then end the label while any key down
 * */
int ShowTipLabel(HWND hParent, enum labelIndex index, bool endLabelKeyUp, unsigned int timeoutMs, bool hasicon, char *ptext)
{
	db_msg("<***ShowTipLabel ***>, labelIndex %d\n", index);
	BroadcastMessage(MSG_KEEP_ADAS_SCREEN_FLAG,0,0);

	tipLabelData_t tipLabelData;
	ResourceManager* rm;

	if(index >= TABLESIZE(labelStringCmd)) {
		db_error("invalid index: %d\n", index);
		return -1;
	}

	rm = ResourceManager::getInstance();
	memset(&tipLabelData, 0, sizeof(tipLabelData) );

	if(getTipLabelData(&tipLabelData) < 0) {
		db_error("get tipLabelData failed\n");	
		return -1;
	}

	tipLabelData.hasicon = hasicon;
	tipLabelData.title = rm->getLabel(labelStringCmd[index].TitleCmd);
	if(!tipLabelData.title) {
		db_error("get title failed, TitleCmd %d\n", labelStringCmd[index].TitleCmd);
		return -1;
	}

	if(!strcmp(tipLabelData.title,rm->getLabel(LANG_LABEL_IMPACT_NUM))){
		StorageManager *sm = StorageManager::getInstance();
		char p[256]={0};
		sm->readImpactTimeFile(p);
		tipLabelData.text = p;
		tipLabelData.timeoutMs = 5 * 1000;
		remove(GENE_IMPACT_TIME_FILE);
		if(strlen(p)>70)
			return showTipLabel(hParent, &tipLabelData,1);
		else
			return showTipLabel(hParent, &tipLabelData);
	}else{
		tipLabelData.text = rm->getLabel(labelStringCmd[index].TextCmd);
		tipLabelData.timeoutMs = timeoutMs;
	}
	if(!tipLabelData.text) {
			db_error("get LANG_LABEL_LOW_POWER_TEXT failed\n");
			return -1;
	}
	tipLabelData.endLabelKeyUp = endLabelKeyUp;
	if(index == LABEL_LOW_POWER_SHUTDOWN || 
			index == LABEL_SHUTDOWN_NOW  || 
			index == LABEL_30S_NOWORK_SHUTDOWN ||
			index == LABEL_15S_SHUTDOWN||
			index == LABEL_30S_SHUTDOWN||
			index == LABEL_60S_SHUTDOWN||
			index == LABEL_CANCEL_SHUTDOWN) {
		tipLabelData.timeoutMs = 2 * 1000;
	} else if(index == LABEL_10S_SHUTDOWN) {
		tipLabelData.timeoutMs = 10 * 1000;
	}
	if (index == LABEL_WIFI_CONNECT) {
		return showTipLabel(hParent, &tipLabelData, 1);
	}
	db_msg("****************ptext:%s",ptext);
	if(ptext){
		static char tipstext[50]={0};
		memset(tipstext,0,sizeof(tipstext));
		if(index == LABEL_30S_NOWORK_SHUTDOWN){
			ResourceManager * rm = ResourceManager::getInstance();
			bool flag = rm->getResBoolValue(ID_MENU_LIST_STANDBY_MODE);
			if(flag){
				const char*lable = rm->getLabel(LANG_LABEL_NOWORK_ENTER_STANDBY);
				sprintf(tipstext, "%s%s",ptext,lable);
			}else{
				const char*lable = rm->getLabel(LANG_LABEL_30S_NOWORK_SHUTDOWN);
				sprintf(tipstext, "%s%s",ptext,lable);
			}
			tipLabelData.text = tipstext;
		}else{
			const char*lable = rm->getLabel(LANG_LABEL_CARD_READ_WRITE);
			sprintf(tipstext, "%s  %s",lable,ptext);
			tipLabelData.text = tipstext;
		}
	}
	db_msg("*****************text:%s",tipLabelData.text);
	return showTipLabel(hParent, &tipLabelData);
}

static int shutdownDialog(HWND hParent)
{
	MessageBox_t messageBoxData;
	ResourceManager* rm;

	memset(&messageBoxData, 0, sizeof(messageBoxData));
	rm = ResourceManager::getInstance();

	messageBoxData.flag_end_key = 1;	/* End Dialog when key up */
	messageBoxData.dwStyle = MB_OKCANCEL | MB_HAVE_TITLE | MB_HAVE_TEXT;

	messageBoxData.title = rm->getLabel(LANG_LABEL_SHUTDOWN_TITLE);
	if(messageBoxData.title == NULL) {
		db_msg("get shutdown title failed\n");
		return -1;
	}
	messageBoxData.text = rm->getLabel(LANG_LABEL_SHUTDOWN_TEXT);
	if(messageBoxData.text == NULL) {
		db_msg("get shutdown text failed\n");
		return -1;
	}

	messageBoxData.buttonStr[0] = rm->getLabel(LANG_LABEL_SUBMENU_OK);
	if(messageBoxData.buttonStr[0] == NULL) {
		db_msg("get button ok text failed\n");
		return -1;
	}
	messageBoxData.buttonStr[1] = rm->getLabel(LANG_LABEL_SUBMENU_CANCEL);
	if(messageBoxData.buttonStr[1] == NULL) {
		db_msg("get button ok text failed\n");
		return -1;
	}
	
	messageBoxData.pLogFont = rm->getLogFont();
	rm->getResRect(ID_WARNNING_MB, messageBoxData.rect);
	messageBoxData.bgcWidget	= rm->getResColor(ID_WARNNING_MB, COLOR_BGC);
	messageBoxData.fgcWidget	= rm->getResColor(ID_WARNNING_MB, COLOR_FGC);
	messageBoxData.linecTitle	= rm->getResColor(ID_WARNNING_MB, COLOR_LINEC_TITLE);
	messageBoxData.linecItem	= rm->getResColor(ID_WARNNING_MB, COLOR_LINEC_ITEM);

	return showMessageBox(hParent, &messageBoxData);
}

int CdrDialog(HWND hParent, cdrDialogType_t type)
{
	switch(type) {
	case CDR_DIALOG_SHUTDOWN:
		return shutdownDialog(hParent);
		break;
	default:
		break;
	}

	return 0;
}

int CloseTipLabel(void)
{
	db_msg("close TipLabel\n");
	BroadcastMessage(MSG_RECOVER_ADAS_SCREEN_FLAG,0,0);
	BroadcastMessage(MSG_CLOSE_TIP_LABEL, 0, 0);
	return 0;
}



static int createImageWidget(enum ResourceID resID, HWND hParent, BITMAP* bmp)
{
	int retval;
	CDR_RECT rect;
	HWND retWnd;
	ResourceManager *rm;
	
	rm = ResourceManager::getInstance();

	retval = rm->getResRect(resID, rect);
	if(retval < 0) {
		db_error("get rect failed, resID is %d\n", resID);
		return -1;
	}
	db_msg("%d %d %d %d", rect.x, rect.y, rect.w, rect.h);
/*
	retval = rm->getResBmp(resID, BMPTYPE_BASE, *bmp);
	if(retval < 0) {
		db_error("loadb bitmap failed, resID is %d\n", resID);
		return -1;	
	}
 */
	retWnd = CreateWindowEx(CTRL_STATIC, "",
			WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE ,
			WS_EX_NONE,
			resID,
			rect.x, rect.y, rect.w, rect.h,
			hParent, (DWORD)bmp);
	if(retWnd == HWND_INVALID) {
		db_error("create widget failed\n");
		return -1;
	}

	return 0;
}

static BITMAP poweroffBmp;
static int showPowerOffWidget(HWND hParent)
{
	int ret;
	HWND retWnd;
	ret = createImageWidget(ID_POWEROFF, hParent, &poweroffBmp);
	if(ret < 0) {
		db_error("create imageWidget failed\n");
		return ret;
	}
	retWnd = GetDlgItem(hParent, ID_POWEROFF);
	SendMessage(GetDlgItem(hParent, ID_POWEROFF), MSG_PAINT, 0 ,0);
	//ShowWindow(GetDlgItem(hParent, ID_POWEROFF), SW_SHOWNORMAL);
	//UpdateWindow(GetDlgItem(hParent, ID_POWEROFF), true);

	return 0;
}

#ifdef PLATFORM_0
void bmpInfo(unsigned char *mem, int *w, int *h, int *bpp)
{
	int width, height;

	width = *(mem+0x12)+*(mem+0x13)*0x100+*(mem+0x14)*0x10000+*(mem+0x15)*0x1000000;
	height = *(mem+0x16)+*(mem+0x17)*0x100+*(mem+0x18)*0x10000+*(mem+0x19)*0x1000000;
	height = (height) >0 ? (height) : -(height);
	*bpp = *(mem+0x1c)+*(mem+0x1f)*0x100;
	if (0x20 == *bpp)
	{
		*bpp = 32;
	}
	else if (0x18 == *bpp)
	{
		*bpp = 24;
	}
	else if (0xf == *bpp)
	{
		*bpp = 16;
	}
	*w = width;
	*h = height;
}

int getBmpSize(unsigned char *data)
{
	int w, h, bpp;
	bmpInfo(data, &w, &h, &bpp);
	db_msg("w:%d h:%d bpp:%d", w, h, bpp);
	return (w*h*bpp/4);
}

#endif
int showImageWidget(HWND hParent, imageWidgetType_t type, unsigned char *data)
{
	db_msg(" ");
	switch(type) {
	case IMAGEWIDGET_POWEROFF:
		printf("shut down logo .................\n");
		db_msg(" shut down logo");
#if 1 //def PLATFORM_1
		LoadBitmapFromMem(HDC_SCREEN, &poweroffBmp, data, 512*1024, "jpg");
#endif
	db_msg(" ");
		return showPowerOffWidget(hParent);
		break;
	}
		
	return 0;
}

