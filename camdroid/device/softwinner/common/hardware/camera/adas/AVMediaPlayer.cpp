#define LOG_NDEBUG 0
#define LOG_TAG "AVMediaPlayer"
#include <cutils/log.h>
#include "AVMediaPlayer.h"

AVMediaPlayer::AVMediaPlayer()
{
	ALOGD("<***AVMediaPlayer::AVMediaPlayer()***>");
    mAVMediaListener  = NULL;
	mCedarMediaPlayer = NULL;
	mediaPlayerStatus = IDLE;
	mTargetStatus     = IDLE;
	avMediaPlayerInit();
}

AVMediaPlayer::~AVMediaPlayer()
{
	avMediaPlayerDestroy();
}

void AVMediaPlayer::avMediaPlayerInit() {
	ALOGD("<***AVMediaPlayer::avMediaPlayerInit()***>");
	if(mCedarMediaPlayer != NULL){
	    ALOGV("avMediaPlayerInit mCedarMediaPlayer != NULL,init false \n");
	    return;
	}
    mCedarMediaPlayer = new CedarMediaPlayer();	
	mCedarMediaPlayer->setOnPreparedListener(this);
	mCedarMediaPlayer->setOnCompletionListener(this);
	mCedarMediaPlayer->setOnErrorListener(this);
	mCedarMediaPlayer->setOnInfoListener(this);
	ipc_mutex_init(&mMPStatusMutex);
	ipc_mutex_init(&mMPListenMutex);
	//StorageManager *sm = StorageManager::getInstance();
	//sm->regStorageStatusListener(this);
	mediaPlayerStatus = IDLE; 
	ALOGD("<***endend***>");
}

void AVMediaPlayer::avMediaPlayerDestroy()
{
    delete mCedarMediaPlayer;
	ipc_mutex_deinit(&mMPStatusMutex);
	ipc_mutex_deinit(&mMPListenMutex);
	//StorageManager *sm = StorageManager::getInstance();
	//sm->unRegStorageStatusListener(this);	
	//(StorageManager::getInstance())->unRegStorageStatusListener(this);
}

void AVMediaPlayer::notify(int what, int status, char *msg)
{
	/*switch(status){
		case STORAGE_STATUS_UNMOUNTING:
		case STORAGE_STATUS_NO_MEDIA:
		{						
			stopPlay();
			memset(mName,0,sizeof(mName));
			break;
		}
		case STORAGE_STATUS_CHECKING:
		case STORAGE_STATUS_MOUNTED:
		{							
			break;
		}	
	}*/
}

void AVMediaPlayer::registerAVMediaListener(AVMediaListener *listener)
{
    ipc_mutex_lock(&mMPListenMutex);
	mAVMediaListener = listener;
	ipc_mutex_unlock(&mMPListenMutex);
}

void AVMediaPlayer::unRegisterAVMediaListener(AVMediaListener *listener)
{
    ipc_mutex_lock(&mMPListenMutex);
	mAVMediaListener = NULL;
	ipc_mutex_unlock(&mMPListenMutex);
}

void AVMediaPlayer::onPrepared(CedarMediaPlayer *pMp)
{
	ALOGD("AVMediaPlayer::onPrepared start mTargetStatus=%d",mTargetStatus);
/*
	//ipc_mutex_lock(&mMPStatusMutex);
	if(mTargetStatus == STOPPED){
		mCedarMediaPlayer->stop();
		mCedarMediaPlayer->reset();
		mediaPlayerStatus = IDLE;
		if(mAVMediaListener != NULL){
			mAVMediaListener->notify(pMp, ON_PREPARED_LISTENER,STARTED,mName);
		}
	}else{
		mCedarMediaPlayer->start();
		mediaPlayerStatus = STARTED;	
	}	
	//ipc_mutex_unlock(&mMPStatusMutex);
*/
}

void AVMediaPlayer::onCompletion(CedarMediaPlayer *pMp)
{    
	ipc_mutex_lock(&mMPStatusMutex);	
	ALOGV("**********AVMediaPlayer::onCompletion*********");
	//mCedarMediaPlayer->release();
	mediaPlayerStatus = COMPLETED;
	ipc_mutex_unlock(&mMPStatusMutex);
	ipc_mutex_lock(&mMPListenMutex);
	mCedarMediaPlayer->reset();
	mediaPlayerStatus = IDLE;
	memset(mName,0,sizeof(mName));
	if(mAVMediaListener != NULL){
	    mAVMediaListener->notify(pMp, ON_COMPLETION_LISTENER, COMPLETED, mName);
	}	
	ipc_mutex_unlock(&mMPListenMutex);
	//const char *path = "/test";//????????????????????????????????
	//reStart(path);	
}

