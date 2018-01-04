#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>
#include <minigui/ctrl/edit.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <HerbCamera.h>
#include <HerbMediaRecorder.h>
#include <CedarMediaPlayer.h>
#include <CedarDisplay.h>
#include <hwdisp_def.h>
#include <fb.h>
#include <cutils/android_reboot.h>
using namespace android;

#define TAG "MiniGUI"
#include <dragonboard.h>


#define FIFO_TF_DEV   "fifo_tf"
#define FIFO_SPK_DEV  "fifo_spk"
#define FIFO_MIC_DEV  "fifo_mic"
#define FIFO_RTC_DEV  "fifo_rtc"
#define FIFO_ACC_DEV  "fifo_acc"
#define FIFO_NOR_DEV  "fifo_nor"
#define FIFO_DDR_DEV  "fifo_ddr"
#define FIFO_WIFI_DEV "fifo_wifi"



static int lcdWidth, lcdHeight;
static int chipType;			// 0-V3; 1-A20

// content that read from fifo will display on LCD
static char key_str[30];
static char tf_str[30];
static char spk_str[30];
static char mic_str[30];

static char rtc_str[30];
static char acc_str[30];
static char nor_str[30];
static char ddr_str[30];

static char wifi_str[50];

// whnd ID define
#define ID_MWND   100
#define ID_CAM1   101
#define ID_CAM2   102
#define ID_WIFI   103
#define ID_HAND   104
#define ID_AUTO   105
#define ID_MICB   106
#define ID_LCDR   107
#define ID_LCDG   108
#define ID_LCDB   109
#define ID_TIMER  110		// timer(0.5s) for refresh the screen
#define ID_KEY    111
#define ID_TF     112
#define ID_MIC    113
#define ID_SPK    114
#define ID_RTC    115
#define ID_ACC    116
#define ID_NOR    117
#define ID_DDR    118

// set these vars by ConfigureParser()
// testxxx: 1--test and display it(default) 
//          0--NOT display
// testxxxPath: default in /mnt/extsd/ 
//              use /system/bin/ when NOT find it in tf
int  testKey = 1;
//char testKeyPath[40] = "keytester";			// path name can NOT over 40 chars!!!
int  testTf = 1;
char testTfPath[40] = "/mnt/extsd/DragonBoard/tftester";	// if it can NOT be found, use /system/bin/ by system()
int  testMic = 1;
char testMicPath[40] = "/mnt/extsd/DragonBoard/mictester";
int  testSpk = 1;
char testSpkPath[40] = "/mnt/extsd/DragonBoard/spktester";
int  testRtc = 1;
char testRtcPath[40] = "/mnt/extsd/DragonBoard/rtctester";
int  testAcc = 1;
char testAccPath[40] = "/mnt/extsd/DragonBoard/acctester";	// NOT sensortester, may be confused with video's sensor
int  testNor = 1;
char testNorPath[40] = "/mnt/extsd/DragonBoard/nortester";
int  testDdr = 1;
char testDdrPath[40] = "/mnt/extsd/DragonBoard/ddrtester";
int  testWifi = 1;
char testWifiPath[40] = "/mnt/extsd/DragonBoard/wifitester";
int  testVideo = 1;						// forground video, NOT confused with G-sensor
//int  testUvc = 1;						// background video(NOT used)(UVC + TVIN), it is contained in testVideo

void DetectChipType(void)
{
/*
	int ffb;
	struct fb_var_screeninfo var;				// detect which chip
	if ((ffb = open("/dev/graphics/fb0", O_RDWR)) < 0) {
		db_warn("open fb0 failed\n");
		chipType = 1;					// default: A20
		return;
	}
	ioctl(ffb, FBIOGET_VSCREENINFO, &var);			// TODO:
	if (var.xres == 320) {					// There will be trouble by this method in the future if there are more resolution for LCD
		chipType = 0;	// V3				// But now(2015-01-09), it works well
		close(ffb);
	} else if (var.xres == 480){				// V3-CDR: 320*240; V3-perf: 800*480; A20-CDR: 480*272
		chipType = 1;	// A20
		close(ffb);
	} else {
		db_msg("res: %dX%d, chip type unknown! but as A20!\n", var.xres, var.yres);
		chipType = 1;	// look it as A20
		close(ffb);
	}
*/
	db_msg("==============read cpuinfo==================\n");
	int cnt, fd;
	char buf[4096], cpuHardware[10];
	char *cpuHardwarePtr, *cpuInfoPtr;
	cpuHardwarePtr = cpuHardware;
	if ((fd = open("/proc/cpuinfo", O_RDONLY)) > 0) {
		cnt = read(fd, buf, 4096);
		if (cnt != -1) {
			cpuInfoPtr = strstr(buf, "sun");
			while ((*cpuHardwarePtr++ = *cpuInfoPtr++) != '\n');
			*--cpuHardwarePtr = '\0';
			if (strcmp(cpuHardware, "sun8i") == 0)		// V3
				chipType = 0;
			else if (strcmp(cpuHardware, "sun7i") == 0)	// A20
				chipType = 1;
			else {
				chipType = 2;				// unknown
				db_warn("UNKNOWN chip!\n");
			}
		}
		db_msg("Hardware: %s, chipType = %d\n", cpuHardware, chipType);
		close(fd);
	} else {
		chipType = -1;
		db_warn("can NOT open cpuinfo!\n");
	}
	db_msg("=============end of cpuinfo==================\n");

	return;
}

