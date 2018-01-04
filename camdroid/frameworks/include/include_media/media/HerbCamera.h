/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
 /**
 * Information about a camera
 */
#ifndef __HERB_CAMERA_H__
#define __HERB_CAMERA_H__

//#include "jni.h"
//#include "JNIHelp.h"
//#include "android_runtime/AndroidRuntime.h"

//#include <cutils/properties.h>

#include <utils/Mutex.h>
#include <utils/Vector.h>
#include <utils/KeyedVector.h>
#include <utils/String8.h>

#include <ui/Rect.h>
//#include <gui/SurfaceTexture.h>
//#include <gui/Surface.h>
#include <camera/Camera.h>
//#include <binder/IMemory.h>
#include <camera/CameraParameters.h>
#include <include_media/media/MediaCallbackDispatcher.h>
#include <include_adas/Adas_interface.h>	/* for ADAS */

namespace android {

class CMediaMemory : public RefBase
{
public:
    CMediaMemory(int size);
    ~CMediaMemory();

    void* getPointer() const;
    int getSize() const;
private:
    void    *pMem;
    int     mSize;
};

class HerbCameraContext;
class HerbCamera : public RefBase
{
private:
    class EventHandler;
public:
    class ShutterCallback;
    class PictureCallback;
    class PreviewCallback;
    class AutoFocusCallback;
    class AutoFocusMoveCallback;
    class OnZoomChangeListener;
    class FaceDetectionListener;
    class ErrorCallback;
	class Area;
	class AWMoveDetectionListener;	/* MOTION_DETECTION_ENABLE */
	class AdasDetectionListener;	/* for ADAS */
    class QrDecodeListener;

private:
    static const String8 TAG;   // = "Camera";
    void            *mNativeContext;
    EventHandler    *mpEventHandler;
    ShutterCallback *mpShutterCallback;
    PictureCallback *mpRawImageCallback;
    PictureCallback *mpJpegCallback;
    PreviewCallback *mpPreviewCallback;
    PictureCallback *mpPostviewCallback;
    AutoFocusCallback       *mpAutoFocusCallback;
    AutoFocusMoveCallback   *mpAutoFocusMoveCallback;
    OnZoomChangeListener    *mpZoomListener;
    FaceDetectionListener   *mpFaceListener;
    ErrorCallback   *mpErrorCallback;
	AWMoveDetectionListener *mpAWMDListener;	/* MOTION_DETECTION_ENABLE */
#ifdef SUPPORT_TVIN
	TVinListener *mpTVinListener;
#endif
    bool mOneShot;
    bool mWithBuffer;
    bool mFaceDetectionRunning;
	bool mAWMDrunning;	/* MOTION_DETECTION_ENABLE */
    Mutex mAutoFocusCallbackLock;
	/* for ADAS start */
	AdasDetectionListener *mpAdasListener;
	bool mAdasDetectionRunning;
	/* for ADAS end */
    QrDecodeListener *mpQrDecodeListener;

public:
    static sp<Camera> get_native_camera(HerbCamera* pC, HerbCameraContext** pContext);
    /**
     * Returns the number of physical cameras available on this device.
     */
    static int getNumberOfCameras();

    class CameraInfo
    {
    public:
        /**
         * The direction that the camera faces. It should be
         * CAMERA_FACING_BACK or CAMERA_FACING_FRONT.
         */     
        enum
        {
            /**
             * The facing of the camera is opposite to that of the screen.
             */
            CAMERA_FACING_BACK = 0,
            /**
             * The facing of the camera is the same as that of the screen.
             */
            CAMERA_FACING_FRONT = 1,
        };
        int facing;  //enum CameraFacingDirection

        /**
         * <p>The orientation of the camera image. The value is the angle that the
         * camera image needs to be rotated clockwise so it shows correctly on
         * the display in its natural orientation. It should be 0, 90, 180, or 270.</p>
         *
         * <p>For example, suppose a device has a naturally tall screen. The
         * back-facing camera sensor is mounted in landscape. You are looking at
         * the screen. If the top side of the camera sensor is aligned with the
         * right edge of the screen in natural orientation, the value should be
         * 90. If the top side of a front-facing camera sensor is aligned with
         * the right of the screen, the value should be 270.</p>
         *
         * @see #setDisplayOrientation(int)
         * @see Parameters#setRotation(int)
         * @see Parameters#setPreviewSize(int, int)
         * @see Parameters#setPictureSize(int, int)
         * @see Parameters#setJpegThumbnailSize(int, int)
         */
        int orientation;

        /**
         * <p>Whether the shutter sound can be disabled.</p>
         *
         * <p>On some devices, the camera shutter sound cannot be turned off
         * through {@link #enableShutterSound enableShutterSound}. This field
         * can be used to determine whether a call to disable the shutter sound
         * will succeed.</p>
         *
         * <p>If this field is set to true, then a call of
         * {@code enableShutterSound(false)} will be successful. If set to
         * false, then that call will fail, and the shutter sound will be played
         * when {@link Camera#takePicture takePicture} is called.</p>
         */
        bool canDisableShutterSound;
    };
    
    /**
         * Returns the information about a particular camera.
         * If {@link #getNumberOfCameras()} returns N, the valid id is 0 to N-1.
         */
    static void getCameraInfo(int cameraId, CameraInfo *pCameraInfo);
    
    /**
     * Creates a new Camera object to access a particular hardware camera. If
     * the same camera is opened by other applications, this will throw a
     * RuntimeException.
     *
     * <p>You must call {@link #release()} when you are done using the camera,
     * otherwise it will remain locked and be unavailable to other applications.
     *
     * <p>Your application should only have one Camera object active at a time
     * for a particular hardware camera.
     *
     * <p>Callbacks from other methods are delivered to the event loop of the
     * thread which called open().  If this thread has no event loop, then
     * callbacks are delivered to the main application event loop.  If there
     * is no main application event loop, callbacks are not delivered.
     *
     * <p class="caution"><b>Caution:</b> On some devices, this method may
     * take a long time to complete.  It is best to call this method from a
     * worker thread (possibly using {@link android.os.AsyncTask}) to avoid
     * blocking the main application UI thread.
     *
     * @param cameraId the hardware camera to access, between 0 and
     *     {@link #getNumberOfCameras()}-1.
     * @return a new Camera object, connected, locked and ready for use.
     * @throws RuntimeException if opening the camera fails (for example, if the
     *     camera is in use by another process or device policy manager has
     *     disabled the camera).
     * @see android.app.admin.DevicePolicyManager#getCameraDisabled(android.content.ComponentName)
     */
    static HerbCamera* open(int cameraId);

