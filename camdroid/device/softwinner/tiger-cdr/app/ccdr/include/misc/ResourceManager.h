
#ifndef __RESOURCEMANAGER_H__
#define __RESOURCEMANAGER_H__


#include <minigui/common.h>
#include <minigui/gdi.h>

#include "cdr.h"
#include "cdrLang.h"

#include <utils/Vector.h>
#include <utils/String8.h>
#include <utils/Mutex.h>
#include "cdr_message.h"

#include <DataStruct.h>

#define MAJOR_VERSION	"2"
#define MINOR_VERSION	"0"
#define RELEASE_CANDIDATE_VERSION "3"

//v2.0 rc1
#define FIRMWARE_VERSION ("v"MAJOR_VERSION"."MINOR_VERSION" rc"RELEASE_CANDIDATE_VERSION)

#define NO_SELECT_DIFF

enum ResourceID {
	ID_SCREEN	= 0,
	ID_STATUSBAR,
	ID_STATUSBAR_TOP,
	ID_STATUSBAR_BOTTOM,
	ID_STATUSBAR_LABEL1,
	ID_STATUSBAR_LABEL_RECORD_TIME,
	ID_STATUSBAR_LABEL_SYSTEM_TIME,
	ID_STATUSBAR_LABEL_PLAYBACK_FILE_LIST,			// for playbackPreview
	ID_IMAGE_ICON_START,
	ID_STATUSBAR_ICON_WINDOWPIC,
	ID_STATUSBAR_ICON_AWMD,
	ID_STATUSBAR_ICON_UVC,
	ID_STATUSBAR_ICON_WIFI,
	ID_STATUSBAR_ICON_PARK,
	ID_STATUSBAR_ICON_LOCK,
	ID_STATUSBAR_ICON_TFCARD,
	ID_STATUSBAR_ICON_RECORD_SOUND,
	ID_STATUSBAR_ICON_BAT,
	ID_STATUSBAR_ICON_VIDEO_RESOLUTION,
	ID_STATUSBAR_ICON_LOOP_COVERAGE,
	ID_STATUSBAR_ICON_GSENSOR,
	ID_STATUSBAR_ICON_RECORD_HINT,
	ID_STATUSBAR_ICON_PHOTO_RESOLUTION,				// for photo window
	ID_STATUSBAR_ICON_PHOTO_COMPRESSION_QUALITY,	// for photo window
	ID_STATUSBAR_ICON_PLAYBACK_FILE_STATUS,			// for playbackPreview
	ID_PLAYBACKPREVIEW_ICON,
	ID_PLAYBACK_ICON,
	ID_IMAGE_ICON_END,
	ID_MENU_LIST,
	ID_MENU_LIST_INDICATOR,
	ID_MENU_LIST_MB,
	ID_MENU_LIST_ITEM_START,
	ID_MENU_LIST_PARK,
	ID_MENU_LIST_GSENSOR,
	ID_MENU_LIST_RECORD_SOUND,
	ID_MENU_LIST_KEYTONE,		// key tone
	ID_MENU_LIST_VOICEVOL,
	ID_MENU_LIST_LIGHT_FREQ,
	ID_MENU_LIST_TFCARD_INFO,
	ID_MENU_LIST_DELAY_SHUTDOWN,
	ID_MENU_LIST_FORMAT,
	ID_MENU_LIST_LANG,
	ID_MENU_LIST_DATE,
	ID_MENU_LIST_SS,
	ID_MENU_LIST_FIRMWARE,
	ID_MENU_LIST_STANDBY_MODE,
	ID_MENU_LIST_FRESET,
	ID_MENU_LIST_WIFI,
	ID_MENU_LIST_ALIGNLINE,

	ID_MENU_LIST_VIDEO_RESOLUTION,
	ID_MENU_LIST_VIDEO_BITRATE,
	ID_MENU_LIST_VTL,
	ID_MENU_LIST_WB,
	ID_MENU_LIST_CONTRAST,
	ID_MENU_LIST_EXPOSURE,
	ID_MENU_LIST_POR,
	ID_MENU_LIST_TWM,
	ID_MENU_LIST_LWM,
	ID_MENU_LIST_AWMD,
	ID_MENU_LIST_LICENSE_PLATE_WM,
#ifdef APP_ADAS
	ID_MENU_LIST_SMARTALGORITHM,
#endif

	ID_MENU_LIST_PHOTO_RESOLUTION,
	ID_MENU_LIST_PHOTO_COMPRESSION_QUALITY,
	ID_MENU_LIST_PHOTO_WM,
	ID_MENU_LIST_ITEM_END,

