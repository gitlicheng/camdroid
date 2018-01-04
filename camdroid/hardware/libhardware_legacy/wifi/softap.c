#include <stdio.h>
#include <errno.h>
#include<stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include "cutils/properties.h"
#include "hardware_legacy/wifi.h"
#include "hardware_legacy/wifi_api.h"
#include "hardware_legacy/softap.h"
#include <pthread.h>

static const char HOSTAPD_CONF_FILE[]    = "/data/misc/wifi/hostapd.conf";
#define AP_BSS_START_DELAY      200000
#ifdef USE_GO_AS_SOFTAP
#if defined RTL_8188EU_WIFI_USED 
static const char INTERFACE_NAME[]    = "wlan1";
#elif defined RTL_8189ES_WIFI_USED
static const char INTERFACE_NAME[]    = "wlan1";
#elif defined MT7601_WIFI_USED
static const char INTERFACE_NAME[]    = "p2p0";
#endif
#else
static const char INTERFACE_NAME[]    = "wlan0";
#endif  
pid_t mPid = 0;
pid_t mDaemonPid = 0;
//int   mDaemonFd;
pthread_t thread_id;

#define BUF_SIZE 256
#define EVENT_BUF_SIZE 2048
int softapStatus=0;
#ifdef USE_GO_AS_SOFTAP
static char GO_NAME[64]="Ipcamera";
static char GO_PSK[64]="12345678";
#endif
static char temp_property[PROPERTY_VALUE_MAX];
int copyHostapdConf()
{
  int ret;
	ret = access("/data/misc/wifi/hostapd.conf", F_OK);
	if(ret == -1)
	{
	    system("cp /etc/wifi/hostapd.conf /data/misc/wifi/hostapd.conf");
        chmod("/data/misc/wifi/hostapd.conf",0660);
	    chown("/data/misc/wifi/hostapd.conf", 1000, 1010);
		usleep(100000);
	}
	return 0;

}
#ifdef USE_GO_AS_SOFTAP
int startForceGo()
{
    char cmd[256];
#ifdef MT7601_WIFI_USED
	int ret = access("/data/misc/wifi/RT2870STA.dat", F_OK);
	if(ret == -1)
	{
	    system("cp /etc/wifi/RT2870STA.dat /data/misc/wifi/RT2870STA.dat");
	    sleep(1);
	}
#endif
	property_get("GO.status", temp_property, "original");
	if (strcmp("running",temp_property) == 0) {
		return 0;
	}
	property_set("GO.status", "running");
#if defined MT7601_WIFI_USED	
	system("ifconfig wlan0 up");
	system("ifconfig p2p0 up");
	system("ifconfig p2p0 192.168.100.1");
	sprintf(cmd,"iwpriv p2p0 set SSID=%s",GO_NAME);
	system(cmd);
	if(GO_PSK[0]!=0)
	{
	    printf("\n encrypt GO set");
	    system("iwpriv p2p0 set AuthMode=wpa2psk");
	    system("iwpriv p2p0 set EncrypType=aes");
	    sprintf(cmd,"iwpriv p2p0 set WPAPSK=%s",GO_PSK);

	    system(cmd);
	}
	else 
	{
		printf("\n open GO set");
		system("iwpriv p2p0 set AuthMode=open");
		system("iwpriv p2p0 set EncrypType=none");
	}
	system("iwpriv p2p0 set Channel=6");
	system("iwpriv p2p0 set P2pOpMode=1");
#elif defined RTL_8188EU_WIFI_USED ||defined RTL_8189ES_WIFI_USED
	
	sprintf(cmd,"SET device_name DIRECT-%s",GO_NAME);	
	p2p_wifi_doBooleanCommand(cmd);
	sprintf(cmd,"SET_NETWORK 0 ssid \"%s\"",GO_NAME);
	p2p_wifi_doBooleanCommand(cmd);
	
	if(GO_PSK[0]!=0)
	{
		sprintf(cmd,"SET_NETWORK 0 psk \"%s\"",GO_PSK);
		p2p_wifi_doBooleanCommand(cmd);
	}
	
	p2p_wifi_doBooleanCommand("SAVE_CONFIG");
	p2p_wifi_doBooleanCommand("RECONFIGURE");
	sleep(1);
	p2p_wifi_doBooleanCommand("P2P_GROUP_ADD persistent=0");
//	p2p_wifi_doBooleanCommand("WPS_PBC");
//	p2p_wifi_doBooleanCommand("P2P_FIND");

#endif
	return 0;
}

