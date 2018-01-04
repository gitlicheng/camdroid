#ifndef __IPCMsgProc__H__
#define __IPCMsgProc__H__

#ifdef __cplusplus
extern "C"{
#endif 

void* IPCTCPServer(void*);
void ExitIPCTCPServer(void);

int IPCSendToPhoneMsg(int session, int cmd, int param, int param_len);
#ifdef __cplusplus
}
#endif 
#endif //__IPCMsgProc__H__