    HerbCamera(int cameraId);
    ~HerbCamera();

    /**
     * Disconnects and releases the Camera object resources.
     *
     * <p>You must call this as soon as you're done with the Camera object.</p>
     */
    void release();

    /**
     * Unlocks the camera to allow another process to access it.
     * Normally, the camera is locked to the process with an active Camera
     * object until {@link #release()} is called.  To allow rapid handoff
     * between processes, you can call this method to release the camera
     * temporarily for another process to use; once the other process is done
     * you can call {@link #reconnect()} to reclaim the camera.
     *
     * <p>This must be done before calling
     * {@link android.media.MediaRecorder#setCamera(Camera)}. This cannot be
     * called after recording starts.
     *
     * <p>If you are not recording video, you probably do not need this method.
     *
     * @throws RuntimeException if the camera cannot be unlocked.
     */
    status_t unlock();

    /**
     * Re-locks the camera to prevent other processes from accessing it.
     * Camera objects are locked by default unless {@link #unlock()} is
     * called.  Normally {@link #reconnect()} is used instead.
     *
     * <p>Since API level 14, camera is automatically locked for applications in
     * {@link android.media.MediaRecorder#start()}. Applications can use the
     * camera (ex: zoom) after recording starts. There is no need to call this
     * after recording starts or stops.
     *
     * <p>If you are not recording video, you probably do not need this method.
     *
     * @throws RuntimeException if the camera cannot be re-locked (for
     *     example, if the camera is still in use by another process).
     */
    status_t lock();

    /**
     * Reconnects to the camera service after another process used it.
     * After {@link #unlock()} is called, another process may use the
     * camera; when the process is done, you must reconnect to the camera,
     * which will re-acquire the lock and allow you to continue using the
     * camera.
     *
     * <p>Since API level 14, camera is automatically locked for applications in
     * {@link android.media.MediaRecorder#start()}. Applications can use the
     * camera (ex: zoom) after recording starts. There is no need to call this
     * after recording starts or stops.
     *
     * <p>If you are not recording video, you probably do not need this method.
     *
     * @throws IOException if a connection cannot be re-established (for
     *     example, if the camera is still in use by another process).
     */
    status_t reconnect();

    /**
     * Sets the {@link Surface} to be used for live preview.
     * Either a surface or surface texture is necessary for preview, and
     * preview is necessary to take pictures.  The same surface can be re-set
     * without harm.  Setting a preview surface will un-set any preview surface
     * texture that was set via {@link #setPreviewTexture}.
     *
     * <p>The {@link SurfaceHolder} must already contain a surface when this
     * method is called.  If you are using {@link android.view.SurfaceView},
     * you will need to register a {@link SurfaceHolder.Callback} with
     * {@link SurfaceHolder#addCallback(SurfaceHolder.Callback)} and wait for
     * {@link SurfaceHolder.Callback#surfaceCreated(SurfaceHolder)} before
     * calling setPreviewDisplay() or starting preview.
     *
     * <p>This method must be called before {@link #startPreview()}.  The
     * one exception is that if the preview surface is not set (or set to null)
     * before startPreview() is called, then this method may be called once
     * with a non-null parameter to set the preview surface.  (This allows
     * camera setup and surface creation to happen in parallel, saving time.)
     * The preview surface may not otherwise change while preview is running.
     *
     * @param holder containing the Surface on which to place the preview,
     *     or null to remove the preview surface
     * @throws IOException if the method fails (for example, if the surface
     *     is unavailable or unsuitable).
     */
    status_t setPreviewDisplay(int hlay);

    /**
     * Callback interface used to deliver copies of preview frames as
     * they are displayed.
     *
     * @see #setPreviewCallback(Camera.PreviewCallback)
     * @see #setOneShotPreviewCallback(Camera.PreviewCallback)
     * @see #setPreviewCallbackWithBuffer(Camera.PreviewCallback)
     * @see #startPreview()
     */
    class PreviewCallback
    {
    public:
    	PreviewCallback(){}
    	virtual ~PreviewCallback(){}
        virtual void onPreviewFrame(void *data, int size, HerbCamera* pCamera) = 0;
    };

    /**
     * Starts capturing and drawing preview frames to the screen.
     * Preview will not actually start until a surface is supplied
     * with {@link #setPreviewDisplay(SurfaceHolder)} or
     * {@link #setPreviewTexture(SurfaceTexture)}.
     *
     * <p>If {@link #setPreviewCallback(Camera.PreviewCallback)},
     * {@link #setOneShotPreviewCallback(Camera.PreviewCallback)}, or
     * {@link #setPreviewCallbackWithBuffer(Camera.PreviewCallback)} were
     * called, {@link Camera.PreviewCallback#onPreviewFrame(byte[], Camera)}
     * will be called when preview data becomes available.
     */
    status_t startPreview();

    /**
     * Stops capturing and drawing preview frames to the surface, and
     * resets the camera for a future call to {@link #startPreview()}.
     */
    void stopPreview();

    /**
     * Return current preview state.
     *
     * FIXME: Unhide before release
     * @hide
     */
    bool previewEnabled();

