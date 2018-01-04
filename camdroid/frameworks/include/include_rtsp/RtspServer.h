#ifndef __RTSP_SERVER_CPP_H__
#define __RTSP_SERVER_CPP_H__

enum RtspStreamType {
	RTSP_STREAM_H264,
	RTSP_STREAM_ACC,
	RTSP_STREAM_AMR
};

enum RtspMessage {
	RTSP_SESSION_DISCONNECTED,
	RTSP_SESSION_CONNECTED,
	RTSP_SET_STREAM_BITRATE = 3,
	RTSP_SET_STREAM_FRAMERATE = 4,
	RTSP_SET_STREAM_RESOLUTION = 5
};

enum RtspNotification {
	RTSP_NOTIFY_DATA_AVAILABLE
};

enum RtspSendDataCBPts {
	RTSP_SEND_DATA_PTS_NOT_AVAILABLE = -1
};

class RtspServerSession
{
public:
	RtspServerSession(void *rtspRes, RtspStreamType type, const char *session_name);
	~RtspServerSession();
public:
	class RtspMsgHandler{
	public:
		RtspMsgHandler(){};
		virtual ~RtspMsgHandler(){};
		
		virtual int handleRtspMsg(void *cookie, int msg, void *argv[]) = 0;
	};
	void setMsgHandler(void *cookie, RtspMsgHandler *hdlr);
	
	class RtspStreamDataSender {
	public:
		RtspStreamDataSender() {};
		virtual ~RtspStreamDataSender() {};
		
		virtual int sendRtspStreamData(void *cookie, unsigned char *buffer, unsigned int size, long long *pts) = 0;
	};
	void setStreamDataSender(void *cookie, RtspStreamDataSender *sender);
	
	class ADTSInfo 
	{
	public:
		ADTSInfo(unsigned samplingFrequency, unsigned numChannels, unsigned profile);
		~ADTSInfo();
	public:
		unsigned m_samplingFrequency;
		unsigned m_numChannels;
		unsigned m_profile;
	};
	void setADTSInfo(ADTSInfo *adtsInfo);
	
	int sendNotify(const int msg, const int value);
	int getUrl(const char *ipstr, char *url);
	
	
	void setSessionDest(char * ip, int port, int dst_rtp_port, int dst_rtcp_port);
	void setSessionBitRateAdaptFlag(int istrue);
	void setSessionMode(int mode, int width , int height, int bitrate, int framrate, int iframeinter);
	
	char getSetupStreamFlag();
	void setSetupStreamFlag(char ch);

public:
	RtspMsgHandler *m_StatusCB;
	RtspStreamDataSender *m_SendDataCB;
	void *m_SendDataCookie;
	void *m_StatusCookie;
	
	void *m_RtspEnvHandler;
	void *m_SessionHandler;
	int m_type;
};


class RtspServer
{
public:
	RtspServer();
	~RtspServer();
public:
	RtspServerSession *createSession(RtspStreamType type, const char *session_name);

	int exitSession(RtspServerSession *sessionID);
	
	int start();			//Start RTSP message loop
	int stop();			//Stop RTSP message loop
private:
	void *m_RtspEnvHandler;
};

#endif //__RTSP_SERVER_INTERFACE_H__

