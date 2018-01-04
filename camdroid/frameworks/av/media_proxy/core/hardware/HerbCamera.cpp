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
#define LOG_TAG "HerbCamera"
#include <utils/Log.h>

//#include "jni.h"
//#include "JNIHelp.h"
//#include "android_runtime/AndroidRuntime.h"

#include <cutils/properties.h>
#include <utils/Vector.h>

//#include <gui/SurfaceTexture.h>
//#include <gui/Surface.h>
#include <camera/Camera.h>
#include <binder/IMemory.h>

#include <HerbCamera.h>
namespace android {

CMediaMemory::CMediaMemory(int size)
{
    pMem = malloc(size);
    if(pMem == NULL)
    {
        mSize = 0;
        ALOGE("(f:%s, l:%d) malloc size[%d] fail", __FUNCTION__, __LINE__, size);
    }
    else
    {
        mSize = size;
    }
}
CMediaMemory::~CMediaMemory()
{
    if(pMem)
    {
        free(pMem);
    }
}
void* CMediaMemory::getPointer() const
{
    return pMem;
}
int CMediaMemory::getSize() const
{
    return mSize;
}

const String8 HerbCamera::TAG("HerbCamera");

static Mutex sLock;

// provides persistent context for calls from native code to Java
class HerbCameraContext: public CameraListener
{
public:
    HerbCameraContext(HerbCamera* pC, const sp<Camera>& camera);
    ~HerbCameraContext() { release(); }
    virtual void notify(int32_t msgType, int32_t ext1, int32_t ext2);
    virtual void postData(int32_t msgType, const sp<IMemory>& dataPtr,
                           camera_frame_metadata_t *metadata);
    virtual void postDataTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr);
    void postMetadata(int32_t msgType, camera_frame_metadata_t *metadata);
    //void addCallbackBuffer(JNIEnv *env, jbyteArray cbb, int msgType);
    void addCallbackBuffer(sp<CMediaMemory>& cbb, int msgType);
    
    void setCallbackMode(bool installed, bool manualMode);
    sp<Camera> getCamera() { Mutex::Autolock _l(mLock); return mCamera; }
    bool isRawImageCallbackBufferAvailable() const;
    void release();

private:
    void copyAndPost(const sp<IMemory>& dataPtr, int msgType);
    void clearCallbackBuffers_l(Vector<sp<CMediaMemory> > *buffers);
    void clearCallbackBuffers_l();
    //jbyteArray getCallbackBuffer(JNIEnv *env, Vector<jbyteArray> *buffers, size_t bufferSize);
    sp<CMediaMemory> getCallbackBuffer(Vector<sp<CMediaMemory> >* buffers, size_t bufferSize);

    //jobject     mCameraJObjectWeak;     // weak reference to java object
    //jclass      mCameraJClass;          // strong reference to java class
    //wp<HerbCamera> mpHerbCamera;
    HerbCamera* mpHerbCamera;
    sp<Camera>  mCamera;                // strong reference to native object
    //jclass      mFaceClass;  // strong reference to Face class
    //jclass      mRectClass;  // strong reference to Rect class
    Mutex       mLock;

    /*
     * Global reference application-managed raw image buffer queue.
     *
     * Manual-only mode is supported for raw image callbacks, which is
     * set whenever method addCallbackBuffer() with msgType =
     * CAMERA_MSG_RAW_IMAGE is called; otherwise, null is returned
     * with raw image callbacks.
     */
    //Vector<jbyteArray> mRawImageCallbackBuffers;
    Vector<sp<CMediaMemory> > mRawImageCallbackBuffers;

    /*
     * Application-managed preview buffer queue and the flags
     * associated with the usage of the preview buffer callback.
     */
    //Vector<jbyteArray> mCallbackBuffers; // Global reference application managed byte[]
    Vector<sp<CMediaMemory> > mCallbackBuffers; // Global reference application managed byte[]
    bool mManualBufferMode;              // Whether to use application managed buffers.
    bool mManualCameraCallbackSet;       // Whether the callback has been set, used to
                                         // reduce unnecessary calls to set the callback.
};
bool HerbCameraContext::isRawImageCallbackBufferAvailable() const
{
    return !mRawImageCallbackBuffers.isEmpty();
}

//JNICameraContext::JNICameraContext(JNIEnv* env, jobject weak_this, jclass clazz, const sp<Camera>& camera)
HerbCameraContext::HerbCameraContext(HerbCamera* pC, const sp<Camera>& camera)
{
    //mCameraJObjectWeak = env->NewGlobalRef(weak_this);
    //mCameraJClass = (jclass)env->NewGlobalRef(clazz);
    mCamera = camera;

    //jclass faceClazz = env->FindClass("android/hardware/Camera$Face");
    //mFaceClass = (jclass) env->NewGlobalRef(faceClazz);

    //jclass rectClazz = env->FindClass("android/graphics/Rect");
    //mRectClass = (jclass) env->NewGlobalRef(rectClazz);

    mManualBufferMode = false;
    mManualCameraCallbackSet = false;

    mpHerbCamera = pC;
}

void HerbCameraContext::release()
{
    ALOGV("release");
    Mutex::Autolock _l(mLock);
    //JNIEnv *env = AndroidRuntime::getJNIEnv();

//    if (mCameraJObjectWeak != NULL) {
//        env->DeleteGlobalRef(mCameraJObjectWeak);
//        mCameraJObjectWeak = NULL;
//    }
//    if (mCameraJClass != NULL) {
//        env->DeleteGlobalRef(mCameraJClass);
//        mCameraJClass = NULL;
//    }
//    if (mFaceClass != NULL) {
//        env->DeleteGlobalRef(mFaceClass);
//        mFaceClass = NULL;
//    }
//    if (mRectClass != NULL) {
//        env->DeleteGlobalRef(mRectClass);
//        mRectClass = NULL;
//    }
    clearCallbackBuffers_l();
    mCamera.clear();
}

//void JNICameraContext::notify(int32_t msgType, int32_t ext1, int32_t ext2)
void HerbCameraContext::notify(int32_t msgType, int32_t ext1, int32_t ext2)
{
    ALOGV("notify");

    // VM pointer will be NULL if object is released
    Mutex::Autolock _l(mLock);
//    if (mCameraJObjectWeak == NULL) {
//        ALOGW("callback on dead camera object");
//        return;
//    }
    if (mpHerbCamera == NULL) {
        ALOGW("callback on dead camera object");
        return;
    }
            
    /*
     * If the notification or msgType is CAMERA_MSG_RAW_IMAGE_NOTIFY, change it
     * to CAMERA_MSG_RAW_IMAGE since CAMERA_MSG_RAW_IMAGE_NOTIFY is not exposed
     * to the Java app.
     */
    if (msgType == CAMERA_MSG_RAW_IMAGE_NOTIFY) {
        msgType = CAMERA_MSG_RAW_IMAGE;
    }

    HerbCamera::postEventFromNative(mpHerbCamera, msgType, ext1, ext2);
}

//jbyteArray JNICameraContext::getCallbackBuffer(
//        JNIEnv* env, Vector<jbyteArray>* buffers, size_t bufferSize)
sp<CMediaMemory> HerbCameraContext::getCallbackBuffer(
        Vector<sp<CMediaMemory> >* buffers, size_t bufferSize)
{
    //jbyteArray obj = NULL;
    sp<CMediaMemory> obj = NULL;

    // Vector access should be protected by lock in postData()
    if (!buffers->isEmpty()) {
        ALOGV("Using callback buffer from queue of length %d", buffers->size());
        //jbyteArray globalBuffer = buffers->itemAt(0);
        sp<CMediaMemory> globalBuffer = buffers->itemAt(0);
        buffers->removeAt(0);

        //obj = (jbyteArray)env->NewLocalRef(globalBuffer);
        //env->DeleteGlobalRef(globalBuffer);
        obj = globalBuffer;

        if (obj != NULL) {
            //jsize bufferLength = env->GetArrayLength(obj);
            int bufferLength = obj->getSize();
            if ((int)bufferLength < (int)bufferSize) {
                ALOGE("Callback buffer was too small! Expected %d bytes, but got %d bytes!",
                    bufferSize, bufferLength);
                //env->DeleteLocalRef(obj);
                obj = NULL;
                return obj;
            }
        }
    }

    return obj;
}

void HerbCameraContext::copyAndPost(const sp<IMemory>& dataPtr, int msgType)
{
    //ALOGE("(f:%s, l:%d) only for test, for fun. Don't use it now", __FUNCTION__, __LINE__);
    sp<CMediaMemory> obj = NULL;

    if (dataPtr != NULL) {
        ssize_t offset;
        size_t size;
        sp<IMemoryHeap> heap = dataPtr->getMemory(&offset, &size);
        ALOGV("copyAndPost: off=%ld, size=%d", offset, size);
        uint8_t *heapBase = (uint8_t*)heap->base();

        if (heapBase != NULL) {
            const uint8_t* data = heapBase + offset;

            if (msgType == CAMERA_MSG_RAW_IMAGE) {
                obj = getCallbackBuffer(&mRawImageCallbackBuffers, size);
            } else if (msgType == CAMERA_MSG_PREVIEW_FRAME && mManualBufferMode) {
                obj = getCallbackBuffer(&mCallbackBuffers, size);

                if (mCallbackBuffers.isEmpty()) {
                    ALOGV("Out of buffers, clearing callback!");
                    mCamera->setPreviewCallbackFlags(CAMERA_FRAME_CALLBACK_FLAG_NOOP);
                    mManualCameraCallbackSet = false;

                    if (obj == NULL) {
                        return;
                    }
                }
            } else {
                ALOGV("Allocating callback buffer");
                obj = new CMediaMemory(size);
            }

            if (obj == NULL) {
                ALOGE("Couldn't allocate byte array for JPEG data");
            } else {
                memcpy(obj->getPointer(), data, size);
            }
        } else {
            ALOGE("image heap is NULL");
        }
    }

    HerbCamera::postEventFromNative(mpHerbCamera, msgType, 0, 0, &obj);
}

void HerbCameraContext::postData(int32_t msgType, const sp<IMemory>& dataPtr,
                                camera_frame_metadata_t *metadata)
{
    // VM pointer will be NULL if object is released
    Mutex::Autolock _l(mLock);
    if (mpHerbCamera == NULL) {
        ALOGW("callback on dead camera object");
        return;
    }

    int32_t dataMsgType = msgType & ~CAMERA_MSG_PREVIEW_METADATA;

    // return data based on callback type
    switch (dataMsgType) {
        case CAMERA_MSG_VIDEO_FRAME:
            // should never happen
            break;

        // For backward-compatibility purpose, if there is no callback
        // buffer for raw image, the callback returns null.
        case CAMERA_MSG_RAW_IMAGE:
            ALOGV("rawCallback");
            if (mRawImageCallbackBuffers.isEmpty()) {
//                env->CallStaticVoidMethod(mCameraJClass, fields.post_event,
//                        mCameraJObjectWeak, dataMsgType, 0, 0, NULL);
                HerbCamera::postEventFromNative(mpHerbCamera, msgType, 0, 0);
            } else {
                copyAndPost(dataPtr, dataMsgType);
            }
            break;

        // There is no data.
        case 0:
            break;

        case CAMERA_MSG_ADAS_METADATA:
        case CAMERA_MSG_COMPRESSED_IMAGE:
        case CAMERA_MSG_POSTVIEW_FRAME:
        case CAMERA_MSG_QRDECODE:
        {
            HerbCamera::postEventFromNative(mpHerbCamera, dataMsgType, 0, 0, (void*)&dataPtr);
            break;
        }

        default:
            ALOGV("dataCallback(%d, %p)", dataMsgType, dataPtr.get());
            copyAndPost(dataPtr, dataMsgType);
            break;
    }

    // post frame metadata to Java
    if (NULL != metadata) {
    	if (msgType & CAMERA_MSG_PREVIEW_METADATA) {
    	    postMetadata(CAMERA_MSG_PREVIEW_METADATA, (camera_frame_metadata_t *)metadata);
    	}
    }
}

//void JNICameraContext::postDataTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr)
void HerbCameraContext::postDataTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr)
{
    // TODO: plumb up to Java. For now, just drop the timestamp
    postData(msgType, dataPtr, NULL);
}

