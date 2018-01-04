
#ifndef __DATESETTING_H__
#define __DATESETTING_H__

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>

#include "cdr_widgets.h"
#include "cdr_message.h"
#include "keyEvent.h"
#include "Rtc.h"


#define IDC_LABEL_START		200
#define IDC_LABEL_TITLE		200
#define IDC_LABEL_YEAR		201
#define IDC_LABEL_MONTH		202
#define IDC_LABEL_DAY		203
#define IDC_LABEL_HOUR		204
#define IDC_LABEL_MINUTE	205
#define IDC_LABEL_SECOND	206
#define IDC_START	207
#define IDC_YEAR	207
#define IDC_MONTH	208
#define IDC_DAY		209
#define IDC_HOUR	210
#define IDC_MINUTE	211
#define IDC_SECOND	212
#define IDC_END		212

#define INDEX_LABEL_TITLE	0
#define INDEX_LABEL_YEAR	1
#define INDEX_LABEL_MONTH	2
#define INDEX_LABEL_DAY		3
#define INDEX_LABEL_HOUR	4
#define INDEX_LABEL_MINUTE	5
#define INDEX_LABEL_SECOND	6
#define INDEX_YEAR			7
#define INDEX_MONTH			8
#define INDEX_DAY			9
#define INDEX_HOUR			10
#define INDEX_MINUTE		11
#define INDEX_SECOND		12

#define SELECT_BORDER	1
#define UNSELECT_BORDER 0







#endif
