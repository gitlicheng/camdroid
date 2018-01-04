#ifndef _MAINWINDOW_H
#define _MAINWINDOW_H

#include <ProcessState.h>
#include <IPCThreadState.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

#include "windows.h"
#include "cdr_message.h"
#include "keyEvent.h"

#include "PowerManager.h"
#include "EventManager.h"

/* MainWindow */
#define ID_TIMER_KEY 100

#define CDRMain \
MiniGUIAppMain (int args, const char* argv[], CdrMain*); \
int main_entry (int args, const char* argv[]) \
{ \
	sys_log_init(); \
	sys_log("main entry\n"); \
    int iRet = 0; \
	CdrMain *cdrMain = new CdrMain(); \
	cdrMain->initPreview(NULL); \
    if (InitGUI (args, argv) != 0) { \
        return 1; \
    } \
    iRet = MiniGUIAppMain (args, argv, cdrMain); \
    TerminateGUI (iRet); \
    return iRet; \
} \
int MiniGUIAppMain
#endif
