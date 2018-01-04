
#include "CameraDebug.h"
#if DBG_CAMERA_HARDWARE
#define LOG_NDEBUG 0
#endif

#define LOG_TAG "CameraHardware2"
#include <cutils/log.h>

#include <cutils/properties.h>
#include <ui/Rect.h>

//#include <drv_display.h>

#include "CameraHardware2.h"
#include "V4L2CameraDevice2.h"


#ifdef LOGV
#undef LOGV
#define LOGV(fmt, arg...)	ALOGV("<F:%s,L:%d,Camera[%d]> " fmt, __FUNCTION__, __LINE__, mV4L2CameraDevice->getInputSource(), ##arg)
#endif

#ifdef LOGD
#undef LOGD
#define LOGD(fmt, arg...)	ALOGD("<F:%s,L:%d,Camera[%d]> " fmt, __FUNCTION__, __LINE__, mV4L2CameraDevice->getInputSource(), ##arg)
#endif

#ifdef LOGI
#undef LOGI
#define LOGI(fmt, arg...)	ALOGI("<F:%s,L:%d,Camera[%d]> " fmt, __FUNCTION__, __LINE__, mV4L2CameraDevice->getInputSource(), ##arg)
#endif

#ifdef LOGW
#undef LOGW
#define LOGW(fmt, arg...)	ALOGW("<F:%s,L:%d,Camera[%d]> " fmt, __FUNCTION__, __LINE__, mV4L2CameraDevice->getInputSource(), ##arg)
#endif

#ifdef LOGE
#undef LOGE
#define LOGE(fmt, arg...)	ALOGE("<F:%s,L:%d,Camera[%d]> " fmt, __FUNCTION__, __LINE__, mV4L2CameraDevice->getInputSource(), ##arg)
#endif


#define BASE_ZOOM	0

namespace android {

// defined in HALCameraFactory.cpp
extern void getCallingProcessName(char *name);

#if DEBUG_PARAM
/* Calculates and logs parameter changes.
 * Param:
 *  current - Current set of camera parameters.
 *  new_par - String representation of new parameters.
 */
static void PrintParamDiff(const CameraParameters& current, const char* new_par);
#else
#define PrintParamDiff(current, new_par)   (void(0))
#endif  /* DEBUG_PARAM */

/* A helper routine that adds a value to the camera parameter.
 * Param:
 *  param - Camera parameter to add a value to.
 *  val - Value to add.
 * Return:
 *  A new string containing parameter with the added value on success, or NULL on
 *  a failure. If non-NULL string is returned, the caller is responsible for
 *  freeing it with 'free'.
 */
static char* AddValue(const char* param, const char* val);

#if 0  // face
static int faceNotifyCb(int cmd, void * data, void * user)
{
	CameraHardware* camera_hw = (CameraHardware *)user;
	
	switch (cmd)
	{
		case FACE_NOTITY_CMD_REQUEST_FRAME:
			return camera_hw->getCurrentFaceFrame(data);
			
		case FACE_NOTITY_CMD_RESULT:
			return camera_hw->faceDetection((camera_frame_metadata_t*)data);

		case FACE_NOTITY_CMD_POSITION:
			{
				FocusArea_t * pdata = (FocusArea_t *)data;
				char face_area[128];
				sprintf(face_area, "(%d, %d, %d, %d, 1)", 
						pdata->x, pdata->y, pdata->x1, pdata->y1);
				return camera_hw->parse_focus_areas(face_area, true);
			}
		case FACE_NOTITY_CMD_REQUEST_ORIENTION:
			camera_hw->getCurrentOriention((int*)data);
			break;
			
		default:
			break;
	}
	
	return 0;
}
#endif

// Parse string like "640x480" or "10000,20000"
static int parse_pair(const char *str, int *first, int *second, char delim,
                      char **endptr = NULL)
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

CameraHardware::CameraHardware(struct hw_module_t* module, CCameraConfig* pCameraCfg)
        : mPreviewWindow()
        , mCallbackNotifier()
        , mCameraConfig(pCameraCfg)
        , mIsCameraIdle(true)
        , mFirstSetParameters(true)
        , mFullSizeWidth(0)
        , mFullSizeHeight(0)
        , mCaptureWidth(0)
        , mCaptureHeight(0)
        //, mVideoCaptureWidth(0)
        //, mVideoCaptureHeight(0)
        , mUseHwEncoder(false)
#if 0  // face
        , mFaceDetection(NULL)
#endif
        , mAutoFocusThreadExit(true)
        , mFocusStatus(FOCUS_STATUS_IDLE)
        , mIsSingleFocus(false)
        , mOriention(0)
        , mZoomRatio(100)
        //, mIsImageCaptureIntent(false)
{
    /*
     * Initialize camera_device descriptor for this object.
     */
	ALOGV("Constructor");

    /* Common header */
    common.tag = HARDWARE_DEVICE_TAG;
    common.version = 0;
    common.module = module;
    common.close = CameraHardware::close;

    /* camera_device fields. */
    ops = &mDeviceOps;
    priv = this;

	// instance V4L2CameraDevice object
	mV4L2CameraDevice = new V4L2CameraDevice(this, &mPreviewWindow, &mCallbackNotifier);
	if (mV4L2CameraDevice == NULL)
	{
		ALOGE("Failed to create V4L2Camera instance");
		return ;
	}

	memset((void*)mCallingProcessName, 0, sizeof(mCallingProcessName));

	memset(&mFrameRectCrop, 0, sizeof(mFrameRectCrop));
	memset((void*)mFocusAreasStr, 0, sizeof(mFocusAreasStr));
	memset((void*)&mLastFocusAreas, 0, sizeof(mLastFocusAreas));
	memset(&mHalCameraInfo, 0 , sizeof(mHalCameraInfo));
	// init command queue
	OSAL_QueueCreate(&mQueueCommand, CMD_QUEUE_MAX);
	memset((void*)mQueueElement, 0, sizeof(mQueueElement));

	// init command thread
	pthread_mutex_init(&mCommandMutex, NULL);
	pthread_cond_init(&mCommandCond, NULL);
	mCommandThread = new DoCommandThread(this);
	mCommandThread->startThread();
	
	// init auto focus thread
	pthread_mutex_init(&mAutoFocusMutex, NULL);
	pthread_cond_init(&mAutoFocusCond, NULL);
	mAutoFocusThread = new DoAutoFocusThread(this);
}

CameraHardware::~CameraHardware()
{
	if (mCommandThread != NULL)
	{
		mCommandThread->stopThread();
		pthread_cond_signal(&mCommandCond);
		mCommandThread.clear();
		mCommandThread = 0;
	}
		
	pthread_mutex_destroy(&mCommandMutex);
	pthread_cond_destroy(&mCommandCond);
	
	if (mAutoFocusThread != NULL)
	{
		mAutoFocusThread.clear();
		mAutoFocusThread = 0;
	}

	pthread_mutex_destroy(&mAutoFocusMutex);
	pthread_cond_destroy(&mAutoFocusCond);

	OSAL_QueueTerminate(&mQueueCommand);

#if 0  // face
	if (mFaceDetection != NULL)
	{
		DestroyFaceDetectionDev(mFaceDetection);
		mFaceDetection = NULL;
	}
#endif

	if (mV4L2CameraDevice != NULL)
	{
		delete mV4L2CameraDevice;
		mV4L2CameraDevice = NULL;
	}
}

bool CameraHardware::autoFocusThread()
{
	bool ret = true;
	int status = -1;
	FocusStatus new_status = FOCUS_STATUS_IDLE;

	usleep(30000);

	const char * valstr = mParameters.get(CameraParameters::KEY_RECORDING_HINT);
	//if(valstr != NULL 
	//	&& strcmp(valstr, CameraParameters::sTRUE) == 0)
	//{
	//	 return true;		// for cts
	//}

	bool timeout = false;
	int64_t lastTimeMs = systemTime() / 1000000;

	pthread_mutex_lock(&mAutoFocusMutex);
	if (mAutoFocusThread->getThreadStatus() == THREAD_STATE_EXIT)
	{
		LOGD("autoFocusThread exited");
		ret = false;		// exit thread
		pthread_mutex_unlock(&mAutoFocusMutex);
		goto FOCUS_THREAD_EXIT;
	}
	mAutoFocusThreadExit = false;
	pthread_mutex_unlock(&mAutoFocusMutex);

	if (mIsSingleFocus)
	{
		while(1)
		{
			// if hw af ok
			status = mV4L2CameraDevice->getAutoFocusStatus();
			if ( status == V4L2_AUTO_FOCUS_STATUS_REACHED)
			{
				LOGV("auto focus ok, use time: %lld(ms)", systemTime() / 1000000 - lastTimeMs);
				break;
			}
			else if (status == V4L2_AUTO_FOCUS_STATUS_BUSY
						|| status == V4L2_AUTO_FOCUS_STATUS_IDLE)
			{
				if ((systemTime() / 1000000 - lastTimeMs) > 2000)	// 2s timeout
				{
					LOGW("auto focus time out, ret = %d", status);
					timeout = true;
					break;
				}
				//LOGV("wait auto focus ......");
				usleep(30000);
			}
			else if (status == V4L2_AUTO_FOCUS_STATUS_FAILED)
			{
				LOGW("auto focus failed");
				break;
			}
			else if (status < 0)
			{
				LOGE("auto focus interface error");
				break;
			}
		}
		
		mCallbackNotifier.autoFocusMsg(status == V4L2_AUTO_FOCUS_STATUS_REACHED);

		if (status == V4L2_AUTO_FOCUS_STATUS_REACHED)
		{
			mV4L2CameraDevice->set3ALock(V4L2_LOCK_FOCUS);
		}

		mIsSingleFocus = false;

		mFocusStatus = FOCUS_STATUS_DONE;
	}
	else
	{
		status = mV4L2CameraDevice->getAutoFocusStatus();
		
		if (status == V4L2_AUTO_FOCUS_STATUS_BUSY)
		{
			new_status = FOCUS_STATUS_BUSY;
		}
		else if (status == V4L2_AUTO_FOCUS_STATUS_REACHED)
		{
			new_status = FOCUS_STATUS_SUCCESS;
		}
		else if (status == V4L2_AUTO_FOCUS_STATUS_FAILED)
		{
			new_status = FOCUS_STATUS_FAIL;
		}
		else if (status == V4L2_AUTO_FOCUS_STATUS_IDLE)
		{
			// do nothing
			new_status = FOCUS_STATUS_IDLE;
		}
		else if ((__u32)status == 0xFF000000)
		{
			LOGV("getAutoFocusStatus, status = 0xFF000000");
			ret = false;		// exit thread
			goto FOCUS_THREAD_EXIT;
		}
		else
		{
			LOGW("unknow focus status: %d", status);
			ret = true;
			goto FOCUS_THREAD_EXIT;
		}

		// LOGD("status change, old: %d, new: %d", mFocusStatus, new_status);

		if (mFocusStatus == new_status)
		{
			ret = true;
			goto FOCUS_THREAD_EXIT;
		}

		if ((mFocusStatus == FOCUS_STATUS_IDLE
				|| mFocusStatus & FOCUS_STATUS_DONE)
			&& new_status == FOCUS_STATUS_BUSY)
		{
			// start focus
			LOGV("start focus, %d -> %d", mFocusStatus, new_status);
			// show focus animation
			mCallbackNotifier.autoFocusContinuousMsg(true);
		}
		else if (mFocusStatus == FOCUS_STATUS_BUSY
				&& (new_status & FOCUS_STATUS_DONE))
		{
			// focus end
			LOGV("focus %s, %d -> %d", (new_status == FOCUS_STATUS_SUCCESS)?"ok":"fail", mFocusStatus, new_status);
			mCallbackNotifier.autoFocusContinuousMsg(false);
		}
		else if ((mFocusStatus & FOCUS_STATUS_DONE)
				&& new_status == FOCUS_STATUS_IDLE)
		{
			// focus end
			LOGV("do nothing, %d -> %d", mFocusStatus, new_status);
		}
		else
		{
			LOGW("unknow status change, %d -> %d", mFocusStatus, new_status);
		}

		mFocusStatus = new_status;
	}

FOCUS_THREAD_EXIT:
	if (ret == false)
	{
		pthread_mutex_lock(&mAutoFocusMutex);
		mAutoFocusThreadExit = true;
		pthread_cond_signal(&mAutoFocusCond);
		pthread_mutex_unlock(&mAutoFocusMutex);
	}
	return ret;
}

bool CameraHardware::commandThread()
{
	pthread_mutex_lock(&mCommandMutex);
	if (mCommandThread->getThreadStatus() == THREAD_STATE_EXIT)
	{
		LOGD("commandThread exited");
		pthread_mutex_unlock(&mCommandMutex);
		return false;
	}
	pthread_mutex_unlock(&mCommandMutex);
	
	Queue_Element * queue = (Queue_Element *)OSAL_Dequeue(&mQueueCommand);
	if (queue == NULL)
	{
		LOGV("wait commond queue ......");
		pthread_mutex_lock(&mCommandMutex);
		pthread_cond_wait(&mCommandCond, &mCommandMutex);
		pthread_mutex_unlock(&mCommandMutex);
		return true;
	}

	V4L2CameraDevice* pV4L2Device = mV4L2CameraDevice;
	int cmd = queue->cmd;
	switch(cmd)
	{
		case CMD_QUEUE_SET_COLOR_EFFECT:
		{
			int new_image_effect = queue->data;
			LOGV("CMD_QUEUE_SET_COLOR_EFFECT: %d", new_image_effect);
			
			if (pV4L2Device->setImageEffect(new_image_effect) < 0) 
			{
                LOGE("Fail on mV4L2Camera->setImageEffect(effect(%d))", new_image_effect);
            }
			break;
		}
		case CMD_QUEUE_SET_WHITE_BALANCE:
		{
			int new_white = queue->data;
			LOGV("CMD_QUEUE_SET_WHITE_BALANCE: %d", new_white);
			
            if (pV4L2Device->setWhiteBalance(new_white) < 0) 
			{
                LOGE("Fail on mV4L2Camera->setWhiteBalance(white(%d))", new_white);
            }
			break;
		}
		case CMD_QUEUE_SET_EXPOSURE_COMPENSATION:
		{
			int new_exposure_compensation = queue->data;
			LOGV("CMD_QUEUE_SET_EXPOSURE_COMPENSATION: %d", new_exposure_compensation);
			
			if (pV4L2Device->setExposureCompensation(new_exposure_compensation) < 0) 
			{
				LOGE("Fail on mV4L2Camera->setExposureCompensation(brightness(%d))", new_exposure_compensation);
			}
			break;
		}
		case CMD_QUEUE_SET_FLASH_MODE:
		{
			LOGD("CMD_QUEUE_SET_FLASH_MODE to do ...");
			break;
		}
		case CMD_QUEUE_SET_FOCUS_MODE:
		{
			LOGV("CMD_QUEUE_SET_FOCUS_MODE");
			if(setAutoFocusRange() != OK)
	        {
				LOGE("unknown focus mode");
	       	}
			break;
		}
		case CMD_QUEUE_SET_FOCUS_AREA:
		{
			char * new_focus_areas_str = (char *)queue->data;
			if (new_focus_areas_str != NULL)
			{
				LOGV("CMD_QUEUE_SET_FOCUS_AREA: %s", new_focus_areas_str);
        		parse_focus_areas(new_focus_areas_str);
			}
        	break;
		}

#if 0  // face
		case CMD_QUEUE_START_FACE_DETECTE:
		{
            LOGE("not support face detect!");
			int width = 0, height = 0;
			LOGV("CMD_QUEUE_START_FACE_DETECTE");
			if (mHalCameraInfo.fast_picture_mode)
			{
				int scale = pV4L2Device->getSuitableThumbScale(mCaptureWidth, mCaptureHeight);
				if (scale <= 0)
				{
					LOGE("error thumb scale: %d, full-size: %dx%d", scale, mCaptureWidth, mCaptureHeight);
					break;
				}
				width = mCaptureWidth / scale;
				height = mCaptureHeight / scale;
			}
			else
			{
//				const char * preview_capture_width_str = mParameters.get(KEY_PREVIEW_CAPTURE_SIZE_WIDTH);
//				const char * preview_capture_height_str = mParameters.get(KEY_PREVIEW_CAPTURE_SIZE_HEIGHT);
//				if (preview_capture_width_str != NULL
//					&& preview_capture_height_str != NULL)
//				{
//					width  = atoi(preview_capture_width_str);
//					height = atoi(preview_capture_height_str);
//				}
                mParameters.getPreviewSize(&width, &height);
			}
			
//			if (mFaceDetection != 0)
//			{
//				LOGV("start facedetection size: %dx%d", width, height);
//				mFaceDetection->ioctrl(mFaceDetection, FACE_OPS_CMD_START, width, height);
//			}
//			else
//			{
//				LOGW("CMD_QUEUE_START_FACE_DETECTE failed, mFaceDetection not opened.");
//			}
			break;
		}
		case CMD_QUEUE_STOP_FACE_DETECTE:
		{
            LOGE("not support face detect!");
			LOGV("CMD_QUEUE_STOP_FACE_DETECTE");
//			if (mFaceDetection != 0)
//			{
//				mFaceDetection->ioctrl(mFaceDetection, FACE_OPS_CMD_STOP, 0, 0);
//			}
//			else
//			{
//				LOGW("CMD_QUEUE_STOP_FACE_DETECTE failed, mFaceDetection not opened.");
//			}
			break;
		}
#endif

		case CMD_QUEUE_TAKE_PICTURE:
		{
			LOGV("CMD_QUEUE_TAKE_PICTURE");
			doTakePicture();
			break;
		}
		case CMD_QUEUE_PICTURE_MSG:
		{
			LOGV("CMD_QUEUE_PICTURE_MSG");
			void * frame = (void *)queue->data;
			mCallbackNotifier.notifyPictureMsg(frame);
			//if (strcmp(mCallingProcessName, "com.android.cts.stub") != 0
			//	&& strcmp(mCallingProcessName, "com.android.cts.mediastress") != 0
			//	&& mIsImageCaptureIntent == false)
			//{			
				doTakePictureEnd();
			//}
			break;
		}
		case CMD_QUEUE_STOP_CONTINUOUSSNAP:
		{
			cancelPicture();
			doTakePictureEnd();
			break;
		}
		case CMD_QUEUE_SET_FOCUS_STATUS:
		{
			mCallbackNotifier.autoFocusMsg(true);
			break;
		}
#ifdef MOTION_DETECTION_ENABLE
		case CMD_QUEUE_START_AWMD:
		{
			LOGV("CMD_QUEUE_START_AWMD");
			mV4L2CameraDevice->AWMDInitialize();
			break;
		}
		case CMD_QUEUE_STOP_AWMD:
		{
			LOGV("CMD_QUEUE_STOP_AWMD");
			mV4L2CameraDevice->AWMDDestroy();
			break;
		}
		case CMD_QUEUE_SET_AWMD_SENSITIVITY_LEVEL:
		{
			LOGV("CMD_QUEUE_SET_AWMD_SENSITIVITY_LEVEL");
			int level = queue->data;
			mV4L2CameraDevice->AWMDSetSensitivityLevel(level);
			break;
		}
#endif
#ifdef ADAS_ENABLE
		case CMD_QUEUE_ADAS_START_DETECT:
		{
			LOGV("CMD_QUEUE_ADAS_START_DETECT");
			mV4L2CameraDevice->adasInitialize();
			break;
		}
		case CMD_QUEUE_ADAS_STOP_DETECT:
		{
			LOGV("CMD_QUEUE_ADAS_STOP_DETECT");
			mV4L2CameraDevice->adasDestroy();
			break;
		}
		case CMD_QUEUE_ADAS_SET_LANELINE_OFFSET_SENSITY:
		{
			LOGV("CMD_QUEUE_ADAS_SET_LANELINE_OFFSET_SENSITY");
			int level = queue->data;
			mV4L2CameraDevice->adasSetLaneLineOffsetSensity(level);
			break;
		}
		case CMD_QUEUE_ADAS_SET_DISTANCE_DETECT_LEVEL:
		{
			LOGV("CMD_QUEUE_ADAS_SET_DISTANCE_DETECT_LEVEL");
			int level = queue->data;
			mV4L2CameraDevice->adasSetDistanceDetectLevel(level);
			break;
		}
		case CMD_QUEUE_ADAS_SET_CAR_SPEED:
		{
			LOGV("CMD_QUEUE_ADAS_SET_CAR_SPEED");
			int speed = queue->data;
			mV4L2CameraDevice->adasSetCarSpeed(speed);
			break;
		}
		case CMD_QUEUE_ADAS_SET_AUDIO_VOL:
		{
			LOGV("CMD_QUEUE_ADAS_SET_AUDIO_VOL");
			int vol = queue->data;
			mV4L2CameraDevice->adasSetAudioVol(vol);
			break;
		}
		case CMD_QUEUE_ADAS_SET_MODE:
		{
			LOGV("CMD_QUEUE_ADAS_SET_MODE");
			int mode = queue->data;
			mV4L2CameraDevice->adasSetMode(mode);
			break;
		}
		case CMD_QUEUE_ADAS_SET_TIPS_MODE:
		{
			LOGV("CMD_QUEUE_ADAS_SET_TIPS_MODE");
			int mode = queue->data;
			mV4L2CameraDevice->adasSetTipsMode(mode);
			break;
		}
#endif

#ifdef QRDECODE_ENABLE
        case CMD_QUEUE_SET_QRDECODE:
        {
			LOGV("CAMERA_CMD_SET_QRDECODE");
            bool enable = (queue->data==0) ? false : true;
			mV4L2CameraDevice->setQrDecode(enable);
            break;
        }
#endif

		case CMD_QUEUE_START_RENDER:
		{
			LOGV("CMD_QUEUE_START_RENDER");
			mPreviewWindow.startPreview();
			break;
		}
		case CMD_QUEUE_STOP_RENDER:
		{
			LOGV("CMD_QUEUE_STOP_RENDER");
			if (mPreviewWindow.isPreviewEnabled())
			{
				mPreviewWindow.stopPreview();
			}
			break;
		}

		case CMD_QUEUE_SET_COLORFX:
		{
			LOGV("CMD_QUEUE_SET_COLORFX");
			int value = queue->data;
			mV4L2CameraDevice->setColorFx(value);
			break;
		}

		default:
			LOGW("unknown queue command: %d", cmd);
			break;
	}
	
	return true;
}

/****************************************************************************
 * Public API
 ***************************************************************************/

status_t CameraHardware::Initialize()
{
	LOGV();

	getCallingProcessName(mCallingProcessName);
	mCallbackNotifier.setCallingProcess(mCallingProcessName);

#if 0  // face
	if (mFaceDetection == NULL)
	{
		// create FaceDetection object
		CreateFaceDetectionDev(&mFaceDetection);
		if (mFaceDetection == NULL)
		{
			LOGE("create FaceDetection failed");
			return UNKNOWN_ERROR;
		}
	}

	mFaceDetection->ioctrl(mFaceDetection, FACE_OPS_CMD_REGISTE_USER, (int)this, 0);
	mFaceDetection->setCallback(mFaceDetection, faceNotifyCb);
#endif

	initDefaultParameters();

    return NO_ERROR;
}

void CameraHardware::initDefaultParameters()
{
    CameraParameters p = mParameters;
	String8 parameterString;
	char * value;

	// version
	p.set(CameraParameters::KEY_AWEXTEND_CAMERA_HAL_VERSION, CAMERA_HAL_VERSION);
    p.setPictureMode(CameraParameters::AWEXTEND_PICTURE_MODE_NORMAL);
    p.setPictureSizeMode(CameraParameters::AWEXTEND_PICTURE_SIZE_MODE::KeepSourceSize);

	// facing and orientation
	if (mHalCameraInfo.facing == CAMERA_FACING_BACK)
	{
	    p.set(CameraHardware::FACING_KEY, CameraHardware::FACING_BACK);
	    LOGV("camera is facing %s", CameraHardware::FACING_BACK);
	}
	else
	{
	    p.set(CameraHardware::FACING_KEY, CameraHardware::FACING_FRONT);
	    LOGV("camera is facing %s", CameraHardware::FACING_FRONT);
	}
	
    p.set(CameraHardware::ORIENTATION_KEY, mHalCameraInfo.orientation);

	// exif Make and Model
	mCallbackNotifier.setExifMake(mCameraConfig->getExifMake());
	mCallbackNotifier.setExifModel(mCameraConfig->getExifModel());

	LOGD("........................... to do initDefaultParameters");
	// for USB camera
	if (mHalCameraInfo.is_uvc)
	{
		// preview, picture, video size
		char sizeStr[256];
		mV4L2CameraDevice->enumSize(sizeStr, 256);
		LOGV("enum size: %s", sizeStr);
		if (strlen(sizeStr) > 0)
		{
			p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, sizeStr);
			p.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, sizeStr);
		}
		else
		{
			p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, "1280x720");
			p.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, "1280x720");
		}
		p.set(CameraParameters::KEY_PREVIEW_SIZE, "1280x720");
		p.set(CameraParameters::KEY_PICTURE_SIZE, "1280x720");
		p.set(CameraParameters::KEY_VIDEO_SIZE, "1280x720");

		// add for android CTS
		p.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES, CameraParameters::FOCUS_MODE_AUTO);
		p.set(CameraParameters::KEY_FOCUS_MODE, CameraParameters::FOCUS_MODE_AUTO);
		p.set(CameraParameters::KEY_FOCUS_AREAS, "(0,0,0,0,0)");
		p.set(CameraParameters::KEY_FOCAL_LENGTH, "3.43");
		mCallbackNotifier.setFocalLenght(3.43);
		p.set(CameraParameters::KEY_FOCUS_DISTANCES, "0.10,1.20,Infinity");

		// fps
		p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES, "30");
		p.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, "5000,60000");				// add temp for CTS
		p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, "(5000,60000)");	// add temp for CTS
		p.set(CameraParameters::KEY_PREVIEW_FRAME_RATE, "30");
		mV4L2CameraDevice->setFrameRate(30);

		// exposure compensation
		p.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, "0");
		p.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, "0");
		p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, "0");
		p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, "0");
		
		goto COMMOM_PARAMS;
	}

	// fast picture mode
	if (mHalCameraInfo.fast_picture_mode)
	{
		// capture size of picture-mode preview
		// get full size from the driver, to do
		mFullSizeWidth = 2592;
		mFullSizeHeight = 1936;

		mV4L2CameraDevice->getFullSize(&mFullSizeWidth, &mFullSizeHeight);
        
        value = mCameraConfig->defaultPreviewSizeValue();
        p.set(CameraParameters::KEY_SUBCHANNEL_SIZE, value);
		// any other differences ??
	}
	
	// preview size
	LOGV("init preview size");
	value = mCameraConfig->supportPreviewSizeValue();
	p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, value);
	LOGV("supportPreviewSizeValue: [%s] %s", CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, value);

