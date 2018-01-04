#define LOG_TAG "StandByService"

#include <utils/Log.h>
#include <cutils/log.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>

#include <standby/StandBy.h>
#include <pthread.h>
//#include <standby/OtaUpdate.h>


namespace android {

	 void StandbyService::instantiate()
	{
		ALOGE("instantiate");
		status_t st = defaultServiceManager()->addService(
			String16("camdroid.service.standby"), new StandbyService() );
	}

	StandbyService::StandbyService()
	{
		
	}

	StandbyService::~StandbyService()
	{
		
	}

	static void *startOtaThread(void *ptx)
	{
		ALOGE("**** TFTFTF ota***");
	 	system("system/bin/cdrUpdate");
		return NULL;
	}

	int StandbyService::enterStandby(int args)
	{
		ALOGE("enter Standby!!!  PID  args=%d****",args);
		if(args>0){
			int ret=kill(args,SIGKILL);
			ALOGE("****kill newcdr** ret=%d***",ret);		
			pthread_t thread_id = 0;
			int err = pthread_create(&thread_id, NULL, startOtaThread, NULL);
			if(err || !thread_id) {
				ALOGE("Create startOtaThread  thread error.");
				return -1;
			}
		}
		return 0;
	}
	
};