int SetTestcase(const char* ptr1, const char* ptr2)
{	
	char name[10];
	char path[50];
	FILE* fp;

//	db_debug("[debug] [%s] [%s] [%s]\n", __FUNCTION__, ptr1, ptr2);

	if (strncmp(ptr1 + 1, "KEY", 3) == 0) {
		testKey = atoi(ptr1 + 8);
		return 0;
	} else if (strncmp(ptr1 + 1, "TF", 2) == 0) {
//		testTf = atoi(ptr1 + 7);				// ptr1 + 7 NOT good, because length of tester-name not All 2 chars!!!
									// so it is ptr1 + 8 for [MIC] = x, and ptr1 + 9 for [WIFI] = x to detect x(0/1)
		while (!isdigit(*ptr1))
			ptr1++;
		testTf = atoi(ptr1);
		if (strncmp(ptr2, "path = ", 7) == 0) {
			strlcpy(path, strchr(ptr2, '"') + 1, strrchr(ptr2, '"') - strchr(ptr2, '"'));
			if ((fp = fopen(path, "r")) != NULL) {		// bin may NOT exist, use default
				strlcpy(testTfPath, path, 40);			// careful for path's length!!!
				fclose(fp);
			}
		}
		return 0;
	} else if (strncmp(ptr1 + 1, "MIC", 3) == 0) {
//		testMic = atoi(ptr1 + 8);
		while (!isdigit(*ptr1))
			ptr1++;
		testMic = atoi(ptr1);
		if (strncmp(ptr2, "path = ", 7) == 0) {
			strlcpy(path, strchr(ptr2, '"') + 1, strrchr(ptr2, '"') - strchr(ptr2, '"'));
			if ((fp = fopen(path, "r")) != NULL) {		// bin may NOT exist, use default
				strlcpy(testMicPath, path, 40);
				fclose(fp);
			}
		}
		return 0;
	} else if (strncmp(ptr1 + 1, "SPK", 3) == 0) {
//		testSpk = atoi(ptr1 + 8);
		while (!isdigit(*ptr1))
			ptr1++;
		testSpk = atoi(ptr1);
		return 0;
	} else if (strncmp(ptr1 + 1, "RTC", 3) == 0) {
//		testRtc = atoi(ptr1 + 8);
		while (!isdigit(*ptr1))
			ptr1++;
		testRtc = atoi(ptr1);
		if (strncmp(ptr2, "path = ", 7) == 0) {
			strlcpy(path, strchr(ptr2, '"') + 1, strrchr(ptr2, '"') - strchr(ptr2, '"'));
			if ((fp = fopen(path, "r")) != NULL) {		// bin may NOT exist, use default
				strlcpy(testRtcPath, path, 40);
				fclose(fp);
			}
		}
		return 0;
	} else if (strncmp(ptr1 + 1, "ACC", 3) == 0) {
//		testAcc = atoi(ptr1 + 8);
		while (!isdigit(*ptr1))
			ptr1++;
		testAcc = atoi(ptr1);
		if (strncmp(ptr2, "path = ", 7) == 0) {
			strlcpy(testAccPath, strchr(ptr2, '"') + 1, strrchr(ptr2, '"') - strchr(ptr2, '"'));
			if ((fp = fopen(path, "r")) != NULL) {		// bin may NOT exist, use default
				strlcpy(testAccPath, path, 40);
				fclose(fp);
			}
		}
		return 0;
	} else if (strncmp(ptr1 + 1, "NOR", 3) == 0) {
//		testNor = atoi(ptr1 + 8);
		while (!isdigit(*ptr1))
			ptr1++;
		testNor = atoi(ptr1);
		if (strncmp(ptr2, "path = ", 7) == 0) {
			strlcpy(path, strchr(ptr2, '"') + 1, strrchr(ptr2, '"') - strchr(ptr2, '"'));
			if ((fp = fopen(path, "r")) != NULL) {		// bin may NOT exist, use default
				strlcpy(testNorPath, path, 40);
				fclose(fp);
			}
		}
		return 0;
	} else if (strncmp(ptr1 + 1, "DDR", 3) == 0) {
//		testDdr = atoi(ptr1 + 8);
		while (!isdigit(*ptr1))
			ptr1++;
		testDdr = atoi(ptr1);
		if (strncmp(ptr2, "path = ", 7) == 0) {
			strlcpy(path, strchr(ptr2, '"') + 1, strrchr(ptr2, '"') - strchr(ptr2, '"'));
			if ((fp = fopen(path, "r")) != NULL) {		// bin may NOT exist, use default
				strlcpy(testDdrPath, path, 40);
				fclose(fp);
			}
		}
		return 0;
	} else if (strncmp(ptr1 + 1, "WIFI", 4) == 0) {
//		testWifi = atoi(ptr1 + 9);				// NOT [ptr1 + 8]!!! [WIFI] = x
		while (!isdigit(*ptr1))
			ptr1++;
		testWifi = atoi(ptr1);
		if (strncmp(ptr2, "path = ", 7) == 0) {
			strlcpy(path, strchr(ptr2, '"') + 1, strrchr(ptr2, '"') - strchr(ptr2, '"'));
			if ((fp = fopen(path, "r")) != NULL) {		// bin may NOT exist, use default
				strlcpy(testWifiPath, path, 40);
				fclose(fp);
			}
		}
		return 0;
	} else if (strncmp(ptr1 + 1, "VIDEO", 5) == 0) {
		while(!isdigit(*ptr1))
			ptr1++;
		testVideo = atoi(ptr1);
		return 0;
	} else {
		snprintf(name, strchr(ptr1, ']') - ptr1, "%s", ptr1 + 1);
		db_warn("Unknown testcase: [%s]!\n", name);
		return -1;
	}
}

int ConfigureParser(void)
{
	FILE *fp;
	char nameBuf[100];		// NOTICE: make sure NOT over 15 chars for testcase name line!!!
	char pathBuf[100];		// NOTICE: make sure NOT over 100 chars for path every line!!!
	int retVal;
	char cmpbuf[10];
	char* pcfg = "/mnt/extsd/DragonBoard/dragonboard.cfg";	// TODO: make sure xx.cfg is in tf
								// put it in tf to make it easy to modify display

	fp = fopen(pcfg, "r");
	if (fp == NULL) {
		db_warn("configure [%s] NOT found! enable all the testcase by default!\n", pcfg);
	} else {
		db_msg("parse the configure [%s], prepare for dragonboard!\n", pcfg);
		while (fgets(nameBuf, 100, fp) != NULL) {
			if ((*nameBuf == '[') && ((strstr(nameBuf,"] = ")) != NULL)) {	// [KEY] = 1
				fgets(pathBuf, 100, fp);			// path = "xxx"
				retVal = SetTestcase(nameBuf, pathBuf);
			}
		}
		fclose(fp);
		db_verb("testKey = %d\n", testKey);
		db_verb("testTf = %d, path = %s\n", testTf, testTfPath);
		db_verb("testMic = %d, path = %s\n", testMic, testMicPath);
		db_verb("testRtc = %d, path = %s\n", testRtc, testRtcPath);
		db_verb("testAcc = %d, path = %s\n", testAcc, testAccPath);
		db_verb("testNor = %d, path = %s\n", testNor, testNorPath);
		db_verb("testDdr = %d, path = %s\n", testDdr, testDdrPath);
		db_verb("testWifi = %d, path = %s\n", testWifi, testWifiPath);
	}
	if (chipType == 0) {
//		testWifi = 1;
	} else {
		testWifi = 0;		// A20 has no wifi
	}

	return 0;
}

int HelloWinProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
	char buff[20];
	int keyVal;
	static clock_t timestamp1, timestamp2;
	static HWND hHandTester, hAutoTester,hMicBar;
	static HWND hwnd_key, hwnd_tf, hwnd_spk, hwnd_mic; 
	static HWND hwnd_rtc, hwnd_acc, hwnd_nor, hwnd_ddr; 
	static HWND hwnd_wifi;
	static HWND hwnd_lcdr, hwnd_lcdg, hwnd_lcdb; 

	switch (message) {
		case MSG_CREATE:
			hHandTester = CreateWindow("static", "HandTester", 
					WS_CHILD | WS_VISIBLE, ID_HAND,
					0, 0, (lcdWidth - 10)/2, 20,
					hWnd, 0);
			SetWindowBkColor(hHandTester, COLOR_yellow);
			hAutoTester = CreateWindow("static", "AutoTester", 
					WS_CHILD | WS_VISIBLE, ID_AUTO,
					lcdWidth/2 + 5, 0, (lcdWidth - 10)/2, 20,
					hWnd, 0);
			SetWindowBkColor(hAutoTester, COLOR_yellow);

			hMicBar = CreateWindowEx("progressbar", NULL,
					WS_CHILD | WS_VISIBLE | PBS_VERTICAL,
					WS_EX_NONE, IDC_STATIC,
					(lcdWidth - 10)/2, 0, 10, 190,
					hWnd, 0);
			SetWindowBkColor(hMicBar, COLOR_darkmagenta);

			// HandTester
			if (testKey) {			// 1: test&display; 0: NOT test&display
				hwnd_key = CreateWindow(CTRL_STATIC, NULL,
						WS_CHILD | WS_VISIBLE, ID_KEY,
						0, 30, (lcdWidth - 10)/2, 20,
						hWnd, 0);
				SetWindowBkColor(hwnd_key, COLOR_blue);
			}
			if (testTf) {
				hwnd_tf = CreateWindow(CTRL_STATIC, NULL,
						WS_CHILD | WS_VISIBLE, ID_TF,
						0, 55, (lcdWidth - 10)/2, 20,
						hWnd, 0);
				SetWindowBkColor(hwnd_tf, COLOR_blue);
			}
			if (testSpk) {
				hwnd_spk = CreateWindow(CTRL_STATIC, NULL,
						WS_CHILD | WS_VISIBLE, ID_SPK,
						0, 80, (lcdWidth - 10)/2, 20,
						hWnd, 0);
				SetWindowBkColor(hwnd_spk, COLOR_blue);
			}
			if (testMic) {
				hwnd_mic = CreateWindow(CTRL_STATIC, NULL,
						WS_CHILD | WS_VISIBLE, ID_MIC,
						0, 105, (lcdWidth - 10)/2, 20,
						hWnd, 0);
				SetWindowBkColor(hwnd_mic, COLOR_blue);
			}


			// AutoTester
			if (testRtc) {
				hwnd_rtc = CreateWindow(CTRL_STATIC, NULL,
						WS_CHILD | WS_VISIBLE, ID_RTC,
						lcdWidth/2 + 5, 30, (lcdWidth - 10)/2, 20,
						hWnd, 0);
				SetWindowBkColor(hwnd_rtc, COLOR_blue);
			}
			if (testAcc) {
				hwnd_acc = CreateWindow(CTRL_STATIC, NULL,
						WS_CHILD | WS_VISIBLE, ID_ACC,
						lcdWidth/2 + 5, 55, (lcdWidth - 10)/2, 20,
						hWnd, 0);
				SetWindowBkColor(hwnd_acc, COLOR_blue);
			}
			if (testNor) {
				hwnd_nor = CreateWindow(CTRL_STATIC, NULL,
						WS_CHILD | WS_VISIBLE, ID_NOR,
						lcdWidth/2 + 5, 80, (lcdWidth - 10)/2, 20,
						hWnd, 0);
				SetWindowBkColor(hwnd_nor, COLOR_blue);
			}
			if (testDdr) {
				hwnd_ddr = CreateWindow(CTRL_STATIC, NULL,
						WS_CHILD | WS_VISIBLE, ID_DDR,
						lcdWidth/2 + 5, 105, (lcdWidth - 10)/2, 20,
						hWnd, 0);
				SetWindowBkColor(hwnd_ddr, COLOR_blue);
			}

			// wifi tester
			if (testWifi) {
				hwnd_wifi = CreateWindow(CTRL_STATIC, NULL,
						WS_CHILD | WS_VISIBLE, ID_WIFI,
						0, 130, lcdWidth, 20,
						hWnd, 0);
				SetWindowBkColor(hwnd_wifi, COLOR_blue);
			}

			// LCD RGB tester
			// lcdHeight-160-20-50 means video's height is 50 => make lcdrgb's height stable, but video's height changable 
			// 10(hwnd_lcdr's height), 50(video's height), 20(caption's height) -- V3(320*240)
			hwnd_lcdr = CreateWindow(CTRL_STATIC, NULL,
					WS_CHILD | WS_VISIBLE, ID_LCDR,
			//		0, 160, lcdWidth/3, lcdHeight - 160 - 20 - 50,
					0, 160, lcdWidth/3, 10,		// make LCD_R/G/B height = 10
					hWnd, 0);
			SetWindowBkColor(hwnd_lcdr, COLOR_red);
			hwnd_lcdg = CreateWindow(CTRL_STATIC, NULL,
					WS_CHILD | WS_VISIBLE, ID_LCDG,
			//		lcdWidth/3, 160, lcdWidth/3, lcdHeight - 160 - 20 - 50,
					lcdWidth/3, 160, lcdWidth/3, 10,
					hWnd, 0);
			SetWindowBkColor(hwnd_lcdg, COLOR_green);
			hwnd_lcdb = CreateWindow(CTRL_STATIC, NULL,
					WS_CHILD | WS_VISIBLE, ID_LCDB,
			//		lcdWidth*2/3, 160, lcdWidth/3, lcdHeight - 160 - 20 - 50,
					lcdWidth*2/3, 160, lcdWidth/3, 10,
					hWnd, 0);
			SetWindowBkColor(hwnd_lcdb, COLOR_blue);

			SetTimer(hWnd, ID_TIMER, 20);		// set timer 0.2s(20), 1s(100)
			break;

		case MSG_TIMER:
			// HandTester
			if (testKey) {
				if (*key_str == 'P') {
					SetWindowBkColor(hwnd_key, PIXEL_green);
					SetDlgItemText(hWnd, ID_KEY, key_str+1);
				} else {
					SetWindowBkColor(hwnd_key, PIXEL_cyan);
					SetDlgItemText(hWnd, ID_KEY, "[KEY] waiting");
				}
			}

			if (testTf) {
				if (*tf_str == 'P') {
					SetWindowBkColor(hwnd_tf, PIXEL_green);		// green means pass 
					SetDlgItemText(hWnd, ID_TF, tf_str+1);
				} else if (*tf_str == 'F') {
					SetWindowBkColor(hwnd_tf, PIXEL_red);
					SetDlgItemText(hWnd, ID_TF, "[TF] fail");	// red means fail
				} else {
					SetWindowBkColor(hwnd_tf, PIXEL_cyan);		// cyan means wait
					SetDlgItemText(hWnd, ID_TF, "[TF] waiting");
				}
			}

			if (testSpk) {
				if (*spk_str == 'P') {
					SetWindowBkColor(hwnd_spk, PIXEL_green);	// green means pass 
					SetDlgItemText(hWnd, ID_SPK, spk_str+1);
				} else if (*spk_str == 'F') {
					SetWindowBkColor(hwnd_spk, PIXEL_red);
					SetDlgItemText(hWnd, ID_SPK, "[SPK] fail");	// red means fail
				} else {
					SetWindowBkColor(hwnd_spk, PIXEL_cyan);		// cyan means wait
					SetDlgItemText(hWnd, ID_SPK, "[SPK] waiting");
				}
			}

			if (testMic) {
				if (*mic_str == 'P') {
					SetWindowBkColor(hwnd_mic, PIXEL_green);	// green means pass 
					SetDlgItemText(hWnd, ID_MIC, mic_str+1);
				} else if (*mic_str == 'F') {
					SetWindowBkColor(hwnd_mic, PIXEL_red);
					SetDlgItemText(hWnd, ID_MIC, "[MIC] fail");	// red means fail
				} else {
					SetWindowBkColor(hwnd_mic, PIXEL_cyan);		// cyan means wait
					SetDlgItemText(hWnd, ID_MIC, "[MIC] waiting");
				}
			}
			
			// AutoTester
			if (testRtc) {
				if (*rtc_str == 'P') {
					SetWindowBkColor(hwnd_rtc, PIXEL_green);	// green means pass 
					SetDlgItemText(hWnd, ID_RTC, rtc_str+1);
				} else if (*rtc_str == 'F') {
					SetWindowBkColor(hwnd_rtc, PIXEL_red);
					SetDlgItemText(hWnd, ID_RTC, rtc_str+1);	// red means fail
				} else {
				//	SetWindowBkColor(hwnd_rtc, PIXEL_cyan);		// cyan means wait
				//	SetDlgItemText(hWnd, ID_RTC, "[RTC] waiting");
					SetWindowBkColor(hwnd_rtc, PIXEL_cyan);		// cyan means wait
					SetDlgItemText(hWnd, ID_RTC, "[RTC] waiting");
				}
			}

			if (testAcc) {
				if (*acc_str == 'P') {
					SetWindowBkColor(hwnd_acc, PIXEL_green);
					SetDlgItemText(hWnd, ID_ACC, acc_str+1);
				} else if (*acc_str == 'F'){
					SetWindowBkColor(hwnd_acc, PIXEL_red);
					SetDlgItemText(hWnd, ID_ACC, "[ACC] fail");
				} else if (*acc_str == 'M'){					// add 'M' signal as debug info.
					SetWindowBkColor(hwnd_acc, PIXEL_cyan);
					SetDlgItemText(hWnd, ID_ACC, "[ACC] low memory");
				} else {
					SetWindowBkColor(hwnd_acc, PIXEL_cyan);
					SetDlgItemText(hWnd, ID_ACC, "[ACC] waiting");
				}
			}

			if (testNor) {
				if (*nor_str == 'P') {
					SetWindowBkColor(hwnd_nor, PIXEL_green);
					SetDlgItemText(hWnd, ID_NOR, nor_str+1);
				} else if (*nor_str == 'F') {
					SetWindowBkColor(hwnd_nor, PIXEL_red);
					SetDlgItemText(hWnd, ID_NOR, "[NOR] fail");
				} else {
					SetWindowBkColor(hwnd_nor, PIXEL_cyan);
					SetDlgItemText(hWnd, ID_NOR, "[NOR] waiting");
				}
			}

			if (testDdr) {
				if (*ddr_str == 'P') {
					SetWindowBkColor(hwnd_ddr, PIXEL_green);
					SetDlgItemText(hWnd, ID_DDR, ddr_str+1);
				} else if (*ddr_str == 'F') {
					SetWindowBkColor(hwnd_ddr, PIXEL_red);
					SetDlgItemText(hWnd, ID_DDR, "[DDR] fail");
				} else {
					SetWindowBkColor(hwnd_ddr, PIXEL_cyan);
					SetDlgItemText(hWnd, ID_DDR, "[DDR] waiting");
				} 
			}

			if (testWifi) {
				if (*wifi_str == 'P') {
					SetWindowBkColor(hwnd_wifi, PIXEL_green);
					SetDlgItemText(hWnd, ID_WIFI, wifi_str+1);
				} else if (*wifi_str == 'F'){
					SetWindowBkColor(hwnd_wifi, PIXEL_red);
					SetDlgItemText(hWnd, ID_WIFI, "[WIFI] fail");
				} else {
					SetWindowBkColor(hwnd_wifi, PIXEL_cyan);
					SetDlgItemText(hWnd, ID_WIFI, "[WIFI] waiting");
				}
				break;
			}

		case MSG_KEYDOWN:
			// int otherKeyPress = 0;
			// printf("fd = %#x, type = %#x\n", LOWORD(wParam), HIWORD(wParam));
			keyVal = LOWORD(wParam);
			// SetWindowBkColor(hwnd_key, COLOR_green);
			// V3 and A20 button has a little difference
			if (chipType == 0) {				// V3 button define
			//	switch (keyVal) {
			//		case 0x80:
			//			strlcpy(key_str, "P[KEY] up", 15);
			//			break;
			//		case 0x81:
			//			strlcpy(key_str, "P[KEY] down", 15);
			//			break;
			//		case 0x82:
			//			strlcpy(key_str, "P[KEY] ok", 15);
			//			break;
			//		case 0x83:
			//			strlcpy(key_str, "P[KEY] menu", 15);
			//			break;
			//		case 0x84:
			//			timestamp1 = clock() / CLOCKS_PER_SEC;
			//			strlcpy(key_str, "P[KEY] power", 15);
			//			break;
			//		default:
			//			db_warn("WARNING: other key?\n");
			//			break;
			//	}
				switch (keyVal) {
					case 0x80:
						sprintf(key_str, "P[KEY] up(%#x)", keyVal);
						SetWindowBkColor(hwnd_key, COLOR_green);
						break;
					case 0x81:
						sprintf(key_str, "P[KEY] down(%#x)", keyVal);
						SetWindowBkColor(hwnd_key, COLOR_green);		// again
						break;
					case 0x82:
						sprintf(key_str, "P[KEY] ok(%#x)", keyVal);
						SetWindowBkColor(hwnd_key, COLOR_green);		// again, repeat this NOT just low-efficient, but just because the board or GUI's lib has bug!!!
						break;
					case 0x83:
						sprintf(key_str, "P[KEY] menu(%#x)", keyVal);
						SetWindowBkColor(hwnd_key, COLOR_green);		// again
						break;
					case 0x84:
						timestamp1 = clock() / CLOCKS_PER_SEC;
						sprintf(key_str, "P[KEY] power(%#x)", keyVal);
						SetWindowBkColor(hwnd_key, COLOR_green);		// again
						break;
					default:
						sprintf(key_str, "P[KEY] undef(%#x)", keyVal);
						SetWindowBkColor(hwnd_key, COLOR_green);		// again
						break;
					//	otherKeyPress = 1;
					//	db_debug("WARNING: other key?\n");
						//sprintf(key_str, "P[KEY] unknown keyVal(%#x)", keyVal);
						break;
				}
			} else if (chipType == 1) {			// A20 button define
				switch (keyVal) {
					case 0x80:
						sprintf(key_str, "P[KEY] down(%#x)", keyVal);
						SetWindowBkColor(hwnd_key, COLOR_green);
						break;
					case 0x81:
						sprintf(key_str, "P[KEY] up(%#x)", keyVal);
						SetWindowBkColor(hwnd_key, COLOR_green);		// again
						break;
					case 0x82:
						sprintf(key_str, "P[KEY] ok(%#x)", keyVal);
						SetWindowBkColor(hwnd_key, COLOR_green);		// again
						break;
					case 0x83:
						sprintf(key_str, "P[KEY] menu(%#x)", keyVal);
						SetWindowBkColor(hwnd_key, COLOR_green);		// again
						break;
					case 0x84:
						timestamp1 = clock() / CLOCKS_PER_SEC;
						sprintf(key_str, "P[KEY] power(%#x)", keyVal);
						SetWindowBkColor(hwnd_key, COLOR_green);		// again
						break;
					case 0x85:
						sprintf(key_str, "P[KEY] save(%#x)", keyVal);
						SetWindowBkColor(hwnd_key, COLOR_green);		// again
						break;
					default:
					//	otherKeyPress = 1;
					//	db_debug("WARNING: other key?\n");		// cancel it, there's flush to terminal, maybe MiniGUI's lib have troubles
						//sprintf(key_str, "P[KEY] unknown keyVal(%#x)", keyVal);
						break;
				}
			}
		//	if (otherKeyPress) {					// because the minigui's lib transplantation NOT full or the board's design, there's always a key response
		//		SetWindowBkColor(hwnd_key, COLOR_cyan);
		//	} else {
		//		SetWindowBkColor(hwnd_key, COLOR_green);
		//	}
			break;
		case MSG_KEYUP:
			keyVal = LOWORD(wParam);
			switch (keyVal) {
				case 0x84:
					timestamp2 = clock() / CLOCKS_PER_SEC;
					if ( timestamp2 - timestamp1 > 2) {	// long press >= 3s
						// strlcpy(key_str, "P[KEY] reboot...", 20);
						// sleep(5);						// sleep 5s to display string "reboot..."
						// system("reboot");		// NO cmd "poweroff"
						android_reboot(ANDROID_RB_POWEROFF, 0, 0);
					}
			}
			break;
		case MSG_CLOSE:
			KillTimer(hWnd, ID_TIMER);
			DestroyAllControls(hWnd);
			DestroyMainWindow(hWnd);
			PostQuitMessage(hWnd);
			return 0;
	}

	return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

