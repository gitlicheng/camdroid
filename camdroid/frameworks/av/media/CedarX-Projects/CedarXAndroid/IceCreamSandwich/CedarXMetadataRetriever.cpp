/*
 * Copyright (C) 2009 The Android Open Source Project
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

//#define LOG_NDEBUG 0
#include <CDX_LogNDebug.h>
#define LOG_TAG "CedarXMetadataRetriever"
#include <CDX_Debug.h>

#include "CedarXMetadataRetriever.h"

#include <media/stagefright/ColorConverter.h>
#include <media/stagefright/DataSource.h>
#include <media/stagefright/FileSource.h>
#if (CEDARX_ANDROID_VERSION < 7)
#include <media/stagefright/MediaDebug.h>
#else
#include <media/stagefright/foundation/ADebug.h>
#endif
#include <media/stagefright/MediaExtractor.h>
#include <media/stagefright/MetaData.h>
//#include <media/stagefright/OMXCodec.h>
#include <byteswap.h>

#include <CDX_PlayerAPI.h>
#include <CDX_UglyDef.h>
#include <stdio.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>

namespace android
{

/* add by Gary. start {{----------------------------------- */
static int _Convert2UTF8( const uint8_t *src, size_t size, __a_audio_fonttype_e charset, String8 *s );
/* add by Gary. end   -----------------------------------}} */

CedarXMetadataRetriever::CedarXMetadataRetriever()
    : bCDXMetaRetriverInit(false),
      mParsedMetaData(false),
      mAlbumArt(NULL)
{
    LOGV("constructor");
    mRetriever = NULL;
}

CedarXMetadataRetriever::~CedarXMetadataRetriever()
{
    LOGV("destructor");
    if(bCDXMetaRetriverInit)
    {
    	LOGV("CDXRetriever_Destroy called!");
        CDXRetriever_Destroy(mRetriever);
        bCDXMetaRetriverInit = false;
    }

    if(mAlbumArt)
    {
    	delete mAlbumArt;
        mAlbumArt = NULL;
    }
	//ALOGV("destroy retriever finish.");
}

status_t CedarXMetadataRetriever::setDataSource(const char *url, const KeyedVector<String8, String8> *headers)
{
	LOGV("setDataSource(%s)", url);
    mParsedMetaData = false;

    mMetaData.clear();

    LOGV(" set data source %s.",url);
    if(mAlbumArt != NULL)
    {
    	delete mAlbumArt;
    	mAlbumArt = NULL;
    }

    //ALOGV("set data source step 2.");
    if(!bCDXMetaRetriverInit)
    {
    	if(CDXRetriever_Create((void**)&mRetriever) != 0)
    	    return UNKNOWN_ERROR;

    	bCDXMetaRetriverInit = true;
    }

    //ALOGV("set data source step 3.");
    if(mRetriever->control(mRetriever, CDX_SET_DATASOURCE_URL, (unsigned int)url, 0) != 0)
       	return UNKNOWN_ERROR;

    bCDXMetaRetriverInit = true;

    //ALOGV("set data source step 4.");
    return OK;
}

// Warning caller retains ownership of the filedescriptor! Dup it if necessary.
status_t CedarXMetadataRetriever::setDataSource(int fd, int64_t offset, int64_t length)
{
	LOGV("setDataSource(%d, %lld, %lld)", fd, offset, length);

    fd = dup(fd);

    mParsedMetaData = false;

    mMetaData.clear();

    if(mAlbumArt != NULL)
    {
    	delete mAlbumArt;
    	mAlbumArt = NULL;
    }

    if(!bCDXMetaRetriverInit)
    {
    	if(CDXRetriever_Create((void**)&mRetriever) != 0) {
            ALOGE("<F:%s, L:%d>CDXRetriever_Create error!", __FUNCTION__, __LINE__);
            ::close(fd);
    		return UNKNOWN_ERROR;
    	}

    	bCDXMetaRetriverInit = true;
    }

    CedarXExternFdDesc cdx_ext_fd;
    cdx_ext_fd.fd     = fd;
    cdx_ext_fd.offset = offset;
    cdx_ext_fd.length = length;
    if(mRetriever->control(mRetriever, CDX_SET_DATASOURCE_FD, (unsigned int)(&cdx_ext_fd), 0) != 0) {
        ALOGE("<F:%s, L:%d>CDX_SET_DATASOURCE_FD error!", __FUNCTION__, __LINE__);
        ::close(fd);
    	return UNKNOWN_ERROR;
    }

    return OK;
}