int stopForceGo()
{
    property_set("GO.status", "stopped");
#if defined MT7601_WIFI_USED
	system("iwpriv p2p0 set P2pOpMode=0");
	//system("ifconfig p2p0 down");
#elif defined  RTL_8188EU_WIFI_USED	|| defined RTL_8189ES_WIFI_USED
	p2p_wifi_doBooleanCommand("P2P_GROUP_REMOVE wlan1");
#endif
	return 0;
}
#endif
int startSoftap() {
    pid_t pid = 1;
    int ret = 0;
    property_get("hostapd.status", temp_property, "original");
    if (strcmp("running",temp_property) == 0) {
		char path[30];
		
		property_get("hostapd.mpid", temp_property, "original");	
		sprintf(path ,"%s/%s","/proc",temp_property);
		
		if(access(path, F_OK)==0){
		printf("Softap already started\n");
		return 0;
		}
	}
   
    if ((pid = fork()) < 0) {
        printf("fork failed (%s) \n", strerror(errno));
        return -1;
    }

    if (!pid) {
        ensure_entropy_file_exists();
		property_set("hostapd.status", "running");
        if (execl("/system/bin/hostapd", "/system/bin/hostapd","-d",
                  "-e", WIFI_ENTROPY_FILE,
                  HOSTAPD_CONF_FILE, (char *) NULL)) {
            printf("execl failed (%s) \n", strerror(errno));
        }
        printf("Should never get here! \n");
        return -1;
    } else {
	    char pid_num[10];
        mPid = pid;
		snprintf(pid_num,8,"%d",pid);
        printf("Softap startap - Ok\n");
        usleep(AP_BSS_START_DELAY);
		property_set("hostapd.mpid",pid_num);
    }
    return ret;

}
int stopSoftap() {
    property_get("hostapd.status", temp_property, "original");
	
     if (strcmp("running",temp_property) != 0) {
        printf("Softap already stopped\n");
        return 0;
    }
    property_get("hostapd.mpid", temp_property, "0");
	mPid = atoi(temp_property);
	if(mPid == 0)return 0;
    printf("Stopping Softap service pid %d\n",mPid);
	
    kill(mPid, SIGTERM);
    waitpid(mPid, NULL, 0);
    property_set("hostapd.status", "stopped");
	property_set("hostapd.mpid", "0");
    mPid = 0;
    printf("Softap service stopped\n");
    usleep(300000);
    return 0;
}

int setip()
{
char cmd[80];
sprintf(cmd,"ifconfig %s 192.168.100.1 netmask 255.255.255.0",INTERFACE_NAME);
system(cmd);
return 1;
}