//void JNICameraContext::postMetadata(JNIEnv *env, int32_t msgType, camera_frame_metadata_t *metadata)
void HerbCameraContext::postMetadata(int32_t msgType, camera_frame_metadata_t *metadata)
{
    ALOGE("(f:%s, l:%d) only for test, for fun. Don't use it now", __FUNCTION__, __LINE__);
    //jobjectArray obj = NULL;
    int i;
    Vector<HerbCamera::Face> obj;
//    obj = (jobjectArray) env->NewObjectArray(metadata->number_of_faces,
//                                             mFaceClass, NULL);
    for(i=0; i<metadata->number_of_faces; i++)
    {
        obj.add(HerbCamera::Face());
    }
    if (obj.size() == 0) {
        ALOGE("Couldn't allocate face metadata array");
        return;
    }

    for (int i = 0; i < metadata->number_of_faces; i++) {
//        jobject face = env->NewObject(mFaceClass, fields.face_constructor);
//        env->SetObjectArrayElement(obj, i, face);
//
//        jobject rect = env->NewObject(mRectClass, fields.rect_constructor);
//        env->SetIntField(rect, fields.rect_left, metadata->faces[i].rect[0]);
//        env->SetIntField(rect, fields.rect_top, metadata->faces[i].rect[1]);
//        env->SetIntField(rect, fields.rect_right, metadata->faces[i].rect[2]);
//        env->SetIntField(rect, fields.rect_bottom, metadata->faces[i].rect[3]);
//
//        env->SetObjectField(face, fields.face_rect, rect);
//        env->SetIntField(face, fields.face_score, metadata->faces[i].score);
//
//        env->DeleteLocalRef(face);
//        env->DeleteLocalRef(rect);

        HerbCamera::Face face;
        face.rect.left = metadata->faces[i].rect[0];
        face.rect.top = metadata->faces[i].rect[1];
        face.rect.right = metadata->faces[i].rect[2];
        face.rect.bottom = metadata->faces[i].rect[3];
        face.score = metadata->faces[i].score;
        obj.add(face);
    }
//    env->CallStaticVoidMethod(mCameraJClass, fields.post_event,
//            mCameraJObjectWeak, msgType, 0, 0, obj);
//    env->DeleteLocalRef(obj);

    //warning: this is dangerous. Vector<HerbCamera::Face> can not send out.
    HerbCamera::postEventFromNative(mpHerbCamera, msgType, 0, 0, &obj);
}

void HerbCameraContext::setCallbackMode(bool installed, bool manualMode)
{
    Mutex::Autolock _l(mLock);
    mManualBufferMode = manualMode;
    mManualCameraCallbackSet = false;

    // In order to limit the over usage of binder threads, all non-manual buffer
    // callbacks use CAMERA_FRAME_CALLBACK_FLAG_BARCODE_SCANNER mode now.
    //
    // Continuous callbacks will have the callback re-registered from handleMessage.
    // Manual buffer mode will operate as fast as possible, relying on the finite supply
    // of buffers for throttling.

    if (!installed) {
        mCamera->setPreviewCallbackFlags(CAMERA_FRAME_CALLBACK_FLAG_NOOP);
        clearCallbackBuffers_l(&mCallbackBuffers);
    } else if (mManualBufferMode) {
        if (!mCallbackBuffers.isEmpty()) {
            mCamera->setPreviewCallbackFlags(CAMERA_FRAME_CALLBACK_FLAG_CAMERA);
            mManualCameraCallbackSet = true;
        }
    } else {
        mCamera->setPreviewCallbackFlags(CAMERA_FRAME_CALLBACK_FLAG_BARCODE_SCANNER);
        clearCallbackBuffers_l(&mCallbackBuffers);
    }
}

void HerbCameraContext::addCallbackBuffer(sp<CMediaMemory>& cbb, int msgType)
{
    ALOGV("addCallbackBuffer: 0x%x", msgType);
    if (cbb != NULL) {
        Mutex::Autolock _l(mLock);
        switch (msgType) {
            case CAMERA_MSG_PREVIEW_FRAME: {
                //jbyteArray callbackBuffer = (jbyteArray)env->NewGlobalRef(cbb);
                sp<CMediaMemory>& callbackBuffer = cbb;
                mCallbackBuffers.push(callbackBuffer);

                ALOGV("Adding callback buffer to queue, %d total",
                        mCallbackBuffers.size());

                // We want to make sure the camera knows we're ready for the
                // next frame. This may have come unset had we not had a
                // callbackbuffer ready for it last time.
                if (mManualBufferMode && !mManualCameraCallbackSet) {
                    mCamera->setPreviewCallbackFlags(CAMERA_FRAME_CALLBACK_FLAG_CAMERA);
                    mManualCameraCallbackSet = true;
                }
                break;
            }
            case CAMERA_MSG_RAW_IMAGE: {
                //jbyteArray callbackBuffer = (jbyteArray)env->NewGlobalRef(cbb);
                sp<CMediaMemory>& callbackBuffer = cbb;
                mRawImageCallbackBuffers.push(callbackBuffer);
                break;
            }
            default: {
//                jniThrowException(env,
//                        "java/lang/IllegalArgumentException",
//                        "Unsupported message type");
                ALOGE("(f:%s, l:%d) Unsupported message type", __FUNCTION__, __LINE__);
                return;
            }
        }
    } else {
       ALOGE("Null byte array!");
    }
}

//void JNICameraContext::clearCallbackBuffers_l(JNIEnv *env)
void HerbCameraContext::clearCallbackBuffers_l()
{
    clearCallbackBuffers_l(&mCallbackBuffers);
    clearCallbackBuffers_l(&mRawImageCallbackBuffers);
}

//void JNICameraContext::clearCallbackBuffers_l(JNIEnv *env, Vector<sp<CMediaMemory> > *buffers) 
void HerbCameraContext::clearCallbackBuffers_l(Vector<sp<CMediaMemory> > *buffers)
{
    ALOGV("Clearing callback buffers, %d remained", buffers->size());
    while (!buffers->isEmpty()) {
        //env->DeleteGlobalRef(buffers->top());
        buffers->pop();
    }
}

sp<Camera> HerbCamera::get_native_camera(HerbCamera* pC, HerbCameraContext** pContext)
{
    sp<Camera> camera;
    Mutex::Autolock _l(sLock);
    HerbCameraContext* context = reinterpret_cast<HerbCameraContext*>(pC->mNativeContext);
    if (context != NULL) {
        camera = context->getCamera();
    }
    ALOGV("get_native_camera: context=%p, camera=%p", context, camera.get());
    if (camera == 0) {
        ALOGE("(f:%s, l:%d) Method called after release() ", __FUNCTION__, __LINE__);
    }

    if (pContext != NULL) *pContext = context;
    return camera;
}

HerbCamera::EventHandler::EventHandler(HerbCamera *pC)
    : mpCamera(pC)
{
}

void HerbCamera::EventHandler::handleMessage(const MediaCallbackMessage &msg)
{
    switch(msg.what)
    {
        case CAMERA_MSG_SHUTTER:
        {
            if (mpCamera->mpShutterCallback != NULL) 
            {
                mpCamera->mpShutterCallback->onShutter();
            }
            return;
        }
        case CAMERA_MSG_RAW_IMAGE:
        {
            if (mpCamera->mpRawImageCallback != NULL) {
                ALOGE("(f:%s, l:%d) only for test, for fun. Don't use it now", __FUNCTION__, __LINE__);
                //mpCamera->mpRawImageCallback->onPictureTaken((byte[])msg.obj, mCamera);
                mpCamera->mpRawImageCallback->onPictureTaken(msg.obj, 0, mpCamera);
            }
            return;
        }

        case CAMERA_MSG_COMPRESSED_IMAGE:
        {
            if (mpCamera->mpJpegCallback != NULL) {
                sp<IMemory> dataPtr = msg.dataCompressedImage;
                uint8_t* data = NULL;
                ssize_t offset = 0;
                size_t size = 0;
                if (dataPtr != NULL) {
                    sp<IMemoryHeap> heap = dataPtr->getMemory(&offset, &size);
                    ALOGV("<IMemory>: off=%ld, size=%d", offset, size);
                    uint8_t *heapBase = (uint8_t*)heap->base();

                    if (heapBase != NULL) {
                        //const jbyte* data = reinterpret_cast<const jbyte*>(heapBase + offset);
                        data = heapBase + offset;
                    } else {
                        ALOGE("image heap is NULL");
                    }
                }
                //mJpegCallback.onPictureTaken((byte[])msg.obj, mCamera);
                mpCamera->mpJpegCallback->onPictureTaken((void*)data, size, mpCamera);
            }
            return;
        }

        case CAMERA_MSG_PREVIEW_FRAME:
        {
            PreviewCallback *pCb = mpCamera->mpPreviewCallback;
            if (pCb != NULL) {
                ALOGE("(f:%s, l:%d) only for test, for fun. Don't use it now", __FUNCTION__, __LINE__);
                if (mpCamera->mOneShot) {
                    // Clear the callback variable before the callback
                    // in case the app calls setPreviewCallback from
                    // the callback function
                    mpCamera->mpPreviewCallback = NULL;
                } else if (!mpCamera->mWithBuffer) {
                    // We're faking the camera preview mode to prevent
                    // the app from being flooded with preview frames.
                    // Set to oneshot mode again.
                    mpCamera->setHasPreviewCallback(true, false);
                }
                //pCb.onPreviewFrame((byte[])msg.obj, mCamera);
                pCb->onPreviewFrame(msg.obj, 0, mpCamera);
            }
            return;
        }
        
        case CAMERA_MSG_POSTVIEW_FRAME:
        {
            if (mpCamera->mpPostviewCallback != NULL) {
                sp<IMemory> dataPtr = msg.dataCompressedImage;
                uint8_t* data = NULL;
                ssize_t offset = 0;
                size_t size = 0;
                if (dataPtr != NULL) {
                    sp<IMemoryHeap> heap = dataPtr->getMemory(&offset, &size);
                    ALOGV("<IMemory>: off=%ld, size=%d", offset, size);
                    uint8_t *heapBase = (uint8_t*)heap->base();

                    if (heapBase != NULL) {
                        data = heapBase + offset;
                    } else {
                        ALOGE("image heap is NULL");
                    }
                }
                mpCamera->mpPostviewCallback->onPictureTaken((void*)data, size, mpCamera);
            }
            return;
        }

        case CAMERA_MSG_FOCUS:
        {
            AutoFocusCallback *pCb = NULL;
            {
                Mutex::Autolock _l(mpCamera->mAutoFocusCallbackLock);
                pCb = mpCamera->mpAutoFocusCallback;
            }
            if (pCb != NULL) {
                bool success = msg.arg1 == 0 ? false : true;
                //cb.onAutoFocus(success, mCamera);
                pCb->onAutoFocus(success, mpCamera);
            }
            return;
        }

        case CAMERA_MSG_ZOOM:
        {
            if (mpCamera->mpZoomListener != NULL) {
                mpCamera->mpZoomListener->onZoomChange(msg.arg1, msg.arg2 != 0, mpCamera);
            }
            return;
        }

        case CAMERA_MSG_PREVIEW_METADATA:
            if (mpCamera->mpFaceListener != NULL) {
                ALOGE("(f:%s, l:%d) only for test, for fun. Don't use it now", __FUNCTION__, __LINE__);
                Vector<Face >   faces;
                mpCamera->mpFaceListener->onFaceDetection(faces, mpCamera);
            }
            return;
		case CAMERA_MSG_ADAS_METADATA:
            if (mpCamera->mpAdasListener != NULL) { 
                sp<IMemory> dataPtr = msg.dataCompressedImage;
                uint8_t* data = NULL;
                ssize_t offset = 0;
                size_t size = 0;
                if (dataPtr != NULL) {
                    sp<IMemoryHeap> heap = dataPtr->getMemory(&offset, &size);
                    ALOGV("<IMemory>: off=%ld, size=%d", offset, size);
                    uint8_t *heapBase = (uint8_t*)heap->base();

                    if (heapBase != NULL) {
                        data = heapBase + offset;
                    } else {
                        ALOGE("image heap is NULL");
                    }
                }
                mpCamera->mpAdasListener->onAdasDetection((void*)data, size, mpCamera);
            }
			return;

        case CAMERA_MSG_QRDECODE:
            if (mpCamera->mpQrDecodeListener != NULL) {
                sp<IMemory> dataPtr = msg.dataCompressedImage;
                uint8_t* data = NULL;
                ssize_t offset = 0;
                size_t size = 0;
                if (dataPtr != NULL) {
                    sp<IMemoryHeap> heap = dataPtr->getMemory(&offset, &size);
                    ALOGV("<IMemory>: off=%ld, size=%d", offset, size);
                    uint8_t *heapBase = (uint8_t*)heap->base();

                    if (heapBase != NULL) {
                        data = heapBase + offset;
                    } else {
                        ALOGE("image heap is NULL");
                    }
                }
                mpCamera->mpQrDecodeListener->onQrDecode((void*)data, size, mpCamera);
            }
            return;
        case CAMERA_MSG_ERROR :
            ALOGE("(f:%s, l:%d) Error %d", __FUNCTION__, __LINE__, msg.arg1);
            if (mpCamera->mpErrorCallback != NULL) {
                mpCamera->mpErrorCallback->onError(msg.arg1, mpCamera);
            }
            return;

        case CAMERA_MSG_FOCUS_MOVE:
            if (mpCamera->mpAutoFocusMoveCallback != NULL) {
                mpCamera->mpAutoFocusMoveCallback->onAutoFocusMoving(msg.arg1 == 0 ? false : true, mpCamera);
            }
            return;

		case CAMERA_MSG_AWMD:	/* MOTION_DETECTION_ENABLE */
            if (mpCamera->mpAWMDListener != NULL) {
                mpCamera->mpAWMDListener->onAWMoveDetection(msg.arg1, mpCamera);
            }
			return;

#ifdef SUPPORT_TVIN
		case CAMERA_MSG_TVIN_SYSTEM_CHANGE:
			if (mpCamera->mpTVinListener != NULL) {
				mpCamera->mpTVinListener->onTVinSystemChange(msg.arg1, mpCamera);
			}
			return;
#endif

        default:
        {
            ALOGE("Unknown message type %d", msg.what);
            return;
        }
    }
}

