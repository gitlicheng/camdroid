/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef _SOFTAP_H
#define _SOFTAP_H

#if __cplusplus
extern "C" {
#endif
/*返回值，0表示成功，-1表示失败*/
int enableSoftAp();
int disableSoftAp();
int setSoftap(char * name, char *password);
int getSoftAPIPaddr(char *IPaddr);

/*返回1表示已经开启，0 表示还未开启*/
int isSoftAPenabled();

#if __cplusplus
};  // extern "C"
#endif

#endif  // _WIFI_API_H