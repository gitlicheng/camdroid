#ifndef _SOFT_AP_ENABLER_H_
#define _SOFT_AP_ENABLER_H_

typedef enum {
	SOFTAP_DISABLED,
	SOFTAP_ENABLING,
	SOFTAP_ENABLED,
	SOFTAP_DISABLING,
	SOFTAP_ENABLE_ERROR
}softap_state_t;

class SoftApEnabler {
	
public:
	SoftApEnabler();
	~SoftApEnabler();
	
	class SofApEnableListener {
	public:
		SofApEnableListener() {};
		virtual ~SofApEnableListener() {};
		virtual void onSoftApEnableChanged(void *cookie, int enable) = 0;
	};
	
	void setEnableListener(void *cookie, SofApEnableListener *pl);
	
	int setEnable(int enable);
	int getEnable();
	int getIp(char *ipstr);
	
	softap_state_t getState();
	void updateState(softap_state_t state);
	int setPwd(const char *pwd);
	int fResetWifiPWD(const char *pwd);
	softap_state_t mState;
	
	pthread_t mEnableThreadId;
	pthread_t mDisableThreadId;
	pthread_mutex_t mMutex;
	
	void *mEnableCbCookie;
	SofApEnableListener *mEnableCb;
};

#endif	//_SOFT_AP_ENABLER_H_
