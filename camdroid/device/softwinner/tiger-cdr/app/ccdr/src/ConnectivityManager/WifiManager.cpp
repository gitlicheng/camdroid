#define LOG_NDEBUG 1
#define LOG_TAG "WifiManager"
#include <cutils/log.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <hardware_legacy/wifi_api.h>
#include <cutils/properties.h>
//#include <netutils/ifc.h>
//#include <netutils/dhcp.h>
#include <ConfigData.h>
#include <WifiManager.h>

WifiManager::WifiManager(DhcpHandler *dhcpHdlr)
		: mEnableState(WIFI_DISABLED), mApConnHandler(NULL), mConnCookie(NULL), mConnListener(NULL), mConnEnable(0),
		mLinkState(WIFI_LINK_UNKNOWN), mConnState(WIFI_CONN_UNKNOWN), mEnableThreadId(0), mDisableThreadId(0), mConnThreadId(0) {
	mDhcpHandler = dhcpHdlr;
	memset(&mConnAp, 0, sizeof(ConnApTarget));
	pthread_mutex_init(&mMutex, NULL);
	pthread_mutex_init(&mConnApMutex, NULL);
}

WifiManager::~WifiManager() {
	pthread_mutex_destroy(&mConnApMutex);
	pthread_mutex_destroy(&mMutex);
	mDhcpHandler = NULL;
}

void WifiManager::setConnStateListener(void *cookie, WifiConnStateListener *pl) {
	mConnCookie = cookie;
	mConnListener = pl;
}

static void handleWifiEvent(event_t event, int arg, void *cookie) {
	WifiManager *p_this = (WifiManager *)cookie;
	if(NULL == p_this) {
		ALOGE("handleWifiEvent()..null cookie.");
		return;
	}
	
	ALOGV("handleWifiEvent()..event = %d, arg = %d", event, arg);
	switch(event) {
	case EVENT_SCAN_RESULT:
		break;
	case EVENT_CONNECTED:
		p_this->updateLinkState(WIFI_LINK_UP);
		p_this->handleLinkState(WIFI_LINK_UP, arg);
		break;
	case EVENT_DISCONNECTED:
		p_this->updateLinkState(WIFI_LINK_DOWN);
		p_this->handleLinkState(WIFI_LINK_DOWN, arg);
		break;
	default:
		ALOGE("Unknown event.");
		break;
	}
}

int WifiManager::init() {
	wifi_setCallBack(handleWifiEvent, (void *)this);
	return 0;
}

static void *wifiEnableThread(void *p_ctx) {
	WifiManager *p_this = (WifiManager *)p_ctx;
	
	pthread_mutex_lock(&p_this->mMutex);
	if(WIFI_ENABLING != p_this->mEnableState) {
		pthread_mutex_unlock(&p_this->mMutex);
		return NULL;
	}
	
	int ret = setWifiEnable(1);
	if(ret != 1) {
		ALOGE("Enable wifi error.");
		p_this->mEnableState = WIFI_ENABLE_ERROR;
		pthread_mutex_unlock(&p_this->mMutex);
		
		pthread_mutex_lock(&p_this->mConnApMutex);
		p_this->mConnAp.conning = 0;
		pthread_mutex_unlock(&p_this->mConnApMutex);
		
		p_this->updateConnState(WIFI_DISCONNECTED);
	    if(p_this->mApConnHandler != NULL) {
			p_this->mApConnHandler->handleWifiApConnResult(-1);
			p_this->mApConnHandler = NULL;
	    }
		if(p_this->mConnListener != NULL) {
			p_this->mConnListener->handleWifiConnState(p_this->mConnCookie, WIFI_DISCONNECTED);
		}
		
		return NULL;
	}
	
	ALOGV("Wifi enabled.");
	p_this->mEnableState = WIFI_ENABLED;
	pthread_mutex_unlock(&p_this->mMutex);
	
	p_this->connectApTarget();
	
	return NULL;
}

static void *wifiDisableThread(void *p_ctx) {
	WifiManager *p_this = (WifiManager *)p_ctx;
	
	pthread_mutex_lock(&p_this->mMutex);
	if(WIFI_DISABLING != p_this->mEnableState) {
		pthread_mutex_unlock(&p_this->mMutex);
		return NULL;
	}
	int ret = setWifiEnable(0);
	if(ret != 1) {
		ALOGE("Disable wifi error.");
		p_this->mEnableState = WIFI_ENABLE_ERROR;
		pthread_mutex_unlock(&p_this->mMutex);
		return NULL;
	}
	ALOGV("Wifi disabled.");
	p_this->mEnableState = WIFI_DISABLED;
	pthread_mutex_unlock(&p_this->mMutex);
	
	return NULL;
}

