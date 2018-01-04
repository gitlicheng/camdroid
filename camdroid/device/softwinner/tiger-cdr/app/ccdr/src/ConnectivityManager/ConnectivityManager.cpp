#define LOG_NDEBUG 0
#define LOG_TAG "ConnectivityManager"
#include <cutils/log.h>

#include <netutils/ifc.h>
#include <netutils/dhcp.h>
#include <ConnectivityManager.h>

ConnectivityManager::ConnectivityManager() 
		: mConnChannel(CONN_CHANNEL_NONE), mCbCookie(NULL), mConnListener(NULL) {
	pthread_mutex_init(&mMutex, NULL);
	mDhcpHandler = new DhcpHandler();
//	mEthernetManager = new EthernetManager(mDhcpHandler);
	mWifiManager = new WifiManager(mDhcpHandler);
	mSoftApEnabler = new SoftApEnabler();
}

ConnectivityManager::~ConnectivityManager() {
	delete mSoftApEnabler;
	delete mWifiManager;
//	delete mEthernetManager;
	delete mDhcpHandler;
	pthread_mutex_destroy(&mMutex);
}

void ConnectivityManager::setConnectivityListener(void *cookie, ConnectivityListener *connl) {
	mCbCookie = cookie;
	mConnListener = connl;
}

int ConnectivityManager::start() {
//	mEthernetManager->setConnStateListener((void *)this, this);
	mWifiManager->setConnStateListener((void *)this, this);
	mSoftApEnabler->setEnableListener((void *)this, this);
	ALOGD("9999999ConnectivityManager start");
//	mEthernetManager->init();
	mWifiManager->init();
	
//	if(ETH_DISCONNECTED == mEthernetManager->getConnState())
	{
		mWifiManager->setConnEnable(1);
		if(mWifiManager->connect() == 0) {
            ALOGD("9999999mWifiManager->connect()");
			return 0;
		}
		mSoftApEnabler->setEnable(1);
	}
	
	return 0;
}

void ConnectivityManager::updateConnChannel(conn_channel_t channel) {
	pthread_mutex_lock(&mMutex);
	ALOGV("updateConnChannel()..channel = %d, mConnChannel = %d", channel, mConnChannel);
	if(channel == mConnChannel) {
		pthread_mutex_unlock(&mMutex);
		return;
	}
	
	if(CONN_CHANNEL_NONE == mConnChannel) {
		mConnChannel = channel;
		pthread_mutex_unlock(&mMutex);
		if(mConnListener != NULL) {
			mConnListener->handleConnectivity(mCbCookie, CONN_AVAILABLE);
		}
		return;
	}
	
	if(CONN_CHANNEL_NONE == channel) {
		mConnChannel = CONN_CHANNEL_NONE;
		pthread_mutex_unlock(&mMutex);
		if(mConnListener != NULL) {
			mConnListener->handleConnectivity(mCbCookie, CONN_UNAVAILABLE);
		}
		return;
	}
	
	mConnChannel = channel;
	pthread_mutex_unlock(&mMutex);
	if(mConnListener != NULL) {
		mConnListener->handleConnectivity(mCbCookie, CONN_CHANNEL_CHANGE);
	}
}

conn_channel_t ConnectivityManager::getConnChannel() {
	conn_channel_t channel = CONN_CHANNEL_NONE;
	pthread_mutex_lock(&mMutex);
	channel = mConnChannel;
	pthread_mutex_unlock(&mMutex);
	
	return channel;
}

//int ConnectivityManager::handleEthernetConnState(void *cookie, eth_conn_state_t state) {
//	ALOGV("handleEthernetConnState()..state = %d", state);
//	if(ETH_CONNECTED == state) {
//		updateConnChannel(CONN_CHANNEL_ETHERNET);
//		mWifiManager->setConnEnable(0);
//		mWifiManager->disconnect();
//		mSoftApEnabler->setEnable(0);
//		return 0;
//	}
//	
//	if(ETH_DISCONNECTED == state) {
//		updateConnChannel(CONN_CHANNEL_NONE);
//		mWifiManager->setConnEnable(1);
//		if(mWifiManager->connect() == 0) {
//			return 0;
//		}
//		mSoftApEnabler->setEnable(1);
//	}
//	
//	return 0;
//}

