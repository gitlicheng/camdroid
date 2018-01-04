#include <stdio.h>
#include <stdarg.h> 
#include<stdlib.h>
#include <unistd.h>
#include "hardware_legacy/wifi.h"
#include "hardware_legacy/wifi_api.h"
#include "hardware_legacy/softap.h"
#include <pthread.h>
#include "cutils/properties.h"

#define LOG_TAG "WifiHW"
#include "cutils/log.h"

#define IFNAME "wlan0"
#define BUF_SIZE 256
#define EVENT_BUF_SIZE 2048

typedef  unsigned char uchar;

int wifi_last_ip_addr=0;
unsigned int WIFI_ENABLE_STATUS = 0;

wifiCallBack eventCallback = NULL;
void * m_cookies = NULL;

// android_wifi_enable return value 0 fail ,1 success
int copy_wifi_conf()
{
	int ret;
#ifdef MT7601_WIFI_USED
	ret = access("/data/misc/wifi/RT2870STA.dat", F_OK);
	if(ret == -1)
	{
	    
		system("cp /etc/wifi/RT2870STA.dat /data/misc/wifi/RT2870STA.dat");
		ALOGE(" RT2870sta\n");
	}
#endif
	ret = access("/data/misc/wifi/wpa_supplicant.conf", F_OK);
	if(ret == -1)
	{
		system("cp /etc/wifi/wpa_supplicant.conf /data/misc/wifi/wpa_supplicant.conf");
		chmod("/data/misc/wifi/wpa_supplicant.conf",0660);
		chown("/data/misc/wifi/wpa_supplicant.conf", 1000, 1010);
		
		usleep(100000);
	}

#ifdef USE_GO_AS_SOFTAP
	
	ret = access("/data/misc/wifi/p2p_supplicant.conf", F_OK);
	if(ret == -1)
	{
		system("cp /etc/wifi/p2p_supplicant.conf /data/misc/wifi/p2p_supplicant.conf");
		chmod("/data/misc/wifi/p2p_supplicant.conf",0660);
		chown("/data/misc/wifi/p2p_supplicant.conf", 1000, 1010);
		
		usleep(100000);
	}
#endif

	return 0;
}
pthread_t ntid;

void * waitEventThread()
{
    int i = -1;
	int arg;
	char buf[512];
	char temp_property[PROPERTY_VALUE_MAX];
	char * read_results = NULL;
	
	i = wifi_connect_to_supplicant(IFNAME);

	if(i != 0)
	{
		ALOGE("[waitEventThread] wifi_connect_to_supplicant failed return !\n");
		return NULL;
	}
	
    while(1)
    {
	    i= -1;
		if(!WIFI_ENABLE_STATUS)break;
		property_get("init.svc.wpa_supplicant", temp_property, "original");
		if(strcmp(temp_property,"running") !=0 )
		{
			ALOGE("[waitEventThread] wpa_supplicant is not running, break \n");
			break;
		}

	    read_results = android_wifi_waitForEvent(buf,sizeof(buf));
		
		if(read_results == NULL)
		{
			//mzhu need break or not ?
			ALOGE("[waitEventThread]: android_wifi_waitForEvent failed break !!!\n");
			break;
		}
		
		
		if(strncmp(buf,STATE_CHANGE,strlen(STATE_CHANGE))==0)
		{
		    
		}
		else if(strncmp(buf,SCAN_NEW_BSS,strlen(SCAN_NEW_BSS))==0)
		{
		     
		}
		else if(strncmp(buf,SCAN_RESULT,strlen(SCAN_RESULT))==0)
		{
		    ALOGE("[waitEventThread] read : [%s]\n",buf);

			if(eventCallback != NULL )
			eventCallback(EVENT_SCAN_RESULT,i,m_cookies);	
		}
		else if(strncmp(buf,BSS_CONNECTED,strlen(BSS_CONNECTED))==0)
		{
		    ALOGE("[waitEventThread] read : [%s]\n",buf);

		    char * p = strstr(buf,"id=");
		    if( p !=NULL)
			i = atoi(p+3);

			if(eventCallback != NULL )
				eventCallback(EVENT_CONNECTED,i,m_cookies);
			
		}
		else if(strncmp(buf,BSS_DISCONNECTED,strlen(BSS_DISCONNECTED))==0)
		{
		    ALOGE("[waitEventThread] read : [%s]\n",buf);
		
		    char * p = strstr(buf,"id=");
		    if( p !=NULL)
			i = atoi(p+3);

			char * res = strstr(buf,"reason=3");
			if( res !=NULL)
			{
				ALOGE("CTRL-EVENT-DISCONNECTED continue  reason=3 !!! \n");
				if(eventCallback != NULL )
					eventCallback(EVENT_DISCONNECTED_BY_USER,i,m_cookies);
				
				continue;
			}

			if(eventCallback != NULL )
				eventCallback(EVENT_DISCONNECTED,i,m_cookies);
		}
		else if(NULL != strstr(buf, "auth_failures="))
		{
		
	ALOGE("Wrong PASSWORD do break connect  !!! \n");
			disconnectAp(0);

			if(eventCallback != NULL )
				eventCallback(EVENT_WRONG_PSK, i, m_cookies);

		}
        	
    }
	ALOGE("waitEventThread exit !!! \n");
	return (void*)0;

}