int HerbCamera::getNumberOfCameras()
{
    return Camera::getNumberOfCameras();
}

void HerbCamera::getCameraInfo(int cameraId, CameraInfo *pCameraInfo)
{
    if(pCameraInfo == NULL)
    {
        ALOGE("(f:%s, l:%d) pCameraInfo == NULL ", __FUNCTION__, __LINE__);
        return;
    }
    android::CameraInfo cameraInfo;
    status_t rc = Camera::getCameraInfo(cameraId, &cameraInfo);
    if (rc != NO_ERROR) {
        //jniThrowRuntimeException(env, "Fail to get camera info");
        ALOGE("(f:%s, l:%d) Fail to get camera info", __FUNCTION__, __LINE__);
        return;
    }
    pCameraInfo->facing = cameraInfo.facing;
    pCameraInfo->orientation = cameraInfo.orientation;

    char value[PROPERTY_VALUE_MAX];
    property_get("ro.camera.sound.forced", value, "0");
    bool canDisableShutterSound = (strncmp(value, "0", 2) == 0);
    pCameraInfo->canDisableShutterSound = canDisableShutterSound;
}
    
HerbCamera* HerbCamera::open(int cameraId)
{
    HerbCamera *pHC = new HerbCamera(cameraId);
    sp<Camera> nativeCamera = pHC->get_native_camera(pHC, NULL);
    if(nativeCamera == NULL)
    {
        delete pHC;
        pHC = NULL;
    }
    return pHC;
}

HerbCamera::HerbCamera(int cameraId)
{
    mpShutterCallback = NULL;
    mpRawImageCallback = NULL;
    mpJpegCallback = NULL;
    mpPreviewCallback = NULL;
    mpPostviewCallback = NULL;
    mpZoomListener = NULL;

    mpAutoFocusCallback = NULL;
    mpAutoFocusMoveCallback = NULL;
    mpFaceListener = NULL;
    mpErrorCallback = NULL;

    mFaceDetectionRunning = false;
	mpAWMDListener = NULL;	/* MOTION_DETECTION_ENABLE */
	mAWMDrunning = false;	/* MOTION_DETECTION_ENABLE */
	/* for ADAS start */
	mpAdasListener = NULL;
	mAdasDetectionRunning = false;
	/* for ADAS end */
    mpQrDecodeListener = NULL;
#ifdef SUPPORT_TVIN
	mpTVinListener = NULL;
#endif
    mNativeContext = NULL;
    mpEventHandler = new EventHandler(this);
    
    sp<Camera> camera = Camera::connect(cameraId);
    if (camera == NULL) {
        //jniThrowRuntimeException(env, "Fail to connect to camera service");
        ALOGE("(f:%s, l:%d) Fail to connect to camera service", __FUNCTION__, __LINE__);
        return;
    }
    // make sure camera hardware is alive
    if (camera->getStatus() != NO_ERROR) {
        //jniThrowRuntimeException(env, "Camera initialization failed");
        ALOGE("(f:%s, l:%d) Camera initialization failed", __FUNCTION__, __LINE__);
        return;
    }

    // We use a weak reference so the Camera object can be garbage collected.
    // The reference is only used as a proxy for callbacks.
    sp<HerbCameraContext> context = new HerbCameraContext(this, camera);
    context->incStrong(this);
    camera->setListener(context);

    // save context in opaque field
    mNativeContext = (void*)context.get();
}

HerbCamera::~HerbCamera()
{
    ALOGD("(f:%s, l:%d)", __FUNCTION__, __LINE__);
    if(mpEventHandler)
    {
        delete mpEventHandler;
        mpEventHandler = NULL;
    }
}

// disconnect from camera service
// It's okay to call this when the native camera context is already null.
// This handles the case where the user has called release() and the
// finalizer is invoked later.
void HerbCamera::release()
{
    // TODO: Change to ALOGV
    ALOGV("release camera");
    HerbCameraContext* context = NULL;
    sp<Camera> camera;
    {
        Mutex::Autolock _l(sLock);
        context = reinterpret_cast<HerbCameraContext*>(mNativeContext);

        // Make sure we do not attempt to callback on a deleted Java object.
        mNativeContext = NULL;
    }

    // clean up if release has not been called before
    if (context != NULL) {
        camera = context->getCamera();
        context->release();
        ALOGV("native_release: context=%p camera=%p", context, camera.get());

        // clear callbacks
        if (camera != NULL) {
            camera->setPreviewCallbackFlags(CAMERA_FRAME_CALLBACK_FLAG_NOOP);
            camera->disconnect();
        }

        // remove context to prevent further Java access
        context->decStrong(this);
    }
    
    mFaceDetectionRunning = false;
}

status_t HerbCamera::unlock()
{
    ALOGV("unlock");
    sp<Camera> camera = get_native_camera(this, NULL);
    if (camera == 0) return INVALID_OPERATION;

	status_t ret = camera->unlock();
    if (ret != NO_ERROR) {
        //jniThrowRuntimeException(env, "unlock failed");
        ALOGE("(f:%s, l:%d) unlock failed", __FUNCTION__, __LINE__);
		return ret;
    }
	return NO_ERROR;
}

status_t HerbCamera::lock()
{
    ALOGV("lock");
    sp<Camera> camera = get_native_camera(this, NULL);
    if (camera == 0) return INVALID_OPERATION;

	status_t ret = camera->lock();
    if (ret != NO_ERROR) {
        //jniThrowRuntimeException(env, "lock failed");
        ALOGE("(f:%s, l:%d) lock failed", __FUNCTION__, __LINE__);
		return ret;
    }
	return NO_ERROR;
}

status_t HerbCamera::reconnect()
{
    ALOGV("reconnect");
    sp<Camera> camera = get_native_camera(this, NULL);
    if (camera == 0) return INVALID_OPERATION;

	status_t ret = camera->reconnect();
    if (ret != NO_ERROR) {
        //jniThrowException(env, "java/io/IOException", "reconnect failed");
        ALOGE("(f:%s, l:%d) reconnect failed", __FUNCTION__, __LINE__);
        return ret;
    }
	return NO_ERROR;
}

status_t HerbCamera::setPreviewDisplay(int hlay)
{
    ALOGV("setPreviewDisplay");
    sp<Camera> camera = get_native_camera(this, NULL);
    if (camera == 0) return INVALID_OPERATION;

    int surfaceLayerId = hlay;
	status_t ret = camera->setPreviewDisplay(surfaceLayerId);
    if (ret != NO_ERROR) {
        ALOGE("(f:%s, l:%d) setPreviewDisplay failed", __FUNCTION__, __LINE__);
		return ret;
    }
	return NO_ERROR;
}

status_t HerbCamera::startPreview()
{
    ALOGV("startPreview");
    sp<Camera> camera = get_native_camera(this, NULL);
    if (camera == 0) return INVALID_OPERATION;

	status_t ret = camera->startPreview();
    if (ret != NO_ERROR) {
        ALOGE("(f:%s, l:%d) startPreview failed", __FUNCTION__, __LINE__);
        return ret;
    }
	return NO_ERROR;
}

void HerbCamera::stopPreview()
{
    ALOGV("stopPreview");
    sp<Camera> c = get_native_camera(this, NULL);
    if (c == 0) 
    {
        ALOGE("(f:%s, l:%d) native_camera is null", __FUNCTION__, __LINE__);
        return;
    }

    c->stopPreview();
    
    mFaceDetectionRunning = false;

    mpShutterCallback = NULL;
    mpRawImageCallback = NULL;
    mpPostviewCallback = NULL;
    mpJpegCallback = NULL;
    {
        Mutex::Autolock _l(mAutoFocusCallbackLock);
        mpAutoFocusCallback = NULL;
    }
    mpAutoFocusMoveCallback = NULL;
}

bool HerbCamera::previewEnabled()
{
    ALOGV("previewEnabled");
    sp<Camera> c = get_native_camera(this, NULL);
    if (c == 0) return false;

    return c->previewEnabled();
}

void HerbCamera::setPreviewCallback(PreviewCallback *pCb)
{
    mpPreviewCallback = pCb;
    mOneShot = false;
    mWithBuffer = false;

    bool installed = (mpPreviewCallback!=NULL);
    bool manualBuffer = false;
    // Always use one-shot mode. We fake camera preview mode by
    // doing one-shot preview continuously.
    setHasPreviewCallback(pCb!=NULL, false);
}

    
    /**
     * <p>Installs a callback to be invoked for the next preview frame in
     * addition to displaying it on the screen.  After one invocation, the
     * callback is cleared. This method can be called any time, even when
     * preview is live.  Any other preview callbacks are overridden.</p>
     *
     * <p>If you are using the preview data to create video or still images,
     * strongly consider using {@link android.media.MediaActionSound} to
     * properly indicate image capture or recording start/stop to the user.</p>
     *
     * @param cb a callback object that receives a copy of the next preview frame,
     *     or null to stop receiving callbacks.
     * @see android.media.MediaActionSound
     */
void HerbCamera::setOneShotPreviewCallback(PreviewCallback *pCb)
{
    mpPreviewCallback = pCb;
    mOneShot = true;
    mWithBuffer = false;
    setHasPreviewCallback(pCb != NULL, false);
}

void HerbCamera::setHasPreviewCallback(bool installed, bool manualBuffer)
{
    ALOGV("setHasPreviewCallback: installed:%d, manualBuffer:%d", (int)installed, (int)manualBuffer);
    // Important: Only install preview_callback if the Java code has called
    // setPreviewCallback() with a non-null value, otherwise we'd pay to memcpy
    // each preview frame for nothing.
    HerbCameraContext* context;
    sp<Camera> camera = get_native_camera(this, &context);
    if (camera == 0) return;

    // setCallbackMode will take care of setting the context flags and calling
    // camera->setPreviewCallbackFlags within a mutex for us.
    context->setCallbackMode(installed, manualBuffer);
}

void HerbCamera::setPreviewCallbackWithBuffer(PreviewCallback *pCb) {
        mpPreviewCallback = pCb;
        mOneShot = false;
        mWithBuffer = true;
        setHasPreviewCallback(pCb != NULL, true);
}

void HerbCamera::addCallbackBuffer(sp<CMediaMemory> &callbackBuffer)
{
    _addCallbackBuffer(callbackBuffer, CAMERA_MSG_PREVIEW_FRAME);
}

void HerbCamera::addRawImageCallbackBuffer(sp<CMediaMemory> &callbackBuffer)
{
    addCallbackBuffer(callbackBuffer, CAMERA_MSG_RAW_IMAGE);
}

void HerbCamera::addCallbackBuffer(sp<CMediaMemory> &callbackBuffer, int msgType)
{
    // CAMERA_MSG_VIDEO_FRAME may be allowed in the future.
    if (msgType != CAMERA_MSG_PREVIEW_FRAME &&
        msgType != CAMERA_MSG_RAW_IMAGE) {
        ALOGE("(f:%s, l:%d) Unsupported message type: %d", __FUNCTION__, __LINE__, msgType);
    }

    _addCallbackBuffer(callbackBuffer, msgType);
}

void HerbCamera::_addCallbackBuffer(sp<CMediaMemory> &callbackBuffer, int msgType)
{
    ALOGV("addCallbackBuffer: 0x%x", msgType);

    HerbCameraContext* context = reinterpret_cast<HerbCameraContext*>(mNativeContext);

    if (context != NULL) {
        context->addCallbackBuffer(callbackBuffer, msgType);
    }
}

