#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#define TAG "parser"
#include <dragonboard.h>

int  testKey = 1;
char testKeyPath[50] = "keytester";			// path name can NOT over 50 chars!!!
int  testTf = 1;
char testTfPath[50] = "tftester";
int  testMic = 1;
char testMicPath[50] = "mictester";
int  testSpk = 1;
char testSpkPath[50] = "spktester";
int  testRtc = 1;
char testRtcPath[50] = "rtctester";
int  testAcc = 1;
char testAccPath[50] = "acctester";
int  testNor = 1;
char testNorPath[50] = "nortester";
int  testDdr = 1;
char testDdrPath[50] = "ddrtester";
int  testWifi = 1;
char testWifiPath[50] = "wifitester";

int SetTestcase(const char* ptr1, const char* ptr2)
{	
	char name[10];
	char path[50];
	FILE* fp;

	if (strncmp(ptr1 + 1, "KEY", 3) == 0) {
		testKey = atoi(ptr1 + 8);
		return 0;
	} else if (strncmp(ptr1 + 1, "TF", 2) == 0) {
		testTf = atoi(ptr1 + 8);				// BUG: NOT "ptr1 + 8"!!! use other ways in dragonboard.cpp
		if (strncmp(ptr2, "path = ", 7) == 0) {
			strlcpy(path, strchr(ptr2, '"') + 1, strrchr(ptr2, '"') - strchr(ptr2, '"'));
			if ((fp = fopen(path, "r")) != NULL) {		// bin may NOT exist, use default
				strlcpy(testTfPath, path, 50);
				fclose(fp);
			}
		}
		return 0;
	} else if (strncmp(ptr1 + 1, "MIC", 3) == 0) {
		testMic = atoi(ptr1 + 8);
		if (strncmp(ptr2, "path = ", 7) == 0) {
			strlcpy(path, strchr(ptr2, '"') + 1, strrchr(ptr2, '"') - strchr(ptr2, '"'));
			if ((fp = fopen(path, "r")) != NULL) {		// bin may NOT exist, use default
				strlcpy(testMicPath, path, 50);
				fclose(fp);
			}
		}
		return 0;
	} else if (strncmp(ptr1 + 1, "SPK", 3) == 0) {
		testSpk = atoi(ptr1 + 8);
		return 0;
	} else if (strncmp(ptr1 + 1, "RTC", 3) == 0) {
		testRtc = atoi(ptr1 + 8);
		if (strncmp(ptr2, "path = ", 7) == 0) {
			strlcpy(path, strchr(ptr2, '"') + 1, strrchr(ptr2, '"') - strchr(ptr2, '"'));
			if ((fp = fopen(path, "r")) != NULL) {		// bin may NOT exist, use default
				strlcpy(testRtcPath, path, 50);
				fclose(fp);
			}
		}
		return 0;
	} else if (strncmp(ptr1 + 1, "ACC", 3) == 0) {
		testAcc = atoi(ptr1 + 8);
		if (strncmp(ptr2, "path = ", 7) == 0) {
			strlcpy(testAccPath, strchr(ptr2, '"') + 1, strrchr(ptr2, '"') - strchr(ptr2, '"'));
			if ((fp = fopen(path, "r")) != NULL) {		// bin may NOT exist, use default
				strlcpy(testAccPath, path, 50);
				fclose(fp);
			}
		}
		return 0;
	} else if (strncmp(ptr1 + 1, "NOR", 3) == 0) {
		testNor = atoi(ptr1 + 8);
		if (strncmp(ptr2, "path = ", 7) == 0) {
			strlcpy(path, strchr(ptr2, '"') + 1, strrchr(ptr2, '"') - strchr(ptr2, '"'));
			if ((fp = fopen(path, "r")) != NULL) {		// bin may NOT exist, use default
				strlcpy(testNorPath, path, 50);
				fclose(fp);
			}
		}
		return 0;
	} else if (strncmp(ptr1 + 1, "DDR", 3) == 0) {
		testDdr = atoi(ptr1 + 8);
		if (strncmp(ptr2, "path = ", 7) == 0) {
			strlcpy(path, strchr(ptr2, '"') + 1, strrchr(ptr2, '"') - strchr(ptr2, '"'));
			if ((fp = fopen(path, "r")) != NULL) {		// bin may NOT exist, use default
				strlcpy(testDdrPath, path, 50);
				fclose(fp);
			}
		}
		return 0;
	} else if (strncmp(ptr1 + 1, "WIFI", 4) == 0) {
		testWifi = atoi(ptr1 + 8);
		if (strncmp(ptr2, "path = ", 7) == 0) {
			strlcpy(path, strchr(ptr2, '"') + 1, strrchr(ptr2, '"') - strchr(ptr2, '"'));
			if ((fp = fopen(path, "r")) != NULL) {		// bin may NOT exist, use default
				strlcpy(testWifiPath, path, 50);
				fclose(fp);
			}
		}
		return 0;
	} else {
		snprintf(name, strchr(ptr1, ']') - ptr1, "%s", ptr1 + 1);
		db_warn("Unknown testcase: [%s]!\n", name);
		return -1;
	}
}


int main(void)
{
	FILE *fp;
	char nameBuf[15];		// NOTICE: make sure NOT over 15 chars for testcase name line!!!
	char pathBuf[100];		// NOTICE: make sure NOT over 100 chars for path every line!!!
	int retVal;
	char cmpbuf[10];
	char* pcfg = "/mnt/extsd/dragonboard.cfg";


	fp = fopen(pcfg, "r");
	if (fp == NULL) {
		db_warn("configure [%s] NOT found! enable all the testcase by default!\n", pcfg);
		return -1;
	} else {
		db_msg("parse the configure [%s], prepare for dragonboard!\n", pcfg);
	}

	while (fgets(nameBuf, 15, fp) != NULL) {
		if ((*nameBuf == '[') && ((strstr(nameBuf,"] = ")) != NULL)) {	// [KEY] = 1
			fgets(pathBuf, 100, fp);			// path = "xxx"
			retVal = SetTestcase(nameBuf, pathBuf);
		}
	}
	db_debug("testKey = %d, path = %s\n", testKey, testKeyPath);
	db_debug("testTf = %d, path = %s\n", testTf, testTfPath);
	db_debug("testMic = %d, path = %s\n", testMic, testMicPath);
	db_debug("testRtc = %d, path = %s\n", testRtc, testRtcPath);
	db_debug("testAcc = %d, path = %s\n", testAcc, testAccPath);
	db_debug("testNor = %d, path = %s\n", testNor, testNorPath);
	db_debug("testDdr = %d, path = %s\n", testDdr, testDdrPath);
	db_debug("testWifi = %d, path = %s\n", testWifi, testWifiPath);

	fclose(fp);
	return 0;
}
