/*
**
** Copyright (C) 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

//#define LOG_NDEBUG 0
#define LOG_TAG "Camera"
#include <utils/Log.h>
#include <utils/threads.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/IMemory.h>

#include <camera/Camera.h>
#include <camera/ICameraRecordingProxyListener.h>
#include <camera/ICameraService.h>
#include <type_camera.h>

//#include <gui/ISurfaceTexture.h>
//#include <gui/Surface.h>

#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>

namespace android {

// client singleton for camera service binder interface
Mutex Camera::mLock;
sp<ICameraService> Camera::mCameraService;
sp<Camera::DeathNotifier> Camera::mDeathNotifier;


// establish binder interface to camera service
const sp<ICameraService>& Camera::getCameraService()
{
    Mutex::Autolock _l(mLock);
    if (mCameraService.get() == 0) {
        sp<IServiceManager> sm = defaultServiceManager();
        sp<IBinder> binder;
        do {
            binder = sm->getService(String16("media.camera"));
            if (binder != 0)
                break;
            ALOGW("CameraService not published, waiting...");
            usleep(500000); // 0.5 s
        } while(true);
        if (mDeathNotifier == NULL) {
            mDeathNotifier = new DeathNotifier();
        }
        binder->linkToDeath(mDeathNotifier);
        mCameraService = interface_cast<ICameraService>(binder);
    }
    ALOGE_IF(mCameraService==0, "no CameraService!?");
    return mCameraService;
}

// ---------------------------------------------------------------------------

Camera::Camera()
{
    init();
}

// construct a camera client from an existing camera remote
sp<Camera> Camera::create(const sp<ICamera>& camera)
{
     ALOGV("create");
     if (camera == 0) {
         ALOGE("camera remote is a NULL pointer");
         return 0;
     }

    sp<Camera> c = new Camera();
    if (camera->connect(c) == NO_ERROR) {
        c->mStatus = NO_ERROR;
        c->mCamera = camera;
        camera->asBinder()->linkToDeath(c);
        return c;
    }
    return 0;
}

void Camera::init()
{
    mStatus = UNKNOWN_ERROR;
//    int i;
//    for(i=0;i<10;i++)
//    {
//        mReleaseBufCnt[i] = 0;
//    }
    mMaxLeftFrameNum = 0;
}

Camera::~Camera()
{
    // We don't need to call disconnect() here because if the CameraService
    // thinks we are the owner of the hardware, it will hold a (strong)
    // reference to us, and we can't possibly be here. We also don't want to
    // call disconnect() here if we are in the same process as mediaserver,
    // because we may be invoked by CameraService::Client::connect() and will
    // deadlock if we call any method of ICamera here.
}

int32_t Camera::getNumberOfCameras()
{
    const sp<ICameraService>& cs = getCameraService();
    if (cs == 0) return 0;
    return cs->getNumberOfCameras();
}

status_t Camera::getCameraInfo(int cameraId,
                               struct CameraInfo* cameraInfo) {
    const sp<ICameraService>& cs = getCameraService();
    if (cs == 0) return UNKNOWN_ERROR;
    return cs->getCameraInfo(cameraId, cameraInfo);
}

sp<Camera> Camera::connect(int cameraId)
{
    ALOGV("connect");
    sp<Camera> c = new Camera();
    const sp<ICameraService>& cs = getCameraService();
    if (cs != 0) {
        c->mCamera = cs->connect(c, cameraId);
    }
    if (c->mCamera != 0) {
        c->mCamera->asBinder()->linkToDeath(c);
        c->mStatus = NO_ERROR;
    } else {
        c.clear();
    }
    return c;
}

void Camera::disconnect()
{
    ALOGV("disconnect");
    if (mCamera != 0) {
        mCamera->disconnect();
        mCamera->asBinder()->unlinkToDeath(this);
        mCamera = 0;
    }
}

status_t Camera::reconnect()
{
    ALOGV("reconnect");
    sp <ICamera> c = mCamera;
    if (c == 0) return NO_INIT;
    return c->connect(this);
}

sp<ICamera> Camera::remote()
{
    return mCamera;
}

status_t Camera::lock()
{
    sp <ICamera> c = mCamera;
    if (c == 0) return NO_INIT;
    return c->lock();
}

status_t Camera::unlock()
{
    sp <ICamera> c = mCamera;
    if (c == 0) return NO_INIT;
    return c->unlock();
}

// pass the buffered Surface to the camera service
//status_t Camera::setPreviewDisplay(const sp<Surface>& surface)
status_t Camera::setPreviewDisplay(const int hlay)
{
    ALOGV("setPreviewDisplay(%d)", hlay);
    sp <ICamera> c = mCamera;
    if (c == 0) return NO_INIT;
    if (hlay >= 0) {
        return c->setPreviewDisplay(hlay);
    } else {
        ALOGD("app passed NULL surface");
        return c->setPreviewDisplay(hlay);
    }
}

// pass the buffered ISurfaceTexture to the camera service
//status_t Camera::setPreviewTexture(const sp<ISurfaceTexture>& surfaceTexture)
status_t Camera::setPreviewTexture(const int hlay)
{
    ALOGV("setPreviewTexture(%d)", hlay);
    sp <ICamera> c = mCamera;
    if (c == 0) return NO_INIT;
    if (hlay >= 0) {
        return c->setPreviewTexture(hlay);
    } else {
        ALOGD("app passed NULL surface");
        return c->setPreviewTexture(hlay);
    }
}

// start preview mode
status_t Camera::startPreview()
{
    ALOGV("startPreview");
    sp <ICamera> c = mCamera;
    if (c == 0) return NO_INIT;
    return c->startPreview();
}

status_t Camera::storeMetaDataInBuffers(bool enabled)
{
    ALOGV("storeMetaDataInBuffers: %s",
            enabled? "true": "false");
    sp <ICamera> c = mCamera;
    if (c == 0) return NO_INIT;
    return c->storeMetaDataInBuffers(enabled);
}

// start recording mode, must call setPreviewDisplay first
status_t Camera::startRecording()
{
    ALOGV("startRecording");
    sp <ICamera> c = mCamera;
    if (c == 0) return NO_INIT;
    return c->startRecording();
}

// stop preview mode
void Camera::stopPreview()
{
    ALOGV("stopPreview");
    sp <ICamera> c = mCamera;
    if (c == 0) return;
    c->stopPreview();
}

// stop recording mode
void Camera::stopRecording(unsigned int recorderId)
{
    ALOGV("stopRecording");
	{
		Mutex::Autolock _l(mLock);
		for (size_t i = 0; i < mRecordingListener.size(); i++) {
			RecorderListener &listener = mRecordingListener.editItemAt(i);
			if (listener.recorderId == recorderId) {
				listener.listener.clear();
				mRecordingListener.removeAt(i);
				break;
			}
		}
		if (mRecordingListener.size() > 0) {
			return;
		}
    }

    sp <ICamera> c = mCamera;
    if (c == 0) return;
    c->stopRecording();
}

// release a recording frame
void Camera::releaseRecordingFrame(const sp<IMemory>& mem, int bufIdx)
{
    ALOGV("releaseRecordingFrame");
    sp <ICamera> c = mCamera;
    if (c == 0) return;
	//if (bufIdx < 0 || bufIdx > 6) {
	//	ALOGE("releaseRecordingFrame: fatal error, buf index not right, index=%d!!", bufIdx\);
	//	return;
	//}
    Mutex::Autolock _frameLock(mFrameOpLock);
//    if(mReleaseBufCnt[index] > 0)
//    {
//	    mReleaseBufCnt[index]--;
//    }
    ssize_t tmpIndex = mFrameOpCounter.indexOfKey(bufIdx);
    if(tmpIndex < 0)
    {
        ALOGE("(f:%s, l:%d) fatal error! not release frame! not find index[%d]", __FUNCTION__, __LINE__, bufIdx);
    }
    else
    {
        if(mFrameOpCounter.valueFor(bufIdx) <= 0)
        {
            ALOGD("(f:%s, l:%d) maybe no recorder to send frame[%d], count[%d], release immediately", __FUNCTION__, __LINE__, bufIdx, mFrameOpCounter.valueFor(bufIdx));
        }
        else
        {
            mFrameOpCounter.editValueFor(bufIdx)--;
        }
    }
//	if (mReleaseBufCnt[index] > 0) {
//		return;
//	}
    if(mFrameOpCounter.valueFor(bufIdx) > 0)
    {
        return;
    }
    int value = mFrameOpCounter.valueFor(bufIdx);
    if(value != 0)
    {
        ALOGE("(f:%s, l:%d) fatal error! not release frame! index[%d], value[%d]!=0", __FUNCTION__, __LINE__, bufIdx, value);
    }
    mFrameOpCounter.removeItem(bufIdx);
    int tmpNum = mFrameOpCounter.size();
    if(mMaxLeftFrameNum < tmpNum)
    {
        ALOGD("(f:%s, l:%d) not release frame! now left [%d] frames, old[%d]", __FUNCTION__, __LINE__, tmpNum, mMaxLeftFrameNum);
        mMaxLeftFrameNum = tmpNum;
    }
    //if(mFrameOpCounter.size() == 0)
    //{
    //    ALOGD("releaseRecordingFrame, frameSize=%d, frameIndex[%d]", mFrameOpCounter.size(), index);
    //}
    //ALOGD("releaseRecordingFrame: index=%d", index);
    c->releaseRecordingFrame(mem);
}

// get preview state
bool Camera::previewEnabled()
{
    ALOGV("previewEnabled");
    sp <ICamera> c = mCamera;
    if (c == 0) return false;
    return c->previewEnabled();
}

// get recording state
bool Camera::recordingEnabled()
{
    ALOGV("recordingEnabled");
    sp <ICamera> c = mCamera;
    if (c == 0) return false;
    return c->recordingEnabled();
}

status_t Camera::autoFocus()
{
    ALOGV("autoFocus");
    sp <ICamera> c = mCamera;
    if (c == 0) return NO_INIT;
    return c->autoFocus();
}

status_t Camera::cancelAutoFocus()
{
    ALOGV("cancelAutoFocus");
    sp <ICamera> c = mCamera;
    if (c == 0) return NO_INIT;
    return c->cancelAutoFocus();
}

// take a picture
status_t Camera::takePicture(int msgType)
{
    ALOGV("takePicture: 0x%x", msgType);
    sp <ICamera> c = mCamera;
    if (c == 0) return NO_INIT;
    return c->takePicture(msgType);
}

// set preview/capture parameters - key/value pairs
status_t Camera::setParameters(const String8& params)
{
    ALOGV("setParameters");
    sp <ICamera> c = mCamera;
    if (c == 0) return NO_INIT;
    return c->setParameters(params);
}

// get preview/capture parameters - key/value pairs
String8 Camera::getParameters() const
{
    ALOGV("getParameters");
    String8 params;
    sp <ICamera> c = mCamera;
    if (c != 0) params = mCamera->getParameters();
    return params;
}

status_t Camera::getV4l2QueryCtrl(struct v4l2_queryctrl *ctrl, int id)
{
    ALOGV("getV4l2QueryCtrl");
    sp <ICamera> c = mCamera;
    if (c == 0) return NO_INIT;
    return c->getV4l2QueryCtrl(ctrl, id);
}

status_t Camera::getInputSource(int *source)
{
    ALOGV("getInputSource");
    sp <ICamera> c = mCamera;
    if (c == 0) return NO_INIT;
    return c->getInputSource(source);
}

status_t Camera::getTVinSystem(int *system)
{
    ALOGV("getTVinSystem");
    sp <ICamera> c = mCamera;
    if (c == 0) return NO_INIT;
    return c->getTVinSystem(system);
}

status_t Camera::setWaterMark(int enable, const char *str)
{
    ALOGV("setWaterMark");
    sp <ICamera> c = mCamera;
    if (c == 0) return NO_INIT;
    return c->setWaterMark(enable, str);
}

status_t Camera::setUvcGadgetMode(int value)
{
    ALOGV("setUvcGadgetMode(%d)", value);
    sp <ICamera> c = mCamera;
    if (c == 0) return NO_INIT;
    return c->setUvcGadgetMode(value);
}

status_t Camera::adasSetGsensorData(int val0, float val1, float val2)
{
   // ALOGV("adasSetGsensorData(%d,%f,%f)", val0, val1, val2);
    sp <ICamera> c = mCamera;
    if (c == 0) return NO_INIT;
    return c->adasSetGsensorData(val0, val1, val2);
}


// send command to camera driver
status_t Camera::sendCommand(int32_t cmd, int32_t arg1, int32_t arg2)
{
    ALOGV("sendCommand");
    sp <ICamera> c = mCamera;
    if (c == 0) return NO_INIT;
    return c->sendCommand(cmd, arg1, arg2);
}

void Camera::setListener(const sp<CameraListener>& listener)
{
    Mutex::Autolock _l(mLock);
    mListener = listener;
}

void Camera::setRecordingProxyListener(const sp<ICameraRecordingProxyListener>& listener, unsigned int recorderId)
{
	RecorderListener recListener;
	recListener.recorderId = recorderId;
	recListener.listener = listener;
	{
	    Mutex::Autolock _l(mLock);
	    mRecordingListener.push(recListener);
	}
}

void Camera::setPreviewCallbackFlags(int flag)
{
    ALOGV("setPreviewCallbackFlags");
    sp <ICamera> c = mCamera;
    if (c == 0) return;
    mCamera->setPreviewCallbackFlag(flag);
}

// callback from camera service
void Camera::notifyCallback(int32_t msgType, int32_t ext1, int32_t ext2)
{
    sp<CameraListener> listener;
    {
        Mutex::Autolock _l(mLock);
        listener = mListener;
    }
    if (listener != NULL) {
        listener->notify(msgType, ext1, ext2);
    }
}

// callback from camera service when frame or image is ready
void Camera::dataCallback(int32_t msgType, const sp<IMemory>& dataPtr,
                          camera_frame_metadata_t *metadata)
{
    sp<CameraListener> listener;
    {
        Mutex::Autolock _l(mLock);
        listener = mListener;
    }
    if (listener != NULL) {
        listener->postData(msgType, dataPtr, metadata);
    }
}

// callback from camera service when timestamped frame is ready
void Camera::dataCallbackTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr, int bufIdx)
{
    // If recording proxy listener is registered, forward the frame and return.
    // The other listener (mListener) is ignored because the receiver needs to
    // call releaseRecordingFrame.

	sp<ICameraRecordingProxyListener> proxylistener;
	//if (bufIdx < 0 || bufIdx > NB_BUFFER) {
	//	ALOGE("dataCallbackTimestamp: fatal error, buf index not right, index = %d!!", bufIdx);
	//	return;
	//}
    bool bNeedReleaseFrame = false;
    {
    	Mutex::Autolock _frameLock(mFrameOpLock);
        //ALOGD("dataCallbackTimestamp: index=%d", pbuf->index);
//    	if(mReleaseBufCnt[pbuf->index] != 0)
//    	{
//            ALOGE("(f:%s, l:%d) fatal error! not release frame! mReleaseBufCnt[%d]=%d, != 0!", __FUNCTION__, __LINE__, pbuf->index, mReleaseBufCnt[pbuf->index]);
//            mReleaseBufCnt[pbuf->index] = 0;
//    	}
        ssize_t tmpIndex = mFrameOpCounter.indexOfKey(bufIdx);
        if(tmpIndex >= 0)
        {
            ALOGE("(f:%s, l:%d) fatal error! not release frame! index[%d] duplicate, count=%d!", __FUNCTION__, __LINE__, bufIdx, mFrameOpCounter.valueFor(bufIdx));
        }
        mFrameOpCounter.add(bufIdx, 0);
        //try to send frame to recorder.
        {
            Mutex::Autolock _l(mLock);
    		for (size_t i = 0; i < mRecordingListener.size(); i++) {
    			{
    				//Mutex::Autolock _l(mLock);
    				RecorderListener &recListener = mRecordingListener.editItemAt(i);
    				proxylistener = recListener.listener;
    			}
    			if (proxylistener != NULL) {
    				//mReleaseBufCnt[pbuf->index]++;
                    mFrameOpCounter.editValueFor(bufIdx)++;
    				proxylistener->dataCallbackTimestamp(timestamp, msgType, dataPtr);
    				continue;
    			}
    			sp<CameraListener> listener;
    			{
    				//Mutex::Autolock _l(mLock);
    				listener = mListener;
    			}
    			if (listener != NULL) {
    				//mReleaseBufCnt[pbuf->index]++;
                    mFrameOpCounter.editValueFor(bufIdx)++;
    				listener->postDataTimestamp(timestamp, msgType, dataPtr);
    			} /*else {
    				ALOGW("No listener was set. Drop a recording frame.");
    				releaseRecordingFrame(dataPtr);
    			}*/
    		}
        }
        if(0 == mFrameOpCounter.valueFor(bufIdx))
        {
//            if(mReleaseBufCnt[pbuf->index] != 0)
//            {
//                ALOGE("(f:%s, l:%d) fatal error! mReleaseBufCnt[%d] = %d, != 0!", __FUNCTION__, __LINE__, pbuf->index, mReleaseBufCnt[pbuf->index]);
//            }
            ALOGW("(f:%s, l:%d) No listener was set. Drop a recording frame[%d].", __FUNCTION__, __LINE__, bufIdx);
            bNeedReleaseFrame = true;
        }
        //ALOGD("dataCallbackTimestamp, send, frameSize=%d", mFrameOpCounter.size());