//void HerbCamera::postEventFromNative(wp<HerbCamera>& camera_ref, int what, int arg1, int arg2, void *obj)
void HerbCamera::postEventFromNative(HerbCamera* pC, int what, int arg1, int arg2, void *obj)
{
    //sp<HerbCamera> pC = camera_ref.promote();
    if (pC == NULL)
    {
        ALOGE("(f:%s, l:%d) pC==NULL, impossible!", __FUNCTION__, __LINE__);
        return;
    }
    if (pC->mpEventHandler != NULL) {
        MediaCallbackMessage msg;
        msg.what = what;
        msg.arg1 = arg1;
        msg.arg2 = arg2;
        if (CAMERA_MSG_COMPRESSED_IMAGE == msg.what ||
            CAMERA_MSG_ADAS_METADATA == msg.what ||
            CAMERA_MSG_POSTVIEW_FRAME == msg.what ||
            CAMERA_MSG_QRDECODE == msg.what)
        {
            msg.obj = NULL;
            msg.dataCompressedImage = *(sp<IMemory>*)obj;
        }
        else
        {
            msg.obj = obj;
        }
        pC->mpEventHandler->post(msg);
    }
}

status_t HerbCamera::autoFocus(AutoFocusCallback *pCb)
{
    {
        Mutex::Autolock _l(mAutoFocusCallbackLock);
        mpAutoFocusCallback = pCb;
    }
    ALOGV("autoFocus");
    HerbCameraContext* context;
    sp<Camera> c = get_native_camera(this, &context);
    if (c == 0) return INVALID_OPERATION;

	status_t ret = c->autoFocus();
    if (ret != NO_ERROR) {
        //jniThrowRuntimeException(env, "autoFocus failed");
        ALOGE("(f:%s, l:%d) autoFocus failed", __FUNCTION__, __LINE__);
		return ret;
    }
	return NO_ERROR;
}

status_t HerbCamera::cancelAutoFocus()
{
    ALOGE("(f:%s, l:%d) only for test, for fun. Don't use it now", __FUNCTION__, __LINE__);
    {
        Mutex::Autolock _l(mAutoFocusCallbackLock);
        mpAutoFocusCallback = NULL;
    }

    ALOGV("cancelAutoFocus");
    HerbCameraContext* context;
    sp<Camera> c = get_native_camera(this, &context);
    if (c == 0) return INVALID_OPERATION;

	status_t ret = c->cancelAutoFocus();
    if (ret != NO_ERROR) {
        //jniThrowRuntimeException(env, "cancelAutoFocus failed");
        ALOGE("(f:%s, l:%d) cancelAutoFocus failed", __FUNCTION__, __LINE__);
		return ret;
    }
    // CAMERA_MSG_FOCUS should be removed here because the following
    // scenario can happen:
    // - An application uses the same thread for autoFocus, cancelAutoFocus
    //   and looper thread.
    // - The application calls autoFocus.
    // - HAL sends CAMERA_MSG_FOCUS, which enters the looper message queue.
    //   Before event handler's handleMessage() is invoked, the application
    //   calls cancelAutoFocus and autoFocus.
    // - The application gets the old CAMERA_MSG_FOCUS and thinks autofocus
    //   has been completed. But in fact it is not.
    //
    // As documented in the beginning of the file, apps should not use
    // multiple threads to call autoFocus and cancelAutoFocus at the same
    // time. It is HAL's responsibility not to send a CAMERA_MSG_FOCUS
    // message after native_cancelAutoFocus is called.
    ALOGE("(f:%s, l:%d) sorry, not implement removeMessages now.", __FUNCTION__, __LINE__);
    //mpEventHandler.removeMessages(CAMERA_MSG_FOCUS);
    return NO_ERROR;
}

status_t HerbCamera::setAutoFocusMoveCallback(AutoFocusMoveCallback *pCb)
{
    mpAutoFocusMoveCallback = pCb;
    return enableFocusMoveCallback((mpAutoFocusMoveCallback != NULL) ? 1 : 0);
}

status_t HerbCamera::enableFocusMoveCallback(int enable)
{
    ALOGV("enableFocusMoveCallback");
    sp<Camera> camera = get_native_camera(this, NULL);
    if (camera == 0) return INVALID_OPERATION;

	status_t ret = camera->sendCommand(CAMERA_CMD_ENABLE_FOCUS_MOVE_MSG, enable, 0);
    if (ret != NO_ERROR) {
        //jniThrowRuntimeException(env, "enable focus move callback failed");
        ALOGE("(f:%s, l:%d) enable focus move callback failed", __FUNCTION__, __LINE__);
		return ret;
    }
	return NO_ERROR;
}

status_t HerbCamera::takePicture(ShutterCallback *pShutter, PictureCallback *pRaw,
            PictureCallback *pJpeg)
{
    return takePicture(pShutter, pRaw, NULL, pJpeg);
}
status_t HerbCamera::takePicture(ShutterCallback *pShutter, PictureCallback *pRaw,
            PictureCallback *pPostview, PictureCallback *pJpeg) {
    mpShutterCallback = pShutter;
    mpRawImageCallback = pRaw;
    mpPostviewCallback = pPostview;
    mpJpegCallback = pJpeg;

    // If callback is not set, do not send me callbacks.
    int msgType = 0;
	status_t ret = NO_ERROR;

    if (mpShutterCallback != NULL) {
        msgType |= CAMERA_MSG_SHUTTER;
    }
    if (mpRawImageCallback != NULL) {
        msgType |= CAMERA_MSG_RAW_IMAGE;
    }
    if (mpPostviewCallback != NULL) {
        msgType |= CAMERA_MSG_POSTVIEW_FRAME;
    }
    if (mpJpegCallback != NULL) {
        msgType |= CAMERA_MSG_COMPRESSED_IMAGE;
    }

    ALOGV("takePicture");
    HerbCameraContext* context;
    sp<Camera> camera = get_native_camera(this, &context);
    if (camera == 0) {
		ret = INVALID_OPERATION;
		goto _exit_takePicture;
    }

    /*
     * When CAMERA_MSG_RAW_IMAGE is requested, if the raw image callback
     * buffer is available, CAMERA_MSG_RAW_IMAGE is enabled to get the
     * notification _and_ the data; otherwise, CAMERA_MSG_RAW_IMAGE_NOTIFY
     * is enabled to receive the callback notification but no data.
     *
     * Note that CAMERA_MSG_RAW_IMAGE_NOTIFY is not exposed to the
     * Java application.
     */
    if (msgType & CAMERA_MSG_RAW_IMAGE) {
        ALOGV("Enable raw image callback buffer");
        if (!context->isRawImageCallbackBufferAvailable()) {
            ALOGV("Enable raw image notification, since no callback buffer exists");
            msgType &= ~CAMERA_MSG_RAW_IMAGE;
            msgType |= CAMERA_MSG_RAW_IMAGE_NOTIFY;
        }
    }

    if ((ret = camera->takePicture(msgType)) != NO_ERROR) {
        //jniThrowRuntimeException(env, "takePicture failed");
        ALOGE("(f:%s, l:%d) takePicture failed", __FUNCTION__, __LINE__);
    }

_exit_takePicture:
    mFaceDetectionRunning = false;
	return ret;
}


status_t HerbCamera::startSmoothZoom(int value)
{
    ALOGV("startSmoothZoom");
    sp<Camera> camera = get_native_camera(this, NULL);
    if (camera == 0) return INVALID_OPERATION;

    status_t rc = camera->sendCommand(CAMERA_CMD_START_SMOOTH_ZOOM, value, 0);
    if (rc == BAD_VALUE) {
        char msg[64];
        sprintf(msg, "invalid zoom value=%d", value);
        ALOGE("(f:%s, l:%d) %s", __FUNCTION__, __LINE__, msg);
    } else if (rc != NO_ERROR) {
        ALOGE("(f:%s, l:%d) start smooth zoom failed", __FUNCTION__, __LINE__);
    }
	return rc;
}

status_t HerbCamera::stopSmoothZoom()
{
    ALOGV("stopSmoothZoom");
    sp<Camera> camera = get_native_camera(this, NULL);
    if (camera == 0) return INVALID_OPERATION;

	status_t ret = camera->sendCommand(CAMERA_CMD_STOP_SMOOTH_ZOOM, 0, 0);
    if (ret != NO_ERROR) {
        //jniThrowRuntimeException(env, "stop smooth zoom failed");
        ALOGE("(f:%s, l:%d) stop smooth zoom failed", __FUNCTION__, __LINE__);
		return ret;
    }
	return NO_ERROR;
}

status_t HerbCamera::setDisplayOrientation(int degrees)
{
    ALOGV("setDisplayOrientation");
    sp<Camera> camera = get_native_camera(this, NULL);
    if (camera == 0) return INVALID_OPERATION;

	status_t ret = camera->sendCommand(CAMERA_CMD_SET_DISPLAY_ORIENTATION, degrees, 0);
    if (ret != NO_ERROR) {
        //jniThrowRuntimeException(env, "set display orientation failed");
        ALOGE("(f:%s, l:%d) set display orientation failed", __FUNCTION__, __LINE__);
		return ret;
    }
	return NO_ERROR;
}

bool HerbCamera::enableShutterSound(bool enabled)
{
//    if (!enabled) {
//        IBinder b = ServiceManager.getService(Context.AUDIO_SERVICE);
//        IAudioService audioService = IAudioService.Stub.asInterface(b);
//        try {
//            if (audioService.isCameraSoundForced()) return false;
//        } catch (RemoteException e) {
//            Log.e(TAG, "Audio service is unavailable for queries");
//        }
//    }
    if(!enabled)
    {
        ALOGW("(f:%s, l:%d) now not check audioService.", __FUNCTION__, __LINE__);
    }
    ALOGV("enableShutterSound");
    sp<Camera> camera = get_native_camera(this, NULL);
    if (camera == 0) return false;

    int32_t value = (enabled == true) ? 1 : 0;
    status_t rc = camera->sendCommand(CAMERA_CMD_ENABLE_SHUTTER_SOUND, value, 0);
    if (rc == NO_ERROR) {
        return true;
    } else if (rc == PERMISSION_DENIED) {
        return false;
    } else {
        //jniThrowRuntimeException(env, "enable shutter sound failed");
        ALOGE("(f:%s, l:%d) enable shutter sound failed", __FUNCTION__, __LINE__);
        return false;
    }
}

void HerbCamera::setZoomChangeListener(OnZoomChangeListener *pListener)
{
    mpZoomListener = pListener;
}

void HerbCamera::setFaceDetectionListener(FaceDetectionListener *pListener)
{
    mpFaceListener = pListener;
}

void HerbCamera::startFaceDetection() {
    if (mFaceDetectionRunning) {
        //throw new RuntimeException("Face detection is already running");
        ALOGE("(f:%s, l:%d) Face detection is already running", __FUNCTION__, __LINE__);
    }
    
    int type = CAMERA_FACE_DETECTION_HW;
    ALOGV("startFaceDetection");
    HerbCameraContext* context;
    sp<Camera> camera = get_native_camera(this, &context);
    if (camera == 0) return;

    status_t rc = camera->sendCommand(CAMERA_CMD_START_FACE_DETECTION, type, 0);
    if (rc == BAD_VALUE) {
        char msg[64];
        snprintf(msg, sizeof(msg), "invalid face detection type=%d", type);
        //jniThrowException(env, "java/lang/IllegalArgumentException", msg);
        ALOGE("(f:%s, l:%d) %s", __FUNCTION__, __LINE__, msg);
    } else if (rc != NO_ERROR) {
        //jniThrowRuntimeException(env, "start face detection failed");
        ALOGE("(f:%s, l:%d) start face detection failed", __FUNCTION__, __LINE__);
    }

    mFaceDetectionRunning = true;
}

void HerbCamera::stopFaceDetection() {
    ALOGV("stopFaceDetection");
    sp<Camera> camera = get_native_camera(this, NULL);
    if (camera == 0) goto _exit_stopFaceDetection;

    if (camera->sendCommand(CAMERA_CMD_STOP_FACE_DETECTION, 0, 0) != NO_ERROR) {
        //jniThrowRuntimeException(env, "stop face detection failed");
        ALOGE("(f:%s, l:%d) stop face detection failed", __FUNCTION__, __LINE__);
    }
_exit_stopFaceDetection:
    mFaceDetectionRunning = false;
}

/* MOTION_DETECTION_ENABLE start*/
void HerbCamera::setAWMoveDetectionListener(AWMoveDetectionListener *pListener)
{
    mpAWMDListener = pListener;
}