#ifdef USE_NEW_MODE
	// KEY_SUPPORTED_VIDEO_SIZES and KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO should be set
	// at the same time, or both NULL. 
	// At present, we use preview and video the same size. Next version, maybe different.
	// They are different now.

	// video size
	value = mCameraConfig->supportVideoSizeValue();
	p.set(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES, value);
    value = mCameraConfig->defaultPreviewSizeValue();
	p.set(CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO, value);
#endif

	value = mCameraConfig->defaultPreviewSizeValue();
	p.set(CameraParameters::KEY_PREVIEW_SIZE, value);
    value = mCameraConfig->defaultVideoSizeValue();
	p.set(CameraParameters::KEY_VIDEO_SIZE, value);
	
	// picture size
	LOGV("to init picture size");
	value = mCameraConfig->supportPictureSizeValue();
	p.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, value);
	LOGV("supportPictureSizeValue: [%s] %s", CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, value);

	value = mCameraConfig->defaultPictureSizeValue();
	p.set(CameraParameters::KEY_PICTURE_SIZE, value);

	// frame rate
	LOGV("to init frame rate");
	value = mCameraConfig->supportFrameRateValue();
	p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES, value);
	LOGV("supportFrameRateValue: [%s] %s", CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES, value);

	p.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, "5000,60000");				// add temp for CTS
	p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, "(5000,60000)");	// add temp for CTS

	value = mCameraConfig->defaultFrameRateValue();
	p.set(CameraParameters::KEY_PREVIEW_FRAME_RATE, value);

	mV4L2CameraDevice->setFrameRate(atoi(value));

	// focus
	LOGV("to init focus");
	if (mCameraConfig->supportFocusMode())
	{
		value = mCameraConfig->supportFocusModeValue();
		p.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES, value);
		value = mCameraConfig->defaultFocusModeValue();
		p.set(CameraParameters::KEY_FOCUS_MODE, value);
		p.set(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS,"1");
	}
	else
	{
		// add for CTS
		p.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES, CameraParameters::FOCUS_MODE_FIXED);
		p.set(CameraParameters::KEY_FOCUS_MODE, CameraParameters::FOCUS_MODE_FIXED);
	}
	p.set(CameraParameters::KEY_FOCUS_AREAS, "(0,0,0,0,0)");
	p.set(CameraParameters::KEY_FOCAL_LENGTH, "3.43");
	mCallbackNotifier.setFocalLenght(3.43);
	p.set(CameraParameters::KEY_FOCUS_DISTANCES, "0.10,1.20,Infinity");

	
	// color effect 
	LOGV("to init color effect");
	if (mCameraConfig->supportColorEffect())
	{
		value = mCameraConfig->supportColorEffectValue();
		p.set(CameraParameters::KEY_SUPPORTED_EFFECTS, value);
		value = mCameraConfig->defaultColorEffectValue();
		p.set(CameraParameters::KEY_EFFECT, value);
	}

	// flash mode
	LOGV("to init flash mode");
	if (mCameraConfig->supportFlashMode())
	{
		value = mCameraConfig->supportFlashModeValue();
		p.set(CameraParameters::KEY_SUPPORTED_FLASH_MODES, value);
		value = mCameraConfig->defaultFlashModeValue();
		p.set(CameraParameters::KEY_FLASH_MODE, value);
	}

	// scene mode
	LOGV("to init scene mode");
	if (mCameraConfig->supportSceneMode())
	{
		value = mCameraConfig->supportSceneModeValue();
		p.set(CameraParameters::KEY_SUPPORTED_SCENE_MODES, value);
		value = mCameraConfig->defaultSceneModeValue();
		p.set(CameraParameters::KEY_SCENE_MODE, value);
	}

	// white balance
	LOGV("to init white balance");
	if (mCameraConfig->supportWhiteBalance())
	{
		value = mCameraConfig->supportWhiteBalanceValue();
		p.set(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE, value);
		value = mCameraConfig->defaultWhiteBalanceValue();
		p.set(CameraParameters::KEY_WHITE_BALANCE, value);
	}

	// exposure compensation
	LOGV("to init exposure compensation");
	if (mCameraConfig->supportExposureCompensation())
	{
		value = mCameraConfig->minExposureCompensationValue();
		p.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, value);

		value = mCameraConfig->maxExposureCompensationValue();
		p.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, value);

		value = mCameraConfig->stepExposureCompensationValue();
		p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, value);

		value = mCameraConfig->defaultExposureCompensationValue();
		p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, value);
	}
	else
	{
		p.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, "0");
		p.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, "0");
		p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, "0");
		p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, "0");
	}

COMMOM_PARAMS:
	// zoom
	LOGV("to init zoom");
	p.set(CameraParameters::KEY_ZOOM_SUPPORTED, "true");
	p.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, "false");
	p.set(CameraParameters::KEY_MAX_ZOOM, "100");

	int max_zoom = 100;
	char zoom_ratios[1024];
	memset(zoom_ratios, 0, 1024);
	for (int i = 0; i <= max_zoom; i++)
	{
		int i_ratio = 200 * i / max_zoom + 100;
		char str[8];
		sprintf(str, "%d,", i_ratio);
		strcat(zoom_ratios, str);
	}
	int len = strlen(zoom_ratios);
	zoom_ratios[len - 1] = 0;
	LOGV("zoom_ratios: %s", zoom_ratios);
	p.set(CameraParameters::KEY_ZOOM_RATIOS, zoom_ratios);
	p.set(CameraParameters::KEY_ZOOM, "0");
	
	mV4L2CameraDevice->setCrop(BASE_ZOOM, max_zoom);

	mZoomRatio = 100;
	
	// for some apps
