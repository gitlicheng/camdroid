#ifndef _ETHERNET_MANAGER_H_
#define _ETHERNET_MANAGER_H_

#include <DhcpHandler.h>

typedef enum {
	ETH_LINK_DOWN,
	ETH_LINK_UP,
	ETH_LINK_UNKNOWN
}eth_link_state_t;

typedef enum {
	ETH_DISCONNECTED,
	ETH_CONNECTING,
	ETH_CONNECTED,
	ETH_DISCONNECTING,
	ETH_CONN_UNKNOWN
}eth_conn_state_t;

class EthernetManager {
	
public:
	EthernetManager(DhcpHandler *dhcpHdlr);
	~EthernetManager();
	
	class EthernetConnStateListener {
	public:
		EthernetConnStateListener() {};
		virtual ~EthernetConnStateListener() {};
		virtual int handleEthernetConnState(void *cookie, eth_conn_state_t state) = 0;
	};
	
	void setConnStateListener(void *cookie, EthernetConnStateListener *pl);
	
	int init();
	
	int start();
	
	int getIp(char *ipstr);
	
	eth_conn_state_t getConnState();
	
	void handleLinkState(eth_link_state_t state);
	void updateConnState(eth_conn_state_t state);
	
	DhcpHandler *mDhcpHandler;
	void *mConnCookie;
	EthernetConnStateListener *mConnListener;
	
private:
	int requestIp();
	
	eth_conn_state_t mConnState;
	
	pthread_mutex_t mMutex;
	pthread_t mLinkThreadId;
	pthread_t mConnThreadId;
	
};

#endif	//_ETHERNET_MANAGER_H_