    /**
     * <p>Installs a callback to be invoked for every preview frame in addition
     * to displaying them on the screen.  The callback will be repeatedly called
     * for as long as preview is active.  This method can be called at any time,
     * even while preview is live.  Any other preview callbacks are
     * overridden.</p>
     *
     * <p>If you are using the preview data to create video or still images,
     * strongly consider using {@link android.media.MediaActionSound} to
     * properly indicate image capture or recording start/stop to the user.</p>
     *
     * @param cb a callback object that receives a copy of each preview frame,
     *     or null to stop receiving callbacks.
     * @see android.media.MediaActionSound
     */
    void setPreviewCallback(PreviewCallback *pCb);
    
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
    void setOneShotPreviewCallback(PreviewCallback *pCb);
private:
    void setHasPreviewCallback(bool installed, bool manualBuffer);
public:
    /**
     * <p>Installs a callback to be invoked for every preview frame, using
     * buffers supplied with {@link #addCallbackBuffer(byte[])}, in addition to
     * displaying them on the screen.  The callback will be repeatedly called
     * for as long as preview is active and buffers are available.  Any other
     * preview callbacks are overridden.</p>
     *
     * <p>The purpose of this method is to improve preview efficiency and frame
     * rate by allowing preview frame memory reuse.  You must call
     * {@link #addCallbackBuffer(byte[])} at some point -- before or after
     * calling this method -- or no callbacks will received.</p>
     *
     * <p>The buffer queue will be cleared if this method is called with a null
     * callback, {@link #setPreviewCallback(Camera.PreviewCallback)} is called,
     * or {@link #setOneShotPreviewCallback(Camera.PreviewCallback)} is
     * called.</p>
     *
     * <p>If you are using the preview data to create video or still images,
     * strongly consider using {@link android.media.MediaActionSound} to
     * properly indicate image capture or recording start/stop to the user.</p>
     *
     * @param cb a callback object that receives a copy of the preview frame,
     *     or null to stop receiving callbacks and clear the buffer queue.
     * @see #addCallbackBuffer(byte[])
     * @see android.media.MediaActionSound
     */
    void setPreviewCallbackWithBuffer(PreviewCallback *pCb);
    /**
     * Adds a pre-allocated buffer to the preview callback buffer queue.
     * Applications can add one or more buffers to the queue. When a preview
     * frame arrives and there is still at least one available buffer, the
     * buffer will be used and removed from the queue. Then preview callback is
     * invoked with the buffer. If a frame arrives and there is no buffer left,
     * the frame is discarded. Applications should add buffers back when they
     * finish processing the data in them.
     *
     * <p>For formats besides YV12, the size of the buffer is determined by
     * multiplying the preview image width, height, and bytes per pixel. The
     * width and height can be read from
     * {@link Camera.Parameters#getPreviewSize()}. Bytes per pixel can be
     * computed from {@link android.graphics.ImageFormat#getBitsPerPixel(int)} /
     * 8, using the image format from
     * {@link Camera.Parameters#getPreviewFormat()}.
     *
     * <p>If using the {@link android.graphics.ImageFormat#YV12} format, the
     * size can be calculated using the equations listed in
     * {@link Camera.Parameters#setPreviewFormat}.
     *
     * <p>This method is only necessary when
     * {@link #setPreviewCallbackWithBuffer(PreviewCallback)} is used. When
     * {@link #setPreviewCallback(PreviewCallback)} or
     * {@link #setOneShotPreviewCallback(PreviewCallback)} are used, buffers
     * are automatically allocated. When a supplied buffer is too small to
     * hold the preview frame data, preview callback will return null and
     * the buffer will be removed from the buffer queue.
     *
     * @param callbackBuffer the buffer to add to the queue. The size of the
     *   buffer must match the values described above.
     * @see #setPreviewCallbackWithBuffer(PreviewCallback)
     */
    void addCallbackBuffer(sp<CMediaMemory> &callbackBuffer);
    /**
     * Adds a pre-allocated buffer to the raw image callback buffer queue.
     * Applications can add one or more buffers to the queue. When a raw image
     * frame arrives and there is still at least one available buffer, the
     * buffer will be used to hold the raw image data and removed from the
     * queue. Then raw image callback is invoked with the buffer. If a raw
     * image frame arrives but there is no buffer left, the frame is
     * discarded. Applications should add buffers back when they finish
     * processing the data in them by calling this method again in order
     * to avoid running out of raw image callback buffers.
     *
     * <p>The size of the buffer is determined by multiplying the raw image
     * width, height, and bytes per pixel. The width and height can be
     * read from {@link Camera.Parameters#getPictureSize()}. Bytes per pixel
     * can be computed from
     * {@link android.graphics.ImageFormat#getBitsPerPixel(int)} / 8,
     * using the image format from {@link Camera.Parameters#getPreviewFormat()}.
     *
     * <p>This method is only necessary when the PictureCallbck for raw image
     * is used while calling {@link #takePicture(Camera.ShutterCallback,
     * Camera.PictureCallback, Camera.PictureCallback, Camera.PictureCallback)}.
     *
     * <p>Please note that by calling this method, the mode for
     * application-managed callback buffers is triggered. If this method has
     * never been called, null will be returned by the raw image callback since
     * there is no image callback buffer available. Furthermore, When a supplied
     * buffer is too small to hold the raw image data, raw image callback will
     * return null and the buffer will be removed from the buffer queue.
     *
     * @param callbackBuffer the buffer to add to the raw image callback buffer
     *     queue. The size should be width * height * (bits per pixel) / 8. An
     *     null callbackBuffer will be ignored and won't be added to the queue.
     *
     * @see #takePicture(Camera.ShutterCallback,
     * Camera.PictureCallback, Camera.PictureCallback, Camera.PictureCallback)}.
     *
     * {@hide}
     */
    void addRawImageCallbackBuffer(sp<CMediaMemory> &callbackBuffer);
private:
    void addCallbackBuffer(sp<CMediaMemory> &callbackBuffer, int msgType);
    void _addCallbackBuffer(sp<CMediaMemory> &callbackBuffer, int msgType);
private:
    class EventHandler : public MediaCallbackDispatcher
    {
    private:
        HerbCamera *mpCamera;
    public:
        EventHandler(HerbCamera *pC);
        virtual void handleMessage(const MediaCallbackMessage &msg);
    };
public:
//    static void postEventFromNative(Object camera_ref,
//                                            int what, int arg1, int arg2, Object obj)
    //static void postEventFromNative(wp<HerbCamera>& camera_ref, int what, int arg1, int arg2, void *obj=NULL);
    static void postEventFromNative(HerbCamera* pC, int what, int arg1, int arg2, void *obj=NULL);
    /**
     * Callback interface used to notify on completion of camera auto focus.
     *
     * <p>Devices that do not support auto-focus will receive a "fake"
     * callback to this interface. If your application needs auto-focus and
     * should not be installed on devices <em>without</em> auto-focus, you must
     * declare that your app uses the
     * {@code android.hardware.camera.autofocus} feature, in the
     * <a href="{@docRoot}guide/topics/manifest/uses-feature-element.html">&lt;uses-feature></a>
     * manifest element.</p>
     *
     * @see #autoFocus(AutoFocusCallback)
     */
    class AutoFocusCallback
    {
    public:
		AutoFocusCallback(){}
		virtual ~AutoFocusCallback(){}
        /**
         * Called when the camera auto focus completes.  If the camera
         * does not support auto-focus and autoFocus is called,
         * onAutoFocus will be called immediately with a fake value of
         * <code>success</code> set to <code>true</code>.
         *
         * The auto-focus routine does not lock auto-exposure and auto-white
         * balance after it completes.
         *
         * @param success true if focus was successful, false if otherwise
         * @param camera  the Camera service object
         * @see android.hardware.Camera.Parameters#setAutoExposureLock(boolean)
         * @see android.hardware.Camera.Parameters#setAutoWhiteBalanceLock(boolean)
         */
        virtual void onAutoFocus(bool success, HerbCamera* pCamera) = 0;
    };

