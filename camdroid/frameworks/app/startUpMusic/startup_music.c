#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <cutils/log.h>


#define bool int
#define true 1
#define false 0

typedef struct{
	int vol;
	int voice;
}volumeTable;

volumeTable VolNum[]={{0,29},{1,24},{2,19}};

int getCDRVolume()
{
	FILE * fp = NULL ;
	char str[64]={0};
	int volume=1;
	if((fp = fopen("/data/menu.cfg", "r")) == NULL){
		ALOGD("open /data/menu.cfg failed");
		if((fp = fopen("/system/res/cfg/menu.cfg", "r")) == NULL){
			ALOGD("open /system/res/cfg/menu.cfg  failed");	
			return VolNum[1].voice;
		}
	}
	bool flag1=true,flag2=false;
	while(fgets(str,sizeof(str),fp)!=NULL){
		if(flag1){
	    	if (strncmp(str+1, "voicevol", strlen("voicevol")) == 0) {
				flag1=false;
				flag2=true;
			}
		}
		if(flag2){
	    	if (strncmp(str, "current", strlen("current")) == 0) {
		           char *ret = strstr(str, "=");
	               ret += 1;
				   volume = atoi(ret);
				   break;		
			}

		}
		memset(str, 0, sizeof(char)*64);
	}
	if(fp)
		fclose(fp);
	return VolNum[volume].voice;

}


int main(int argc, char **argv)
{
	char player[128]="tinyplay ";
	char setVol[64]={0};
	sprintf(setVol, "tinymix 1 %d", getCDRVolume());
	ALOGD("<***setVol:%s***>",setVol);
	system("tinymix 16 1");
	system(setVol);
	strcat(player, argv[1]);
	system(player);
	return 0;
}
