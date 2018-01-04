/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _WIFI_API_H
#define _WIFI_API_H

#if __cplusplus
extern "C" {
#endif

#define BOOL int
#define FALSE 0
#define TRUE 1

#define SCAN_RESULT_RESULT_MAX 64
#define CONNECTED_NETWORK_MAX 30


#define STATE_CHANGE       "CTRL-EVENT-STATE-CHANGE"
#define SCAN_NEW_BSS       "CTRL-EVENT-BSS-ADDED"
#define SCAN_RESULT        "CTRL-EVENT-SCAN-RESULTS"
#define BSS_CONNECTED      "CTRL-EVENT-CONNECTED"
#define BSS_DISCONNECTED   "CTRL-EVENT-DISCONNECTED"


/**
 * android_wifi_enable
 *
 * @return 0 on success, < 0 on failure.
 */
int android_wifi_enable();

/**
 * android_wifi_doBooleanCommand
 *
 * @return 1 on success, 0 on failure.
 */
BOOL android_wifi_doBooleanCommand( char * cmd);

/**
 * android_wifi_doIntCommand
 *
 * @return  -1 on failure.other 
 */
int android_wifi_doIntCommand(char * cmd);

/**
 * android_wifi_doStringCommand
 *
 * @return  reply==NULL FAIL
 * suggest reply buffer size 4096
 */
char* android_wifi_doStringCommand( char* reply,int replybuflen,char * cmd);

/**
 * android_wifi_scan
 *
 * @return  1 SUCCESS  0 FIAL
 * 
 */
BOOL android_wifi_scan();

 /**
     * Format of results:
     * =================
     * bssid=68:7f:74:d7:1b:6e
     * freq=2412
     * level=-43
     * tsf=1344621975160944
     * age=2623
     * flags=[WPA2-PSK-CCMP][WPS][ESS]
     * ssid=zubyb
     *
     * RANGE=ALL gets all scan results
     * MASK=<N> see wpa_supplicant/src/common/wpa_ctrl.h for details
     *///replybuf size suggest 4096
char* android_wifi_scanResults(char* replybuf,int bufsize);

char* android_wifi_getStatus(char* replybuf,int bufsize);


char* android_wifi_getMacAddress(char* replybuf,int bufsize);

BOOL android_wifi_ping();

char* android_wifi_listNetworks(char* replybuf,int bufsize); 

int android_wifi_addNetwork();

BOOL android_wifi_setNetworkVariable(int netId, char* name, char* value);

BOOL android_wifi_removeNetwork(int netId);


BOOL android_wifi_enableNetwork(int netId);

BOOL android_wifi_disableNetwork(int netId);


BOOL android_wifi_reconnect();


BOOL android_wifi_reassociate();


BOOL android_wifi_disconnect();
/**
 * android_wifi_waitForEvent
 *
 * @return  reply==NULL FAIL
 * suggest reply buffer size 2048
 */

char * android_wifi_waitForEvent(char* replybuf,int bufsize);

//return value 1 true ; 0 false
int android_wifi_disable();

int android_wifi_request_ip(int * ip);

char *  android_wifi_get_ip(char *buf);

int saveWifiConfig();

typedef struct {
char bssid[32];
char freq[6];
char level[6];
char tsf[21];
char flags[63];
char ssid[32];
}wifi_scanitem;

typedef struct 
{
int list_count;
wifi_scanitem list[SCAN_RESULT_RESULT_MAX];
}wifi_scanlist;

typedef struct {
char network_id[5];
char ssid[32];
char bssid[32];
char flags[63];
}wifi_connected_item;


typedef struct 
{
wifi_connected_item connected_list[CONNECTED_NETWORK_MAX];
int list_count;
}wifi_connected_list;

typedef enum 
{
EVENT_SCAN_RESULT=0,
EVENT_CONNECTED,
EVENT_DISCONNECTED,
EVENT_WRONG_PSK,
EVENT_DISCONNECTED_BY_USER
}event_t;

typedef enum 
{
STATUS_UNKNOW=0,
STATUS_SCANING,
STATUS_CONNECTING,
STATUS_CONNECTED,
STATUS_DISCONNECTED
}wifi_status;


/*返回值为当前添加的网络号，失败则返回-1 encryption是加密方式 ，WEP或WPA/WPA2或OPEN  */
int connectWifiAp(const char *ssid, const char *pw,const char *Encryption);

/*成功则返回1，失败则返回 0*/
int setWifiEnable(int enable);
int getWifiEnable(unsigned int *enable);
int getWifiApList(wifi_scanlist *result_list);
int getConfiguredWifiApList(wifi_connected_list *list);
int disconnectAp(int networkid);
/*在ENABLE成功后获取当前wifi状态*/
wifi_status getWifiStatus();

/*删除一个网络， netId：网络ID号*/
int forgetWifiAp(int netId);

/*由网络ID号，获取ssid*/
char *wifi_getNetworkSsid(char *ssid,int size ,int netId);
/*由网络ID号，获取bssid*/
char *wifi_getNetworkBssid(char *bssid,int size ,int netId);

typedef void (*wifiCallBack)(event_t event,int arg,void *cookie);
int wifi_setCallBack(wifiCallBack pHandler,void * cookies);

#if __cplusplus
};  // extern "C"
#endif

#endif  // _WIFI_API_H