    /**
     * Starts camera auto-focus and registers a callback function to run when
     * the camera is focused.  This method is only valid when preview is active
     * (between {@link #startPreview()} and before {@link #stopPreview()}).
     *
     * <p>Callers should check
     * {@link android.hardware.Camera.Parameters#getFocusMode()} to determine if
     * this method should be called. If the camera does not support auto-focus,
     * it is a no-op and {@link AutoFocusCallback#onAutoFocus(boolean, Camera)}
     * callback will be called immediately.
     *
     * <p>If your application should not be installed
     * on devices without auto-focus, you must declare that your application
     * uses auto-focus with the
     * <a href="{@docRoot}guide/topics/manifest/uses-feature-element.html">&lt;uses-feature></a>
     * manifest element.</p>
     *
     * <p>If the current flash mode is not
     * {@link android.hardware.Camera.Parameters#FLASH_MODE_OFF}, flash may be
     * fired during auto-focus, depending on the driver and camera hardware.<p>
     *
     * <p>Auto-exposure lock {@link android.hardware.Camera.Parameters#getAutoExposureLock()}
     * and auto-white balance locks {@link android.hardware.Camera.Parameters#getAutoWhiteBalanceLock()}
     * do not change during and after autofocus. But auto-focus routine may stop
     * auto-exposure and auto-white balance transiently during focusing.
     *
     * <p>Stopping preview with {@link #stopPreview()}, or triggering still
     * image capture with {@link #takePicture(Camera.ShutterCallback,
     * Camera.PictureCallback, Camera.PictureCallback)}, will not change the
     * the focus position. Applications must call cancelAutoFocus to reset the
     * focus.</p>
     *
     * <p>If autofocus is successful, consider using
     * {@link android.media.MediaActionSound} to properly play back an autofocus
     * success sound to the user.</p>
     *
     * @param cb the callback to run
     * @see #cancelAutoFocus()
     * @see android.hardware.Camera.Parameters#setAutoExposureLock(boolean)
     * @see android.hardware.Camera.Parameters#setAutoWhiteBalanceLock(boolean)
     * @see android.media.MediaActionSound
     */
    status_t autoFocus(AutoFocusCallback *pCb);

    /**
     * Cancels any auto-focus function in progress.
     * Whether or not auto-focus is currently in progress,
     * this function will return the focus position to the default.
     * If the camera does not support auto-focus, this is a no-op.
     *
     * @see #autoFocus(Camera.AutoFocusCallback)
     */
    status_t cancelAutoFocus();

    /**
     * Callback interface used to notify on auto focus start and stop.
     *
     * <p>This is only supported in continuous autofocus modes -- {@link
     * Parameters#FOCUS_MODE_CONTINUOUS_VIDEO} and {@link
     * Parameters#FOCUS_MODE_CONTINUOUS_PICTURE}. Applications can show
     * autofocus animation based on this.</p>
     */
    class AutoFocusMoveCallback
    {
    public:
		AutoFocusMoveCallback(){}
		virtual ~AutoFocusMoveCallback(){}
        /**
         * Called when the camera auto focus starts or stops.
         *
         * @param start true if focus starts to move, false if focus stops to move
         * @param camera the Camera service object
         */
        virtual void onAutoFocusMoving(bool start, HerbCamera* pCamera) = 0;
    };

    /**
     * Sets camera auto-focus move callback.
     *
     * @param cb the callback to run
     */
    status_t setAutoFocusMoveCallback(AutoFocusMoveCallback *pCb);
private:
    status_t enableFocusMoveCallback(int enable);
public:
    /**
     * Callback interface used to signal the moment of actual image capture.
     *
     * @see #takePicture(ShutterCallback, PictureCallback, PictureCallback, PictureCallback)
     */
    class ShutterCallback
    {
    public:
		ShutterCallback(){}
		virtual ~ShutterCallback(){}
        /**
         * Called as near as possible to the moment when a photo is captured
         * from the sensor.  This is a good opportunity to play a shutter sound
         * or give other feedback of camera operation.  This may be some time
         * after the photo was triggered, but some time before the actual data
         * is available.
         */
        virtual void onShutter() = 0;
    };

    /**
     * Callback interface used to supply image data from a photo capture.
     *
     * @see #takePicture(ShutterCallback, PictureCallback, PictureCallback, PictureCallback)
     */
    class PictureCallback 
    {
    public:
		PictureCallback(){}
		virtual ~PictureCallback(){}
        /**
         * Called when image data is available after a picture is taken.
         * The format of the data depends on the context of the callback
         * and {@link Camera.Parameters} settings.
         *
         * @param data   a byte array of the picture data
         * @param camera the Camera service object
         */
        virtual void onPictureTaken(void *data, int size, HerbCamera* pCamera) = 0;
    };

    /**
     * Equivalent to takePicture(shutter, raw, null, jpeg).
     *
     * @see #takePicture(ShutterCallback, PictureCallback, PictureCallback, PictureCallback)
     */
    status_t takePicture(ShutterCallback *pShutter, PictureCallback *pRaw,
            PictureCallback *pJpeg);

    /**
     * Triggers an asynchronous image capture. The camera service will initiate
     * a series of callbacks to the application as the image capture progresses.
     * The shutter callback occurs after the image is captured. This can be used
     * to trigger a sound to let the user know that image has been captured. The
     * raw callback occurs when the raw image data is available (NOTE: the data
     * will be null if there is no raw image callback buffer available or the
     * raw image callback buffer is not large enough to hold the raw image).
     * The postview callback occurs when a scaled, fully processed postview
     * image is available (NOTE: not all hardware supports this). The jpeg
     * callback occurs when the compressed image is available. If the
     * application does not need a particular callback, a null can be passed
     * instead of a callback method.
     *
     * <p>This method is only valid when preview is active (after
     * {@link #startPreview()}).  Preview will be stopped after the image is
     * taken; callers must call {@link #startPreview()} again if they want to
     * re-start preview or take more pictures. This should not be called between
     * {@link android.media.MediaRecorder#start()} and
     * {@link android.media.MediaRecorder#stop()}.
     *
     * <p>After calling this method, you must not call {@link #startPreview()}
     * or take another picture until the JPEG callback has returned.
     *
     * @param shutter   the callback for image capture moment, or null
     * @param raw       the callback for raw (uncompressed) image data, or null
     * @param postview  callback with postview image data, may be null
     * @param jpeg      the callback for JPEG image data, or null
     */
    status_t takePicture(ShutterCallback *pShutter, PictureCallback *pRaw,
            PictureCallback *pPostview, PictureCallback *pJpeg);
    /**
     * Zooms to the requested value smoothly. The driver will notify {@link
     * OnZoomChangeListener} of the zoom value and whether zoom is stopped at
     * the time. For example, suppose the current zoom is 0 and startSmoothZoom
     * is called with value 3. The
     * {@link Camera.OnZoomChangeListener#onZoomChange(int, boolean, Camera)}
     * method will be called three times with zoom values 1, 2, and 3.
     * Applications can call {@link #stopSmoothZoom} to stop the zoom earlier.
     * Applications should not call startSmoothZoom again or change the zoom
     * value before zoom stops. If the supplied zoom value equals to the current
     * zoom value, no zoom callback will be generated. This method is supported
     * if {@link android.hardware.Camera.Parameters#isSmoothZoomSupported}
     * returns true.
     *
     * @param value zoom value. The valid range is 0 to {@link
     *              android.hardware.Camera.Parameters#getMaxZoom}.
     * @throws IllegalArgumentException if the zoom value is invalid.
     * @throws RuntimeException if the method fails.
     * @see #setZoomChangeListener(OnZoomChangeListener)
     */
    status_t startSmoothZoom(int value);

