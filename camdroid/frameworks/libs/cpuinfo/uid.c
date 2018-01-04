#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const char UUID_SEED[] = "0123456789abcdef";

void genUUID(char *uuid) 
{
	struct timeval time;
	gettimeofday(&time, NULL);
	srand(time.tv_usec);
	int i;
	for(i = 0; i < 16; i++) 
	{
		uuid[i] = UUID_SEED[rand() % 16];
	}
	uuid[16] = '\0';
}

int getUUID(char *uuid)
{
    char cmdline[1024];
    char *ptr;
    char *uidstr;
    int fd;

    fd = open("/proc/cmdline", O_RDONLY);
    if (fd >= 0) 
    {
        int n = read(fd, cmdline, 1023);
        if (n < 0) 
        {
        	n = 0;
        	
        	genUUID(uuid);
        	
        	printf("getUUID - 2, [%s]", uuid);

        	return -2;
        }

        /* get rid of trailing newline, it happens */
        if (n > 0 && cmdline[n-1] == '\n') 
        {
        	n--;
        }

        cmdline[n] = 0;
        
        close(fd);
    } 
    else 
    {
        cmdline[0] = 0;
        
        genUUID(uuid);
        
        printf("getUUID - 3, [%s]", uuid);
        
        return -3;
    }

    uidstr	= strstr(cmdline, "uid=");
    ptr		= uidstr;
    if (ptr && *ptr) 
    {
        char *x = strchr(ptr, ' ');
        if (x != 0) 
        {
        	*x++ = 0;
        }
        
        char *value = strchr(ptr, '=');
        if (value == 0) 
        {
        	genUUID(uuid);
        	
        	printf("getUUID - 1, [%s]", uuid);
        	
        	return -1;
        }
        char *value_tmp = value ;
        char *tmp =  strchr(value_tmp, ';');
        int len = 0;
        if(tmp)
        {
        	len  = tmp - value-1;
        }
        printf("uid len=%d\n",len);
      
    	*value++ = 0;
    	
    	
    	if (len == 0)
    	{
        	strncpy(uuid, value,strlen(value));
        }
        else
        {
        	strncpy(uuid, value,len);
        }
        printf("getUUID 0, [%s]", uuid);
        
        return 0;
    }
    
    return 0;
}
