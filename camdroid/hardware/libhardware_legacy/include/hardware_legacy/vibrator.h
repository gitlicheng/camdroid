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

#ifndef _HARDWARE_VIBRATOR_H
#define _HARDWARE_VIBRATOR_H

#if __cplusplus
extern "C" {
#endif

/**
 * vib_select :M0  "/sys/vibrator/vib_m0"   M1  "/sys/vibrator/vib_m1"
 *
 * direction :0 forword  ,1 backforword    times: 2100 one round
 *
 * return value: 0 success      -1 fail
 */
 int sendit(char * vib_select,int direction ,int times) ;

#if __cplusplus
}  // extern "C"
#endif

#endif  // _HARDWARE_VIBRATOR_H