    /**
     * Stops the smooth zoom. Applications should wait for the {@link
     * OnZoomChangeListener} to know when the zoom is actually stopped. This
     * method is supported if {@link
     * android.hardware.Camera.Parameters#isSmoothZoomSupported} is true.
     *
     * @throws RuntimeException if the method fails.
     */
    status_t stopSmoothZoom();

    /**
     * Set the clockwise rotation of preview display in degrees. This affects
     * the preview frames and the picture displayed after snapshot. This method
     * is useful for portrait mode applications. Note that preview display of
     * front-facing cameras is flipped horizontally before the rotation, that
     * is, the image is reflected along the central vertical axis of the camera
     * sensor. So the users can see themselves as looking into a mirror.
     *
     * <p>This does not affect the order of byte array passed in {@link
     * PreviewCallback#onPreviewFrame}, JPEG pictures, or recorded videos. This
     * method is not allowed to be called during preview.
     *
     * <p>If you want to make the camera image show in the same orientation as
     * the display, you can use the following code.
     * <pre>
     * public static void setCameraDisplayOrientation(Activity activity,
     *         int cameraId, android.hardware.Camera camera) {
     *     android.hardware.Camera.CameraInfo info =
     *             new android.hardware.Camera.CameraInfo();
     *     android.hardware.Camera.getCameraInfo(cameraId, info);
     *     int rotation = activity.getWindowManager().getDefaultDisplay()
     *             .getRotation();
     *     int degrees = 0;
     *     switch (rotation) {
     *         case Surface.ROTATION_0: degrees = 0; break;
     *         case Surface.ROTATION_90: degrees = 90; break;
     *         case Surface.ROTATION_180: degrees = 180; break;
     *         case Surface.ROTATION_270: degrees = 270; break;
     *     }
     *
     *     int result;
     *     if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
     *         result = (info.orientation + degrees) % 360;
     *         result = (360 - result) % 360;  // compensate the mirror
     *     } else {  // back-facing
     *         result = (info.orientation - degrees + 360) % 360;
     *     }
     *     camera.setDisplayOrientation(result);
     * }
     * </pre>
     *
     * <p>Starting from API level 14, this method can be called when preview is
     * active.
     *
     * @param degrees the angle that the picture will be rotated clockwise.
     *                Valid values are 0, 90, 180, and 270. The starting
     *                position is 0 (landscape).
     * @see #setPreviewDisplay(SurfaceHolder)
     */
    status_t setDisplayOrientation(int degrees);

    /**
     * <p>Enable or disable the default shutter sound when taking a picture.</p>
     *
     * <p>By default, the camera plays the system-defined camera shutter sound
     * when {@link #takePicture} is called. Using this method, the shutter sound
     * can be disabled. It is strongly recommended that an alternative shutter
     * sound is played in the {@link ShutterCallback} when the system shutter
     * sound is disabled.</p>
     *
     * <p>Note that devices may not always allow disabling the camera shutter
     * sound. If the shutter sound state cannot be set to the desired value,
     * this method will return false. {@link CameraInfo#canDisableShutterSound}
     * can be used to determine whether the device will allow the shutter sound
     * to be disabled.</p>
     *
     * @param enabled whether the camera should play the system shutter sound
     *                when {@link #takePicture takePicture} is called.
     * @return {@code true} if the shutter sound state was successfully
     *         changed. {@code false} if the shutter sound state could not be
     *         changed. {@code true} is also returned if shutter sound playback
     *         is already set to the requested state.
     * @see #takePicture
     * @see CameraInfo#canDisableShutterSound
     * @see ShutterCallback
     */
    bool enableShutterSound(bool enabled);

    /**
     * Callback interface for zoom changes during a smooth zoom operation.
     *
     * @see #setZoomChangeListener(OnZoomChangeListener)
     * @see #startSmoothZoom(int)
     */
    class OnZoomChangeListener
    {
    public:
		OnZoomChangeListener(){}
		virtual ~OnZoomChangeListener(){}
        /**
         * Called when the zoom value has changed during a smooth zoom.
         *
         * @param zoomValue the current zoom value. In smooth zoom mode, camera
         *                  calls this for every new zoom value.
         * @param stopped whether smooth zoom is stopped. If the value is true,
         *                this is the last zoom update for the application.
         * @param camera  the Camera service object
         */
        virtual void onZoomChange(int zoomValue, bool stopped, HerbCamera *pCamera) = 0;
    };

    /**
     * Registers a listener to be notified when the zoom value is updated by the
     * camera driver during smooth zoom.
     *
     * @param listener the listener to notify
     * @see #startSmoothZoom(int)
     */
    void setZoomChangeListener(OnZoomChangeListener *pListener);

    /**
     * Information about a face identified through camera face detection.
     *
     * <p>When face detection is used with a camera, the {@link FaceDetectionListener} returns a
     * list of face objects for use in focusing and metering.</p>
     *
     * @see FaceDetectionListener
     */
    class Face {
    public:
        /**
         * Create an empty face.
         */
        Face(){};

