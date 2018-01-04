#include "CdrMediaRecorder.h"
#include <RtspServer.h>
#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "CdrMediaRecorder"
#endif
#include "debug.h"

CdrMediaRecorder::CdrMediaRecorder(int cam_id, void *rtspSession)
	: mHMR(NULL),
	  mListener(NULL),
	  mErrListener(NULL),
	  mCamId(-1),
	  mCurrentState(RECORDER_NOT_INITIAL),
	  mStartImpactFile(false),
	  mCacheTime(BFTIMEMS)
	  //mImpactFd(-1)
{
	mHMR = new HerbMediaRecorder();
	mHMR->setOnInfoListener(this);
	mHMR->setOnDataListener(this);
	mHMR->setOnErrorListener(this);
	mCamId = cam_id;
	mSilent = true;
	mDuration = 60 * 1000;
	mMuxerId = -1;
	//memset(&record_param, 0, sizeof(record_param));
	
	mCurrentState = RECORDER_IDLE;
	mRtspSession = rtspSession;
	db_msg("mRtspSession :%p", mRtspSession);
	if (mRtspSession) {
		mMRecorderHandle = (MRecorderHandle*)calloc(sizeof(MRecorderHandle),1);
	}
}

void *CdrMediaRecorder::getRtspSession()
{
	return mRtspSession;
}

int CdrMediaRecorder::sendRtspStreamData(void *cookie, unsigned char *buffer, unsigned int size, long long *pts) {
	//ALOGV("MediaStreamP2p::sendRtspStreamData ");
	if(mSpsPpsFlag == true) {
		SpsPps *pps = getPPS();
		if(pps != NULL) {
			memcpy(buffer, pps->ptr, pps->length);
			mSpsPpsFlag++;
			return pps->length;
		}
		
		ALOGE("sendRtspStreamData()..pps null.");
		if(mFrame != NULL) {
			mHMR->freeOneBsFrame(mFrame, mMem);
			mFrame = NULL;
		}
		mSpsPpsFlag = 0;
		return 0;
	}
	
	if(mSpsPpsFlag == 2) {
		if(mFrame != NULL) {
			if(mFrame->data_size - 4 <= (int)size) {
				memcpy(buffer, mFrame->data + 4, mFrame->data_size - 4);
				*pts = -1;	//mFrame->pts;
				int size = mFrame->data_size - 4;
				mHMR->freeOneBsFrame(mFrame, mMem);
				mFrame = NULL;
				mSpsPpsFlag++;
				return size;
			}
			
			ALOGE("sendRtspStreamData()..send i frame error.");
			mHMR->freeOneBsFrame(mFrame, mMem);
			mFrame = NULL;
			mSpsPpsFlag = 0;
			return 0;
		}
		
		ALOGE("sendRtspStreamData()..i frame null.");
		mSpsPpsFlag = 0;
		return 0;
	}
	
	mFrame = mHMR->getOneBsFrame(0, &mMem);
	if(mFrame != NULL) {
		//ALOGV("Encode buffer size = %d", mFrame->data_size);
		if((*(mFrame->data + 4) & 0x1F) == 5) {
			SpsPps *sps = getSPS();
			if(sps != NULL) {
				memcpy(buffer, sps->ptr, sps->length);
				mSpsPpsFlag = 1;
				return sps->length;
			}
			
			ALOGE("sendRtspStreamData()..sps null.");
			mHMR->freeOneBsFrame(mFrame, mMem);
			mFrame = NULL;
			mSpsPpsFlag = 0;
			return 0;
		}
		
		if(mSpsPpsFlag > 2) {
			if(mFrame->data_size - 4 <= (int)size) {
				memcpy(buffer, mFrame->data + 4, mFrame->data_size - 4);
				*pts = -1;	//mFrame->pts;
				int size = mFrame->data_size - 4;
				mHMR->freeOneBsFrame(mFrame, mMem);
				mFrame = NULL;
				mSpsPpsFlag++;
				return size;
			}
			
			ALOGE("sendRtspStreamData()..send p frame error.");
			mHMR->freeOneBsFrame(mFrame, mMem);
			mFrame = NULL;
			mSpsPpsFlag = 0;
			return 0;
		}
		
		ALOGV("Skip p frame!!!!!!!!!!!");
		mHMR->freeOneBsFrame(mFrame, mMem);
		mFrame = NULL;
		mSpsPpsFlag = 0;
		return 0;
	}
	
	//ALOGV("Get video encode buffer null.");
	return 0;
}

