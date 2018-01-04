/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef HEALTHD_STORAGE_MONTIOR_H
#define HEALTHD_STORAGE_MONTIOR_H

#include <stdlib.h> 
#include <sys/mount.h>

#include <utils/Vector.h>
#include <utils/Mutex.h>
#include <utils/Thread.h>

using namespace android;

enum {
	STORAGE_STATUS_NO_MEDIA   = 0,
	STORAGE_STATUS_IDLE		  = 1,
	STORAGE_STATUS_PENDING	  = 2,
	STORAGE_STATUS_CHECKING   = 3,
	STORAGE_STATUS_MOUNTED    = 4,
	STORAGE_STATUS_UNMOUNTING = 5,
	STORAGE_STATUS_FORMATTING = 6,
	STORAGE_STATUS_SHARED     = 7,
	STORAGE_STATUS_SHAREDMNT  = 8,
	STORAGE_STATUS_REMOVE     = 9,
};

class StorageMonitorListener {
public:
	StorageMonitorListener(){};
	virtual ~StorageMonitorListener(){};
public:
	virtual void notify(int what, int status, char *msg) = 0;
};

class StorageMonitor {
public:
	StorageMonitor();
	~StorageMonitor();

public: 
	int init(void);
	int exit(void);
	int setMonitorListener(StorageMonitorListener * listener);
	int mountToPC();
	int unmountFromPC();
	int formatSDcard();
	bool isInsert();
	bool isMount();
	bool isShare();
	bool isFormated();

	class UpdateThread : public Thread {
	public:
		UpdateThread(StorageMonitor* handle) :
						Thread(false),
						mSM(handle){
		}
			  
		void startThread() {
			run("UpdateThread", PRIORITY_NORMAL);
		}
		
		void stopThread() {
			requestExitAndWait();
		}
		
		virtual bool threadLoop() {
			mSM->updateLoop();
			return true;
		}
		
	private:
		StorageMonitor* mSM;
	};

private:
	bool mShared;
	bool mSharing;
	bool mInserted;
	bool mMounted;
	bool mFormated;
	bool mFormating;

	int mStatus;
	int mConnectFD;
    int mWriteFD;
	char mPath[125];
	sp<UpdateThread> mUpdateThread;
	StorageMonitorListener * mListener;
	
	int	updateLoop(void);	
	void processStr(char *src);
};

#endif // HEALTHD_STORAGE_MONTIOR_H

