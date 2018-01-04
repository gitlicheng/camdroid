#define LOG_NDEBUG 0
#define LOG_TAG "TVDecoder"
#include <utils/Log.h>

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <utils/Errors.h>
#include <linux/videodev.h> 

#include "TVDecoder.h"


namespace android {

TVDecoder::TVDecoder(int fd)
	: mTVDFd(fd)
	, mInterface(TVD_CVBS)
	, mSystem(TVD_NTSC)
	, mFormat(TVD_PL_YUV420)
	, mChannel(TVD_CHANNEL_ONLY_1)
	, mLuma(50)
	, mContrast(50)
	, mSaturation(50)
	, mHue(50)
{
}

TVDecoder::~TVDecoder()
{
}

status_t TVDecoder::v4l2SetVideoParams(int *width, int *height)
{
	struct v4l2_format format;
	struct v4l2_streamparm parms;
	int system;
	status_t ret;

RESET_PARAMS:
	ALOGV("TVD interface=%d, system=%d, format=%d, channel=%d", mInterface, mSystem, mFormat, mChannel);
	memset(&format, 0, sizeof(format));
	format.type = V4L2_BUF_TYPE_PRIVATE;
	format.fmt.raw_data[0] = mInterface;
	format.fmt.raw_data[1] = mSystem;
	if (ioctl(mTVDFd, VIDIOC_S_FMT, &format) < 0)
	{
		ALOGE("<F:%s, L:%d>VIDIOC_S_FMT error(%s)!", __FUNCTION__, __LINE__, strerror(errno));
		return UNKNOWN_ERROR;
	}
	usleep(100000);

	format.fmt.raw_data[2] = mFormat;

	switch(mChannel)
	{
		case TVD_CHANNEL_ONLY_1:
			format.fmt.raw_data[8]  = 1;	        //row
			format.fmt.raw_data[9]  = 1;	        //column
			format.fmt.raw_data[10] = 1;		//channel_index0
			format.fmt.raw_data[11] = 0;		//channel_index1
			format.fmt.raw_data[12] = 0;		//channel_index2
			format.fmt.raw_data[13] = 0;		//channel_index3
			break;

		case TVD_CHANNEL_ONLY_2:
			format.fmt.raw_data[8]  = 1;	        //row
			format.fmt.raw_data[9]  = 1;	        //column
			format.fmt.raw_data[10] = 0;		//channel_index0
			format.fmt.raw_data[11] = 1;		//channel_index1
			format.fmt.raw_data[12] = 0;		//channel_index2
			format.fmt.raw_data[13] = 0;		//channel_index3
			break;

		case TVD_CHANNEL_ONLY_3:
			format.fmt.raw_data[8]  = 1;	        //row
			format.fmt.raw_data[9]  = 1;	        //column
			format.fmt.raw_data[10] = 0;          //channel_index0
			format.fmt.raw_data[11] = 0;          //channel_index1
			format.fmt.raw_data[12] = 1;          //channel_index2
			format.fmt.raw_data[13] = 0;          //channel_index3
			break;

		case TVD_CHANNEL_ONLY_4:
			format.fmt.raw_data[8]  = 1;	        //row
			format.fmt.raw_data[9]  = 1;	        //column
			format.fmt.raw_data[10] = 0;          //channel_index0
			format.fmt.raw_data[11] = 0;          //channel_index1
			format.fmt.raw_data[12] = 0;          //channel_index2
			format.fmt.raw_data[13] = 1;          //channel_index3
			break;

		case TVD_CHANNEL_ALL_2x2:
			format.fmt.raw_data[8]  = 2;	        //row
			format.fmt.raw_data[9]  = 2;	        //column
			format.fmt.raw_data[10] = 1;          //channel_index0
			format.fmt.raw_data[11] = 2;          //channel_index1
			format.fmt.raw_data[12] = 3;          //channel_index2
			format.fmt.raw_data[13] = 4;          //channel_index3
			break;

		case TVD_CHANNEL_ALL_1x4:
			format.fmt.raw_data[8]  = 1;	        //row
			format.fmt.raw_data[9]  = 4;	        //column
			format.fmt.raw_data[10] = 1;          //channel_index0
			format.fmt.raw_data[11] = 2;          //channel_index1
			format.fmt.raw_data[12] = 3;          //channel_index2
			format.fmt.raw_data[13] = 4;          //channel_index3
			break;

		case TVD_CHANNEL_ALL_4x1:
			format.fmt.raw_data[8]  = 4;	        //row
			format.fmt.raw_data[9]  = 1;	        //column
			format.fmt.raw_data[10] = 1;          //channel_index0
			format.fmt.raw_data[11] = 2;          //channel_index1
			format.fmt.raw_data[12] = 3;          //channel_index2
			format.fmt.raw_data[13] = 4;          //channel_index 3
			break;

		case TVD_CHANNEL_ALL_1x2:
			format.fmt.raw_data[8]  = 1;	        //row
			format.fmt.raw_data[9]  = 2;	        //column
			format.fmt.raw_data[10] = 1;          //channel_index0
			format.fmt.raw_data[11] = 2;          //channel_index1
			format.fmt.raw_data[12] = 0;          //channel_index2
			format.fmt.raw_data[13] = 0;          //channel_index 3
			break;

		case TVD_CHANNEL_ALL_2x1:
			format.fmt.raw_data[8]  = 2;	        //row
			format.fmt.raw_data[9]  = 1;	        //column
			format.fmt.raw_data[10] = 1;          //channel_index0
			format.fmt.raw_data[11] = 2;          //channel_index1
			format.fmt.raw_data[12] = 0;          //channel_index2
			format.fmt.raw_data[13] = 0;          //channel_index 3
			break;

		default:
		    break;
	}

	if (ioctl(mTVDFd, VIDIOC_S_FMT, &format) < 0)
	{
		ALOGE("<F:%s, L:%d>VIDIOC_S_FMT error(%s)!", __FUNCTION__, __LINE__, strerror(errno));
		return UNKNOWN_ERROR;
	}

    int nTryMatchCnt = 0;
    for (int i = 0; i < 100; i++)
    {
        ret = getSystem(&system);
        if (ret == NO_ERROR)
        {
            if (mSystem != system)
            {
                ALOGD("(f:%s, l:%d) [%d], TVSystem[%d] not match[%d], consume [%d]ms, goto RESET_PARAMS", __FUNCTION__, __LINE__, i, mSystem, system, i*40);
                mSystem = system;
                goto RESET_PARAMS;
            }
            else
            {
                nTryMatchCnt++;
                if(nTryMatchCnt >= 2)
                {
                    ALOGD("(f:%s, l:%d) test mSystem[%d] match [%d]times, can sure! consume [%d]ms", __FUNCTION__, __LINE__, mSystem, nTryMatchCnt, i*40);
                    break;
                }
                else
                {
                    usleep(40000);
                    continue;
                }
            }
        }
        else if (ret == WOULD_BLOCK)
        {
            usleep(40000);
            continue;
        }
        else
        {
            ALOGE("<F:%s, L:%d>getSystem error", __FUNCTION__, __LINE__);
            return UNKNOWN_ERROR;
        }
    }
    
//checkGeometry:
	if (getGeometry(width, height) != NO_ERROR) {
		ALOGE("<F:%s, L:%d>getGeometry error", __FUNCTION__, __LINE__);
		return UNKNOWN_ERROR;
	}
	ALOGV("system=%d, width=%d, height=%d", mSystem, *width, *height);
//    if(TVD_NTSC == mSystem)
//    {
//        if(*height!=480)
//        {
//            ALOGD("(f:%s, l:%d) possible error, TVD_NTSC, [%dx%d], wait 40ms", __FUNCTION__, __LINE__, *width, *height);
//            usleep(40000);
//            goto checkGeometry;
//        }
//    }
//    else if(TVD_PAL == mSystem)
//    {
//        if(*height!=576)
//        {
//            ALOGD("(f:%s, l:%d) possible error, TVD_PAL, [%dx%d], wait 40ms", __FUNCTION__, __LINE__, *width, *height);
//            usleep(40000);
//            goto checkGeometry;
//        }
//    }

	v4l2SetColor();

	if(mFormat == TVD_MB_YUV420)
	{
		parms.parm.raw_data[0] = TVD_UV_SWAP;
		parms.parm.raw_data[1] = 0;
	}
	else
	{
		parms.parm.raw_data[0] = TVD_UV_SWAP;
		parms.parm.raw_data[1] = 0;
	}
	parms.type = V4L2_BUF_TYPE_PRIVATE;
	if(ioctl(mTVDFd, VIDIOC_S_PARM, &parms) < 0)
	{
		ALOGE("<F:%s, L:%d>VIDIOC_S_PARM error(%s)!", __FUNCTION__, __LINE__, strerror(errno));
		return UNKNOWN_ERROR;
	}

	return OK;
}

status_t TVDecoder::getSystem(int *system)
{
	struct v4l2_format format;

	memset(&format, 0, sizeof(format));

	format.type = V4L2_BUF_TYPE_PRIVATE;
	if (ioctl (mTVDFd, VIDIOC_G_FMT, &format) < 0)
	{
		ALOGE("<F:%s, L:%d>VIDIOC_G_FMT error!", __FUNCTION__, __LINE__);
		return UNKNOWN_ERROR;
	}

	switch(mChannel)
	{
		case TVD_CHANNEL_ONLY_1:
			if((format.fmt.raw_data[16] & 1) == 0)
			{
				ALOGV("<F:%s, L:%d>No signal detected", __FUNCTION__, __LINE__);
				return WOULD_BLOCK;
			}
			if((format.fmt.raw_data[16] & (1 << 4)) != 0)
			{
				*system = TVD_PAL;
			}
			else
			{
				*system = TVD_NTSC;
			}
			//ALOGV("format.fmt.raw_data[16] =0x%x",format.fmt.raw_data[16]);
			break;

		case TVD_CHANNEL_ONLY_2:
			if((format.fmt.raw_data[17] & 1) == 0)
			{
				//ALOGV("<F:%s, L:%d>No signal detected", __FUNCTION__, __LINE__);
				return WOULD_BLOCK;
			}
			if((format.fmt.raw_data[17] & (1 << 4)) != 0)
			{
				*system = TVD_PAL;
			}
			else
			{
				*system = TVD_NTSC;
			}
			//ALOGV("format.fmt.raw_data[17] =0x%x",format.fmt.raw_data[17]);
			break;

		case TVD_CHANNEL_ONLY_3:
			if((format.fmt.raw_data[18] & 1) == 0)
			{
				//ALOGV("<F:%s, L:%d>No signal detected", __FUNCTION__, __LINE__);
				return WOULD_BLOCK;
			}
			if((format.fmt.raw_data[18] & (1 << 4)) != 0)
			{
				*system = TVD_PAL;
			}
			else
			{
				*system = TVD_NTSC;
			}
			//ALOGV("format.fmt.raw_data[18] =0x%x",format.fmt.raw_data[18]);
			break;

		case TVD_CHANNEL_ONLY_4:
			if((format.fmt.raw_data[19] & 1) == 0)
			{
				//ALOGV("<F:%s, L:%d>No signal detected", __FUNCTION__, __LINE__);
				return WOULD_BLOCK;
			}
			if((format.fmt.raw_data[19] & (1 << 4 )) != 0)
			{
				*system = TVD_PAL;
			}
			else
			{
				*system = TVD_NTSC;
			}
			//ALOGV("format.fmt.raw_data[19] =0x%x",format.fmt.raw_data[19]);
			break;

		case TVD_CHANNEL_ALL_2x2:
		case TVD_CHANNEL_ALL_1x4:
		case TVD_CHANNEL_ALL_4x1:
			if (((format.fmt.raw_data[16] & 1) || (format.fmt.raw_data[17] & 1)
			|| (format.fmt.raw_data[18] & 1) || (format.fmt.raw_data[19] & 1)) == 0)
			{
				//ALOGV("<F:%s, L:%d>No signal detected", __FUNCTION__, __LINE__);
				return WOULD_BLOCK; 
			}
			if((format.fmt.raw_data[16]&(1<<4)) !=0 )
			{
				*system = TVD_PAL;
			}
			else if((format.fmt.raw_data[16]&(1<<4)) == 0)
			{
				*system = TVD_NTSC;
			}
			break;

		case TVD_CHANNEL_ALL_1x2:
		case TVD_CHANNEL_ALL_2x1:
		if (((format.fmt.raw_data[16] & 1) || (format.fmt.raw_data[17] & 1)) == 0)
		{
			//ALOGV("<F:%s, L:%d>No signal detected", __FUNCTION__, __LINE__);
			return WOULD_BLOCK;
		}
		if ((format.fmt.raw_data[16]&(1<<4)) != 0) {
			*system = TVD_PAL;
		} else if ((format.fmt.raw_data[16] & (1<<4)) == 0) {
			*system = TVD_NTSC;
		}
		break;
	}

	return NO_ERROR;
}

status_t TVDecoder::v4l2SetColor()
{
	struct v4l2_streamparm parms;
	int tmp_hue = 0;

	if (mHue >= 50 && mHue <= 100) {
		tmp_hue = mHue - 50;
	} else if (mHue >= 0 && mHue < 50) {
		tmp_hue = 50 - mHue;
	}
	parms.type = V4L2_BUF_TYPE_PRIVATE;
	parms.parm.raw_data[0] = TVD_COLOR_SET;
	parms.parm.raw_data[1] = mLuma / 1.2;			        //luma(0~255)
	parms.parm.raw_data[2] = mContrast * 255 / 100;			//contrast(0~255)
	parms.parm.raw_data[3] = mSaturation * 255 / 100;			//saturation(0~255)
	parms.parm.raw_data[4] = tmp_hue* 256 / 100;		        //hue(0~255)
	if(ioctl(mTVDFd, VIDIOC_S_PARM, &parms) < 0)
	{
		ALOGE("<F:%s, L:%d>VIDIOC_S_PARM error(%s)!", __FUNCTION__, __LINE__, strerror(errno));
		return UNKNOWN_ERROR;
	}
	return NO_ERROR;
}

status_t TVDecoder::getGeometry(int *width, int *height)
{
	struct v4l2_format format;

	memset(&format, 0, sizeof(v4l2_format));
	format.type     = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(mTVDFd, VIDIOC_G_FMT, &format) < 0)
	{
		ALOGE("<F:%s, L:%d>VIDIOC_G_FMT Failed(%s)", __FUNCTION__, __LINE__, strerror(errno));
		return UNKNOWN_ERROR;
	}

	*width = format.fmt.pix.width;
	*height = format.fmt.pix.height;
	ALOGV("<F:%s, L:%d>=====width=%d, height=%d", __FUNCTION__, __LINE__, *width, *height);
	return NO_ERROR;
}

bool TVDecoder::isSystemChange()
{
	int system;

	if (getSystem(&system) == NO_ERROR)
	{
		if (mSystem != system)
		{
			mSystem = system;
			return true;
		}
	}
	return false;
}

int TVDecoder::getFormat()
{
	return mFormat;
}

int TVDecoder::getSysValue()
{
	return mSystem;
}

};

