/*******************************************************************************
--                                                                            --
--                    CedarX Multimedia Framework                             --
--                                                                            --
--          the Multimedia Framework for Linux/Android System                 --
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                         Softwinner Products.                               --
--                                                                            --
--                   (C) COPYRIGHT 2011 SOFTWINNER PRODUCTS                   --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
*******************************************************************************/

//#define LOG_NDEBUG 0
#define LOG_TAG "cedarx_stream_file"
#include <CDX_Debug.h>

#include "cedarx_stream.h"
#include "cedarx_stream_file.h"
#include <tsemaphore.h>
#include <errno.h>
#include <assert.h>
#include <ConfigOption.h>
#include <stdlib.h>
#if (CDXCFG_FILE_SYSTEM==OPTION_FILE_SYSTEM_DIRECT_FATFS)
#include <fat_user.h>
static const int FATFS_BLOCK_SIZE = (64*1024);
#elif (CDXCFG_FILE_SYSTEM==OPTION_FILE_SYSTEM_DIRECT_IO)
static const int DIRECTIO_UNIT_SIZE = (64*1024);   //cluster is 64*1024, align by cluster unit will has the max speed. (512)
static const int DIRECTIO_USER_MEMORY_ALIGN = (512);

#endif

static char *generateFilepathFromFd(const int fd)
{
    char fdPath[1024];
    char absoluteFilePath[1024];
    int ret;
    if(fd < 0)
    {
        ALOGE("(f:%s, l:%d) fatal error! fd[%d] wrong", __FUNCTION__, __LINE__, fd);
        return NULL;
    }

    snprintf(fdPath, sizeof(fdPath), "/proc/self/fd/%d", fd);
    ret = readlink(fdPath, absoluteFilePath, 1024 - 1);
    if (ret == -1)
    {
        ALOGE("(f:%s, l:%d) fatal error! readlink[%s] failure, errno(%s)", __FUNCTION__, __LINE__, fdPath, strerror(errno));
        return NULL;
    }
    absoluteFilePath[ret] = '\0';
    ALOGD("(f:%s, l:%d) readlink[%s], filePath[%s]", __FUNCTION__, __LINE__, fdPath, absoluteFilePath);
    return strdup(absoluteFilePath);
}

int cdx_seek_stream_file(struct cdx_stream_info *stream, cdx_off_t offset, int whence)
{
#if (CDXCFG_FILE_SYSTEM==OPTION_FILE_SYSTEM_DIRECT_FATFS)
    unsigned int nAbsoluteOffset;

    switch(whence)
    {
        case SEEK_SET:
        {
            stream->read_cache.data_len = 0;
            stream->read_cache.buf_pos = stream->read_cache.buf;
            nAbsoluteOffset = (unsigned int)offset;
            break;
        }
        case SEEK_CUR:
        {
            int left_len = stream->read_cache.buf_pos - stream->read_cache.buf;
            int right_len = stream->read_cache.data_len - left_len;
            if (offset > 0) {
                if (offset < right_len) {
                    stream->read_cache.buf_pos += offset;
                    return 0;
                } else {
                    stream->read_cache.data_len = 0;
                    stream->read_cache.buf_pos = stream->read_cache.buf;
                    offset -= right_len;
                }
            } else {
                if (left_len + offset > 0) {
                    stream->read_cache.buf_pos += offset;
                    return 0;
                } else {
                    stream->read_cache.data_len = 0;
                    stream->read_cache.buf_pos = stream->read_cache.buf;
                    offset -= left_len;
                }
            }
            int64_t nCurPos = fat_tell((int)stream->file_handle);
            nAbsoluteOffset = (unsigned int)(nCurPos + offset);
            break;
        }
        case SEEK_END:
        {
            ALOGW("(f:%s, l:%d) call SEEK_END during writeFile?", __FUNCTION__, __LINE__);
            stream->read_cache.data_len = 0;
            stream->read_cache.buf_pos = stream->read_cache.buf;
            int64_t fileSize = fat_size((int)stream->file_handle);
            nAbsoluteOffset = (unsigned int)(fileSize + offset);
            break;
        }
        default:
        {
            ALOGE("(f:%s, l:%d) fatal error! unknown frowWhere[%d]", __FUNCTION__, __LINE__, whence);
            return -1;
        }
    }
    return fat_lseek((int)stream->file_handle, nAbsoluteOffset);
#elif (defined(__OS_ANDROID))
//    int fd = fileno(stream->file_handle);
//    int seek_ret;
//	cdx_off_t seekResult = lseek64(fd, offset, whence);
//    seek_ret = (seekResult == -1) ? -1 : 0;
//    return seek_ret;
    return fseeko(stream->file_handle, offset, whence);
#else
    return fseeko(stream->file_handle, offset, whence);
#endif
}

cdx_off_t cdx_tell_stream_file(struct cdx_stream_info *stream)
{
#if (CDXCFG_FILE_SYSTEM==OPTION_FILE_SYSTEM_DIRECT_FATFS)
    int right_len = stream->read_cache.data_len - (stream->read_cache.buf_pos - stream->read_cache.buf);
    cdx_off_t pos = fat_tell((int)stream->file_handle);
    pos -= right_len;
#elif (defined(__OS_ANDROID))
//    int fd = fileno(stream->file_handle);
//    cdx_off_t pos = lseek64(fd, 0, SEEK_CUR);
    cdx_off_t pos = ftello(stream->file_handle);
#else
    cdx_off_t pos = ftello(stream->file_handle);
#endif
return pos;
}

ssize_t cdx_read_stream_file(void *ptr, size_t size, size_t nmemb, struct cdx_stream_info *stream)
{
#if (CDXCFG_FILE_SYSTEM==OPTION_FILE_SYSTEM_DIRECT_FATFS)
    int nLeftSize = size*nmemb;
    char* pCurRead = ptr;
    int left = stream->read_cache.data_len - (stream->read_cache.buf_pos - stream->read_cache.buf);
    if (nLeftSize <= left) {
        memcpy(pCurRead, stream->read_cache.buf_pos, nLeftSize);
        stream->read_cache.buf_pos += nLeftSize;
        return nLeftSize;
    } else {
        if (left > 0) {
            memcpy(pCurRead, stream->read_cache.buf_pos, left);
            pCurRead += left;
            nLeftSize -= left;
        }
        stream->read_cache.buf_pos = stream->read_cache.buf;
        stream->read_cache.data_len = 0;
        int nCurReadLen;
        int nReadResult;
        while(nLeftSize > stream->read_cache.buf_len)
        {
            nReadResult = fat_read((int)stream->file_handle, pCurRead, stream->read_cache.buf_len);
            if (nReadResult  > 0) {
                pCurRead += nReadResult;
                nLeftSize -= nReadResult;
            }
            if(nReadResult != stream->read_cache.buf_len)
            {
                ALOGE("(f:%s, l:%d) fat_read error [%d]!=[%d](%s)", __FUNCTION__, __LINE__, nReadResult, stream->read_cache.buf_len, strerror(errno));
                return size*nmemb - nLeftSize;
            }
        }
        nReadResult = fat_read((int)stream->file_handle, stream->read_cache.buf, stream->read_cache.buf_len);
        if(nReadResult != stream->read_cache.buf_len)
        {
            ALOGE("(f:%s, l:%d) fat_read error [%d]!=[%d](%s)", __FUNCTION__, __LINE__, nReadResult, stream->read_cache.buf_len, strerror(errno));
            if (nReadResult <= 0) {
                return size*nmemb - nLeftSize;
            }
        }
        if (nLeftSize < nReadResult) {
            memcpy(pCurRead, stream->read_cache.buf_pos, nLeftSize);
            stream->read_cache.buf_pos += nLeftSize;
            nLeftSize = 0;
        } else {
            memcpy(pCurRead, stream->read_cache.buf_pos, nReadResult);
            stream->read_cache.buf_pos += nReadResult;
            nLeftSize -= nReadResult;
        }
        stream->read_cache.data_len = nReadResult;
        return size*nmemb - nLeftSize;
    }
#elif (defined(__OS_ANDROID))
//    int fd = fileno(stream->file_handle);
//    int bytesToBeRead = size * nmemb;
//    int result = read(fd, ptr, bytesToBeRead);
//    if (result > 1) {
//    	result /= size;
//    }
//    return result;
    return fread(ptr, size, nmemb,stream->file_handle);
#else
    return fread(ptr, size, nmemb,stream->file_handle);
#endif
}