//	if (strcmp(mCallingProcessName, "com.android.facelock") == 0)
//	{
//		p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, "160x120");
//		p.set(CameraParameters::KEY_PREVIEW_SIZE, "160x120");
//	}

	// capture size
	const char *parm = p.get(CameraParameters::KEY_PREVIEW_SIZE);
	parse_pair(parm, &mCaptureWidth, &mCaptureHeight, 'x');
	LOGV("default preview size: %dx%d", mCaptureWidth, mCaptureHeight);
	// default callback size
	mCallbackNotifier.setCBSize(mCaptureWidth, mCaptureHeight);

    int tryCaptureWidth = mCaptureWidth;
    int tryCaptureHeight = mCaptureHeight;
	mV4L2CameraDevice->tryFmtSize(&tryCaptureWidth, &tryCaptureHeight);
    if(tryCaptureWidth != mCaptureWidth || tryCaptureHeight != mCaptureHeight)
    {
//        LOGE("fatal error! camera not support videoSize[%dx%d], similar supported videoSize is [%dx%d]\n"
//            "camera.cfg is wrong, modify it. We don't do fault tolerance!", 
//            mCaptureWidth, mCaptureHeight, tryCaptureWidth, tryCaptureHeight);
    }
//    mVideoCaptureWidth = mCaptureWidth;
//    mVideoCaptureHeight= mCaptureHeight;
//	p.set(KEY_CAPTURE_SIZE_WIDTH, mCaptureWidth);
//	p.set(KEY_CAPTURE_SIZE_HEIGHT, mCaptureHeight);
//	p.set(KEY_PREVIEW_CAPTURE_SIZE_WIDTH, mCaptureWidth);
//	p.set(KEY_PREVIEW_CAPTURE_SIZE_HEIGHT, mCaptureHeight);

	// preview formats, CTS must support at least 2 formats
	parameterString = CameraParameters::PIXEL_FORMAT_YUV420SP;			// NV21, default
	parameterString.append(",");
	parameterString.append(CameraParameters::PIXEL_FORMAT_YUV420P);		// YV12
	p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS, parameterString.string());
	
	p.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT, CameraParameters::PIXEL_FORMAT_YUV420SP);
	p.set(CameraParameters::KEY_PREVIEW_FORMAT, CameraParameters::PIXEL_FORMAT_YUV420SP);

    p.set(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS, CameraParameters::PIXEL_FORMAT_JPEG);
	
	p.set(CameraParameters::KEY_JPEG_QUALITY, "95"); // maximum quality
	p.set(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES, "320x240,0x0");
	p.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, "320");
	p.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, "240");
	p.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, "90");
	
	p.setPictureFormat(CameraParameters::PIXEL_FORMAT_JPEG);

	mCallbackNotifier.setJpegThumbnailSize(320, 240);

	// record hint
	p.set(CameraParameters::KEY_RECORDING_HINT, "false");

	// rotation
	p.set(CameraParameters::KEY_ROTATION, 0);
		
	// add for CTS
	p.set(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE, "54.6");
    p.set(CameraParameters::KEY_VERTICAL_VIEW_ANGLE, "39.4");

	p.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW, 0);
	p.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_SW, 0);
	
	// take picture in video mode
	p.set(CameraParameters::KEY_VIDEO_SNAPSHOT_SUPPORTED, "true");

    LOGV("set video buffer number to %d", DEFAULT_BUFFER_NUM);
    p.setVideoCaptureBufferNum(DEFAULT_BUFFER_NUM);
    mV4L2CameraDevice->setVideoCaptureBufferNum(DEFAULT_BUFFER_NUM);

    LOGV("set continuous picture number to %d", DEFAULT_CONTINUOUS_PICTURE_NUMBER);
    p.setContinuousPictureNumber(DEFAULT_CONTINUOUS_PICTURE_NUMBER);
    mCallbackNotifier.setContinuousPictureNumber(DEFAULT_CONTINUOUS_PICTURE_NUMBER);
    mV4L2CameraDevice->setContinuousPictureNumber(DEFAULT_CONTINUOUS_PICTURE_NUMBER);

    LOGV("set continuous picture interval to %d", DEFAULT_CONTINUOUS_PICTURE_INTERVAL);
    p.setContinuousPictureIntervalMs(DEFAULT_CONTINUOUS_PICTURE_INTERVAL);
    mV4L2CameraDevice->setContinuousPictureInterval(DEFAULT_CONTINUOUS_PICTURE_INTERVAL);

    p.setPreviewRotation(0);
    mV4L2CameraDevice->setPreviewRotation(0);

	mParameters = p;

	mFirstSetParameters = true;

	mLastFocusAreas.x1 = -1000;
	mLastFocusAreas.y1 = -1000;
	mLastFocusAreas.x2 = -1000;
	mLastFocusAreas.y2 = -1000;

	LOGV("CameraHardware::initDefaultParameters ok");
}

void CameraHardware::onCameraDeviceError(int err)
{
	LOGV();
    /* Errors are reported through the callback notifier */
    mCallbackNotifier.onCameraDeviceError(err);
}

/****************************************************************************
 * Camera API implementation.
 ***************************************************************************/

status_t CameraHardware::setCameraHardwareInfo(HALCameraInfo * halInfo)
{
	// check input params
	if (halInfo == NULL)
	{
		LOGE("ERROR camera info");
		return EINVAL;
	}
	
	memcpy((void*)&mHalCameraInfo, (void*)halInfo, sizeof(HALCameraInfo));

	return NO_ERROR;
}

bool CameraHardware::isCameraIdle()
{
	Mutex::Autolock locker(&mCameraIdleLock);
	return mIsCameraIdle;
}

status_t CameraHardware::connectCamera(hw_device_t** device)
{
	LOGV();
    status_t res = EINVAL;
	
	{
		Mutex::Autolock locker(&mCameraIdleLock);
		mIsCameraIdle = false;
	}

	if (mV4L2CameraDevice != NULL)
	{
		res = mV4L2CameraDevice->connectDevice(&mHalCameraInfo);
		if (res == NO_ERROR)
		{
			*device = &common;

			if (mCameraConfig->supportFocusMode())
			{
				mV4L2CameraDevice->setAutoFocusInit();
				mV4L2CameraDevice->setExposureMode(V4L2_EXPOSURE_AUTO);
				
				// start focus thread
				mAutoFocusThread->startThread();
			}
		}
		else
		{
			Mutex::Autolock locker(&mCameraIdleLock);
			mIsCameraIdle = true;
		}
	}

    return -res;
}

status_t CameraHardware::closeCamera()
{
    LOGV();
	
    return cleanupCamera();
}

status_t CameraHardware::setPreviewWindow(struct preview_stream_ops* window)
{
	LOGV();
    /* Callback should return a negative errno. */
	return -mPreviewWindow.setPreviewWindow(window);
}

void CameraHardware::setCallbacks(camera_notify_callback notify_cb,
                                  camera_data_callback data_cb,
                                  camera_data_timestamp_callback data_cb_timestamp,
                                  camera_request_memory get_memory,
                                  void* user)
{
	LOGV();
    mCallbackNotifier.setCallbacks(notify_cb, data_cb, data_cb_timestamp,
                                    get_memory, user);
}

void CameraHardware::enableMsgType(int32_t msg_type)
{
    mCallbackNotifier.enableMessage(msg_type);
}

void CameraHardware::disableMsgType(int32_t msg_type)
{
    mCallbackNotifier.disableMessage(msg_type);
}

int CameraHardware::isMsgTypeEnabled(int32_t msg_type)
{
    return mCallbackNotifier.isMessageEnabled(msg_type);
}

status_t CameraHardware::startPreview()
{
	LOGV();
    Mutex::Autolock locker(&mObjectLock);
    /* Callback should return a negative errno. */
    return -doStartPreview();
}

void CameraHardware::stopPreview()
{
	LOGV();

#if 0  // face
	mQueueElement[CMD_QUEUE_STOP_FACE_DETECTE].cmd = CMD_QUEUE_STOP_FACE_DETECTE;
	OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_STOP_FACE_DETECTE]);
#endif

#ifdef ADAS_ENABLE
	mQueueElement[CMD_QUEUE_ADAS_STOP_DETECT].cmd = CMD_QUEUE_ADAS_STOP_DETECT;
	OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_ADAS_STOP_DETECT]);
#endif
#ifdef MOTION_DETECTION_ENABLE
	mQueueElement[CMD_QUEUE_STOP_AWMD].cmd = CMD_QUEUE_STOP_AWMD;
	OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_STOP_AWMD]);
#endif
	pthread_cond_signal(&mCommandCond);
#ifdef WATERMARK_ENABLE
    mV4L2CameraDevice->setWaterMark(0, NULL);
#endif
	
    Mutex::Autolock locker(&mObjectLock);
    doStopPreview();
}

int CameraHardware::isPreviewEnabled()
{
	LOGV();
    return mPreviewWindow.isPreviewEnabled();
}

status_t CameraHardware::storeMetaDataInBuffers(int enable)
{
	LOGV();
    /* Callback should return a negative errno. */
    return -mCallbackNotifier.storeMetaDataInBuffers(enable);
}

status_t CameraHardware::startRecording()
{
	LOGV();
	
	// video hint
    const char * valstr = mParameters.get(CameraParameters::KEY_RECORDING_HINT);
    if (valstr) 
	{
		LOGV("KEY_RECORDING_HINT: %s -> true", valstr);
    }

	mParameters.set(CameraParameters::KEY_AWEXTEND_SNAP_PATH, "");
	mCallbackNotifier.setSnapPath("");

	mParameters.set(CameraParameters::KEY_RECORDING_HINT, CameraParameters::sTRUE);
	
	if (mCameraConfig->supportFocusMode())
	{
		mV4L2CameraDevice->set3ALock(V4L2_LOCK_FOCUS);
	}
    int nVideoWidth = 0;
	int nVideoHeight = 0;
    mParameters.getVideoSize(&nVideoWidth, &nVideoHeight);
	LOGV("VideoCapture:%dx%d, Capture:%dx%d", nVideoWidth, nVideoHeight, mCaptureWidth, mCaptureHeight);
	if (nVideoWidth != mCaptureWidth || nVideoHeight != mCaptureHeight)
	{
        LOGW("Be careful!videoSize[%dx%d] != curCaptureSize[%dx%d], restartPreview!", nVideoWidth, nVideoHeight, mCaptureWidth, mCaptureHeight);
		doStopPreview();
		doStartPreview();
	}
	
    /* Callback should return a negative errno. */
    return -mCallbackNotifier.enableVideoRecording();
}

void CameraHardware::stopRecording()
{
	LOGV();
    mCallbackNotifier.disableVideoRecording();
    mParameters.set(CameraParameters::KEY_RECORDING_HINT, CameraParameters::sFALSE);
    LOGD("not need set mV4L2CameraDevice hwEncoder false, so delete it.");
	//mV4L2CameraDevice->setHwEncoder(false);
	
	if (mCameraConfig->supportFocusMode())
	{
		mV4L2CameraDevice->set3ALock(~V4L2_LOCK_FOCUS);
	}
}

int CameraHardware::isRecordingEnabled()
{
	LOGV();
    return mCallbackNotifier.isVideoRecordingEnabled();
}

void CameraHardware::releaseRecordingFrame(const void* opaque)
{
	if (mUseHwEncoder)
	{
    	mV4L2CameraDevice->releasePreviewFrame(*(int*)opaque);
	}
}

/****************************************************************************
 * Focus 
 ***************************************************************************/

status_t CameraHardware::setAutoFocus()
{
	// start singal focus
	int ret = 0;
	const char *new_focus_mode_str = mParameters.get(CameraParameters::KEY_FOCUS_MODE);

	if (!mCameraConfig->supportFocusMode())
	{
		mQueueElement[CMD_QUEUE_SET_FOCUS_STATUS].cmd = CMD_QUEUE_SET_FOCUS_STATUS;
		OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_SET_FOCUS_STATUS]);
		pthread_cond_signal(&mCommandCond);
		
		return OK;
	}
	
	pthread_mutex_lock(&mAutoFocusMutex);
	
	if (!strcmp(new_focus_mode_str, CameraParameters::FOCUS_MODE_INFINITY)
		|| !strcmp(new_focus_mode_str, CameraParameters::FOCUS_MODE_FIXED))
	{
		// do nothing
	}
	else
	{
		mV4L2CameraDevice->set3ALock(~(V4L2_LOCK_FOCUS | V4L2_LOCK_EXPOSURE| V4L2_LOCK_WHITE_BALANCE));
		mV4L2CameraDevice->setAutoFocusStart();
	}

	mIsSingleFocus = true;
	pthread_mutex_unlock(&mAutoFocusMutex);

	return OK;
}

status_t CameraHardware::cancelAutoFocus()
{
	LOGV();

	/* TODO: Future enhancements. */
	return NO_ERROR;
}

int CameraHardware::parse_focus_areas(const char *str, bool is_face)
{
	int ret = -1;
	char *ptr,*tmp;
	char p1[6] = {0}, p2[6] = {0};
	char p3[6] = {0}, p4[6] = {0}, p5[6] = {0};
	int  l,t,r,b;
	int  w,h;

	if (str == NULL
		|| !mCameraConfig->supportFocusMode())
	{
		return 0;
	}

	// LOGV("parse_focus_areas : %s", str);

	tmp = strchr(str,'(');
	tmp++;
	ptr = strchr(tmp,',');
	memcpy(p1,tmp,ptr-tmp);
	
	tmp = ptr+1;
	ptr = strchr(tmp,',');
	memcpy(p2,tmp,ptr-tmp);

	tmp = ptr+1;
	ptr = strchr(tmp,',');
	memcpy(p3,tmp,ptr-tmp);

	tmp = ptr+1;
	ptr = strchr(tmp,',');
	memcpy(p4,tmp,ptr-tmp);

	tmp = ptr+1;
	ptr = strchr(tmp,')');
	memcpy(p5,tmp,ptr-tmp);

	l = atoi(p1);
	t = atoi(p2);
	r = atoi(p3);
	b = atoi(p4);

	w = l + (r-l)/2;
	h = t + (b-t)/2;

	struct v4l2_win_coordinate win;
	win.x1 = l;
	win.y1 = t;
	win.x2 = r;
	win.y2 = b;
	if (abs(mLastFocusAreas.x1 - win.x1) >= 40
		|| abs(mLastFocusAreas.y1 - win.y1) >= 40
		|| abs(mLastFocusAreas.x2 - win.x2) >= 40
		|| abs(mLastFocusAreas.y2 - win.y2) >= 40)
	{
		if (!is_face && (mZoomRatio != 0))
		{
			win.x1 = win.x1 * 100 / mZoomRatio;
			win.y1 = win.y1 * 100 / mZoomRatio;
			win.x2 = win.x2 * 100 / mZoomRatio;
			win.y2 = win.y2 * 100 / mZoomRatio;
		}

		LOGV("mZoomRatio: %d, v4l2_win_coordinate: [%d,%d,%d,%d]", 
			mZoomRatio, win.x1, win.y1, win.x2, win.y2);
		
		if ((l == 0) && (t == 0) && (r == 0) && (b == 0))
		{
			mV4L2CameraDevice->set3ALock(~(V4L2_LOCK_FOCUS | V4L2_LOCK_EXPOSURE | V4L2_LOCK_WHITE_BALANCE));
			setAutoFocusRange();
			mV4L2CameraDevice->setAutoFocusWind(0, (void*)&win);
			mV4L2CameraDevice->setExposureWind(0, (void*)&win);
		}
		else
		{
			mV4L2CameraDevice->setAutoFocusWind(1, (void*)&win);
			mV4L2CameraDevice->setExposureWind(1, (void*)&win);
		}
		mLastFocusAreas.x1 = win.x1;
		mLastFocusAreas.y1 = win.y1;
		mLastFocusAreas.x2 = win.x2;
		mLastFocusAreas.y2 = win.y2;
	}
	
	return 0;
}

