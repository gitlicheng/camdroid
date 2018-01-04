#ifndef _CONNECTIVITY_MANAGER_H_
#define _CONNECTIVITY_MANAGER_H_

//#include <EthernetManager.h>
#include <WifiManager.h>
#include <SoftApEnabler.h>

enum conn_channel_t {
	CONN_CHANNEL_ETHERNET = 0,
	CONN_CHANNEL_WIFI,
	CONN_CHANNEL_SOFTAP,
	CONN_CHANNEL_NONE
};

enum conn_msg_t {
	CONN_UNAVAILABLE = 0,
	CONN_AVAILABLE,
	CONN_CHANNEL_CHANGE
};

class ConnectivityManager: 
		WifiManager::WifiConnStateListener, SoftApEnabler::SofApEnableListener {	//private EthernetManager::EthernetConnStateListener,
	
public:
	ConnectivityManager();
	~ConnectivityManager();
	
	class ConnectivityListener {
	public:
		ConnectivityListener() {};
		virtual ~ConnectivityListener() {};
		virtual void handleConnectivity(void *cookie, conn_msg_t msg) = 0;
	};
	
	void setConnectivityListener(void *cookie, ConnectivityListener *connl);
	
	int start();
	
	int getIp(char *ipstr);
	
	void setWifiApConnResultHandler(WifiManager::WifiApConnResultHandler *hdlr);
	int setWifiEnable(unsigned int enable);
	int getWifiEnable(unsigned int *enable);
	int getWifiApList(WifiApListItem **plist);
	int connectWifiAp(const char *ssid, const char *pw, const char *encryption);
	int getConnectedWifiAp(char *ssid);
	int forgetWifiAp(const char *ssid);
	int setPwd(const char *old_pwd, const char *new_pwd);
	int fResetWifiPWD(const char *pwd);

private:
	void updateConnChannel(conn_channel_t channel);
	conn_channel_t getConnChannel();
//	int handleEthernetConnState(void *cookie, eth_conn_state_t state);
	int handleWifiConnState(void *cookie, wifi_conn_state_t state);
	void onSoftApEnableChanged(void *cookie, int enable);
	
	DhcpHandler *mDhcpHandler;
//	EthernetManager *mEthernetManager;
	WifiManager *mWifiManager;
	SoftApEnabler *mSoftApEnabler;
	
	conn_channel_t mConnChannel;
	pthread_mutex_t mMutex;
	
	void *mCbCookie;
	ConnectivityListener *mConnListener;
};

#endif	//_CONNECTIVITY_MANAGER_H_