ssize_t cdx_write_stream_file(const void *ptr, size_t size, size_t nmemb, struct cdx_stream_info *stream)
{
#if (CDXCFG_FILE_SYSTEM==OPTION_FILE_SYSTEM_DIRECT_FATFS)
    int nLeftSize = size*nmemb;
    int nCurWriteLen;
    int nWriteResult;
    char* pCurWrite = ptr;
    if (stream->read_cache.data_len > 0) {
        int64_t right_len = stream->read_cache.data_len - (stream->read_cache.buf_pos - stream->read_cache.buf);
        int64_t nCurPos = fat_tell((int)stream->file_handle);
        off_t offset = nCurPos - right_len;
        fat_lseek((int)stream->file_handle, offset);
        stream->read_cache.data_len = 0;
        stream->read_cache.buf_pos = stream->read_cache.buf;
    }
    while(nLeftSize > 0)
    {
        nCurWriteLen = nLeftSize;
        if(nCurWriteLen > FATFS_BLOCK_SIZE)
        {
            nCurWriteLen = FATFS_BLOCK_SIZE;
        }
        nWriteResult = fat_write((int)stream->file_handle, pCurWrite, nCurWriteLen);
        nLeftSize -= nWriteResult;
        pCurWrite += nWriteResult;
        if(nWriteResult!=nCurWriteLen)
        {
            ALOGE("(f:%s, l:%d) [%d]fwrite error [%d]!=[%d](%s)", __FUNCTION__, __LINE__, (int)stream->file_handle, nWriteResult, nCurWriteLen, strerror(errno));
            break;
        }
    }
    return size*nmemb - nLeftSize;
#elif (defined(__OS_ANDROID))
//    int fd = fileno(stream->file_handle);
//    int num_bytes_written = write(fd , ptr, (size * nmemb));
//    if (num_bytes_written != -1) {
//    	num_bytes_written /=size;
//    }
//    return num_bytes_written;
    //ALOGD("(f:%s, l:%d) ptr[%p], size[%d]nmemb[%d]file_handle[%p]!", __FUNCTION__, __LINE__, ptr, size, nmemb, stream->file_handle);
    return fwrite(ptr, size, nmemb,stream->file_handle);
    //ALOGD("(f:%s, l:%d) writeNum[%d]", __FUNCTION__, __LINE__, writeNum);
#else
    return fwrite(ptr, size, nmemb,stream->file_handle);
#endif
}

long long cdx_get_stream_size_file(struct cdx_stream_info *stream)
{
	long long size = -1;
	cdx_off_t curr_offset;

	if (stream->data_src_desc.stream_type == CEDARX_STREAM_NETWORK)
		return -1;

	if (stream->data_src_desc.source_type == CEDARX_SOURCE_FD) {
		if (stream->data_src_desc.ext_fd_desc.length >= 0) {
			size = stream->data_src_desc.ext_fd_desc.length;
			return size;
		}
	}
#if (CDXCFG_FILE_SYSTEM==OPTION_FILE_SYSTEM_DIRECT_FATFS)
    size = fat_size((int)stream->file_handle);
#elif (defined(__OS_ANDROID))
//    int fd = fileno(stream->file_handle);
//    curr_offset = lseek64(fd , 0, SEEK_CUR);
//	size = lseek64(fd, 0, SEEK_END);
//	lseek64(fd, curr_offset, SEEK_SET);
    curr_offset = ftello(stream->file_handle);
	fseeko(stream->file_handle, 0, SEEK_END);
	size = ftello(stream->file_handle);
	fseeko(stream->file_handle, curr_offset, SEEK_SET);
#else
    curr_offset = ftello(stream->file_handle);
	fseeko(stream->file_handle, 0, SEEK_END);
	size = ftello(stream->file_handle);
	fseeko(stream->file_handle, curr_offset, SEEK_SET);
#endif

	return size;
}


int cdx_truncate_stream_file(struct cdx_stream_info *stream, cdx_off_t length)
{
    int ret;
#if (CDXCFG_FILE_SYSTEM==OPTION_FILE_SYSTEM_DIRECT_FATFS)
    int seekRet;
    stream->read_cache.data_len = 0;
    stream->read_cache.buf_pos = stream->read_cache.buf;
    seekRet = fat_lseek((int)stream->file_handle, length);
    if(0==seekRet)
    {
        ret = fat_truncate((int)stream->file_handle);
        if(ret!=0)
        {
            ALOGE("(f:%s, l:%d) fatal error! fat_truncate fail", __FUNCTION__, __LINE__);
        }
    }
    else
    {
        ALOGE("(f:%s, l:%d) fatal error! fat_lseek length[%lld] fail, truncate fail", __FUNCTION__, __LINE__, length);
        ret = -1;
    }
#else
    int nFd = fileno(stream->file_handle);
    ret = ftruncate(nFd, length);
    if(ret!=0)
    {
        ALOGE("(f:%s, l:%d) fatal error! ftruncate fail", __FUNCTION__, __LINE__);
    }
#endif
    return ret;
}

int cdx_fallocate_stream_file(struct cdx_stream_info *stream, int mode, int64_t offset, int64_t len)
{
#if (CDXCFG_FILE_SYSTEM==OPTION_FILE_SYSTEM_DIRECT_FATFS)
    if(fat_allocate((int)stream->file_handle, mode, offset, len) < 0)
    {
        ALOGE("<F:%s, L:%d>Failed to fallocate size %lld, (%s)", __FUNCTION__, __LINE__, len, strerror(errno));
        return -1;
    }
    return 0;
#else
    int nFd = fileno(stream->file_handle);
    if(nFd >= 0)
    {
        CDX_S64 tm1, tm2;
        tm1 = CDX_GetSysTimeUsMonotonic();
        if (fallocate(nFd, mode, offset, len) < 0)
        {
            ALOGE("<F:%s, L:%d>Failed to fallocate size %lld, (%s)", __FUNCTION__, __LINE__, len, strerror(errno));
            return -1;
        }
        tm2 = CDX_GetSysTimeUsMonotonic();
        ALOGD("(f:%s, l:%d) stream[%p] fallocate size(%d)Byte, time(%lld)ms", __FUNCTION__, __LINE__, stream, len, (tm2-tm1)/1000);
        return 0;
    }
    else
    {
        ALOGE("(f:%s, l:%d) fatal error! wrong fd[%d]", __FUNCTION__, __LINE__, nFd);
        return -1;
    }
#endif
}

