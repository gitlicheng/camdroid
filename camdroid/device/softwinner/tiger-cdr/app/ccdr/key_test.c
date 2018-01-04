#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdlib.h>

struct input_event buff; 
int fd; 
int read_nu; 

int main(int argc, char *argv[])
{
	int i = 0;
	fd = open("/dev/input/event0", O_RDONLY); //may be the powerkey is /dev/input/event1
	if (fd < 0) { 
		perror("can not open device usbkeyboard!"); 
		exit(1); 
	} 

	printf("--fd:%d--\n",fd);
	while(1)
	{
		while(read(fd,&buff,sizeof(struct input_event))==0)
		{
			;
		}
		//if(buff.code > 40)
		printf("time %u:%u type:%d code:%d value:%d\n",buff.time.tv_sec,buff.time.tv_usec,buff.type,buff.code,buff.value); 

		//#if 0
		//i++;
		//if(i > 12)
		//{
		//break;
		//} 
		//#endif
	} 

	close(fd); 
	return 1;
}