void HerbCamera::startAWMoveDetection() {
	ALOGV("startAWMoveDetection");
	if (mAWMDrunning) {
		ALOGW("(f:%s, l:%d) AW move detection is already running", __FUNCTION__, __LINE__);
		return;
	}
	sp<Camera> camera = get_native_camera(this, NULL);
	if (camera == NULL) return;

	status_t rc = camera->sendCommand(CAMERA_CMD_START_AWMD, 0, 0);
	if (rc != NO_ERROR) {
		ALOGE("start AW move detection failed");
		return;
	}
	mAWMDrunning = true;
}

void HerbCamera::stopAWMoveDetection() {
	ALOGV("stopAWMoveDetection");
	if (!mAWMDrunning) {
		ALOGW("(f:%s, l:%d) AW move detection is not running", __FUNCTION__, __LINE__);
		return;
	}
	sp<Camera> camera = get_native_camera(this, NULL);
	if (camera == NULL) goto EXIT_AWMD;

	if (camera->sendCommand(CAMERA_CMD_STOP_AWMD, 0, 0) != NO_ERROR) {
		ALOGE("stop AW move detection failed");
	}
EXIT_AWMD:
	mAWMDrunning = false;
}

status_t HerbCamera::setAWMDSensitivityLevel(int level)
{
	ALOGV("setAWMDSensitivityLevel(%d)", level);
	if (!mAWMDrunning) {
		ALOGW("(f:%s, l:%d) AW move detection is not running", __FUNCTION__, __LINE__);
		return INVALID_OPERATION;
	}
	sp<Camera> camera = get_native_camera(this, NULL);
	if (camera == NULL) {
		ALOGW("(f:%s, l:%d) camera not initialize!!", __FUNCTION__, __LINE__);
		return INVALID_OPERATION;
	}
	if (camera->sendCommand(CAMERA_CMD_SET_AWMD_SENSITIVITY_LEVEL, level, 0) != NO_ERROR) {
		ALOGE("(f:%s, l:%d) set AW move detection sensitivity level failed", __FUNCTION__, __LINE__);
		return UNKNOWN_ERROR;
	}
	return NO_ERROR;
}
/* MOTION_DETECTION_ENABLE end*/

/* for ADAS start*/
void HerbCamera::setAdasDetectionListener(AdasDetectionListener *pListener)
{
    mpAdasListener = pListener;
}

void HerbCamera::adasStartDetection() {
	ALOGV("adasStartDetection");
	if (mAdasDetectionRunning) {
		ALOGW("(f:%s, l:%d) Adas detection is already running", __FUNCTION__, __LINE__);
		return;
	}
	sp<Camera> camera = get_native_camera(this, NULL);
	if (camera == NULL) return;

	status_t rc = camera->sendCommand(CAMERA_CMD_ADAS_START_DETECTION, 0, 0);
	if (rc != NO_ERROR) {
		ALOGE("start Adas detection failed");
		return;
	}
	mAdasDetectionRunning = true;
}

void HerbCamera::adasStopDetection() {
	ALOGV("adasStopDetection");
	if (!mAdasDetectionRunning) {
		ALOGW("(f:%s, l:%d) Adas detection is not running", __FUNCTION__, __LINE__);
		return;
	}
	sp<Camera> camera = get_native_camera(this, NULL);
	if (camera == NULL) goto EXIT_ADAS;

	if (camera->sendCommand(CAMERA_CMD_ADAS_STOP_DETECTION, 0, 0) != NO_ERROR) {
		ALOGE("stop Adas detection failed");
	}
EXIT_ADAS:
	mAdasDetectionRunning = false;
}

status_t HerbCamera::adasSetLaneLineOffsetSensity(int level)
{
	sp<Camera> camera = get_native_camera(this, NULL);
	if (camera == NULL) {
        ALOGE("<F:%s, L:%d> get_native_camera error!", __FUNCTION__, __LINE__);
        return UNKNOWN_ERROR;
	}
    status_t ret = camera->sendCommand(CAMERA_CMD_ADAS_SET_LANELINE_OFFSET_SENSITY, level, 0);
	if (ret != NO_ERROR) {
		ALOGE("CAMERA_CMD_ADAS_SET_LANELINE_OFFSET_SENSITY error!");
        return ret;
	}
    return NO_ERROR;
}

status_t HerbCamera::adasSetDistanceDetectLevel(int level)
{
	sp<Camera> camera = get_native_camera(this, NULL);
	if (camera == NULL) {
        ALOGE("<F:%s, L:%d> get_native_camera error!", __FUNCTION__, __LINE__);
        return UNKNOWN_ERROR;
	}
    status_t ret = camera->sendCommand(CAMERA_CMD_ADAS_SET_DISTANCE_DETECT_LEVEL, level, 0);
	if (ret != NO_ERROR) {
		ALOGE("CAMERA_CMD_ADAS_SET_DISTANCE_DETECT_LEVEL error!");
        return ret;
	}
    return NO_ERROR;
}

status_t HerbCamera::adasSetMode(int mode)
{
	sp<Camera> camera = get_native_camera(this, NULL);
	if (camera == NULL) {
        ALOGE("<F:%s, L:%d> get_native_camera error!", __FUNCTION__, __LINE__);
        return UNKNOWN_ERROR;
	}
    status_t ret = camera->sendCommand(CAMERA_CMD_ADAS_MODE, mode, 0);
	if (ret != NO_ERROR) {
		ALOGE("CAMERA_CMD_ADAS_MODE error!");
        return ret;
	}
	 return NO_ERROR;
}

status_t HerbCamera::setAdasTipsMode(int mode)
{
	sp<Camera> camera = get_native_camera(this, NULL);
	if (camera == NULL) {
        ALOGE("<F:%s, L:%d> get_native_camera error!", __FUNCTION__, __LINE__);
        return UNKNOWN_ERROR;
	}
    status_t ret = camera->sendCommand(CAMERA_CMD_ADAS_TIPS_MODE, mode, 0);
	if (ret != NO_ERROR) {
		ALOGE("CAMERA_CMD_ADAS_TIPS_MODE error!");
        return ret;
	}
	 return NO_ERROR;
}

status_t HerbCamera::adasSetCarSpeed(float speed)
{
	sp<Camera> camera = get_native_camera(this, NULL);
	if (camera == NULL) {
        ALOGE("<F:%s, L:%d> get_native_camera error!", __FUNCTION__, __LINE__);
        return UNKNOWN_ERROR;
	}
    int32_t spd = (int32_t)(speed * 10000);
    status_t ret = camera->sendCommand(CAMERA_CMD_ADAS_SET_CAR_SPEED, spd, 0);
	if (ret != NO_ERROR) {
		ALOGE("CAMERA_CMD_ADAS_SET_CAR_SPEED error!");
        return ret;
	}
    return NO_ERROR;
}

status_t HerbCamera::adasSetAduioVol(int val)
{
       sp<Camera> camera = get_native_camera(this, NULL);
       if (camera == NULL) {
        ALOGE("<F:%s, L:%d> get_native_camera error!", __FUNCTION__, __LINE__);
        return UNKNOWN_ERROR;
       }
    status_t ret = camera->sendCommand(CAMERA_CMD_ADAS_AUDIO_VOL,(int)val,0);
       if (ret != NO_ERROR) {
               ALOGE("CAMERA_CMD_ADAS_SHOW_PLATE error!");
        return ret;
       }
    return NO_ERROR;
}

status_t HerbCamera::adasSetGsensorData(int val0, float val1, float val2)
{
    ALOGV("adasSetGsensorData(%d,%f,%f)", val0, val1, val2);
	sp<Camera> camera = get_native_camera(this, NULL);
	if (camera == NULL) {
        ALOGE("<F:%s, L:%d> get_native_camera error!", __FUNCTION__, __LINE__);
        return UNKNOWN_ERROR;
	}
    status_t ret = camera->adasSetGsensorData(val0, val1, val2);
	if (ret != NO_ERROR) {
		ALOGE("<F:%s, L:%d> adasSetGsensorData error!", __FUNCTION__, __LINE__);
        return ret;
	}
    return NO_ERROR;
}
/* for ADAS end*/

#ifdef SUPPORT_TVIN
void HerbCamera::setTVinListener(TVinListener *pListener)
{
    mpTVinListener = pListener;
}
#endif

/* WATERMARK_ENABLE start */
status_t HerbCamera::setWaterMark(int enable, const char *str)
{
	ALOGV("setWaterMark(%d, %s)", enable, str);
	sp<Camera> camera = get_native_camera(this, NULL);
	if (camera == NULL) {
        ALOGE("<F:%s, L:%d> get_native_camera error!", __FUNCTION__, __LINE__);
        return NO_INIT;
	}

	return camera->setWaterMark(enable, str);
}
/* WATERMARK_ENABLE end */

void HerbCamera::setQrDecodeListener(QrDecodeListener * pListener)
{
    mpQrDecodeListener = pListener;
}

status_t HerbCamera::setQrDecode(bool enable)
{
    ALOGV("setQrDecode");
	sp<Camera> camera = get_native_camera(this, NULL);
	if (camera == NULL) return NO_INIT;

    status_t ret = camera->sendCommand(CAMERA_CMD_SET_QRDECODE, (enable==true ? 1: 0), 0);
	if (ret != NO_ERROR) {
		ALOGE("CAMERA_CMD_SET_QRDECODE failed");
        return ret;
	}
    return NO_ERROR;
}

status_t HerbCamera::setUvcGadgetMode(int value)
{
    ALOGV("setUvcGadgetMode(%d)", value);
	sp<Camera> camera = get_native_camera(this, NULL);
	if (camera == NULL) {
        ALOGE("<F:%s, L:%d> get_native_camera error!", __FUNCTION__, __LINE__);
        return UNKNOWN_ERROR;
	}
    status_t ret = camera->setUvcGadgetMode(value);
	if (ret != NO_ERROR) {
		ALOGE("<F:%s, L:%d> setUvcGadgetMode error!", __FUNCTION__, __LINE__);
        return ret;
	}
    return NO_ERROR;
}

status_t HerbCamera::startRender(void)
{
	sp<Camera> camera = get_native_camera(this, NULL);
	if (camera == NULL) {
        ALOGE("<F:%s, L:%d> get_native_camera error!", __FUNCTION__, __LINE__);
        return UNKNOWN_ERROR;
	}
    status_t ret = camera->sendCommand(CAMERA_CMD_START_RENDER, 0, 0);
	if (ret != NO_ERROR) {
		ALOGE("CAMERA_CMD_START_RENDER error!");
        return ret;
	}
    return NO_ERROR;
}

status_t HerbCamera::stopRender(void)
{
	sp<Camera> camera = get_native_camera(this, NULL);
	if (camera == NULL) {
        ALOGE("<F:%s, L:%d> get_native_camera error!", __FUNCTION__, __LINE__);
        return UNKNOWN_ERROR;
	}
    status_t ret = camera->sendCommand(CAMERA_CMD_STOP_RENDER, 0, 0);
	if (ret != NO_ERROR) {
		ALOGE("CAMERA_CMD_STOP_RENDER error!");
        return ret;
	}
    return NO_ERROR;
}

status_t HerbCamera::setColorFx(int value)
{
    ALOGV("setColorFx %d", value);
	sp<Camera> camera = get_native_camera(this, NULL);
	if (camera == NULL) return BAD_VALUE;

	if (camera->sendCommand(CAMERA_CMD_SET_COLORFX, value, 0) != NO_ERROR) {
		ALOGE("setColorFx failed");
        return BAD_VALUE;
	}
    return NO_ERROR;
}

void HerbCamera::setErrorCallback(ErrorCallback *pCb)
{
    mpErrorCallback = pCb;
}

status_t HerbCamera::getParameters(Parameters *paras)
{
    ALOGV("getParameters");
    sp<Camera> camera = get_native_camera(this, NULL);
    if (camera == 0) return BAD_VALUE;

    String8 params8 = camera->getParameters();
    if (params8.isEmpty()) {
        //jniThrowRuntimeException(env, "getParameters failed (empty parameters)");
        ALOGE("(f:%s, l:%d) getParameters failed (empty parameters)", __FUNCTION__, __LINE__);
        //return 0;
        return BAD_VALUE;
    }

	paras->unflatten(params8);

	return NO_ERROR;
}