int startTethering() {
int num_addrs=2;
    property_get("dnsmasq.status", temp_property, "original");
	printf("enter tether\n");
    if (strcmp("running",temp_property) == 0) {
		char path[30];
		
		property_get("dnsmasq.pidnum", temp_property, "original");
		
		sprintf(path ,"%s/%s","/proc",temp_property);
        
        if(access(path, F_OK)==0){
		printf("Tethering already started");
		return 0;
		}
    }

    printf("Starting tethering services");

    pid_t pid;
   // int pipefd[2];

   // if (pipe(pipefd) < 0) {
    //    printf("pipe failed (%s)", strerror(errno));
   //     return -1;
   // }

    /*
     * TODO: Create a monitoring thread to handle and restart
     * the daemon if it exits prematurely
     */
    if ((pid = fork()) < 0) {
   //     printf("fork failed (%s)", strerror(errno));
   //     close(pipefd[0]);
    //    close(pipefd[1]);
    //    return -1;
    }

    if (!pid) {
       // close(pipefd[1]);
       // if (pipefd[0] != STDIN_FILENO) {
       //     if (dup2(pipefd[0], STDIN_FILENO) != STDIN_FILENO) {
       //         printf("dup2 failed (%s)", strerror(errno));
       //         return -1;
        //    }
        //    close(pipefd[0]);
       // }

        int num_processed_args = 4 + (num_addrs/2) + 1; // 1 null for termination
        char **args = (char **)malloc(sizeof(char *) * num_processed_args);
        args[num_processed_args - 1] = NULL;
        args[0] = (char *)"/system/bin/dnsmasq";
        args[1] = (char *)"--no-daemon";
        args[2] = (char *)"--no-resolv";
        args[3] = (char *)"--no-poll";
        // TODO: pipe through metered status from ConnService
        //args[3] = (char *)"--dhcp-option-force=43,ANDROID_METERED";
        //args[4] = (char *)"--pid-file";
        //args[5] = (char *)"";

        int nextArg = 4;
		int addrIndex;
		char start[] = "192.168.100.100";
		char end[] = "192.168.100.200";
		asprintf(&(args[nextArg++]),"--dhcp-range=%s,%s,1h", start, end);
		/*
        for (addrIndex=0; addrIndex < num_addrs;) {
            char *start = strdup(inet_ntoa(addrs[addrIndex++]));
            char *end = strdup(inet_ntoa(addrs[addrIndex++]));
            asprintf(&(args[nextArg++]),"--dhcp-range=%s,%s,1h", start, end);
        }
         */
		 property_set("dnsmasq.status","running");
        if (execv(args[0], args)) {
            printf("execl failed (%s)", strerror(errno));
        }
        printf("Should never get here!");
        free(args);
        return 0;
    } else {
	    char pid_num[10];
	    snprintf(pid_num,8,"%d",pid);
        //close(pipefd[0]);
        mDaemonPid = pid;
      //  mDaemonFd = pipefd[1];
		property_set("dnsmasq.pidnum",pid_num);
        printf("Tethering services running");
		
    }

    return 0;
}

int stopTethering() {
    property_get("dnsmasq.status", temp_property, "original");
	if(strcmp("running",temp_property) == 0){
	    property_get("dnsmasq.pidnum", temp_property, "0");
		mDaemonPid = atoi(temp_property);
	}
    if (mDaemonPid == 0) {
        printf("Tethering already stopped");
        return 0;
    }

    printf("Stopping tethering services%d \n",mDaemonPid);

    kill(mDaemonPid, SIGTERM);
    waitpid(mDaemonPid, NULL, 0);
    mDaemonPid = 0;
    //close(mDaemonFd);
    //mDaemonFd = -1;
    printf("Tethering services stopped");
	property_set("dnsmasq.status", "stopped");
	property_set("dnsmasq.pidnum", "0");
    return 0;
}

void generatePsk(char *ssid, char *passphrase, char *psk_str) {
    unsigned char psk[SHA256_DIGEST_LENGTH];
    int j;
    // Use the PKCS#5 PBKDF2 with 4096 iterations
    PKCS5_PBKDF2_HMAC_SHA1(passphrase, strlen(passphrase),
            (unsigned char *)(ssid), strlen(ssid),
            4096, SHA256_DIGEST_LENGTH, psk);
    for (j=0; j < SHA256_DIGEST_LENGTH; j++) {
        sprintf(&psk_str[j<<1], "%02x", psk[j]);
    }
    psk_str[j<<1] = '\0';
}

int fwReloadSoftap(char *argv) {
    int i = 0;
    char *fwpath = NULL;

    if (strcmp(argv, "AP") == 0) {
        fwpath = (char *)wifi_get_fw_path(WIFI_GET_FW_PATH_AP);
    } else if (strcmp(argv, "P2P") == 0) {
        fwpath = (char *)wifi_get_fw_path(WIFI_GET_FW_PATH_P2P);
    } else if (strcmp(argv, "STA") == 0) {
        fwpath = (char *)wifi_get_fw_path(WIFI_GET_FW_PATH_STA);
    }
    if (!fwpath)
        return -1;
    if (wifi_change_fw_path((const char *)fwpath)) {
        printf("Softap fwReload failed\n");
        return -1;
    }
    else {
        printf("Softap fwReload - Ok\n");
    }
    return 0;
}

