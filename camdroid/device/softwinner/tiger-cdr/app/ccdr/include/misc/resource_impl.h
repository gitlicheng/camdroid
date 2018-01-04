#ifndef __RESOURCE_IMPL_H__
#define __RESOURCE_IMPL_H__

/* config options in the 480X272.cfg */
#define CFG_FILE_ML				"menu_list"			/* 在配置文件中的key */
#define CFG_FILE_SUBMENU		"sub_menu"			/* 子菜单 */
#define CFG_FILE_ML_MB			"menu_list_message_box"		/* 对话框  */
#define CFG_FILE_BKGROUD_PIC	"bkgroud_pic"
#define CFG_FILE_STB			"status_bar"		/*状态栏在配置文件的的Key*/
#define CFG_FILE_STB_WNDPIC		"status_bar_window_pic"
#define CFG_FILE_STB_LABEL1		"status_bar_label1"
#define CFG_FILE_STB_RECORD_TIME	"status_bar_label_record_time"
#define CFG_FILE_STB_SYSTEM_TIME	"status_bar_label_system_time"
#define CFG_FILE_STB_WIFI			"status_bar_wifi"
#define CFG_FILE_STB_PARK			"status_bar_park"
#define CFG_FILE_STB_UVC			"status_bar_uvc"
#define CFG_FILE_STB_AWMD			"status_bar_awmd"
#define CFG_FILE_STB_LOCK			"status_bar_lock"
#define CFG_FILE_STB_TFCARD			"status_bar_tfcard"
#define CFG_FILE_STB_RECORD_SOUND			"status_bar_record_sound"
#define CFG_FILE_STB_BAT			"status_bar_battery"
#define CFG_FILE_STB_VIDEO_RESOLUTION	"status_bar_video_resolution"
#define CFG_FILE_STB_LOOP_COVERAGE	"status_bar_loop_coverage"
#define CFG_FILE_STB_GSENSOR	"status_bar_gsensor"
#define CFG_FILE_STB_RECORD_HINT	"status_bar_record_hint"
#define CFG_FILE_STB_PHOTO_RESOLUTION	"status_bar_photo_resolution"
#define CFG_FILE_STB_PHOTO_COMPRESSION_QUALITY	"status_bar_photo_compression_quality"
#define CFG_FILE_STB_FILE_STATUS	"status_bar_file_status"
#define CFG_FILE_STB_FILE_LIST	"status_bar_label_file_list"


#define CFG_FILE_ML_VIDEO_RESOLUTION			"menu_list_video_resolution"
#define CFG_FILE_ML_VIDEO_BITRATE			"menu_list_video_bitrate"
#define CFG_FILE_ML_PHOTO_RESOLUTION			"menu_list_photo_resolution"
#define CFG_FILE_ML_PHOTO_COMPRESSION_QUALITY	"menu_list_photo_compression_quality"
#define CFG_FILE_ML_VTL			"menu_list_time_length"
#define CFG_FILE_ML_AWMD		"menu_list_awmd"
#define CFG_FILE_ML_WB			"menu_list_white_balance"
#define CFG_FILE_ML_CONTRAST		"menu_list_contrast"
#define CFG_FILE_ML_EXPOSURE		"menu_list_exposure"
#define CFG_FILE_ML_POR			"menu_list_power_on_record"
#define CFG_FILE_ML_TWM				"menu_list_time_water_mark"
#define CFG_FILE_ML_PHOTO_WM		"menu_list_photo_water_mark"
#define CFG_FILE_ML_LICENSE_PLATE_WM		"menu_list_license_plate_wm"
#define CFG_FILE_ML_SMARTALGORITHM		"menu_list_smartalgorithm"

#define CFG_FILE_ML_PARK		"menu_list_park"
#define CFG_FILE_ML_GSENSOR		"menu_list_gsensor"
#define CFG_FILE_ML_RECORD_SOUND		"menu_list_record_sound"
#define CFG_FILE_ML_VOICEVOL	"menu_list_voicevol"
#define CFG_FILE_ML_KEYTONE		"menu_list_keytone"
#define CFG_FILE_ML_LIGHT_FREQ		"menu_list_light_freq"
#define CFG_FILE_ML_TFCARD_INFO		"menu_list_tfcard_info"
#define CFG_FILE_ML_DELAY_SHUTDOWN	"menu_list_delay_shutdown"
#define CFG_FILE_ML_FORMAT			"menu_list_format"
#define CFG_FILE_ML_LANG			"menu_list_language"
#define CFG_FILE_ML_DATE			"menu_list_date"
#define CFG_FILE_ML_SS			"menu_list_screen_switch"
#define CFG_FILE_ML_FIRMWARE		"menu_list_firmware"
#define CFG_FILE_ML_STANDBY_MODE		"menu_list_standby_mode"
#define CFG_FILE_ML_FACTORYRESET	"menu_list_factory_reset"
#define CFG_FILE_ML_WIFI		"menu_list_wifi"
#define CFG_FILE_ML_ALIGNLINE		"menu_list_alignline"
#define CFG_FILE_ML_INDICATOR		"menu_indicator"