void WifiManager::updateLinkState(wifi_link_state_t state) {
	pthread_mutex_lock(&mMutex);
	if(mLinkState == state) {
		pthread_mutex_unlock(&mMutex);
		return;
	}
	mLinkState = state;
	pthread_mutex_unlock(&mMutex);
}

wifi_link_state_t WifiManager::getLinkState() {
	wifi_link_state_t state = WIFI_LINK_UNKNOWN;
	pthread_mutex_lock(&mMutex);
	state = mLinkState;
	pthread_mutex_unlock(&mMutex);
	return state;
}

void WifiManager::setConnEnable(int enable) {
	pthread_mutex_lock(&mMutex);
	mConnEnable = enable;
	pthread_mutex_unlock(&mMutex);
}

int WifiManager::getConnEnable() {
	int enable;
	pthread_mutex_lock(&mMutex);
	enable = mConnEnable;
	pthread_mutex_unlock(&mMutex);
	return enable;
}

int WifiManager::connect() {
	ALOGV("connect()..000000000000000");
	if(!getConnEnable()) {
		return -1;
	}
	ALOGV("connect()..111111111111111");
	if(WIFI_DISCONNECTING == getConnState()) {
		return -1;
	}
	ALOGV("connect()..222222222222222");
	if(WIFI_CONNECTING == getConnState() || WIFI_CONNECTED == getConnState()) {
		return 0;
	}
	ALOGV("connect()..333333333333333");
	int valid = 0;
	cfgDataGetInt("ipc2.wifi.valid", &valid, 0);
	if(valid) {
		if(WIFI_LINK_UP == getLinkState()) {
			return requestIp();
		}
		ALOGV("connect()..444444444444444");
		pthread_mutex_lock(&mConnApMutex);
		cfgDataGetString("ipc2.wifi.ssid", mConnAp.ssid, NULL);
		cfgDataGetString("ipc2.wifi.password", mConnAp.password, NULL);
		cfgDataGetString("ipc2.wifi.encryption", mConnAp.encryption, NULL);
		mConnAp.conning = 1;
		pthread_mutex_unlock(&mConnApMutex);
		ALOGV("connect()..555555555555555");
		if(WIFI_ENABLED == getEnableState()) {
			return connectApTarget();
		}
		ALOGV("connect()..666666666666666");
		return setEnable(1);
	}
	ALOGV("connect()..777777777777777");
	return -1;
}

int WifiManager::disconnect() {
	pthread_mutex_lock(&mConnApMutex);
	mConnAp.conning = 0;
	pthread_mutex_unlock(&mConnApMutex);
	
	if(WIFI_CONNECTING == getConnState()) {
		return -1;
	}
	
	if(WIFI_CONNECTED == getConnState()) {
		updateConnState(WIFI_DISCONNECTED);
		if(mConnListener != NULL) {
			mConnListener->handleWifiConnState(mConnCookie, WIFI_DISCONNECTED);
		}
		
		mDhcpHandler->wifiReleaseIp();
		
//		int ret = dhcp_stop("wlan0");
//		ALOGV("After dhcp_stop(wlan0), ret = %d", ret);
//		ret = ifc_clear_addresses("wlan0");
//		ALOGV("After ifc_clear_addresses(wlan0), ret = %d", ret);
	}
	
	return 0;
}

void WifiManager::updateEnableState(wifi_enable_state_t state) {
	pthread_mutex_lock(&mMutex);
	if(mEnableState == state) {
		pthread_mutex_unlock(&mMutex);
		return;
	}
	mEnableState = state;
	pthread_mutex_unlock(&mMutex);
}

wifi_enable_state_t WifiManager::getEnableState() {
	wifi_enable_state_t state;
	pthread_mutex_lock(&mMutex);
	state = mEnableState;
	pthread_mutex_unlock(&mMutex);
	return state;
}