void* tfThreadRead(void* argv)			// test tf card
{
	int tfFd, retVal, fifoFd;

	if ((fifoFd = open(FIFO_TF_DEV, O_RDONLY)) < 0) {
		if ((retVal = mkfifo(FIFO_TF_DEV, 0666)) < 0) {
			db_error("can NOT create fifo for TF(%s)\n", strerror(errno));
			strlcpy(tf_str, "F[MIC] fail", 30);
			return NULL;
		} else {
			fifoFd = open(FIFO_TF_DEV, O_RDONLY);
		}
	}
	sleep(2);
	while (1) {
		read(fifoFd, tf_str, 30);
//		if ((tfFd = open("/mnt/extsd", O_RDONLY)) < 0) {	// those was done in tftester
//			strncpy(tf_str, "F[TF] fail", 11);
//		} else {
//			strncpy(tf_str, "P[TF] pass", 11);
//		}
//		close(tfFd);
		usleep(200000);						// detect every 200ms(MUST equal to tftester's value!!!)
//		sleep(1);
	}

	return NULL;
}

void* micThreadRead(void* argv)
{
	int retVal;
	int fifoFd;

	if ((fifoFd = open(FIFO_MIC_DEV, O_RDONLY)) < 0) {
		if ((retVal = mkfifo(FIFO_MIC_DEV, 0666)) < 0) {
			db_error("can NOT create fifo for MIC(%s)\n", strerror(errno));
			strlcpy(mic_str, "F[MIC] fail", 30);
			return NULL;
		} else {
			fifoFd = open(FIFO_MIC_DEV, O_RDONLY);
		}
	}
	while (1) {
		read(fifoFd, mic_str, 30);
		if (strncmp(mic_str, "P[MIC] pass", 11) == 0) {	// MUST be 11! NOT 11 + 1
			close(fifoFd);				// 10s(record-sound time) later, close fifo
			break;					// MUST finish this thread!!! because write-endian was closed!
		}
//		sleep(1);
	}

	return NULL;
}

