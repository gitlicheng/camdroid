#ifndef __IPCMsgProc__H__
#define __IPCMsgProc__H__

#ifdef __cplusplus
extern "C"{
#endif 

void* IPCTCPServer(void*);
void ExitIPCTCPServer(void);
static void IPCSendMsg(char * msg, int len);
void IPCSendMotionMsg(void);
void IPCSendTFCardPlugMsg(int isPlugin);
void IPCSendConnWifiResult(int status);
void IPCSendPlayMusicEndMsg(char * filename);
#ifdef __cplusplus
}
#endif

#endif //__IPCMsgProc__H__
