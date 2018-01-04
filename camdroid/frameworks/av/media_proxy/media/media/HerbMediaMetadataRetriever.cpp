//#define LOG_NDEBUG 0
#define LOG_TAG "HerbMediaMetadataRetriever"

#include <utils/Log.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <HerbMediaMetadataRetriever.h>


namespace android {

HerbMediaMetadataRetriever::HerbMediaMetadataRetriever()
{
    ALOGV("HerbMediaMetadataRetriever constructor");
    mRetriever = new MediaMetadataRetriever();
	if (mRetriever == NULL) {
		ALOGE("<F:%s, L:%d>Out of memory", __FUNCTION__, __LINE__);
	}
}

HerbMediaMetadataRetriever::~HerbMediaMetadataRetriever()
{
    ALOGV("HerbMediaMetadataRetriever destructor");
	if (mRetriever != NULL) {
		delete mRetriever;
		mRetriever = NULL;
	}
}

void HerbMediaMetadataRetriever::process_media_retriever_call(status_t opStatus, const char *message)
{
	if (opStatus == (status_t)INVALID_OPERATION) {
		ALOGE("INVALID_OPERATION");
	} else if (opStatus == (status_t)PERMISSION_DENIED) {
		ALOGE("PERMISSION_DENIED");
	} else if (opStatus != (status_t)OK) {
		ALOGE("%s", message);
	}
}

status_t HerbMediaMetadataRetriever::setDataSource(int fd, int64_t offset, int64_t length)
{
	ALOGV("<F:%s, L:%d>fd(%d), offset(%lld), length(%lld)", __FUNCTION__, __LINE__, fd, offset, length);
	if (mRetriever == NULL) {
		ALOGE("<F:%s, L:%d> MediaMetadataRetriever not initialize", __FUNCTION__, __LINE__);
		return NO_INIT;
	}
	if (fd < 0 || offset < 0 || length < 0) {
		ALOGE("<F:%s, L:%d> Invalid input, fd(%d), offset(%lld), length(%lld)", __FUNCTION__, __LINE__, fd, offset, length);
		return BAD_VALUE;
	}

	status_t opStatus = mRetriever->setDataSource(fd, offset, length);
	process_media_retriever_call(opStatus, "setDataSource failed");
	return opStatus;
}

status_t HerbMediaMetadataRetriever::setDataSource(const char *uri, const KeyedVector<String8, String8> *headers)
{
	ALOGV("<F:%s, L:%d>uri(%s)", __FUNCTION__, __LINE__, uri);
	if (mRetriever == NULL) {
		ALOGE("<F:%s, L:%d> MediaMetadataRetriever not initialize", __FUNCTION__, __LINE__);
		return NO_INIT;
	}
	if (uri == NULL) {
		ALOGE("<F:%s, L:%d> Invalid input parameters", __FUNCTION__, __LINE__);
		return BAD_VALUE;
	}

	status_t opStatus = mRetriever->setDataSource(uri, (headers && headers->size() > 0) ? headers : NULL);
	process_media_retriever_call(opStatus, "setDataSource failed");
	return opStatus;
}

status_t HerbMediaMetadataRetriever::setDataSource(const char *path)
{
	ALOGV("<F:%s, L:%d>path(%s)", __FUNCTION__, __LINE__, path);
	if (mRetriever == NULL) {
		ALOGE("<F:%s, L:%d> MediaMetadataRetriever not initialize", __FUNCTION__, __LINE__);
		return NO_INIT;
	}
	if (path == NULL) {
		ALOGE("<F:%s, L:%d> Invalid path", __FUNCTION__, __LINE__);
		return BAD_VALUE;
	}
    if (access(path, F_OK) == 0)
    {
        int fd = open(path, O_RDWR);
    	if (fd < 0) {
    		ALOGE("Failed to open file %s(%s)", path, strerror(errno));
    		return UNKNOWN_ERROR;
    	}
    	status_t opStatus = setDataSource(fd, 0, 0x7ffffffffffffffL);
        ::close(fd);
        process_media_retriever_call(opStatus, "setDataSource failed");
        return opStatus;
    }
	else
	{
        return setDataSource(path, NULL);
	}
}

status_t HerbMediaMetadataRetriever::setDataSource(int fd)
{
	ALOGV("<F:%s, L:%d>fd(%d)", __FUNCTION__, __LINE__, fd);
	if (mRetriever == NULL) {
		ALOGE("<F:%s, L:%d> MediaMetadataRetriever not initialize", __FUNCTION__, __LINE__);
		return NO_INIT;
	}
	if (fd < 0) {
		ALOGE("<F:%s, L:%d> Invalid input, fd(%d)", __FUNCTION__, __LINE__, fd);
		return BAD_VALUE;
	}

	status_t opStatus = mRetriever->setDataSource(fd, 0, 0x7ffffffffffffffL);
	process_media_retriever_call(opStatus, "setDataSource failed");
	return opStatus;
}

const char *HerbMediaMetadataRetriever::extractMetadata(int keyCode)
{
	ALOGV("<F:%s, L:%d>keyCode(%d)", __FUNCTION__, __LINE__, keyCode);
	if (mRetriever == NULL) {
		ALOGE("<F:%s, L:%d> MediaMetadataRetriever not initialize", __FUNCTION__, __LINE__);
		return NULL;
	}
    const char *value = mRetriever->extractMetadata(keyCode);
    if (value == NULL) {
        ALOGV("extractMetadata: Metadata is not found");
        return NULL;
    }
	return value;
}

sp<IMemory> HerbMediaMetadataRetriever::getFrameAtTime(int64_t timeUs, int option)
{
	ALOGV("<F:%s, L:%d>timeUs(%lld), option(%d)", __FUNCTION__, __LINE__, timeUs, option);
	if (mRetriever == NULL) {
		ALOGE("<F:%s, L:%d> MediaMetadataRetriever not initialize", __FUNCTION__, __LINE__);
		return NULL;
	}
	if (option < OPTION_PREVIOUS_SYNC || option > OPTION_CLOSEST) {
		ALOGE("Unsupported option: %d", option);
		return NULL;
	}
	return mRetriever->getFrameAtTime(timeUs, option);
}

sp<IMemory> HerbMediaMetadataRetriever::getFrameAtTime(int64_t timeUs)
{
	return getFrameAtTime(timeUs, OPTION_CLOSEST_SYNC);
}

sp<IMemory> HerbMediaMetadataRetriever::getFrameAtTime()
{
	return getFrameAtTime(-1, OPTION_CLOSEST_SYNC);
}

sp<IMemory> HerbMediaMetadataRetriever::getEmbeddedPicture(int pictureType)
{
	ALOGV("<F:%s, L:%d>pictureType(%d)", __FUNCTION__, __LINE__, pictureType);
	if (mRetriever == NULL) {
		ALOGE("<F:%s, L:%d> MediaMetadataRetriever not initialize", __FUNCTION__, __LINE__);
		return NULL;
	}

	return mRetriever->extractAlbumArt();
}

};  /* namespace android */