	ID_MENU_LIST_UNFOLD_PIC,
	ID_MENU_LIST_CHECKBOX_PIC,
	ID_MENU_LIST_FIRMWARE_OK,
	ID_MENU_LIST_UNCHECKBOX_PIC,	/* end of menulist */
	ID_SUBMENU,
	ID_SUBMENU_CHOICE_PIC,
	ID_PLAYBACKPREVIEW_IMAGE,
	ID_RECORDPREVIEW_IMAGE,
	ID_PLAYBACKPREVIEW_MENU,
	ID_PLAYBACKPREVIEW_MB,
	ID_PLAYBACK_PGB,
	ID_VOCIE,
	ID_CONNECT2PC,
	ID_WARNNING_MB,
	ID_POWEROFF,
	ID_TIPLABEL
};

enum BmpType{
	BMPTYPE_BASE = 0,
	BMPTYPE_UNSELECTED,
	BMPTYPE_SELECTED
};

enum ColorType{
	COLOR_BGC = 0,
	COLOR_FGC,
	COLOR_FGC_LABEL,
	COLOR_FGC_NUMBER,
	COLOR_BGC_ITEMFOCUS,
	COLOR_BGC_ITEMNORMAL,
	COLOR_MAIN3DBOX,
	COLOR_LINEC_ITEM,
	COLOR_LINEC_TITLE,
	COLOR_STRINGC_NORMAL,
	COLOR_STRINGC_SELECTED,
	COLOR_SCROLLBARC,
	COLOR_VALUEC_NORMAL,
	COLOR_VALUEC_SELECTED,
	COLOR_BORDERC_NORMAL,
	COLOR_BORDERC_SELECTED,
};

enum IntValueType{
	INTVAL_ITEMWIDTH = 0,
	INTVAL_ITEMHEIGHT,
	INTVAL_TITLEHEIGHT,
	INTVAL_HBORDER,
	INTVAL_YEARWIDTH,
	INTVAL_LABELWIDTH,
	INTVAL_NUMBERWIDTH,
	INTVAL_BOXHEIGHT,
	INTVAL_SUBMENU_INDEX,
	INTVAL_SUBMENU_COUNT
};

enum UsbType{
 ID_USB_COMPUTER=0,
 ID_USB_WORKING,	
 ID_USB_CAMERA,
};
using namespace android;

class ResourceManager : public cdrLang
{
private:
	ResourceManager();
	~ResourceManager();
	ResourceManager& operator=(const ResourceManager& o);

public:
	virtual int initLangAndFont(LANGUAGE language);

	static ResourceManager* getInstance();
	HWND getHwnd(unsigned int win_id);
	/*
	 * Only read the screen rect and configuration about status bar from the configs
	 * */
	int initStage1(void);
	/*
	 * init others resources
	 * */
	int initStage2(void);

	/* 
	 * get the GAL pixel color from the config_file
	 * color in the config_file is in the A B G R format
	 * */
	gal_pixel getResColor(enum ResourceID resID, enum ColorType type);

	int getResRect(enum ResourceID resID, CDR_RECT &rect);

	int getResBmp(enum ResourceID resID, enum BmpType, BITMAP &bmp);
	int getResBmp2(enum ResourceID resID, size_t index, BITMAP &bmp);
	int getResBkgroundBmp(size_t index, BITMAP &bmp);
	/*
	 * get the current subMenu's check box bmp
	 * */
	int getResBmpSubMenuCheckbox(enum ResourceID, bool isHilight, BITMAP &bmp);

	int getResIntValue(enum ResourceID, enum IntValueType type);
	int setResIntValue(enum ResourceID, enum IntValueType type, int value);

	bool getResBoolValue(enum ResourceID resID);
	int setResBoolValue(enum ResourceID resID, bool value);
	int getSubMenuCurrentIndex(enum ResourceID resID);

	void setCurrentIconValue(enum ResourceID, int cur_val);
	int getCurrentIconValue(enum ResourceID resID);

	const char* getResMenuItemString(enum ResourceID resID);

	/*
	 * is index is -1, then get the current ItemString
	 * */
	const char* getResSubMenuItemString(enum ResourceID resID, int index);
	const char* getResSubMenuTitle(enum ResourceID resID);

	int refreshLWMResource(void);
	struct {
		bool LWMEnable;
		char lwaterMark[30];
	}menuDataLWM;
	LANGUAGE getLanguage(void);

	PLOGFONT getLogFont(void);

