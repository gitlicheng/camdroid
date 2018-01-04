
#ifndef __DIALOG_H__
#define __DIALOG_H__

#include <minigui/common.h>
#include "cdr_widgets.h"

typedef enum {
	CDR_DIALOG_SHUTDOWN,
}cdrDialogType_t;

typedef enum {
	IMAGEWIDGET_POWEROFF = 0,
}imageWidgetType_t;

#define DIALOG_OK			(IDC_BUTTON_START)
#define DIALOG_CANCEL		(IDC_BUTTON_START + 1)

enum labelIndex {
	LABEL_NO_TFCARD = 0,
	LABEL_TFCARD_FULL,
	LABEL_PLAYBACK_FAIL,
	LABEL_LOW_POWER_SHUTDOWN,
	LABEL_10S_SHUTDOWN,
	LABEL_SHUTDOWN_NOW,
	LABEL_30S_NOWORK_SHUTDOWN,
	LABEL_FILELIST_EMPTY,
	LABEL_WIFI_CONNECT,
	LABEL_IMPACT_NUM,
	LABEL_LABEL_OTA_UPDATE,
	LABEL_FILE_LOCKED,
	LABEL_LOCKED_FILE,
	LABEL_UNLOCK_FILE,
	LABEL_SD_PLUGIN,
	LABEL_SD_PLUGOUT,
	LABEL_SPEED_TFCARD,
	LABEL_15S_SHUTDOWN,
	LABEL_30S_SHUTDOWN,
	LABEL_60S_SHUTDOWN,
	LABEL_CANCEL_SHUTDOWN,
	LABEL_SOS_WARNING,
	LABEL_SUBMENU_TF_CARD_CONTENT2,
	LABEL_FORMAT_TFCARD_FINISH,
	LABEL_FORMATING_TFCARD,
	LABEL_INVAILD_TFCATD,
};

extern int CdrDialog(HWND hParent, cdrDialogType_t type);

extern int showImageWidget(HWND hParent, imageWidgetType_t type, unsigned char* data=NULL);

extern int ShowTipLabel(HWND hParent, enum labelIndex index, bool endLabelKeyUp = true, unsigned int timeoutMs = TIME_INFINITE, bool hasicon = true,char *ptext=NULL);
extern int CloseTipLabel(void);
extern int getTipLabelData(tipLabelData_t* tipLabelData);


#endif