int CameraHardware::setAutoFocusRange()
{
	LOGV();

	v4l2_auto_focus_range af_range = V4L2_AUTO_FOCUS_RANGE_INFINITY;
    if (mCameraConfig->supportFocusMode())
	{
	    // focus
		const char *new_focus_mode_str = mParameters.get(CameraParameters::KEY_FOCUS_MODE);
		if (!strcmp(new_focus_mode_str, CameraParameters::FOCUS_MODE_AUTO))
		{
			af_range = V4L2_AUTO_FOCUS_RANGE_AUTO;
		}
		else if (!strcmp(new_focus_mode_str, CameraParameters::FOCUS_MODE_INFINITY))
		{
			af_range = V4L2_AUTO_FOCUS_RANGE_INFINITY;
		}
		else if (!strcmp(new_focus_mode_str, CameraParameters::FOCUS_MODE_MACRO))
		{
			af_range = V4L2_AUTO_FOCUS_RANGE_MACRO;
		}
		else if (!strcmp(new_focus_mode_str, CameraParameters::FOCUS_MODE_FIXED))
		{
			af_range = V4L2_AUTO_FOCUS_RANGE_INFINITY;
		}
		else if (!strcmp(new_focus_mode_str, CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE)
					|| !strcmp(new_focus_mode_str, CameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO))
		{
			af_range = V4L2_AUTO_FOCUS_RANGE_AUTO;
		}
		else
		{
			return -EINVAL;
		}
	}
	else
	{
		af_range = V4L2_AUTO_FOCUS_RANGE_INFINITY;
	}
	
	mV4L2CameraDevice->setAutoFocusRange(af_range);
	
	return OK;
}

bool CameraHardware::checkFocusMode(const char * mode)
{
	const char * str = mParameters.get(CameraParameters::KEY_SUPPORTED_FOCUS_MODES);
	if (!strstr(str, mode))
	{
		return false;
	}
	return true;
}

bool CameraHardware::checkFocusArea(const char * area)
{
	if (area == NULL)
	{
		return false;
	}

	int i = 0;
	int arr[5];		// [l, t, r, b, w]
	char temp[128];
	strcpy(temp, area);
	char *pval = temp;
	const char *seps = "(,)";
	int offset = 0;
	pval = strtok(pval, seps);
	while (pval != NULL)
	{
		if (i >= 5)
		{
			return false;
		}
		arr[i++] = atoi(pval);
		pval = strtok(NULL, seps);
	}

	LOGV("(%d, %d, %d, %d, %d)", arr[0], arr[1],arr[2],arr[3],arr[4]);
	
	if ((arr[0] == 0)
		&& (arr[1] == 0)
		&& (arr[2] == 0)
		&& (arr[3] == 0)
		&& (arr[4] == 0))
	{
		return true;
	}
	
	if (arr[4] < 1)
	{
		return false;
	}
	
	for(i = 0; i < 4; i++)
	{
		if ((arr[i] < -1000) || (arr[i] > 1000))
		{
			return false;
		}
	}

	if ((arr[0] >= arr[2])
		|| (arr[1] >= arr[3]))
	{
		return false;
	}
	
	return true;
}

/****************************************************************************
 * take picture management
 ***************************************************************************/

status_t CameraHardware::doTakePicture()
{
	LOGV();
    Mutex::Autolock locker(&mObjectLock);

    /* Get JPEG quality. */
    int jpeg_quality = mParameters.getInt(CameraParameters::KEY_JPEG_QUALITY);
    if (jpeg_quality <= 0) {
        jpeg_quality = 90;
    }
    mCallbackNotifier.setJpegQuality(jpeg_quality);

    /* Get JPEG rotate. */
    int jpeg_rotate = mParameters.getInt(CameraParameters::KEY_ROTATION);
    if (jpeg_rotate <= 0) {
        jpeg_rotate = 0;  /* Fall back to default. */
    }
    mCallbackNotifier.setJpegRotate(jpeg_rotate);

    /* Get JPEG size. */
    int pic_width, pic_height;
    mParameters.getPictureSize(&pic_width, &pic_height);
    mCallbackNotifier.setPictureSize(pic_width, pic_height);

	// if in recording mode or in picture_mode_fast
	const char * valstr = mParameters.get(CameraParameters::KEY_RECORDING_HINT);
	bool video_hint = (strcmp(valstr, CameraParameters::sTRUE) == 0);
    bool bPictureModeFast = (0==strcmp(mParameters.getPictureMode(), CameraParameters::AWEXTEND_PICTURE_MODE_FAST));
	if (video_hint || bPictureModeFast)
	{
		//if (strcmp(mCallingProcessName, "com.android.cts.stub"))
		//{
			// not cts
            int nPictureSizeMode = mParameters.getPictureSizeMode();
			if(nPictureSizeMode == CameraParameters::AWEXTEND_PICTURE_SIZE_MODE::KeepSourceSize)
			{
			    mCallbackNotifier.setPictureSize(mCaptureWidth, mCaptureHeight);
			}
            else if(nPictureSizeMode == CameraParameters::AWEXTEND_PICTURE_SIZE_MODE::UseParameterPictureSize)
            {
                mCallbackNotifier.setPictureSize(pic_width, pic_height);
            }
            else
            {
                LOGE("fatal error! unknown picture size mode[%d]", nPictureSizeMode);
            }
		//}
        if(video_hint)
        {
		    mV4L2CameraDevice->setTakePictureState(TAKE_PICTURE_RECORD);
        }
        else if(bPictureModeFast)
        {
            mV4L2CameraDevice->setTakePictureState(TAKE_PICTURE_FAST);
        }
		return OK;
	}

	// picture mode
	const char * cur_picture_mode = mParameters.getPictureMode();
	if (cur_picture_mode != NULL)
	{
		// continuous picture
		if (!strcmp(cur_picture_mode, CameraParameters::AWEXTEND_PICTURE_MODE_CONTINUOUS)
			|| !strcmp(cur_picture_mode, CameraParameters::AWEXTEND_PICTURE_MODE_CONTINUOUS_FAST))
		{
			// test continuous picture
			if (!strcmp(cur_picture_mode, CameraParameters::AWEXTEND_PICTURE_MODE_CONTINUOUS))
			{
				mV4L2CameraDevice->setTakePictureState(TAKE_PICTURE_CONTINUOUS);
			}
			else if (!strcmp(cur_picture_mode, CameraParameters::AWEXTEND_PICTURE_MODE_CONTINUOUS_FAST))
			{
				mV4L2CameraDevice->setTakePictureState(TAKE_PICTURE_CONTINUOUS_FAST);
			}
			mCallbackNotifier.setPictureSize(pic_width, pic_height);
			mCallbackNotifier.startContinuousPicture();
			mV4L2CameraDevice->startContinuousPicture();

			return OK;
		}
	}

	// normal picture mode

	// full-size capture
	//if (mHalCameraInfo.fast_picture_mode/* && mIsImageCaptureIntent == false*/)
	//{
	//	mV4L2CameraDevice->setTakePictureState(TAKE_PICTURE_FAST);
	//	return OK;
	//}

	// normal taking picture mode
	int frame_width = pic_width;
	int frame_height = pic_height;
	if (mV4L2CameraDevice->tryFmtSize(&frame_width, &frame_height) != OK)
	{
		LOGW("Not support capture size[%dx%d], use fast picture mode", frame_width, frame_height);
		mV4L2CameraDevice->setTakePictureState(TAKE_PICTURE_FAST);
		return OK;
	}
    if (frame_width != pic_width || frame_height != pic_height) {
        LOGD("tryFmtSize[%dx%d] != picSize[%dx%d], getFullSize!", frame_width, frame_height, pic_width, pic_height);
    	if (mV4L2CameraDevice->getFullSize(&frame_width, &frame_height) != OK)
    	{
    		LOGW("getFullSize[%dx%d] failed, use fast picture mode", frame_width, frame_height);
    		mV4L2CameraDevice->setTakePictureState(TAKE_PICTURE_FAST);
    		return OK;
    	}
    }
	if (mCaptureWidth == frame_width && mCaptureHeight == frame_height/* && mIsImageCaptureIntent == false*/)
	{
		LOGV("current capture size[%dx%d] is the same as picture size", frame_width, frame_height);
		mV4L2CameraDevice->setTakePictureState(TAKE_PICTURE_FAST);
		return OK;
	}

	mV4L2CameraDevice->setTakePictureCtrl();

	// preview format and video format are the same
	const char* pix_fmt = mParameters.getPictureFormat();
#ifdef __SUNXI__
    LOGD("__SUNXI__, use V4L2_PIX_FMT_NV12");
	uint32_t org_fmt = V4L2_PIX_FMT_NV12;		// SGX support NV12
#else
	uint32_t org_fmt = V4L2_PIX_FMT_NV21;		// MALI support NV21
#endif

	/*
     * Make sure preview is not running, and device is stopped before taking
     * picture.
     */
    const bool preview_on = mPreviewWindow.isPreviewEnabled();
    if (preview_on) {
        doStopPreview();
    }

	/* Camera device should have been stopped when the shutter message has been
	 * enabled. */
	if (mV4L2CameraDevice->isStarted()) 
	{
		LOGW("Camera device is started");
		mV4L2CameraDevice->stopDeliveringFrames();
		mV4L2CameraDevice->stopDevice();
	}

	/*
	 * Take the picture now.
	 */
	
	mV4L2CameraDevice->setTakePictureState(TAKE_PICTURE_NORMAL);

	mCaptureWidth = frame_width;
	mCaptureHeight = frame_height;

	LOGD("Starting camera: %dx%d -> %.4s(%s)",
		 mCaptureWidth, mCaptureHeight, reinterpret_cast<const char*>(&org_fmt), pix_fmt);

    int sub_w = 0, sub_h = 0;
    mParameters.getSubChannelSize(&sub_w, &sub_h);
    int framerate = mParameters.getPreviewFrameRate();
	status_t res = mV4L2CameraDevice->startDevice(mCaptureWidth, mCaptureHeight, sub_w, sub_h, org_fmt, 15, true);
	if (res != NO_ERROR) 
	{
		if (preview_on) {
            doStartPreview();
        }
		return res;
	}

	int format = mV4L2CameraDevice->getVideoFormat();
	mParameters.set(CameraParameters::KEY_CAPTURE_OUT_FORMAT, format);
	mV4L2CameraDevice->showformat(format,"CameraHardware-s2");
	
	res = mV4L2CameraDevice->startDeliveringFrames();
	if (res != NO_ERROR) 
	{
		mV4L2CameraDevice->setTakePictureState(TAKE_PICTURE_NULL);
        if (preview_on) {
            doStartPreview();
        }
	}

	return OK;
}

status_t CameraHardware::doTakePictureEnd()
{
	LOGV("isPreviewEnabled=%d", mPreviewWindow.isPreviewEnabled());
	
    Mutex::Autolock locker(&mObjectLock);

	if (mV4L2CameraDevice->isConnected() 			// camera is connected or started
		&& !mPreviewWindow.isPreviewEnabled()		// preview is not enable
		/*&& !mHalCameraInfo.fast_picture_mode*/)
	{
		LOGV("doTakePictureEnd to doStartPreview");
		doStartPreview();
	}

	return OK;
}

status_t CameraHardware::takePicture()
{
	LOGV();
	mQueueElement[CMD_QUEUE_TAKE_PICTURE].cmd = CMD_QUEUE_TAKE_PICTURE;
	OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_TAKE_PICTURE]);
	pthread_cond_signal(&mCommandCond);
	
    return OK;
}

status_t CameraHardware::cancelPicture()
{
	LOGV();
	mV4L2CameraDevice->setTakePictureState(TAKE_PICTURE_NULL);

    return NO_ERROR;
}

// 
void CameraHardware::notifyPictureMsg(const void* frame)
{
	/*
	pthread_mutex_lock(&mCommandMutex);
	mQueueElement[CMD_QUEUE_PICTURE_MSG].cmd = CMD_QUEUE_PICTURE_MSG;
	mQueueElement[CMD_QUEUE_PICTURE_MSG].data = (int)frame;
	OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_PICTURE_MSG]);
	pthread_cond_signal(&mCommandCond);
	pthread_mutex_unlock(&mCommandMutex);
	*/
	mCallbackNotifier.notifyPictureMsg(frame);
	//if (strcmp(mCallingProcessName, "com.android.cts.stub") != 0
	//	&& strcmp(mCallingProcessName, "com.android.cts.mediastress") != 0
	//	&& mIsImageCaptureIntent == false)
	//{
		doTakePictureEnd();
	//}
}

/****************************************************************************
 * set and get parameters
 ***************************************************************************/
/*
void CameraHardware::setVideoCaptureSize(int video_w, int video_h)
{
	LOGD("setVideoCaptureSize next version to do ......");
	
	// video size is video_w x video_h, capture size may be different
	// now the same
	mVideoCaptureWidth = video_w;
	mVideoCaptureHeight= video_h;
	
	if (mHalCameraInfo.fast_picture_mode)
	{
		if (mVideoCaptureWidth == 640)
		{
			mVideoCaptureWidth = mVideoCaptureWidth * 2;
			mVideoCaptureHeight= mVideoCaptureHeight * 2;
		}
	}

	int videoCaptureWidth = mVideoCaptureWidth;
	int videoCaptureHeight =mVideoCaptureHeight;
	
	int ret = mV4L2CameraDevice->tryFmtSize(&videoCaptureWidth, &videoCaptureHeight);
	if(ret < 0)
	{
		LOGE("setVideoCaptureSize tryFmtSize failed");
		return;
	}

	float widthRate = (float)videoCaptureWidth / (float)mVideoCaptureWidth;
	float heightRate = (float)videoCaptureHeight / (float)mVideoCaptureHeight;

	if((widthRate > 4.0) && (heightRate > 4.0))
	{
		mV4L2CameraDevice->setThumbUsedForVideo(true);
		mV4L2CameraDevice->setVideoSize(video_w, video_h);
		mVideoCaptureWidth = videoCaptureWidth;
		mVideoCaptureHeight=videoCaptureHeight;
//		mParameters.set(KEY_CAPTURE_SIZE_WIDTH, videoCaptureWidth / 2);
//		mParameters.set(KEY_CAPTURE_SIZE_HEIGHT, videoCaptureHeight / 2);
	}
	else
	{
		mV4L2CameraDevice->setThumbUsedForVideo(false);
		mVideoCaptureWidth = videoCaptureWidth;
		mVideoCaptureHeight=videoCaptureHeight;
//		mParameters.set(KEY_CAPTURE_SIZE_WIDTH, videoCaptureWidth);
//		mParameters.set(KEY_CAPTURE_SIZE_HEIGHT, videoCaptureHeight);
	}
}
*/

void CameraHardware::getCurrentOriention(int * oriention)
{
	*oriention = mOriention;
	
	if(mHalCameraInfo.facing == CAMERA_FACING_FRONT)   //for direction of front camera facedetection by fuqiang
	{
		if(*oriention == 90)
		{
			*oriention = 270;
		}
		else if(*oriention == 270)
		{
			*oriention = 90;
		}
	}
}

status_t CameraHardware::setParameters(const char* p)
{
	LOGV();
	int ret = UNKNOWN_ERROR;
	
    PrintParamDiff(mParameters, p);

    CameraParameters params;
	String8 str8_param(p);
    params.unflatten(str8_param);

	V4L2CameraDevice* pV4L2Device = mV4L2CameraDevice;
	if (pV4L2Device == NULL)
	{
		LOGE("getCameraDevice failed");
		return UNKNOWN_ERROR;
	}

#if 0
	// stop continuous picture
	const char * cur_picture_mode = mParameters.getPictureMode();
	const char * stop_continuous_picture = params.get(KEY_CANCEL_CONTINUOUS_PICTURE);
	LOGV("stop_continuous_picture : %s", stop_continuous_picture);
	if (cur_picture_mode != NULL
		&& stop_continuous_picture != NULL
		&& !strcmp(cur_picture_mode, CameraParameters::AWEXTEND_PICTURE_MODE_CONTINUOUS)
		&& !strcmp(stop_continuous_picture, "true")) 
	{
		mQueueElement[CMD_QUEUE_STOP_CONTINUOUSSNAP].cmd = CMD_QUEUE_STOP_CONTINUOUSSNAP;
		OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_STOP_CONTINUOUSSNAP]);
	}
