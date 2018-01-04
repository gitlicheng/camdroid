
#define LOG_TAG "Audio_WakeLock"
#define LOG_NDEBUG 0

#include <utils/Log.h>

#include <stdint.h>
#include <sys/types.h>

#include <binder/Parcel.h>

#include <powermanager/IPowerManager.h>
#include <powermanager/PowerManager.h>
#include <binder/IServiceManager.h>



namespace android {
#define ANDROID_WAKE_LOCK_NAME "libcodec_audio"

static sp<IPowerManager>   gPowerManager;
static  sp<IBinder>        gWakeLockToken;

extern "C" void c_plus_plus_grabPartialWakeLock();
extern "C" void c_plus_plus_releaseWakeLock();


void c_plus_plus_releaseWakeLock() 
{
    if (gWakeLockToken != 0) {
        ALOGV("releaseWakeLock_l() %s", ANDROID_WAKE_LOCK_NAME);
        if (gPowerManager != 0) {
            gPowerManager->releaseWakeLock(gWakeLockToken, 0);
        }
        gWakeLockToken.clear();
    }
}



void c_plus_plus_grabPartialWakeLock() 
{
    if (gPowerManager == 0) {
        // use checkService() to avoid blocking if power service is not up yet
        sp<IBinder> binder =
            defaultServiceManager()->checkService(String16("power"));
        if (binder == 0) {
            ALOGW("Thread %s cannot connect to the power manager service", ANDROID_WAKE_LOCK_NAME);
        } else {
            gPowerManager = interface_cast<IPowerManager>(binder);

        class PMDeathRecipient : public IBinder::DeathRecipient {
        public:
                        PMDeathRecipient()  {}
            virtual     ~PMDeathRecipient() {}

            // IBinder::DeathRecipient
            virtual void  binderDied(const wp<IBinder>& who){
    				c_plus_plus_releaseWakeLock();
				gPowerManager.clear();
    				ALOGW("power manager service died !!!");
			  }

        private:
                        PMDeathRecipient(const PMDeathRecipient&);
                        PMDeathRecipient& operator = (const PMDeathRecipient&);
        };

        sp<IBinder::DeathRecipient> mDeathObserver = new PMDeathRecipient();

        binder->linkToDeath(mDeathObserver);
        }
    }
    if (gPowerManager != 0) {
        sp<IBinder> binder = new BBinder();
        status_t status = gPowerManager->acquireWakeLock(POWERMANAGER_PARTIAL_WAKE_LOCK,
                                                         binder,
                                                         String16(ANDROID_WAKE_LOCK_NAME));
        if (status == NO_ERROR) {
            gWakeLockToken = binder;
        }
        ALOGV("acquireWakeLock_l() %s status %d", ANDROID_WAKE_LOCK_NAME, status);
    }
        ALOGV("acquireWakeLock_l() 1");
}


}