int wifi_readEvent()
{

	pthread_create(&ntid,NULL,waitEventThread,NULL);
	pthread_detach(ntid);

	//pthread_join(ntid, NULL); 
return 0;
}
/***********
 void test(event_t event,int arg,void *cookie)
 {

 }
**************/
int wifi_setCallBack(wifiCallBack pHandler,void * cookies)
{
ALOGE("setcallback\n");
eventCallback = pHandler;
m_cookies = cookies;
return 1;
}
int  android_wifi_enable()
{
    int i = 0; 
	
	ALOGE ("wifi_enable\n");
	copy_wifi_conf();	
    //disableSoftAp(); 互斥的时候调用
	// is_wifi_driver_loaded return value 0 = FAIL   1==SUCCESS 
	if(is_wifi_driver_loaded() == 0)  
	{
        ALOGE("the driver is not loaded,start load driver ...\n");
		i=wifi_load_driver();
		//wifi_load_driver return value -1 == FAIL   0==SUCCESS 
		ALOGE("the wifi_load_driver return value is %d \n" ,i);
		if(i == -1)
		{
		    ALOGE("wifi_load_driver is fail\n");
		    return 0;
		}

	}
	i=wifi_start_supplicant(0);
	//wifi_start_supplicant return value : 0=success -1==fail
	ALOGE("the wifi_start_supplicant return value is %d \n" ,i);
	if(i == -1)
	{
	      ALOGE("wifi_start_supplicant is fail\n");
		  return 0;
	}
	i= wifi_connect_to_supplicant(IFNAME);
	//wifi_connect_to_supplicant return value 0->ok   -1->fail
	
	ALOGE("the wifi_connect_to_supplicant %s value is %d \n" ,IFNAME,i);
	if (i == -1)
	{
	    ALOGE("wifi_connect_to_supplicant wlan0  is fail\n");
	    return 0;
	}

#ifdef USE_GO_AS_SOFTAP
	
	i= wifi_connect_to_supplicant("wlan1");
	//wifi_connect_to_supplicant return value 0->ok   -1->fail
	
	ALOGE("the wifi_connect_to_supplicant %s value is %d \n" ,"wlan1",i);
	if (i == -1)
	{
	    ALOGE("wifi_connect_to_supplicant is fail\n");
	    return 0;
	}
#endif


	WIFI_ENABLE_STATUS = 1;
	wifi_readEvent();
	//wifi_setCallBack((void *)&test,NULL);
	return 1;
}

static int G_errorTimes = 0;
static int doCommand(const char *ifname, const char *cmd, char *replybuf, int replybuflen)
{
    size_t reply_len = replybuflen - 1;

    if (wifi_command(ifname, cmd, replybuf, &reply_len) != 0)
    {
		G_errorTimes++;
		if(G_errorTimes >= 4)
		{
			ALOGE("wifi_command failed G_errorTimes [%d], need reset wpa_supplicant! \n", G_errorTimes);
			android_wifi_disable();
			android_wifi_enable();
			return -2;
	
		}
        return -1;
    }
    else {
        // Strip off trailing newline
        G_errorTimes = 0;
        if (reply_len > 0 && replybuf[reply_len-1] == '\n')
            replybuf[reply_len-1] = '\0';
        else
            replybuf[reply_len] = '\0';
        return 0;
    }
}


static int doIntCommand( const char* fmt, ...)
{
    char buf[BUF_SIZE];
    va_list args;
    va_start(args, fmt);
    int byteCount = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (byteCount < 0 || byteCount >= BUF_SIZE) {
        return -1;
    }
    char reply[BUF_SIZE];
    if (doCommand(IFNAME, buf, reply, sizeof(reply)) != 0) {
        return -1;
    }
    return atoi(reply);
}