#endif

	// picture mode
	const char * new_picture_mode = params.getPictureMode();
	LOGV("new_picture_mode : %s", new_picture_mode);
    if (new_picture_mode != NULL) 
	{
		if (!strcmp(new_picture_mode, CameraParameters::AWEXTEND_PICTURE_MODE_NORMAL)
            || !strcmp(new_picture_mode, CameraParameters::AWEXTEND_PICTURE_MODE_FAST)
			|| !strcmp(new_picture_mode, CameraParameters::AWEXTEND_PICTURE_MODE_CONTINUOUS)
			|| !strcmp(new_picture_mode, CameraParameters::AWEXTEND_PICTURE_MODE_CONTINUOUS_FAST))
		{
        	mParameters.set(CameraParameters::KEY_AWEXTEND_PICTURE_MODE, new_picture_mode);
		}
		else
		{
        	LOGW("unknown picture mode: %s", new_picture_mode);
		}
	
		// continuous picture path
		if (!strcmp(new_picture_mode, CameraParameters::AWEXTEND_PICTURE_MODE_CONTINUOUS)
			|| !strcmp(new_picture_mode, CameraParameters::AWEXTEND_PICTURE_MODE_CONTINUOUS_FAST))
		{
			const char * new_path = params.get(CameraParameters::KEY_AWEXTEND_CONTINUOUS_PICTURE_PATH);
			LOGV("new_path: %s", new_path);
			if (new_path != NULL)
			{
				mParameters.set(CameraParameters::KEY_AWEXTEND_CONTINUOUS_PICTURE_PATH, new_path);
				mCallbackNotifier.setSaveFolderPath(new_path);
			}
			//else
			//{
			//	LOGW("invalid path: %s in %s picture mode", new_path, new_picture_mode);
			//	mParameters.set(CameraParameters::KEY_AWEXTEND_PICTURE_MODE, CameraParameters::AWEXTEND_PICTURE_MODE_NORMAL);
			//}
		}
		else if(!strcmp(new_picture_mode, CameraParameters::AWEXTEND_PICTURE_MODE_NORMAL))
		{
			const char * new_path = params.get(CameraParameters::KEY_AWEXTEND_SNAP_PATH);
			LOGV("snap new_path: %s", new_path);
			if (new_path != NULL)
			{
				mParameters.set(CameraParameters::KEY_AWEXTEND_SNAP_PATH, new_path);
				mCallbackNotifier.setSnapPath(new_path);
			}
		}
    }

#if 0
	const char * is_imagecapture_intent = params.get(KEY_IS_IMAGECAPTURE_INTENT);
	if (is_imagecapture_intent != NULL) 
	{
		if(!strcmp(is_imagecapture_intent, "true"))
		{
			mIsImageCaptureIntent = true;
		}
		else
		{
			mIsImageCaptureIntent = false;
		}
	}
#endif
	// preview format
	const char * new_preview_format = params.getPreviewFormat();
	LOGV("new_preview_format : %s", new_preview_format);
	if (new_preview_format != NULL
		&& (strcmp(new_preview_format, CameraParameters::PIXEL_FORMAT_YUV420SP) == 0
		|| strcmp(new_preview_format, CameraParameters::PIXEL_FORMAT_YUV420P) == 0)) 
	{
		mParameters.setPreviewFormat(new_preview_format);
	}
	else
    {
        LOGE("Only yuv420sp or yuv420p preview is supported");
        return -EINVAL;
    }

	// picture format
	const char * new_picture_format = params.getPictureFormat();
	LOGV("new_picture_format : %s", new_picture_format);
	if (new_picture_format == NULL
		|| (strcmp(new_picture_format, CameraParameters::PIXEL_FORMAT_JPEG) != 0)) 
    {
        LOGE("Only jpeg still pictures are supported");
        return -EINVAL;
    }

	// picture size
	int new_picture_width  = 0;
    int new_picture_height = 0;
    params.getPictureSize(&new_picture_width, &new_picture_height);
    LOGV("new_picture_width x new_picture_height = %dx%d", new_picture_width, new_picture_height);
    if (0 < new_picture_width && 0 < new_picture_height) 
	{
		mParameters.setPictureSize(new_picture_width, new_picture_height);
    }
	else
	{
		LOGE("error picture size");
		return -EINVAL;
	}

    // preview size
    int new_preview_width = 0, new_preview_height = 0;
    int preview_width = 0, preview_height = 0;
    params.getPreviewSize(&new_preview_width, &new_preview_height);
    mParameters.getPreviewSize(&preview_width, &preview_height);
    LOGV("new_preview_width x new_preview_height = %dx%d", new_preview_width, new_preview_height);
    if (mFirstSetParameters || preview_width != new_preview_width || preview_height != new_preview_height) {
        if (new_preview_width == 0 || new_preview_height == 0) {
            mParameters.setPreviewSize(0, 0);
        } else {
            Size newPreviewSize(new_preview_width, new_preview_height);
            bool bPreviewSizeValid = checkPreviewSizeValid(newPreviewSize);
            if (bPreviewSizeValid)
            {
                mParameters.setPreviewSize(new_preview_width, new_preview_height);
                mCallbackNotifier.setCBSize(new_preview_width, new_preview_height);
            }
            else
            {
                int width = 0, height = 0;
                const char *val = mCameraConfig->defaultPreviewSizeValue();
                parse_pair(val, &width, &height, 'x');
                mParameters.setPreviewSize(width, height);
                mCallbackNotifier.setCBSize(width, height);
                LOGW("new preview size was not support, set to default[%dx%d]!", width, height);
            }
        }
    }

    // video size
    int new_video_width = 0, new_video_height = 0;
    int video_width = 0, video_height = 0;
    params.getVideoSize(&new_video_width, &new_video_height);
    mParameters.getVideoSize(&video_width, &video_height);
    LOGV("new_video_width x new_video_height = %dx%d", new_video_width, new_video_height);
    if (mFirstSetParameters || video_width != new_video_width || video_height != new_video_height)
    {
        Size newVideoSize(new_video_width, new_video_height);
        bool bVideoSizeValid = checkVideoSizeValid(newVideoSize);
        if (bVideoSizeValid)
        {
            mParameters.setVideoSize(new_video_width, new_video_height);
        }
        else
        {
            int width = 0, height = 0;
            const char *val = mCameraConfig->defaultVideoSizeValue();
            parse_pair(val, &width, &height, 'x');
            mParameters.setVideoSize(width, height);
            LOGW("new video size was not support, set to default[%dx%d]!", width, height);
        }
    }

