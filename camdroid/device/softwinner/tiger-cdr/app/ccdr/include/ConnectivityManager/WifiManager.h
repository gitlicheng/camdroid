#ifndef _WIFI_MANAGER_H_
#define _WIFI_MANAGER_H_

#include <common/WifiApListItem.h>
#include <DhcpHandler.h>

#define WIFI_AP_LIST_MAX_SIZE 30

enum wifi_enable_state_t {
	WIFI_DISABLED,
	WIFI_ENABLING,
	WIFI_ENABLED,
	WIFI_DISABLING,
	WIFI_ENABLE_ERROR
};

enum wifi_link_state_t {
	WIFI_LINK_UP,
	WIFI_LINK_DOWN,
	WIFI_LINK_UNKNOWN
};

enum wifi_conn_state_t {
	WIFI_DISCONNECTED,
	WIFI_CONNECTING,
	WIFI_CONNECTED,
	WIFI_DISCONNECTING,
	WIFI_CONN_UNKNOWN
};

struct ConnApTarget {
	char ssid[64];
	char password[64];
	char encryption[16];
	int conning;
};

class WifiManager {
	
public:
	WifiManager(DhcpHandler *dhcpHdlr);
	~WifiManager();
	
	class WifiConnStateListener {
	public:
		WifiConnStateListener() {};
		virtual ~WifiConnStateListener() {};
		virtual int handleWifiConnState(void *cookie, wifi_conn_state_t state) = 0;
	};
	
	void setConnStateListener(void *cookie, WifiConnStateListener *pl);	
	
	class WifiApConnResultHandler {
	public:
		WifiApConnResultHandler() {};
		virtual ~WifiApConnResultHandler() {};
		virtual void handleWifiApConnResult(int result) = 0;
	};
	
	void setApConnResultHandler(WifiApConnResultHandler *hdlr);
	
	int init();
	
	void setConnEnable(int enable);
	int getConnEnable();
	
	int connect();
	int disconnect();
	
	int isConnectingAp();
	
	int getIp(char *ipstr);
	
	void updateEnableState(wifi_enable_state_t state);
	wifi_enable_state_t getEnableState();
	
	void updateLinkState(wifi_link_state_t state);
	wifi_link_state_t getLinkState();
	void handleLinkState(wifi_link_state_t state, int netId);
	
	void updateConnState(wifi_conn_state_t state);
	wifi_conn_state_t getConnState();
	
	void handleWifiEnableState(wifi_enable_state_t state);
	wifi_enable_state_t getWifiEnableState();
	void updateWifiEnableState(wifi_enable_state_t state);
	void updateWifiApList();
	
	int setEnable(int enable);
	int getEnable();
	
	int getApList(WifiApListItem **list);
	int connectAp(const char *ssid, const char *pw, const char *encryption);
	int getConnectedAp(char *ssid);
	int forgetAp(const char *ssid);
	
	int connectApTarget();
	
//	int mNetworkId;
	pthread_mutex_t mMutex;
	wifi_enable_state_t mEnableState;
	WifiApConnResultHandler *mApConnHandler;
	DhcpHandler *mDhcpHandler;
	void *mConnCookie;
	WifiConnStateListener *mConnListener;
	
	ConnApTarget mConnAp;
	pthread_mutex_t mConnApMutex;
	
private:
	int requestIp();
	
	int enableWifi();
	int disableWifi();
	
	int mConnEnable;
	wifi_link_state_t mLinkState;
	wifi_conn_state_t mConnState;
	
	pthread_t mEnableThreadId;
	pthread_t mDisableThreadId;
	pthread_t mConnThreadId;
};

#endif	//_WIFI_MANAGER_H_
