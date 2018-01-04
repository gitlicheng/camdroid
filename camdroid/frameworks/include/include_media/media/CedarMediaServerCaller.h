#ifndef __CEDAR_MEDIA_SERVER_CALLER_H_
#define __CEDAR_MEDIA_SERVER_CALLER_H_

#include <binder/IMemory.h>
#include <media/mediaservercaller.h>
#include <utils/String8.h>

namespace android {

class CedarMediaServerCaller
{
public:
    CedarMediaServerCaller();
    ~CedarMediaServerCaller();

    sp<IMemory> jpegDecode(int fd);
    sp<IMemory> jpegDecode(String8 path);

private:
    sp<MediaServerCaller> mCaller;
};

}; /* namespace android */

#endif /* __CEDAR_MEDIA_SERVER_CALLER_H_ */