sp<IMemory> CedarXMetadataRetriever::getStreamAtTime(int64_t timeUs)
{
    VideoThumbnailInfo vd_thumb_info;
    sp<IMemory> mem = NULL;
    unsigned char* pOutBuf;

    memset(&vd_thumb_info, 0, sizeof(VideoThumbnailInfo));
    vd_thumb_info.format         = VIDEO_THUMB_YUVPLANNER; //0: JPEG STREAM  1: YUV RAW STREAM
    vd_thumb_info.capture_time   = 20*1000;
    vd_thumb_info.capture_result = 0;

	mRetriever->control(mRetriever, CDX_CMD_CAPTURE_THUMBNAIL_STREAM, (unsigned int)(&vd_thumb_info), 0);
	if(vd_thumb_info.capture_result)
	{
	    sp<MemoryHeapBase> heap = new MemoryHeapBase(vd_thumb_info.thumb_stream_size + 4, 0, "CedarXMetadataRetriever");
	    if (heap == NULL)
	        LOGE("failed to create MemoryDealer");

	    mem = new MemoryBase(heap, 0, vd_thumb_info.thumb_stream_size + 4);
	    if (mem == NULL)
	    {
	        LOGE("not enough memory for stream size = %u", vd_thumb_info.thumb_stream_size);
	    }
	    else
	    {
		    pOutBuf = static_cast<unsigned char*>(mem->pointer());
		    pOutBuf[0] = (vd_thumb_info.thumb_stream_size) & 0xff;
		    pOutBuf[1] = (vd_thumb_info.thumb_stream_size>>8) & 0xff;
		    pOutBuf[2] = (vd_thumb_info.thumb_stream_size>>16) & 0xff;
		    pOutBuf[3] = (vd_thumb_info.thumb_stream_size>>24) & 0xff;
		    //ALOGV(" athumb stream size is %d", vd_thumb_info.thumb_stream_size);
		    memcpy(pOutBuf+4, vd_thumb_info.thumb_stream_address, vd_thumb_info.thumb_stream_size);
	    }
	}

    mRetriever->control(mRetriever, CDX_CMD_CLOSE_CAPTURE, 0, 0);

    //ALOGV(" getStreamAtTime finish.");
	return mem;
}


VideoFrame *CedarXMetadataRetriever::getFrameAtTime(int64_t timeUs, int option)
{
    ALOGV("getFrameAtTime: timeUs=%lld, option=%d", timeUs, option);
    VideoThumbnailInfo vd_thumb_info;

    memset(&vd_thumb_info, 0, sizeof(VideoThumbnailInfo));
    vd_thumb_info.format         = VIDEO_THUMB_YUVPLANNER; //0: JPEG STREAM  1: YUV RAW STREAM
    //vd_thumb_info.capture_time   = 18*1000;
    vd_thumb_info.capture_time   = 0;
    vd_thumb_info.require_width  = 512;
    vd_thumb_info.require_height = 512;
    vd_thumb_info.capture_result = 0;

	mRetriever->control(mRetriever, CDX_CMD_CAPTURE_THUMBNAIL, (unsigned int)(&vd_thumb_info), 0);

    if(!vd_thumb_info.capture_result)
    {
        ALOGE("<F:%s, L:%d>CDX_CMD_CAPTURE_THUMBNAIL error!", __FUNCTION__, __LINE__);
    	mRetriever->control(mRetriever, CDX_CMD_CLOSE_CAPTURE, 0, 0);
    	return NULL;
    }

    VideoFrame *frame = new VideoFrame;
    int width         = vd_thumb_info.require_width;
    int height        = vd_thumb_info.require_height;

    frame->mWidth         = width;
    frame->mHeight        = height;
    frame->mDisplayWidth  = width;
    frame->mDisplayHeight = height;
    frame->mSize          = width * height * 2;
    frame->mData          = new uint8_t[frame->mSize];
    frame->mRotationAngle = 0;
    //ALOGE("(f:%s, l:%d) can't convert now!", __FUNCTION__, __LINE__);
    ColorConverter converter((OMX_COLOR_FORMATTYPE)OMX_COLOR_FormatYUV420Planar, OMX_COLOR_Format16bitRGB565);
    CHECK(converter.isValid());

    converter.convert(vd_thumb_info.thumb_stream_address,
                      ((width + 31)>>5)<<5,
                      ((height + 31)>>5)<<5,
                      0,
                      0,
                      frame->mWidth - 1,
                      frame->mHeight - 1,
                      frame->mData,
                      frame->mWidth,
                      frame->mHeight,
                      0,
                      0,
                      frame->mWidth - 1,
                      frame->mHeight - 1);

    mRetriever->control(mRetriever, CDX_CMD_CLOSE_CAPTURE, 0, 0);

    return frame;
}

