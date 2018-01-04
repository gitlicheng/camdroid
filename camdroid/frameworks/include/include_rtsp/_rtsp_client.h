#ifndef _RTSP_CLIENT_INTERFACE_H_
#define _RTSP_CLIENT_INTERFACE_H_

class RtspGetDataListener {
	public:
	RtspGetDataListener(){
		mSampleRate = 8000;
		mNumChannels = 0;
	};
	virtual ~RtspGetDataListener(){};
	virtual void feedData(void *buffer, int len, long long pts) = 0;// 修改，feedData添加参数
	virtual void rtspStatus(int status) = 0;
public:
	unsigned mSampleRate;
	unsigned mNumChannels;
};

int openRtspStream(void *rtspRes, char *url);
int registerLiveListener(void *rtspRes, RtspGetDataListener *Listener);
int closeRtspStream(void *rtspRes);
void *createRtspRes(char *username, char *password);
int destroyRtspRes(void *rtspRes);
void setMediaSessionPort(void * res,int  port,  int sourceIP, int sourceport);

// if you want to use TCP, please set before openRtspStream
// true: use TCP, false: use UDP, default: UDP
void setRtspTCPIfNessary(void *rtspRes, bool bUseTCP);

#endif


