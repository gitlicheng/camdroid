
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define VIB_M0  "/sys/vibrator/vib_m0"
#define VIB_M1  "/sys/vibrator/vib_m1"

/* direction :0 forword  ,1 backforword    times: 2100 one round*/
//return value: 0 success      -1 fail
 int sendit(char * vib_select,int direction ,int times)  
{  
   int nwr, ret, fd;  
  char value[20]; 
char *buf=value;  
 
   fd = open(vib_select, O_RDWR);  
    if(fd < 0)  
        {printf("open fail\n");  
		return 0;  
		}
    printf("open ok\n");  
	if(direction ==1){
	value[0] = 'b';
	buf+=1;
	}
    nwr = sprintf(buf, "%d\n", times);  
    ret = write(fd, value, nwr);  
   if(ret > 0)printf("write success!\n");
    close(fd);  
    return (ret == nwr) ? 0 : -1;  
}  

/*
int main(int argc, char *argv[])
{
	
printf("enter vibrator\n");

sendit(VIB_M0,0 ,1000)  ;
sendit(VIB_M0,1,1000)  ;



return 0;
} */









