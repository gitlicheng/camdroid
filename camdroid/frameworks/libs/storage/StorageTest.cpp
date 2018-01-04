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
//#define LOG_NDEBUG 0
#define LOG_TAG "StorageMonitor"
#include <utils/Log.h>

#include "include_storage/StorageMonitor.h"

int main() {
	StorageMonitor* gStorageMonitor;
	
    gStorageMonitor = new StorageMonitor();
    gStorageMonitor->init();
	
	usleep(10*1000*1000);
	gStorageMonitor->sdcardUmount();
	gStorageMonitor->sdcardFormat("aw-cdr");
	gStorageMonitor->sdcardMount();
	usleep(20*1000*1000);
	ALOGE(LOG_TAG, "StorageMonitor test finished\n");
	
    return 0;
}



