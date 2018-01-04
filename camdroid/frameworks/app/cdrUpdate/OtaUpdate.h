#ifndef _WINDOWS_H
#define _WINDOWS_H

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <cutils/log.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <errno.h> 
#include <netinet/in.h>
#include <limits.h> 
#include <netdb.h> 
#include <arpa/inet.h> 
#include <ctype.h> 
#include <linux/watchdog.h>

#define MSG_UPDATE_OVER  1
 
class OtaUpdate{
public:
	OtaUpdate();
	~OtaUpdate();
	OtaUpdate(HWND hwnd);
	int startUpdate();
private:
	HWND mHwnd;
};


#endif







