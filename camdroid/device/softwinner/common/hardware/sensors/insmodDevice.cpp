#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <linux/input.h>
#include <cutils/atomic.h>
#include <cutils/log.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <cutils/misc.h>
#include <cutils/properties.h>
#include "sensors.h"

#define LOG_TAG "insmodDevice"

extern  "C" int init_module(void *, unsigned long, const char *);
extern  "C" int delete_module(const char *, unsigned int);

//#define DEBUG 1

#define INSMOD_PATH                     ("system/vendor/modules/")
#define I2C_DEVICE_CONFIG_PATH          ("data/device.info")
#define LINE_LENGTH  (128)

static char name[LINE_LENGTH] = {'\0'};

static int insmod(const char *filename, const char *args)
{
        void *module;
        unsigned int size;
        int ret;
        
        module = load_file(filename, &size);
        if (!module)
                return -1;
        
        ret = init_module(module, size, args);
        
        free(module);
        
        return ret;
}

static int rmmod(const char *modname)
{
        int ret = -1;
        int maxtry = 10;
        
        while (maxtry-- > 0) {
                ret = delete_module(modname, O_NONBLOCK | O_EXCL);
                if (ret < 0 && errno == EAGAIN)
                        usleep(500000);
                else
                        break;
        }
        
        if (ret != 0)
        ALOGD("Unable to unload driver module \"%s\": %s\n",
                modname, strerror(errno));
        return ret;
}


static char * get_module_name(char * buf)
{
    
        char ch = '\"';
        char * position1 ;
        char * position2 ;
        int s1 = 0,s2 = 0,k = 0;
        
#ifdef  DEBUG
        ALOGD("buf:%s",buf);
#endif
        if(!strlen(buf)){
                ALOGD("the buf is null !");
                return NULL;
        }
        memset(&name,0,sizeof(name));
        position1 = strchr(buf,ch);
        position2 = strrchr(buf,ch);
        s1 = position1 - buf + 1;
        s2 = position2 - buf;
#ifdef  DEBUG
        ALOGD("position1:%s,position2:%s,s1:%d,s2:%d",position1,position2,s1,s2);
#endif
        if((position1 == NULL) ||(position2 == NULL) || (s1 == s2)){
                return NULL;
        }
        while(s1 != s2)
        {
                name[k++] = buf[s1++];
        }
        name[k] = '\0';
        
#ifdef  DEBUG        
        ALOGD("name : %s\n",name);
#endif

        return name;
}

static int insmod_modules(char * buf)
{
        char * module_name;
        char insmod_name[128];
        char ko[] = ".ko";
        memset(&insmod_name,0,sizeof(insmod_name));
        
        module_name = get_module_name(buf);
#ifdef DEBUG
        ALOGD("module_name:%s\n",module_name);
#endif
        if(module_name != NULL){
                sprintf(insmod_name,"%s%s%s",INSMOD_PATH,module_name,ko);
                
#ifdef  DEBUG
                ALOGD("start to insmod %s\n",insmod_name);
#endif                
                if (insmod(insmod_name, "") < 0) {
                        ALOGD(" %s insmod failed!\n",insmod_name);
                        rmmod(module_name);//it may be load driver already,try remove it and insmod again.
                        if (insmod(insmod_name, "") < 0){
                                ALOGD("%s,Once again fail to load!",insmod_name);
                                return 0;
                        }
                } 
        }
        return 1;
}

static int is_alpha(char chr)
{
        int ret = -1;
        ret = ((chr >= 'a') && (chr <= 'z') ) ? 0 : 1;
        return ret;
}

static int get_cfg(void)
{
        FILE *fp;
        char buf[LINE_LENGTH] = {0};
        memset(&buf,0,sizeof(buf));
        if((fp = fopen(I2C_DEVICE_CONFIG_PATH,"rb")) == NULL) {
                ALOGD("can't not open file!\n");
                return 0;
        }
        
        while(fgets(buf, LINE_LENGTH , fp)){
#ifdef DEBUG
                ALOGD("buf:%s\n",buf);
#endif
                if (!is_alpha(buf[0])){
                        
                        if(!insmod_modules(buf)){
                                ALOGD("insmod fail !");
                                return 0;
                        }
                } 
                memset(&buf,0,sizeof(buf));
        }  
          
        fclose(fp);
        return 1;
    
}

 int insmodDevice(void)
{ 
        if(!get_cfg()){
                ALOGD("open fail!\n");
        }
        
        return 0;
}