/*
 * Arguments:
 *      argv[2] - wlan interface
 *      argv[3] - SSID
 *	argv[4] - Security
 *	argv[5] - Key
 *	argv[6] - Channel
 *	argv[7] - Preamble
 *	argv[8] - Max SCB
 */
#ifdef USE_GO_AS_SOFTAP
int setSoftap(char * name, char *password) {
    strcpy(GO_NAME,name);
    if(password != NULL)
    strcpy(GO_PSK,password);
    else
    GO_PSK[0]=0;

return 0;
}
#else
int setSoftap(char * name, char *password) {
    char psk_str[2*SHA256_DIGEST_LENGTH+1];
    int ret = 0, i = 0, fd;
    char *ssid, *iface;
	int argc = 8;
    char **argv = (char **)malloc(sizeof(char *) * argc);
        argv[argc-1] = NULL;
        argv[0] = (char *)"setSoftap";
        argv[1] = (char *)"set";
        argv[2] = INTERFACE_NAME;
        argv[3] = (char *)name;
        // TODO: pipe through metered status from ConnService
		if(password == NULL)
		 argv[4] = (char *)"open";
		 else
        argv[4] = (char *)"wpa2-psk";
        argv[5] = (char *)password;
        argv[6] = (char *)"6";
		

    iface = argv[2];

    char *wbuf = NULL;
    char *fbuf = NULL;

    if (argc > 3) {
        ssid = argv[3];
    } else {
        ssid = (char *)"IPCameraAp";
    }

#ifdef BROADCOM_WIFI_VENDOR
    if(fwReloadSoftap("AP") != 0) {
        printf("fwReloadSoftap AP failed\n");
    }
#endif

    asprintf(&wbuf, "interface=%s\ndriver=nl80211\nctrl_interface="
            "/data/misc/wifi/hostapd\nssid=%s\nchannel=6\nieee80211n=1\nhw_mode=g\nignore_broadcast_ssid=0\n",
            iface, ssid);

    if (argc > 4) {
        if (!strcmp(argv[4], "wpa-psk")) {
            generatePsk(ssid, argv[5], psk_str);
            asprintf(&fbuf, "%swpa=1\nwpa_pairwise=TKIP CCMP\nwpa_psk=%s\n", wbuf, psk_str);
        } else if (!strcmp(argv[4], "wpa2-psk")) {
            generatePsk(ssid, argv[5], psk_str);
            asprintf(&fbuf, "%swpa=2\nrsn_pairwise=CCMP\nwpa_psk=%s\n", wbuf, psk_str);
        } else if (!strcmp(argv[4], "open")) {
            asprintf(&fbuf, "%s", wbuf);
        }
    } else {
        asprintf(&fbuf, "%s", wbuf);
    }

    fd = open(HOSTAPD_CONF_FILE, O_CREAT | O_TRUNC | O_WRONLY | O_NOFOLLOW, 0660);
    if (fd < 0) {
        printf("Cannot update \"%s\": %s\n", HOSTAPD_CONF_FILE, strerror(errno));
        free(wbuf);
        free(fbuf);
		free(argv);
        return -1;
    }
    if (write(fd, fbuf, strlen(fbuf)) < 0) {
        printf("Cannot write to \"%s\": %s\n", HOSTAPD_CONF_FILE, strerror(errno));
        ret = -1;
    }
    free(wbuf);
    free(fbuf);
	free(argv);

    /* Note: apparently open can fail to set permissions correctly at times */
    if (fchmod(fd, 0660) < 0) {
        printf("Error changing permissions of %s to 0660: %s\n",
                HOSTAPD_CONF_FILE, strerror(errno));
        close(fd);
        unlink(HOSTAPD_CONF_FILE);
        return -1;
    }

    if (fchown(fd, 1000, 1010) < 0) {
        printf("Error changing group ownership of %s to %d: %s\n",
                HOSTAPD_CONF_FILE, 1010, strerror(errno));
        close(fd);
        unlink(HOSTAPD_CONF_FILE);
        return -1;
    }

    close(fd);
    return ret;
}
#endif
void * readP2p0Event()
{
    int i = -1;
	int arg;
	char buf[512];
	int ret = wifi_connect_to_supplicant("wlan1");
	if(ret < 0)
		return NULL;
	
    while(1)
    {
		if(!softapStatus)break;
	    ret = wifi_wait_for_event("wlan1", buf, sizeof(buf)-1);
		if(ret <= 0)
		{
			break;
		}
		
		if(strncmp(buf,"P2P-PROV-DISC-PBC-REQ",strlen("P2P-PROV-DISC-PBC-REQ"))==0)
		{
//		    p2p_wifi_doBooleanCommand("P2P_STOP_FIND");
	            sleep(1);
		    p2p_wifi_doBooleanCommand("WPS_PBC");
		
		}
	
        	
    }
	return (void*)0;

}

