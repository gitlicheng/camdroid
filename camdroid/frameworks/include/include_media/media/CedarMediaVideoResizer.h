#ifndef __CEDAR_MEDIA_VIDEO_RESIZER_H_
#define __CEDAR_MEDIA_VIDEO_RESIZER_H_
 
#include <utils/KeyedVector.h>
#include <utils/String8.h>
#include <binder/Parcel.h>

#include <MediaCallbackDispatcher.h>
#include <media/mediavideoresizer.h>

namespace android
{

class CedarMediaVideoResizer
{
public:
    CedarMediaVideoResizer();
    ~CedarMediaVideoResizer();
    status_t    setDataSource(const char *url);
    status_t    setDataSource(int fd, int64_t offset, int64_t length);
    status_t    setVideoSize(int32_t width, int32_t height);
    status_t    setOutputPath(const char *url);
    status_t    setFrameRate(int32_t framerate);
    status_t    setBitRate(int32_t bitrate);
    status_t    prepare();
    status_t    start();
    status_t    stop();
    status_t    seekTo(int msec);
    status_t    reset();
    status_t    release();
    int         getDuration();
    sp<IMemory> getEncDataHeader();

    /* Interface definition for a callback to be invoked when an event occurs while resizing. */
    class OnInfoListener
    {
    public:
		OnInfoListener() {}
		virtual ~OnInfoListener() {}
        /* Called when an event occurs while resizing. */
        virtual void onInfo(CedarMediaVideoResizer *pMr, int what, int extra) = 0;
    };
    /* Register a callback to be invoked when an event occurs while resizing. */
    void setOnInfoListener(OnInfoListener *pListener);

    /* Interface definition for a callback to be invoked when an error occurs while resizing. */
    class OnErrorListener
    {
    public:
		OnErrorListener() {}
		virtual ~OnErrorListener() {}
        /* Called when an error occurs while resizing. */
        virtual void onError(CedarMediaVideoResizer *pMr, int what, int extra) = 0;
    };
    /* Register a callback to be invoked when an error occurs while resizing. */
    void setOnErrorListener(OnErrorListener *pl);

    class OnCompletionListener
    {
    public:
        /**
         * Called when the end of a media source is reached during playback.
         *
         * @param pMr the CedarMediaVideoResizer that reached the end of the file
         */
		OnCompletionListener(){}
		virtual ~OnCompletionListener(){}         
        virtual void onCompletion(CedarMediaVideoResizer *pMr)=0;
    };
    /**
     * Register a callback to be invoked when the end of a media source
     * has been reached during playback.
     *
     * @param pListener the callback that will be run
     */
    void setOnCompletionListener(OnCompletionListener *pListener);

    class OnSeekCompleteListener
    {
    public:
		OnSeekCompleteListener(){}
		virtual ~OnSeekCompleteListener(){}
        /**
         * Called to indicate the completion of a seek operation.
         *
         * @param pMr the MediaPlayer that issued the seek operation
         */
        virtual void onSeekComplete(CedarMediaVideoResizer *pMr) = 0;
    };
    /**
     * Register a callback to be invoked when a seek operation has been
     * completed.
     *
     * @param pListener the callback that will be run
     */
    void setOnSeekCompleteListener(OnSeekCompleteListener *pListener);
    
    class PacketBuffer : public virtual RefBase
    {
    public:
        PacketBuffer(const sp<IMemory>& rawFrame);
        ~PacketBuffer(){}
        MediaVideoResizerStreamType mStreamType; /* 0:video, 1:audio data, 128:video data header, 129:audio data header */
        int         mDataSize; //the length of *mpData.
        long long   mPts;
        char*       mpData;
    private:
        //videoOutBs struct: total_size, RawPacketHeader, char[],
        sp<IMemory> mRawFrame;
    };
    //getPacket() result:
    //NO_ERROR: operation success. maybe get packet null.
    //INVALID_OPERATION: call in wrong status.
    //NOT_ENOUGH_DATA: data end.
    status_t getPacket(sp<PacketBuffer> &pktBuf);
private:
    class EventHandler : public MediaCallbackDispatcher
    {
    private:
        CedarMediaVideoResizer *mCedarMR;
    public:
        EventHandler(CedarMediaVideoResizer *CedarMR)
            : mCedarMR(CedarMR)
        {
        }
        ~EventHandler()
        {
        }
        virtual void handleMessage(const MediaCallbackMessage &msg);
    };

    class CedarMediaVideoResizerListener: public MediaVideoResizerListener
    {
    private:
        sp<EventHandler> mEventHandler;
    public:
        CedarMediaVideoResizerListener(sp<EventHandler> handler)
            : mEventHandler(handler)
        {
        }
        ~CedarMediaVideoResizerListener()
        {
        }
        virtual void notify(int msg, int ext1, int ext2);
    };

private:
    sp<EventHandler> mEventHandler;
    sp<MediaVideoResizer> mVideoResizer;
    Mutex mLock;
    OnErrorListener *mOnErrorListener;
    OnInfoListener  *mOnInfoListener;
    OnCompletionListener    *mOnCompletionListener;
    OnSeekCompleteListener  *mOnSeekCompleteListener;
};

}; /* namespace android */

#endif /* __CEDAR_MEDIA_VIDEO_RESIZER_H_ */