	/*
	 * Load Bitmap from config through the giving section name and key name
	 * On success return 0
	 * On faile return -1
	 * */
	int loadBmpFromConfig(const char* pSection, const char* pKey, PBITMAP bmp);

	/*
	 * unload the giving bitmap structure
	 * */
	void unloadBitMap(BITMAP &bmp);

	void setHwnd(unsigned int win_id, HWND hwnd);
	int notifyAll();
	void dump();
	void syncConfigureToDisk(void);
	void resetResource(void);
	int getConfig(__aw_cdr_net_config *config);
	int sessionControl(int cmd, int param_in, int param_out);
	int setRecordingStatus(bool recording);
	BITMAP * getNoVideoImage();
	int notifyWaterMark();
private:
	bool verifyConfig();
	int initStatusBarResource(void);
	int initPlayBackPreviewResource(void);
	int initPlayBackResource(void);
	int initMenuListResource(void);
	int initMenuResource(void);
	int initOtherResource(void);

	int getCurrentIconFileName(enum ResourceID resID, String8 &file);
	int getMLPICFileName(enum ResourceID, enum BmpType type, String8 &file);
	int getIconUsbConnect(enum UsbType type, String8 &file);
	gal_pixel getStatusBarColor(enum ResourceID resID, enum ColorType type);
	gal_pixel getPlaybackPreviewColor(enum ResourceID resID, enum ColorType type);
	gal_pixel getPlaybackColor(enum ResourceID resID, enum ColorType type);
	gal_pixel getMenuListColor(enum ResourceID resID, enum ColorType type);
	gal_pixel getSubMenuColor(enum ResourceID resID, enum ColorType type);
	gal_pixel getWarningMBColor(enum ResourceID resID, enum ColorType type);
	gal_pixel getConnect2PCColor(enum ResourceID resID, enum ColorType type);

	int getSubMenuCounts(enum ResourceID resID);
	int getSubMenuIntAttr(enum ResourceID, enum IntValueType type);

	int setSubMenuCurrentIndex(enum ResourceID resID, int value);

	void updateLangandFont(int langValue);
	void syncIconCurrent();

	typedef struct {
		unsigned int current;
		Vector < String8 >icon;
	}currentIcon_t;

	typedef struct {
		unsigned int current;
		unsigned int count;
	}menuItem_t;

	/* 
	 * print error msg through the err_code
	 * */
	static char* pEtcError(int err_code);
	static char* pBmpError(int err_code);

	/* 
	 * -------------------------------------------------------------------------------------------
	 * before call the following functions, should use getCfgFileHandle or getCfgMenuHandle first
	 * in the end, should call the releaseCfgFileHandle or releaseCfgMenuHandle to release Handle;
	 * */
	int getCfgFileHandle(void);
	int getCfgMenuHandle(void);
	void releaseCfgFileHandle(void);
	void releaseCfgMenuHandle(void);

	/*
	 *  get the int value from the giving file, mainkey is the section name, subkey is the key value
	 * */
	int getIntValueFromHandle(GHANDLE cfgHandle, const char *mainkey, const char *subkey);


	/*
	 * get the rect from config_file
	 * pWindow the the section name in the config file
	 * On success return ETC_OK
	 * On fail return err code
	 * */
	int getRectFromFileHandle(const char *pWindow, CDR_RECT *rect);

	/*
	 * get the ABGR format color from the config_file
	 * On success return ETC_OK
	 * On fail return err code
	 * */
	int getABGRColorFromFileHandle(const char* pWindow, const char* subkey, gal_pixel &pixel);

	/*
	 * On success return ETC_OK
	 * On fail return err code
	 * */
	int getValueFromFileHandle(const char*mainkey, const char* subkey, char* value, unsigned int len);

	/*
	 * get current icon info from config_file, and fill the giving currentIcon
	 * example : if current is 1, then the icon1 is used
	 * [status_bar_battery]
	 *	current=1
	 *	icon0=/res/topbar/baterry1.png
	 *	icon1=/res/topbar/baterry2.png
	 *	icon2=/res/topbar/baterry3.png
	 *	icon3=/res/topbar/baterry4.png
	 * On success return ETC_OK
	 * On fail return err code
	 * */
	int fillCurrentIconInfoFromFileHandle(const char* pWindow, currentIcon_t &currentIcon);

	/*
	 * get language from config menu
	 * On success return LANGUAGE
	 * On faile return LANG_ERR
	 * */
	LANGUAGE getLangFromMenuHandle(void);
	/* --------------------------------------------------- */