#define CFG_FILE_PLBPREVIEW				"playback_preview"
#define CFG_FILE_PLBPREVIEW_IMAGE		"playback_preview_image"
#define CFG_FILE_PLBPREVIEW_ICON		"playback_preview_icon"
#define CFG_FILE_PLBPREVIEW_MENU		"playback_preview_menu"
#define CFG_FILE_PLBPREVIEW_MESSAGEBOX	"playback_preview_messagebox"

#define CFG_FILE_PLB_ICON				"playback_icon"
#define CFG_FILE_PLB_PGB				"playback_progress_bar"
#define CFG_FILE_CONNECT2PC				"connect2PC"
#define CFG_FILE_WARNING_MB				"warning_messagebox"
#define CFG_FILE_OTHER_PIC				"other_picture"
#define CFG_FILE_TIPLABEL				"tip_label"

/************** define the color attribute subkey *********************/
#define FGC_WIDGET				"fgc_widget"
#define BGC_WIDGET				"bgc_widget"
#define BGC_ITEM_FOCUS			"bgc_item_focus"
#define BGC_ITEM_NORMAL			"bgc_item_normal"
#define MAINC_THREED_BOX		"mainc_3dbox"
#define LINEC_ITEM				"linec_item"
#define LINEC_TITLE				"linec_title"
#define STRINGC_NORMAL			"stringc_normal"
#define STRINGC_SELECTED		"stringc_selected"
#define VALUEC_NORMAL			"valuec_normal"
#define VALUEC_SELECTED			"valuec_selected"
#define SCROLLBARC				"scrollbarc"
#define DATE_NUM				"date_num"
#define DATE_WORD				"date_word"

/* --------------------------- configs for Menu --------------------------- */

/* config options in the menu.cfg */
#define CFG_MENU_SWITCH		"switch"
#define CFG_MENU_WIFI		"wifi"
#define CFG_MENU_LANG		"language"
#define CFG_MENU_AWMD		"awmd"
#define CFG_MENU_RECORD_SOUND		"record_sound"
#define CFG_MENU_POR		"power_on_record"
#define CFG_MENU_TWM		"time_water_mark"
#define CFG_MENU_PHOTO_WM	"photo_water_mark"
#define CFG_MENU_SMARTALGORITHM		"smartalgorithm"
#define CFG_MENU_ALIGNLINE		"alignline"

#define CFG_MENU_KEYTONE			"keytone"
#define CFG_MENU_DELAY_SHUTDOWN		"delay_shutdown"
#define CFG_MENU_STANDBY_MODE		"standby_mode"


#define CFG_MENU_VIDEO_RESOLUTION			"video_resolution"
#define CFG_MENU_VIDEO_BITRATE			"video_bitrate"
#define CFG_MENU_PHOTO_RESOLUTION			"photo_resolution"
#define CFG_MENU_PHOTO_COMPRESSION_QUALITY	"photo_compression_quality"
#define CFG_MENU_VTL		"video_time_length"
#define CFG_MENU_WB			"white_balance"
#define CFG_MENU_CONTRAST	"contrast"
#define CFG_MENU_EXPOSURE	"exposure"
#define CFG_MENU_SS			"screen_switch"
#define CFG_MENU_GSENSOR		"gsensor"
#define CFG_MENU_VOICEVOL		"voicevol"
#define CFG_VERIFICATION	"verification"

#define CFG_FILE_USB_CONNECT            "usb_connect"
#define CFG_MENU_PARK		"park_mode"
#define CFG_MENU_LIGHT_FREQ		"light_freq"

#define CFG_MENU_DEFAULTLICENSE		"defaultlicense"
#define CFG_MENU_TF_CARD		"tfcard_info"


#endif
