#define LOG_NDEBUG 0
#define LOG_TAG "EthernetManager"
#include <cutils/log.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/sockios.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
//#include <netutils/ifc.h>
//#include <netutils/dhcp.h>
#include <cutils/properties.h>
#include <EthernetManager.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define ETHTOOL_GLINK 0x0000000a /* Get link status (ethtool_value) */
    
typedef signed int u32;

/* for passing single values */
struct ethtool_value {  
    u32 cmd;
    u32 data;
};

#define BUFLEN 20480

static eth_link_state_t interface_detect_beat_ethtool(int fd, char *iface) {  
    struct ifreq ifr;
    struct ethtool_value edata;
    
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface, sizeof(ifr.ifr_name) - 1);
    
    edata.cmd = ETHTOOL_GLINK;
    ifr.ifr_data = (caddr_t) &edata;
    
    if (ioctl(fd, SIOCETHTOOL, &ifr) == -1) {
        ALOGE("ETHTOOL_GLINK failed.");
        return ETH_LINK_UNKNOWN;
    }
    
    return edata.data ? ETH_LINK_UP : ETH_LINK_DOWN;
}

static eth_link_state_t getEthLinkState() {
    FILE *fp;
    eth_link_state_t state;
    char buf[512] = {'\0'};
    char hw_name[10] = {'\0'};
    char *token = NULL;
    int fd;
    
    fp = fopen("/proc/net/dev", "r");
    if(!fp) {
    	ALOGE("Open /proc/net/dev error.");
    	return ETH_LINK_UNKNOWN;
    }
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        if(strstr(buf, "eth") != NULL) {
            token = strtok(buf, ":");
            while (*token == ' ') ++token;
            strncpy(hw_name, token, strlen(token));
        }
    }
    fclose(fp);
    
    if((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        ALOGE("Create socket error.");
        return ETH_LINK_UNKNOWN;
    }
    state = interface_detect_beat_ethtool(fd, hw_name);
    close(fd);
    
    return state;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

EthernetManager::EthernetManager(DhcpHandler *dhcpHdlr)
		: mConnCookie(NULL), mConnListener(NULL), mConnState(ETH_CONN_UNKNOWN),
		mLinkThreadId(0), mConnThreadId(0) {
	mDhcpHandler = dhcpHdlr;
	pthread_mutex_init(&mMutex, NULL);
}

EthernetManager::~EthernetManager() {
	pthread_mutex_destroy(&mMutex);
	mDhcpHandler = NULL;
}

void EthernetManager::setConnStateListener(void *cookie, EthernetConnStateListener *pl) {
	mConnCookie = cookie;
	mConnListener = pl;
}

static void *ethernetLinkThread(void *p_ctx) {
	EthernetManager *p_this = (EthernetManager *)p_ctx;
	
	int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if(fd == -1) {
		ALOGE("Create netlink socket error.");
		return NULL;
    }
        
    int len = BUFLEN;
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &len, sizeof(len));
    struct sockaddr_nl addr;
    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = RTNLGRP_LINK;
    bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    
    int retval;
    char buf[BUFLEN] = {0};
    struct nlmsghdr *nh;
    struct ifinfomsg *ifinfo;
    struct rtattr *attr;
    while((retval = read(fd, buf, BUFLEN)) > 0) {
        ALOGV("ethernetLinkThread buf=%s",buf);
        for(nh = (struct nlmsghdr *)buf; NLMSG_OK(nh, (unsigned int)retval); nh = NLMSG_NEXT(nh, retval)) {
            if(nh->nlmsg_type == NLMSG_DONE) {
                break;
            }
            if(nh->nlmsg_type == NLMSG_ERROR) {
                ALOGE("Get message error.");
                continue;
            }
            if (nh->nlmsg_type != RTM_NEWLINK) {
                continue;
            }
            ifinfo = (struct ifinfomsg *)NLMSG_DATA(nh);
            //ALOGV("ifinfo->ifi_flags = %d\n",ifinfo->ifi_flags);
            
            //ignore wireless link event.
            attr = (struct rtattr*)(((char*)nh) + NLMSG_SPACE(sizeof(*ifinfo)));
            len = nh->nlmsg_len - NLMSG_SPACE(sizeof(*ifinfo));
            for(; RTA_OK(attr, len); attr = RTA_NEXT(attr, len)) {
            	if(attr->rta_type == IFLA_IFNAME) {
            		break;
            	}
            }
    		if(strcmp("eth0", (char*)RTA_DATA(attr)) != 0) {
    			//ALOGV("Ignore link event of: %s", (char*)RTA_DATA(attr));
    			continue;
    		}
            
            if((ifinfo->ifi_flags & IFF_LOWER_UP)!= 0) {
            	ALOGV("Ethernet link up.");
            	p_this->handleLinkState(ETH_LINK_UP);
           	}
           	else {
           		ALOGV("Ethernet link down.");
           		p_this->handleLinkState(ETH_LINK_DOWN);
           	}
        }
    }
    
    close(fd);
    
    ALOGE("Ethernet link thread exit.");
    return NULL;
}