///////////////////////////////////////////////////////////////////////////////
int cdx_seek_fd_file(struct cdx_stream_info *stream, cdx_off_t offset, int whence)
{
    LOGV("stream %p, cur offset 0x%08llx, seek offset 0x%08llx, whence %d, length 0x%08llx",
			stream, stream->fd_desc.cur_offset, offset, whence, stream->fd_desc.length);
    cdx_off_t seek_result;
	if(whence == SEEK_SET) {
		//set from start, add origial offset.
		offset += stream->fd_desc.offset;
	} else if(whence == SEEK_CUR) {
		//seek from current position, seek to cur offset at first.
		seek_result = lseek64(stream->fd_desc.fd, stream->fd_desc.cur_offset, SEEK_SET);
        if(-1 == seek_result)
        {
            ALOGE("(f:%s, l:%d) fatal error! seek failed (%s)", __FUNCTION__, __LINE__, strerror(errno));
            return -1;
        }
	} else {
      #if (CDXCFG_FILE_SYSTEM==OPTION_FILE_SYSTEM_DIRECT_IO)
        if(stream->mIODirectionFlag)
        {
            //seek from backfront, no matter with current position and offset.
            if(stream->mFileEndOffset<stream->mFileSize)
            {
                ALOGE("(f:%s, l:%d) fatal error! called seek when endOffset[%lld]!=fileSize[%lld]", __FUNCTION__, __LINE__, stream->mFileEndOffset, stream->mFileSize);
                offset -= (stream->mFileSize - stream->mFileEndOffset);
            }
            else if (stream->mFileEndOffset == stream->mFileSize)
            {
            }
            else
            {
                ALOGE("(f:%s, l:%d) fatal error! [%lld]>[%lld]", __FUNCTION__, __LINE__, stream->mFileEndOffset, stream->mFileSize);
                assert(0);
            }
        }
      #endif
	}
#if (CDXCFG_FILE_SYSTEM==OPTION_FILE_SYSTEM_DIRECT_IO)
    if(stream->mIODirectionFlag)
    {
        cdx_off_t   absSeekPos;
        if(whence == SEEK_SET)
        {
            absSeekPos = offset;
        }
        else if(whence == SEEK_CUR)
        {
            absSeekPos = offset + stream->fd_desc.cur_offset;
        }
        else
        {
            absSeekPos = offset + stream->mFileSize;
        }
        if(absSeekPos > stream->mFileSize)
        {
            //we use write to extend file size, to avoid linux kernel use cache.
            cdx_off_t nExtendSize = absSeekPos - stream->mFileSize;
            stream->mFtruncateFlag = 1;
            CDX_S64 tm1, tm2;
            tm1 = CDX_GetNowUs();
            ssize_t wtSize = stream->write(NULL, 1, nExtendSize, stream);
            tm2 = CDX_GetNowUs();
            stream->mFtruncateFlag = 0;
            ALOGD("(f:%s, l:%d) directIO seek extend [%lld]bytes, use [%lld]ms", __FUNCTION__, __LINE__, nExtendSize, (tm2-tm1)/1000);
            if(wtSize == nExtendSize)
            {
                seek_result = lseek64(stream->fd_desc.fd, 0, SEEK_CUR);
            }
            else
            {
                ALOGE("(f:%s, l:%d) fatal error! directIO truncate fail, wtSize[%d]!=extendSize[%d]", __FUNCTION__, __LINE__, wtSize, nExtendSize);
                seek_result = -1;
            }
        }
        else
        {
            seek_result = lseek64(stream->fd_desc.fd, offset, whence);
        }
    }
    else
    {
        seek_result = lseek64(stream->fd_desc.fd, offset, whence);
    }
#else
    seek_result = lseek64(stream->fd_desc.fd, offset, whence);
#endif
	
	int seek_ret = 0;
	if(seek_result == -1) 
    {
		ALOGE("(f:%s, l:%d) fatal error! seek failed (%s)", __FUNCTION__, __LINE__, strerror(errno));
		seek_ret = -1;
	} 
    else 
    {
		stream->fd_desc.cur_offset = seek_result < stream->fd_desc.offset
				? stream->fd_desc.offset : seek_result;
        if(seek_result < stream->fd_desc.offset)
        {
            ALOGE("(f:%s, l:%d) fatal error! seek pos[%lld]<offset[%lld]", __FUNCTION__, __LINE__, seek_result, stream->fd_desc.offset);
        }
	}
	return seek_ret;
}

cdx_off_t cdx_tell_fd_file(struct cdx_stream_info *stream)
{
	return stream->fd_desc.cur_offset - stream->fd_desc.offset;
}

int cdx_read_fd_file(void *ptr, size_t size, size_t nmemb, struct cdx_stream_info *stream)
{
	//seek to where current actual pos is.
	int result = lseek64(stream->fd_desc.fd, stream->fd_desc.cur_offset, SEEK_SET);
	if(result == -1) {
        ALOGE("(f:%s, l:%d) fatal error! seek fail!", __FUNCTION__, __LINE__);
		return -1;
	}

	int bytesToBeRead = size * nmemb;
	LOGV("stream %p, cur offset 0x%08llx, require bytes %d, length 0x%08llx",
			stream, stream->fd_desc.cur_offset, bytesToBeRead, stream->fd_desc.length);

    result = read(stream->fd_desc.fd, ptr, bytesToBeRead);
    stream->fd_desc.cur_offset = lseek64(stream->fd_desc.fd, 0, SEEK_CUR);
    if(-1 == stream->fd_desc.cur_offset)
    {
        ALOGE("(f:%s, l:%d) fatal error! seek fail!", __FUNCTION__, __LINE__);
    }
    assert(stream->fd_desc.cur_offset >= stream->fd_desc.offset);

    if (result != -1) 
    {
    	result /= size;
    }
    else
    {
        ALOGE("(f:%s, l:%d) fatal error! read [%d]bytes[%d] fail!", __FUNCTION__, __LINE__, size*nmemb, result);
    }
	LOGV("stream %p, cur offset 0x%08llx, read bytes %d, length 0x%08llx",
			stream, stream->fd_desc.cur_offset, result, stream->fd_desc.length);
    return result;
}

