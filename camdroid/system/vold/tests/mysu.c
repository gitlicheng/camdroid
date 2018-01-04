#include <stdio.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <sys/un.h>  
#include <pthread.h>
#define UNIX_DOMAIN "/dev/socket/vold"
#ifdef __cplusplus
extern "C" {
#endif   

typedef void (*VolumeListen)(char *data, int dataLeng);

VolumeListen globalVolumeListen = NULL;
pthread_t       tid ;
pthread_attr_t  attr;
int connect_fd = 0;

void registerVolumeListen(VolumeListen listen);
void unRegisterVolumeListen();
void registerVolumeListen(VolumeListen listen)
{
    globalVolumeListen = listen;
}

void unRegisterVolumeListen()
{
    if(connect_fd >=0 )
	{
	    close(connect_fd);
	}	
	globalVolumeListen = NULL;
}

void* threadRun()
{
      
    int ret;   
    char snd_buf[512];   
    int i;   
    static struct sockaddr_un srv_addr;  
	srv_addr.sun_family=AF_UNIX;   
	strcpy(srv_addr.sun_path,UNIX_DOMAIN);	
	while(1)
	{
		printf("  while step 1  \n");
		memset(snd_buf,0,sizeof(snd_buf));  
		connect_fd=socket(PF_UNIX,SOCK_STREAM,0);   
		if(connect_fd<0)   
		{      
			break;   
		}      
		printf("  while step 2  \n");	
		ret = connect(connect_fd,(struct sockaddr*)&srv_addr,sizeof(srv_addr));   
		if(ret == -1)   
		{   
			//perror("cannot connect to the server");
			printf("  while connect erro 3  \n");
			close(connect_fd);   
			break;   
		}
		printf("  while step 3  \n");
		 
		ret = read(connect_fd,snd_buf,sizeof(snd_buf)-1);  
		//printf("ret =%d,snd_buf =%s \n",ret,snd_buf);
		if(globalVolumeListen)
		{
		    globalVolumeListen(snd_buf,strlen(snd_buf)+1);
		}		
		if(ret < 0)
		{
		    break;
		}		    
		close(connect_fd);
		printf("  while step 4  \n");
	}
	return ((void *)0);
}

void startListen()
{
     pthread_attr_init(&attr);
	 pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); 
     pthread_create(&tid, &attr, threadRun, NULL);
}
void myListen(char *data, int dataLeng)
{
	printf("  while step 6666666666  \n");
	printf("2mylisten data = %s dataLeng= %d\n",data,dataLeng);
	printf("  while step 7777777777  \n");
}

int main(void)   
{       
   registerVolumeListen(myListen);
   startListen();
   
   while(1)
   {
       sleep(100);
   }
   return 0;   
}  
#ifdef __cplusplus
}
#endif