void CdrMediaRecorder::onInfo(HerbMediaRecorder *pMR, int what, int extra)
{
	if (mCamId >= CAM_CNT) {

	} else {
		if(mListener) {
			mListener->recordListener(this, what, extra);
		}
	}
}

void CdrMediaRecorder::onError(HerbMediaRecorder *pMR, int what, int extra)
{
	if (mCamId >= CAM_CNT) {

	} else {	
		if(mErrListener) {
			mErrListener->errorListener(this, what, extra);
		}
	}
}

void CdrMediaRecorder::onData(HerbMediaRecorder *pMR, int what, int extra)
{
	static int count = 1;
	if (count > 0) {
		db_msg("~onData: camid %d\n", mCamId);
		count = 0;
	}
	if (mCamId >= CAM_CNT) {
		mFrame = mHMR->getOneBsFrame(0, &mMem);
		if (mFrame != NULL) {
			rtsp_data data;
			int ret = 0;
			if((*(mFrame->data + 4) & 0x1F) == 5) {
				SpsPps *sps = getSPS();
				data.frame = (char*)sps->ptr;
				data.len = sps->length;
				data.pts = -1;
				ret = send_rtsp_data(mRtspSession, 0, &data, 0);
				if (ret == -1)
				{
					db_msg("send sps error");
				}
				usleep(5000);
				SpsPps *pps = getPPS();
				data.frame = (char*)pps->ptr;
				data.len = pps->length;
				data.pts = -1;
				ret = send_rtsp_data(mRtspSession, 0, &data, 0);
				if (ret == -1)
				{
					db_msg("send pps error");
				}
				usleep(5000);
			}
			data.frame = mFrame->data + 4;
			data.len = mFrame->data_size - 4;
			data.pts = -1;
			//sendRtspStreamData(void * cookie, unsigned char * buffer, unsigned int size, long long * pts);
			ret = send_rtsp_data(mRtspSession, 0, &data, 0);
			if (ret == -1)
			{
				db_msg("send pps error");
			}
			mHMR->freeOneBsFrame(mFrame, mMem);						
		}
	} else {
		if(mListener) {
			mListener->recordListener(this, what, extra);
		}
	}
}
void CdrMediaRecorder::setRecordParam(CdrRecordParam_t &param)
{
	mParam = param;
}
void CdrMediaRecorder::getRecordParam(CdrRecordParam_t &param)
{
	param = mParam;
}
CdrMediaRecorder::~CdrMediaRecorder()
{
	delete mHMR;
	mHMR = NULL;
}
void CdrMediaRecorder::setOnInfoCallback(RecordListener* listener)
{
	mListener = listener;	
}

void CdrMediaRecorder::setOnErrorCallback(ErrorListener* listener)
{
	mErrListener = listener;	
}

SpsPps *CdrMediaRecorder::getSPS()
{
	//ALOGV("===================getSPS=======================length=%d", mMRecorderHandle->sps.length);
	return &(mMRecorderHandle->sps);
}

SpsPps *CdrMediaRecorder::getPPS()
{
	//ALOGV("getPPS=%x**",mMRecorderHandle->pps->ptr);
	//ALOGV("===================getPPS=======================length=%d", mMRecorderHandle->pps.length);
	return &(mMRecorderHandle->pps);
}