void* spkThreadRead(void* argv)
{
	int retVal;
	int fifoFd;

	if ((fifoFd = open(FIFO_SPK_DEV, O_RDONLY)) < 0) {
		if ((retVal = mkfifo(FIFO_SPK_DEV, 0666)) < 0) {
			db_error("can NOT create fifo for SPK(%s)\n", strerror(errno));
			strlcpy(spk_str, "F[SPK] fail", 30);
			return NULL;
		} else {
			fifoFd = open(FIFO_SPK_DEV, O_RDONLY);
		}
	}
	while (1) {
		read(fifoFd, spk_str, 30);
		if (strcmp(spk_str, "P[SPK] rec.. end") == 0) {
			close(fifoFd);			// DO need it after play sample
			strcpy(spk_str, "P[SPK] pass");
			break;					// end this thread, because mictester will exit soon
		}
		sleep(1);
	}

	return NULL;
}

void* rtcThreadRead(void* argv)
{
	int fifoFd, retVal;
	struct fifo_param fifo_data;

	if ((fifoFd = open(FIFO_RTC_DEV, O_RDONLY)) < 0) {
		if ((retVal = mkfifo(FIFO_RTC_DEV, 0666)) < 0) {
			db_error("can NOT create fifo for RTC(%s)\n", strerror(errno));
			return NULL;
		} else {
			fifoFd = open(FIFO_RTC_DEV, O_RDONLY);
		}
	}

	while (1) {
//		read(fifoFd, &fifo_data, sizeof(fifo_param_t));
		read(fifoFd, rtc_str, 30);
	//	puts("==================================================");
	//	db_debug("get info.[%s]\n", rtc_str);
	//	puts("==================================================");
//		if (fifo_data.result == 'F') {
//			strlcpy(rtc_str, "F[RTC] fail", 20);
//			close(fifoFd);
//			break;		// finish thread as long as receive fail signal('F'), is it too strict? YES, cancel it
//		} else {		// It's better to process data in fifo's write-endian, NOT here! 
//			snprintf(rtc_str, 30, "%c[RTC] %.2d:%.2d:%.2d", fifo_data.result, fifo_data.val.rtc.tm_hour, fifo_data.val.rtc.tm_min, fifo_data.val.rtc.tm_sec);
//		}
		sleep(1);		// renew time by 1s, NOT 2s even if rtctester read rtc-hardware, then sleep 1s(must equal)
	}

	return NULL;
}

void* accThreadRead(void* argv)
{
	int fifoFd, retVal;
	struct fifo_param fifo_data;

	if ((fifoFd = open(FIFO_ACC_DEV, O_RDONLY)) < 0) {
		if ((retVal = mkfifo(FIFO_ACC_DEV, 0666)) < 0) {
			db_error("can NOT create fifo for ACC(%s)\n", strerror(errno));
			return NULL;
		} else {
			fifoFd = open(FIFO_ACC_DEV, O_RDONLY);
		}
	}

	while (1) {
		read(fifoFd, &fifo_data, sizeof(fifo_param_t));
		if (fifo_data.result == 'F') {
			strlcpy(acc_str, "F[ACC] fail", 20);
//			close(fifoFd);
//			break;
		} else if (fifo_data.result == 'M') {		// hardly happen
			strlcpy(acc_str, "M[ACC] low memory", 20);
//			close(fifoFd);
//			break;
		} else {
			snprintf(acc_str, 30, "%c[ACC] <%3.1f,%3.1f,%3.1f>",fifo_data.result, fifo_data.val.acc[0], fifo_data.val.acc[1], fifo_data.val.acc[2]);
		}
		sleep(1);
	}

	return NULL;
}

void* norThreadRead(void* argv)
{
	int fifoFd, retVal;

	if ((fifoFd = open(FIFO_NOR_DEV, O_RDONLY)) < 0) {
		if ((retVal = mkfifo(FIFO_NOR_DEV, 0666)) < 0) {
			db_error("can NOT create fifo for ACC(%s)\n", strerror(errno));
			return NULL;
		} else {
			fifoFd = open(FIFO_NOR_DEV, O_RDONLY);
		}
	}

	while (1) {
		read(fifoFd, nor_str, 30);
//		sprintf(nor_str, "P[NOR] pass");
		sleep(1);			// time control is done in testcase => avoid read flood in case of write-endian shutdown 
	}

	return NULL;
}