MediaAlbumArt* CedarXMetadataRetriever::extractAlbumArt()
{
    return NULL;
    //LOGV("extractAlbumArt (extractor: %s)", mExtractor.get() != NULL ? "YES" : "NO");
#ifdef __ANDROID_VERSION_2_3_1
    if (0 == (mMode & METADATA_MODE_METADATA_RETRIEVAL_ONLY)) {
        LOGV("extractAlbumArt/metadata retrieval disabled by mode");

        return NULL;
    }
#endif

    if(bCDXMetaRetriverInit == false)
        return NULL;

    if (!mParsedMetaData)
    {
        parseMetaData();
        mParsedMetaData = true;
    }

    if (mAlbumArt)
        return new MediaAlbumArt(*mAlbumArt);

    return NULL;
}

const char* CedarXMetadataRetriever::extractMetadata(int keyCode)
{
    return NULL;

#ifdef __ANDROID_VERSION_2_3_1
    if (0 == (mMode & METADATA_MODE_METADATA_RETRIEVAL_ONLY)) {
        LOGV("extractAlbumArt/metadata retrieval disabled by mode");

        return NULL;
    }
#endif

    if(bCDXMetaRetriverInit == false)
        return NULL;

    if (!mParsedMetaData)
    {
        parseMetaData(); //TODO SEGFAUT
        mParsedMetaData = true;
    }

    ssize_t index = mMetaData.indexOfKey(keyCode);

    if (index < 0)
        return NULL;

    return strdup(mMetaData.valueAt(index).string());
}