int enableSoftAp()
{
    int re = -1;
	int temp;
	int count =10;
	int enable =0;
	softapStatus=1;
#ifdef USE_GO_AS_SOFTAP
    getWifiEnable(&enable);
	if(!enable)setWifiEnable(1);
	startForceGo();
	
#else	
	copyHostapdConf();
	//sleep(1);
	//setWifiEnable(0);
	//signal(SIGCHLD, SIG_IGN);
	startSoftap();
#endif	
    
	//sleep(1);
	setip();
	sleep(2);
	startTethering();
	
	while(count -- >0)
	{
	    sleep(1);
	    temp =isSoftAPenabled();
	    if(temp)
	    {
	        re =0 ;
	        break;
	    }
	
	}

#ifdef USE_GO_AS_SOFTAP
	pthread_create(&thread_id,NULL,readP2p0Event,NULL);
#endif
	return re;
}

int disableSoftAp()
{
    int re = 0;
	stopTethering();
#ifdef USE_GO_AS_SOFTAP
    stopForceGo();
#else	
#ifdef BROADCOM_WIFI_VENDOR
    if(fwReloadSoftap("STA") != 0) {
        printf("fwReloadSoftap STA failed\n");
    }
#endif
	stopSoftap();
#endif	
//	p2p_wifi_doBooleanCommand("P2P_STOP_FIND");
	softapStatus=0;
	return re;
}
int isSoftAPenabled()
{
    int re = 0 ;
    char hostapd_property[PROPERTY_VALUE_MAX];
	char dnsmasq_property[PROPERTY_VALUE_MAX];
#ifdef USE_GO_AS_SOFTAP
    property_get("GO.status", hostapd_property, "original");
#else
    property_get("hostapd.status", hostapd_property, "original");
#endif
	property_get("dnsmasq.status", dnsmasq_property, "original");
	
	if(strcmp(hostapd_property,"running") == 0 && strcmp(dnsmasq_property,"running") == 0)
	re = 1;
    return re;
}
int getSoftAPIPaddr(char *IPaddr)
{
    if(IPaddr != NULL)
	strcpy(IPaddr,"192.168.100.1");
    return 0;
}

static int p2p_doCommand(const char *cmd, char *replybuf, int replybuflen)
{
    size_t reply_len = replybuflen - 1;

    if (wifi_command(INTERFACE_NAME, cmd, replybuf, &reply_len) != 0)
        return -1;
    else {
        // Strip off trailing newline
        if (reply_len > 0 && replybuf[reply_len-1] == '\n')
            replybuf[reply_len-1] = '\0';
        else
            replybuf[reply_len] = '\0';
        return 0;
    }
}
static BOOL p2p_doBooleanCommand( const char* expect, const char* fmt, ...)
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
    if (p2p_doCommand( buf, reply, sizeof(reply)) != 0) {
        return FALSE;
    }
	printf("doBooleanCommand reply  :%s \n",reply);
    return (strcmp(reply, expect) == 0);
}
BOOL p2p_wifi_doBooleanCommand( char * cmd)
{
    if (cmd == NULL) {
        return FALSE;
    }
    printf("p2p_wifi_doBooleanCommand: %s",cmd);
    return p2p_doBooleanCommand("OK", "%s", cmd);
}