void* ddrThreadRead(void* argv)
{
	int fifoFd, retVal;

	if ((fifoFd = open(FIFO_DDR_DEV, O_RDONLY)) < 0) {
		if ((retVal = mkfifo(FIFO_DDR_DEV, 0666)) < 0) {
			db_error("can NOT create fifo for DDR(%s)\n", strerror(errno));
		} else {
			fifoFd = open(FIFO_DDR_DEV, O_RDONLY);
		}
	}

	while (1) {
		read(fifoFd, ddr_str, 30);
//		sprintf(ddr_str, "P[DDR] pass");
		sleep(1);		// sleep 1s to avoid read flood
	}

	return NULL;
}


void* wifiThreadRead(void* argv)
{
	int fifoFd, retVal;
	char hotspot[50];
	//char* ptr_hs1;		// set wifi hotspots data type is done in wifitester
	//char* ptr_hs2;		// xxThreadRead() just receive str from fifo, NOT analysize here
	//struct fifo_param fifo_data;

	if ((fifoFd = open(FIFO_WIFI_DEV, O_RDONLY)) < 0) {
		if ((retVal = mkfifo(FIFO_WIFI_DEV, 0666)) < 0) {
			db_error("can NOT create fifo for WIFI(%s)\n", strerror(errno));
			return NULL;
		} else {
			fifoFd = open(FIFO_WIFI_DEV, O_RDONLY);
		}
	}
	
	while (1) {
		retVal = read(fifoFd, hotspot, 50);	// NOTICE: when hotspot info. over 50 chars...
		if (*hotspot == 'F') {
			wifi_str[0] = 'F';
			close(fifoFd);
//			break;
//			db_warn("can not get hotspots, chech out wheather wifi is opened\n");
		} else if (*hotspot == 'P') {
//			db_verb("get from wifi:[%s]\n", hotspot+1);
			strncpy(wifi_str, hotspot, 50);
		}
		sleep(1);
	}

	return NULL;
}

// NOTICE: below here are start tester thread, which can comunicate with testerThreadRead()
// TODO: system() will block until cmd finish, execv() will replace current process
// so should create a startTester thread for every tester!
// copy testcase to / to avoid program stop when plug out the tf card
// if bin of dragonboard.c was exec in TF card, can NOT plug out TF card!!! Next work was how to do for this condition
void* startTfTester(void* argv)
{
	FILE* fp;
	char tmpPath[50];					// make sure "cp /mnt/extsd/DragonBoard/tftester /" less than 60 chars!!!

	if (testTf) {
		sleep(2);					// waiting time 2s to display "waiting"
		if ((fp = fopen(testTfPath, "r")) != NULL) {	// path by dragonboard.cfg
			fclose(fp);
			sprintf(tmpPath, "cp %s /", testTfPath);
			printf("tmpPath: %s\n", tmpPath);
			system(tmpPath);			// copy testcase to /(TF card -> /)(TF -> mem)
			system("/tftester");		// TODO: carefully for tester's path!!!
		} else {
			system("tftester");		// exact path: /system/bin/tftester
		}
	} else {
		db_warn("NOT test tf!\n");
	}
//	system("/mnt/extsd/DragonBoard/tftester");		
//	system("tftester");

	return NULL;
}

void* startMicTester(void* argv)
{
	FILE* fp;
	char tmpPath[50];					// make sure "cp /mnt/extsd/DragonBoard/tftester /" less than 60 chars!!!

	if (testMic) {
		sleep(2);					// waiting time 2s to display "waiting"
		if ((fp = fopen(testMicPath, "r")) != NULL) {
			fclose(fp);
			sprintf(tmpPath, "cp %s /", testMicPath);
			system(tmpPath);			// copy testcase to /(TF card -> /)(TF -> mem)
			system("/mictester");
//			system(testMicPath);		// TODO: carefully for tester's path!!!
		} else {
			system("mictester");
		}
	} else {
		db_warn("NOT test micphone and speaker!\n");
	}
//	system("/mnt/extsd/DragonBoard/mictester");
//	system("mictester");

	return NULL;
}

void* startRtcTester(void* argv)
{
	FILE* fp;
	char tmpPath[50];					// make sure "cp /mnt/extsd/DragonBoard/tftester /" less than 60 chars!!!

	if (testRtc) {
		sleep(2);					// waiting time 2s to display "waiting"
		if ((fp = fopen(testRtcPath, "r")) != NULL) {
			fclose(fp);
			sprintf(tmpPath, "cp %s /", testRtcPath);
			system(tmpPath);			// copy testcase to /(TF card -> /)(TF -> mem)
			system("/rtctester");
//			system(testRtcPath);		// TODO: carefully for tester's path!!!
		} else {
			system("rtctester");
		}
	} else {
		db_warn("NOT test rtc!\n");
	}

	return NULL;
}

void* startAccTester(void* argv)
{
	FILE* fp;
	char tmpPath[50];					// make sure "cp /mnt/extsd/DragonBoard/tftester /" less than 60 chars!!!

	if (testAcc) {					// "Acc" - accerator
		sleep(2);					// waiting time 2s to display "waiting"
		if ((fp = fopen(testAccPath, "r")) != NULL) {
			fclose(fp);
			sprintf(tmpPath, "cp %s /", testAccPath);
			system(tmpPath);			// copy testcase to /(TF card -> /)(TF -> mem)
			system("/acctester");
//			system(testAccPath);		// TODO: carefully for tester's path!!!
		} else {
			system("acctester");
		}
	} else {
		db_warn("NOT test sensor(acc, g-sensor, gyr)!\n");
	}

	return NULL;
}

void* startNorTester(void* argv)		// need add real detect program later
{
	FILE* fp;
	char tmpPath[50];					// make sure "cp /mnt/extsd/DragonBoard/tftester /" less than 60 chars!!!

	if (testNor) {
		sleep(2);					// waiting time 2s to display "waiting"
		if ((fp = fopen(testNorPath, "r")) != NULL) {
			fclose(fp);
			sprintf(tmpPath, "cp %s /", testNorPath);
			system(tmpPath);			// copy testcase to /(TF card -> /)(TF -> mem)
			system("/nortester");
//			system(testNorPath);		// TODO: carefully for tester's path!!!
		} else {
			system("nortester");
		}
	} else {
		db_warn("NOT test nor!\n");
	}

	return NULL;
}

void* startDdrTester(void* argv)		// need add real detect program later
{
	FILE* fp;
	char tmpPath[50];					// make sure "cp /mnt/extsd/DragonBoard/tftester /" less than 60 chars!!!

	if (testDdr) {
		sleep(2);					// waiting time 2s to display "waiting"
		if ((fp = fopen(testDdrPath, "r")) != NULL) {
			fclose(fp);
			sprintf(tmpPath, "cp %s /", testDdrPath);
			system(tmpPath);			// copy testcase to /(TF card -> /)(TF -> mem)
			system("/ddrtester");
//			system(testDdrPath);		// TODO: carefully for tester's path!!!
		} else {
			system("ddrtester");
		}
	} else {
		db_warn("NOT test ddr!\n");
	}

	return NULL;
}