//    int new_subchannel_width = 0;
//    int new_subchannel_height = 0;
//    params.getSubChannelSize(&new_subchannel_width, &new_subchannel_height);
//    if (new_subchannel_width > 0 && new_subchannel_width > 0)
//    {
//        int subchannel_width = 0;
//        int subchannel_height = 0;
//        mParameters.getSubChannelSize(&subchannel_width, &subchannel_height);
//        if (subchannel_width != new_subchannel_width || subchannel_height != new_subchannel_height) {
//            mParameters.setSubChannelSize(new_subchannel_width, new_subchannel_height);
//        }
//    }
    //subChannel size
    if(mHalCameraInfo.fast_picture_mode)
    {
        int previewWidth, previewHeight;
        mParameters.getPreviewSize(&previewWidth, &previewHeight);
        int videoWidth, videoHeight;
        mParameters.getVideoSize(&videoWidth, &videoHeight);
        if(videoWidth*videoHeight>previewWidth*previewHeight)
        {
            LOGD("device_id[%d], setSubChannelSize[%dx%d]", mHalCameraInfo.device_id, previewWidth, previewHeight);
            mParameters.setSubChannelSize(previewWidth, previewHeight);
        }
        else if(videoWidth==previewWidth && videoHeight==previewHeight)
        {
            LOGD("device_id[%d], previewSize[%dx%d] same as videoSize, we may disable subChannel in future.", mHalCameraInfo.device_id, previewWidth, previewHeight);
            mParameters.setSubChannelSize(previewWidth, previewHeight);
        }
        else
        {
            LOGE("fatal error, param wrong! device_id[%d], previewSize[%dx%d] videoSize[%dx%d]",
                mHalCameraInfo.device_id, previewWidth, previewHeight, videoWidth, videoHeight);
            mParameters.setSubChannelSize(0, 0);
        }
    }
    else
    {
        LOGD("device_id[%d], fast_picture_mode is 0, set subChannelSize to 0.", mHalCameraInfo.device_id);
        mParameters.setSubChannelSize(0, 0);
    }

	// video hint
    const char * valstr = params.get(CameraParameters::KEY_RECORDING_HINT);
    if (valstr) 
	{
		LOGV("KEY_RECORDING_HINT: %s", valstr);
        mParameters.set(CameraParameters::KEY_RECORDING_HINT, valstr);
    }

	// frame rate
	int new_min_frame_rate, new_max_frame_rate;
	params.getPreviewFpsRange(&new_min_frame_rate, &new_max_frame_rate);
	int new_preview_frame_rate = params.getPreviewFrameRate();
    bool bFrameRateValid = checkPreviewFrameRateValid(new_preview_frame_rate);
	if (bFrameRateValid && 0 < new_min_frame_rate 
		&& new_min_frame_rate <= new_max_frame_rate)
	{
		int preview_frame_rate = mParameters.getPreviewFrameRate();
		if (mFirstSetParameters
			|| preview_frame_rate != new_preview_frame_rate)
		{
			mParameters.setPreviewFrameRate(new_preview_frame_rate);
			pV4L2Device->setFrameRate(new_preview_frame_rate);
		}
	}
	else
	{
		if (pV4L2Device->getCaptureFormat() == V4L2_PIX_FMT_YUYV || pV4L2Device->getCaptureFormat() == V4L2_PIX_FMT_MJPEG)
		{
			LOGV("getCaptureFormat[0x%x], may be usb camera, don't care frame rate", pV4L2Device->getCaptureFormat());
		}
		else
		{
			LOGE("error preview frame rate");
			return -EINVAL;
		}
	}

	// JPEG image quality
    int new_jpeg_quality = params.getInt(CameraParameters::KEY_JPEG_QUALITY);
    LOGV("new_jpeg_quality %d", new_jpeg_quality);
    if (new_jpeg_quality >=1 && new_jpeg_quality <= 100) 
	{
		mParameters.set(CameraParameters::KEY_JPEG_QUALITY, new_jpeg_quality);
    }
	else
	{
		if (pV4L2Device->getCaptureFormat() == V4L2_PIX_FMT_YUYV)
		{
			LOGV("may be usb camera, don't care picture quality");
			mParameters.set(CameraParameters::KEY_JPEG_QUALITY, 90);
		}
		else
		{
			LOGE("error picture quality");
			return -EINVAL;
		}
	}

	// rotation	
	int new_rotation = params.getInt(CameraParameters::KEY_ROTATION);
    LOGV("new_rotation %d", new_rotation);
    if (0 <= new_rotation) 
	{
		mOriention = new_rotation;
        mParameters.set(CameraParameters::KEY_ROTATION, new_rotation);
    }
	else
	{
		LOGE("error rotate");
		return -EINVAL;
	}

	// image effect
	if (mCameraConfig->supportColorEffect())
	{
		const char *now_image_effect_str = mParameters.get(CameraParameters::KEY_EFFECT);
		const char *new_image_effect_str = params.get(CameraParameters::KEY_EFFECT);
		LOGV("new_image_effect_str %s", new_image_effect_str);
	    if ((new_image_effect_str != NULL)
			&& (mFirstSetParameters || strcmp(now_image_effect_str, new_image_effect_str)))
		{
	        int  new_image_effect = -1;

	        if (!strcmp(new_image_effect_str, CameraParameters::EFFECT_NONE))
	            new_image_effect = V4L2_COLORFX_NONE;
	        else if (!strcmp(new_image_effect_str, CameraParameters::EFFECT_MONO))
	            new_image_effect = V4L2_COLORFX_BW;
	        else if (!strcmp(new_image_effect_str, CameraParameters::EFFECT_SEPIA))
	            new_image_effect = V4L2_COLORFX_SEPIA;
	        else if (!strcmp(new_image_effect_str, CameraParameters::EFFECT_AQUA))
	            new_image_effect = V4L2_COLORFX_GRASS_GREEN;
	        else if (!strcmp(new_image_effect_str, CameraParameters::EFFECT_NEGATIVE))
	            new_image_effect = V4L2_COLORFX_NEGATIVE;
	        else {
	            //posterize, whiteboard, blackboard, solarize
	            LOGE("Invalid effect(%s)", new_image_effect_str);
	            ret = UNKNOWN_ERROR;
	        }

	        if (new_image_effect >= 0) {
	            mParameters.set(CameraParameters::KEY_EFFECT, new_image_effect_str);
				mQueueElement[CMD_QUEUE_SET_COLOR_EFFECT].cmd = CMD_QUEUE_SET_COLOR_EFFECT;
				mQueueElement[CMD_QUEUE_SET_COLOR_EFFECT].data = new_image_effect;
				OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_SET_COLOR_EFFECT]);
	        }
	    }
	}
	
	// white balance
	if (mCameraConfig->supportWhiteBalance())
	{
		const char *now_white_str = mParameters.get(CameraParameters::KEY_WHITE_BALANCE);
		const char *new_white_str = params.get(CameraParameters::KEY_WHITE_BALANCE);
	    LOGV("new_white_str %s", new_white_str);
	    if ((new_white_str != NULL)
			&& (mFirstSetParameters || strcmp(now_white_str, new_white_str)))
		{
	        int new_white = -1;
	        int no_auto_balance = 1;

	        if (!strcmp(new_white_str, CameraParameters::WHITE_BALANCE_AUTO))
	        {
	            new_white = V4L2_WHITE_BALANCE_AUTO;
	            no_auto_balance = 0;
	        }
	        else if (!strcmp(new_white_str,
	                         CameraParameters::WHITE_BALANCE_DAYLIGHT))
	            new_white = V4L2_WHITE_BALANCE_DAYLIGHT;
	        else if (!strcmp(new_white_str,
	                         CameraParameters::WHITE_BALANCE_CLOUDY_DAYLIGHT))
	            new_white = V4L2_WHITE_BALANCE_CLOUDY;
	        else if (!strcmp(new_white_str,
	                         CameraParameters::WHITE_BALANCE_FLUORESCENT))
	            new_white = V4L2_WHITE_BALANCE_FLUORESCENT_H;
	        else if (!strcmp(new_white_str,
	                         CameraParameters::WHITE_BALANCE_INCANDESCENT))
	            new_white = V4L2_WHITE_BALANCE_INCANDESCENT;
	        else if (!strcmp(new_white_str,
	                         CameraParameters::WHITE_BALANCE_WARM_FLUORESCENT))
	            new_white = V4L2_WHITE_BALANCE_FLUORESCENT;
	        else{
	            LOGE("Invalid white balance(%s)", new_white_str); //twilight, shade
	            ret = UNKNOWN_ERROR;
	        }

	        mCallbackNotifier.setWhiteBalance(no_auto_balance);

	        if (0 <= new_white)
			{
				mParameters.set(CameraParameters::KEY_WHITE_BALANCE, new_white_str);
				mQueueElement[CMD_QUEUE_SET_WHITE_BALANCE].cmd = CMD_QUEUE_SET_WHITE_BALANCE;
				mQueueElement[CMD_QUEUE_SET_WHITE_BALANCE].data = new_white;
				OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_SET_WHITE_BALANCE]);
	        }
	    }
	}
	
	// exposure compensation
	if (mCameraConfig->supportExposureCompensation())
	{
		int now_exposure_compensation = mParameters.getInt(CameraParameters::KEY_EXPOSURE_COMPENSATION);
		int new_exposure_compensation = params.getInt(CameraParameters::KEY_EXPOSURE_COMPENSATION);
		int max_exposure_compensation = params.getInt(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION);
		int min_exposure_compensation = params.getInt(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION);
		LOGV("new_exposure_compensation %d", new_exposure_compensation);
		if ((min_exposure_compensation <= new_exposure_compensation)
			&& (max_exposure_compensation >= new_exposure_compensation))
		{
			if (mFirstSetParameters || (now_exposure_compensation != new_exposure_compensation))
			{
				mParameters.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, new_exposure_compensation);
				mQueueElement[CMD_QUEUE_SET_EXPOSURE_COMPENSATION].cmd = CMD_QUEUE_SET_EXPOSURE_COMPENSATION;
				mQueueElement[CMD_QUEUE_SET_EXPOSURE_COMPENSATION].data = new_exposure_compensation;
				OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_SET_EXPOSURE_COMPENSATION]);
			}
		}
		else
		{
			LOGE("invalid exposure compensation: %d", new_exposure_compensation);
			return -EINVAL;
		}
	}

	int now_contrast_value = mParameters.getInt(CameraParameters::KEY_CID_CONTRAST);
	int new_contrast_value = params.getInt(CameraParameters::KEY_CID_CONTRAST);
	if (mFirstSetParameters || (now_contrast_value != new_contrast_value))
	{
		mParameters.set(CameraParameters::KEY_CID_CONTRAST, new_contrast_value);
		mV4L2CameraDevice->setContrastValue(new_contrast_value);
	}


	int now_brightness_value = mParameters.getInt(CameraParameters::KEY_CID_BRIGHTNESS);
	int new_brightness_value = params.getInt(CameraParameters::KEY_CID_BRIGHTNESS);
	if (mFirstSetParameters || (now_brightness_value != new_brightness_value))
	{
		mParameters.set(CameraParameters::KEY_CID_BRIGHTNESS, new_brightness_value);
		mV4L2CameraDevice->setBrightnessValue(new_brightness_value);
	}

	int now_saturation_value = mParameters.getInt(CameraParameters::KEY_CID_SATURATION);
	int new_saturation_value = params.getInt(CameraParameters::KEY_CID_SATURATION);
	if (mFirstSetParameters || (now_saturation_value != new_saturation_value))
	{
		mParameters.set(CameraParameters::KEY_CID_SATURATION, new_saturation_value);
		mV4L2CameraDevice->setSaturationValue(new_saturation_value);
	}

	int now_hue_value = mParameters.getInt(CameraParameters::KEY_CID_HUE);
	int new_hue_value = params.getInt(CameraParameters::KEY_CID_HUE);
	if (mFirstSetParameters || (now_hue_value != new_hue_value))
	{
		mParameters.set(CameraParameters::KEY_CID_HUE, new_hue_value);
		mV4L2CameraDevice->setHueValue(new_hue_value);
	}

	int now_sharpness_value = mParameters.getInt(CameraParameters::KEY_CID_SHARPNESS);
	int new_sharpness_value = params.getInt(CameraParameters::KEY_CID_SHARPNESS);
	if (mFirstSetParameters || (now_sharpness_value != new_sharpness_value))
	{
		mParameters.set(CameraParameters::KEY_CID_SHARPNESS, new_sharpness_value);
		mV4L2CameraDevice->setSharpnessValue(new_sharpness_value);
	}

	int now_vflip_value = mParameters.getInt(CameraParameters::KEY_CID_VFLIP);
	int new_vflip_value = params.getInt(CameraParameters::KEY_CID_VFLIP);
    LOGV("now_vflip_value=%d,new_vflip_value=%d",now_vflip_value,new_vflip_value );
	if (mFirstSetParameters || (now_vflip_value != new_vflip_value))
	{
        if(new_vflip_value >= 0){
            mParameters.set(CameraParameters::KEY_CID_VFLIP, new_vflip_value);
    		mV4L2CameraDevice->setVflip(new_vflip_value);
        }

	}
    
 	int now_hflip_value = mParameters.getInt(CameraParameters::KEY_CID_HFLIP);
	int new_hflip_value = params.getInt(CameraParameters::KEY_CID_HFLIP);
    LOGV("now_hflip_value=%d,new_hflip_value=%d",now_hflip_value,new_hflip_value );
	if (mFirstSetParameters || (now_hflip_value != new_hflip_value))
	{
        if(new_hflip_value >= 0){
            mParameters.set(CameraParameters::KEY_CID_HFLIP, new_hflip_value);
    		mV4L2CameraDevice->setHflip(new_hflip_value);            
        }

	} 

	int now_image_value = mParameters.getInt(CameraParameters::KEY_CID_EFFECT);
	int new_image_value = params.getInt(CameraParameters::KEY_CID_EFFECT);
    LOGV("now_image_value: %d,new_image_value:%d", now_image_value,new_image_value);
	if (mFirstSetParameters || (now_image_value != new_image_value))
	{
        if(new_image_value!=-1)
        {
            mParameters.set(CameraParameters::KEY_CID_SHARPNESS, new_image_value);
    		mV4L2CameraDevice->setImageEffect(new_image_value);
        }
	}
    
    int now_plf_value = mParameters.getInt(CameraParameters::KEY_CID_POWER_LINE_FREQUENCY);
    int new_plf_value = params.getInt(CameraParameters::KEY_CID_POWER_LINE_FREQUENCY);
    LOGV("now_plf_value: %d,new_plf_value:%d", now_plf_value,new_plf_value);
    if (mFirstSetParameters || (now_plf_value != new_plf_value))
    {
        if(new_plf_value!=-1)
        {
            mParameters.set(CameraParameters::KEY_CID_POWER_LINE_FREQUENCY, new_plf_value);
            mV4L2CameraDevice->setPowerLineFrequencyValue(new_plf_value);
        }
    }

	// flash mode	
	if (mCameraConfig->supportFlashMode())
	{
		const char *new_flash_mode_str = params.get(CameraParameters::KEY_FLASH_MODE);
		mParameters.set(CameraParameters::KEY_FLASH_MODE, new_flash_mode_str);
	}

	// zoom
	int max_zoom = mParameters.getInt(CameraParameters::KEY_MAX_ZOOM);
	int new_zoom = params.getInt(CameraParameters::KEY_ZOOM);
	LOGV("new_zoom: %d", new_zoom);
	if (0 <= new_zoom && new_zoom <= max_zoom)
	{
		mParameters.set(CameraParameters::KEY_ZOOM, new_zoom);
		pV4L2Device->setCrop(new_zoom + BASE_ZOOM, max_zoom);
		mZoomRatio = (new_zoom + BASE_ZOOM) * 2 * 100 / max_zoom + 100;
	}
	else
	{
		LOGE("invalid zoom value: %d", new_zoom);
		return -EINVAL;
	}

	// focus
	if (mCameraConfig->supportFocusMode())
	{
		const char *now_focus_mode_str = mParameters.get(CameraParameters::KEY_FOCUS_MODE);
		const char *now_focus_areas_str = mParameters.get(CameraParameters::KEY_FOCUS_AREAS);
		const char *new_focus_mode_str = params.get(CameraParameters::KEY_FOCUS_MODE);
        const char *new_focus_areas_str = params.get(CameraParameters::KEY_FOCUS_AREAS);

		if (!checkFocusArea(new_focus_areas_str))
		{
			LOGE("invalid focus areas");
			return -EINVAL;
		}
		
		if (!checkFocusMode(new_focus_mode_str))
		{
			LOGE("invalid focus mode");
			return -EINVAL;
		}
		
		if (mFirstSetParameters || strcmp(now_focus_mode_str, new_focus_mode_str))
		{
			mParameters.set(CameraParameters::KEY_FOCUS_MODE, new_focus_mode_str);
			mQueueElement[CMD_QUEUE_SET_FOCUS_MODE].cmd = CMD_QUEUE_SET_FOCUS_MODE;
			OSAL_QueueSetElem(&mQueueCommand, &mQueueElement[CMD_QUEUE_SET_FOCUS_MODE]);
		}

		// to do, check running??
		if (/*pV4L2Device->getThreadRunning()
			&&*/ strcmp(now_focus_areas_str, new_focus_areas_str))
		{
			mParameters.set(CameraParameters::KEY_FOCUS_AREAS, new_focus_areas_str);

#if 0
			strcpy(mFocusAreasStr, new_focus_areas_str);
			mQueueElement[CMD_QUEUE_SET_FOCUS_AREA].cmd = CMD_QUEUE_SET_FOCUS_AREA;
			mQueueElement[CMD_QUEUE_SET_FOCUS_AREA].data = (int)&mFocusAreasStr;
			OSAL_QueueSetElem(&mQueueCommand, &mQueueElement[CMD_QUEUE_SET_FOCUS_AREA]);
#else
			parse_focus_areas(new_focus_areas_str);
#endif
		}
	}
	else
	{
		const char *new_focus_mode_str = params.get(CameraParameters::KEY_FOCUS_MODE);
		if (strcmp(new_focus_mode_str, CameraParameters::FOCUS_MODE_FIXED))
		{
			LOGE("invalid focus mode: %s", new_focus_mode_str);
			//return -EINVAL;
		}
		mParameters.set(CameraParameters::KEY_FOCUS_MODE, CameraParameters::FOCUS_MODE_FIXED);
	}

	// gps latitude
    const char *new_gps_latitude_str = params.get(CameraParameters::KEY_GPS_LATITUDE);
	if (new_gps_latitude_str) {
		mCallbackNotifier.setGPSLatitude(atof(new_gps_latitude_str));
        mParameters.set(CameraParameters::KEY_GPS_LATITUDE, new_gps_latitude_str);
    } else {
    	mCallbackNotifier.setGPSLatitude(0.0);
        mParameters.remove(CameraParameters::KEY_GPS_LATITUDE);
    }

    // gps longitude
    const char *new_gps_longitude_str = params.get(CameraParameters::KEY_GPS_LONGITUDE);
    if (new_gps_longitude_str) {
		mCallbackNotifier.setGPSLongitude(atof(new_gps_longitude_str));
        mParameters.set(CameraParameters::KEY_GPS_LONGITUDE, new_gps_longitude_str);
    } else {
    	mCallbackNotifier.setGPSLongitude(0.0);
        mParameters.remove(CameraParameters::KEY_GPS_LONGITUDE);
    }
  
    // gps altitude
    const char *new_gps_altitude_str = params.get(CameraParameters::KEY_GPS_ALTITUDE);
	if (new_gps_altitude_str) {
		mCallbackNotifier.setGPSAltitude(atol(new_gps_altitude_str));
        mParameters.set(CameraParameters::KEY_GPS_ALTITUDE, new_gps_altitude_str);
    } else {
		mCallbackNotifier.setGPSAltitude(0);
        mParameters.remove(CameraParameters::KEY_GPS_ALTITUDE);
    }

    // gps timestamp
    const char *new_gps_timestamp_str = params.get(CameraParameters::KEY_GPS_TIMESTAMP);
	if (new_gps_timestamp_str) {
		mCallbackNotifier.setGPSTimestamp(atol(new_gps_timestamp_str));
        mParameters.set(CameraParameters::KEY_GPS_TIMESTAMP, new_gps_timestamp_str);
    } else {
		mCallbackNotifier.setGPSTimestamp(0);
        mParameters.remove(CameraParameters::KEY_GPS_TIMESTAMP);
    }

    // gps processing method
    const char *new_gps_processing_method_str = params.get(CameraParameters::KEY_GPS_PROCESSING_METHOD);
	if (new_gps_processing_method_str) {
		mCallbackNotifier.setGPSMethod(new_gps_processing_method_str);
        mParameters.set(CameraParameters::KEY_GPS_PROCESSING_METHOD, new_gps_processing_method_str);
    } else {
		mCallbackNotifier.setGPSMethod("");
        mParameters.remove(CameraParameters::KEY_GPS_PROCESSING_METHOD);
    }
	
	// JPEG thumbnail size
	int new_jpeg_thumbnail_width = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
	int new_jpeg_thumbnail_height= params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);
	LOGV("new_jpeg_thumbnail_width: %d, new_jpeg_thumbnail_height: %d",
		new_jpeg_thumbnail_width, new_jpeg_thumbnail_height);
	if (0 <= new_jpeg_thumbnail_width && 0 <= new_jpeg_thumbnail_height) {
		mCallbackNotifier.setJpegThumbnailSize(new_jpeg_thumbnail_width, new_jpeg_thumbnail_height);
		mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, new_jpeg_thumbnail_width);
		mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, new_jpeg_thumbnail_height);
	}

    // Preview flip. value is CameraParameters::AWEXTEND_PREVIEW_FLIP
    int newPreviewFlip = params.getInt(CameraParameters::KEY_AWEXTEND_PREVIEW_FLIP);
    int previewFlip = mParameters.getInt(CameraParameters::KEY_AWEXTEND_PREVIEW_FLIP);
    if(newPreviewFlip >= 0)
    {
        if(mFirstSetParameters || previewFlip!=newPreviewFlip)
        {
            LOGV("set previewFlip[%d]", newPreviewFlip);
            mParameters.set(CameraParameters::KEY_AWEXTEND_PREVIEW_FLIP, newPreviewFlip);
            pV4L2Device->setPreviewFlip(newPreviewFlip);
        }
    }
    // picture size mode. value is CameraParameters::KEY_AWEXTEND_PICTURE_SIZE_MODE
    int newPicSizeMode = params.getInt(CameraParameters::KEY_AWEXTEND_PICTURE_SIZE_MODE);
    int curPicSizeMode = mParameters.getInt(CameraParameters::KEY_AWEXTEND_PICTURE_SIZE_MODE);
    if(newPicSizeMode!=-1)
    {
        if(mFirstSetParameters || curPicSizeMode!=newPicSizeMode)
        {
            LOGV("set picture size mode[%d]", newPicSizeMode);
            mParameters.set(CameraParameters::KEY_AWEXTEND_PICTURE_SIZE_MODE, newPicSizeMode);
        }
    }

    int newBufferNum = params.getVideoCaptureBufferNum();
    int curBufferNum = mParameters.getVideoCaptureBufferNum();
    if (mFirstSetParameters || newBufferNum != curBufferNum) {
        if (newBufferNum > 0 && newBufferNum < 50 && !mV4L2CameraDevice->isStarted()) {
            LOGV("set video buffer number to %d", newBufferNum);
            mParameters.setVideoCaptureBufferNum(newBufferNum);
            mV4L2CameraDevice->setVideoCaptureBufferNum(newBufferNum);
        } else {
            LOGW("Can't set video capture buffer num, newBufferNum=%d, camera is %s",
                newBufferNum, mV4L2CameraDevice->isStarted() ? "started" : "not start");
        }
    }

    int newNum = params.getContinuousPictureNumber();
    int curNum = mParameters.getContinuousPictureNumber();
    if (mFirstSetParameters || newNum != curNum) {
        if (newBufferNum > 0) {
            LOGV("set continuous picture number to %d", newNum);
            mParameters.setContinuousPictureNumber(newNum);
            mV4L2CameraDevice->setContinuousPictureNumber(newNum);
        } else {
            LOGW("Can't set continuous picture number to %d", newNum);
        }
    }

    int newInterval = params.getContinuousPictureIntervalMs();
    int curInterval = mParameters.getContinuousPictureIntervalMs();
    if (mFirstSetParameters || newInterval != curInterval) {
        if (newInterval >= 0) {
            LOGV("set continuous picture interval to %d", newInterval);
            mParameters.setContinuousPictureIntervalMs(newInterval);
            mV4L2CameraDevice->setContinuousPictureInterval(newInterval);
        } else {
            LOGW("Can't set continuous picture interval to %d", newInterval);
        }
    }

    int newPreviewRotation = params.getPreviewRotation();
    int curPreviewRotation = mParameters.getPreviewRotation();
    if (mFirstSetParameters || newPreviewRotation != curPreviewRotation) {
        LOGV("set preview rotation to %d", newPreviewRotation);
        mParameters.setPreviewRotation(newPreviewRotation);
        mV4L2CameraDevice->setPreviewRotation(newPreviewRotation);
    }

	mFirstSetParameters = false;
	pthread_cond_signal(&mCommandCond);

    return NO_ERROR;
}

/* A dumb variable indicating "no params" / error on the exit from
 * CameraHardware::getParameters(). */
static char lNoParam = '\0';
char* CameraHardware::getParameters()
{
	LOGV();
    String8 params(mParameters.flatten());
    char* ret_str =
        reinterpret_cast<char*>(malloc(sizeof(char) * (params.length()+1)));
    memset(ret_str, 0, params.length()+1);
    if (ret_str != NULL) {
        strncpy(ret_str, params.string(), params.length()+1);
        return ret_str;
    } else {
        LOGE("Unable to allocate string for %s", params.string());
        /* Apparently, we can't return NULL fron this routine. */
        return &lNoParam;
    }
}

void CameraHardware::putParameters(char* params)
{
	LOGV();
    /* This method simply frees parameters allocated in getParameters(). */
    if (params != NULL && params != &lNoParam) {
        free(params);
    }
}

void CameraHardware::setNewCrop(Rect * rect)
{
	LOGV();
	memcpy(&mFrameRectCrop, rect, sizeof(Rect));
}