#if (CDXCFG_FILE_SYSTEM==OPTION_FILE_SYSTEM_DIRECT_IO)
int cdx_write_fd_file(const void *ptr, size_t size, size_t nmemb, struct cdx_stream_info *stream)
{
    off64_t result = lseek64(stream->fd_desc.fd, stream->fd_desc.cur_offset, SEEK_SET);
	if(result == -1) {
        ALOGE("(f:%s, l:%d) fatal error! seek fail!", __FUNCTION__, __LINE__);
		return -1;
	}
    if(NULL == stream->mpAlignBuf)
    {
        stream->mAlignBufSize = DIRECTIO_UNIT_SIZE;
        int ret = posix_memalign((void **)&stream->mpAlignBuf, 4096, stream->mAlignBufSize);
        if(ret!=0)
        {
            ALOGE("(f:%s, l:%d) fatal error! malloc fail!", __FUNCTION__, __LINE__);
            return -1;
        }
    }
    char *pCurPtr = (char*)ptr;
    off64_t nByteSize = size*nmemb;
    off64_t fileOffset = result;
    off64_t alignOffset;
    if(stream->mFtruncateFlag)
    {
        memset(stream->mpAlignBuf, 0xFF, stream->mAlignBufSize);
        fileOffset = lseek64(stream->fd_desc.fd, 0, SEEK_END);
        ALOGD("(f:%s, l:%d) use write() to ftruncate file! curOffset[%lld], endOffset[%lld], curSize[%lld], extend[%lld]bytes", 
            __FUNCTION__, __LINE__, fileOffset, stream->mFileEndOffset, stream->mFileSize, nByteSize);
        stream->mFileEndOffset = stream->mFileSize;

        off64_t nAlignByteSize = (nByteSize/DIRECTIO_UNIT_SIZE)*DIRECTIO_UNIT_SIZE;
        nByteSize -= nAlignByteSize;
        if(nAlignByteSize > 0)
        {
            char *pTmpBuf = NULL;
            if(nAlignByteSize <= stream->mAlignBufSize)
            {
                pTmpBuf = stream->mpAlignBuf;
            }
            else
            {
                int ret = posix_memalign((void **)&pTmpBuf, 4096, nAlignByteSize);
                if(ret!=0)
                {
                    ALOGE("(f:%s, l:%d) fatal error! malloc fail!", __FUNCTION__, __LINE__);
                    return -1;
                }
            }
            
            memset(pTmpBuf, 0xFF, nAlignByteSize);
            off64_t wrtSize = write(stream->fd_desc.fd, pTmpBuf, nAlignByteSize);
            if(wrtSize!=nAlignByteSize)
            {
                ALOGE("(f:%s, l:%d) fatal error! [%lld]!=[%lld]", __FUNCTION__, __LINE__, wrtSize, nAlignByteSize);
            }
            fileOffset = stream->mFileEndOffset = stream->mFileSize = lseek64(stream->fd_desc.fd, 0, SEEK_CUR);
            if(nAlignByteSize <= stream->mAlignBufSize)
            {
            }
            else
            {
                free(pTmpBuf);
                pTmpBuf = NULL;
            }
        }
        if(0 == nByteSize)
        {
            return nmemb;
        }
        else
        {
            ALOGW("(f:%s, l:%d) why it happened? write() instead of ftruncate(), left [%lld]bytes to write again!", __FUNCTION__, __LINE__, nByteSize);
        }
    }
    //process fileOffset align of first block.
    if(fileOffset%DIRECTIO_UNIT_SIZE!=0)
    {
        ALOGD("(f:%s, l:%d) DirectIO! process file offset not align![%lld]", __FUNCTION__, __LINE__, fileOffset);
        alignOffset = (fileOffset/DIRECTIO_UNIT_SIZE)*DIRECTIO_UNIT_SIZE;
        off64_t seekRet = lseek64(stream->fd_desc.fd, alignOffset, SEEK_SET);
        if(-1 == seekRet)
        {
            ALOGE("(f:%s, l:%d) fatal error! seek fail!", __FUNCTION__, __LINE__);
            return -1;
        }
        int blockLeftSize = DIRECTIO_UNIT_SIZE - (fileOffset - alignOffset);
        result = read(stream->fd_desc.fd, stream->mpAlignBuf, DIRECTIO_UNIT_SIZE);
        if (-1 == result) 
        {
        	ALOGE("(f:%s, l:%d) fatal error! read [%d]bytes[%lld] fail!", __FUNCTION__, __LINE__, DIRECTIO_UNIT_SIZE, result);
            return -1;
        }
        if(result != DIRECTIO_UNIT_SIZE)
        {
            ALOGW("(f:%s, l:%d) fatal error! read [%d]bytes[%lld], maybe read file end!", __FUNCTION__, __LINE__, DIRECTIO_UNIT_SIZE, result);
            //int trucRet = ftruncate(stream->fd_desc.fd, alignOffset + DIRECTIO_UNIT_SIZE);
            off64_t seekPos = lseek64(stream->fd_desc.fd, alignOffset, SEEK_SET);
            if(-1 == seekPos)
            {
                ALOGE("(f:%s, l:%d) fatal error! seek fail!", __FUNCTION__, __LINE__);
                return -1;
            }
            memset(stream->mpAlignBuf+result, 0xFF, DIRECTIO_UNIT_SIZE-result);
            off64_t wrtSize = write(stream->fd_desc.fd, stream->mpAlignBuf, DIRECTIO_UNIT_SIZE);
            ALOGW("(f:%s, l:%d) fatal error! truncate size to [%lld]bytes!", __FUNCTION__, __LINE__, alignOffset + DIRECTIO_UNIT_SIZE);
            if(-1 == wrtSize)
            {
                ALOGE("(f:%s, l:%d) fatal error! truncate fail", __FUNCTION__, __LINE__);
                return -1;
            }
            stream->mFileEndOffset = alignOffset + result;
            stream->mFileSize = alignOffset + DIRECTIO_UNIT_SIZE;
        }
        result = lseek64(stream->fd_desc.fd, alignOffset, SEEK_SET);
        if(-1 == result)
        {
            ALOGE("(f:%s, l:%d) fatal error! seek fail!", __FUNCTION__, __LINE__);
            return -1;
        }
        if(blockLeftSize >= nByteSize)
        {
            ALOGD("(f:%s, l:%d) can write done once! fileOffset[%lld][%lld], writeSize[%lld]", __FUNCTION__, __LINE__, alignOffset, fileOffset, nByteSize);
            if(0 == stream->mFtruncateFlag)
            {
                memcpy(stream->mpAlignBuf + (fileOffset - alignOffset), pCurPtr, nByteSize);
            }
            result = write(stream->fd_desc.fd, stream->mpAlignBuf, DIRECTIO_UNIT_SIZE);
            if (result != DIRECTIO_UNIT_SIZE) 
            {
                ALOGE("(f:%s, l:%d) fatal error! write[%d]bytes[%lld] fail[%s]!", __FUNCTION__, __LINE__, DIRECTIO_UNIT_SIZE, result, strerror(errno));
                return -1;
            }
            pCurPtr += nByteSize;
            result = lseek64(stream->fd_desc.fd, fileOffset+nByteSize, SEEK_SET);
            if(-1 == result)
            {
                ALOGE("(f:%s, l:%d) fatal error! seek fail!", __FUNCTION__, __LINE__);
                return -1;
            }
            stream->fd_desc.cur_offset = result;
            return nmemb;
        }
        else
        {
            ALOGD("(f:%s, l:%d) fileOffset[%lld][%lld], write[%d], need writeSize[%lld]", __FUNCTION__, __LINE__, alignOffset, fileOffset, blockLeftSize, nByteSize);
            if(0 == stream->mFtruncateFlag)
            {
                memcpy(stream->mpAlignBuf + (fileOffset - alignOffset), pCurPtr, blockLeftSize);
            }
            result = write(stream->fd_desc.fd, stream->mpAlignBuf, DIRECTIO_UNIT_SIZE);
            if (result != DIRECTIO_UNIT_SIZE) 
            {
                ALOGE("(f:%s, l:%d) fatal error! write[%d]bytes[%lld] fail[%s]!", __FUNCTION__, __LINE__, blockLeftSize, result, strerror(errno));
                return -1;
            }
            pCurPtr += blockLeftSize;
            nByteSize -= blockLeftSize;
        }
    }
    //write more whole blockSize
    off64_t alignSize = (nByteSize/DIRECTIO_UNIT_SIZE)*DIRECTIO_UNIT_SIZE;
    nByteSize -= alignSize;
    if(stream->mFtruncateFlag || (off64_t)pCurPtr%DIRECTIO_USER_MEMORY_ALIGN!=0)
    {
		if(pCurPtr!=NULL)
		{
        	ALOGW("(f:%s, l:%d) directIO ptr[%p] not align [%d]bytes, size[%lld]bytes copy", __FUNCTION__, __LINE__, pCurPtr, DIRECTIO_USER_MEMORY_ALIGN, alignSize);
		}
        while(alignSize > 0)
        {
            if(alignSize >= stream->mAlignBufSize)
            {
                if(0 == stream->mFtruncateFlag)
                {
                    memcpy(stream->mpAlignBuf, pCurPtr, stream->mAlignBufSize);
                }
                result = write(stream->fd_desc.fd, stream->mpAlignBuf, stream->mAlignBufSize);
                if(result != stream->mAlignBufSize)
                {
                    ALOGE("(f:%s, l:%d) fatal error! write[%d]bytes[%lld] fail[%s]!", __FUNCTION__, __LINE__, stream->mAlignBufSize, result, strerror(errno));
                    return -1;
                }
                pCurPtr += stream->mAlignBufSize;
                alignSize -= stream->mAlignBufSize;
            }
            else
            {
                if(alignSize%DIRECTIO_UNIT_SIZE!=0)
                {
                    ALOGE("(f:%s, l:%d) fatal error! not align[%lld][%d]!", __FUNCTION__, __LINE__, alignSize, DIRECTIO_UNIT_SIZE);
                    assert(0);
                }
                if(0 == stream->mFtruncateFlag)
                {
                    memcpy(stream->mpAlignBuf, pCurPtr, alignSize);
                }
                result = write(stream->fd_desc.fd, stream->mpAlignBuf, alignSize);
                if(result != alignSize)
                {
                    ALOGE("(f:%s, l:%d) fatal error! write[%lld]bytes[%lld] fail[%s]!", __FUNCTION__, __LINE__, alignSize, result, strerror(errno));
                    return -1;
                }
                pCurPtr += alignSize;
                alignSize = 0;
            }
        }
    }
    else
    {
        //ALOGD("(f:%s, l:%d) directIO ptr[%p] and size[%lld] all align [%d]bytes, directly write", __FUNCTION__, __LINE__, pCurPtr, alignSize, DIRECTIO_UNIT_SIZE);
        if(alignSize > 0)
        {
            result = write(stream->fd_desc.fd, pCurPtr, alignSize);
            if(result != alignSize)
            {
                ALOGE("(f:%s, l:%d) fatal error! write[%lld]bytes[%lld] fail[%s]!", __FUNCTION__, __LINE__, alignSize, result, strerror(errno));
                return -1;
            }
            pCurPtr += alignSize;
            alignSize = 0;
        }
    }
    //write last bytes, must < DIRECTIO_UNIT_SIZE
    if(nByteSize)
    {
        ALOGD("(f:%s, l:%d) DirectIO! left not align bytes[%lld] to process!", __FUNCTION__, __LINE__, nByteSize);
        off64_t curOffset = lseek64(stream->fd_desc.fd, 0, SEEK_CUR);   //must align.
        if(-1 == curOffset)
        {
            ALOGE("(f:%s, l:%d) fatal error! seek fail!", __FUNCTION__, __LINE__);
            return -1;
        }
        off64_t desOffset = curOffset + nByteSize;
        if(curOffset == stream->mFileEndOffset)
        {
            ALOGD("(f:%s, l:%d) DirectIO! not need read because curOffset[%lld]==fileEndOffset!", __FUNCTION__, __LINE__, curOffset);
            result = 0;
        }
        else
        {
            result = read(stream->fd_desc.fd, stream->mpAlignBuf, DIRECTIO_UNIT_SIZE);
            if (-1 == result) 
            {
            	ALOGE("(f:%s, l:%d) fatal error! read [%d]bytes[%lld] fail!", __FUNCTION__, __LINE__, DIRECTIO_UNIT_SIZE, result);
                return -1;
            }
        }
        if(result != DIRECTIO_UNIT_SIZE)
        {
            ALOGW("(f:%s, l:%d) read [%d]bytes[%lld], maybe read file end! extend file size to [%lld]bytes", 
                __FUNCTION__, __LINE__, DIRECTIO_UNIT_SIZE, result, curOffset + DIRECTIO_UNIT_SIZE);
            //I don't know whether it can run normally when call ftruncate() here, so I use write() to extend file size.
            memset(stream->mpAlignBuf+result, 0xFF, DIRECTIO_UNIT_SIZE-result);
            //need update mEndOffset and mFileSize.
            stream->mFileEndOffset = desOffset;
            stream->mFileSize = curOffset + DIRECTIO_UNIT_SIZE;
        }
        else
        {
            //if can read full size, it means file not extend. so not need update mEndOffset and mFileSize.
            if(curOffset + DIRECTIO_UNIT_SIZE == stream->mFileSize)
            {
                //if meet file physical end, process mEndOffset carefully.
                if(stream->mFileEndOffset < desOffset)
                {
                    ALOGW("(f:%s, l:%d) Be careful! update FileEndOffset[%lld] to [%lld]!", __FUNCTION__, __LINE__, stream->mFileEndOffset, desOffset);
                    stream->mFileEndOffset = desOffset;
                }
            }
            else if(curOffset + DIRECTIO_UNIT_SIZE > stream->mFileSize)
            {
                ALOGE("(f:%s, l:%d) fatal error! [%lld][%d][%lld]!", __FUNCTION__, __LINE__, curOffset, DIRECTIO_UNIT_SIZE, stream->mFileSize);
                assert(0);
            }
        }
        result = lseek64(stream->fd_desc.fd, curOffset, SEEK_SET);
        if(-1 == result)
        {
            ALOGE("(f:%s, l:%d) fatal error! seek fail!", __FUNCTION__, __LINE__);
            return -1;
        }
        if(0 == stream->mFtruncateFlag)
        {
            memcpy(stream->mpAlignBuf, pCurPtr, nByteSize);
        }
        result = write(stream->fd_desc.fd, stream->mpAlignBuf, DIRECTIO_UNIT_SIZE);
        if(result != DIRECTIO_UNIT_SIZE)
        {
            ALOGE("(f:%s, l:%d) fatal error! write[%d]bytes[%lld] fail[%s]!", __FUNCTION__, __LINE__, DIRECTIO_UNIT_SIZE, result, strerror(errno));
            return -1;
        }
        pCurPtr += nByteSize;
        nByteSize = 0;
        result = lseek64(stream->fd_desc.fd, desOffset, SEEK_SET);
        if(-1 == result)
        {
            ALOGE("(f:%s, l:%d) fatal error! seek fail!", __FUNCTION__, __LINE__);
            return -1;
        }
        stream->fd_desc.cur_offset = result;
    }
    else
    {
        //update mEndOffset and mFileSize.
        fileOffset = lseek64(stream->fd_desc.fd, 0, SEEK_CUR);
        if(-1 == fileOffset)
        {
            ALOGE("(f:%s, l:%d) fatal error! seek fail!", __FUNCTION__, __LINE__);
            return -1;
        }
        stream->fd_desc.cur_offset = fileOffset;
        if(stream->mFileSize < fileOffset)
        {
            stream->mFileSize = fileOffset;
        }
        if(stream->mFileEndOffset < fileOffset)
        {
            stream->mFileEndOffset = fileOffset;
        }
    }
    return nmemb;
}