void* startWifiTester(void* argv)
{
	FILE* fp;
	char tmpPath[50];					// make sure "cp /mnt/extsd/DragonBoard/tftester /" less than 60 chars!!!

	if (testWifi) {
		sleep(2);					// waiting time 2s to display "waiting"
		if ((fp = fopen(testWifiPath, "r")) != NULL) {
			fclose(fp);
			sprintf(tmpPath, "cp %s /", testWifiPath);
			system(tmpPath);			// copy testcase to /(TF card -> /)(TF -> mem)
			system("/wifitester");
//			system(testWifiPath);		// TODO: carefully for tester's path!!!
		} else {
			system("wifitester");
		}
	} else {
		db_warn("NOT test wifi!\n");
	}

	return NULL;
}

void* startVideoTester(void* argv)
{
	int nPreviewEnableFlag = 1;
	int nRecordEnableFlag = 0;
	int nTestSensorFlag = 1;
	int nTestUvcFlag = 1;
	int nEnableWaterMark = 0;
	bool nVideoOnly = true;
	int nSensorBitRate = 5*1024*1024;
	int nUvcBitRate = 5*1024*1024;
	unsigned int nRecordedTime = 60;
	int nBackSensorExistFlag = 0;

	sp<ProcessState> proc(ProcessState::self());
	ProcessState::self()->startThreadPool();

	HerbCamera* mHC = NULL;
	HerbCamera* mUvcHC = NULL;
	CedarDisplay* mCD = NULL;
	CedarDisplay* mUvcCD = NULL;
	HerbMediaRecorder* mHMR = NULL;
	HerbMediaRecorder* mUvcHMR = NULL;
	HerbCamera::Parameters HCParam;
	HerbCamera::Parameters uvcHCParam;
	int hlay = -1;
	int hlay2 = -1;
	
	int wantedFrameRate = 30;
	Size wantedPreviewSize(640, 360);
	Size wantedVideoSize(1920, 1080);

	int uvcFrameRate = 25;
	Size uvcPreviewSize(640, 360);
	Size uvcVideoSize(1920, 1080);
	Size uvcEncodeOutVideoSize = uvcVideoSize;

	int videoCodec = HerbMediaRecorder::VideoEncoder::H264;
	int nSourceChannel;
	if (nTestSensorFlag) {
		db_msg("before HerbCamera open\n");
		mHC = HerbCamera::open(0);
		db_msg("after HerbCamera open\n");
		mHC->getParameters(&HCParam);
		size_t i;
		Vector<Size> supportedPreviewSizes;
		Vector<Size> supportedVideoSizes;
		Vector<int>  supportedFrameRates;
		Size defaultPreviewSize(0, 0);
		Size defaultVideoSize(0, 0);
		int defaultFrameRate;
		HCParam.getSupportedPreviewSizes(supportedPreviewSizes);
		HCParam.getSupportedPreviewFrameRates(supportedFrameRates);
		HCParam.getSupportedVideoSizes(supportedVideoSizes);
		HCParam.getPreviewSize(defaultPreviewSize);
//		HCParam.getVideoSize(defaultVideoSize);				// A20 NOT support this
		defaultFrameRate = HCParam.getPreviewFrameRate();
		// preview
		for (i = 0; i < supportedPreviewSizes.size(); i++) {
			if (wantedPreviewSize.width == supportedPreviewSizes[i].width && wantedPreviewSize.height ==  supportedPreviewSizes[i].height) {
				db_verb("find previewSizes(%d)(%d, %d)\n", i, wantedPreviewSize.width, wantedPreviewSize.height);
				break;
			} else {
				db_debug("previewSizes(%d, %d) NOT support\n", wantedPreviewSize.width, wantedPreviewSize.height);
			}
		}
		if (i == supportedVideoSizes.size()) {
			db_warn("wantedVideoSize(%d, %d) NOT supported by camera! USE default videoSize(%d, %d) of camera!\n", wantedVideoSize.width, wantedVideoSize.height, defaultVideoSize.width, defaultVideoSize.height);
			wantedVideoSize = defaultVideoSize;
		}
		// frameRate for preview and record
		for (i = 0; i < supportedFrameRates.size(); i++) {
			if (wantedFrameRate == supportedFrameRates[i]) {
				db_verb("find wantedFrameRate(%d)(%d)\n", i, wantedFrameRate);
				break;
			} else {
				db_debug("frameRate(%d)(%d) NOT support\n", i, wantedFrameRate);
			}
		}
		if (i == supportedFrameRates.size()) {
			db_warn("wantedFrameRate(%d) NOT support! USE default frameRate(%d) of camera\n", wantedFrameRate, defaultFrameRate);
			wantedFrameRate = defaultFrameRate;
		}
		HCParam.setVideoSize(wantedVideoSize.width, wantedVideoSize.height);
		HCParam.setPreviewSize(wantedPreviewSize.width, wantedPreviewSize.height);
		HCParam.setPreviewFrameRate(wantedFrameRate);
		mHC->setParameters(&HCParam);
	}
	if (nTestUvcFlag) {
		int fd = 0;								// A20 has 2 background video: UVC & TVIN
											// /dev/video0: CSI, this node always exists
											// /dev/video1: TVIN, this node always exists
											// /dev/video2: UVC, this node exists only when UVC is plugged in
		if (chipType == 0) {			// machine: V3
			if ((fd = open("/dev/video1", O_RDONLY)) > 0) {
				db_msg("open UVC(%d) of V3\n", fd);
				mUvcHC = HerbCamera::open(1);					// TODO: 1:Uvc(both V3 and A20 have this device); 2:TVIN(only A20 have)
				close(fd);
			} else {
				db_msg("UVC of V3 NOT FOUND!\n");
			}
		} else if (chipType == 1) { 		// machine: A20
			if ((fd = open("/dev/video2", O_RDONLY)) > 0) {
				db_msg("open UVC(%d) of A20\n", fd);
				mUvcHC = HerbCamera::open(1);					// UVC
				close(fd);
			} else if ((fd = open("/dev/video1", O_RDONLY)) > 0){
				db_msg("open TVIN(%d) of A20\n", fd);
				mUvcHC = HerbCamera::open(2);					// TVIN
				close(fd);
			} else {
				db_msg("Neither UVC nor TVIN of A20 was found!\n");
			}
		//	mUvcHC = HerbCamera::open(2);						// TODO: A20 has two kind of machine(one has Uvc, the other has TVIN)
		}										// here need other methods to distinguish them
		if (mUvcHC != NULL) {
			mUvcHC->getParameters(&uvcHCParam);
			uvcHCParam.dump();
			uvcHCParam.setPreviewSize(uvcPreviewSize.width, uvcPreviewSize.height);
			uvcHCParam.setVideoSize(uvcVideoSize.width, uvcVideoSize.height);
			uvcHCParam.setPreviewFrameRate(uvcFrameRate);
			mUvcHC->setParameters(&uvcHCParam);
		} else {
			nTestUvcFlag = 0;
		}
	}

	if (nPreviewEnableFlag) {
		unsigned int fb_w;
		unsigned int fb_h;
		int ffb;
		struct fb_var_screeninfo var;
		if ((ffb = open("/dev/graphics/fb0", O_RDWR)) < 0) {
			db_warn("open fb0 failed\n");
			goto _EXIT;
		}
		ioctl(ffb, FBIOGET_VSCREENINFO, &var);
		fb_w = var.xres;
		fb_h = var.yres;
		struct view_info sur;
		struct view_info sur2;
		sur.x = 0;
		sur.y = 190;			// check out y position!!!
		sur.w = fb_w/2 - 5;
		sur.h = fb_h - 190;		// check out sensor video's height!!!
		sur2.x = fb_w/2 + 5;
		sur2.y = 190;			// check out y position!!!
		sur2.w = fb_w/2 - 5;
		sur2.h = fb_h - 190;		// check out uvc video's height!!!

		db_msg("prepare startPreview\n");
		if (nTestSensorFlag) {
			mCD = new CedarDisplay(0);
			hlay = mCD->requestSurface(&sur);
			mHC->setPreviewDisplay(hlay);
			mHC->startPreview();
		}
		if (nTestUvcFlag) {
			mUvcCD = new CedarDisplay(1);
			hlay2 = mUvcCD->requestSurface(&sur2);
			mUvcHC->setPreviewDisplay(hlay2);
			mUvcHC->startPreview();
		}
		db_msg("prepare startPreview finish\n");
		if (0 == nRecordEnableFlag) {
			db_msg("keep preview forever!\n");
			while (1) {
				sleep(5);
			}
			db_msg("HerbCamera stopPreview!\n");
			mHC->stopPreview();
			mCD->releaseSurface(hlay);
			if (nTestUvcFlag) {
				mUvcHC->stopPreview();
				mUvcCD->releaseSurface(hlay2);
			}
		}
	}

_EXIT:
	db_warn("delete HerbCamera and HerbMediaRecorder\n");
	if (mHC) {
		db_msg("[Sensor] HerbCamera::release()\n");
		mHC->release();
		delete mHC;
		mHC == NULL;
	}
	if (mUvcHC) {
		db_msg("[Uvc] HerbCamera::release()\n");
		mUvcHC->release();
		mUvcHC = NULL;
	}
	if (mHMR) {
		delete mHMR;
		mHMR = NULL;
	}
	if (mUvcHMR) {
		delete mUvcHMR;
		mUvcHMR = NULL;
	}
_exit:
	IPCThreadState::self()->joinThreadPool();

	return NULL;

}