status_t CameraHardware::sendCommand(int32_t cmd, int32_t arg1, int32_t arg2)
{
    LOGV("cmd = %x, arg1 = %d, arg2 = %d", cmd, arg1, arg2);

    /* TODO: Future enhancements. */

	switch (cmd)
	{
	case CAMERA_CMD_SET_CEDARX_RECORDER:
		mUseHwEncoder = true;
		mV4L2CameraDevice->setHwEncoder(true);
		return OK;
	case CAMERA_CMD_START_FACE_DETECTION:
	{
		const char *face = mParameters.get(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW);
		if (face == NULL || (atoi(face) <= 0))
		{
			return -EINVAL;
		}
		mQueueElement[CMD_QUEUE_START_FACE_DETECTE].cmd = CMD_QUEUE_START_FACE_DETECTE;
		OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_START_FACE_DETECTE]);
		pthread_cond_signal(&mCommandCond);
		return OK;
	}
	case CAMERA_CMD_STOP_FACE_DETECTION:
		mQueueElement[CMD_QUEUE_STOP_FACE_DETECTE].cmd = CMD_QUEUE_STOP_FACE_DETECTE;
		OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_STOP_FACE_DETECTE]);
		pthread_cond_signal(&mCommandCond);
		return OK;
	case CAMERA_CMD_PING:
		return OK;
	case CAMERA_CMD_ENABLE_FOCUS_MOVE_MSG:
	{
		bool enable = static_cast<bool>(arg1);
        if (enable) {
			enableMsgType(CAMERA_MSG_FOCUS_MOVE);
        } else {
			disableMsgType(CAMERA_MSG_FOCUS_MOVE);
        }
		return OK;
	}
	case CAMERA_CMD_SET_DISPLAY_ORIENTATION:
		LOGD("CAMERA_CMD_SET_DISPLAY_ORIENTATION, to do ...");
		return OK;
#ifdef MOTION_DETECTION_ENABLE
	case CAMERA_CMD_START_AWMD:
		LOGV("CAMERA_CMD_START_AWMD");
		pthread_mutex_lock(&mCommandMutex);
		mQueueElement[CMD_QUEUE_START_AWMD].cmd = CMD_QUEUE_START_AWMD;
		OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_START_AWMD]);
		pthread_cond_signal(&mCommandCond);
		pthread_mutex_unlock(&mCommandMutex);
		return OK;
	case CAMERA_CMD_STOP_AWMD:
		LOGV("CAMERA_CMD_STOP_AWMD");
		pthread_mutex_lock(&mCommandMutex);
		mQueueElement[CMD_QUEUE_STOP_AWMD].cmd = CMD_QUEUE_STOP_AWMD;
		OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_STOP_AWMD]);
		pthread_cond_signal(&mCommandCond);
		pthread_mutex_unlock(&mCommandMutex);
		return OK;
	case CAMERA_CMD_SET_AWMD_SENSITIVITY_LEVEL:
		LOGV("CAMERA_CMD_SET_AWMD_SENSITIVITY_LEVEL");
		pthread_mutex_lock(&mCommandMutex);
		mQueueElement[CMD_QUEUE_SET_AWMD_SENSITIVITY_LEVEL].cmd = CMD_QUEUE_SET_AWMD_SENSITIVITY_LEVEL;
		mQueueElement[CMD_QUEUE_SET_AWMD_SENSITIVITY_LEVEL].data = arg1;
		OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_SET_AWMD_SENSITIVITY_LEVEL]);
		pthread_cond_signal(&mCommandCond);
		pthread_mutex_unlock(&mCommandMutex);
		return OK;
#endif

#ifdef ADAS_ENABLE
	case CAMERA_CMD_ADAS_START_DETECTION:
		pthread_mutex_lock(&mCommandMutex);
		mQueueElement[CMD_QUEUE_ADAS_START_DETECT].cmd = CMD_QUEUE_ADAS_START_DETECT;
		OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_ADAS_START_DETECT]);
		pthread_cond_signal(&mCommandCond);
		pthread_mutex_unlock(&mCommandMutex);
		return OK;
	case CAMERA_CMD_ADAS_STOP_DETECTION:
		pthread_mutex_lock(&mCommandMutex);
		mQueueElement[CMD_QUEUE_ADAS_STOP_DETECT].cmd = CMD_QUEUE_ADAS_STOP_DETECT;
		OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_ADAS_STOP_DETECT]);
		pthread_cond_signal(&mCommandCond);
		pthread_mutex_unlock(&mCommandMutex);
		return OK;
    case CAMERA_CMD_ADAS_SET_LANELINE_OFFSET_SENSITY:
		LOGV("CAMERA_CMD_ADAS_SET_LANELINE_OFFSET_SENSITY");
		pthread_mutex_lock(&mCommandMutex);
		mQueueElement[CMD_QUEUE_ADAS_SET_LANELINE_OFFSET_SENSITY].cmd = CMD_QUEUE_ADAS_SET_LANELINE_OFFSET_SENSITY;
		mQueueElement[CMD_QUEUE_ADAS_SET_LANELINE_OFFSET_SENSITY].data = arg1;
		OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_ADAS_SET_LANELINE_OFFSET_SENSITY]);
		pthread_cond_signal(&mCommandCond);
		pthread_mutex_unlock(&mCommandMutex);
		return OK;
    case CAMERA_CMD_ADAS_SET_DISTANCE_DETECT_LEVEL:
		LOGV("CAMERA_CMD_ADAS_SET_DISTANCE_DETECT_LEVEL");
		pthread_mutex_lock(&mCommandMutex);
		mQueueElement[CMD_QUEUE_ADAS_SET_DISTANCE_DETECT_LEVEL].cmd = CMD_QUEUE_ADAS_SET_DISTANCE_DETECT_LEVEL;
		mQueueElement[CMD_QUEUE_ADAS_SET_DISTANCE_DETECT_LEVEL].data = arg1;
		OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_ADAS_SET_DISTANCE_DETECT_LEVEL]);
		pthread_cond_signal(&mCommandCond);
		pthread_mutex_unlock(&mCommandMutex);
		return OK;
    case CAMERA_CMD_ADAS_SET_CAR_SPEED:
		LOGV("CAMERA_CMD_ADAS_SET_CAR_SPEED");
		pthread_mutex_lock(&mCommandMutex);
		mQueueElement[CMD_QUEUE_ADAS_SET_CAR_SPEED].cmd = CMD_QUEUE_ADAS_SET_CAR_SPEED;
		mQueueElement[CMD_QUEUE_ADAS_SET_CAR_SPEED].data = arg1;
		OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_ADAS_SET_CAR_SPEED]);
		pthread_cond_signal(&mCommandCond);
		pthread_mutex_unlock(&mCommandMutex);
		return OK;
	case CAMERA_CMD_ADAS_AUDIO_VOL:
		LOGV("CAMERA_CMD_ADAS_AUDIO_VOL");
		pthread_mutex_lock(&mCommandMutex);
		mQueueElement[CMD_QUEUE_ADAS_SET_AUDIO_VOL].cmd = CMD_QUEUE_ADAS_SET_AUDIO_VOL;
		mQueueElement[CMD_QUEUE_ADAS_SET_AUDIO_VOL].data = arg1;
		OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_ADAS_SET_AUDIO_VOL]);
		pthread_cond_signal(&mCommandCond);
		pthread_mutex_unlock(&mCommandMutex);
		return OK;
	case CAMERA_CMD_ADAS_MODE:
		LOGV("CAMERA_CMD_ADAS_MODE");
		pthread_mutex_lock(&mCommandMutex);
		mQueueElement[CMD_QUEUE_ADAS_SET_MODE].cmd = CMD_QUEUE_ADAS_SET_MODE;
		mQueueElement[CMD_QUEUE_ADAS_SET_MODE].data = arg1;
		OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_ADAS_SET_MODE]);
		pthread_cond_signal(&mCommandCond);
		pthread_mutex_unlock(&mCommandMutex);
		return OK;
	case CAMERA_CMD_ADAS_TIPS_MODE:
		LOGV("CAMERA_CMD_ADAS_TIPS_MODE");
		pthread_mutex_lock(&mCommandMutex);
		mQueueElement[CMD_QUEUE_ADAS_SET_TIPS_MODE].cmd = CMD_QUEUE_ADAS_SET_TIPS_MODE;
		mQueueElement[CMD_QUEUE_ADAS_SET_TIPS_MODE].data = arg1;
		OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_ADAS_SET_TIPS_MODE]);
		pthread_cond_signal(&mCommandCond);
		pthread_mutex_unlock(&mCommandMutex);
		return OK;
#endif

#ifdef QRDECODE_ENABLE
    case CAMERA_CMD_SET_QRDECODE:
		LOGV("CAMERA_CMD_SET_QRDECODE");
		pthread_mutex_lock(&mCommandMutex);
		mQueueElement[CMD_QUEUE_SET_QRDECODE].cmd = CMD_QUEUE_SET_QRDECODE;
		mQueueElement[CMD_QUEUE_SET_QRDECODE].data = arg1;
		OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_SET_QRDECODE]);
		pthread_cond_signal(&mCommandCond);
		pthread_mutex_unlock(&mCommandMutex);
		return OK;
#endif

	case CAMERA_CMD_STOP_RENDER: 
		LOGV("CAMERA_CMD_STOP_RENDER");
		pthread_mutex_lock(&mCommandMutex);
		mQueueElement[CMD_QUEUE_STOP_RENDER].cmd = CMD_QUEUE_STOP_RENDER;
		OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_STOP_RENDER]);
		pthread_cond_signal(&mCommandCond);
		pthread_mutex_unlock(&mCommandMutex);
		return OK;
	case CAMERA_CMD_START_RENDER: 
		LOGV("CAMERA_CMD_START_RENDER");
		pthread_mutex_lock(&mCommandMutex);
		mQueueElement[CMD_QUEUE_START_RENDER].cmd = CMD_QUEUE_START_RENDER;
		OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_START_RENDER]);
		pthread_cond_signal(&mCommandCond);
		pthread_mutex_unlock(&mCommandMutex);
		return OK;

	case CAMERA_CMD_SET_COLORFX:
		LOGV("CAMERA_CMD_SET_COLORFX");
		pthread_mutex_lock(&mCommandMutex);
		mQueueElement[CMD_QUEUE_SET_COLORFX].cmd = CMD_QUEUE_SET_COLORFX;
		mQueueElement[CMD_QUEUE_SET_COLORFX].data = arg1;
		OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_SET_COLORFX]);
		pthread_cond_signal(&mCommandCond);
		pthread_mutex_unlock(&mCommandMutex);
		return OK;

    case CAMERA_CMD_CANCEL_CONTINUOUS_SNAP:
        LOGV("CAMERA_CMD_CANCEL_CONTINUOUS_SNAP");
		pthread_mutex_lock(&mCommandMutex);
		mQueueElement[CMD_QUEUE_STOP_CONTINUOUSSNAP].cmd = CMD_QUEUE_STOP_CONTINUOUSSNAP;
		OSAL_Queue(&mQueueCommand, &mQueueElement[CMD_QUEUE_STOP_CONTINUOUSSNAP]);
		pthread_cond_signal(&mCommandCond);
		pthread_mutex_unlock(&mCommandMutex);
        return OK;
	}

    return -EINVAL;
}

status_t CameraHardware::getV4l2QueryCtrl(struct v4l2_queryctrl *ctrl, int id)
{

	status_t ret = -1;
	
	if (id == V4L2_CID_CONTRAST) {
		ret =  mV4L2CameraDevice->getContrastCtrl(ctrl);
	} else if (id == V4L2_CID_BRIGHTNESS) {
		ret =  mV4L2CameraDevice->getBrightnessCtrl(ctrl);
	} else if (id == V4L2_CID_SATURATION) {
		ret =  mV4L2CameraDevice->getSaturationCtrl(ctrl);
	} else if (id == V4L2_CID_HUE) {
		ret =  mV4L2CameraDevice->getHueCtrl(ctrl);
	} else if (id == V4L2_CID_SHARPNESS) {
		ret =  mV4L2CameraDevice->getSharpnessCtrl(ctrl);
	}

	return ret;
}

status_t CameraHardware::getInputSource(int *source)
{
	*source = mV4L2CameraDevice->getInputSource();
	return NO_ERROR;
}

status_t CameraHardware::getTVinSystem(int *system)
{
	*system = mV4L2CameraDevice->getTVinSystem();
	return NO_ERROR;
}

#ifdef WATERMARK_ENABLE
status_t CameraHardware::setWaterMark(int enable, const char *str)
{
    mV4L2CameraDevice->setWaterMark(enable, str);
    return NO_ERROR;
}
#endif

status_t CameraHardware::setUvcGadgetMode(int value)
{
    return mV4L2CameraDevice->setUvcGadgetMode(value);
}

#ifdef ADAS_ENABLE
status_t CameraHardware::adasSetGsensorData(int val0, float val1, float val2)
{
    return mV4L2CameraDevice->adasSetGsensorData(val0, val1, val2);
}
#endif

void CameraHardware::releaseCamera()
{
	LOGV();

    cleanupCamera();
}

status_t CameraHardware::dumpCamera(int fd)
{
	LOGV();

    /* TODO: Future enhancements. */
    return -EINVAL;
}

#if 0  // face
/****************************************************************************
 * Facedetection management
 ***************************************************************************/
int CameraHardware::getCurrentFaceFrame(void * frame)
{
	return mV4L2CameraDevice->getCurrentFaceFrame(frame);
}

int CameraHardware::faceDetection(camera_frame_metadata_t *face)
{
	int number_of_faces = face->number_of_faces;
	if (number_of_faces == 0)
	{
		parse_focus_areas("(0, 0, 0, 0, 0)", true);
	}
	else
	{
		if (mZoomRatio != 0)
		{
			for(int i =0; i < number_of_faces; i++)
			{
				face->faces[i].rect[0] = (face->faces[i].rect[0] * mZoomRatio) / 100;
				face->faces[i].rect[1] = (face->faces[i].rect[1] * mZoomRatio) / 100;
				face->faces[i].rect[2] = (face->faces[i].rect[2] * mZoomRatio) / 100;
				face->faces[i].rect[3] = (face->faces[i].rect[3] * mZoomRatio) / 100;
			}
		}
	}
	return mCallbackNotifier.faceDetectionMsg(face);
}
#endif

/****************************************************************************
 * Preview management.
 ***************************************************************************/

status_t CameraHardware::doStartPreview()
{
	LOGV();
	
	V4L2CameraDevice* const camera_dev = mV4L2CameraDevice;

	if (camera_dev->isStarted()
		&& mPreviewWindow.isPreviewEnabled())
	{
		LOGD("CameraHardware::doStartPreview has been already started");
		return NO_ERROR;
	}
	
	if (camera_dev->isStarted()) 
	{
        camera_dev->stopDeliveringFrames();
        camera_dev->stopDevice();
    }

	status_t res = mPreviewWindow.startPreview();
    if (res != NO_ERROR) 
	{
        return res;
    }

	// Make sure camera device is connected.
	if (!camera_dev->isConnected())
	{
		res = camera_dev->connectDevice(&mHalCameraInfo);
		if (res != NO_ERROR) 
		{
			mPreviewWindow.stopPreview();
			return res;
		}

		camera_dev->setAutoFocusInit();
	}

	const char * valstr = mParameters.get(CameraParameters::KEY_RECORDING_HINT);
	bool video_hint = (strcmp(valstr, CameraParameters::sTRUE) == 0);

	// preview size
	int preview_width = 0, preview_height = 0;
//	const char * preview_capture_width_str = mParameters.get(KEY_PREVIEW_CAPTURE_SIZE_WIDTH);
//	const char * preview_capture_height_str = mParameters.get(KEY_PREVIEW_CAPTURE_SIZE_HEIGHT);
//	if (preview_capture_width_str != NULL
//		&& preview_capture_height_str != NULL)
//	{
//		preview_width  = atoi(preview_capture_width_str);
//		preview_height = atoi(preview_capture_height_str);
//	}
    mParameters.getPreviewSize(&preview_width, &preview_height);

	// video size
	int video_width = 0, video_height = 0;
	mParameters.getVideoSize(&video_width, &video_height);

	// capture size
	if (video_hint)
	{
//		mCaptureWidth = mVideoCaptureWidth;
//		mCaptureHeight= mVideoCaptureHeight;
        mCaptureWidth = video_width;
		mCaptureHeight= video_height;
	}
	else
	{
		if (mHalCameraInfo.fast_picture_mode)
		{
//			mCaptureWidth = mFullSizeWidth;
//			mCaptureHeight= mFullSizeHeight;
            mCaptureWidth = video_width;
    		mCaptureHeight= video_height;
		}
		else
		{
			mCaptureWidth = preview_width;
			mCaptureHeight= preview_height;
		}
	}

//	if(strcmp(mCallingProcessName, "com.android.cts.verifier") == 0)  //add for CTS Verifier by fuqiang
//	{
//		mCaptureWidth = preview_width;
//		mCaptureHeight= preview_height;
//	}

	// preview format and video format are the same
	uint32_t org_fmt = V4L2_PIX_FMT_NV21;		// android default
	const char* preview_format = mParameters.getPreviewFormat();
	if (preview_format != NULL) 
	{
		if (strcmp(preview_format, CameraParameters::PIXEL_FORMAT_YUV420SP) == 0)
		{
#ifdef __SUNXI__
            LOGD("__SUNXI__, use V4L2_PIX_FMT_NV12");
			org_fmt = V4L2_PIX_FMT_NV12;		// SGX support NV12
#else
			org_fmt = V4L2_PIX_FMT_NV21;		// MALI support NV21
#endif
		}
		else if (strcmp(preview_format, CameraParameters::PIXEL_FORMAT_YUV420P) == 0)
		{
			org_fmt = V4L2_PIX_FMT_YVU420;		// YV12
		}
		else
		{
			LOGE("unknown preview format");
		}
	}
	
	LOGD("Starting camera: %dx%d -> %.4s(%s)",
         mCaptureWidth, mCaptureHeight, reinterpret_cast<const char*>(&org_fmt), preview_format);

    int sub_w = 0, sub_h = 0;
    mParameters.getSubChannelSize(&sub_w, &sub_h);
    int framerate = mParameters.getPreviewFrameRate();
    res = camera_dev->startDevice(mCaptureWidth, mCaptureHeight, sub_w, sub_h, org_fmt, framerate, false);
    if (res != NO_ERROR) 
	{
        mPreviewWindow.stopPreview();
        return res;
    }

	int format = mV4L2CameraDevice->getVideoFormat();
	mParameters.set(CameraParameters::KEY_CAPTURE_OUT_FORMAT, format);
	mV4L2CameraDevice->showformat(format,"CameraHardware-s2");

	res = camera_dev->startDeliveringFrames();
    if (res != NO_ERROR) 
	{
        camera_dev->stopDevice();
        mPreviewWindow.stopPreview();
    }
	
    return res;
}