static BOOL doBooleanCommand( const char* expect, const char* fmt, ...)
{
    char buf[BUF_SIZE];
    va_list args;
    va_start(args, fmt);
    int byteCount = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (byteCount < 0 || byteCount >= BUF_SIZE) {
        return FALSE;
    }
    char reply[BUF_SIZE];
    if (doCommand(IFNAME, buf, reply, sizeof(reply)) != 0) {
        return FALSE;
    }
	ALOGE("doBooleanCommand reply  :%s \n",reply);
    return (strcmp(reply, expect) == 0);
}

static char* doStringCommand( char* reply,int replybuflen,const char* fmt, ...) {
    char buf[BUF_SIZE];
    va_list args;
    va_start(args, fmt);
    int byteCount = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (byteCount < 0 || byteCount >= BUF_SIZE) {
        return NULL;
    }

    if (doCommand(IFNAME, buf, reply, replybuflen) != 0) {
        return NULL;
    }
    // TODO: why not just NewStringUTF?
    
    return reply;
}
BOOL android_wifi_doBooleanCommand( char * cmd)
{
    if (cmd == NULL) {
        return FALSE;
    }
    ALOGE("android_wifi_doBooleanCommand: %s\n",cmd);
    return doBooleanCommand("OK", "%s", cmd);
}
int android_wifi_doIntCommand(char * cmd)
{
    if (cmd == NULL) {
        return -1;
    }
    ALOGE("android_wifi_doIntCommand: %s",cmd);
    return doIntCommand("%s",cmd);
}
char* android_wifi_doStringCommand( char* reply,int replybuflen,char * cmd)
{
    if (cmd == NULL) {
        return NULL;
    }
    ALOGE("android_wifi_doStringCommand: %s",cmd);
    return doStringCommand(reply,replybuflen,"%s",cmd);
}
BOOL android_wifi_scan()
{return android_wifi_doBooleanCommand("SCAN");}


char* android_wifi_scanResults(char* replybuf,int bufsize)
{
    return android_wifi_doStringCommand(replybuf,bufsize,"BSS RANGE=ALL MASK=0x1986");
}


char* android_wifi_getStatus(char* replybuf,int bufsize) 
{
    return android_wifi_doStringCommand(replybuf,bufsize,"STATUS");
 }

char* android_wifi_getMacAddress(char* replybuf,int bufsize)
 {
    return android_wifi_doStringCommand(replybuf,bufsize,"DRIVER MACADDR");
 }
BOOL android_wifi_ping()
{
char replybuf[256];
 
android_wifi_doStringCommand(replybuf,sizeof(replybuf),"PING");
if((NULL != replybuf)&& (strcmp(replybuf,"PONG") == 0))
return TRUE;
return FALSE;
} 
	
char* android_wifi_listNetworks(char* replybuf,int bufsize) 
{
return android_wifi_doStringCommand(replybuf,bufsize,"LIST_NETWORKS");
}
char *wifi_getNetworkSsid(char *ssid,int size ,int netId)
{
char cmd[64];
char temp_str[64];
char *p;

sprintf(cmd,"%s %d %s","GET_NETWORK",netId,"ssid");

p= android_wifi_doStringCommand(temp_str,60,cmd);

if(p==NULL)return NULL;

if(size > strlen(temp_str) - 2) size = strlen(temp_str) -2;
strncpy(ssid,temp_str + 1,size);

return ssid;
}

char *wifi_getNetworkBssid(char *bssid,int size ,int netId)
{
char cmd[64];
char temp_str[64];
char *p;

sprintf(cmd,"%s %d %s","GET_NETWORK",netId,"bssid");
p= android_wifi_doStringCommand(temp_str,60,cmd);

if(p==NULL)return NULL;

if(size > strlen(temp_str) - 2) size = strlen(temp_str) -2;
strncpy(bssid,temp_str + 1,size);

return bssid;
}

int android_wifi_addNetwork()
{
return android_wifi_doIntCommand("ADD_NETWORK");
}