//        if(mFrameOpCounter.size() == 5)
//        {
//            for(int i=0;i<5;i++)
//            {
//                ALOGD("dataCallbackTimestamp, send, frameIndex=[%d, %d]", mFrameOpCounter.keyAt(i), mFrameOpCounter.valueAt(i));
//            }
//        }
    }
    //release frame.
    if(bNeedReleaseFrame)
    {
        sp<MemoryHeapBase>  frameIndexHeap;
        sp<MemoryBase>      frameIndexBuffer;
        frameIndexHeap = new MemoryHeapBase(2*sizeof(int));
        if(frameIndexHeap == NULL)
        {
            ALOGE("(f:%s, l:%d) fatal error! new MemoryHeapBase fail", __FUNCTION__, __LINE__);
        }
        if (frameIndexHeap->getHeapID() < 0)
        {
        	ALOGE("ERR(%s): Record heap creation fail", __func__);
            frameIndexHeap.clear();
        }
        frameIndexBuffer = new MemoryBase(frameIndexHeap, 0, 2*sizeof(int));
        if(frameIndexBuffer == NULL)
        {
            ALOGE("(f:%s, l:%d) fatal error! new MemoryHeapBase fail", __FUNCTION__, __LINE__);
        }
        int* p = (int*)(frameIndexBuffer->pointer());
        *p = bufIdx;
        *(p+1) = 0; //because no recorder now, so fill 0.
        releaseRecordingFrame(frameIndexBuffer, bufIdx);
    }
}