bool AVMediaPlayer::onError(CedarMediaPlayer *pMp, int what, int extra)
{
    
	ipc_mutex_lock(&mMPListenMutex);	 
	if(mAVMediaListener != NULL){
	    mAVMediaListener->notify(pMp, ON_ERROR_LISTENER, ERRO, NULL);
	}
	ipc_mutex_unlock(&mMPListenMutex);
	return true;
}
bool AVMediaPlayer::onInfo(CedarMediaPlayer *pMp, int what, int extra)
{
    ipc_mutex_lock(&mMPListenMutex);
	if(mAVMediaListener != NULL){
	    mAVMediaListener->notify(pMp, ON_INFO_LISTENER, 0, NULL);
	}  
	ipc_mutex_unlock(&mMPListenMutex);
    return true;
}

void AVMediaPlayer::reStart(const char *path)
{
    ipc_mutex_lock(&mMPStatusMutex);
    mCedarMediaPlayer->reset();	
	mCedarMediaPlayer->setDataSource(String8(path));
	mCedarMediaPlayer->setAudioStreamType(AUDIO_STREAM_MUSIC);
	mCedarMediaPlayer->prepareAsync();
	mediaPlayerStatus = PREPARING;
	ipc_mutex_unlock(&mMPStatusMutex);
}

int AVMediaPlayer::startPlay(const char *path,float val)
{
	//char path[255];	
	int ret = 0;
    char *name = NULL;
	ipc_mutex_lock(&mMPStatusMutex);
	mCedarMediaPlayer->reset();
	mediaPlayerStatus = IDLE;
	if(mediaPlayerStatus == IDLE && mCedarMediaPlayer){
		mCedarMediaPlayer->setVolume(val,val);
		ret = mCedarMediaPlayer->setDataSource(String8(path));
		if(ret < 0){
		    ALOGE("setDataSource fail ret=%d",ret);
		    mediaPlayerStatus = IDLE;
			ipc_mutex_unlock(&mMPStatusMutex);
		    return ret;
		}
		memset(mName,0,sizeof(mName));
		name = strrchr(path, '/');
        if(name && strlen(name) > 1){
            name = name + 1;
            strcpy(mName,name);	
        }	
		ALOGV("<****AVMediaPlayer::startPlay 777***>");
		mCedarMediaPlayer->setAudioStreamType(AUDIO_STREAM_MUSIC);
		ALOGV("<****AVMediaPlayer::startPlay 7888***>");
		if (mCedarMediaPlayer->prepare() != NO_ERROR) {
			ALOGD("mCedarMediaPlayer->prepare() return error");
		}
		ALOGV("<****AVMediaPlayer::startPlay 888***>");
		mCedarMediaPlayer->start();
		mediaPlayerStatus = STARTED;
		//ret = mCedarMediaPlayer->getDuration();
		ALOGV("startPlay  ,mediaPlayerStatus=%d,ret=%d",mediaPlayerStatus,ret);
		ipc_mutex_unlock(&mMPStatusMutex);
		return ret;

	}else{
	
	   ALOGE("startPlay fail ,mediaPlayerStatus=%d\n",mediaPlayerStatus);
	}
	mTargetStatus = PREPARING;
	ipc_mutex_unlock(&mMPStatusMutex);
	return -1;
}

int AVMediaPlayer::isPlaying()
{
	int ret;
	ipc_mutex_lock(&mMPStatusMutex);
	ret = mediaPlayerStatus == IDLE?0:1;
	ipc_mutex_unlock(&mMPStatusMutex);
	return ret;   
}

int AVMediaPlayer::stopPlay() 
{
	ALOGV("stopPlay mediaPlayerStatus=%d",mediaPlayerStatus);
	ipc_mutex_lock(&mMPStatusMutex);
	if(mediaPlayerStatus == STARTED || mediaPlayerStatus == PREPARING){
	    mCedarMediaPlayer->stop();
		mCedarMediaPlayer->reset();
		mediaPlayerStatus = IDLE;
		memset(mName,0,sizeof(mName));
	}else{
	    ALOGE("stopPlay fail ,mediaPlayerStatus=%d\n",mediaPlayerStatus);
	}	
    mTargetStatus = IDLE;	
	ipc_mutex_unlock(&mMPStatusMutex);
	return 0;
}
int AVMediaPlayer::getName(char *name)
{
	if(strlen(mName) > 0){
	    strcpy(name,mName);
		return 0;
	}
	return -1;   
}