int ConnectivityManager::handleWifiConnState(void *cookie, wifi_conn_state_t state) {
	ALOGV("handleWifiConnState()..state = %d", state);
	if(WIFI_CONNECTED == state) {
		if(CONN_CHANNEL_ETHERNET == getConnChannel()) {
			mWifiManager->setConnEnable(0);
			mWifiManager->disconnect();
			mSoftApEnabler->setEnable(0);
			return 0;
		}
		
		updateConnChannel(CONN_CHANNEL_WIFI);
		mSoftApEnabler->setEnable(0);
		return 0;
	}
	
	if(WIFI_DISCONNECTED == state || WIFI_CONN_UNKNOWN == state) {
		if(CONN_CHANNEL_ETHERNET == getConnChannel()) {
			mWifiManager->setConnEnable(0);
			mSoftApEnabler->setEnable(0);
			return 0;
		}
		
		if(CONN_CHANNEL_WIFI == getConnChannel()) {
			updateConnChannel(CONN_CHANNEL_NONE);
			if(!mWifiManager->isConnectingAp()) {
				mSoftApEnabler->setEnable(1);
			}
			return 0;
		}
		
		if(CONN_CHANNEL_SOFTAP == getConnChannel()) {
			return 0;
		}
		
		mSoftApEnabler->setEnable(1);
		return 0;
	}
	
	if(WIFI_CONN_UNKNOWN == state) {
		if(CONN_CHANNEL_NONE == getConnChannel()) {
			ALOGV("here11111");
			mWifiManager->setConnEnable(1);
			if(mWifiManager->connect() == 0) {
				return 0;
			}
			ALOGV("here22222");
			mSoftApEnabler->setEnable(1);
			return 0;
		}
	}
	
	return 0;
}

void ConnectivityManager::onSoftApEnableChanged(void *cookie, int enable) {
	ALOGV("onSoftApEnableChanged()..enable = %d", enable);
	if(!enable) {
		if(CONN_CHANNEL_ETHERNET == getConnChannel()) {
			mWifiManager->setConnEnable(0);
			mWifiManager->disconnect();
			mSoftApEnabler->setEnable(0);
			return;
		}
		
		if(CONN_CHANNEL_WIFI == getConnChannel()) {
			mSoftApEnabler->setEnable(0);
			return;
		}
		
		updateConnChannel(CONN_CHANNEL_SOFTAP);
		mWifiManager->setEnable(0);
		return;
	}
		return;
/*	if(CONN_CHANNEL_ETHERNET == getConnChannel()) {
		mWifiManager->setConnEnable(0);
		mWifiManager->disconnect();
		return;
	}
	
	if(CONN_CHANNEL_WIFI == getConnChannel()) {
		return;
	}
	
	mWifiManager->setConnEnable(1);
	if(mWifiManager->connect() == 0) {
		return;
	}
	mSoftApEnabler->setEnable(1);*/
}

int ConnectivityManager::getIp(char *ipstr) {
	conn_channel_t channel = getConnChannel();
	
//	if(CONN_CHANNEL_ETHERNET == channel) {
//		eth_conn_state_t ethConnState = mEthernetManager->getConnState();
//		if(ETH_CONNECTED == ethConnState) {
//			return mEthernetManager->getIp(ipstr);
//		}
//	}
//	else 
	if(CONN_CHANNEL_WIFI == channel) {
		wifi_conn_state_t wifiConnState = mWifiManager->getConnState();
		if(WIFI_CONNECTED == wifiConnState) {
			return mWifiManager->getIp(ipstr);
		}
	}
	else if(CONN_CHANNEL_SOFTAP == channel) {
		if(mSoftApEnabler->getEnable()) {
			return mSoftApEnabler->getIp(ipstr);
		}
	}
	
	ALOGE("Get ip error, none channel or channel disconnected.");
	return -1;
}

void ConnectivityManager::setWifiApConnResultHandler(WifiManager::WifiApConnResultHandler *hdlr){
	mWifiManager->setApConnResultHandler(hdlr);
}

int ConnectivityManager::setWifiEnable(unsigned int enable) {
	return mSoftApEnabler->setEnable(enable);
}

int ConnectivityManager::getWifiEnable(unsigned int *enable) {
	*enable = (unsigned int)mWifiManager->getEnable();
	return 0;
}

int ConnectivityManager::getWifiApList(WifiApListItem **plist) {
//	return mWifiManager->getApList(plist);
	return 0;
}

int ConnectivityManager::connectWifiAp(const char *ssid, const char *pw, const char *encryption) {
	return mWifiManager->connectAp(ssid, pw, encryption);
}

int ConnectivityManager::getConnectedWifiAp(char *ssid) {
	return mWifiManager->getConnectedAp(ssid);
}

int ConnectivityManager::forgetWifiAp(const char *ssid) {
	return mWifiManager->forgetAp(ssid);
}

int ConnectivityManager::setPwd(const char *old_pwd, const char *new_pwd)
{
	return mSoftApEnabler->setPwd(new_pwd);
}

int ConnectivityManager::fResetWifiPWD(const char *pwd)
{
	return mSoftApEnabler->fResetWifiPWD(pwd);
}

