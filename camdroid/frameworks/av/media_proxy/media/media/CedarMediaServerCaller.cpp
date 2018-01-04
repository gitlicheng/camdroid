//#define LOG_NDEBUG 0
#define LOG_TAG "CedarMediaServerCaller"
#include <utils/Log.h>
#include <unistd.h>
#include <fcntl.h>

#include <CedarMediaServerCaller.h>


namespace android {

CedarMediaServerCaller::CedarMediaServerCaller()
{
    ALOGV("constructor");
    mCaller = new MediaServerCaller();
    if (mCaller == NULL) {
        ALOGE("Failed to alloc MediaServerCaller");
    }
}


CedarMediaServerCaller::~CedarMediaServerCaller()
{
    ALOGV("desstructor");
}

sp<IMemory> CedarMediaServerCaller::jpegDecode(int fd)
{
    if (fd < 0) {
        ALOGE("Invalid fd(%d)", fd);
        return NULL;
    }
    if (mCaller == NULL) {
        ALOGE("MediaServerCaller not initialize!");
        return NULL;
    }
    return mCaller->jpegDecode(fd);
}

sp<IMemory> CedarMediaServerCaller::jpegDecode(String8 path)
{
    if (mCaller == NULL) 
    {
        ALOGE("MediaServerCaller not initialize!");
        return NULL;
    }
    if(path.isEmpty())
    {
        ALOGE("(f:%s, l:%d) path is empty", __FUNCTION__, __LINE__);
        return NULL;
    }
    if (access(path.string(), F_OK) == 0) 
    {
        int fd = open(path.string(), O_RDONLY);
        if (fd < 0) 
        {
            ALOGE("Failed to open file %s(%s)", path.string(), strerror(errno));
            return NULL;
        }
        sp<IMemory> mem = jpegDecode(fd);
        ::close(fd);
        return mem;
    } 
    else 
    {
        return mCaller->jpegDecode(path.string());
    }
}

}; /* namespace android */

