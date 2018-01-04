#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <hardware_legacy/wifi.h>
#include <hardware_legacy/wifi_api.h>

#define TAG "wifitester"
#include <dragonboard.h>

#define IFNAME          "wlan0"
#define BUF_SIZE        256
#define EVENT_BUF_SIZE  2048
#define FIFO_DEV        "fifo_wifi"

int fifoFd;

void HandleWifiEvent(event_t event, int arg, void* cookie) {
	int i;
	char buf[50];				// V3 width 320 pixels, which is 40 * 8(pixels per char)
	wifi_scanlist result_list;
	switch(event) {
		case EVENT_SCAN_RESULT:
			getWifiApList(&result_list);
//			for (i = 0; i < 2; i++) {
//				printf("ssid:[%s],\tlevel:[%s]\n", result_list.list[i].ssid, result_list.list[i].level);
//			}
//			puts("");
			if (result_list.list_count == 0) {
				write(fifoFd, "P[WIFI] there is no hotspot here", 50);	// no hotspot but wifi is OK
				setWifiEnable(0);	// disable wifi
				setWifiEnable(1);	// enable wifi
			} else if (result_list.list_count == 1) {			// 1 hotspot in local
				strlcpy(buf, "P[WIFI] ", 9);
				if (strlen(result_list.list[0].ssid) > 25) {		// NOTICE: do NOT use strlcat!
					strncat(buf, result_list.list[0].ssid, 26);	// cat 25 chars
					strncat(buf, ":", 2);				// cat 1 chars (not include '\0')
					strncat(buf, result_list.list[0].level, 4); 	// cat 3 chars
					strncat(buf, "dBm", 4);				// cat 3 chars
				} else {
					strncat(buf, result_list.list[0].ssid, 26);	// cat 25 at most
					strncat(buf, ":", 2);
					strncat(buf, result_list.list[0].level, 4);	// 3 + length('\0') = 4
					strncat(buf, " dBm", 5);
				}
				write(fifoFd, buf, 50);
			} else {							// more than 1 hotspot in local
				strlcpy(buf, "P[WIFI] 1.", 11);				// strlen(ssid1) + strlen(ssid2) = 12
				if (strlen(result_list.list[0].ssid) + strlen(result_list.list[1].ssid) > 12) {
					if (strlen(result_list.list[0].ssid) > 5) {
						strncat(buf, result_list.list[0].ssid, 6);	// cat 6 chars, NOT 5
						strncat(buf, ":", 2);				// cat 1 char, then '\0'
						strncat(buf, result_list.list[0].level, 4);
						strncat(buf, "dBm  2.", 8);
						strncat(buf, result_list.list[1].ssid, 6);
						strncat(buf, ":", 2);
						strncat(buf, result_list.list[1].level, 4);
						strncat(buf, "dBm", 4);
					} else {
						strncat(buf, result_list.list[0].ssid, strlen(result_list.list[0].ssid));
						strncat(buf, ":", 2);
						strncat(buf, result_list.list[0].level, 4);
						strncat(buf, "dBm  2.", 8);
						strncat(buf, result_list.list[1].ssid, 12 - strlen(result_list.list[0].ssid));
						strncat(buf, ":", 2);
						strncat(buf, result_list.list[1].level, 4);
						strncat(buf, "dBm", 4);
					}
				} else {
						strncat(buf, result_list.list[0].ssid, strlen(result_list.list[0].ssid));
						strncat(buf, ":", 2);
						strncat(buf, result_list.list[0].level, 4);
						strncat(buf, "dBm  2.", 8);
						strncat(buf, result_list.list[1].ssid, strlen(result_list.list[0].ssid));
						strncat(buf, ":", 2);
						strncat(buf, result_list.list[1].level, 4);
						strncat(buf, "dBm", 4);
				}
				write(fifoFd, buf, 50);
			}
//			db_debug("[send to fifo][%s]\n", buf);
			break;
		default:
			db_debug("Unknow event");
			break;
	}
}

int main(int argc, char** argv)
{
	int retVal, i;
	wifi_setCallBack(HandleWifiEvent, NULL);

	if ((fifoFd = open(FIFO_DEV, O_WRONLY)) < 0) {
		if (mkfifo(FIFO_DEV, 0666) < 0) {
			db_error("mkfifo failed(%s)\n", strerror(errno));
			return -1;
		} else {
			fifoFd = open(FIFO_DEV, O_WRONLY);
		}
	}

	retVal = setWifiEnable(1);	// enable wifi
	if (retVal == 0) {
		db_error("open wifi failed\n");
		write(fifoFd, "F[WIFI]", 8);
		close(fifoFd);
		return -1;
	} else {
		db_msg("open wifi success\n");
	}

//	i = 0;
	while (1) {
		android_wifi_scan();
		sleep(10);
//		db_msg("detect wifi scan counter: [%d]\n", i++);
	}

	return 0;
}