status_t HerbCamera::setParameters(Parameters *params)
{
    ALOGV("setParameters");
    sp<Camera> camera = get_native_camera(this, NULL);
    if (camera == 0) return BAD_VALUE;

    String8 params8;
    if(params)
    {
        params8 = params->flatten();
    }
    if (camera->setParameters(params8) != NO_ERROR) {
        //jniThrowRuntimeException(env, "setParameters failed");
        ALOGE("(f:%s, l:%d) setParameters failed", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    return NO_ERROR;
}

int HerbCamera::getV4l2QueryCtrl(struct v4l2_queryctrl *ctrl, int id)
{
	ALOGV("getV4l2QueryCtrl");
	sp<Camera> camera = get_native_camera(this, NULL);
	if (camera == 0) return BAD_VALUE;
	
	return camera->getV4l2QueryCtrl(ctrl, id);
}

int HerbCamera::getContrastCtrl(struct v4l2_queryctrl *ctrl)
{
	return getV4l2QueryCtrl(ctrl, V4L2_CID_CONTRAST);
}

int HerbCamera::getBrightnessCtrl(struct v4l2_queryctrl *ctrl)
{
	return getV4l2QueryCtrl(ctrl, V4L2_CID_BRIGHTNESS);
}

int HerbCamera::getSaturationCtrl(struct v4l2_queryctrl *ctrl)
{
	return getV4l2QueryCtrl(ctrl, V4L2_CID_SATURATION);
}

int HerbCamera::getHueCtrl(struct v4l2_queryctrl *ctrl)
{
	return getV4l2QueryCtrl(ctrl, V4L2_CID_HUE);
}

int HerbCamera::getSharpnessCtrl(struct v4l2_queryctrl *ctrl)
{
	return getV4l2QueryCtrl(ctrl, V4L2_CID_SHARPNESS);
}

status_t HerbCamera::getInputSource(int *source)
{
	ALOGV("getInputSource");
	sp<Camera> camera = get_native_camera(this, NULL);
	if (camera == NULL) return BAD_VALUE;
	
	return camera->getInputSource(source);
}

status_t HerbCamera::getTVinSystem(int *system)
{
	ALOGV("getTVinSystem");
	sp<Camera> camera = get_native_camera(this, NULL);
	if (camera == NULL) return BAD_VALUE;
	
	return camera->getTVinSystem(system);
}

status_t HerbCamera::cancelContinuousPicture()
{
	ALOGV("cancelContinuousPicture");
	sp<Camera> camera = get_native_camera(this, NULL);
	if (camera == NULL) {
        ALOGE("<F:%s, L:%d> get_native_camera error!", __FUNCTION__, __LINE__);
        return NO_INIT;
	}
    status_t ret = camera->sendCommand(CAMERA_CMD_CANCEL_CONTINUOUS_SNAP, 0, 0);
	if (ret != NO_ERROR) {
		ALOGE("CAMERA_CMD_CANCEL_CONTINUOUS_PICTURE error!");
        return ret;
	}
    return NO_ERROR;
}


const char HerbCamera::Parameters::SUPPORTED_VALUES_SUFFIX[] = "-values";

// Parse string like "640x480" or "10000,20000"
int HerbCamera::parse_pair(const char *str, int *first, int *second, char delim,
                      char **endptr)
{
    // Find the first integer.
    char *end;
    int w = (int)strtol(str, &end, 10);
    // If a delimeter does not immediately follow, give up.
    if (*end != delim) {
        ALOGE("Cannot find delimeter (%c) in str=%s", delim, str);
        return -1;
    }

    // Find the second integer, immediately after the delimeter.
    int h = (int)strtol(end+1, &end, 10);

    *first = w;
    *second = h;

    if (endptr) {
        *endptr = end;
    }

    return 0;
}

void HerbCamera::parseSizesList(const char *sizesStr, Vector<Size> &sizes)
{
    if (sizesStr == 0) {
        return;
    }

    char *sizeStartPtr = (char *)sizesStr;

    while (true) {
        int width, height;
        int success = parse_pair(sizeStartPtr, &width, &height, 'x',
                                 &sizeStartPtr);
        if (success == -1 || (*sizeStartPtr != ',' && *sizeStartPtr != '\0')) {
            ALOGE("Picture sizes string \"%s\" contains invalid character.", sizesStr);
            return;
        }
        sizes.push(Size(width, height));

        if (*sizeStartPtr == '\0') {
            return;
        }
        sizeStartPtr++;
    }
}

void HerbCamera::parseIntList(const char *intStr, Vector<int>& values)
{
	char *str = (char *)intStr;
    if (str == NULL) return;
	char *end;
	int val = 0;

	while (true) {
		val = (int)strtol(str, &end, 10);
		if (*end == ',') {
			values.push(val);
			str = end + 1;
		} else if (*end == '\0') {
			values.push(val);
			break;
		} else {
			break;
		}
	}
}

void HerbCamera::parseRangeList(const char *rangeStr, Vector<int[2]>& ranges)
{
	if (rangeStr == NULL || rangeStr[0] != '(' || rangeStr[strlen(rangeStr) - 1] != ')') {
		ALOGE("Invalid range list string");
		return;
	}
	char *end;
	char *str = (char *)rangeStr;
	int val[2];

	str++;
	while (true) {
		val[0] = strtol(str, &end, 10);
		if (*end != ',') {
			break;
		}
		str = end + 1;

		val[1] = strtol(str, &end, 10);
		if (*end != ')') {
			break;
		}
		str = end + 1;

		ranges.push(val);
		if (str[0] != ',' || str[1] != '(') {
			break;
		}
		str += 2;
	}
}

void HerbCamera::parseString8List(const char *strIn, Vector<String8>& strs)
{
	if (strIn == NULL) {
		ALOGE("Invalid range list string NULL");
		return;
	}
	char *str = (char *)strIn;
	char *end;
	char buf[256];
	int size;

	while (true) {
		end = strchr(str, ',');
		if (end == NULL) {
			break;
		}
		size = end - str;
		if (size > 255) {
			break;
		}
		memcpy(buf, str, size);
		buf[size] = '\0';
		strs.push(String8(buf));
		str = end + 1;
	}
}

void HerbCamera::parseFloat(const char *floatStr, float *values, int size)
{
	if (floatStr == NULL || values == NULL) {
		return;
	}
	char *end;
	char *str = (char *)floatStr;

	for (int i = 0; i < size; i++) {
		*values++ = strtod(str, &end);
		if (*end != ',') {
			break;
		}
		str = end + 1;
	}
}

void HerbCamera::parseInt(const char *intStr, int *values, int size)
{
	if (intStr == NULL || values == NULL) {
		return;
	}
	char *end;
	char *str = (char *)intStr;

	for (int i = 0; i < size; i++) {
		*values++ = strtol(str, &end, 10);
		if (*end != ',') {
			break;
		}
		str = end + 1;
	}
}

void HerbCamera::parseAreaList(const char *areaStr, Vector<Area> &areas)
{
	if (areaStr == NULL || areaStr[0] != '(' || areaStr[strlen(areaStr) - 1] != ')') {
		ALOGE("Invalid area list string");
		return;
	}
	char *end;
	char *str = (char *)areaStr;

	str++;
	while (true) {
		Area area;
		area.mRect->left = strtol(str, &end, 10);
		if (*end != ',') break;
		str = end + 1;
		area.mRect->top = strtol(str, &end, 10);
		if (*end != ',') break;
		str = end + 1;
		area.mRect->right = strtol(str, &end, 10);
		if (*end != ',') break;
		str = end + 1;
		area.mRect->bottom = strtol(str, &end, 10);
		if (*end != ',') break;
		str = end + 1;
		area.mWeight = strtol(str, &end, 10);
		if (*end != ')') break;
		str = end + 1;
		areas.push(area);

		if (str[0] != ',' || str[1] != '(') break;
		str += 2;
	}
}

HerbCamera::Parameters::Parameters() 
						:mMap()
{
}

HerbCamera::Parameters::~Parameters()
{
}

void HerbCamera::Parameters::dump()
{
	ALOGD("dump: mMap.size=%d", mMap.size());
	for (size_t i = 0; i < mMap.size(); i++) {
		String8 k, v;
        k = mMap.keyAt(i);
        v = mMap.valueAt(i);
		ALOGD("%s: %s\n", k.string(), v.string());
	}
}


String8 HerbCamera::Parameters::flatten() const
{
	String8 flattened("");
	size_t size = mMap.size();

	for (size_t i = 0; i < size; i++) {
		String8 k, v;
		k = mMap.keyAt(i);
		v = mMap.valueAt(i);

		flattened += k;
		flattened += "=";
		flattened += v;
		if (i != size-1)
			flattened += ";";
	}

	return flattened;
}

void HerbCamera::Parameters::unflatten(const String8 &params)
{
    const char *a = params.string();
    const char *b;

    mMap.clear();

    for (;;) {
        // Find the bounds of the key name.
        b = strchr(a, '=');
        if (b == 0)
            break;

        // Create the key string.
        String8 k(a, (size_t)(b-a));

        // Find the value.
        a = b+1;
        b = strchr(a, ';');
        if (b == 0) {
            // If there's no semicolon, this is the last item.
            String8 v(a);
            mMap.add(k, v);
            break;
        }

        String8 v(a, (size_t)(b-a));
        mMap.add(k, v);
        a = b+1;
    }
}

void HerbCamera::Parameters::remove(const char *key)
{
    mMap.removeItem(String8(key));
}

void HerbCamera::Parameters::set(const char *key, const char *value)
{
    if (strchr(key, '=') || strchr(key, ';')) {
        return;
    }

    if (strchr(value, '=') || strchr(value, ';')) {
        return;
    }

    mMap.replaceValueFor(String8(key), String8(value));
}

void HerbCamera::Parameters::set(const char *key, int value)
{
    char str[64];
    snprintf(str, 64, "%d", value);
    set(key, str);
}

void HerbCamera::Parameters::set(const char *key, Vector<Area> &areas)
{
    int size = areas.size();
    if (size == 0) {
        set(key, "(0,0,0,0,0)");
        return;
    }
//	char buffer[256];
//
//	buffer[0] = 0;
//	for (int i = 0; i < size; i++) {
//		const Area *p = &areas.itemAt(i);
//		snprintf(buffer, 256, "%s(%d,%d,%d,%d,%d)", buffer, 
//			p->mRect->left, p->mRect->top, p->mRect->right, p->mRect->bottom, p->mWeight);
//		if (i < size - 1) {
//			snprintf(buffer, 256, "%s,", buffer);
//		}
//	}
//	set(key, buffer);

    //use String8
    String8 bufferString8;
    for (int i = 0; i < size; i++) {
        const Area *p = &areas.itemAt(i);
        bufferString8.appendFormat("(%d,%d,%d,%d,%d)", p->mRect->left, p->mRect->top, p->mRect->right, p->mRect->bottom, p->mWeight);
        if (i < size - 1) {
            bufferString8.appendFormat(",");
        }
    }
    set(key, bufferString8.string());
}

const char *HerbCamera::Parameters::get(const char *key) const
{
    String8 v = mMap.valueFor(String8(key));
    if (v.length() == 0)
        return 0;
    return v.string();
}

int HerbCamera::Parameters::getInt(const char *key, int defaultValue) const
{
    const char *v = get(key);
    if (v == 0)
        return defaultValue;
    return strtol(v, 0, 0);
}

int HerbCamera::Parameters::getInt(const char *key) const
{
    const char *v = get(key);
    if (v == 0)
        return -1;
    return strtol(v, 0, 0);
}

float HerbCamera::Parameters::getFloat(const char *key, float defaultValue) const
{
    const char *v = get(key);
    if (v == 0) return defaultValue;
    return strtof(v, 0);
}

float HerbCamera::Parameters::getFloat(const char *key) const
{
    const char *v = get(key);
    if (v == 0) return -1;
    return strtof(v, 0);
}

void HerbCamera::Parameters::setPreviewSize(int width, int height)
{
    char str[32];
    snprintf(str, 32, "%dx%d", width, height);
    set(CameraParameters::KEY_PREVIEW_SIZE, str);
}

void HerbCamera::Parameters::setVideoSize(int width, int height)
{
    char str[32];
    snprintf(str, 32, "%dx%d", width, height);
    set(CameraParameters::KEY_VIDEO_SIZE, str);
}

void HerbCamera::Parameters::setSubChannelSize(int width, int height)
{
    char str[32];
    snprintf(str, 32, "%dx%d", width, height);
    set(CameraParameters::KEY_SUBCHANNEL_SIZE, str);
}

void HerbCamera::Parameters::getPreviewSize(Size &size) const
{
    size.width = size.height = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(CameraParameters::KEY_PREVIEW_SIZE);
    if (p == 0)  return;
    parse_pair(p, &size.width, &size.height, 'x');
}

void HerbCamera::Parameters::getVideoSize(Size &size) const
{
    size.width = size.height = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(CameraParameters::KEY_VIDEO_SIZE);
    if (p == 0)  return;
    parse_pair(p, &size.width, &size.height, 'x');
}


void HerbCamera::Parameters::getSupportedPreviewSizes(Vector<Size> &sizes) const
{
    const char *previewSizesStr = get(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES);
    parseSizesList(previewSizesStr, sizes);
}

void HerbCamera::Parameters::getSupportedVideoSizes(Vector<Size> &sizes) const
{
    String8 KEY_SUPPORTED_VIDEO_SIZES = String8(CameraParameters::KEY_VIDEO_SIZE) + String8(SUPPORTED_VALUES_SUFFIX);
    const char *videoSizesStr = get(KEY_SUPPORTED_VIDEO_SIZES.string());
    parseSizesList(videoSizesStr, sizes);
}

void HerbCamera::Parameters::getPreferredPreviewSizeForVideo(int *width, int *height) const
{
    *width = *height = -1;
    const char *p = get(CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO);
    if (p == 0)  return;
    parse_pair(p, width, height, 'x');
}

void HerbCamera::Parameters::setJpegThumbnailSize(int width, int height) {
	set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, width);
	set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, height);
}