int WifiManager::setEnable(int enable) {
	int err = 0;
	wifi_enable_state_t state = getEnableState();
	ALOGV("setEnable()..state = %d, enable = %d", state, enable);
	if(enable) {
		if(WIFI_DISABLING == state) {
			ALOGE("Now can not enable wifi, is disabling.");
			return -1;
		}
		
		if(WIFI_ENABLED == state || WIFI_ENABLING == state) {
			return 0;
		}
		
		updateEnableState(WIFI_ENABLING);
		err = pthread_create(&mEnableThreadId, NULL, wifiEnableThread, (void *)this);
		if(err || !mEnableThreadId) {
			ALOGE("Create wifiEnableThread error.");
			updateEnableState(WIFI_ENABLE_ERROR);
			return -1;
		}
	}
	else {
		if(WIFI_ENABLING == state) {
			ALOGE("Now can not disable wifi, is enabling.");
			return -1;
		}
		
		if(WIFI_DISABLED == state || WIFI_DISABLING == state) {
			return 0;
		}
		
		updateEnableState(WIFI_DISABLING);
		err = pthread_create(&mDisableThreadId, NULL, wifiDisableThread, (void *)this);
		if(err || !mDisableThreadId) {
			ALOGE("Create wifiDisableThread error.");
			updateEnableState(WIFI_ENABLE_ERROR);
			return -1;
		}
	}
	
	return 0;
}

int WifiManager::getEnable() {
	wifi_enable_state_t state = getEnableState();
	if(WIFI_ENABLED == state) {
		return 1;
	}
	return 0;
}

void WifiManager::updateConnState(wifi_conn_state_t state) {
	pthread_mutex_lock(&mMutex);
	if(mConnState == state) {
		pthread_mutex_unlock(&mMutex);
		return;
	}
	mConnState = state;
	pthread_mutex_unlock(&mMutex);
}

wifi_conn_state_t WifiManager::getConnState() {
	wifi_conn_state_t state = WIFI_CONN_UNKNOWN;
	pthread_mutex_lock(&mMutex);
	state = mConnState;
	pthread_mutex_unlock(&mMutex);
	return state;
}

static void *wifiConnThread(void *p_ctx) {
	WifiManager *p_this = (WifiManager *)p_ctx;
	
	wifi_conn_state_t state = p_this->getConnState();
	if(WIFI_CONNECTING != state) {
		return NULL;
	}
	
	int ret = p_this->mDhcpHandler->wifiRequestIp();
	if(ret != 0) {
		ALOGE("Wifi request ip error.");
		p_this->updateConnState(WIFI_CONN_UNKNOWN);
		if(p_this->mConnListener != NULL) {
			p_this->mConnListener->handleWifiConnState(p_this->mConnCookie, WIFI_CONN_UNKNOWN);
		}
		return NULL;
	}
	
	state = p_this->getConnState();
	if(WIFI_CONNECTING != state && WIFI_CONN_UNKNOWN != state) {
		return NULL;
	}
	
	ALOGV("Wifi connected.");
    p_this->updateConnState(WIFI_CONNECTED);
	
	pthread_mutex_lock(&p_this->mConnApMutex);
	p_this->mConnAp.conning = 0;
	pthread_mutex_unlock(&p_this->mConnApMutex);
	
	if(p_this->mConnListener != NULL) {
		p_this->mConnListener->handleWifiConnState(p_this->mConnCookie, WIFI_CONNECTED);
	}
	
    return NULL;
}

int WifiManager::requestIp() {
	updateConnState(WIFI_CONNECTING);
	int err = pthread_create(&mConnThreadId, NULL, wifiConnThread, (void *)this);
	if(err || !mConnThreadId) {
		ALOGE("Create wifi conn thread error.");
		updateConnState(WIFI_DISCONNECTED);
	    if(mApConnHandler != NULL) {
			mApConnHandler->handleWifiApConnResult(-1);
			mApConnHandler = NULL;
	    }
		if(mConnListener != NULL) {
			mConnListener->handleWifiConnState(mConnCookie, WIFI_DISCONNECTED);
		}
		return -1;
	}
	return 0;
}