        /**
         * Bounds of the face. (-1000, -1000) represents the top-left of the
         * camera field of view, and (1000, 1000) represents the bottom-right of
         * the field of view. For example, suppose the size of the viewfinder UI
         * is 800x480. The rect passed from the driver is (-1000, -1000, 0, 0).
         * The corresponding viewfinder rect should be (0, 0, 400, 240). It is
         * guaranteed left < right and top < bottom. The coordinates can be
         * smaller than -1000 or bigger than 1000. But at least one vertex will
         * be within (-1000, -1000) and (1000, 1000).
         *
         * <p>The direction is relative to the sensor orientation, that is, what
         * the sensor sees. The direction is not affected by the rotation or
         * mirroring of {@link #setDisplayOrientation(int)}. The face bounding
         * rectangle does not provide any information about face orientation.</p>
         *
         * <p>Here is the matrix to convert driver coordinates to View coordinates
         * in pixels.</p>
         * <pre>
         * Matrix matrix = new Matrix();
         * CameraInfo info = CameraHolder.instance().getCameraInfo()[cameraId];
         * // Need mirror for front camera.
         * boolean mirror = (info.facing == CameraInfo.CAMERA_FACING_FRONT);
         * matrix.setScale(mirror ? -1 : 1, 1);
         * // This is the value for android.hardware.Camera.setDisplayOrientation.
         * matrix.postRotate(displayOrientation);
         * // Camera driver coordinates range from (-1000, -1000) to (1000, 1000).
         * // UI coordinates range from (0, 0) to (width, height).
         * matrix.postScale(view.getWidth() / 2000f, view.getHeight() / 2000f);
         * matrix.postTranslate(view.getWidth() / 2f, view.getHeight() / 2f);
         * </pre>
         *
         * @see #startFaceDetection()
         */
        Rect rect;

        /**
         * <p>The confidence level for the detection of the face. The range is 1 to
         * 100. 100 is the highest confidence.</p>
         *
         * <p>Depending on the device, even very low-confidence faces may be
         * listed, so applications should filter out faces with low confidence,
         * depending on the use case. For a typical point-and-shoot camera
         * application that wishes to display rectangles around detected faces,
         * filtering out faces with confidence less than 50 is recommended.</p>
         *
         * @see #startFaceDetection()
         */
        int score;

        /**
         * An unique id per face while the face is visible to the tracker. If
         * the face leaves the field-of-view and comes back, it will get a new
         * id. This is an optional field, may not be supported on all devices.
         * If not supported, id will always be set to -1. The optional fields
         * are supported as a set. Either they are all valid, or none of them
         * are.
         */
        int id; // = -1;

        /**
         * The coordinates of the center of the left eye. The coordinates are in
         * the same space as the ones for {@link #rect}. This is an optional
         * field, may not be supported on all devices. If not supported, the
         * value will always be set to null. The optional fields are supported
         * as a set. Either they are all valid, or none of them are.
         */
        Point leftEye;  //= null

        /**
         * The coordinates of the center of the right eye. The coordinates are
         * in the same space as the ones for {@link #rect}.This is an optional
         * field, may not be supported on all devices. If not supported, the
         * value will always be set to null. The optional fields are supported
         * as a set. Either they are all valid, or none of them are.
         */
        Point rightEye; // = null

        /**
         * The coordinates of the center of the mouth.  The coordinates are in
         * the same space as the ones for {@link #rect}. This is an optional
         * field, may not be supported on all devices. If not supported, the
         * value will always be set to null. The optional fields are supported
         * as a set. Either they are all valid, or none of them are.
         */
        Point mouth; // = null
    };
    /**
     * Callback interface for face detected in the preview frame.
     *
     */
    class FaceDetectionListener
    {
    public:
		FaceDetectionListener(){}
		virtual ~FaceDetectionListener(){}
        /**
         * Notify the listener of the detected faces in the preview frame.
         *
         * @param faces The detected faces in a list
         * @param camera  The {@link Camera} service object
         */
        virtual void onFaceDetection(Vector<Face > &faces, HerbCamera *pCamera) = 0;
    };

    /**
     * Registers a listener to be notified about the faces detected in the
     * preview frame.
     *
     * @param listener the listener to notify
     * @see #startFaceDetection()
     */
    void setFaceDetectionListener(FaceDetectionListener *pListener);
    
    /**
     * Starts the face detection. This should be called after preview is started.
     * The camera will notify {@link FaceDetectionListener} of the detected
     * faces in the preview frame. The detected faces may be the same as the
     * previous ones. Applications should call {@link #stopFaceDetection} to
     * stop the face detection. This method is supported if {@link
     * Parameters#getMaxNumDetectedFaces()} returns a number larger than 0.
     * If the face detection has started, apps should not call this again.
     *
     * <p>When the face detection is running, {@link Parameters#setWhiteBalance(String)},
     * {@link Parameters#setFocusAreas(List)}, and {@link Parameters#setMeteringAreas(List)}
     * have no effect. The camera uses the detected faces to do auto-white balance,
     * auto exposure, and autofocus.
     *
     * <p>If the apps call {@link #autoFocus(AutoFocusCallback)}, the camera
     * will stop sending face callbacks. The last face callback indicates the
     * areas used to do autofocus. After focus completes, face detection will
     * resume sending face callbacks. If the apps call {@link
     * #cancelAutoFocus()}, the face callbacks will also resume.</p>
     *
     * <p>After calling {@link #takePicture(Camera.ShutterCallback, Camera.PictureCallback,
     * Camera.PictureCallback)} or {@link #stopPreview()}, and then resuming
     * preview with {@link #startPreview()}, the apps should call this method
     * again to resume face detection.</p>
     *
     * @throws IllegalArgumentException if the face detection is unsupported.
     * @throws RuntimeException if the method fails or the face detection is
     *         already running.
     * @see FaceDetectionListener
     * @see #stopFaceDetection()
     * @see Parameters#getMaxNumDetectedFaces()
     */
    void startFaceDetection();
    /**
     * Stops the face detection.
     *
     * @see #startFaceDetection()
     */
    void stopFaceDetection();

	/* MOTION_DETECTION_ENABLE start */
	void startAWMoveDetection();
	void stopAWMoveDetection();
	status_t setAWMDSensitivityLevel(int level);
    void setAWMoveDetectionListener(AWMoveDetectionListener *pListener);
    class AWMoveDetectionListener
    {
    public:
		AWMoveDetectionListener(){}
		virtual ~AWMoveDetectionListener(){}
        virtual void onAWMoveDetection(int value, HerbCamera *pCamera) = 0;
    };
	/* MOTION_DETECTION_ENABLE end */

	/* WATERMARK_ENABLE start*/
    status_t  setWaterMark(int enable, const char *str);
	/* WATERMARK_ENABLE end*/