#else
int cdx_write_fd_file(const void *ptr, size_t size, size_t nmemb, struct cdx_stream_info *stream)
{
	int result = lseek64(stream->fd_desc.fd, stream->fd_desc.cur_offset, SEEK_SET);
	if(result == -1) {
        ALOGE("(f:%s, l:%d) fatal error! seek fail!", __FUNCTION__, __LINE__);
		return -1;
	}

    int num_bytes_written = write(stream->fd_desc.fd , ptr, (size * nmemb));
	cdx_off_t seekResult = lseek64(stream->fd_desc.fd, 0, SEEK_CUR);
    if(-1 == seekResult)
    {
        ALOGE("(f:%s, l:%d) fatal error! seek error!", __FUNCTION__, __LINE__);
        return -1;
    }
	stream->fd_desc.cur_offset = seekResult < stream->fd_desc.offset
			? stream->fd_desc.offset: seekResult;
    if(seekResult < stream->fd_desc.offset)
    {
        ALOGE("(f:%s, l:%d) fatal error! write pos[%lld]<offset[%lld]", __FUNCTION__, __LINE__, seekResult, stream->fd_desc.offset);
    }
    if (num_bytes_written != -1) 
    {
    	num_bytes_written /= size;
    }
    else
    {
        ALOGE("(f:%s, l:%d) fatal error! write[%d]bytes[%d] fail[%s]!", __FUNCTION__, __LINE__, (size*nmemb), num_bytes_written, strerror(errno));
    }

    return num_bytes_written;
}

