#ifndef _CDR_SERVER_H_
#define _CDR_SERVER_H_
#include <RtspServer.h>
#include "windows.h"
#include <http_server.h>

class CdrMain;
class CdrServer
{
public:
	CdrServer(CdrMain *cdrMain);
	~CdrServer();
	int start();
	void *getSession();
	int sessionControl(int cmd, int param_in, int param_out);
	int notify(int cmd, int param_in);
	int reply(int cmd, int param_in);
	http_srv* mHttpServer;
private:
	void *mRtspServer;
	void *mRtspSession;
	CdrMain *mCdrMain;
};

#endif	//_CDR_SERVER_H_
