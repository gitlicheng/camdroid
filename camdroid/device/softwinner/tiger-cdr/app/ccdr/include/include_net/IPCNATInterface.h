#ifndef __NAT__INTERFACE__
#define __NAT__INTERFACE__

#ifdef __cplusplus
extern "C"{
#endif	

#include <common/MediaListItem.h>
#include <common/WifiApListItem.h>

typedef struct 
{
	char devName[24];
	void * session;
	void * audio_session;
	void * recorder_session;
}NATMainThreadArgV;

/**************************************************************************
* len_in(in): length of cmd
* cmd(in):    xml
* len_out(in): max length of msg_out
* msg_out(in): format of output must be <key1>out1</key1><key2>out2</key2>
* return value: if success, return 0. otherwise, return -1
**************************************************************************/
typedef int (*HandleEasycamCmd)(int cookie, int cmd, int param_in, int param_out);

typedef struct NativeFunc
{
HandleEasycamCmd                                handleEasycamCmd;
}NativeFunc;

void* NatMain(void * in);
void ExitNatMain(void);

void RegisterNativeFunctions(NativeFunc allNativeFunc,void *);
void UnRegisterNativeFunctions(void);

#ifdef __cplusplus
}
#endif	

#endif	//__NAT__INTERFACE__