BOOL android_wifi_setNetworkVariable(int netId, char* name, char* value)
{
    char cmd[255];
    if(netId<0||name==NULL||strlen(name)==0)
    return FALSE;
	memset(cmd,0,sizeof(cmd));

    if(value ==NULL ||strlen(value)==0)
	{
	    sprintf(cmd,"SET_NETWORK %d key_mgmt NONE",netId);
	}
	else
	{
	    sprintf(cmd,"SET_NETWORK %d %s \"%s\"",netId,name,value);
	}
	ALOGE("android_wifi_setNetworkVariable cmd:%s \n",cmd);
    return android_wifi_doBooleanCommand(cmd);
}

BOOL android_wifi_setNetworkVariable2(int netId, char* name, char* value)
{
    char cmd[255];
    if(netId<0||name==NULL||strlen(name)==0)
    return FALSE;
	memset(cmd,0,sizeof(cmd));
	
    if(value ==NULL ||strlen(value)==0)
	{
	    sprintf(cmd,"SET_NETWORK %d key_mgmt NONE",netId);
	}
	else
	{
	    sprintf(cmd,"SET_NETWORK %d %s %s",netId,name,value);
	}
	ALOGE("android_wifi_setNetworkVariable cmd:%s \n",cmd);
    return android_wifi_doBooleanCommand(cmd);
}

BOOL android_wifi_removeNetwork(int netId)
{
    char cmd[25];
	if(netId<0)return FALSE;
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"REMOVE_NETWORK %d",netId);
	return android_wifi_doBooleanCommand(cmd);	
}
BOOL wifi_selectNetwork(int netId)
{
    char cmd[25];
	if(netId<0)return FALSE;
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"SELECT_NETWORK %d",netId);
	return android_wifi_doBooleanCommand(cmd);	
}
BOOL android_wifi_removeNetwork2(char *net)
{
    char cmd[25];
	if(net == NULL || strlen(net) == 0)return FALSE;
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"REMOVE_NETWORK %s",net);
	return android_wifi_doBooleanCommand(cmd);	
}

BOOL android_wifi_enableNetwork(int netId)
{
    char cmd[25];
	if(netId<0)return FALSE;
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"ENABLE_NETWORK %d",netId);
	return android_wifi_doBooleanCommand(cmd);
}


BOOL android_wifi_disableNetwork(int netId)
{
    char cmd[25];
	if(netId<0)return FALSE;
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"DISABLE_NETWORK %d",netId);
	return android_wifi_doBooleanCommand(cmd);
}

BOOL android_wifi_reconnect()
{return android_wifi_doBooleanCommand("RECONNECT");}

BOOL android_wifi_reassociate()
{return android_wifi_doBooleanCommand("REASSOCIATE");}

BOOL android_wifi_disconnect()
{return android_wifi_doBooleanCommand("DISCONNECT");}
 
//reply buf suggest size 2048
char * android_wifi_waitForEvent(char* replybuf,int bufsize)
{
	if(replybuf==NULL||bufsize<=1)
	return NULL;
	int nread = wifi_wait_for_event(IFNAME, replybuf, bufsize-1);
	   if (nread > 0) {
	        return replybuf;
	    } else {
	        return NULL;
	    }
}


int android_wifi_disable()
{ 
	WIFI_ENABLE_STATUS =0;
	int i = -1;
	//pthread_cancel(ntid);
	i = wifi_stop_supplicant(0);
	wifi_close_supplicant_connection(IFNAME);
	if(-1==i)return 0;
	//i= wifi_unload_driver();
	//if(-1==i)return 0;
	return 1;
#ifndef USE_GO_AS_SOFTAP 
#endif		
}

int android_wifi_request_ip(int * ip)
{
     int gateway,mask,dns1,dns2,server,lease,re;
	 re = do_dhcp_request(&wifi_last_ip_addr, &gateway, &mask,
                    &dns1, &dns2, &server, &lease);
	if(re == 0) *ip =wifi_last_ip_addr;
	else *ip=0;
	return re;
}
char * int2ipstr (const int ip, char *buf) 
{  
sprintf (buf, "%u.%u.%u.%u", 
(uchar) * ((char *) &ip + 0), 
(uchar) * ((char *) &ip + 1),  
(uchar) * ((char *) &ip + 2), (uchar) * ((char *) &ip + 3)); 
return buf; 
} 

char *  android_wifi_get_ip(char *buf)
{
	if(wifi_last_ip_addr == 0)return NULL;
	return int2ipstr(wifi_last_ip_addr,buf);
	
}
int saveWifiConfig()
{
return android_wifi_doBooleanCommand("SAVE_CONFIG");
}
int setWifiEnable(int enable)
{
    int re = 0;
	if(enable)
	{
		re = android_wifi_enable();
		android_wifi_scan();
	}
	else
	{
		re=android_wifi_disable();	
	}
	return re;

}

