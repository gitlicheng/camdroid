#ifndef _AVMEDIA_PLAYER_H
#define _AVMEDIA_PLAYER_H
#include <utils/RefBase.h>
#include <utils/Thread.h> 
#include <CedarMediaPlayer.h>
#include "SemaphoreMutex.h"

using namespace android;
enum AVMediaListener_notify_what{
	ON_PREPARED_LISTENER        = 0,  
	ON_COMPLETION_LISTENER      = 1,  
	ON_ERROR_LISTENER           = 2,
	ON_INFO_LISTENER            = 3,
};

enum AVMediaListener_status{
	IDLE                        = 0,  
	PREPARING                   = 1,
	PREPARED                    = 2,
	STARTED						= 3,
	PAUSED						= 4,
	STOPPED						= 5,
	COMPLETED			        = 6,
	ERRO                        = 7,
};


class AVMediaListener
{
public:
	AVMediaListener(){};
	virtual ~AVMediaListener(){};
    virtual void notify(void *mp, int what, int arg1, char *rsz) = 0;
};	

class AVMediaPlayer :public android::RefBase
				,CedarMediaPlayer::OnPreparedListener
				,CedarMediaPlayer::OnCompletionListener
				,CedarMediaPlayer::OnErrorListener
				,CedarMediaPlayer::OnInfoListener{
public :
    AVMediaPlayer();
	~AVMediaPlayer();
		
	void avMediaPlayerInit();

	void avMediaPlayerDestroy();

	void registerAVMediaListener(AVMediaListener *listener);
	void unRegisterAVMediaListener(AVMediaListener *listener);
	int stopPlay() ;
	int startPlay(const char *path,float val=0.6);
	int isPlaying();
	int getName(char *name);
	void notify(int what, int status, char *msg);
private:	
    AVMediaListener       *mAVMediaListener;
	CedarMediaPlayer      *mCedarMediaPlayer;
	int                    mediaPlayerStatus;
	int                    mTargetStatus;
	
	//MediaPlayerThread     *mMediaPlayerThread;
	//ipc_sem_t		      mmMediaPlayerSem;	
	ipc_mutex_t		      mMPStatusMutex;
	ipc_mutex_t		      mMPListenMutex;
	
	void reStart(const char *name);
	void onPrepared(CedarMediaPlayer *pMr);
	void onCompletion(CedarMediaPlayer *pMp);
	bool onError(CedarMediaPlayer *pMp, int what, int extra);
	bool onInfo(CedarMediaPlayer *pMp, int what, int extra);	
    char mName[125];
};

#endif	//_MEDIA_PLAYER_H