	/* -------------------- the following function do not need the cfgHandle------------------------------- */
	LANGUAGE getLangFromConfigFile(void);

private:
	Mutex mLock;
	HWND mHwnd[WIN_CNT];
	char configFile[32];
	const char* configMenu;
	char* defaultConfigFile;
	const char* defaultConfigMenu;
	GHANDLE mCfgFileHandle;
	GHANDLE mCfgMenuHandle;
	bool mRecording;
	int mScreenOffTime;
	BITMAP  mNoVideoImage;
	
	/* ------------- resource datas ------------ */
	CDR_RECT rScreenRect;
	currentIcon_t rBkgroundPic;

	/* ---- status bar ------ */
	struct {
		CDR_RECT STBRect;
		gal_pixel bgc;
		gal_pixel fgc;

		CDR_RECT windowPicRect;
		currentIcon_t windowPicIcon;

		CDR_RECT label1Rect;
		CDR_RECT labelRecordTimeRect;
		CDR_RECT labelSystemTimeRect;

		// for playbackPreview window
		CDR_RECT fileListRect;

		CDR_RECT wifiRect;
		currentIcon_t wifiIcon;

		CDR_RECT parkRect;
		currentIcon_t parkIcon;

		CDR_RECT uvcRect;
		currentIcon_t uvcIcon;

		CDR_RECT awmdRect;
		currentIcon_t awmdIcon;
		
		CDR_RECT lockRect;
		currentIcon_t lockIcon;

		CDR_RECT tfCardRect;
		currentIcon_t tfCardIcon;

		CDR_RECT recordSoundRect;
		currentIcon_t recordSoundIcon;

		CDR_RECT batteryRect;
		currentIcon_t batteryIcon;

		CDR_RECT videoResolutionRect;
		currentIcon_t videoResolutionIcon;

		CDR_RECT loopCoverageRect;
		currentIcon_t loopCoverageIcon;

		CDR_RECT gsensorRect;
		currentIcon_t gsensorIcon;

		CDR_RECT recordHintRect;
		currentIcon_t recordHintIcon;

		// for photo window
		CDR_RECT photoResolutionRect;
		currentIcon_t photoResolutionIcon;

		CDR_RECT photoCompressionQualityRect;
		currentIcon_t photoCompressionQualityIcon;

		// for playbackPreview window
		CDR_RECT fileStatusRect;
		currentIcon_t fileStatusIcon;
	}rStatusBarData;

	/* PlaybackPreview */
	struct {
		CDR_RECT imageRect;		/* display the image thumnail */
		CDR_RECT iconRect;		/* display the play icon */
		currentIcon_t icon;

		CDR_RECT popupMenuRect;
		unsigned int popupMenuItemWidth;
		unsigned int popupMenuItemHeight;
		gal_pixel popupMenuBgcWidget;
		gal_pixel popupMenuBgcItemNormal;
		gal_pixel popUpMenuBgcItemFocus;
		gal_pixel popUpMenuMain3dBox;

		CDR_RECT messageBoxRect;
		unsigned int messageBoxItemWidth;
		unsigned int messageBoxItemHeight;
		gal_pixel messageBoxBgcWidget;
		gal_pixel messageBoxFgcWidget;
		gal_pixel messageBoxBgcItemNormal;
		gal_pixel messageBoxBgcItemFous;
		gal_pixel messageBoxMain3dBox;
	}rPlayBackPreviewData;

	/* PlayBack */
	struct {
		CDR_RECT iconRect;
		currentIcon_t icon;

		CDR_RECT PGBRect;
		gal_pixel PGBBgcWidget;
		gal_pixel PGBFgcWidget;
	}rPlayBackData;

	/* Menu */
	struct {
		CDR_RECT menuListRect;
		CDR_RECT subMenuRect;
		CDR_RECT messageBoxRect;
		CDR_RECT dateRect;

		unsigned int dateTileRectH;
		unsigned int dateHBorder;
		unsigned int dateYearW;;
		unsigned int dateLabelW;;
		unsigned int dateNumberW;;
		unsigned int dateBoxH;;

		String8 choicePic;
		String8 checkedNormalPic;
		String8 checkedPressPic;
		String8 uncheckedNormalPic;
		String8 uncheckedPressPic;
		String8 unfoldNormalPic;
		String8 unfoldPressPic;
		String8 firme_ok;

		gal_pixel bgcWidget;
		gal_pixel fgcWidget;
		gal_pixel linecItem;
		gal_pixel stringcNormal;
		gal_pixel stringcSelected;
		gal_pixel valuecNormal;
		gal_pixel valuecSelected;
		gal_pixel scrollbarc;