void CedarXMetadataRetriever::parseMetaData()
{
    return;
	CedarXMetaData     cdx_metadata;
	audio_file_info_t* audio_metadata = &cdx_metadata.audio_metadata;

    /* modified by Gary. start {{----------------------------------- */
    String8 s8;
    int     ret;
    
    memset(&cdx_metadata, 0,sizeof(CedarXMetaData));

	//LOGV("begin CDX_CMD_GET_METADATA mRetriever:%p",mRetriever);
    mRetriever->control(mRetriever, CDX_CMD_GET_METADATA, (unsigned int)&cdx_metadata, 0);
    LOGV("add meta data...");

    if(cdx_metadata.cdx_metadata_type == CDX_METADATA_TYPE_AUDIO)
    {
		ret = _Convert2UTF8( (uint8_t *)audio_metadata->ulauthor,
				             audio_metadata->ulauthor_sz,
							 audio_metadata->ulauthorCharEncode,
							 &s8 );
		if( ret == 0 )
		{
			mMetaData.add(METADATA_KEY_ARTIST, s8);
			mMetaData.add(METADATA_KEY_ALBUMARTIST, s8);
			mMetaData.add(METADATA_KEY_AUTHOR, s8);
			mMetaData.add(METADATA_KEY_WRITER, s8);
		}

		ret = _Convert2UTF8( (uint8_t *)audio_metadata->ulGenre,
				             audio_metadata->ulGenre_sz,
							 audio_metadata->ulGenreCharEncode,
							 &s8 );
		if( ret == 0 )
			mMetaData.add(METADATA_KEY_GENRE, s8);

		ret = _Convert2UTF8( (uint8_t *)audio_metadata->ultitle,
				             audio_metadata->ultitle_sz,
							 audio_metadata->ultitleCharEncode,
							 &s8 );

		if( ret == 0 )
			mMetaData.add(METADATA_KEY_TITLE, s8);

		ret = _Convert2UTF8( (uint8_t *)audio_metadata->ulAlbum,
				             audio_metadata->ulAlbum_sz,
							 audio_metadata->ulAlbumCharEncode,
							 &s8 );

		if( ret == 0 )
			mMetaData.add(METADATA_KEY_ALBUM, s8);

		ret = _Convert2UTF8( (uint8_t *)audio_metadata->ulYear,
				             audio_metadata->ulYear_sz,
							 audio_metadata->ulYearCharEncode,
							 &s8);

		if( ret == 0 )
			mMetaData.add(METADATA_KEY_YEAR, s8);

		if( audio_metadata->ulDuration > 0 )
		{
			char* str = NULL;

			str = new char[32];
			if(str != NULL)
			{
				sprintf( str, "%d", audio_metadata->ulDuration);
				mMetaData.add(METADATA_KEY_DURATION, String8(str));
				delete[] str;
			}
		}
		/* modified by Gary. end   -----------------------------------}} */
    }
    else
    {
    	char str[32];

    	if(cdx_metadata.geo_len > 0)
    	{
			String8 s8_geo;

			s8_geo = cdx_metadata.geo_data;
			LOGV("geo_data:%s", cdx_metadata.geo_data);
			mMetaData.add(METADATA_KEY_LOCATION, s8_geo);
		}

    	if(cdx_metadata.duration > 0)
		{
			sprintf(str, "%d", cdx_metadata.duration);
			LOGV("METADATA_KEY_DURATION:%d", cdx_metadata.duration);
			mMetaData.add(METADATA_KEY_DURATION, String8(str));
		}

        if(cdx_metadata.nHasAudio> 0)                       
		{
			sprintf(str, "%d", cdx_metadata.nHasAudio);
			LOGV("METADATA_KEY_HAS_AUDIO:%d", cdx_metadata.nHasAudio);
			mMetaData.add(METADATA_KEY_HAS_AUDIO, String8(str));
		} 

        if(cdx_metadata.nHasVideo> 0)
		{
			sprintf(str, "%d", cdx_metadata.nHasVideo);
			LOGV("METADATA_KEY_HAS_VIDEO:%d", cdx_metadata.nHasVideo);
			mMetaData.add(METADATA_KEY_HAS_VIDEO, String8(str));
 
            sprintf(str, "%d", cdx_metadata.width);
			LOGV("METADATA_KEY_VIDEO_WIDTH:%d", cdx_metadata.width);
			mMetaData.add(METADATA_KEY_VIDEO_WIDTH, String8(str));

            sprintf(str, "%d", cdx_metadata.height);
			LOGV("METADATA_KEY_VIDEO_HEIGHT:%d", cdx_metadata.height);
			mMetaData.add(METADATA_KEY_VIDEO_HEIGHT, String8(str));
            
        #if (CEDARX_ANDROID_VERSION > 7)
			sprintf(str, "%d", cdx_metadata.nRotationAngle);
			LOGV("METADATA_KEY_VIDEO_ROTATION:%d", cdx_metadata.nRotationAngle);
			mMetaData.add(METADATA_KEY_VIDEO_ROTATION, String8(str));  
        #endif
        
		}

    }
    
//    { kKeyMIMEType, METADATA_KEY_MIMETYPE },//ulAudio_name_sz
//    { kKeyCDTrackNumber, METADATA_KEY_CD_TRACK_NUMBER },
//    { kKeyDiscNumber, METADATA_KEY_DISC_NUMBER },
//    { kKeyAlbum, METADATA_KEY_ALBUM },//pic
//    { kKeyArtist, METADATA_KEY_ARTIST },//ulauthor
//    { kKeyAlbumArtist, METADATA_KEY_ALBUMARTIST },//ulauthor
//    { kKeyAuthor, METADATA_KEY_AUTHOR },//ulauthor
//    { kKeyComposer, METADATA_KEY_COMPOSER },//ulauthor
//    { kKeyDate, METADATA_KEY_DATE },//
//    { kKeyGenre, METADATA_KEY_GENRE },//ulGenre
//    { kKeyTitle, METADATA_KEY_TITLE }, //ultitle
//    { kKeyYear, METADATA_KEY_YEAR },//year
//    { kKeyWriter, METADATA_KEY_WRITER },//ulauthor ?

}


/* add by Gary. start {{----------------------------------- */

/*
****************************************************************************************************
*Name        : _GeneralStream2UTF8Stream
*Prototype   : int _GeneralStream2UTF8Stream( const uint8_t *src, int len, uint8_t *dst, int size )
*Arguments   : src   input. the source byte stream in some charset.
*              len   input. the size of the source byte stream in byte.
*              dst   output.a buffer to store the output stream in UTF8.
*              size  input. the size of the buffer.
*Return      : the numbers of the source stream bytes converted to UTF8. -1 means fail.
*Description : convert a byte stream in some charset to a stream in UTF8.
*Other       :
****************************************************************************************************
*/
static int _GeneralStream2UTF8Stream( const uint8_t *src, int len, uint8_t *dst, int size )
{
    uint8_t *ptr;
    uint8_t *end;
    int      i;
    
    if( src == NULL || dst == NULL)
        return -1;
        
    ptr = dst;
    end = dst+size;
    for (i = 0; i < len; ++i)
    {
        if (src[i] == '\0')
        {
            break;
        }
        else if (src[i] < 0x80)
        {
            if( end - ptr < 2 )
                break;
            *ptr++ = src[i];
        }
        else if (src[i] < 0xc0)
        {
            if( end - ptr < 3 )
                break;
            *ptr++ = 0xc2;
            *ptr++ = src[i];
        }
        else
        {
            if( end - ptr < 3 )
                break;
            *ptr++ = 0xc3;
            *ptr++ = src[i] - 64;
        }
    }

    *ptr = '\0';
    return i;
}