void WifiManager::handleLinkState(wifi_link_state_t state, int netId) {
	ALOGV("handleLinkState()..state = %d, netId = %d", state, netId);
	char ssid[64] = {0};
	wifi_getNetworkSsid(ssid, 64, netId);
	ALOGV("After getNetworkSsid()..ssid = %s", ssid);
	int ret = -1;
	if(WIFI_LINK_DOWN == state) {
		updateConnState(WIFI_DISCONNECTED);
		
		pthread_mutex_lock(&mConnApMutex);
//		ALOGV("mConnAp.conning = %d, mConnAp.ssid = %s", mConnAp.conning, mConnAp.ssid);
		if(mConnAp.conning) {
			ret = strcmp(ssid, mConnAp.ssid);
//			ALOGV("after strcmp()..ret = %d", ret);
			if(ret == 0) {
				mConnAp.conning = 0;
				pthread_mutex_unlock(&mConnApMutex);
			    if(mApConnHandler != NULL) {
					mApConnHandler->handleWifiApConnResult(-1);
					mApConnHandler = NULL;
			    }
				if(mConnListener != NULL) {
					mConnListener->handleWifiConnState(mConnCookie, WIFI_DISCONNECTED);
				}
				
				char savedSsid[64] = {0};
				cfgDataGetString("ipc2.wifi.ssid", savedSsid, NULL);
				if(strcmp(savedSsid, ssid) == 0) {
					cfgDataSetInt("ipc2.wifi.valid", 0);
				}
			}
			else {
				pthread_mutex_unlock(&mConnApMutex);
				if(mConnListener != NULL) {
					mConnListener->handleWifiConnState(mConnCookie, WIFI_DISCONNECTED);
				}
			}
		}
		else {
			pthread_mutex_unlock(&mConnApMutex);
			if(mConnListener != NULL) {
				mConnListener->handleWifiConnState(mConnCookie, WIFI_DISCONNECTED);
			}
			
			mDhcpHandler->wifiReleaseIp();
			
//			ret = dhcp_stop("wlan0");
//			ALOGV("After dhcp_stop(wlan0), ret = %d", ret);
//			ret = ifc_clear_addresses("wlan0");
//			ALOGV("After ifc_clear_addresses(wlan0), ret = %d", ret);
		}
		
		return;
	}
	
	if(WIFI_LINK_UP == state) {
	    if(mApConnHandler != NULL) {
			mApConnHandler->handleWifiApConnResult(0);
			mApConnHandler = NULL;
	    }
		if(getConnEnable()) {
			requestIp();
		}
		
		pthread_mutex_lock(&mConnApMutex);
		ALOGV("When link up, mConnAp.conning = %d, mConnAp.ssid = %s", mConnAp.conning, mConnAp.ssid);
		if(!mConnAp.conning) {
			pthread_mutex_unlock(&mConnApMutex);
			return;
		}
//		mConnAp.conning = 0;
		ret = strcmp(ssid, mConnAp.ssid);
		ALOGV("strcmp result = %d", ret);
		if(ret != 0) {
			pthread_mutex_unlock(&mConnApMutex);
			return;
		}
		ALOGV("Save conn ap, ssid = %s, pw = %s, encry = %s", mConnAp.ssid, mConnAp.password, mConnAp.encryption);
		cfgDataSetString("ipc2.wifi.ssid", mConnAp.ssid);
		cfgDataSetString("ipc2.wifi.password", mConnAp.password);
		cfgDataSetString("ipc2.wifi.encryption", mConnAp.encryption);
		pthread_mutex_unlock(&mConnApMutex);
		cfgDataSetInt("ipc2.wifi.valid", 1);
		
		return;
	}
	
	ALOGE("Unknown wifi link state.");
	return;
}

int WifiManager::getIp(char *ipstr) {
	android_wifi_get_ip(ipstr);
	ALOGV("Get wifi ip = %s", ipstr);
	return 0;
}

static void *connTimerThread(void *p_ctx) {
	WifiManager *p_this = (WifiManager *)p_ctx;
	ALOGE("connTimerThread()..000");
	pthread_mutex_lock(&p_this->mConnApMutex);
	if(!p_this->mConnAp.conning) {
		pthread_mutex_unlock(&p_this->mConnApMutex);
		return NULL;
	}
	pthread_mutex_unlock(&p_this->mConnApMutex);
	ALOGE("connTimerThread()..111");
	usleep(20000000);
	ALOGE("connTimerThread()..222");
	pthread_mutex_lock(&p_this->mConnApMutex);
	if(!p_this->mConnAp.conning) {
		pthread_mutex_unlock(&p_this->mConnApMutex);
		return NULL;
	}
//	p_this->mConnAp.conning = 0;
	pthread_mutex_unlock(&p_this->mConnApMutex);
	
	ALOGE("Wifi connect timeout.");
//	p_this->updateConnState(WIFI_CONN_UNKNOWN);
    if(p_this->mApConnHandler != NULL) {
		p_this->mApConnHandler->handleWifiApConnResult(-1);
		p_this->mApConnHandler = NULL;
    }
	if(p_this->mConnListener != NULL) {
		p_this->mConnListener->handleWifiConnState(p_this->mConnCookie, WIFI_CONN_UNKNOWN);
	}
	
	return NULL;
}