		gal_pixel subMenuBgcWidget;
		gal_pixel subMenuFgcWidget;
		gal_pixel subMenuLinecTitle;

		gal_pixel messageBoxBgcWidget;
		gal_pixel messageBoxFgcWidget;
		gal_pixel messageBoxLinecTitle;
		gal_pixel messageBoxLinecItem;

		gal_pixel dateBgcWidget;
		gal_pixel dateFgc_label;
		gal_pixel dateFgc_number;
		gal_pixel dateLinecTitle;
		gal_pixel dateBordercSelected;
		gal_pixel dateBordercNormal;

		// menu indicator
		currentIcon_t indicatorIcon;
		CDR_RECT	  indicatorRect;
		gal_pixel	bgcIndicator;
		gal_pixel	scrollbarcIndicator;

		// ------------ menu item icon --------------
		// system menu
	    currentIcon_t parkIcon;
		currentIcon_t gsensorIcon;
		currentIcon_t recordSoundIcon;
		currentIcon_t voicevolIcon;
		currentIcon_t keyToneIcon;		// key tone, key voice
		currentIcon_t lightFreqIcon;
		currentIcon_t tfcardInfoIcon;
		currentIcon_t delayShutdownIcon;
		currentIcon_t formatIcon;
		currentIcon_t languageIcon;
		currentIcon_t dateIcon;
		currentIcon_t screenSwitchIcon;
		currentIcon_t firmwareIcon;
		currentIcon_t standbyModeIcon;
		currentIcon_t factoryResetIcon;
		currentIcon_t wifiIcon;
		currentIcon_t alignlineIcon;

		// record menu
		currentIcon_t videoResolutionIcon;
		currentIcon_t videoBitrateIcon;
		currentIcon_t timeLengthIcon;
		currentIcon_t whiteBalanceIcon;
		currentIcon_t contrastIcon;
		currentIcon_t exposureIcon;
		currentIcon_t PORIcon;
		currentIcon_t TWMIcon;
		currentIcon_t moveDetectIcon;
		currentIcon_t licensePlateWMIcon;
		currentIcon_t smartalgorithm;
		
		// photo menu
		currentIcon_t photoResolutionIcon;
		currentIcon_t photoCompressionQualityIcon;
		currentIcon_t photoWMIcon;		// photo watermark


		// ---------- menu item submenu info -------------------
		// system menu
		menuItem_t menuDataPark;
		menuItem_t menuDataGsensor;
		menuItem_t menuDataVoiceVol;
		menuItem_t menuDataLightFreq;
		menuItem_t menuDataTFcard;
		menuItem_t menuDataLang;		
		menuItem_t menuDataSS;
		menuItem_t menuDataShutDown;
		
		// record menu
		menuItem_t menuDataVideoResolution;
		menuItem_t menuDataVideoBitrate;
		menuItem_t menuDataVTL;
		menuItem_t menuDataWB;
		menuItem_t menuDataContrast;
		menuItem_t menuDataExposure;

		// photo menu
		menuItem_t menuDataPhotoResolution;
		menuItem_t menuDataPhotoCompressionQuality;


		// ----------- menu item checkbox info ---------------
		bool recordSoundEnable;
		bool keyToneEnable;
		bool standbyModeEnable;
		bool wifiEnable;
		bool alignlineEnable;

		bool TWMEnable;
		bool POREnable;
		bool AWMDEnable;
		bool photoWMEnable;
		bool smartAlgorithmEnable;
		bool menuDataFamireEnable;
	}rMenuList;

	struct {
		CDR_RECT rect;
		gal_pixel fgcWidget;
		gal_pixel bgcWidget;
		gal_pixel linecTitle;
		gal_pixel linecItem;
	}rWarningMB;

	struct {
		CDR_RECT rect;	
		unsigned int itemWidth;
		unsigned int itemHeight;

		gal_pixel bgcWidget;
		gal_pixel bgcItemFocus;
		gal_pixel bgcItemNormal;
		gal_pixel mainc_3dbox;
	}rConnectToPC;

	struct {
		CDR_RECT rect;	
		unsigned int titleHeight;
		gal_pixel fgcWidget;
		gal_pixel bgcWidget;
		gal_pixel linecTitle;
	}rTipLabel;

	String8 rPoweroffPic;

	struct { 
	  String8 usb_computer ;
	  String8 usb_camera ;
	  String8 usb_working ;
	   }rUSBconnect;
public:
	
	float mCdrVolume;
	/* ---------- end of resource datas ------------ */
};
#endif