#endif

long long cdx_get_fd_size_file(struct cdx_stream_info *stream)
{
    if(stream->mFileEndOffset != stream->mFileSize)
    {
        ALOGE("(f:%s, l:%d) fatal error! [%lld]!=[%lld]", __FUNCTION__, __LINE__, stream->mFileEndOffset, stream->mFileSize);
    }
	//return stream->fd_desc.length;
	cdx_off_t fdSize = lseek64(stream->fd_desc.fd, 0, SEEK_END);
    if(-1 == fdSize)
    {
        ALOGE("(f:%s, l:%d) fatal error! seek fail!", __FUNCTION__, __LINE__);
	}
    cdx_off_t size = fdSize - stream->fd_desc.offset;
	cdx_off_t seekResult = lseek64(stream->fd_desc.fd, stream->fd_desc.cur_offset, SEEK_SET);
    if(-1 == seekResult)
    {
        ALOGE("(f:%s, l:%d) fatal error! seek fail!", __FUNCTION__, __LINE__);
	}
    if(size < 0)
    {
        ALOGE("(f:%s, l:%d) fatal error! size[%lld]<0, fdSize[%lld], offset[%lld]", __FUNCTION__, __LINE__, size, fdSize, stream->fd_desc.offset);
    }
    return size;
}

int cdx_truncate_fd_file(struct cdx_stream_info *stream, cdx_off_t length)
{
    int ret = ftruncate(stream->fd_desc.fd, length);
    if(ret!=0)
    {
        ALOGE("(f:%s, l:%d) fatal error! ftruncate fail[%d]", __FUNCTION__, __LINE__, ret);
    }
    stream->mFileSize = length;
    stream->mFileEndOffset = length;
    return ret;
}

int cdx_fallocate_fd_file(struct cdx_stream_info *stream, int mode, int64_t offset, int64_t len)
{
//#if (CDXCFG_FILE_SYSTEM==OPTION_FILE_SYSTEM_DIRECT_IO)
//    ALOGW("(f:%s, l:%d) Don't fallocate when use directIO, mode[%d], offset[%lld], len[%lld]", __FUNCTION__, __LINE__, mode, offset, len);
//    return 0;
//#else
    if(stream->fd_desc.fd >= 0)
    {
        CDX_S64 tm1, tm2;
        tm1 = CDX_GetSysTimeUsMonotonic();
        if (fallocate(stream->fd_desc.fd, mode, offset, len) < 0)
        {
            ALOGE("(f:%s, l:%d) fatal error! Failed to fallocate size %lld, (%s)", __FUNCTION__, __LINE__, len, strerror(errno));
            return -1;
        }
        tm2 = CDX_GetSysTimeUsMonotonic();
        ALOGD("(f:%s, l:%d) stream[%p] fallocate size(%d)Byte, time(%lld)ms", __FUNCTION__, __LINE__, stream, len, (tm2-tm1)/1000);
        return 0;
    }
    else
    {
        ALOGE("(f:%s, l:%d) fatal error! wrong fd[%d]", __FUNCTION__, __LINE__, stream->fd_desc.fd);
        return -1;
    }
//#endif
}

void file_destory_instream_handle(struct cdx_stream_info * stm_info)
{
    if(!stm_info)
    {
		return;
	}
    if(stm_info->data_src_desc.source_type == CEDARX_SOURCE_FILEPATH) 
    {
#if (CDXCFG_FILE_SYSTEM==OPTION_FILE_SYSTEM_DIRECT_FATFS)
        if((int)stm_info->file_handle >= 0)
        {
            fat_close((int)stm_info->file_handle);
            stm_info->file_handle = -1;
        }
        if (stm_info->read_cache.buf != NULL) 
        {
            free(stm_info->read_cache.buf);
            stm_info->read_cache.buf = NULL;
        }
#else
        if(stm_info->file_handle != NULL)
        {
            fclose(stm_info->file_handle);
            stm_info->file_handle = NULL;
        }
#endif
    } 
    if(stm_info->fd_desc.fd >= 0)
    {
        if(0 == stm_info->fd_desc.fd)
        {
            ALOGD("(f:%s, l:%d) Be careful! fd==0!", __FUNCTION__, __LINE__);
        }
        //Local videos.
        close(stm_info->fd_desc.fd);
        stm_info->fd_desc.fd = -1;
    }
	
    if(stm_info->mpFilePath)
    {
        free(stm_info->mpFilePath);
        stm_info->mpFilePath = NULL;
    }
    if(stm_info->mpAlignBuf)
    {
        free(stm_info->mpAlignBuf);
        stm_info->mpAlignBuf = NULL;
    }
    stm_info->mAlignBufSize = 0;
}

