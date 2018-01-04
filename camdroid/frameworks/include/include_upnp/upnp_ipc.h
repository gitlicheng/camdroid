#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef UPNP_IPC_H_
#define UPNP_IPC_H_

void init_upnp_ipc(const char *friendly_name, const char *uuid, int port);
void destroy_upnp_ipc();

#endif	//UPNP_IPC_H_

#ifdef __cplusplus
}
#endif /* __cplusplus */