/*
****************************************************************************************************
*Name        : _IsUTF8Stream
*Prototype   : int _IsUTF8Stream( const char* bytes )
*Arguments   : bytes   input. a byte stream.
*              size    input. the size of the stream.
*Return      : return 1 if the string is in UTF8; else return 0.
*Description : check whether a string is in UTF8 or not.
*Other       :
****************************************************************************************************
*/
static int _IsUTF8Stream( const char* bytes, int size )
{
    const char* end = bytes+size;
    char  utf8;
    
    if (bytes == NULL || size <= 0)
        return 0;

    while ( bytes < end )
    {
        utf8 = *(bytes++);
        // Switch on the high four bits.
        switch (utf8 >> 4)
        {
            case 0x00:
            case 0x01:
            case 0x02:
            case 0x03:
            case 0x04:
            case 0x05:
            case 0x06:
            case 0x07: {
                // Bit pattern 0 No need for any extra bytes.
                break;
            }
            case 0x08:
            case 0x09:
            case 0x0a:
            case 0x0b:
            case 0x0f: {
                /*
                 * Bit pattern 10xx or 1111, which are illegal start bytes.
                 * Note: 1111 is valid for normal UTF-8, but not the
                 * modified UTF-8 used here.
                 */
                LOGV("JNI WARNING: illegal start byte 0x%x\n", utf8);
                goto fail;
            }
            case 0x0e: {
                // Bit pattern 1110, so there are two additional bytes.
                utf8 = *(bytes++);
                if ( bytes > end || (utf8 & 0xc0) != 0x80) {
                    LOGV("JNI WARNING: illegal continuation byte 0x%x\n", utf8);
                    goto fail;
                }
                // Fall through to take care of the final byte.
            }
            case 0x0c:
            case 0x0d: {
                // Bit pattern 110x, so there is one additional byte.
                utf8 = *(bytes++);
                if ( bytes > end || (utf8 & 0xc0) != 0x80) {
                    LOGV("JNI WARNING: illegal continuation byte 0x%x\n", utf8);
                    goto fail;
                }
                break;
            }
        }
    }

    return 1;

fail:
    return 0;
}



static int _Convert2UTF8( const uint8_t *src, size_t size, __a_audio_fonttype_e charset, String8 *s )
{
    uint8_t *buf = NULL;
    int      buf_size;
    
    if( src == NULL || size <= 0 )
    {
        return -1;
    }
    
    switch( charset )
    {
        case A_AUDIO_FONTTYPE_UTF_16BE:
        {
            // API wants number of characters, not number of bytes...
            int len = size / 2;
            const char16_t *framedata = (const char16_t *) src;
            char16_t *framedatacopy = new char16_t[len];
            // swap
            for (int i = 0; i < len; i++) {
                framedatacopy[i] = bswap_16(framedata[i]);
            }
            framedata = framedatacopy;
            // skip BOM
            if (*framedata == 0xfeff) {
                framedata++;
                len--;
            }
            s->setTo(framedata, len);
            if (framedatacopy != NULL) {
                delete[] framedatacopy;
            }
            break;
        }
        case A_AUDIO_FONTTYPE_UTF_16LE:
        {
            int len = size / 2;
            const char16_t *framedata = (const char16_t *) src;
            // skip BOM
            if (*framedata == 0xfeff) {
                framedata++;
                len--;
            }
            s->setTo(framedata, len);
            break;
        }
        case A_AUDIO_FONTTYPE_UTF_8:
        {
            if( _IsUTF8Stream( (const char *)src, size ) ){
                s->setTo( (const char *)src, size );
                break;
            }
            // no "break" and go on 
        }
        default :
        {
            buf_size = size*2+1;
            buf = new uint8_t[buf_size];
            if( buf == NULL )
            {
                LOGV("Fail in allocating memory with size %d.\n", buf_size);
                return -1;
            }
    
            _GeneralStream2UTF8Stream( src, size, buf, buf_size );
            s->setTo( (const char*)buf, strlen( (char *)buf ) );
            if(  buf != NULL )
            {
                delete[] buf;
            }
            break;
        }
    }
    
    return 0;
}
        
/* add by Gary. end   -----------------------------------}} */
        
};  // namespace android

        