int file_create_instream_handle(CedarXDataSourceDesc *datasource_desc, struct cdx_stream_info *stm_info)
{
    int ret = 0;
    stm_info->mIODirectionFlag = 0;
    stm_info->fd_desc.fd = -1;
    if(datasource_desc->source_type == CEDARX_SOURCE_FILEPATH) 
    {
        LOGV(" source url = %s", datasource_desc->source_url);
//              stm_info->fd_desc.fd  = open(datasource_desc->source_url,
//                      O_RDONLY | O_LARGEFILE, S_IRUSR | S_IWUSR | S_IRGRP
//                      | S_IWGRP | S_IROTH | S_IWOTH);
#if (CDXCFG_FILE_SYSTEM==OPTION_FILE_SYSTEM_DIRECT_FATFS)
        stm_info->read_cache.buf = (unsigned char*)malloc(READ_CACHE_LEN);
        assert(stm_info->read_cache.buf != NULL);
        memset(stm_info->read_cache.buf, 0, READ_CACHE_LEN);
        stm_info->read_cache.buf_len = READ_CACHE_LEN;
        stm_info->read_cache.buf_pos = stm_info->read_cache.buf;
        stm_info->read_cache.data_len = 0;
        stm_info->file_handle = fat_open(datasource_desc->source_url, FA_READ | FA_OPEN_EXISTING);
        if ((int)stm_info->file_handle < 0) 
        {
            ALOGE("<F:%s, L:%d> fat_open error(%d)", __FUNCTION__, __LINE__, (int)stm_info->file_handle);
            ret = -1;
        }
#else
        stm_info->file_handle = fopen(datasource_desc->source_url,"rb");
        if (stm_info->file_handle == NULL) 
        {
            ALOGE("<F:%s, L:%d> fat_open error(%s)", __FUNCTION__, __LINE__, strerror(errno));
            ret = -1;
        }
#endif
        if(-1 == ret)
        {
            return ret;
        }
        stm_info->seek  = cdx_seek_stream_file ;
        stm_info->tell  = cdx_tell_stream_file ;
        stm_info->read  = cdx_read_stream_file ;
        stm_info->write = cdx_write_stream_file;
        stm_info->getsize = cdx_get_stream_size_file;
        stm_info->destory = file_destory_instream_handle;
        LOGV("open url file:%s",datasource_desc->source_url);
        return ret;
    }       
    else if(datasource_desc->source_type == CEDARX_SOURCE_FD)
    {
        //Must dup a new file descirptor here and close it when destroying stream handle.
        stm_info->fd_desc.fd         = dup(datasource_desc->ext_fd_desc.fd);
        stm_info->fd_desc.offset     = datasource_desc->ext_fd_desc.offset;
        stm_info->fd_desc.cur_offset = datasource_desc->ext_fd_desc.offset;
        stm_info->fd_desc.length     = datasource_desc->ext_fd_desc.length;
        LOGV("stm_info %p, open fd file:%d, offset %lld, length %lld", stm_info, stm_info->fd_desc.fd,
                stm_info->fd_desc.offset, stm_info->fd_desc.length);
        stm_info->seek      = cdx_seek_fd_file;
        stm_info->tell      = cdx_tell_fd_file;
        stm_info->read      = cdx_read_fd_file;
        stm_info->write     = cdx_write_fd_file;
        stm_info->getsize   = cdx_get_fd_size_file;
        stm_info->destory = file_destory_instream_handle;
        stm_info->seek(stm_info, 0, SEEK_SET);
        return ret;
    }
    else
    {
        ALOGE("(f:%s, l:%d) fatal error! source_type[%d] stream_type[%d] wrong!", __FUNCTION__, __LINE__, datasource_desc->source_type, datasource_desc->stream_type);
        ret = -1;
        return ret;
    }
}

static int destory_outstream_handle_file(struct cdx_stream_info *stm_info)
{
#if (CDXCFG_FILE_SYSTEM==OPTION_FILE_SYSTEM_DIRECT_FATFS)
	if((int)stm_info->file_handle >= 0)
    {
        fat_close((int)stm_info->file_handle);
        stm_info->file_handle = -1;
	}
#else
  #if (CDXCFG_FILE_SYSTEM==OPTION_FILE_SYSTEM_DIRECT_IO)
    if(stm_info->mFileEndOffset < stm_info->mFileSize)
    {
        ALOGD("(f:%s, l:%d) fd[%d], [%lld]<[%lld], DirectIO use ftruncate() before close.", 
            __FUNCTION__, __LINE__, stm_info->fd_desc.fd, stm_info->mFileEndOffset, stm_info->mFileSize);
        int ret = ftruncate(stm_info->fd_desc.fd, stm_info->mFileEndOffset);
        if(ret!=0)
        {
            ALOGE("(f:%s, l:%d) fatal error! ftruncate fail[%d]", __FUNCTION__, __LINE__, ret);
        }
        stm_info->mFileSize = stm_info->mFileEndOffset;
    }
  #endif

	if(stm_info->file_handle != NULL)
    {
    #if (defined(__OS_ANDROID))
//            int fd = fileno(stm_info->file_handle);
//            if(fd) {
//        		close(fd);
//        		fd = -1;
//        	}
            fclose(stm_info->file_handle);
            stm_info->file_handle = NULL;
    #else
            fclose(stm_info->file_handle);
            stm_info->file_handle = NULL;
    #endif
	}
#endif
    if(stm_info->fd_desc.fd >= 0)
    {
        if(0 == stm_info->fd_desc.fd)
        {
            ALOGD("(f:%s, l:%d) Be careful! close fd==0", __FUNCTION__, __LINE__);
        }
        close(stm_info->fd_desc.fd);
        stm_info->fd_desc.fd = -1;
    }

    if(stm_info->mpFilePath)
    {
        free(stm_info->mpFilePath);
        stm_info->mpFilePath = NULL;
    }
    if(stm_info->mpAlignBuf)
    {
        free(stm_info->mpAlignBuf);
        stm_info->mpAlignBuf = NULL;
    }
    stm_info->mAlignBufSize = 0;
	return 0;
}