void HerbCamera::Parameters::getJpegThumbnailSize(Size &size) {
	size.width = getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
	size.height = getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);
}

void HerbCamera::Parameters::getSupportedJpegThumbnailSizes(Vector<Size> &sizes)
{
	const char *sizeStr = get(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES);
    parseSizesList(sizeStr, sizes);
}

void HerbCamera::Parameters::setJpegThumbnailQuality(int quality)
{
	set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, quality);
}

int HerbCamera::Parameters::getJpegThumbnailQuality()
{
	return getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY);
}

void HerbCamera::Parameters::setJpegQuality(int quality)
{
	set(CameraParameters::KEY_JPEG_QUALITY, quality);
}

int HerbCamera::Parameters::getJpegQuality()
{
	return getInt(CameraParameters::KEY_JPEG_QUALITY);
}

void HerbCamera::Parameters::setPreviewFrameRate(int fps)
{
	set(CameraParameters::KEY_PREVIEW_FRAME_RATE, fps);
}

int HerbCamera::Parameters::getPreviewFrameRate()
{
	return getInt(CameraParameters::KEY_PREVIEW_FRAME_RATE);
}

void HerbCamera::Parameters::getSupportedPreviewFrameRates(Vector<int> &frameRates)
{
	const char *frameRatesStr = get(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES);
    parseIntList(frameRatesStr, frameRates);
}

void HerbCamera::Parameters::setPreviewFpsRange(int min, int max)
{
	char buffer[64];
	snprintf(buffer, 64, "%d,%d", min, max);
	set(CameraParameters::KEY_PREVIEW_FPS_RANGE, buffer);
}

void HerbCamera::Parameters::getPreviewFpsRange(int *min, int *max)
{
    *min = *max = -1;
    const char *p = get(CameraParameters::KEY_PREVIEW_FPS_RANGE);
    if (p == NULL)  return;
    parse_pair(p, min, max, ',');
}

void HerbCamera::Parameters::getSupportedPreviewFpsRange(Vector<int[2]> &range)
{
	const char *str = get(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE);
	if (str == NULL) return;
    parseRangeList(str, range);
}

void HerbCamera::Parameters::setPreviewFormat(int pixel_format)
{
	const char *s = cameraFormatForPixelFormat(pixel_format);
	if (s == NULL) {
		ALOGE("Invalid pixel_format=%d", pixel_format);
		return;
	}
    set(CameraParameters::KEY_PREVIEW_FORMAT, s);
}

int HerbCamera::Parameters::getPreviewFormat()
{
    return pixelFormatForCameraFormat(get(CameraParameters::KEY_PREVIEW_FORMAT));
}

void HerbCamera::Parameters::getSupportedPreviewFormats(Vector<int> &formats)
{
	const char *str = get(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS);
	if (str == NULL) {
		return;
	}
	Vector<String8> vstr8;
	int f;
	parseString8List(str, vstr8);
	for (size_t i = 0; i < vstr8.size(); i++) {
		f = pixelFormatForCameraFormat(vstr8.itemAt(i).string());
		if (f == IMAGE_FORMAT_UNKNOWN) continue;
		formats.push(f);
	}
}

void HerbCamera::Parameters::setPictureSize(int width, int height)
{
    char str[32];
    sprintf(str, "%dx%d", width, height);
	ALOGD("setPictureSize, width = %d, height = %d", width, height);
    set(CameraParameters::KEY_PICTURE_SIZE, str);
}

void HerbCamera::Parameters::getPictureSize(int *width, int *height) const
{
    *width = *height = -1;
    const char *p = get(CameraParameters::KEY_PICTURE_SIZE);
    if (p == 0) return;
    parse_pair(p, width, height, 'x');
}

void HerbCamera::Parameters::getSupportedPictureSizes(Vector<Size> &sizes) const
{
    const char *pictureSizesStr = get(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES);
    parseSizesList(pictureSizesStr, sizes);
}

void HerbCamera::Parameters::setPictureFormat(int pixel_format)
{
    const char *s = cameraFormatForPixelFormat(pixel_format);
    if (s == NULL) {
        ALOGE("Invalid pixel_format=%d", pixel_format);
		return;
    }

    set(CameraParameters::KEY_PICTURE_FORMAT, s);
}

int HerbCamera::Parameters::getPictureFormat()
{
    return pixelFormatForCameraFormat(get(CameraParameters::KEY_PICTURE_FORMAT));
}

void HerbCamera::Parameters::getSupportedPictureFormats(Vector<int> &formats)
{
    //ALOGE("(f:%s, l:%d) not implement", __FUNCTION__, __LINE__);
    const char *str = get(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS);
	if (str == NULL) {
		return;
	}
	Vector<String8> vstr8;
	int f;
	parseString8List(str, vstr8);
	for (size_t i = 0; i < vstr8.size(); i++) {
		f = pixelFormatForCameraFormat(vstr8.itemAt(i).string());
		if (f == IMAGE_FORMAT_UNKNOWN) continue;
		formats.push(f);
	}
}

const char *HerbCamera::Parameters::cameraFormatForPixelFormat(int pixel_format)
{
    //ALOGE("(f:%s, l:%d) not implement", __FUNCTION__, __LINE__);
    switch(pixel_format) {
    case IMAGE_FORMAT_NV16:       return CameraParameters::PIXEL_FORMAT_YUV422SP;
    case IMAGE_FORMAT_NV21:       return CameraParameters::PIXEL_FORMAT_YUV420SP;
    case IMAGE_FORMAT_YUY2:       return CameraParameters::PIXEL_FORMAT_YUV422I;
    case IMAGE_FORMAT_YV12:       return CameraParameters::PIXEL_FORMAT_YUV420P;
    case IMAGE_FORMAT_RGB_565:    return CameraParameters::PIXEL_FORMAT_RGB565;
    case IMAGE_FORMAT_JPEG:       return CameraParameters::PIXEL_FORMAT_JPEG;
    case IMAGE_FORMAT_BAYER_RGGB: return CameraParameters::PIXEL_FORMAT_BAYER_RGGB;
    default:                      return NULL;
    }
    return NULL;
}

int HerbCamera::Parameters::pixelFormatForCameraFormat(const char *format)
{
    //ALOGE("(f:%s, l:%d) not implement", __FUNCTION__, __LINE__);
    if (format == NULL) {
		return IMAGE_FORMAT_UNKNOWN;
    }
	if (strcmp(format, CameraParameters::PIXEL_FORMAT_YUV422SP) == 0) {
		return IMAGE_FORMAT_NV16;
    }
	if (strcmp(format, CameraParameters::PIXEL_FORMAT_YUV420SP) == 0) {
		return IMAGE_FORMAT_NV21;
    }
	if (strcmp(format, CameraParameters::PIXEL_FORMAT_YUV422I) == 0) {
		return IMAGE_FORMAT_YUY2;
    }
	if (strcmp(format, CameraParameters::PIXEL_FORMAT_YUV420P) == 0) {
		return IMAGE_FORMAT_YV12;
    }
	if (strcmp(format, CameraParameters::PIXEL_FORMAT_RGB565) == 0) {
		return IMAGE_FORMAT_RGB_565;
    }
	if (strcmp(format, CameraParameters::PIXEL_FORMAT_JPEG) == 0) {
		return IMAGE_FORMAT_JPEG;
    }
    return IMAGE_FORMAT_UNKNOWN;
}

void HerbCamera::Parameters::setRotation(int rotation)
{
	if (rotation != 0 && rotation != 90 && rotation != 180 && rotation != 270) {
		ALOGE("setRotation: Invalid rotation=%d", rotation);
		return;
	}
	char buffer[64];
	snprintf(buffer, 64, "%d", rotation);
	set(CameraParameters::KEY_ROTATION, buffer);
}

void HerbCamera::Parameters::setPreviewFlip(int flip)
{
    if (flip != PREVIEW_FLIP::NoFlip && flip != PREVIEW_FLIP::LeftRightFlip && flip != PREVIEW_FLIP::TopBottomFlip) {
        ALOGE("Invalid flip=%d", flip);
        return;
    }
    set(CameraParameters::KEY_AWEXTEND_PREVIEW_FLIP, flip);
}

int HerbCamera::Parameters::getPreviewFlip() const
{
    return getInt(CameraParameters::KEY_AWEXTEND_PREVIEW_FLIP, PREVIEW_FLIP::NoFlip);
}

void HerbCamera::Parameters::setPreviewRotation(int rotation)
{
	if (rotation != 0 && rotation != 90 && rotation != 180 && rotation != 270) {
		ALOGE("setPreviewRotation: Invalid rotation=%d", rotation);
		return;
	}
	set(CameraParameters::KEY_AWEXTEND_PREVIEW_ROTATION, rotation);
}

int HerbCamera::Parameters::getPreviewRotation() const
{
    return getInt(CameraParameters::KEY_AWEXTEND_PREVIEW_ROTATION, 0);
}

void HerbCamera::Parameters::setPictureMode(const char *mode)
{
    if(strcmp(mode, CameraParameters::AWEXTEND_PICTURE_MODE_NORMAL) 
        && strcmp(mode, CameraParameters::AWEXTEND_PICTURE_MODE_FAST)
        && strcmp(mode, CameraParameters::AWEXTEND_PICTURE_MODE_CONTINUOUS)
        && strcmp(mode, CameraParameters::AWEXTEND_PICTURE_MODE_CONTINUOUS_FAST))
    {
        ALOGE("(f:%s, l:%d) unsupport picture mode[%s]", __FUNCTION__, __LINE__, mode);
        return;
    }
    set(CameraParameters::KEY_AWEXTEND_PICTURE_MODE, mode);
}

const char *HerbCamera::Parameters::getPictureMode() const
{
    return get(CameraParameters::KEY_AWEXTEND_PICTURE_MODE);
}

void HerbCamera::Parameters::setPictureSizeMode(int mode)
{
    set(CameraParameters::KEY_AWEXTEND_PICTURE_SIZE_MODE, mode);
}

int HerbCamera::Parameters::getPictureSizeMode() const
{
    return getInt(CameraParameters::KEY_AWEXTEND_PICTURE_SIZE_MODE, CameraParameters::AWEXTEND_PICTURE_SIZE_MODE::KeepSourceSize);
}

void HerbCamera::Parameters::setGpsLatitude(double latitude)
{
	char buffer[64];
	snprintf(buffer, 64, "%f", latitude);
	set(CameraParameters::KEY_GPS_LATITUDE, buffer);
}

void HerbCamera::Parameters::setGpsLongitude(double longitude)
{
	char buffer[64];
	snprintf(buffer, 64, "%f", longitude);
	set(CameraParameters::KEY_GPS_LONGITUDE, buffer);
}

void HerbCamera::Parameters::setGpsAltitude(double altitude)
{
	char buffer[64];
	snprintf(buffer, 64, "%f", altitude);
	set(CameraParameters::KEY_GPS_LONGITUDE, buffer);
}

void HerbCamera::Parameters::setGpsTimestamp(long timestamp)
{
	char buffer[64];
	snprintf(buffer, 64, "%ld", timestamp);
	set(CameraParameters::KEY_GPS_LONGITUDE, buffer);
}

void HerbCamera::Parameters::setGpsProcessingMethod(const char *processing_method)
{
	set(CameraParameters::KEY_GPS_PROCESSING_METHOD, processing_method);
}

void HerbCamera::Parameters::removeGpsData()
{
	remove(CameraParameters::KEY_GPS_LATITUDE);
	remove(CameraParameters::KEY_GPS_LONGITUDE);
	remove(CameraParameters::KEY_GPS_ALTITUDE);
	remove(CameraParameters::KEY_GPS_TIMESTAMP);
	remove(CameraParameters::KEY_GPS_PROCESSING_METHOD);
}
const char *HerbCamera::Parameters::getWhiteBalance()
{
	return get(CameraParameters::KEY_WHITE_BALANCE);
}

