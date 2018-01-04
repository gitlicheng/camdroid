#include "../CameraDebug.h"
#if DBG_ADAS_CAMERA
#define LOG_NDEBUG 0
#endif
#define LOG_TAG "CameraAdas"

#include <cutils/log.h>
#include <sys/mman.h> 
#include <hardware/camera.h>
#include <utils/Thread.h>
#include <type_camera.h>

#include <wTable.h>

#include "../V4L2CameraDevice2.h"
#include "../CallbackNotifier.h"
#include "CameraAdas.h"


#define ADAS_SAVE_720P_DATA_PATH			"/data/adas_720P"
#define ADAS_SAVE_1080P_DATA_PATH			"/data/adas_1080P"
#define ADAS_SAVE_1080P_DATA_PATH_THUMB		"/data/adas_1080P_thumb"

namespace android {

static unsigned int  s_adasCount =0;
static int s_openadasscreen = true;

CameraAdas::CameraAdas(CallbackNotifier *cb, void *dev)
    : mCallbackNotifier(cb)
    , mV4L2CameraDevice(dev)
    , mV4l2Buf(NULL)
    , mStarted(false)
{
	LOGD("InitialAdas DrawAdasThread");
	mAdasTh= new DrawAdasThread(this);
	mAdasTh->start();
	mAdasCars = new AdasCars();
	mIsDrawNow = true;
	adasCount=1;

	//madasdata.adasOut = (ADASOUT*)calloc(1, sizeof(CARS)*CAR_CNT+sizeof(LANE));
	madasdata.adasOut = (ADASOUT*)calloc(1, sizeof(ADASOUT));
	madasdata.adasOut->cars.carP = (CAR *)calloc(1, sizeof(CAR)*CAR_CNT);

	isStop = 0;
    vSpeed = 0;
    aSpeed = 0;
	Adasmode = 0;
	
#ifdef ADAS_WAIT_CAP_FRAME
    pthread_mutex_init(&mMutex, NULL);
    pthread_cond_init(&mCond, NULL);
#else
    pthread_mutex_init(&mlock, NULL);
#endif
}

CameraAdas::~CameraAdas()
{

#ifdef ADAS_WAIT_CAP_FRAME
    pthread_mutex_destroy(&mMutex);
    pthread_cond_destroy(&mCond);
#else
    pthread_mutex_destroy(&mlock);
#endif
}

status_t CameraAdas::readData(ADASIN& adasin)
{
    FILE *fp;
    int ret;
    char path[256];

    adasin.vanishLine = 0;
    adasin.vanishLineIsOk = 0;
    adasin.table.imgSize.width = 4;
    adasin.table.imgSize.height = adasin.imgSizeSmall.height;
    if (adasin.imgSizeSmall.height == 720) {
        adasin.table.ptr = Wtable720p;
        snprintf(path, 256, "%s", ADAS_SAVE_720P_DATA_PATH);
    } else {
        adasin.table.ptr= Wtable1080p;
        snprintf(path, 256, "%s", ADAS_SAVE_1080P_DATA_PATH);
    }
    if (access(path, F_OK) != 0) {
        return 0;
    }
    fp = fopen(path, "r");
    if (NULL == fp) {
        LOGE("Failed to open file %s(%s)!", path, strerror(errno));
        return -1;
    }

    ret = fread(&adasin.vanishLine, sizeof(unsigned short), 1, fp);
    if (ret < 0) goto READ_ERR;
    ret = fread(&adasin.vanishLineIsOk, sizeof(unsigned char), 1, fp);
    if (ret < 0) goto READ_ERR;
    ret = fread(&adasin.table.imgSize.width, sizeof(int), 1, fp);
    if (ret < 0) goto READ_ERR;
    ret = fread(&adasin.table.imgSize.height, sizeof(int), 1, fp);
    if (ret < 0) goto READ_ERR;

    ret = fread(adasin.table.ptr, sizeof(unsigned short) * adasin.table.imgSize.width * adasin.table.imgSize.height, 1, fp);
    if (ret < 0) goto READ_ERR;

    fclose(fp);
    return 0;

    READ_ERR:
    LOGE("Failed to read file %s!", path);
    fclose(fp);
    return -1;
}

status_t CameraAdas::writeData(SAVEPARA& para)
{
    FILE *fp;
    int ret;
    char path[256];

    if (para.table.imgSize.height == 720) {
        snprintf(path, 256, "%s", ADAS_SAVE_720P_DATA_PATH);
    } else {
        snprintf(path, 256, "%s", ADAS_SAVE_1080P_DATA_PATH);
    }

    fp = fopen(path, "w+");
    if (NULL == fp) {
        LOGE("Failed to open file %s(%s)!", path, strerror(errno));
        return -1;
    }

    ret = fwrite(&para.vanishLine, sizeof(unsigned short), 1, fp);
    if (ret < 0) goto WRITE_ERR;
    ret = fwrite(&para.vanishLineIsOk, sizeof(unsigned char), 1, fp);
    if (ret < 0) goto WRITE_ERR;
    ret = fwrite(&para.table.imgSize.width, sizeof(int), 1, fp);
    if (ret < 0) goto WRITE_ERR;
    ret = fwrite(&para.table.imgSize.height, sizeof(int), 1, fp);
    if (ret < 0) goto WRITE_ERR;
    ret = fwrite(para.table.ptr, sizeof(unsigned short) * para.table.imgSize.width * para.table.imgSize.height, 1, fp);
    if (ret < 0) {
        goto WRITE_ERR;
    }

    fclose(fp);
    return 0;

    WRITE_ERR:
    LOGE("Failed to write file %s!", path);
    fclose(fp);
    return -1;
}

status_t CameraAdas::getCurrentFrame(FRAMEIN &imgIn)
{
    int ret;
    struct timeval now;
    struct timespec timeout;

    if (!mStarted) {
        LOGW(" Adas already stop!");
        imgIn.imgSmall.ptr = NULL;
        imgIn.imgSmall.imgSize.width = mSmallImgWidth;
        imgIn.imgSmall.imgSize.height = mSmallImgHeight;

        imgIn.imgBig.ptr = NULL;
        imgIn.imgBig.imgSize.width = mBigImgWidth;
        imgIn.imgBig.imgSize.height = mBigImgHeight;

        imgIn.sensity.fcwSensity = mDistanceDetectLevel;
        imgIn.sensity.ldwSensity = mLaneLineOffsetSensity;
        imgIn.gps.speed = mCarSpeed;
        imgIn.gps.enable = mCarSpeed >= 0 ? true : false;
        return 0;
    }
#ifdef ADAS_WAIT_CAP_FRAME
    gettimeofday(&now, NULL);
    timeout.tv_sec= now.tv_sec + 1;
    timeout.tv_nsec = now.tv_usec * 1000;
    pthread_mutex_lock(&mMutex);
    ret = pthread_cond_timedwait(&mCond, &mMutex, &timeout);
    pthread_mutex_unlock(&mMutex);
    if (ret == ETIMEDOUT) {
        LOGW("<F:%s, L:%d> pthread_cond_timedwait time out!", __FUNCTION__, __LINE__);
    }

    if (!mStarted) {
        LOGW("Adas already stop after wait!");
        imgIn.imgSmall.ptr = NULL;
        imgIn.imgSmall.imgSize.width = mSmallImgWidth;
        imgIn.imgSmall.imgSize.height = mSmallImgHeight;

        imgIn.imgBig.ptr = NULL;
        imgIn.imgBig.imgSize.width = mBigImgWidth;
        imgIn.imgBig.imgSize.height = mBigImgHeight;

        imgIn.sensity.fcwSensity = mDistanceDetectLevel;
        imgIn.sensity.ldwSensity = mLaneLineOffsetSensity;
        imgIn.gps.speed = mCarSpeed;
        imgIn.gps.enable = mCarSpeed >= 0 ? true:false;
        return 0;
    }
#else
    pthread_mutex_lock(&mlock);
#endif

    if (mV4l2Buf == NULL || mV4l2Buf->addrVirY == 0) {
#ifndef ADAS_WAIT_CAP_FRAME
        pthread_mutex_unlock(&mlock);
#endif
        LOGW("Adas already stop after wait!");
        imgIn.imgSmall.ptr = NULL;
        imgIn.imgSmall.imgSize.width = mSmallImgWidth;
        imgIn.imgSmall.imgSize.height = mSmallImgHeight;

        imgIn.imgBig.ptr = NULL;
        imgIn.imgBig.imgSize.width = mBigImgWidth;
        imgIn.imgBig.imgSize.height = mBigImgHeight;

        imgIn.sensity.fcwSensity = mDistanceDetectLevel;
        imgIn.sensity.ldwSensity = mLaneLineOffsetSensity;
        imgIn.gps.speed = mCarSpeed;
        imgIn.gps.enable = mCarSpeed >= 0 ? true:false;
        return 0;
    }

    imgIn.imgSmall.ptr = (unsigned char*)mV4l2Buf->addrVirY + ALIGN_4K(ALIGN_16B(mV4l2Buf->width) * ALIGN_16B(mV4l2Buf->height) * 3 / 2);
    imgIn.imgBig.ptr = (unsigned char*)mV4l2Buf->addrVirY;
    imgIn.imgSmall.imgSize.width = ALIGN_16B(mV4l2Buf->thumb_crop_rect.width);
    imgIn.imgSmall.imgSize.height = mV4l2Buf->thumb_crop_rect.height;
    imgIn.imgBig.imgSize.width = mV4l2Buf->crop_rect.width;
    imgIn.imgBig.imgSize.height = mV4l2Buf->crop_rect.height;
    //LOGV("bigSize[%dx%d]crop[%dx%d], smallSize[%dx%d]crop[%dx%d]",
    //    mV4l2Buf->width, mV4l2Buf->height, mV4l2Buf->crop_rect.width,mV4l2Buf->crop_rect.height,
    //    mV4l2Buf->thumbWidth, mV4l2Buf->thumbHeight, mV4l2Buf->thumb_crop_rect.width, mV4l2Buf->thumb_crop_rect.height);
#ifndef ADAS_WAIT_CAP_FRAME
    pthread_mutex_unlock(&mlock);
#endif

    imgIn.sensity.fcwSensity = mDistanceDetectLevel;
    imgIn.sensity.ldwSensity = mLaneLineOffsetSensity;
    imgIn.gps.speed = mCarSpeed;
    imgIn.gps.enable = mCarSpeed >= 0 ? true : false;

//initialize adas detect sensity and v3 has no gps
	//imgIn.sensity.fcwSensity = 1;
    imgIn.sensity.ldwSensity = 1;
    imgIn.gps.speed = 0;
    imgIn.gps.enable = false;
   // LOGD("isStop:%d,aSpeed:%f,vSpeed:%f",isStop,aSpeed,vSpeed);	
    imgIn.gsensor.isStop = isStop;
    imgIn.gsensor.a = aSpeed;
    imgIn.gsensor.v = vSpeed;
	imgIn.mode = Adasmode;

	return 0;
}

status_t CameraAdas::camDataReady(V4L2BUF_t *v4l2_buf)
{
    if (!mStarted) {
        return 0;
    }
#ifndef ADAS_WAIT_CAP_FRAME
    pthread_mutex_lock(&mlock);
#endif
    mV4l2Buf = v4l2_buf;
#ifndef ADAS_WAIT_CAP_FRAME
    pthread_mutex_unlock(&mlock);
#endif
#ifdef ADAS_WAIT_CAP_FRAME
    pthread_mutex_lock(&mMutex);
    pthread_cond_signal(&mCond);
    pthread_mutex_unlock(&mMutex);
#endif
    return 0;
}

void AdasInCallBack(FRAMEIN &imgIn, void *dev)
{
    CameraAdas *d = (CameraAdas *)dev;
    d->getCurrentFrame(imgIn);
}

void AdasOutCallBack(ADASOUT &adasOut, void *dev)
{
    //int64_t time0 = getSystemTimeUs();
    CameraAdas *d = (CameraAdas *)dev;
    CallbackNotifier *cb = d->getCallbackNotifier();
    V4L2CameraDevice *v4l2CamDev = (V4L2CameraDevice *)d->getV4L2CameraDevice();

    if (adasOut.savePara.isSave) {
        LOGD("save data");
        d->writeData(adasOut.savePara);
    }

    if (!d->getStatus()) {
        LOGW("Adas already stop!");
        return;
    }
    ADASOUT_IF adasOutIf;
    adasOutIf.subWidth = d->getPreviewWidth();
    adasOutIf.subHeight = d->getPreviewHeight();
#if 0
    ADASOUT out;
    AdasSetData(out);
    adasOutIf.adasOut = &out;
    cb->adasDetectionMsg(&adasOutIf);
    AdasFreeData(&out);
#else
    //LOGD("------------------------------------num %d, disp %d", adasOut.cars.Num, adasOut.lane.isDisp);
    adasOutIf.adasOut = &adasOut;
    v4l2CamDev->adasSetRoiArea(&adasOutIf);
#if 1
//	if((d->isAdasWarn(adasOut) || s_adasCount++%3==0) && d->getIsDrawNow()){
	int warnType = d->isAdasWarn(adasOut);
	static int warnSleep = 0;
	if(warnSleep > 0){
		warnSleep ++ ;
		if(warnSleep == 30)
			warnSleep = 0;
		return ;
	}
	if(((warnType > 0) || (s_adasCount++%3==0)) && d->getIsDrawNow()){
		if((warnType == 1)||(warnType == 3) )   // ImageWarn + VoiceWarn don't Update AdasLayer
			warnSleep = 1;
		d->setAdasIfData(&adasOutIf);
		d->adasSignal();
	}
#else
	if(d->getIsDrawNow()){
		d->setAdasIfData(&adasOutIf);
		d->adasSignal();
	}
#endif
	if(s_openadasscreen){
    	cb->adasDetectionMsg(&adasOutIf);
		s_openadasscreen = false;
	}
#endif
    //int64_t timeInterval = getSystemTimeUs() - time0;
    //if (timeInterval > 10*1000) {
    //    LOGD("AdasOutCallBack time %lldms", timeInterval/1000);
    //}
}

status_t CameraAdas::setLaneLineOffsetSensity(int level)
{
    mLaneLineOffsetSensity = level;
    return NO_ERROR;
}

status_t CameraAdas::setDistanceDetectLevel(int level)
{
    mDistanceDetectLevel = level;
    return NO_ERROR;
}

status_t CameraAdas::adasSetMode(int mode)
{
	Adasmode = mode;
	return NO_ERROR;
}

status_t CameraAdas::adasSetTipsMode(int mode)
{
	if(mAdasCars != NULL){
		mAdasCars->adasSetTipsMode(mode);
	}
	return NO_ERROR;
}

status_t CameraAdas::setCarSpeed(float speed)
{
    mCarSpeed = speed;
    return NO_ERROR;
}

status_t CameraAdas::adasSetGsensorData(int val0, float val1, float val2)
{
       isStop = val0;
       aSpeed = val1;
       vSpeed = val2;
       return NO_ERROR;
}

status_t CameraAdas::initialize(int width, int height, int thumbWidth, int thumbHeight, int fps)
{
    ADASIN adasIn;
    int ang_h, ang_v;

    if (mStarted) {
        LOGW("Adas already running");
        return 0;
    }
    callbackIn = AdasInCallBack;
    callbackOut = AdasOutCallBack;

    ang_h = ((V4L2CameraDevice*)mV4L2CameraDevice)->getHorVisualAngle();
    if (ang_h < 0) {
        ang_h = 105;
    }
    ang_v = ((V4L2CameraDevice*)mV4L2CameraDevice)->getVerVisualAngle();
    if (ang_v < 0) {
        ang_v = 62;
    }
    adasIn.fps = fps;

    adasIn.imgSizeSmall.width = thumbWidth;
    adasIn.imgSizeSmall.height = thumbHeight;

    adasIn.imgSizeBig.width = width;
    adasIn.imgSizeBig.height = height;

    adasIn.viewAng.widthOri = width;
    adasIn.viewAng.heightOri = height;

    mSmallImgWidth = thumbWidth;
    mSmallImgHeight = thumbHeight;

    mBigImgWidth = width;
    mBigImgHeight = height;

    adasIn.viewAng.angH = (unsigned char)ang_h;
    adasIn.viewAng.angV = (unsigned char)ang_v;
    readData(adasIn);

	adasIn.config.isGps = 0;
	adasIn.config.isGsensor = 1;
	adasIn.config.isObd = 0;
	adasIn.config.isCalibrate = 0;

    s_adasCount = 0;
    s_openadasscreen = true;

    LOGD("FrameRate=%d, width=%d, height=%d, thumbWidth=%d, thumbHeight=%d, angH=%d, angV=%d",
    fps, width, height, thumbWidth, thumbHeight, ang_h, ang_v);
    InitialAdas(adasIn, (void **)this);
    LOGD("InitialAdas end");
    mStarted = true;
    return NO_ERROR;
}

void CameraAdas::setAdasIfData(ADASOUT_IF *data)
{
	madasdata.subHeight = data->subHeight;
	madasdata.subWidth  = data->subWidth;
	madasdata.adasOut->cars.Num = data->adasOut->cars.Num;
	memcpy(madasdata.adasOut->cars.carP, data->adasOut->cars.carP, sizeof(CAR)*CAR_CNT);
    memcpy(&madasdata.adasOut->lane, &data->adasOut->lane, sizeof(LANE));
}

int CameraAdas::isAdasWarn(ADASOUT &adas_out)
{
	int mWarn = 0;
	for (int i = 0; i < adas_out.cars.Num && i < CAR_CNT; ++i) {
		if(adas_out.cars.carP[i].isWarn != 0 ){
			mWarn = 1;
			return mWarn;
		}
	}
	if (adas_out.lane.isDisp) {
		if((adas_out.lane.ltWarn == 128) || (adas_out.lane.rtWarn == 128)){
			mWarn = 2;
		}else if((adas_out.lane.ltWarn == 255) || (adas_out.lane.rtWarn == 255)){
			mWarn = 3;
		}
	}
	return mWarn;
}


void CameraAdas::adasUpdateScreen()
{
	if(mAdasCars != NULL){
		mAdasCars->drawCars(madasdata);
	}
}

void CameraAdas::setAdasAudioVol(float val)
{
	AdasCars::setAdasAudioVol(val);
}

status_t CameraAdas::destroy()
{
    if (!mStarted) {
        LOGW("Adas is not running yet!");
        return NO_ERROR;
    }

    LOGD("start");
    mStarted = false;
#ifdef ADAS_WAIT_CAP_FRAME
    pthread_mutex_lock(&mMutex);
    pthread_cond_signal(&mCond);
    pthread_mutex_unlock(&mMutex);
#endif

    LOGD("ReleaseAdas start");
    ReleaseAdas();
    LOGD("ReleaseAdas end");

	LOGD("Delete mAdasTh");
	if(mAdasTh != NULL) {
		mAdasTh->stopThread();
		adasSignal();
		mAdasTh.clear();
		mAdasTh = 0;
	}
    if (mAdasCars != NULL) {
		delete mAdasCars;
    }
	if (madasdata.adasOut->cars.carP != NULL) {
		free(madasdata.adasOut->cars.carP);
	}
	if (madasdata.adasOut != NULL) {
		free(madasdata.adasOut);
	}

    return NO_ERROR;
}

}; /* namespace android */

