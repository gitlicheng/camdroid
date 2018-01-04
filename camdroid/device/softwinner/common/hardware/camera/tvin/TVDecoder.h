#ifndef __TV_DECODER_H__
#define __TV_DECODER_H__

namespace android {

typedef enum
{
	TVD_UV_SWAP             = 0,
	TVD_COLOR_SET           = 1,
}tvd_param_t;

typedef enum
{
	TVD_CVBS                = 0,
	TVD_YPBPR_I             = 1,
	TVD_YPBPR_P             = 2,
}tvd_interface_t;

typedef enum
{
	TVD_NTSC                = 0,
	TVD_PAL                 = 1,
	TVD_SECAM               = 2,
}tvd_system_t;

typedef enum
{
	TVD_PL_YUV420           = 0,
	TVD_MB_YUV420           = 1,
	TVD_PL_YUV422           = 2,
}tvd_fmt_t;

typedef enum
{
	TVD_CHANNEL_ONLY_1      = 0,
	TVD_CHANNEL_ONLY_2      = 1,
	TVD_CHANNEL_ONLY_3      = 2, 
	TVD_CHANNEL_ONLY_4      = 3,
	TVD_CHANNEL_ALL_2x2     = 4,
	TVD_CHANNEL_ALL_1x4     = 5,
	TVD_CHANNEL_ALL_4x1     = 6,
	TVD_CHANNEL_ALL_1x2	    = 7,
	TVD_CHANNEL_ALL_2x1	    = 8,
}tvd_channel_t;


class TVDecoder {
public:
	TVDecoder(int fd);
	~TVDecoder();
	status_t v4l2SetVideoParams(int *width, int *height);
	status_t v4l2SetColor();
	status_t getGeometry(int *width, int *height);
	status_t getSystem(int *system);
	bool isSystemChange();
	int getFormat();
	int getSysValue();

private:
	int mTVDFd;
	int mInterface;
	int mSystem;    //TVD_NTSC
	int mFormat;
	int mChannel;
	int mLuma;
	int mContrast;
	int mSaturation;
	int mHue;
};

};

#endif