	/* for ADAS start */
	void adasStartDetection();
	void adasStopDetection();
    status_t adasSetLaneLineOffsetSensity(int level);
    status_t adasSetDistanceDetectLevel(int level);
	status_t adasSetMode(int mode);
	status_t setAdasTipsMode(int mode);
    status_t adasSetCarSpeed(float speed);
	status_t adasSetAduioVol(int val);
    status_t adasSetGsensorData(int val0, float val1, float val2);
    void setAdasDetectionListener(AdasDetectionListener *pListener);
    class AdasDetectionListener
    {
    public:
		AdasDetectionListener(){}
		virtual ~AdasDetectionListener(){}
        virtual void onAdasDetection(void *data, int size, HerbCamera *pCamera) = 0;
    };
	/* for ADAS end */

    status_t setQrDecode(bool enable);
    void setQrDecodeListener(QrDecodeListener *pListener);
    class QrDecodeListener
    {
    public:
		QrDecodeListener(){}
		virtual ~QrDecodeListener(){}
        virtual void onQrDecode(void *data, int size, HerbCamera *pCamera) = 0;
    };

    status_t startRender(void);
    status_t stopRender(void);

#ifdef SUPPORT_TVIN
	class TVinListener
	{
		TVinListener(){}
		virtual ~TVinListener(){}
		virtual onTVinSystemChange(int system, HerbCamera *pCamera) = 0;
	};
#endif

    // Error codes match the enum in include/ui/Camera.h

    enum{
        /**
         * Unspecified camera error.
         * @see Camera.ErrorCallback
         */
        CAMERA_ERROR_UNKNOWN = 1,

        /**
         * Media server died. In this case, the application must release the
         * Camera object and instantiate a new one.
         * @see Camera.ErrorCallback
         */
        CAMERA_ERROR_SERVER_DIED = 100,
    };
    /**
     * Callback interface for camera error notification.
     *
     * @see #setErrorCallback(ErrorCallback)
     */
    class ErrorCallback
    {
    public:
		ErrorCallback(){}
		virtual ~ErrorCallback(){}
        /**
         * Callback for camera errors.
         * @param error   error code:
         * <ul>
         * <li>{@link #CAMERA_ERROR_UNKNOWN}
         * <li>{@link #CAMERA_ERROR_SERVER_DIED}
         * </ul>
         * @param camera  the Camera service object
         */
        virtual void onError(int error, HerbCamera *pCamera) = 0;
    };

    /**
     * Registers a callback to be invoked when an error occurs.
     * @param cb The callback to run
     */
    void setErrorCallback(ErrorCallback *pCb);

    class Area {
    public:
        inline Area() {
            mWeight = 0;
            mRect = new Rect(0, 0, 0, 0);
        }
        inline Area(Rect *rect, int weight) {
            mWeight = weight;
            mRect = new Rect(rect->left, rect->top, rect->right, rect->bottom);
        }
        inline ~Area() {
            delete mRect;
        }

        Rect *mRect;
        int mWeight;
    };

    /**
     * Camera service settings.
     *
     * <p>To make camera parameters take effect, applications have to call
     * {@link Camera#setParameters(Camera.Parameters)}. For example, after
     * {@link Camera.Parameters#setWhiteBalance} is called, white balance is not
     * actually changed until {@link Camera#setParameters(Camera.Parameters)}
     * is called with the changed parameters object.
     *
     * <p>Different devices may have different camera capabilities, such as
     * picture size or flash modes. The application should query the camera
     * capabilities before setting parameters. For example, the application
     * should call {@link Camera.Parameters#getSupportedColorEffects()} before
     * calling {@link Camera.Parameters#setColorEffect(String)}. If the
     * camera does not support color effects,
     * {@link Camera.Parameters#getSupportedColorEffects()} will return null.
     */
    //必须遵循android底层的 CameraParameters(.\android\frameworks\av\include\camera\CameraParameters.h)所制定的规范.
    //.\android\device\softwinner\polaris-common\hardware\camera\CameraHardware2.cpp的函数
    //status_t CameraHardware::setParameters(const char* p)是示范.只有按照这个示范书写.
    class Parameters
    {
    public:
    	Parameters();
    	~Parameters();
    	void dump();
    	String8 flatten() const;
    	void unflatten(const String8 &params);
    	void remove(const char *key);
    	void set(const char *key, const char *value);
    	void set(const char *key, int value);
    	const char *get(const char *key) const;
    	int getInt(const char *key) const;
    	float getFloat(const char *key) const;
    	void setPreviewSize(int width, int height);
		void setVideoSize(int width, int height);
    	void getPreviewSize(Size &size) const;
        void getVideoSize(Size &size) const;
    	void getSupportedPreviewSizes(Vector<Size> &sizes) const;
    	void getSupportedVideoSizes(Vector<Size> &sizes) const;
    	void getPreferredPreviewSizeForVideo(int *width, int *height) const;
    	void setJpegThumbnailSize(int width, int height);
    	void getJpegThumbnailSize(Size &size);
    	void getSupportedJpegThumbnailSizes(Vector<Size> &sizes);
    	void setJpegThumbnailQuality(int quality);
    	int getJpegThumbnailQuality();
    	void setJpegQuality(int quality);
    	int getJpegQuality();
    	void setPreviewFrameRate(int fps);
    	int getPreviewFrameRate();
    	void getSupportedPreviewFrameRates(Vector<int> &frameRates);
    	void setPreviewFpsRange(int min, int max);
    	void getPreviewFpsRange(int *min, int *max);
    	void getSupportedPreviewFpsRange(Vector<int[2]> &range);
    	void setPreviewFormat(int pixel_format);
    	int getPreviewFormat();
    	void getSupportedPreviewFormats(Vector<int> &formats);
    	void setPictureSize(int width, int height);
    	void getPictureSize(int *width, int *height) const;
    	void getSupportedPictureSizes(Vector<Size> &sizes) const;
    	void setPictureFormat(int pixel_format);
    	int getPictureFormat();
    	void getSupportedPictureFormats(Vector<int> &formats);
    	void setRotation(int rotation);
        void setPreviewFlip(int flip);
        int getPreviewFlip() const;
        void setPreviewRotation(int rotation);
        int getPreviewRotation() const;
        void setPictureMode(const char *mode);
        const char *getPictureMode() const;
        void setPictureSizeMode(int mode);
        int getPictureSizeMode() const;
    	void setGpsLatitude(double latitude);
    	void setGpsLongitude(double longitude);
    	void setGpsAltitude(double altitude);
    	void setGpsTimestamp(long timestamp);
    	void setGpsProcessingMethod(const char *processing_method);
    	void removeGpsData();
    	const char *getWhiteBalance();
    	void setWhiteBalance(const char *value);
    	void getSupportedWhiteBalance(Vector<String8>& whiteBalances);
    	const char *getColorEffect();
    	void setColorEffect(const char *value);
    	void getSupportedColorEffects(Vector<String8>& effects);
    	const char *getAntibanding();
    	void setAntibanding(const char *antibanding);
    	void getSupportedAntibanding(Vector<String8>& antibanding);
    	const char *getSceneMode();
    	void setSceneMode(const char *value);
    	void getSupportedSceneModes(Vector<String8>& sceneModes);
    	const char *getFlashMode();
    	void setFlashMode(const char *value);
    	void getSupportedFlashModes(Vector<String8>& flashModes);
        const char *getFocusMode();
    	void setFocusMode(const char *value);
    	void getSupportedFocusModes(Vector<String8>& focusModes);
    	float getFocalLength();
    	float getHorizontalViewAngle();
    	float getVerticalViewAngle();
    	int getExposureCompensation();
    	void setExposureCompensation(int value);
    	int getMaxExposureCompensation();
    	int getMinExposureCompensation();
    	float getExposureCompensationStep();
		