int EthernetManager::init() {
	int err = pthread_create(&mLinkThreadId, NULL, ethernetLinkThread, (void *)this);
	if(err || !mLinkThreadId) {
		ALOGE("Create ethernet link thread error.");
		return -1;
	}
	
    eth_link_state_t state = getEthLinkState();
    ALOGV("Ethlink init state = %d", state);
    if(ETH_LINK_UNKNOWN == state) {
    	ALOGE("Ethlink init state error.");
    	return -1;
    }
    
    if(ETH_LINK_UP == state) {
    	requestIp();
    	return 0;
    }
    
    updateConnState(ETH_DISCONNECTED);
    return 0;
}

void EthernetManager::handleLinkState(eth_link_state_t state) {
	ALOGV("handleLinkState()..state = %d", state);
	if(ETH_LINK_DOWN == state) {
		updateConnState(ETH_DISCONNECTED);
		if(mConnListener != NULL) {
			mConnListener->handleEthernetConnState(mConnCookie, ETH_DISCONNECTED);
		}
		
		mDhcpHandler->ethernetReleaseIp();
		
//		int ret = dhcp_stop("eth_eth0");
//		ALOGV("After dhcp_stop(eth0), ret = %d", ret);
//		ret = ifc_clear_addresses("eth0");
//		ALOGV("After ifc_clear_addresses(eth0), ret = %d", ret);
		
		return;
	}
	
	if(ETH_LINK_UP == state) {
		requestIp();
		return;
	}
	
	ALOGE("Unknown eth link state.");
	return;
}

void EthernetManager::updateConnState(eth_conn_state_t state) {
	ALOGV("updateConnState()..mConnState = %d, state = %d", mConnState, state);
	pthread_mutex_lock(&mMutex);
	if(mConnState == state) {
		pthread_mutex_unlock(&mMutex);
		return;
	}
	mConnState = state;
	pthread_mutex_unlock(&mMutex);
}

eth_conn_state_t EthernetManager::getConnState() {
	eth_conn_state_t state = ETH_CONN_UNKNOWN;
	pthread_mutex_lock(&mMutex);
	state = mConnState;
	pthread_mutex_unlock(&mMutex);
	return state;
}

static void *ethernetConnThread(void *p_ctx) {
	EthernetManager *p_this = (EthernetManager *)p_ctx;
	
	eth_conn_state_t state = p_this->getConnState();
	if(ETH_CONNECTING != state) {
		return NULL;
	}
/*
	char ipstr[32];
	char gateway[32];
    uint32_t prefixLength;
    char dns1[32];
    char dns2[32];
    char server[32];
    uint32_t lease;
    char vendorInfo[32];
*/
//	dhcp_stop("eth_eth0");

	if(p_this->mDhcpHandler->ethernetRequestIp() != 0) {
        ALOGE("ethernetRequestIp() != 0");
		return NULL;
	}

//	int ret = dhcp_do_request("eth_eth0", ipstr, gateway, &prefixLength, dns1, dns2, server, &lease, vendorInfo);
//	ALOGV("After request ip, ret = %d, ipstr = %s", ret, ipstr);
//	if(ret != 0) {
//		ALOGE("Ethernet request ip error.");
//		return NULL;
//	}
	
	state = p_this->getConnState();
	if(ETH_CONNECTING != state) {
		return NULL;
	}
	
	ALOGV("Ethernet connected.");
    p_this->updateConnState(ETH_CONNECTED);
	if(p_this->mConnListener != NULL) {
		p_this->mConnListener->handleEthernetConnState(p_this->mConnCookie, ETH_CONNECTED);
	}
	
    return NULL;
}

int EthernetManager::requestIp() {
	updateConnState(ETH_CONNECTING);
	
	int err = pthread_create(&mConnThreadId, NULL, ethernetConnThread, (void *)this);
	if(err || !mConnThreadId) {
		ALOGE("Create ethernet conn thread error.");
		updateConnState(ETH_DISCONNECTED);
		if(mConnListener != NULL) {
			mConnListener->handleEthernetConnState(mConnCookie, ETH_DISCONNECTED);
		}
		return -1;
	}
	
	return 0;
}

int EthernetManager::getIp(char *ipstr) {	
    property_get("dhcp.eth0.ipaddress", ipstr, NULL);
	ALOGV("Get eth ip = %s", ipstr);
	return 0;
}
