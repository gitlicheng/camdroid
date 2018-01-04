#define LOG_NDEBUG 1
#define LOG_TAG "SoftApEnabler"
#include <cutils/log.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <include_utils/utils.h>
#include <hardware_legacy/softap.h>
#include <SoftApEnabler.h>
#include <StorageManager.h>
#include <ConfigData.h>
#include <StorageManager.h>

static int getSsid(char *ssid) {
	strcpy(ssid, "");

	cfgDataGetString("persist.sys.wifi.ssid", ssid, " ");
	if (strcmp(ssid, " ") == 0) {
		char ret[10]={0};
		StorageManager *sm = StorageManager::getInstance();
		sm->readCpuInfo(ret);
		ssid[0]='\0';
		strcat(ssid,ret);
		cfgDataSetString("persist.sys.wifi.ssid", ssid);
	}
	
	ALOGD("SoftAp ssid = %s", ssid);
	return 0;
}

SoftApEnabler::SoftApEnabler()
		: mState(SOFTAP_DISABLED), mEnableThreadId(0), mDisableThreadId(0), mEnableCbCookie(NULL), mEnableCb(NULL) {
	pthread_mutex_init(&mMutex, NULL);
	
	char ssid[16] = {0};
	getSsid(ssid);
	char password[64] = "";//"66666666";
	cfgDataGetString("persist.sys.wifi.pwd", password, "66666666");
	setSoftap(ssid, password);
	cfgDataSetString("persist.sys.wifi.ssid", ssid);
}

SoftApEnabler::~SoftApEnabler() {
	pthread_mutex_destroy(&mMutex);
}
	
void SoftApEnabler::setEnableListener(void *cookie, SofApEnableListener *pl) {
	mEnableCbCookie = cookie;
	mEnableCb = pl;
}

void SoftApEnabler::updateState(softap_state_t state) {
	pthread_mutex_lock(&mMutex);
	if(mState == state) {
		pthread_mutex_unlock(&mMutex);
		return;
	}
	mState = state;
	pthread_mutex_unlock(&mMutex);
}

softap_state_t SoftApEnabler::getState() {
	softap_state_t state;
	pthread_mutex_lock(&mMutex);
	state = mState;
	pthread_mutex_unlock(&mMutex);
	return state;
}

static void *softApEnableThread(void *p_ctx) {
	SoftApEnabler *p_this = (SoftApEnabler *)p_ctx;
	
	pthread_mutex_lock(&p_this->mMutex);
	if(SOFTAP_ENABLING != p_this->mState) {
		pthread_mutex_unlock(&p_this->mMutex);
		return NULL;
	}
	int ret = enableSoftAp();
	if(ret != 0) {
		ALOGE("Enable soft ap error.");
		p_this->mState = SOFTAP_ENABLE_ERROR;
		pthread_mutex_unlock(&p_this->mMutex);
		return NULL;
	}
	ALOGV("Soft ap enabled.");
	p_this->mState = SOFTAP_ENABLED;
	pthread_mutex_unlock(&p_this->mMutex);
	
	if(p_this->mEnableCb != NULL) {
		p_this->mEnableCb->onSoftApEnableChanged(p_this->mEnableCbCookie, 1);
	}
	
	return NULL;
}

static void *softApDisableThread(void *p_ctx) {
	SoftApEnabler *p_this = (SoftApEnabler *)p_ctx;
	
	pthread_mutex_lock(&p_this->mMutex);
	if(SOFTAP_DISABLING != p_this->mState) {
		pthread_mutex_unlock(&p_this->mMutex);
		return NULL;
	}
	int ret = disableSoftAp();
	if(ret != 0) {
		ALOGE("Disable soft ap error.");
		p_this->mState = SOFTAP_ENABLE_ERROR;
		pthread_mutex_unlock(&p_this->mMutex);
		return NULL;
	}
	ALOGV("Soft ap disabled.");
	p_this->mState = SOFTAP_DISABLED;
	pthread_mutex_unlock(&p_this->mMutex);
	
	if(p_this->mEnableCb != NULL) {
		p_this->mEnableCb->onSoftApEnableChanged(p_this->mEnableCbCookie, 0);
	}
	
	return NULL;
}

int SoftApEnabler::setEnable(int enable) {
	int err = 0;
	softap_state_t state = getState();
	ALOGD("999999setEnable()..state = %d, enable = %d", state, enable);
	if(enable) {
		if(SOFTAP_DISABLING == state) {
			ALOGE("Now can not enable soft ap, is disabling.");
			return -1;
		}
		
		if(SOFTAP_ENABLED == state || SOFTAP_ENABLING == state) {
			return 0;
		}
		
		updateState(SOFTAP_ENABLING);
		err = pthread_create(&mEnableThreadId, NULL, softApEnableThread, (void *)this);
		if(err || !mEnableThreadId) {
			ALOGE("Create softApEnableThread error.");
			updateState(SOFTAP_ENABLE_ERROR);
			return -1;
		}
	}
	else {
		if(SOFTAP_ENABLING == state) {
			ALOGE("Now can not disable soft ap, is enabling.");
			return -1;
		}
		
		if(SOFTAP_DISABLED == state || SOFTAP_DISABLING == state) {
			return 0;
		}
		
		updateState(SOFTAP_DISABLING);
		err = pthread_create(&mDisableThreadId, NULL, softApDisableThread, (void *)this);
		if(err || !mDisableThreadId) {
			ALOGE("Create softApDisableThread error.");
			updateState(SOFTAP_ENABLE_ERROR);
			return -1;
		}
	}
	
	return 0;
}

int SoftApEnabler::getEnable() {
	softap_state_t state = getState();
	if(SOFTAP_ENABLED == state) {
		return 1;
	}
	return 0;
}

int SoftApEnabler::getIp(char *ipstr) {
	int ret = getSoftAPIPaddr(ipstr);
	ALOGV("Get soft ap ip = %s, ret = %d", ipstr, ret);
	return ret;
}

int SoftApEnabler::setPwd(const char *pwd)
{
	int ret = 0;
	char ssid[16] = {0};
	pthread_mutex_lock(&mMutex);
	getSsid(ssid);
	ALOGD("!!setSoftap newpwd:%s", pwd);
	if (isSoftAPenabled()) {
		disableSoftAp();
	}
	cfgDataSetString("persist.sys.wifi.pwd", pwd);
	ret = setSoftap(ssid, (char*)pwd);
	enableSoftAp();
	pthread_mutex_unlock(&mMutex);
	return ret;
}

int SoftApEnabler::fResetWifiPWD(const char *pwd)
{
	int ret = 0;
	char ssid[16] = {0};
	cfgDataSetString("persist.sys.wifi.ssid", " ");
	getSsid(ssid);
	cfgDataSetString("persist.sys.wifi.pwd", pwd);
	ret = setSoftap(ssid, (char*)pwd);
	return 0;
}