int getWifiEnable(unsigned int *enable)
{
	*enable = WIFI_ENABLE_STATUS;
	return 1;
}

int getWifiApList(wifi_scanlist *result_list)
{
int re = 0;
int i = 0;
int index =0;
char buf[8192] = {0};

char bssid[32];
char freq[6];
char level[6];
char tsf[21];
char flags[63];
char ssid[32];

wifi_scanitem *scanitem = NULL;
memset(result_list,0,sizeof(wifi_scanlist));
android_wifi_scanResults(buf,8192);

if(strlen(buf) == 0)
re = 0;
else
{
	char *p;  
	p = strtok(buf, "\n");
	re = 1;
	while(p)  
	{ 
		switch(i)
		{case 0 :
		   
		    if(strncmp(p,"bssid=",strlen("bssid=")) == 0 && strlen(p) > 6)
			{
				memset(bssid,0,sizeof(bssid));
				strncpy(bssid,p+6,sizeof(bssid)-1);
				
			}
			
			i++;
			break;
		case 1 :
		if(strncmp(p,"freq=",strlen("freq=")) == 0 && strlen(p) > 5)
			{
				memset(freq,0,sizeof(freq));
				strncpy(freq,p+5,sizeof(freq)-1);
				}
			
			i++;
			break;
		case 2 :
		 if(strncmp(p,"level=",strlen("level=")) == 0 && strlen(p) > 6)
			{
				memset(level,0,sizeof(level));
				strncpy(level,p+6,sizeof(level)-1);
			}
			
			i++;
			break;
		case 3 :
			if(strncmp(p,"tsf=",strlen("tsf=")) == 0  && strlen(p) > 4)
			{
				memset(tsf,0,sizeof(tsf));
				strncpy(tsf,p+4,sizeof(tsf)-1);
			}
		
			i++;
			break;
		case 4 :
		    if(strncmp(p,"flags=",strlen("flags=")) == 0 && strlen(p) > 6)
			{
				memset(flags,0,sizeof(flags));
				strncpy(flags,p+6,sizeof(flags)-1);
			}
			
			i++;
			break;
		case 5:
			if(strncmp(p,"ssid=",strlen("ssid=")) == 0 && strlen(p) > 5)
			{
				memset(ssid,0,sizeof(ssid));
				strncpy(ssid,p+5,sizeof(ssid)-1);
			}
		
			i++;
			break;
		case 6 : 
			
		    index = result_list->list_count;
			scanitem = &result_list->list[index];
			strncpy(result_list->list[index].bssid,bssid,sizeof(bssid)-2);
			strncpy(result_list->list[index].freq,freq,sizeof(freq)-2);
			strncpy(result_list->list[index].level,level,sizeof(level)-2);
			strncpy(result_list->list[index].tsf,tsf,sizeof(tsf)-2);
			strncpy(result_list->list[index].flags,flags,sizeof(flags)-2);
			strncpy(result_list->list[index].ssid,ssid,sizeof(ssid)-2);
	
			result_list->list_count += 1;
	//		ALOGE("one item:%s %s %s %s %s %s\n",result_list->list[index].bssid,result_list->list[index].freq,
	//		result_list->list[index].level,result_list->list[index].tsf,result_list->list[index].flags,result_list->list[index].ssid);
		
			i=0;
			break;			
		default:
			break;
		}
		if(result_list->list_count >= SCAN_RESULT_RESULT_MAX)break;
		p = strtok(NULL, "\n");     
	}     

}
return re;
}


int getConfiguredWifiApList(wifi_connected_list *list)
{
    int re = 0;
	char buf[1024];
	char *p;  
	char network_id[5];
    char ssid[32];
    char bssid[32];
    char flags[63];
	wifi_connected_item *wifi_item = NULL;
	if(list == NULL)return re;
	
	memset(list,0,sizeof(wifi_connected_list));
    android_wifi_listNetworks(buf,1024);
	ALOGE("list of networks \n%s\n" ,buf);
	p = strtok(buf, "\n");
	p = strtok(NULL, "\n");
	
	while(p)  
	{
	    int index = list -> list_count;
		if(index >= CONNECTED_NETWORK_MAX)break;
		
		wifi_item = &list->connected_list[index];
		
		if(*p==' ')break;
		sscanf(p,"%s\t%s\t%s\t%s",wifi_item->network_id,wifi_item->ssid,wifi_item->bssid,wifi_item->flags);
		ALOGE("connected list: %s %s %s %s",wifi_item->network_id,wifi_item->ssid,wifi_item->bssid,wifi_item->flags);
		list -> list_count += 1;
		p = strtok(NULL, "\n");
		
	}
	return re;
}

