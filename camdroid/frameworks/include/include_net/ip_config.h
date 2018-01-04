#ifndef __IP_CONFIG_H__
#define __IP_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern int config_ip(void);  
extern int get_eth0_ip(char * locIP);
extern int set_eth0_ip(const char *ipaddr);  
extern int isValidIP(const char * ip);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //__IP_CONFIG_H__
