#ifndef __HERB_MEDIA_METADATA_RETRIEVER_H__
#define __HERB_MEDIA_METADATA_RETRIEVER_H__

#include <utils/KeyedVector.h>
#include <utils/String8.h>
#include <media/mediametadataretriever.h>

namespace android {

enum found_frame_type {
	OPTION_PREVIOUS_SYNC = 0,
	OPTION_NEXT_SYNC,
	OPTION_CLOSEST_SYNC,
	OPTION_CLOSEST,
	OPTION_LIST_END
};

class HerbMediaMetadataRetriever
{
public:
	HerbMediaMetadataRetriever();
	~HerbMediaMetadataRetriever();

	status_t setDataSource(const char *path);
	status_t setDataSource(const char *uri, const KeyedVector<String8, String8> *headers);
	status_t setDataSource(int fd);
	const char *extractMetadata(int keyCode);
	sp<IMemory> getFrameAtTime(int64_t timeUs, int option);
	sp<IMemory> getFrameAtTime(int64_t timeUs);
	sp<IMemory> getFrameAtTime();
	sp<IMemory> getEmbeddedPicture(int pictureType);
	void release();

private:
	status_t setDataSource(int fd, int64_t offset, int64_t length);
	static void process_media_retriever_call(status_t opStatus, const char *message);
	MediaMetadataRetriever *mRetriever;
};

};  /* namespace android */

#endif  /* _HERBMEDIARECORDER_H_ */

