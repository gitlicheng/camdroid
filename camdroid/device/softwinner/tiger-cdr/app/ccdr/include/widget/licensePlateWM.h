
#ifndef __LICENSE_PLATE_WM_H__
#define __LICENSE_PLATE_WM_H__

#include "cdr.h"

#define LICENSE_PLATE_NUMBER_COUNT	8
#define CANDICATE_NUMBER_MAX_COUNT	36
#define CANDICATE_BUFFER_LEN	(CANDICATE_NUMBER_MAX_COUNT * 4)
typedef struct _license_plate_wm_data{
	PLOGFONT pLogFont;
	CDR_RECT dlgRect;	
	PBITMAP	 pBmpTitle;
	PBITMAP	 pBmpEnableChioce;
	PBITMAP	 pBmpDisableChioce;
	PBITMAP  pBmpArrow;
	const char* titleText;
	const char* enableText;
	const char* disableText;
	char licensePlateNumber[LICENSE_PLATE_NUMBER_COUNT + 1];
	char candidateEnNumbers[CANDICATE_BUFFER_LEN + 1 ];
	char candidateCnNumbers[CANDICATE_BUFFER_LEN + 1 ];
	LANGUAGE language;
}LicensePlateWM_data_t;


typedef struct {
	bool current;
	int position[8];
	char NumStr[10];
}menuPlate_t;


#define CFG_VERIFICATION	"verification"




#define lconfigMenu "/data/menu.cfg"
#define defaultConfigMenu "/system/res/cfg/menu.cfg"





extern int licensePlateWM_Dialog(HWND hParent, LicensePlateWM_data_t* dialogData);




#endif