int CdrMediaRecorder::__parseHeadData()
{
	unsigned char  *bufptr = NULL;
	int bufptrLeng;
     ALOGV("__parseHeadData start");
	sp<IMemory> mem = mHMR->getEncDataHeader();
    IMemory *pmem = mem.get();
    if(pmem == NULL)
    {
        ALOGE("(f:%s, l:%d) fatal error! EncDataHeader ptr=NULL", __FUNCTION__, __LINE__);
        return -1;
    }
	bufptr     = (unsigned char*)mem->pointer();
	bufptrLeng = mem->size();
	ALOGD("bufptrLeng=%d",bufptrLeng);
	SpsPps sps,pps;
	sps.ptr = bufptr + 4;
	
	int j;
	int sps_max_size = bufptrLeng - 4 - 4;
	for(j = 0; j < sps_max_size; j++) {
		if(*(sps.ptr + j) == 0 && *(sps.ptr + j + 1) == 0
				&& *(sps.ptr + j + 2) == 0 && *(sps.ptr + j + 3) == 1) {
			break;
		}
	}
	sps.length = j;
    if(sps.length <= 0){
        ALOGE("sps.length=%d",sps.length);
        return -1;
    }    
	mMRecorderHandle->sps.ptr = (unsigned char*)calloc(sps.length,1);	
	mMRecorderHandle->sps.length = 	sps.length;

	memcpy(mMRecorderHandle->sps.ptr,sps.ptr,sps.length);
	
	pps.length = bufptrLeng - 4 - sps.length - 4 - 4;
	mMRecorderHandle->pps.length = 	pps.length;	
    if(pps.length <= 0){
        ALOGE("pps.length=%d",pps.length);
        return -1;
    }      
	mMRecorderHandle->pps.ptr = (unsigned char*)calloc(pps.length,1);	
    
	memcpy(mMRecorderHandle->pps.ptr,(sps.ptr + sps.length + 4),pps.length);

    return 0;
}

#ifdef PLATFORM_0
//enable output thumb picture from recorder
void CdrMediaRecorder::setOutputThumbState(bool bSwith)
{
	if (mHMR) {
		mHMR->setOutputThumbState(bSwith);
	}
}

