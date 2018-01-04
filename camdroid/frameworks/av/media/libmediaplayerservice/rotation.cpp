#include <utils/Log.h>
#include <utils/RefBase.h>

#include <binder/IBinder.h>
#include <binder/Parcel.h>
#include <binder/IServiceManager.h>


#define LOG_TAG "rotation-demo"

namespace android {

int getRotation(){
	const String16 name("window");
	const String16 interface("android.view.IWindowManager");	
	const sp<IServiceManager> sm = defaultServiceManager(); 
	
	sp<IBinder> window;	

	window = sm->getService(name);
	
	if( window != NULL ){

		Parcel data, reply;    		
		data.writeInterfaceToken(interface);
		window->transact(56, data, &reply, 0);	
		reply.readInt32();
		int rotation = reply.readInt32();
		return rotation;
	}
	return -1;	
}
}

//int main(int argc, char** argv) {
//	SLOGD("rotation is %d",getRotation());
//}
