#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