void CdrMediaRecorder::setRecorderOutputThumb(HerbCamera *hc)
{
	if (mHMR)
		mHMR->setRecorderOutputThumb(hc);
}
#endif
status_t CdrMediaRecorder::initRecorder(HerbCamera *mHC, Size &videoSize, uint32_t frameRate, String8 &file, uint32_t fileSize ,uint32_t bitRate )
{
	status_t ret = 0;
	int output_format = 0;
	db_msg("mCamId:%d", mCamId);
	ret = mHMR->setCamera(mHC);
    if (ret != NO_ERROR) {
    	db_msg("setCamera Failed(%d)", ret);
   		return ret;
    }
//	if (mCamId == CAM_CSI || mCamId == CAM_CSI_NET) {
		ret = mHMR->setAudioSource(HerbMediaRecorder::AudioSource::MIC);
		if (ret != NO_ERROR) {
			return ret;
		}
//	}
	ret = mHMR->setVideoSource(HerbMediaRecorder::VideoSource::CAMERA);
	if (ret != NO_ERROR) {
    	db_error("setVideoSource Failed(%d)", ret);
   		return ret;
    }
	//ret = mHMR->setOutputFormat(HerbMediaRecorder::OutputFormat::DEFAULT);
	/*
	if (!mRtspSession) {
		ret = mHMR->setOutputFormat(HerbMediaRecorder::OutputFormat::DEFAULT);
		if (ret != NO_ERROR) {
	    	db_error("setOutputFormat Failed(%d)", ret);
	   		return ret;
	    }
	} else {
	*/

	if (mCamId >= CAM_CNT) {
		mMuxerId = mHMR->addOutputFormatAndOutputSink(HerbMediaRecorder::OutputFormat::OUTPUT_FORMAT_RAW, -1, 0, true);
		//mHMR->addOutputFormatAndOutputSink(HerbMediaRecorder::OutputFormat::OUTPUT_FORMAT_RAW, -1, fileSize, true);

	} else {	//TO_SD
#ifdef TS_MUXER
		output_format = HerbMediaRecorder::OutputFormat::OUTPUT_FORMAT_MPEG2TS;
#else
		output_format = HerbMediaRecorder::OutputFormat::MPEG_4;
#endif
		mMuxerId = mHMR->addOutputFormatAndOutputSink(output_format, (char*)file.string(), fileSize, false);
		//close(fd);
	}
	
		//if(MEDIA_ENCORDER_ACTION_DATA_ONLY_TO_APP == status)
//        {
//            
//        }
//        else if(MEDIA_ENCORDER_ACTION_DATA_ONLY_TO_SD == status)
//        {
//            mHMR->addOutputFormatAndOutputSink(HerbMediaRecorder::OutputFormat::MPEG_4, fd, fileSize, false);
//        }
//        else if(MEDIA_ENCORDER_ACTION_DATA_TO_SDAPP == status)
//        {
//            mHMR->addOutputFormatAndOutputSink(HerbMediaRecorder::OutputFormat::OUTPUT_FORMAT_RAW, -1, fileSize, true);
//            mHMR->addOutputFormatAndOutputSink(HerbMediaRecorder::OutputFormat::MPEG_4, fd, fileSize, false);
//        }
//	}
	//setFile(fd, fileSize);
	ret = mHMR->setVideoFrameRate(frameRate);
	if (ret != NO_ERROR) {
		db_error("setVideoFrameRate Failed(%d), frameRate %d", ret, frameRate);
   		return ret;
    }
#ifdef PLATFORM_1
    ret = mHMR->setVideoEncodingIFramesNumberInterval(frameRate);
    if (ret != NO_ERROR) {
    	db_error("setVideoEncodingIFramesNumberInterval(%d) Failed(%d)", frameRate, ret);
   		return ret;
    }
#endif

/*************************************************************/
	db_msg(">>>>>>>>>>>>>>>set bitrate : %dM; \n",bitRate/(1024*1024));
	int bitrate = bitRate;
    if (mCamId == CAM_UVC) {
        bitrate = BACK_CAM_BITRATE;
    }
	if (mCamId >= CAM_CNT) {
		bitrate >>= 2;	//small pic bitrate
	}
	ret = mHMR->setVideoEncodingBitRate(bitrate);
	if (ret != NO_ERROR) {
    	db_error("setVideoEncodingBitRate Failed(%d)", ret);
   		return ret;
    }
/*************************************************************/
	
	ret = mHMR->setVideoSize(videoSize.width, videoSize.height);
	if (ret != NO_ERROR) {
    	db_error("setVideoSize Failed(%d)", ret);
   		return ret;
    }
	ret = mHMR->setVideoEncoder(HerbMediaRecorder::VideoEncoder::H264);
	if (ret != NO_ERROR) {
    	db_error("setVideoEncoder Failed(%d)", ret);
   		return ret;
    }
	//if (mCamId == CAM_CSI || mCamId == CAM_CSI_NET) {
		ret = mHMR->setAudioEncoder(HerbMediaRecorder::AudioEncoder::AAC);
		if (ret != NO_ERROR) {
			return ret;
		}
	//}
#ifdef PLATFORM_0
	if (mCamId >= CAM_CNT) {
		mHMR->setSourceChannel(1); //small pic
	}
#endif

	setDuration();
	if (mCamId < CAM_CNT) {	//there is no need to prepare 5s cache when app gets stream
		db_msg("cache time:%d", mCacheTime);
		ret = mHMR->setImpactFileDuration(mCacheTime, AFTIMEMS);
		if (ret != NO_ERROR) {
			db_error("setImpactFileDuration Failed(%d)", ret);
			return ret;
		}
	}

	ret = mHMR->enableDynamicBitRateControl(true);
	if (ret != NO_ERROR) {
		db_error("enableDynamicBitRateControl Failed(%d)", ret);
		return ret;
	}

	ret = mHMR->prepare();
	if (ret != NO_ERROR) {
    	db_error("prepare Failed(%d)", ret);
   		return ret;
    }

	mCurrentState = RECORDER_PREPARED;
	if (mCamId >= CAM_CNT) {
		__parseHeadData();
	}
	
	return NO_ERROR;
}

