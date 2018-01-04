#ifndef _RTSP_CLIENT_CPP_H_
#define _RTSP_CLIENT_CPP_H_

class RtspClient
{
public:
	RtspClient(char *username, char *password);
	~RtspClient();
public:
	class GetDataListener {
	public:
		GetDataListener(){
			mSampleRate = 8000;
			mNumChannels = 0;
		};
		virtual ~GetDataListener(){};
		virtual void feedData(void *buffer, int len, long long pts) = 0;// 修改，feedData添加参数
//		virtual void rtspStatus(int status) = 0;
	public:
		unsigned mSampleRate;
		unsigned mNumChannels;
	};

	int openStream(char *url);
	int registerListener(GetDataListener *Listener);
	int closeStream();
	
	// if you want to use TCP, please set before openRtspStream
	// true: use TCP, false: use UDP, default: UDP
	void setTCPIfNessary(bool bUseTCP); 
	void setP2PPortAndIP(int rtsp_port, int rtp_ip, int rtp_port);
public:
	GetDataListener *m_RecvData;
private:
	void *m_envClient;
};

#endif