int WifiManager::isConnectingAp() {
	int ret;
	pthread_mutex_lock(&mConnApMutex);
	ret = mConnAp.conning;
	pthread_mutex_unlock(&mConnApMutex);
	return ret;
}

int WifiManager::connectApTarget() {
	pthread_mutex_lock(&mConnApMutex);
	if(!mConnAp.conning) {
		pthread_mutex_unlock(&mConnApMutex);
		return -1;
	}
	
	ALOGV("Connect ap: ssid = %s, password = %s, encryption = %s, conning = %d", mConnAp.ssid, mConnAp.password, mConnAp.encryption, mConnAp.conning);
	int ret = connectWifiAp(mConnAp.ssid, mConnAp.password, mConnAp.encryption);
	if(ret == -1) {
		mConnAp.conning = 0;
		pthread_mutex_unlock(&mConnApMutex);
		ALOGV("Connect ap error.");
	    if(mApConnHandler != NULL) {
			mApConnHandler->handleWifiApConnResult(-1);
			mApConnHandler = NULL;
	    }
		if(mConnListener != NULL) {
			mConnListener->handleWifiConnState(mConnCookie, WIFI_CONN_UNKNOWN);
		}
		return -1;
	}
	pthread_mutex_unlock(&mConnApMutex);
	
	pthread_t threadId = 0;
	int err = pthread_create(&threadId, NULL, connTimerThread, (void *)this);
	if(err || !threadId) {
		ALOGE("Create connTimerThread error.");
	}
	
	return 0;
}

void WifiManager::setApConnResultHandler(WifiApConnResultHandler *hdlr) {
	mApConnHandler = hdlr;
}

int WifiManager::connectAp(const char *ssid, const char *pw, const char *encryption) {
	ALOGV("Connect ap: ssid = %s, pw = %s, encryption = %s", ssid, pw, encryption);
	
	pthread_mutex_lock(&mConnApMutex);
	strcpy(mConnAp.ssid, ssid);
	strcpy(mConnAp.password, pw);
	strcpy(mConnAp.encryption, encryption);
	mConnAp.conning = 1;
	pthread_mutex_unlock(&mConnApMutex);
	
	wifi_enable_state_t state = getEnableState();
	ALOGV("connectAp()..enable state = %d", state);
	if(WIFI_DISABLED == state) {
		return setEnable(1);
	}
	
	if(WIFI_ENABLED == state) {
		return connectApTarget();
	}
	
	return -1;
}

int WifiManager::getConnectedAp(char *ssid) {
	int ret = -1;
	int valid = 0;
	cfgDataGetInt("ipc2.wifi.valid", &valid, 0);
	if(valid) {
		if(cfgDataGetString("ipc2.wifi.ssid", ssid, NULL) > 0) {
			ret = 0;
			if(WIFI_ENABLED == getEnableState() && WIFI_CONNECTED == getConnState()) {
				ret = 1;
			}
		}
	}
	ALOGV("getConnectedAp()..ssid = %s, valid = %d, ret = %d", ssid, valid, ret);
	return ret;
}

int WifiManager::forgetAp(const char *ssid) {
	cfgDataSetInt("ipc2.wifi.valid", 0);
	
	wifi_enable_state_t state = getEnableState();
	if(WIFI_ENABLED != state) {
		return 0;
	}
	
	wifi_conn_state_t connState = getConnState();
	if(WIFI_CONNECTED != connState) {
		return 0;
	}
	
	android_wifi_disconnect();
	
//	wifi_connected_list apList;
//	memset(&apList, 0, sizeof(wifi_connected_list));
//	int ret = getConfiguredWifiApList(&apList);
//	ALOGV("After getConfiguredWifiApList()..ret = %d, count = %d", ret, apList.list_count);
//	for(int i = 0; i < apList.list_count; i++) {
//		if(strcmp(apList.connected_list[i].ssid, ssid) == 0) {
//			int netId = atoi(apList.connected_list[i].network_id);
//			ret = forgetWifiAp(netId);
//			ALOGV("After forgetWifiAp()..ret = %d, ssid = %s, netId = %d", ret, ssid, netId);
//		}
//	}
	
	return 0;
}

//int WifiManager::getApList(WifiApListItem **list) {
//	wifi_enable_state_t state = getWifiEnableState();
//	if(WIFI_ENABLED != state) {
//		return -1;
//	}
//	
//	return 0;
//}