void HerbCamera::Parameters::setWhiteBalance(const char *value)
{
	if (value == NULL) return;
	const char *oldValue = get(CameraParameters::KEY_WHITE_BALANCE);
	if (oldValue != NULL && !strcmp(value, oldValue)) return;
	set(CameraParameters::KEY_WHITE_BALANCE, value);
	set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK, CameraParameters::sFALSE);
}

void HerbCamera::Parameters::getSupportedWhiteBalance(Vector<String8>& whiteBalances)
{
	const char *values = get(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE);
	if (values == NULL) return;
	parseString8List(values, whiteBalances);
}

const char *HerbCamera::Parameters::getColorEffect()
{
	return get(CameraParameters::KEY_EFFECT);
}

void HerbCamera::Parameters::setColorEffect(const char *value)
{
	set(CameraParameters::KEY_EFFECT, value);
}

void HerbCamera::Parameters::getSupportedColorEffects(Vector<String8>& effects)
{
	const char *values = get(CameraParameters::KEY_SUPPORTED_EFFECTS);
	if (values == NULL) return;
	parseString8List(values, effects);
}

const char *HerbCamera::Parameters::getAntibanding()
{
	return get(CameraParameters::KEY_ANTIBANDING);
}

void HerbCamera::Parameters::setAntibanding(const char *antibanding)
{
	set(CameraParameters::KEY_ANTIBANDING, antibanding);
}

void HerbCamera::Parameters::getSupportedAntibanding(Vector<String8>& antibanding)
{
	const char *values = get(CameraParameters::KEY_SUPPORTED_ANTIBANDING);
	if (values == NULL) return;
	parseString8List(values, antibanding);
}

const char *HerbCamera::Parameters::getSceneMode()
{
	return get(CameraParameters::KEY_SCENE_MODE);
}

void HerbCamera::Parameters::setSceneMode(const char *value)
{
	set(CameraParameters::KEY_SCENE_MODE, value);
}

void HerbCamera::Parameters::getSupportedSceneModes(Vector<String8>& sceneModes)
{
	const char *values = get(CameraParameters::KEY_SUPPORTED_SCENE_MODES);
	if (values == NULL) return;
	parseString8List(values, sceneModes);
}

const char *HerbCamera::Parameters::getFlashMode()
{
	return get(CameraParameters::KEY_FLASH_MODE);
}

void HerbCamera::Parameters::setFlashMode(const char *value)
{
	set(CameraParameters::KEY_FLASH_MODE, value);
}

void HerbCamera::Parameters::getSupportedFlashModes(Vector<String8>& flashModes)
{
	const char *values = get(CameraParameters::KEY_SUPPORTED_FLASH_MODES);
	if (values == NULL) return;
	parseString8List(values, flashModes);
}

const char *HerbCamera::Parameters::getFocusMode()
{
	return get(CameraParameters::KEY_FOCUS_MODE);
}

void HerbCamera::Parameters::setFocusMode(const char *value)
{
	set(CameraParameters::KEY_FOCUS_MODE, value);
}

void HerbCamera::Parameters::getSupportedFocusModes(Vector<String8>& focusModes)
{
	const char *values = get(CameraParameters::KEY_SUPPORTED_FOCUS_MODES);
	if (values == NULL) return;
	parseString8List(values, focusModes);
}

float HerbCamera::Parameters::getFocalLength()
{
	return getFloat(CameraParameters::KEY_FOCAL_LENGTH);
}

float HerbCamera::Parameters::getHorizontalViewAngle()
{
	return getFloat(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE);
}

float HerbCamera::Parameters::getVerticalViewAngle()
{
	return getFloat(CameraParameters::KEY_VERTICAL_VIEW_ANGLE);
}

int HerbCamera::Parameters::getExposureCompensation()
{
	return getInt(CameraParameters::KEY_EXPOSURE_COMPENSATION, 0);
}

void HerbCamera::Parameters::setExposureCompensation(int value)
{
	set(CameraParameters::KEY_EXPOSURE_COMPENSATION, value);
}

int HerbCamera::Parameters::getMaxExposureCompensation()
{
	return getInt(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, 0);
}

int HerbCamera::Parameters::getMinExposureCompensation()
{
    return getInt(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, 0);
}

float HerbCamera::Parameters::getExposureCompensationStep()
{
	return getFloat(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, 0);
}

void HerbCamera::Parameters::setContrastValue(int value)
{
	set(CameraParameters::KEY_CID_CONTRAST, value);
}

int HerbCamera::Parameters::getContrastValue()
{
	return getInt(CameraParameters::KEY_CID_CONTRAST, 128);
}

void HerbCamera::Parameters::setBrightnessValue(int value)
{
	set(CameraParameters::KEY_CID_BRIGHTNESS, value);
}

int HerbCamera::Parameters::getBrightnessValue()
{
	return getInt(CameraParameters::KEY_CID_BRIGHTNESS, 128);
}


void HerbCamera::Parameters::setSaturationValue(int value)
{
	set(CameraParameters::KEY_CID_SATURATION, value);
}

int HerbCamera::Parameters::getSaturationValue()
{
	return getInt(CameraParameters::KEY_CID_SATURATION, 128);
}

void HerbCamera::Parameters::setHueValue(int value)
{
	set(CameraParameters::KEY_CID_HUE, value);
}

int HerbCamera::Parameters::getHueValue()
{
	return getInt(CameraParameters::KEY_CID_HUE, 90);
}

void HerbCamera::Parameters::setSharpnessValue(int value)
{
	set(CameraParameters::KEY_CID_SHARPNESS, value);
}

void HerbCamera::Parameters::setHflip(int value)
{
	set(CameraParameters::KEY_CID_HFLIP, value);
}

void HerbCamera::Parameters::setVflip(int value)
{
	set(CameraParameters::KEY_CID_VFLIP, value);
}
void HerbCamera::Parameters::setImageEffect(int effect)
{
	set(CameraParameters::KEY_CID_EFFECT, effect);
}

int HerbCamera::Parameters::getSharpnessValue()
{
	return getInt(CameraParameters::KEY_CID_SHARPNESS, 3);
}

void HerbCamera::Parameters::setPowerLineFrequencyValue(int value)
{
	set(CameraParameters::KEY_CID_POWER_LINE_FREQUENCY, value);
}

int HerbCamera::Parameters::getPowerLineFrequencyValue()
{
	return getInt(CameraParameters::KEY_CID_POWER_LINE_FREQUENCY, PowerLineFrequency::PLF_50HZ);
}

void HerbCamera::Parameters::setAutoExposureLock(bool toggle)
{
	set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK, toggle ? CameraParameters::sTRUE : CameraParameters::sFALSE);
}

bool HerbCamera::Parameters::getAutoExposureLock()
{
	const char *str = get(CameraParameters::KEY_AUTO_EXPOSURE_LOCK);
	return (strcmp(CameraParameters::sTRUE, str) == 0) ? true : false;
}

bool HerbCamera::Parameters::isAutoExposureLockSupported()
{
	const char *str = get(CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED);
	return (strcmp(CameraParameters::sTRUE, str) == 0) ? true : false;
}

void HerbCamera::Parameters::setAutoWhiteBalanceLock(bool toggle)
{
	set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK, toggle ? CameraParameters::sTRUE : CameraParameters::sFALSE);
}

bool HerbCamera::Parameters::getAutoWhiteBalanceLock()
{
	const char *str = get(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK);
	return (strcmp(CameraParameters::sTRUE, str) == 0) ? true : false;
}

bool HerbCamera::Parameters::isAutoWhiteBalanceLockSupported()
{
	const char *str = get(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED);
	return (strcmp(CameraParameters::sTRUE, str) ==0) ? true : false;
}

int HerbCamera::Parameters::getZoom()
{
	return getInt(CameraParameters::KEY_ZOOM, 0);
}

void HerbCamera::Parameters::setZoom(int value)
{
	set(CameraParameters::KEY_ZOOM, value);
}

bool HerbCamera::Parameters::isZoomSupported()
{
	const char *str = get(CameraParameters::KEY_ZOOM_SUPPORTED);
	return (strcmp(CameraParameters::sTRUE, str) == 0) ? true : false;
}

int HerbCamera::Parameters::getMaxZoom()
{
	return getInt(CameraParameters::KEY_MAX_ZOOM, 0);
}

void HerbCamera::Parameters::getZoomRatios(Vector<int>& ratios)
{
	const char *values = get(CameraParameters::KEY_ZOOM_RATIOS);
	if (values == NULL) return;
	parseIntList(values, ratios);
}

bool HerbCamera::Parameters::isSmoothZoomSupported()
{
	const char *str = get(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED);
	return (strcmp(CameraParameters::sTRUE, str) == 0) ? true : false;
}

void HerbCamera::Parameters::getFocusDistances(float *distances)
{
	if (distances == NULL) return;
	const char *dis = get(CameraParameters::KEY_FOCUS_DISTANCES);
	if (dis == NULL) return;
	parseFloat(dis, distances, 3);
}

int HerbCamera::Parameters::getMaxNumFocusAreas()
{
	return getInt(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS, 0);
}

void HerbCamera::Parameters::getFocusAreas(Vector<Area> &areas)
{
	const char *str = get(CameraParameters::KEY_FOCUS_AREAS);
	if (str == NULL) return;
	parseAreaList(str, areas);
}

void HerbCamera::Parameters::setFocusAreas(Vector<Area> &areas)
{
	set(CameraParameters::KEY_FOCUS_AREAS, areas);
}

int HerbCamera::Parameters::getMaxNumMeteringAreas()
{
	return getInt(CameraParameters::KEY_MAX_NUM_METERING_AREAS, 0);
}

void HerbCamera::Parameters::getMeteringAreas(Vector<Area> &areas)
{
	const char *str = get(CameraParameters::KEY_METERING_AREAS);
	if (str == NULL) return;
	parseAreaList(str, areas);
}

void HerbCamera::Parameters::setMeteringAreas(Vector<Area> &areas)
{
	set(CameraParameters::KEY_METERING_AREAS, areas);
}

int HerbCamera::Parameters::getMaxNumDetectedFaces()
{
	return getInt(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW, 0);
}

void HerbCamera::Parameters::setRecordingHint(bool hint)
{
	set(CameraParameters::KEY_RECORDING_HINT, hint ? CameraParameters::sTRUE : CameraParameters::sFALSE);
}

bool HerbCamera::Parameters::isVideoSnapshotSupported()
{
	const char *str = get(CameraParameters::KEY_VIDEO_SNAPSHOT_SUPPORTED);
	return (strcmp(CameraParameters::sTRUE, str) == 0) ? true : false;
}

void HerbCamera::Parameters::setVideoStabilization(bool toggle)
{
	set(CameraParameters::KEY_VIDEO_STABILIZATION, toggle ? CameraParameters::sTRUE : CameraParameters::sFALSE);
}

bool HerbCamera::Parameters::getVideoStabilization()
{
	const char *str = get(CameraParameters::KEY_VIDEO_STABILIZATION);
	return (strcmp(CameraParameters::sTRUE, str) == 0)? true : false;
}

bool HerbCamera::Parameters::isVideoStabilizationSupported()
{
	const char *str = get(CameraParameters::KEY_VIDEO_STABILIZATION_SUPPORTED);
	return (strcmp(CameraParameters::sTRUE, str) == 0) ? true : false;
}

void HerbCamera::Parameters::setVideoCaptureBufferNum(int number)
{
	set(CameraParameters::KEY_AWEXTEND_VIDEO_CAPTURE_BUFFER_NUM, number);
}

int HerbCamera::Parameters::getVideoCaptureBufferNum()
{
    return getInt(CameraParameters::KEY_AWEXTEND_VIDEO_CAPTURE_BUFFER_NUM, 0);
}

void HerbCamera::Parameters::setContinuousPictureNumber(int number)
{
	set(CameraParameters::KEY_AWEXTEND_CONTINUOUS_PICTURE_NUMBER, number);
}

int HerbCamera::Parameters::getContinuousPictureNumber()
{
    return getInt(CameraParameters::KEY_AWEXTEND_CONTINUOUS_PICTURE_NUMBER, 0);
}

void HerbCamera::Parameters::setContinuousPictureIntervalMs(int timeMs)
{
	set(CameraParameters::KEY_AWEXTEND_CONTINUOUS_PICTURE_INTERVAL, timeMs);
}

int HerbCamera::Parameters::getContinuousPictureIntervalMs()
{
    return getInt(CameraParameters::KEY_AWEXTEND_CONTINUOUS_PICTURE_INTERVAL, 0);
}

}; // namespace android