int forgetWifiAp(int netId)
{
    int re = 0;
	re = android_wifi_removeNetwork(netId);
	//if(re) re =saveWifiConfig();
	return re;

}
int isWepPskValid(char *pw)
{
    int i ,j;
	if(pw == NULL ||strlen(pw) == 0)return 0;
	i = strlen(pw);
	
	if(i == 10 ||i ==26 ||i ==58)
	{
    for(j=0 ;j < i;j++)
	{
	    char t = *(pw + j);
	    if(!(( t >= '0' && t <= '9' ) || ( t >= 'a' && t <= 'f' ) || ( t >= 'A' && t <= 'F' )))break;
	}
	
	return i == j;
	}
	return 1;
	
}
int getHexValue(char ch)
{
    int num = -1;
    if('0' <= ch && '9'>=ch)
		num =ch -'0';
	else if('a' <= ch && 'f'>=ch)
		num =ch -'a' +10;
	else if('A' <= ch && 'F'>=ch)
		num =ch -'A' +10;
	return num;
}
void conversePsk(char *pw)
{
    int i,j;
	char charA,charB;
	int numA,numB;
    i =strlen(pw);
	if(i != 10 && i!=26)return ;
	for(j = 0;j< i/2;j++)
	{
	    charA = *(pw + 2*j);
		charB = *(pw + 2*j+1);
		numA = getHexValue(charA);
		numB = getHexValue(charB);
		
	    *((unsigned char *)pw +j) = 16 * (unsigned char )numA + (unsigned char )numB;
	}
	*(pw +j) = 0;
	
}

int connectWifiAp(const char *ssid, const char *pw,const char *Encryption)
{
    int re = -1;
    int i = -1;
	int isWepEncry = 0;
	if(strcmp(Encryption,"WEP") == 0 ||strcmp(Encryption,"wep") == 0)
        isWepEncry = 1;

	android_wifi_removeNetwork(0);
	//if wifi is connected, disconnect it , and wait for dhcp releas
	sleep(1);
	i=android_wifi_addNetwork();

	if(i<0)
	{
		ALOGE("connectWifiAp failed \n");
		return -1;
	}
	android_wifi_setNetworkVariable(i, "ssid",ssid);
	
	if(!isWepEncry)
	{
	    if(android_wifi_setNetworkVariable(i, "psk",pw)==0)
		return -1;
	}
	else 
	{
	    int len;
	    if(isWepPskValid(pw) == 0)return -1;
		 
	    android_wifi_setNetworkVariable(i,"psk",NULL);
		android_wifi_setNetworkVariable2(i,"auth_alg","OPEN SHARED");
		len = strlen(pw);
		if(len == 10 || len ==26 ||len ==58)
		{
			android_wifi_setNetworkVariable2(i,"wep_key0",pw);
		}
		else 
		{
			android_wifi_setNetworkVariable(i,"wep_key0",pw);
		}

		android_wifi_setNetworkVariable(i,"wep_tx_keyidx","0");
	}

	wifi_selectNetwork(i);
	android_wifi_enableNetwork(i);
	
	return i;
}

wifi_status getWifiStatus()
{
    wifi_status re = STATUS_UNKNOW;
	char buf[1024];
	if(NULL == android_wifi_getStatus(buf,sizeof(buf)))
	return re;
	ALOGE("\ngetstatusbuf:%s\n",buf);
	if(strstr(buf,"wpa_state=SCANNING")!=NULL ||
	      strstr(buf,"wpa_state=INACTIVE")!=NULL)
	{ 
	    re = STATUS_SCANING;
	}
	else if(strstr(buf,"wpa_state=ASSOCIATING")!= NULL)
	{
	    re = STATUS_CONNECTING;
	}
	else if(strstr(buf,"wpa_state=COMPLETED") != NULL)
	{
	    re = STATUS_CONNECTED;
	}
	return re;
}
int disconnectAp(int networkid)
{
	if(networkid <0)
		return 0;
	
	return android_wifi_disconnect();
}