status_t CameraHardware::doStopPreview()
{
	LOGV();
	
	status_t res = NO_ERROR;
	if (mPreviewWindow.isPreviewEnabled()) 
	{
		/* Stop the camera. */
		if (mV4L2CameraDevice->isStarted()) 
		{
			mV4L2CameraDevice->stopDeliveringFrames();
			res = mV4L2CameraDevice->stopDevice();
		}

		if (res == NO_ERROR) 
		{
			/* Disable preview as well. */
			mPreviewWindow.stopPreview();
		}
	}

    return NO_ERROR;
}

status_t CameraHardware::resetCaptureSize(int width, int height)
{
	mCaptureWidth = width;
	mCaptureHeight = height;
	mParameters.setVideoSize(width, height);
	//setVideoCaptureSize(width, height);
	return NO_ERROR;
}

status_t CameraHardware::setPreviewFrameRate(int fps)
{
    mParameters.setPreviewFrameRate(fps);
    return NO_ERROR;
}


/****************************************************************************
 * Private API.
 ***************************************************************************/

status_t CameraHardware::cleanupCamera()
{
	LOGV();

    status_t res = NO_ERROR;

	mParameters.set(CameraParameters::KEY_AWEXTEND_SNAP_PATH, "");
	mCallbackNotifier.setSnapPath("");

	// reset preview format to yuv420sp
	mParameters.set(CameraParameters::KEY_PREVIEW_FORMAT, CameraParameters::PIXEL_FORMAT_YUV420SP);

	mV4L2CameraDevice->setHwEncoder(false);

	mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, 320);
	mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, 240);

	mParameters.set(CameraParameters::KEY_ZOOM, 0);

//	mVideoCaptureWidth = 0;
//	mVideoCaptureHeight = 0;
	mUseHwEncoder = false;
	
	// stop focus thread
	pthread_mutex_lock(&mAutoFocusMutex);
	if (mAutoFocusThread->isThreadStarted())
	{
		mAutoFocusThread->stopThread();
		pthread_cond_signal(&mAutoFocusCond);
	}
	pthread_mutex_unlock(&mAutoFocusMutex);

	if (mCameraConfig->supportFocusMode())
	{
		// wait for auto focus thread exit
		pthread_mutex_lock(&mAutoFocusMutex);
		if (!mAutoFocusThreadExit)
		{
			LOGW("wait for auto focus thread exit");
			pthread_cond_wait(&mAutoFocusCond, &mAutoFocusMutex);
		}
		pthread_mutex_unlock(&mAutoFocusMutex);
	}
	
    /* If preview is running - stop it. */
    res = doStopPreview();
    if (res != NO_ERROR) {
        return -res;
    }

    /* Stop and disconnect the camera device. */
    V4L2CameraDevice* const camera_dev = mV4L2CameraDevice;
    if (camera_dev != NULL) 
	{
        if (camera_dev->isStarted()) 
		{
            camera_dev->stopDeliveringFrames();
            res = camera_dev->stopDevice();
            if (res != NO_ERROR) {
                return -res;
            }
        }
        if (camera_dev->isConnected()) 
		{
            res = camera_dev->disconnectDevice();
            if (res != NO_ERROR) {
                return -res;
            }
        }
    }

    mCallbackNotifier.cleanupCBNotifier();

	{
		Mutex::Autolock locker(&mCameraIdleLock);
		mIsCameraIdle = true;
	}

    return NO_ERROR;
}

/****************************************************************************
 * Camera API callbacks as defined by camera_device_ops structure.
 *
 * Callbacks here simply dispatch the calls to an appropriate method inside
 * CameraHardware instance, defined by the 'dev' parameter.
 ***************************************************************************/

int CameraHardware::set_preview_window(struct camera_device* dev,
                                       struct preview_stream_ops* window)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->setPreviewWindow(window);
}

void CameraHardware::set_callbacks(
        struct camera_device* dev,
        camera_notify_callback notify_cb,
        camera_data_callback data_cb,
        camera_data_timestamp_callback data_cb_timestamp,
        camera_request_memory get_memory,
        void* user)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return;
    }
    ec->setCallbacks(notify_cb, data_cb, data_cb_timestamp, get_memory, user);
}

void CameraHardware::enable_msg_type(struct camera_device* dev, int32_t msg_type)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return;
    }
    ec->enableMsgType(msg_type);
}

void CameraHardware::disable_msg_type(struct camera_device* dev, int32_t msg_type)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return;
    }
    ec->disableMsgType(msg_type);
}

int CameraHardware::msg_type_enabled(struct camera_device* dev, int32_t msg_type)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->isMsgTypeEnabled(msg_type);
}

int CameraHardware::start_preview(struct camera_device* dev)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->startPreview();
}

void CameraHardware::stop_preview(struct camera_device* dev)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return;
    }
    ec->stopPreview();
}

int CameraHardware::preview_enabled(struct camera_device* dev)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->isPreviewEnabled();
}

int CameraHardware::store_meta_data_in_buffers(struct camera_device* dev,
                                               int enable)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->storeMetaDataInBuffers(enable);
}

int CameraHardware::start_recording(struct camera_device* dev)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->startRecording();
}

void CameraHardware::stop_recording(struct camera_device* dev)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return;
    }
    ec->stopRecording();
}

int CameraHardware::recording_enabled(struct camera_device* dev)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->isRecordingEnabled();
}

void CameraHardware::release_recording_frame(struct camera_device* dev,
                                             const void* opaque)
{
	CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return;
    }
    ec->releaseRecordingFrame(opaque);
}

int CameraHardware::auto_focus(struct camera_device* dev)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->setAutoFocus();
}

int CameraHardware::cancel_auto_focus(struct camera_device* dev)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->cancelAutoFocus();
}

int CameraHardware::take_picture(struct camera_device* dev)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->takePicture();
}

int CameraHardware::cancel_picture(struct camera_device* dev)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->cancelPicture();
}

int CameraHardware::set_parameters(struct camera_device* dev, const char* parms)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }

	int64_t lasttime = systemTime();
	int ret = ec->setParameters(parms);
	ALOGV("setParameters use time: %lld(ms)", (systemTime() - lasttime)/1000000);
	
    return ret;
}

char* CameraHardware::get_parameters(struct camera_device* dev)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return NULL;
    }
    return ec->getParameters();
}

void CameraHardware::put_parameters(struct camera_device* dev, char* params)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return;
    }
    ec->putParameters(params);
}

int CameraHardware::send_command(struct camera_device* dev,
                                 int32_t cmd,
                                 int32_t arg1,
                                 int32_t arg2)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->sendCommand(cmd, arg1, arg2);
}

void CameraHardware::release(struct camera_device* dev)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return;
    }
    ec->releaseCamera();
}

int CameraHardware::dump(struct camera_device* dev, int fd)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->dumpCamera(fd);
}


int CameraHardware::get_v4l2_query_ctrl(struct camera_device* dev,
                                 struct v4l2_queryctrl *ctrl, int id)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->getV4l2QueryCtrl(ctrl, id);
}

int CameraHardware::get_input_source(struct camera_device* dev, int *source)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
	return ec->getInputSource(source);
}

int CameraHardware::get_tvin_system(struct camera_device* dev, int *system)
{
    CameraHardware* ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
	return ec->getTVinSystem(system);
}

int CameraHardware::set_waterMark(struct camera_device *dev, int enable, const char* str)
{
#ifdef WATERMARK_ENABLE
    CameraHardware *ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if(ec == NULL){
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }

    return ec->setWaterMark(enable, str);
#else
    return -1;
#endif
}

int CameraHardware::set_uvcGadgetMode(struct camera_device *dev, int value)
{
    CameraHardware *ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if(ec == NULL){
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }

    return ec->setUvcGadgetMode(value);
}

int CameraHardware::set_adasGsensorData(struct camera_device *dev, int val0, float val1, float val2)
{
#ifdef ADAS_ENABLE
    CameraHardware *ec = reinterpret_cast<CameraHardware*>(dev->priv);
    if(ec == NULL){
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }

    return ec->adasSetGsensorData(val0, val1, val2);
#else
    return -1;
#endif
}


int CameraHardware::close(struct hw_device_t* device)
{
    CameraHardware* ec =
        reinterpret_cast<CameraHardware*>(reinterpret_cast<struct camera_device*>(device)->priv);
    if (ec == NULL) {
        ALOGE("%s: Unexpected NULL camera device", __FUNCTION__);
        return -EINVAL;
    }
    return ec->closeCamera();
}

// -------------------------------------------------------------------------
// extended interfaces here <***** star *****>
// -------------------------------------------------------------------------
bool CameraHardware::checkPreviewSizeValid(Size newPreivewSize)
{
    size_t i;
    Vector<Size>    supportedPreviewSizes;
    mParameters.getSupportedPreviewSizes(supportedPreviewSizes);
    for(i=0;i<supportedPreviewSizes.size();i++)
    {
        if(newPreivewSize.width == supportedPreviewSizes[i].width 
            && newPreivewSize.height == supportedPreviewSizes[i].height)
        {
            //LOGD("find the previewSize[%d][%dx%d] we want!", i, newPreivewSize.width, newPreivewSize.height);
            break;
        }
        else
        {
            //LOGD("previewSize[%d][%dx%d] is not we want!", i, mParameters[i].width, mParameters[i].height);
        }
    }
    if(i == supportedPreviewSizes.size())
    {
        LOGW("very bad! wantedPreviewSize[%dx%d] is not support by camera!", newPreivewSize.width, newPreivewSize.height);
        return false;
    }
    else
    {
        return true;
    }
    
}
bool CameraHardware::checkVideoSizeValid(Size newVideoSize)
{
    size_t i;
    Vector<Size>    supportedVideoSizes;
    mParameters.getSupportedVideoSizes(supportedVideoSizes);
    for(i=0;i<supportedVideoSizes.size();i++)
    {
        if(newVideoSize.width == supportedVideoSizes[i].width 
            && newVideoSize.height == supportedVideoSizes[i].height)
        {
            //LOGD("find the videoSize[%d][%dx%d] we want!", i, newVideoSize.width, newVideoSize.height);
            break;
        }
        else
        {
            //LOGD("videoSize[%d][%dx%d] is not we want!", i, supportedVideoSizes[i].width, supportedVideoSizes[i].height);
        }
    }
    if(i == supportedVideoSizes.size())
    {
        LOGW("very bad! newVideoSize[%dx%d] is not support by camera!", newVideoSize.width, newVideoSize.height);
        return false;
    }
    else
    {
        return true;
    }
}
bool CameraHardware::checkPreviewFrameRateValid(int newPreviewFrameRate)
{
    size_t i;
    Vector<int>    supportedFrameRates;
    mParameters.getSupportedPreviewFrameRates(supportedFrameRates);
    for(i=0;i<supportedFrameRates.size();i++)
    {
        if(newPreviewFrameRate == supportedFrameRates[i])
        {
            //LOGD("find the frameRate[%d][%d] we want!", i, newPreviewFrameRate);
            break;
        }
        else
        {
            //LOGD("frameRate[%d][%d] is not we want!", i, supportedFrameRates[i]);
        }
    }
    if(i == supportedFrameRates.size())
    {
        LOGW("very bad! wantedFrameRate[%d] is not support by camera!", newPreviewFrameRate);
        return false;
    }
    else
    {
        return true;
    }
}
/****************************************************************************
 * Static initializer for the camera callback API
 ****************************************************************************/

camera_device_ops_t CameraHardware::mDeviceOps = {
    CameraHardware::set_preview_window,
    CameraHardware::set_callbacks,
    CameraHardware::enable_msg_type,
    CameraHardware::disable_msg_type,
    CameraHardware::msg_type_enabled,
    CameraHardware::start_preview,
    CameraHardware::stop_preview,
    CameraHardware::preview_enabled,
    CameraHardware::store_meta_data_in_buffers,
    CameraHardware::start_recording,
    CameraHardware::stop_recording,
    CameraHardware::recording_enabled,
    CameraHardware::release_recording_frame,
    CameraHardware::auto_focus,
    CameraHardware::cancel_auto_focus,
    CameraHardware::take_picture,
    CameraHardware::cancel_picture,
    CameraHardware::set_parameters,
    CameraHardware::get_parameters,
    CameraHardware::put_parameters,
    CameraHardware::send_command,
    CameraHardware::release,
    CameraHardware::dump,
    CameraHardware::get_v4l2_query_ctrl,
    CameraHardware::get_input_source,
    CameraHardware::get_tvin_system,
    CameraHardware::set_waterMark,
    CameraHardware::set_uvcGadgetMode,
    CameraHardware::set_adasGsensorData,
};

/****************************************************************************
 * Common keys
 ***************************************************************************/

const char CameraHardware::FACING_KEY[]         = "prop-facing";
const char CameraHardware::ORIENTATION_KEY[]    = "prop-orientation";
const char CameraHardware::RECORDING_HINT_KEY[] = "recording-hint";

/****************************************************************************
 * Common string values
 ***************************************************************************/

const char CameraHardware::FACING_BACK[]      = "back";
const char CameraHardware::FACING_FRONT[]     = "front";

/****************************************************************************
 * Helper routines
 ***************************************************************************/

static char* AddValue(const char* param, const char* val)
{
    const size_t len1 = strlen(param);
    const size_t len2 = strlen(val);
    char* ret = reinterpret_cast<char*>(malloc(len1 + len2 + 2));
    LOGE_IF(ret == NULL, "%s: Memory failure", __FUNCTION__);
    if (ret != NULL) {
        memcpy(ret, param, len1);
        ret[len1] = ',';
        memcpy(ret + len1 + 1, val, len2);
        ret[len1 + len2 + 1] = '\0';
    }
    return ret;
}

/****************************************************************************
 * Parameter debugging helpers
 ***************************************************************************/

#if DEBUG_PARAM
static void PrintParamDiff(const CameraParameters& current,
                            const char* new_par)
{
    char tmp[2048];
    const char* wrk = new_par;

    /* Divided with ';' */
    const char* next = strchr(wrk, ';');
    while (next != NULL) {
        snprintf(tmp, sizeof(tmp), "%.*s", next-wrk, wrk);
        /* in the form key=value */
        char* val = strchr(tmp, '=');
        if (val != NULL) {
            *val = '\0'; val++;
            const char* in_current = current.get(tmp);
            if (in_current != NULL) {
                if (strcmp(in_current, val)) {
                    LOGD("=== Value changed: %s: %s -> %s", tmp, in_current, val);
                }
            } else {
                LOGD("+++ New parameter: %s=%s", tmp, val);
            }
        } else {
            LOGW("No value separator in %s", tmp);
        }
        wrk = next + 1;
        next = strchr(wrk, ';');
    }
}
#endif  /* DEBUG_PARAM */

}; /* namespace android */