int MiniGUIMain(int argc, const char **argv)
{
	double length;
	int retVal;
	HWND hMainWnd;
	MSG Msg;

	system("dd if=/dev/zero of=/dev/graphics/fb0");

	DetectChipType();
	// read /mnt/extsd/DragonBoard/dragonborad.cfg and decide which one to test
	ConfigureParser();		// parser which testcase to display and it's path

	pthread_t tfTid, micTid, spkTid;
	pthread_t rtcTid, accTid, norTid, ddrTid;
	pthread_t wifiTid;

	db_msg("create xxxThreadRead to read fifo\n");
	if (testTf) {
		if (pthread_create(&tfTid, NULL, tfThreadRead, NULL) < 0) {	// TF
			db_warn("pthread_create for tf failed");
		}
	}
	if (testMic) {
		if (pthread_create(&micTid, NULL, micThreadRead, NULL) < 0) {	// MIC
			db_warn("pthread_create for mic failed");
		}
	}
	if (testSpk) {
		if (pthread_create(&spkTid, NULL, spkThreadRead, NULL) < 0) {	// SPEAKER
			db_warn("pthread_create for spk failed");
		}
	}
	if (testRtc) {
		if (pthread_create(&rtcTid, NULL, rtcThreadRead, NULL) < 0) {	// RTC
			db_warn("pthread_create for rtc failed");
		}
	}
	if (testAcc) {
		if (pthread_create(&accTid, NULL, accThreadRead, NULL) < 0) {	// G-sensor(Accerator)
			db_warn("pthread_create for sensor-acc failed");
		}
	}
	if (testNor) {
		if (pthread_create(&norTid, NULL, norThreadRead, NULL) < 0) {	// NOR
			db_warn("pthread_create for nor flash failed");
		}
	}
	if (testDdr) {
		if (pthread_create(&ddrTid, NULL, ddrThreadRead, NULL) < 0) {	// DDR
			db_warn("pthread_create for ddr failed");
		}
	}
	if (testWifi) {
		if (pthread_create(&wifiTid, NULL, wifiThreadRead, NULL) < 0) {	// WIFI
			db_warn("pthread_create for wifi failed");
		}
	}

	// this part also can be done by shell script, but here it is done by system() system-call 
	// TODO: notice testcase executable file's path, here I put them in TF card
	// So, it doesn't work when tf card slot trouble or tf card broken
	pthread_t startTfTesterTid, startMicTesterTid;
	pthread_t startRtcTesterTid, startAccTesterTid, startNorTesterTid, startDdrTesterTid;
	pthread_t startWifiTesterTid, startVideoTesterTid;
	db_msg("create startxxxThread to run testcase\n");
	if (testTf) {
		pthread_create(&startTfTesterTid, NULL, startTfTester, NULL);			// TF
	}
	if (testMic) {
		pthread_create(&startMicTesterTid, NULL, startMicTester, NULL);			// MIC
	}
	if (testRtc) {
		pthread_create(&startRtcTesterTid, NULL, startRtcTester, NULL);			// RTC
	}
	if (testAcc) {
		pthread_create(&startAccTesterTid, NULL, startAccTester, NULL);			// G-sensor
	}
	if (testNor) {
		pthread_create(&startNorTesterTid, NULL, startNorTester, NULL);			// NOR
	}
	if (testDdr) {
		pthread_create(&startDdrTesterTid, NULL, startDdrTester, NULL);			// DDR
	}
	if (testWifi) {
		pthread_create(&startWifiTesterTid, NULL, startWifiTester, NULL);		// WIFI
	}

	// NOTICE: dragonboard may crash because of this thread
	if (testVideo) {									// forground and background video
		pthread_create(&startVideoTesterTid, NULL, startVideoTester, NULL);		// VIDEO
	}
	// save LCD's width and height
	lcdWidth = GetGDCapability(HDC_SCREEN, GDCAP_MAXX) + 1;
	lcdHeight = GetGDCapability(HDC_SCREEN, GDCAP_MAXY) + 1;
	db_verb("LCD width & height: (%d, %d)\n", lcdWidth, lcdHeight);

	MAINWINCREATE CreateInfo = {
		WS_VISIBLE | WS_CAPTION,
		WS_EX_NOCLOSEBOX,
		"DragonBoard V0.0.1",
		0,
		GetSystemCursor(0),
		0,
		HWND_DESKTOP,
		HelloWinProc,
		0, 0, lcdWidth, 190,		// height: 190, the left(LCD_Height - 190) for Sensor and Uvc
		COLOR_transparent,
		0, 0};

	hMainWnd = CreateMainWindow(&CreateInfo);
	if (hMainWnd == HWND_INVALID) {
		db_error("CreateMainWindow error");
		return -1;
	}

	while (GetMessage(&Msg, hMainWnd)) {
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	MainWindowCleanup(hMainWnd);

	return 0;
}

#ifndef _MGRM_PROCESSES
#include <minigui/dti.c>
#endif
