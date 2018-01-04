#ifndef _CDR_MEDIA_RECORDER_H
#define _CDR_MEDIA_RECORDER_H

#include <HerbMediaRecorder.h>
#include "misc.h"
#include "camera.h"
#define BFTIMEMS (1000*0)       /* the record time before impact, ms */
#define AFTIMEMS (1000*0)      /* the record time after impact */

struct SpsPps {
	unsigned char *ptr;
	unsigned int length;
};

struct MRecorderHandle{
	int videoSizeWidth;
	int videoSizeHeight;
	int bitRate;
	int frameRate;
	int sampleRate;
	int encodeStatus;
	int recordStatus;
	int audioRecordstatus;
	SpsPps sps;
	SpsPps pps;
};


typedef struct {
	fileType_t video_type;
	uint32_t fileSize;
	int duration;		/* unit: s*/
}CdrRecordParam_t;
using namespace android;
class CdrMediaRecorder;
class RecordListener
{
public:
	RecordListener(){}
	virtual ~RecordListener(){}
	virtual void recordListener(CdrMediaRecorder* cmr, int what, int extra) = 0;
};

class ErrorListener
{
public:
	ErrorListener(){}
	virtual ~ErrorListener(){}
	virtual void errorListener(CdrMediaRecorder* cmr, int what, int extra) = 0;
};

typedef enum{
	RECORDER_ERROR			= 0,
	RECORDER_NOT_INITIAL	= 1 << 0,
	RECORDER_IDLE			= 1 << 1,
	RECORDER_PREPARED		= 1 << 2,
	RECORDER_RECORDING		= 1 << 3
}CdrRecorderState_t;

class CdrMediaRecorder : public HerbMediaRecorder::OnInfoListener
						,public HerbMediaRecorder::OnErrorListener
						,public HerbMediaRecorder::OnDataListener
{
public:
	CdrMediaRecorder(int cam_id, void *rtspSession=NULL);
	~CdrMediaRecorder();
	void setRecordParam(CdrRecordParam_t &param);
	void getRecordParam(CdrRecordParam_t &param);
	void onInfo(HerbMediaRecorder *pMr, int what, int extra);
	void onError(HerbMediaRecorder *pMr, int what, int extra);
	void onData(HerbMediaRecorder *pMr, int what, int extra);
	status_t initRecorder(HerbCamera *mHC, Size &videoSize, uint32_t frameRate, String8 &file, uint32_t fileSize ,uint32_t bitRate);
	status_t start(void);
	void setOnInfoCallback(RecordListener* listener);
	void setOnErrorCallback(ErrorListener* listener);
	int stop(void);
	void release();
	void setDuration(int ms);
	void setNextFile(String8 &file, unsigned int filesize, int nMuxerId);
	void setFile(int fd, unsigned int filesize);
	void setSilent(bool mode);
	void setRecordTime();
	int getCameraID(void);
	CdrRecorderState_t getRecorderState(void);
//	int startExtraFile(int fd);
//	int stopExtraFile();
	void setSdcardState(bool bExist);
	bool getVoiceStatus();
	int startImpactVideo(String8 &file, unsigned long size);
	void stopImpactVideo();
	bool getImpactStatus();
	void setCacheTime(int cacheTime=-1);
	
	int sendRtspStreamData(void *cookie, unsigned char *buffer, unsigned int size, long long *pts);
	SpsPps *getSPS();
	SpsPps *getPPS();
	int __parseHeadData();
#ifdef PLATFORM_0
	void setOutputThumbState(bool bSwith);
	void setRecorderOutputThumb(HerbCamera *hc);
#endif
	void *getRtspSession();
	void reset();
private:
	void setDuration();
	HerbMediaRecorder *mHMR;
	int mMuxerId;
	CdrRecordParam_t mParam;
	RecordListener* mListener;
	ErrorListener* mErrListener;
	int mCamId;
	int mDuration;
	Mutex mLock;
	bool mSilent;
	CdrRecorderState_t mCurrentState;
	bool mStartImpactFile;

	void *mRtspSession;
	bool mSpsPpsFlag;
	MRecorderHandle *mMRecorderHandle;
	sp<VEncBuffer> mFrame;
	sp<IMemory> mMem;
	int mCacheTime;
	//int mImpactFd;
};

#endif