int create_outstream_handle_file(struct cdx_stream_info *stm_info, CedarXDataSourceDesc *datasource_desc)
{
    stm_info->mIODirectionFlag = 1;
//#ifdef __OS_ANDROID
#if 0
	if (datasource_desc->source_type == CEDARX_SOURCE_FD) {
		stm_info->fd_desc.fd = datasource_desc->ext_fd_desc.fd;
	} else {
		stm_info->fd_desc.fd = open(datasource_desc->source_url, O_CREAT | O_TRUNC | O_RDWR, 0644);
	}

	if (stm_info->fd_desc.fd == -1){
		LOGE("open file error!");
		return -1;
	}
#else
    stm_info->fd_desc.fd = -1;
    if(CEDARX_SOURCE_FD == datasource_desc->source_type)
    {
        //ALOGD("(f:%s, l:%d) fd[%d], offset[%lld], length[%lld]", __FUNCTION__, __LINE__, datasource_desc->ext_fd_desc.fd, datasource_desc->ext_fd_desc.offset, datasource_desc->ext_fd_desc.length);
        #if (CDXCFG_FILE_SYSTEM==OPTION_FILE_SYSTEM_DIRECT_IO)
            //get filepath, reopen again!
            stm_info->mpFilePath = generateFilepathFromFd(datasource_desc->ext_fd_desc.fd);
            stm_info->fd_desc.fd = open(stm_info->mpFilePath, O_CREAT | O_TRUNC | O_RDWR | O_DIRECT, 0666);
            stm_info->fd_desc.offset = datasource_desc->ext_fd_desc.offset;
            stm_info->fd_desc.cur_offset = datasource_desc->ext_fd_desc.offset;
            stm_info->fd_desc.length = datasource_desc->ext_fd_desc.length;
            stm_info->mFileEndOffset = lseek64(stm_info->fd_desc.fd, 0, SEEK_END);
            stm_info->mFileSize = stm_info->mFileEndOffset;
            if(stm_info->mFileEndOffset!=0)
            {
                ALOGE("(f:%s, l:%d) fatal error! mEndOffset[%lld], mFileSize[%lld] should be 0!", __FUNCTION__, __LINE__, stm_info->mFileEndOffset, stm_info->mFileSize);
            }
            if(stm_info->fd_desc.cur_offset!=0)
            {
                ALOGE("(f:%s, l:%d) fatal error! cur_offset[%lld]!=0", __FUNCTION__, __LINE__, stm_info->fd_desc.cur_offset);
            }
            stm_info->seek  = cdx_seek_fd_file ;
        	stm_info->tell  = cdx_tell_fd_file ;
        	stm_info->read  = cdx_read_fd_file ;
        	stm_info->write = cdx_write_fd_file;
            stm_info->truncate = cdx_truncate_fd_file;
            stm_info->fallocate = cdx_fallocate_fd_file;
        	stm_info->getsize = cdx_get_fd_size_file;
            stm_info->destory = destory_outstream_handle_file;
            stm_info->seek(stm_info, 0, SEEK_SET);
        	return 0;
        #else
          #if 1
            int nFd;
            if(datasource_desc->ext_fd_desc.fd >= 0)
            {
                nFd = dup(datasource_desc->ext_fd_desc.fd);
            }
            else
            {
                nFd = -1;
            }
            if (nFd < 0)
            {
                ALOGE("(f:%s, l:%d) fatal error! open file error!", __FUNCTION__, __LINE__);
                return -1;
            }
            stm_info->file_handle = fdopen(nFd, "w+b");
            if(stm_info->file_handle==NULL) 
            {
                ALOGE("(f:%s, l:%d) fatal error! get file fd failed", __FUNCTION__, __LINE__);
                close(nFd);
                nFd = -1;
                return -1;
            }
            nFd = -1;
          #else
            stm_info->fd_desc.fd = dup(datasource_desc->ext_fd_desc.fd);
            stm_info->mpFilePath = generateFilepathFromFd(stm_info->fd_desc.fd);
            stm_info->fd_desc.offset = datasource_desc->ext_fd_desc.offset;
            stm_info->fd_desc.cur_offset = datasource_desc->ext_fd_desc.offset;
            stm_info->fd_desc.length = datasource_desc->ext_fd_desc.length;
            stm_info->mFileEndOffset = lseek64(stm_info->fd_desc.fd, 0, SEEK_END);
            stm_info->mFileSize = stm_info->mFileEndOffset;
            if(stm_info->mFileEndOffset!=0)
            {
                ALOGE("(f:%s, l:%d) fatal error! mEndOffset[%lld], mFileSize[%lld] should be 0!", __FUNCTION__, __LINE__, stm_info->mFileEndOffset, stm_info->mFileSize);
            }
            if(stm_info->fd_desc.cur_offset!=0)
            {
                ALOGE("(f:%s, l:%d) fatal error! cur_offset[%lld]!=0", __FUNCTION__, __LINE__, stm_info->fd_desc.cur_offset);
            }
            stm_info->seek  = cdx_seek_fd_file ;
        	stm_info->tell  = cdx_tell_fd_file ;
        	stm_info->read  = cdx_read_fd_file ;
        	stm_info->write = cdx_write_fd_file;
            stm_info->truncate = cdx_truncate_fd_file;
            stm_info->fallocate = cdx_fallocate_fd_file;
        	stm_info->getsize = cdx_get_fd_size_file;
        	stm_info->destory = destory_outstream_handle_file;
            stm_info->seek(stm_info, 0, SEEK_SET);
            return 0;
          #endif
        #endif
    }
    else if(CEDARX_SOURCE_FILEPATH == datasource_desc->source_type)
    {
        #if (CDXCFG_FILE_SYSTEM==OPTION_FILE_SYSTEM_DIRECT_FATFS)
            stm_info->file_handle = fat_open(datasource_desc->source_url, FA_WRITE | FA_OPEN_ALWAYS);
            if((int)stm_info->file_handle < 0) 
            {
                ALOGE("(f:%s, l:%d) fatal error! fat_open[%s] fail", __FUNCTION__, __LINE__, datasource_desc->source_url);
                return -1;
            }
        #else
            stm_info->file_handle = fopen(datasource_desc->source_url, "w+b");
            if(stm_info->file_handle == NULL)
            {
                ALOGE("<F:%s, L:%d> Failed to open file %s(%s)", __FUNCTION__, __LINE__, datasource_desc->source_url, strerror(errno));
            	return -1;
            }
        #endif
    }
    else
    {
        ALOGE("(f:%s, l:%d) fatal error! wrong source_type[%d]", __FUNCTION__, __LINE__, datasource_desc->source_type);
        return -1;
    }
#endif

	stm_info->seek  = cdx_seek_stream_file ;
	stm_info->tell  = cdx_tell_stream_file ;
	stm_info->read  = cdx_read_stream_file ;
	stm_info->write = cdx_write_stream_file;
    stm_info->truncate = cdx_truncate_stream_file;
    stm_info->fallocate = cdx_fallocate_stream_file;
	stm_info->getsize = cdx_get_stream_size_file;
	stm_info->destory = destory_outstream_handle_file;

	return 0;
}

int stream_remove_file(char* fileName)
{
#if (CDXCFG_FILE_SYSTEM==OPTION_FILE_SYSTEM_DIRECT_FATFS)
    return fat_unlink(fileName);
#else
    return remove(fileName);
#endif
}

int stream_mkdir(char *dirName, int mode)
{
    int ret;
#if (CDXCFG_FILE_SYSTEM==OPTION_FILE_SYSTEM_DIRECT_FATFS)
    ret = 0;
    int dirId = fat_opendir(dirName);
    if(dirId >= 0)
    {
        ALOGD("(f:%s, l:%d) dir[%s] is exist.\n", __FUNCTION__, __LINE__, dirName);
        fat_closedir(dirId);
    }
    else
    {
        ALOGD("(f:%s, l:%d) dir[%s] is not exist, create it.\n", __FUNCTION__, __LINE__, dirName);
        ret = fat_mkdir(dirName);
    }
    return ret;
#else
    ret = 0;
    if(access (dirName, F_OK) != 0)
    {
        ALOGV("creat directory , path=%s is not exist,so creat it\n", dirName);
        ret = mkdir(dirName, mode);
    }
    return ret;
#endif
}