status_t CdrMediaRecorder::start(void)
{
	status_t ret;
	mCurrentState = RECORDER_RECORDING;
	setSilent(mSilent);
	ret = mHMR->start();
	return ret;
}

int CdrMediaRecorder::stop(void)
{
	mCurrentState = RECORDER_IDLE;
	return mHMR->stop();
}

void CdrMediaRecorder::release()
{
	mHMR->release();
	mCurrentState = RECORDER_NOT_INITIAL;
}

void CdrMediaRecorder::setDuration()
{
	mHMR->setMaxDuration(mDuration);
}

void CdrMediaRecorder::setDuration(int ms)
{
	Mutex::Autolock _l(mLock);
	mDuration = ms;
	setDuration();
}

void CdrMediaRecorder::setNextFile(String8 &file, unsigned int filesize, int nMuxerId)
{
    if(mMuxerId != nMuxerId)
    {
        db_error("fatal error! setNextFile muxerId[%d]!=[%d]", mMuxerId, nMuxerId);
    }
	mHMR->setOutputFileSync((char*)file.string(), filesize, nMuxerId);
	//close(fd);
}

void CdrMediaRecorder::setFile(int fd, unsigned int filesize)
{
	mHMR->setOutputFile(fd, filesize);
	close(fd);
}

void CdrMediaRecorder::setSilent(bool mode)
{
	db_debug("setMuteMode %d", mode);
	Mutex::Autolock _l(mLock);
	mSilent = mode;
	if (mHMR) {
		db_msg("silent %d", mode);
		mHMR->setMuteMode(mode);
	}
}

bool CdrMediaRecorder::getVoiceStatus()
{
	Mutex::Autolock _l(mLock);
	return mSilent;
}

int CdrMediaRecorder::getCameraID(void)
{
	return mCamId;
}

CdrRecorderState_t CdrMediaRecorder::getRecorderState(void)
{
	return mCurrentState;
}

#if 0
int CdrMediaRecorder::startExtraFile(int fd)
{
	mHMR->setExtraFileFd(fd);
	mHMR->startExtraFile();
	close(fd);
	return 0;
}

int CdrMediaRecorder::stopExtraFile()
{
	mHMR->stopExtraFile();
	return 0;
}
#endif

int CdrMediaRecorder::startImpactVideo(String8 &file, unsigned long size)
{
    Mutex::Autolock _l(mLock);
    //mImpactFd = fd;
    mStartImpactFile = true;
    mHMR->setImpactOutputFile((char*)file.string(), size, mMuxerId);
    //close(fd);
    //db_debug("start impact video mImpactFd:%d***\n",mImpactFd);

    return 0;
}

void CdrMediaRecorder::stopImpactVideo()
{
	Mutex::Autolock _l(mLock);
	mStartImpactFile = false;
//    if(mImpactFd>=0)
//    {
//	    close(mImpactFd);
//        mImpactFd = -1;
//    }
}

bool CdrMediaRecorder::getImpactStatus()
{
	return mStartImpactFile;
}

void CdrMediaRecorder::setSdcardState(bool bExist)
{
	mHMR->setSdcardState(bExist);
}

void CdrMediaRecorder::setCacheTime(int cacheTime)
{
	if (cacheTime < 0) {
		cacheTime = BFTIMEMS;
	}
	mCacheTime = cacheTime;
}

void CdrMediaRecorder::reset()
{
	mHMR->reset();
}

