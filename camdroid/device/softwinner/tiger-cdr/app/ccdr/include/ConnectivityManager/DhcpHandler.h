#ifndef _DHCP_HANDLER_H_
#define _DHCP_HANDLER_H_

class DhcpHandler {
	
public:
	DhcpHandler();
	~DhcpHandler();
	
	int ethernetRequestIp();
	
	int ethernetReleaseIp();
	
	int wifiRequestIp();
	
	int wifiReleaseIp();
	
//	int getIp(char *netName, char *ipstr);
	
private:
	pthread_mutex_t mMutex;
};

#endif	//_DHCP_HANDLER_H_
