#ifndef _WIFI_AP_LIST_ITEM_H_
#define _WIFI_AP_LIST_ITEM_H_

struct WifiApListItem {
	char ssid[32];
	char statement[128];
	char encryption[32];
	int level;
	int isSaved;
	struct WifiApListItem *next;
};

#endif	//_WIFI_AP_LIST_ITEM_H_
