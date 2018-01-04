#ifndef __FS_WRITER_H__
#define __FS_WRITER_H__

#include <utils/Log.h>
#include <CDX_Types.h>
#include <errno.h>
#include <include_omx/OMX_Types.h>
#include <cedarx_stream.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum tag_FSWRITEMODE {
    FSWRITEMODE_CACHETHREAD = 0,
    FSWRITEMODE_SIMPLECACHE,
    FSWRITEMODE_DIRECT,
}FSWRITEMODE;

typedef struct FsCacheMemInfo
{
    OMX_S8              *mpCache;
    OMX_U32             mCacheSize;
}FsCacheMemInfo;

typedef struct tag_FsWriter FsWriter;
typedef struct tag_FsWriter
{
    FSWRITEMODE mMode;
    ssize_t (*fsWrite)(FsWriter *thiz, const OMX_S8 *buf, size_t size);
    OMX_S32 (*fsSeek)(FsWriter *thiz, OMX_S64 nOffset, OMX_S32 fromWhere);
    OMX_S64 (*fsTell)(FsWriter *thiz);
    OMX_S32 (*fsTruncate)(FsWriter *thiz, OMX_S64 nLength);
    OMX_S32 (*fsFlush)(FsWriter *thiz);
    void *mPriv;
}FsWriter;
FsWriter* createFsWriter(FSWRITEMODE mode, struct cdx_stream_info *pStream, OMX_S8 *pCache, OMX_U32 nCacheSize, OMX_U32 vCodec);
OMX_S32 destroyFsWriter(FsWriter *thiz);

extern ssize_t fileWriter(struct cdx_stream_info *pStream, const OMX_S8 *buffer, size_t size);

#if defined(__cplusplus)
}
#endif

#endif
