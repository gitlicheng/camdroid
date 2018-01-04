#define LOG_NDEBUG 1
#define LOG_TAG "DhcpHandler"
#include <cutils/log.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netutils/ifc.h>
#include <netutils/dhcp.h>
#include <cutils/properties.h>
#include <hardware_legacy/wifi_api.h>
#include <DhcpHandler.h>

DhcpHandler::DhcpHandler() {
	pthread_mutex_init(&mMutex, NULL);
}

DhcpHandler::~DhcpHandler() {
	pthread_mutex_destroy(&mMutex);
}

int DhcpHandler::ethernetRequestIp() {
	ALOGV("Enter ethernetRequestIp()..");
	pthread_mutex_lock(&mMutex);
	char ipstr[32];
	char gateway[32];
    uint32_t prefixLength;
    char dns1[32];
    char dns2[32];
    char server[32];
    uint32_t lease;
    char vendorInfo[32];
    
	int ret = dhcp_do_request("eth_eth0", ipstr, gateway, &prefixLength, dns1, dns2, server, &lease, vendorInfo);
	ALOGV("Ethernet request ip, ret = %d, ipstr = %s, gateway = %s", ret, ipstr, gateway);
	if(ret != 0) {
		pthread_mutex_unlock(&mMutex);
		ALOGE("Ethernet request ip error.");
		return -1;
	}
	pthread_mutex_unlock(&mMutex);
	ALOGV("Exit ethernetRequestIp()..");
	return 0;
}

int DhcpHandler::ethernetReleaseIp() {
	ALOGV("Enter ethernetReleaseIp()..");
	pthread_mutex_lock(&mMutex);
	int ret = dhcp_stop("eth_eth0");
	ALOGV("After dhcp_stop(eth0), ret = %d", ret);
	ret = ifc_clear_addresses("eth0");
	ALOGV("After ifc_clear_addresses(eth0), ret = %d", ret);
	pthread_mutex_unlock(&mMutex);
	ALOGV("Exit ethernetReleaseIp()..");
	return 0;
}

int DhcpHandler::wifiRequestIp() {
	ALOGV("Enter wifiRequestIp()..");
	pthread_mutex_lock(&mMutex);
	
	char ipstr[32] = {0};
	int ip = 0;
	int ret = android_wifi_request_ip(&ip);
	android_wifi_get_ip(ipstr);
	ALOGV("Wifi request ip, ret = %d, ip = %d, ipstr = %s", ret, ip, ipstr);
	if(ret != 0) {
		pthread_mutex_unlock(&mMutex);
		ALOGE("Wifi request ip error.");
		return -1;
	}
	
//	char ipstr[32];
//	char gateway[32];
//    uint32_t prefixLength;
//    char dns1[32];
//    char dns2[32];
//    char server[32];
//    uint32_t lease;
//    char vendorInfo[32];
//    
//	int ret = dhcp_do_request("wlan0", ipstr, gateway, &prefixLength, dns1, dns2, server, &lease, vendorInfo);
//	ALOGV("Wifi request ip, ret = %d, ipstr = %s, gateway = %s", ret, ipstr, gateway);
//	if(ret != 0) {
//		pthread_mutex_unlock(&mMutex);
//		ALOGE("Wifi request ip error.");
//		return -1;
//	}
	pthread_mutex_unlock(&mMutex);
	ALOGV("Exit wifiRequestIp()..");
	return 0;
}

int DhcpHandler::wifiReleaseIp() {
	ALOGV("Enter wifiReleaseIp()..");
	pthread_mutex_lock(&mMutex);
	int ret = dhcp_stop("wlan0");
	ALOGV("After dhcp_stop(wlan0), ret = %d", ret);
	ret = ifc_clear_addresses("wlan0");
	ALOGV("After ifc_clear_addresses(wlan0), ret = %d", ret);
	pthread_mutex_unlock(&mMutex);
	ALOGV("Exit wifiReleaseIp()..");
	return 0;
}

//int DhcpHandler::getIp(char *netName, char *ipstr) {
//	
//}
