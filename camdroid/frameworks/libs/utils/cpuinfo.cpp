/*
 * Copyright (C) 2005 The Android Open Source Project
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

#define LOG_TAG "misc"

#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <utils/Log.h>
#include <ctype.h>

static void get_hardware_name(char *hardware, unsigned int *revision)
{
    char data[1024];
    int fd, n;
    char *x, *hw, *rev;

    /* Hardware string was provided on kernel command line */
    if (hardware[0])
    {
        return;
    }

    fd = open("/proc/cpuinfo", O_RDONLY);
    if (fd < 0) 
    {
    	return;
    }

    n = read(fd, data, 1023);
    close(fd);
    if (n < 0) 
    {
    	return;
    }

    data[n] = 0;
    hw = strstr(data, "\nHardware");
    rev = strstr(data, "\nRevision");

    if (hw) 
    {
        x = strstr(hw, ": ");
        if (x) 
        {
            x += 2;
            n = 0;
            while (*x && *x != '\n') 
            {
                if (!isspace(*x))
                    hardware[n++] = tolower(*x);
                x++;
                if (n == 31) break;
            }
            hardware[n] = 0;
        }
    }

    if (rev) 
    {
        x = strstr(rev, ": ");
        if (x) 
        {
            *revision = strtoul(x + 2, 0, 16);
        }
    }
}

int isPlatform()
{
	char hardware[64];
	unsigned int version = 0;
	
	memset(hardware, 0, sizeof(hardware));
	get_hardware_name(hardware, &version);
	if (strncmp(hardware, "sun", 3))
	{
		printf("unsupported platform, [%s]", hardware);
		
		return 0;
	}
	
	return 1;
}
