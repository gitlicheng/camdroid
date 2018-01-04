
#ifndef __STATUSBAR_H__
#define __STATUSBAR_H__

#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include "cdr_res.h"
#include "misc.h"
#include "windows.h"
#include "cdr_message.h"
#include "ResourceManager.h"

#define STB_TIMER_SOS       114
#define STB_TIMER_SYSTEM_TIME       115

enum WINDOWPIC {
	WINDOWPIC_RECORDPREVIEW = 0,
	WINDOWPIC_PHOTOGRAPH,
	WINDOWPIC_PLAYBACKPREVIEW,
	WINDOWPIC_MENU,
};

// ------------------ statusBarTop -----------------------
static CdrIcon::CdrIcon_t s_IconSetsTop[] = {
	{ID_STATUSBAR_ICON_WINDOWPIC,	"WindowPic"},
	{ID_STATUSBAR_ICON_AWMD,		"AWMD"},
	{ID_STATUSBAR_ICON_UVC,			"UVC"},
	{ID_STATUSBAR_ICON_WIFI,		"Wifi"},
	{ID_STATUSBAR_ICON_RECORD_SOUND,		"RecordSound"},
	{ID_STATUSBAR_ICON_TFCARD,		"TFCard"},
	{ID_STATUSBAR_ICON_BAT,			"Bat"},
	{ID_STATUSBAR_ICON_PARK,		"Park"},
	{ID_STATUSBAR_ICON_VIDEO_RESOLUTION,	"VideoResolution"},
	{ID_STATUSBAR_ICON_LOOP_COVERAGE,	"LoopCoverage"},
	{ID_STATUSBAR_ICON_GSENSOR,			"Gsensor"},

	// for photo window
	{ID_STATUSBAR_ICON_PHOTO_RESOLUTION,				"photoResolution"},
	{ID_STATUSBAR_ICON_PHOTO_COMPRESSION_QUALITY,	"photoCompressionQuality"},

	// for playbackPreview window
	{ID_STATUSBAR_ICON_PLAYBACK_FILE_STATUS,		"fileStatus"},
};

static CdrLabel::CdrLabel_t s_LabelSetsTop[] = {
	{ID_STATUSBAR_LABEL1,			"StatusBarLabel1"},

	// for playbackPreview window
	{ID_STATUSBAR_LABEL_PLAYBACK_FILE_LIST,	"fileList"},
};
// ++++++++++++++++++ end of statusBarTop ++++++++++++++++++++


// ------------------ statusBarBottom -----------------------
static CdrIcon::CdrIcon_t s_IconSetsBottom[] = {
	{ID_STATUSBAR_ICON_LOCK,			"Lock"},
	{ID_STATUSBAR_ICON_RECORD_HINT,		"RecordHint"},
};
static CdrLabel::CdrLabel_t s_LabelSetsBottom[] = {
	{ID_STATUSBAR_LABEL_RECORD_TIME,			"RecordTime"},
	{ID_STATUSBAR_LABEL_SYSTEM_TIME,			"SystemTime"},
};
// ++++++++++++++++++ end of statusBarBottom ++++++++++++++++++++

static int STB_SOS = 5;

#endif