void Camera::binderDied(const wp<IBinder>& who) {
    ALOGW("ICamera died");
    notifyCallback(CAMERA_MSG_ERROR, CAMERA_ERROR_SERVER_DIED, 0);
}

void Camera::DeathNotifier::binderDied(const wp<IBinder>& who) {
    ALOGV("binderDied");
    Mutex::Autolock _l(Camera::mLock);
    Camera::mCameraService.clear();
    ALOGW("Camera server died!");
}

sp<ICameraRecordingProxy> Camera::getRecordingProxy() {
    ALOGV("getProxy");
    return new RecordingProxy(this);
}

status_t Camera::RecordingProxy::startRecording(const sp<ICameraRecordingProxyListener>& listener, unsigned int recorderId)
{
    ALOGV("RecordingProxy::startRecording");
    mCamera->setRecordingProxyListener(listener, recorderId);
    mCamera->reconnect();
    return mCamera->startRecording();
}

void Camera::RecordingProxy::stopRecording(unsigned int recorderId)
{
    ALOGV("RecordingProxy::stopRecording");
    mCamera->stopRecording(recorderId);
}

void Camera::RecordingProxy::releaseRecordingFrame(const sp<IMemory>& mem, int bufIdx)
{
    ALOGV("RecordingProxy::releaseRecordingFrame");
    mCamera->releaseRecordingFrame(mem, bufIdx);
}

status_t Camera::RecordingProxy::setPreviewDisplay(const unsigned int hlay)
{
    ALOGV("RecordingProxy::setPreviewDisplay");
    return mCamera->setPreviewDisplay(hlay);
}

status_t Camera::RecordingProxy::sendCommand(int32_t cmd, int32_t arg1, int32_t arg2)
{
    ALOGV("RecordingProxy::sendCommand");
	return mCamera->sendCommand(cmd, arg1, arg2);
}

status_t Camera::RecordingProxy::lock()
{
    ALOGV("RecordingProxy::lock");
	return mCamera->lock();
}

status_t Camera::RecordingProxy::unlock()
{
    ALOGV("RecordingProxy::unlock");
	return mCamera->unlock();
}

String8 Camera::RecordingProxy::getParameters() const
{
    ALOGV("RecordingProxy::getParameters");
	return mCamera->getParameters();
}

status_t Camera::RecordingProxy::setParameters(const String8& params)
{
    ALOGV("RecordingProxy::setParameters");
	return mCamera->setParameters(params);
}

Camera::RecordingProxy::RecordingProxy(const sp<Camera>& camera)
{
    mCamera = camera;
}

}; // namespace android
