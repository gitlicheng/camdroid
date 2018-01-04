#include <cutils/log.h>
#include <CdrServer.h>
#include <IPCMsgProc.h>
#include <IPCBroadCastProc.h>
#include <IPCNATInterface.h>
#include <DataStruct.h>

#include "debug.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "CdrServer"

int status_cb(void *cookie, int status, void * argv[]);

static void handleHttpServerMsg(void * cookie, char *url, int msg) {
	db_msg("handleHttpServerMsg()..url = %s, msg = %d", url, msg);
	CdrServer *p_this = (CdrServer *)cookie;
	if(p_this == NULL) {
		db_msg("handleHttpServerMsg()..null cookie.");
		return;
	}

	/*write date start*/
	if (msg == WRITE_START){
		db_msg("NAT_CMD_SET_AUTO_TICK 5031: write date start");
		IPCSendToPhoneMsg(0, NAT_CMD_SET_AUTO_TICK, 1, sizeof(int));
	}/*other things*/
	else{
		db_msg("NAT_CMD_SET_AUTO_TICK 5031: write data end");
		IPCSendToPhoneMsg(0, NAT_CMD_SET_AUTO_TICK, 0, sizeof(int));
	}
	
	char filePath[128] = {0};
	http_srv_get_abspath(filePath, url);
	db_msg("handleHttpServerMsg()..filePath = %s", filePath);
}

static void *httpServerThread(void *ctx) {
	CdrServer *ipcSrv = (CdrServer *)ctx;
//	ipcSrv->mConnManager->getIp(ipstr);
	http_srv_open(ipcSrv->mHttpServer, (char*)"192.168.100.1");

	ALOGV("Http server thread exit.");
	return NULL;
}

CdrServer::CdrServer(CdrMain *cdrMain)
{
	db_msg("CdrSever!!!");
	mRtspServer = rtsp_server_init();
	mRtspSession = rtsp_create_session(mRtspServer, (char*)"SessionMain", SESSION_VIDEO_H264);
	if (!mRtspSession) {
		db_msg("rtsp session is null");
	}
	set_status_callback(mRtspServer, mRtspSession, cdrMain, status_cb);
	mCdrMain = cdrMain;
	mHttpServer = http_srv_init();
	db_msg("end!!!");
}

CdrServer::~CdrServer()
{
	http_srv_close(mHttpServer);
	destroy_rtsp_server(mRtspServer);
	http_srv_release(mHttpServer);
}

int __handleEasycamCmd(int cookie, int cmd, int param_in, int param_out)
{	
	ALOGD("~~~~~cmd :%d", cmd);
	CdrServer *cdrServer = (CdrServer*)cookie;
	return cdrServer->sessionControl(cmd, param_in, param_out);
}

/* record and plan */
static void registerCallbacks(void *p_this) {
	NativeFunc cbs;
	cbs.handleEasycamCmd    = __handleEasycamCmd; // 
	RegisterNativeFunctions(cbs, (void *)p_this);
}

int status_cb(void *cookie, int status, void * argv[])
{
	CdrMain* cdrMain = (CdrMain*)(cookie);
	if (status == 1)
	{
		cdrMain->sessionChange(true);
		printf("*************status = start\n");
		return 1;
	}
	else
	{
		cdrMain->sessionChange(false);
		printf("*************status = stop\n");
		
		return 0;
	}
	return 1;
}

void *rtsp(void *arg)
{

	ALOGD("create session(SessionMain)\n");
	
	rtsp_server_start(arg);
	return 0;
}

int CdrServer::start()
{
	registerGetDataOrEnd(mHttpServer, (void *)this, handleHttpServerMsg);
	int ret = 0;

	registerCallbacks(this);
	pthread_t mBroadThread, mTcpSrvThread, iThreadId, iHttpThreadId;
	int err = pthread_create(&mBroadThread, NULL, IPCBroadCastClient , (void*)"uuid");
	if(err || !mBroadThread) {
		ALOGE("Create IPCBroadCastClient thread error.");
		return -1;
	}
	
	err = pthread_create(&mTcpSrvThread, NULL, IPCTCPServer , NULL);
	if(err || !mTcpSrvThread) {
		ALOGE("Create IPCTCPServer thread error.");
		return -1;
	}

	pthread_create(&iThreadId, NULL, rtsp, mRtspServer);
	pthread_create(&iHttpThreadId, NULL, httpServerThread, this);
	return ret;
}

void* CdrServer::getSession()
{
	db_msg("cdrserver: mRtspSession %p", mRtspSession);
	return mRtspSession;
}

int CdrServer::sessionControl(int cmd, int param_in, int param_out)
{
	int ret = 0;
    db_msg(" ");
	if (cmd == NAT_CMD_TURN_OFF_WIFI) {//¡Ì
		ret = mCdrMain->wifiEnable(0);
		if (ret != 0)
			return -1;
//		return ret;
	} else if (cmd == NAT_CMD_SET_WIFI) {//¡Ì
		aw_cdr_wifi_setting *pwd = (aw_cdr_wifi_setting*)param_in;
		ret = mCdrMain->wifiSetPwd(pwd->old_pwd, pwd->new_pwd);
		db_msg("set old pwd:%s, new pwd:%s", pwd->old_pwd, pwd->new_pwd);
		return ret;
	} else if (cmd == NAT_CMD_REQUEST_FILE ) {
		aw_cdr_file_trans* in = (aw_cdr_file_trans*)param_in;
		in->local_port = 31601;
		strcpy(in->local_ip, "192.168.100.1");
		if (in->from_to == NAT_CMD_APK_TO_CDR)
		{
			//db_msg("somebody ")
			return 0;
		}
		else if (in->from_to == NAT_CMD_CDR_TO_APK)
		{
			ret = http_srv_getURL(mHttpServer, in->filename, "192.168.100.1", in->url);
			db_msg("in->filename(%s), url(%s)", in->filename, in->url);
			if (access(in->filename, R_OK) != 0)
			{
				ret = -1;
				return ret;
			}
			return 0;
		}
	}
    db_msg(" ");
	ret = mCdrMain->sessionControl(cmd, param_in, param_out);
	return ret;
}

int CdrServer::notify(int cmd, int param_in)
{
	int ret = 0;
	switch(cmd)
	{
		case NAT_EVENT_SOS_FILE:
			ret = IPCSendToPhoneMsg(0, cmd, param_in, 0);
		break;
	}

	return ret;

}

int CdrServer::reply(int cmd, int param_in)
{
	return IPCSendToPhoneMsg(0, cmd, (int)&param_in, sizeof(int));
}