    	void setContrastValue(int value);
    	int getContrastValue();
		void setBrightnessValue(int value);
    	int getBrightnessValue();
		void setSaturationValue(int value);
		int getSaturationValue();
		void setHueValue(int value);
		int getHueValue();
		void setSharpnessValue(int value);
        void setImageEffect(int effect);
		int getSharpnessValue();
        void setHflip(int value);
        void setVflip(int value);
        void setPowerLineFrequencyValue(int value); //HerbCamera::Parameters::PowerLineFrequency::PLF_50HZ
        int getPowerLineFrequencyValue();

    	void setAutoExposureLock(bool toggle);
    	bool getAutoExposureLock();
    	bool isAutoExposureLockSupported();
    	void setAutoWhiteBalanceLock(bool toggle);
    	bool getAutoWhiteBalanceLock();
    	bool isAutoWhiteBalanceLockSupported();
    	int getZoom();
    	void setZoom(int value);
    	bool isZoomSupported();
    	int getMaxZoom();
    	void getZoomRatios(Vector<int>& ratios);
    	bool isSmoothZoomSupported();
    	void getFocusDistances(float *distances);
    	int getMaxNumFocusAreas();
    	void getFocusAreas(Vector<Area> &areas);
    	void setFocusAreas(Vector<Area> &areas);
    	int getMaxNumMeteringAreas();
    	void getMeteringAreas(Vector<Area> &areas);
    	void setMeteringAreas(Vector<Area> &areas);
    	int getMaxNumDetectedFaces();
    	void setRecordingHint(bool hint);
    	bool isVideoSnapshotSupported();
    	void setVideoStabilization(bool toggle);
    	bool getVideoStabilization();
    	bool isVideoStabilizationSupported();

        void setSubChannelSize(int width, int height);

        void setVideoCaptureBufferNum(int number);
        int getVideoCaptureBufferNum();

        void setContinuousPictureNumber(int number);
        int getContinuousPictureNumber();
        void setContinuousPictureIntervalMs(int timeMs);
        int getContinuousPictureIntervalMs();

    	static const char SUPPORTED_VALUES_SUFFIX[];

        // Values for KEY_AWEXTEND_PREVIEW_FLIP
        struct PREVIEW_FLIP   //ref to CameraParameters.AWEXTEND_PREVIEW_FLIP [CameraParameters.h]  
        {
            static const int NoFlip = 0;
            static const int LeftRightFlip = 1;
            static const int TopBottomFlip = 2;
        private:
            PREVIEW_FLIP() {};
        };
        // Values for KEY_CID_POWER_LINE_FREQUENCY
        struct PowerLineFrequency   //ref to [./device/softwinner/common/hardware/include/videodev2_new.h]  
        {
            static const int PLF_50HZ = 1;  //V4L2_CID_POWER_LINE_FREQUENCY_50HZ
            static const int PLF_60HZ = 2;
            static const int PLF_AUTO = 3;
        private:
            PowerLineFrequency() {};
        };

    private:
    	float getFloat(const char *key, float defaultValue) const;
    	int getInt(const char *key, int defaultValue) const;
    	const char *cameraFormatForPixelFormat(int pixel_format);
    	int pixelFormatForCameraFormat(const char *format);
		void set(const char *key, Vector<Area> &areas);

        DefaultKeyedVector<String8,String8>    mMap;

    };

    /**
     * Changes the settings for this Camera service.
     *
     * @param params the Parameters to use for this Camera service
     * @throws RuntimeException if any parameter is invalid or not supported.
     * @see #getParameters()
     */
    status_t setParameters(Parameters *pParams);
    /**
     * Returns the current settings for this Camera service.
     * If modifications are made to the returned Parameters, they must be passed
     * to {@link #setParameters(Camera.Parameters)} to take effect.
     *
     * @see #setParameters(Camera.Parameters)
     */
    status_t getParameters(Parameters* pParams);

	int getContrastCtrl(struct v4l2_queryctrl *ctrl);
	int getBrightnessCtrl(struct v4l2_queryctrl *ctrl);
	int getSaturationCtrl(struct v4l2_queryctrl *ctrl);
	int getHueCtrl(struct v4l2_queryctrl *ctrl);
	int getSharpnessCtrl(struct v4l2_queryctrl *ctrl);
	status_t getInputSource(int *source);
	status_t getTVinSystem(int *system);
    status_t setUvcGadgetMode(int value);
    status_t setColorFx(int value);
    status_t cancelContinuousPicture();
private:
    int getV4l2QueryCtrl(struct v4l2_queryctrl *ctrl, int id);

	static int parse_pair(const char *str, int *first, int *second, char delim, char **endptr = NULL);
	static void parseSizesList(const char *sizesStr, Vector<Size> &sizes);
	static void parseIntList(const char *intStr, Vector<int>& values);
	static void parseRangeList(const char *rangeStr, Vector<int[2]>& ranges);
	static void parseString8List(const char *strIn, Vector<String8>& strs);
	static void parseFloat(const char *floatStr, float *values, int size);
	static void parseInt(const char *intStr, int *values, int size);
	static void parseAreaList(const char *areaStr, Vector<Area> &areas);

public:
	enum {
		IMAGE_FORMAT_UNKNOWN = 0,
		IMAGE_FORMAT_RGB_565 = 4,
		IMAGE_FORMAT_YV12 = 0x32315659,
		IMAGE_FORMAT_NV16 = 0x10,
		IMAGE_FORMAT_NV21 = 0x11,
		IMAGE_FORMAT_YUY2 = 0x14,
		IMAGE_FORMAT_JPEG = 0x100,
		IMAGE_FORMAT_BAYER_RGGB = 0x200,
	};
};

};

#endif /* __HERB_CAMERA_H__ */

