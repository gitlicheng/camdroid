#define N_(x) x

#include <errno.h>
#include <sys/ioctl.h>
#include "sonix_xu_ctrls.h"
#include "sonix_debug.h"
#include <stdlib.h>

struct H264Format *gH264fmt = NULL;

unsigned char m_CurrentFPS = 24;
unsigned char m_CurrentFPS_292 = 30;
unsigned int  chip_id = CHIP_NONE;

#define Default_fwLen 13
const unsigned char Default_fwData[Default_fwLen] = {0x05, 0x00, 0x02, 0xD0, 0x01,		// W=1280, H=720, NumOfFrmRate=1
										   0xFF, 0xFF, 0xFF, 0xFF,			// Frame size
										   0x07, 0xA1, 0xFF, 0xFF,			// 20
											};

#define LENGTH_OF_SONIX_XU_SYS_CTR		(7)
#define LENGTH_OF_SONIX_XU_USR_CTR		(7)
#define SONIX_SN9C291_SERIES_CHIPID 	0x90
#define SONIX_SN9C292_SERIES_CHIPID		0x92
#define SONIX_SN9C292_DDR_64M			0x00
#define SONIX_SN9C292_DDR_16M			0x03

static struct uvc_xu_control_info sonix_xu_sys_ctrls[] = 
{
	{
		.entity   = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector = XU_SONIX_SYS_ASIC_RW,
		.index    = 0,
		.size     = 4,
		.flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX | 
		            UVC_CONTROL_GET_DEF | UVC_CONTROL_AUTO_UPDATE | UVC_CONTROL_GET_CUR
	},
	{
		.entity   = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector = XU_SONIX_SYS_FRAME_INFO,
		.index    = 1,
		.size     = 11,
		.flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX | 
		            UVC_CONTROL_GET_DEF | UVC_CONTROL_AUTO_UPDATE | UVC_CONTROL_GET_CUR
	},
	{
		.entity   = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector = XU_SONIX_SYS_H264_CTRL,
		.index    = 2,
		.size     = 11,
		.flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
	},
	{
		.entity   = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector = XU_SONIX_SYS_MJPG_CTRL,
		.index    = 3,
		.size     = 11,
		.flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
	},
	{
		.entity   = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector = XU_SONIX_SYS_OSD_CTRL,
		.index    = 4,
		.size     = 11,
		.flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX | 
		            UVC_CONTROL_GET_DEF | UVC_CONTROL_AUTO_UPDATE | UVC_CONTROL_GET_CUR
	},
	{
		.entity   = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector = XU_SONIX_SYS_MOTION_DETECTION,
		.index    = 5,
		.size     = 11,
		.flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
	},
	{
		.entity   = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector = XU_SONIX_SYS_IMG_SETTING,
		.index    = 6,
		.size     = 11,
		.flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
	},
};

static struct uvc_xu_control_info sonix_xu_usr_ctrls[] =
{
    {
      .entity   = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector = XU_SONIX_USR_FRAME_INFO,
      .index    = 0,
      .size     = 11,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX |
                  UVC_CONTROL_GET_DEF | UVC_CONTROL_AUTO_UPDATE | UVC_CONTROL_GET_CUR
    },
    {
      .entity   = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector = XU_SONIX_USR_H264_CTRL,
      .index    = 1,
      .size     = 11,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
    },
    {
      .entity   = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector = XU_SONIX_USR_MJPG_CTRL,
      .index    = 2,
      .size     = 11,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
    },
    {
      .entity   = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector = XU_SONIX_USR_OSD_CTRL,
      .index    = 3,
      .size     = 11,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX |
                  UVC_CONTROL_GET_DEF | UVC_CONTROL_AUTO_UPDATE | UVC_CONTROL_GET_CUR
    },
    {
      .entity   = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector = XU_SONIX_USR_MOTION_DETECTION,
      .index    = 4,
      .size     = 24,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
    },
    {
      .entity   = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector = XU_SONIX_USR_IMG_SETTING,
      .index    = 5,
      .size     = 11,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
    },
    {
      .entity   = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector = XU_SONIX_USR_MULTI_STREAM_CTRL,
      .index    = 6,
      .size     = 11,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
    },	
};

//  SONiX XU system Ctrls Mapping
static struct uvc_xu_control_mapping sonix_xu_sys_mappings[] = 
{
	{
		.id        = V4L2_CID_ASIC_RW_SONIX,
		.name      = "SONiX: Asic Read",
		.entity    = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector  = XU_SONIX_SYS_ASIC_RW,
		.size      = 4,
		.offset    = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = UVC_CTRL_DATA_TYPE_SIGNED
	},
	{
		.id        = V4L2_CID_FRAME_INFO_SONIX,
		.name      = "SONiX: H264 Format",
		.entity    = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector  = XU_SONIX_SYS_FRAME_INFO,
		.size      = 11,
		.offset    = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = UVC_CTRL_DATA_TYPE_RAW
	},
	{
		.id        = V4L2_CID_H264_CTRL_SONIX,
		.name      = "SONiX: H264 Control",
		.entity    = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector  = XU_SONIX_SYS_H264_CTRL,
		.size      = 11,
		.offset    = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = UVC_CTRL_DATA_TYPE_UNSIGNED
	},
	{
		.id        = V4L2_CID_MJPG_CTRL_SONIX,
		.name      = "SONiX: MJPG Control",
		.entity    = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector  = XU_SONIX_SYS_MJPG_CTRL,
		.size      = 11,
		.offset    = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = UVC_CTRL_DATA_TYPE_UNSIGNED
	},
	{
		.id        = V4L2_CID_OSD_CTRL_SONIX,
		.name      = "SONiX: OSD Control",
		.entity    = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector  = XU_SONIX_SYS_OSD_CTRL,
		.size      = 11,
		.offset    = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = UVC_CTRL_DATA_TYPE_RAW
	},
	{
		.id        = V4L2_CID_MOTION_DETECTION_SONIX,
		.name      = "SONiX: Motion Detection",
		.entity    = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector  = XU_SONIX_SYS_MOTION_DETECTION,
		.size      = 11,
		.offset    = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = UVC_CTRL_DATA_TYPE_UNSIGNED
	},
	{
		.id        = V4L2_CID_IMG_SETTING_SONIX,
		.name      = "SONiX: Image Setting",
		.entity    = UVC_GUID_SONIX_SYS_HW_CTRL,
		.selector  = XU_SONIX_SYS_IMG_SETTING,
		.size      = 11,
		.offset    = 0,
		.v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		.data_type = UVC_CTRL_DATA_TYPE_UNSIGNED
	},
};

// SONiX XU user Ctrls Mapping
static struct uvc_xu_control_mapping sonix_xu_usr_mappings[] =
{
    {
      .id        = V4L2_CID_FRAME_INFO_SONIX,
      .name      = "SONiX: H264 Format",
      .entity    = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector  = XU_SONIX_USR_FRAME_INFO,
      .size      = 11,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_RAW
    },
    {
      .id        = V4L2_CID_H264_CTRL_SONIX,
      .name      = "SONiX: H264 Control",
      .entity    = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector  = XU_SONIX_USR_H264_CTRL,
      .size      = 11,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_UNSIGNED
    },
    {
      .id        = V4L2_CID_MJPG_CTRL_SONIX,
      .name      = "SONiX: MJPG Control",
      .entity    = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector  = XU_SONIX_USR_MJPG_CTRL,
      .size      = 11,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_UNSIGNED
    },
    {
      .id        = V4L2_CID_OSD_CTRL_SONIX,
      .name      = "SONiX: OSD Control",
      .entity    = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector  = XU_SONIX_USR_OSD_CTRL,
      .size      = 11,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_RAW
    },
    {
      .id        = V4L2_CID_MOTION_DETECTION_SONIX,
      .name      = "SONiX: Motion Detection",
      .entity    = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector  = XU_SONIX_USR_MOTION_DETECTION,
      .size      = 24,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_UNSIGNED
    },
    {
      .id        = V4L2_CID_IMG_SETTING_SONIX,
      .name      = "SONiX: Image Setting",
      .entity    = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector  = XU_SONIX_USR_IMG_SETTING,
      .size      = 11,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_UNSIGNED
    },
    {
      .id        = V4L2_CID_MULTI_STREAM_CTRL_SONIX,
      .name      = "SONiX: Multi Stram Control ",
      .entity    = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector  = XU_SONIX_USR_MULTI_STREAM_CTRL,
      .size      = 11,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_UNSIGNED
    },	
};
// Houston 2011/08/08 XU ctrls ----------------------------------------------------------

int XU_Ctrl_Add(int fd, struct uvc_xu_control_info *info, struct uvc_xu_control_mapping *map) 
{
	int i=0;
	int err=0;
	
	/* try to add controls listed */
	LOGV("Adding XU Ctrls - %s\n", map->name);
	if ((err=ioctl(fd, UVCIOC_CTRL_ADD, info)) < 0 ) 
	{
		if ((errno == EEXIST) || (errno != EACCES)) 
		{	
			LOGV("UVCIOC_CTRL_ADD - Ignored, uvc driver had already defined\n");
			return (-EEXIST);
		}
		else if (errno == EACCES)
		{
			LOGV("Need admin previledges for adding extension unit(XU) controls\n");
			LOGV("please run 'SONiX_UVC_TestAP --add_ctrls' as root (or with sudo)\n");
			return  (-1);
		}
		else perror("Control exists");
	}
	
	/* after adding the controls, add the mapping now */
	LOGV("Mapping XU Ctrls - %s\n", map->name);
	if ((err=ioctl(fd, UVCIOC_CTRL_MAP, map)) < 0) 
	{
		if ((errno!=EEXIST) || (errno != EACCES))
		{
			LOGV("UVCIOC_CTRL_MAP - Error");
			return (-2);
		}
		else if (errno == EACCES)
		{
			LOGV("Need admin previledges for adding extension unit(XU) controls\n");
			LOGV("please run 'SONiX_UVC_TestAP --add_ctrls' as root (or with sudo)\n");
			return  (-1);
		}
		else perror("Mapping exists");
	}
	
	return 0;
}

int XU_Init_Ctrl(int fd) 
{
	int i=0;
	int err=0;
	int length;
	struct uvc_xu_control_info *xu_infos;
	struct uvc_xu_control_mapping *xu_mappings;
	
	// Add xu READ ASIC first
	err = XU_Ctrl_Add(fd, &sonix_xu_sys_ctrls[i], &sonix_xu_sys_mappings[i]);
	if ((err == EEXIST) || (err != EACCES)){}
	else if (err < 0) {return err;}

	// Read chip ID
	err = XU_Ctrl_ReadChipID(fd);
	if (err < 0) return err;

	// Decide which xu set had been add
	if(chip_id == CHIP_SNC291A)
	{
		xu_infos = sonix_xu_sys_ctrls;
		xu_mappings = sonix_xu_sys_mappings;
		i = 1;
		length = LENGTH_OF_SONIX_XU_SYS_CTR;
		LOGV("SN9C291\n");
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		xu_infos = sonix_xu_usr_ctrls;
		xu_mappings = sonix_xu_usr_mappings;
		i = 0;
		length = LENGTH_OF_SONIX_XU_USR_CTR;
		LOGV("SN9C292\n");
	}
	else
	{
		LOGV("Unknown chip id 0x%x\n", chip_id);
		return -1;
	}
	
	// Add other xu accroding chip ID
	for ( ; i<length; i++ ) 
	{
		err = XU_Ctrl_Add(fd, &xu_infos[i], &xu_mappings[i]);
		if (err < 0) break;
	} 
	return 0;
}

int XU_Ctrl_ReadChipID(int fd)
{
	LOGV("XU_Ctrl_ReadChipID ==>\n");
	int ret = 0;
	int err = 0;
	__u8 ctrldata[4];

	struct uvc_xu_control xctrl = {
		3,										/* bUnitID 				*/
		XU_SONIX_SYS_ASIC_RW,					/* function selector	*/
		4,										/* data size			*/
		(__u8*)&ctrldata								/* *data				*/
		};
	
	xctrl.data[0] = 0x1f;
	xctrl.data[1] = 0x10;
	xctrl.data[2] = 0x0;
	xctrl.data[3] = 0xFF;		/* Dummy Write */
	
	/* Dummy Write */
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("  ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		return err;
	}

	/* Asic Read */
	xctrl.data[3] = 0x00;
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) 
	{
		LOGV("   ioctl(UVCIOC_CTRL_GET) FAILED (%i)  \n",err);
		if(err==EINVAL)
			LOGV("    Invalid arguments\n");
		return err;
	}
	
	//LOGV("   == XU_Ctrl_ReadChipID Success == \n");
	LOGV("      ASIC READ data[0] : %x\n", xctrl.data[0]);
	LOGV("      ASIC READ data[1] : %x\n", xctrl.data[1]);
	LOGV("      ASIC READ data[2] : %x (Chip ID)\n", xctrl.data[2]);
	LOGV("      ASIC READ data[3] : %x\n", xctrl.data[3]);
	
	if(xctrl.data[2] == SONIX_SN9C291_SERIES_CHIPID)
	{
		chip_id = CHIP_SNC291A;
	}	
	if(xctrl.data[2] == SONIX_SN9C292_SERIES_CHIPID)
	{
		xctrl.data[0] = 0x07;		//DRAM SIZE
		xctrl.data[1] = 0x16;
		xctrl.data[2] = 0x0;
		xctrl.data[3] = 0xFF;		/* Dummy Write */

		/* Dummy Write */
		if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
		{
			LOGV("  ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
			return err;
		}

		/* Asic Read */
		xctrl.data[3] = 0x00;
		if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) 
		{
			LOGV("   ioctl(UVCIOC_CTRL_GET) FAILED (%i)  \n",err);
			if(err==EINVAL)
				LOGV("    Invalid arguments\n");
			return err;
		}
		
		if(xctrl.data[2] == SONIX_SN9C292_DDR_64M)
			chip_id = CHIP_SNC292A;
		else if(xctrl.data[2] == SONIX_SN9C292_DDR_16M)
			chip_id = CHIP_SNC291B;			
	}

	if (chip_id == CHIP_NONE) {
		LOGW("XU_Ctrl_ReadChipID Error");
		return -1;				
	}
	
	LOGV("ChipID = %d\n",chip_id);
	LOGV("XU_Ctrl_ReadChipID <==\n");
	return ret;
}

int H264_GetFormat(int fd)
{
	LOGV("H264_GetFormat ==>\n");
	int i,j;
	int iH264FormatCount = 0;
	int success = 1;
	
	unsigned char *fwData = NULL;
	unsigned short fwLen = 0;

	// Init H264 XU Ctrl Format
	if( XU_H264_InitFormat(fd) < 0 )
	{
		LOGV(" H264 XU Ctrl Format failed , use default Format\n");
		fwLen = Default_fwLen;
		fwData = (unsigned char *)calloc(fwLen,sizeof(unsigned char));
		memcpy(fwData, Default_fwData, fwLen);
		goto Skip_XU_GetFormat;
	}

	// Probe : Get format through XU ctrl
	success = XU_H264_GetFormatLength(fd, &fwLen);
	if( success < 0 || fwLen <= 0)
	{
		LOGV(" XU Get Format Length failed !\n");
	}
	LOGV("fwLen = 0x%x\n", fwLen);
	
	// alloc memory
	fwData = (unsigned char *)calloc(fwLen,sizeof(unsigned char));

	if( XU_H264_GetFormatData(fd, fwData,fwLen) < 0 )
	{
		LOGV(" XU Get Format Data failed !\n");
	}

Skip_XU_GetFormat :

	// Get H.264 format count
	iH264FormatCount = H264_CountFormat(fwData, fwLen);

	LOGV("H264_GetFormat ==> FormatCount : %d \n", iH264FormatCount);

	if(iH264FormatCount>0)
		gH264fmt = (struct H264Format *)malloc(sizeof(struct H264Format)*iH264FormatCount);
	else
	{
		LOGV("Get Resolution Data Failed\n");
	}

	// Parse & Save Size/Framerate into structure
	success = H264_ParseFormat(fwData, fwLen, gH264fmt);

	if(success)
	{
		for(i=0; i<iH264FormatCount; i++)
		{
			LOGV("Format index: %d --- (%d x %d) ---\n", i+1, gH264fmt[i].wWidth, gH264fmt[i].wHeight);
			for(j=0; j<gH264fmt[i].fpsCnt; j++)
			{
				if(chip_id == CHIP_SNC291A)
				{
					LOGV("(%d) %2d fps\n", j+1, H264_GetFPS(gH264fmt[i].FrPay[j]));
				}
				else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
				{
					LOGV("(%d) %2d fps\n", j+1, H264_GetFPS(gH264fmt[i].FrPay[j*2]));
				}
			}
		}
	}

	if(fwData)
	{
		free(fwData);
		fwData = NULL;
	}

	LOGV("H264_GetFormat <== \n");
	return success;
}

int H264_ReleaseFormatBuffer(void){

	if(gH264fmt)
	{
		free(gH264fmt);
		gH264fmt = NULL;
	}
	
	return 0;
}

int H264_CountFormat(unsigned char *Data, int len)
{
	int fmtCnt = 0;
	int fpsCnt = 0;
	int cur_len = 0;
	int cur_fmtid = 0;
	int cur_fpsNum = 0;
	
	if( Data == NULL || len == 0)
		return 0;

	// count Format numbers
	while(cur_len < len)
	{
		cur_fpsNum = Data[cur_len+4];
		
		LOGV("H264_CountFormat ==> cur_len = %d, cur_fpsNum= %d\n", cur_len , cur_fpsNum);

		if(chip_id == CHIP_SNC291A)
		{
			cur_len += 9 + cur_fpsNum * 4;
		}
		else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
		{
			cur_len += 9 + cur_fpsNum * 6;
		}

		fmtCnt++;
	}
	
	if(cur_len != len)
	{
		LOGV("H264_CountFormat ==> cur_len = %d, fwLen= %d\n", cur_len , len);
		return 0;
	}
	
	LOGV ("  ========  fmtCnt=%d   ======== \n",fmtCnt);
	return fmtCnt;
}
int H264_ParseFormat(unsigned char *Data, int len, struct H264Format *fmt)
{
	LOGV("H264_ParseFormat ==>\n");
	int fpsCnt = 0;
	int cur_len = 0;
	int cur_fmtid = 0;
	int cur_fpsNum = 0;
	int i;

	while(cur_len < len)
	{
		// Copy Size
		fmt[cur_fmtid].wWidth  = ((unsigned short)Data[cur_len]<<8)   + (unsigned short)Data[cur_len+1];
		fmt[cur_fmtid].wHeight = ((unsigned short)Data[cur_len+2]<<8) + (unsigned short)Data[cur_len+3];
		fmt[cur_fmtid].fpsCnt  = Data[cur_len+4];
		fmt[cur_fmtid].FrameSize =	((unsigned int)Data[cur_len+5] << 24) | 
						((unsigned int)Data[cur_len+6] << 16) | 
						((unsigned int)Data[cur_len+7] << 8 ) | 
						((unsigned int)Data[cur_len+8]);

		LOGV("Data[5~8]: 0x%02x%02x%02x%02x \n", Data[cur_len+5],Data[cur_len+6],Data[cur_len+7],Data[cur_len+8]);
		LOGV("fmt[%d].FrameSize: 0x%08x \n", cur_fmtid, fmt[cur_fmtid].FrameSize);

		// Alloc memory for Frame rate 
		cur_fpsNum = Data[cur_len+4];
		
		if(chip_id == CHIP_SNC291A)
		{
			fmt[cur_fmtid].FrPay = (unsigned int *)malloc(sizeof(unsigned int)*cur_fpsNum);
		}
		else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
		{
			fmt[cur_fmtid].FrPay = (unsigned int *)malloc(sizeof(unsigned int)*cur_fpsNum*2);
		}
		
		for(i=0; i<cur_fpsNum; i++)
		{
			if(chip_id == CHIP_SNC291A)
			{
				fmt[cur_fmtid].FrPay[i] =	(unsigned int)Data[cur_len+9+i*4]   << 24 | 
								(unsigned int)Data[cur_len+9+i*4+1] << 16 |
								(unsigned int)Data[cur_len+9+i*4+2] << 8  |
								(unsigned int)Data[cur_len+9+i*4+3] ;
			
			//LOGV("fmt[cur_fmtid].FrPay[%d]: 0x%08x \n", i, fmt[cur_fmtid].FrPay[i]);
			}
			else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
			{
				fmt[cur_fmtid].FrPay[i*2] =	(unsigned int)Data[cur_len+9+i*6]   << 8 | (unsigned int)Data[cur_len+9+i*6+1];
				fmt[cur_fmtid].FrPay[i*2+1] =	(unsigned int)Data[cur_len+9+i*6+2]   << 24 | 
								(unsigned int)Data[cur_len+9+i*6+3] << 16 |
								(unsigned int)Data[cur_len+9+i*6+4] << 8  |
								(unsigned int)Data[cur_len+9+i*6+5] ;

				LOGV("fmt[cur_fmtid].FrPay[%d]: 0x%04x  0x%08x \n", i, fmt[cur_fmtid].FrPay[i*2], fmt[cur_fmtid].FrPay[i*2+1]);
			}
		}
		
		// Do next format
		if(chip_id == CHIP_SNC291A)
		{
			cur_len += 9 + cur_fpsNum * 4;
		}
		else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
		{
			cur_len += 9 + cur_fpsNum * 6;
		}
		cur_fmtid++;
	}
	if(cur_len != len)
	{
		LOGV("H264_ParseFormat <==  fail \n");
		return 0;
	}

	LOGV("H264_ParseFormat <==\n");
	return 1;
}

int H264_GetFPS(unsigned int FrPay)
{
	int fps = 0;

	if(chip_id == CHIP_SNC291A)
	{
		//LOGV("H264_GetFPS==> FrPay = 0x%04x\n", (FrPay & 0xFFFF0000)>>16);

		unsigned short frH = (FrPay & 0xFF000000)>>16;
		unsigned short frL = (FrPay & 0x00FF0000)>>16;
		unsigned short fr = (frH|frL);
	
		//LOGV("FrPay: 0x%x -> fr = 0x%x\n",FrPay,fr);
	
		fps = ((unsigned int)10000000/fr)>>8;

		//LOGV("fps : %d\n", fps);
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		//LOGV("H264_GetFPS==> Fr = 0x%04x\n", (unsigned short)FrPay);

		fps = ((unsigned int)10000000/(unsigned short)FrPay)>>8;

		//LOGV("fps : %d\n", fps);
	}

	return fps;
}


int XU_H264_InitFormat(int fd)
{
	LOGV("XU_H264_InitFormat ==>\n");
	int i = 0;
	int ret = 0;
	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		0,										/* bUnitID 				*/
		0,										/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata								/* *data				*/
		};

	if(chip_id == CHIP_SNC291A)
	{
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_FRAME_INFO;
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_FRAME_INFO;
	}
		
	// Switch command : FMT_NUM_INFO
	// xctrl.data[0] = 0x01;
	xctrl.data[0] = 0x9A;
	xctrl.data[1] = 0x01;
	
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("   Set Switch command : FMT_NUM_INFO FAILED (%i)\n",err);
		return err;
	}
	
	//xctrl.data[0] = 0;
	memset(xctrl.data, 0, xctrl.size);
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0)
	{	
		LOGV("   ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		return err;
	}
	
	for(i=0; i<xctrl.size; i++)
		LOGV(" Get Data[%d] = 0x%x\n",i, xctrl.data[i]);
	LOGV(" ubH264Idx_S1 = %d\n", xctrl.data[5]);

	
	LOGV("XU_H264_InitFormat <== Success\n");
	return ret;
}

int XU_H264_GetFormatLength(int fd, unsigned short *fwLen)
{
	LOGV("XU_H264_GetFormatLength ==>\n");
	int i = 0;
	int ret = 0;
	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		0,										/* bUnitID 				*/
		0,										/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata								/* *data				*/
		};

	if(chip_id == CHIP_SNC291A)
	{
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_FRAME_INFO;
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_FRAME_INFO;
	}	
		
	// Switch command : FMT_Data Length_INFO
	xctrl.data[0] = 0x9A;
	xctrl.data[1] = 0x02;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) >= 0) 
	{
		//for(i=0; i<11; i++)	xctrl.data[i] = 0;		// clean data
		memset(xctrl.data, 0, xctrl.size);

		if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) >= 0)
		{
			for (i=0 ; i<11 ; i+=2)
				LOGV(" Get Data[%d] = 0x%x\n", i, (xctrl.data[i]<<8)+xctrl.data[i+1]);

			// Get H.264 format length
			*fwLen = ((unsigned short)xctrl.data[6]<<8) + xctrl.data[7];
			LOGV(" H.264 format Length = 0x%x\n", *fwLen);
		}
		else
		{
			LOGV("   ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
			return err;
		}
	}
	else
	{
		LOGV("   Set Switch command : FMT_Data Length_INFO FAILED (%i)\n",err);
		return err;
	}
	
	LOGV("XU_H264_GetFormatLength <== Success\n");
	return ret;
}

int XU_H264_GetFormatData(int fd, unsigned char *fwData, unsigned short fwLen)
{
	LOGV("XU_H264_GetFormatData ==>\n");
	int i = 0;
	int ret = 0;
	int err = 0;
	__u8 ctrldata[11]={0};

	int loop = 0;
	int LoopCnt  = (fwLen%11) ? (fwLen/11)+1 : (fwLen/11) ;
	int Copyleft = fwLen;

	struct uvc_xu_control xctrl = {
		0,										/* bUnitID 				*/
		0,										/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata								/* *data				*/
		};

	if(chip_id == CHIP_SNC291A)
	{
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_FRAME_INFO;
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_FRAME_INFO;
	}
	
	// Switch command
	xctrl.data[0] = 0x9A;		
	xctrl.data[1] = 0x03;		// FRM_DATA_INFO
	
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0)
	{
		LOGV("   Set Switch command : FRM_DATA_INFO FAILED (%i)\n",err);
		return err;
	}

	// Get H.264 Format Data
	xctrl.data[0] = 0x02;		// Stream: 1
	xctrl.data[1] = 0x01;		// Format: 1 (H.264)

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) >= 0) 
	{
		// Read all H.264 format data
		for(loop = 0 ; loop < LoopCnt ; loop ++)
		{
			for(i=0; i<11; i++)	xctrl.data[i] = 0;		// clean data
			
			LOGV("--> Loop : %d <--\n",  loop);
			
			if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) >= 0)
			{
				for (i=0 ; i<11 ; i++)
					LOGV(" Data[%d] = 0x%x\n", i, xctrl.data[i]);

				// Copy Data
				if(Copyleft >= 11)
				{
					memcpy( &fwData[loop*11] , xctrl.data, 11);
					Copyleft -= 11;
				}
				else
				{
					memcpy( &fwData[loop*11] , xctrl.data, Copyleft);
					Copyleft = 0;
				}
			}
			else
			{
				LOGV("   ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
				return err;
			}
		}
	}
	else
	{
		LOGV("   Set Switch command : FRM_DATA_INFO FAILED (%i)\n",err);
		return err;
	}
	
	LOGV("XU_H264_GetFormatData <== Success\n");
	return ret;
}

int XU_H264_SetFormat(int fd, struct Cur_H264Format fmt)
{
	// Need to get H264 format first
	if(gH264fmt==NULL)
	{
		LOGV("SONiX_UVC_TestAP @XU_H264_SetFormat : Do XU_H264_GetFormat before setting H264 format\n");
		return -EINVAL;
	}

	if(chip_id == CHIP_SNC291A)
	{
		LOGV("XU_H264_SetFormat ==> %d-%d => (%d x %d):%d fps\n", 
				fmt.FmtId+1, fmt.FrameRateId+1, 
				gH264fmt[fmt.FmtId].wWidth, 
				gH264fmt[fmt.FmtId].wHeight, 
				H264_GetFPS(gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId]));
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		LOGV("XU_H264_SetFormat ==> %d-%d => (%d x %d):%d fps\n", 
				fmt.FmtId+1, fmt.FrameRateId+1, 
				gH264fmt[fmt.FmtId].wWidth, 
				gH264fmt[fmt.FmtId].wHeight, 
				H264_GetFPS(gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId*2]));
	}

	int ret = 0;
	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
			0,										/* bUnitID 				*/
			0,										/* function selector	*/
			11,										/* data size			*/
			(__u8*)&ctrldata								/* *data				*/
		};

	if(chip_id == CHIP_SNC291A)
	{
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_FRAME_INFO;
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_FRAME_INFO;
	}
		
	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x21;				// Commit_INFO
	
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_H264_Set_FMT ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Set H.264 format setting data
	xctrl.data[0] = 0x02;				// Stream : 1
	xctrl.data[1] = 0x01;				// Format index : 1 (H.264) 
	xctrl.data[2] = fmt.FmtId + 1;		// Frame index (Resolution index), firmware index starts from 1
//	xctrl.data[3] = ( gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId] & 0xFF000000 ) >> 24;	// Frame interval
//	xctrl.data[4] = ( gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId] & 0x00FF0000 ) >> 16;
	xctrl.data[5] = ( gH264fmt[fmt.FmtId].FrameSize & 0x00FF0000) >> 16;
	xctrl.data[6] = ( gH264fmt[fmt.FmtId].FrameSize & 0x0000FF00) >> 8;
	xctrl.data[7] = ( gH264fmt[fmt.FmtId].FrameSize & 0x000000FF);
//	xctrl.data[8] = ( gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId] & 0x0000FF00 ) >> 8;
//	xctrl.data[9] = ( gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId] & 0x000000FF ) ;


	if(chip_id == CHIP_SNC291A)
	{
		xctrl.data[3] = ( gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId] & 0xFF000000 ) >> 24;	// Frame interval
		xctrl.data[4] = ( gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId] & 0x00FF0000 ) >> 16;
		xctrl.data[8] = ( gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId] & 0x0000FF00 ) >> 8;
		xctrl.data[9] = ( gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId] & 0x000000FF ) ;
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		xctrl.data[3] = ( gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId*2] & 0x0000FF00 ) >> 8;	// Frame interval
		xctrl.data[4] = ( gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId*2] & 0x000000FF ) ;
		xctrl.data[8] = ( gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId*2+1] & 0xFF000000 ) >> 24;
		xctrl.data[9] = ( gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId*2+1] & 0x00FF0000 ) >> 16;
		xctrl.data[10] = ( gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId*2+1] & 0x0000FF00 ) >> 8;
	}	

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_H264_Set_FMT ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		memset(xctrl.data, 0, xctrl.size);
		xctrl.data[0] = ( gH264fmt[fmt.FmtId].FrPay[fmt.FrameRateId*2+1] & 0x000000FF ) ;

		if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
		{
			LOGV("XU_H264_Set_FMT____2 ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
			if(err==EINVAL)
				LOGV("Invalid arguments\n");
			return err;
		}
	}	

	LOGV("XU_H264_SetFormat <== Success \n");
	return ret;

}


int XU_H264_Get_Mode(int fd, int *Mode)
{	
	LOGV("XU_H264_Get_Mode ==>\n");
	int i = 0;
	int ret = 0;
	int err = 0;
	__u8 ctrldata[11]={0};


	struct uvc_xu_control xctrl = {
			0,										/* bUnitID 				*/
			0,										/* function selector	*/
			11,										/* data size			*/
			(__u8*)&ctrldata								/* *data				*/
		};

	if(chip_id == CHIP_SNC291A)
	{
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_H264_CTRL;
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x01;				// H264_ctrl_type
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_H264_CTRL;
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x06;				// H264_mode
	}
	
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_H264_Get_Mode ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	// Get mode
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) 
	{
		LOGV("XU_H264_Get_Mode ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	LOGV("   == XU_H264_Get_Mode Success == \n");
	LOGV("      Get data[%d] : 0x%x\n", 0, xctrl.data[0]);

	*Mode = xctrl.data[0];
	
	LOGV("XU_H264_Get_Mode (%s)<==\n", *Mode==1?"CBR mode":(*Mode==2?"VBR mode":"error"));
	return ret;
}

int XU_H264_Set_Mode(int fd, int Mode)
{	
	LOGV("XU_H264_Set_Mode (0x%x) ==>\n", Mode);
	int ret = 0;
	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
			0,										/* bUnitID 				*/
			0,										/* function selector	*/
			11,										/* data size			*/
			(__u8*)&ctrldata								/* *data				*/
		};

	if(chip_id == CHIP_SNC291A)
	{
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_H264_CTRL;
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x01;				// H264_ctrl_type
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_H264_CTRL;
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x06;				// H264_mode
	}
	
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_H264_Set_Mode ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	

	// Set CBR/VBR Mode
	memset(xctrl.data, 0, xctrl.size);
	xctrl.data[0] = Mode;
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_H264_Set_Mode ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	LOGV("XU_H264_Set_Mode <== Success \n");
	return ret;

}



int XU_H264_Get_QP_Limit(int fd, int *QP_Min, int *QP_Max)
{	
	LOGV("XU_H264_Get_QP_Limit ==>\n");
	int i = 0;
	int ret = 0;
	int err = 0;
	__u8 ctrldata[11]={0};


	struct uvc_xu_control xctrl = {
			0,										/* bUnitID 				*/
			0,										/* function selector	*/
			11,										/* data size			*/
			(__u8*)&ctrldata								/* *data				*/
		};

	if(chip_id == CHIP_SNC291A)
	{
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_H264_CTRL;
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x02;				// H264_limit
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_H264_CTRL;
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x01;				// H264_limit
	}


	
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_H264_Get_QP_Limit ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	memset(xctrl.data, 0, xctrl.size);
	// Get QP value
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) 
	{
		LOGV("XU_H264_Get_QP_Limit ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	LOGV("   == XU_H264_Get_QP_Limit Success == \n");
	for(i=0; i<2; i++)
			LOGV("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

	*QP_Min = xctrl.data[0];
	*QP_Max = xctrl.data[1];
	LOGV("XU_H264_Get_QP_Limit (0x%x, 0x%x)<==\n", *QP_Min, *QP_Max);
	return ret;

}

int XU_H264_Get_QP(int fd, int *QP_Val)
{	
	LOGV("XU_H264_Get_QP ==>\n");
	int i = 0;
	int ret = 0;
	int err = 0;
	__u8 ctrldata[11]={0};
	*QP_Val = -1;

	if(chip_id == CHIP_SNC291A)
	{
		int qp_min, qp_max;
		XU_H264_Get_QP_Limit(fd, &qp_min, &qp_max);	
	}

	struct uvc_xu_control xctrl = {
			0,										/* bUnitID 				*/
			0,										/* function selector	*/
			11,										/* data size			*/
			(__u8*)&ctrldata								/* *data				*/
		};

	if(chip_id == CHIP_SNC291A)
	{
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_H264_CTRL;
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x05;				// H264_VBR_QP
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_H264_CTRL;
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x07;				// H264_QP
	}
	
	
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_H264_Get_QP ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	// Get QP value
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) 
	{
		LOGV("XU_H264_Get_QP ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	LOGV("   == XU_H264_Get_QP Success == \n");
	LOGV("      Get data[%d] : 0x%x\n", 0, xctrl.data[0]);

	*QP_Val = xctrl.data[0];
	
	LOGV("XU_H264_Get_QP (0x%x)<==\n", *QP_Val);
	return ret;

}

int XU_H264_Set_QP(int fd, int QP_Val)
{	
	LOGV("XU_H264_Set_QP (0x%x) ==>\n", QP_Val);
	int ret = 0;
	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
			0,										/* bUnitID 				*/
			0,										/* function selector	*/
			11,										/* data size			*/
			(__u8*)&ctrldata								/* *data				*/
		};

	if(chip_id == CHIP_SNC291A)
	{
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_H264_CTRL;
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x05;				// H264_VBR_QP
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_H264_CTRL;
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x07;				// H264_QP	
	}
	
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_H264_Get_QP ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	

	// Set QP value
	memset(xctrl.data, 0, xctrl.size);
	xctrl.data[0] = QP_Val;
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_H264_Set_QP ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	LOGV("XU_H264_Set_QP <== Success \n");
	return ret;

}

int XU_H264_Get_BitRate(int fd, double *BitRate)
{
	LOGV("XU_H264_Get_BitRate ==>\n");
	int i = 0;
	int ret = 0;
	int err = 0;
	__u8 ctrldata[11]={0};
	int BitRate_CtrlNum = 0;
	*BitRate = -1.0;

	struct uvc_xu_control xctrl = {
			0,										/* bUnitID 				*/
			0,										/* function selector	*/
			11,										/* data size			*/
			(__u8*)&ctrldata								/* *data				*/
		};

	if(chip_id == CHIP_SNC291A)
	{
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_H264_CTRL;
		
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x03;				// H264_BitRate
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_H264_CTRL;

		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x02;				// H264_BitRate
	}

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_H264_Get_BitRate ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	// Get Bit rate ctrl number
	memset(xctrl.data, 0, xctrl.size);
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) 
	{
		LOGV("XU_H264_Get_BitRate ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	LOGV("   == XU_H264_Get_BitRate Success == \n");
	
	if(chip_id == CHIP_SNC291A)
	{
		for(i=0; i<2; i++)
			LOGV("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

		BitRate_CtrlNum = ( xctrl.data[0]<<8 )| (xctrl.data[1]) ;

		// Bit Rate = BitRate_Ctrl_Num*512*fps*8 /1000(Kbps)
		*BitRate = (double)(BitRate_CtrlNum*512.0*m_CurrentFPS*8)/1000.0;
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		for(i=0; i<3; i++)
			LOGV("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

		BitRate_CtrlNum =  ( xctrl.data[0]<<16 )| ( xctrl.data[1]<<8 )| (xctrl.data[2]) ;

		*BitRate = BitRate_CtrlNum;
	}	
	
	LOGV("XU_H264_Get_BitRate (%.2f)<==\n", *BitRate);
	return ret;
}

int XU_H264_Set_BitRate(int fd, double BitRate)
{	
	LOGV("XU_H264_Set_BitRate (%.2f) ==>\n",BitRate);
	int ret = 0;
	int err = 0;
	__u8 ctrldata[11]={0};
	int BitRate_CtrlNum = 0;

	struct uvc_xu_control xctrl = {
			0,										/* bUnitID 				*/
			0,										/* function selector	*/
			11,										/* data size			*/
			(__u8*)&ctrldata								/* *data				*/
		};

	if(chip_id == CHIP_SNC291A)
	{
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_H264_CTRL;

		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x03;				// H264_BitRate
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_H264_CTRL;

		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x02;				// H264_BitRate
	}
	
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_H264_Set_BitRate ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Set Bit Rate Ctrl Number
	if(chip_id == CHIP_SNC291A)
	{
		// Bit Rate = BitRate_Ctrl_Num*512*fps*8/1000 (Kbps)
		BitRate_CtrlNum = (int)((BitRate*1000)/(512*m_CurrentFPS*8));
		xctrl.data[0] = (BitRate_CtrlNum & 0xFF00)>>8;	// BitRate ctrl Num
		xctrl.data[1] = (BitRate_CtrlNum & 0x00FF);
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		// Bit Rate = BitRate_Ctrl_Num*512*fps*8/1000 (Kbps)
		xctrl.data[0] = ((int)BitRate & 0x00FF0000)>>16;
		xctrl.data[1] = ((int)BitRate & 0x0000FF00)>>8;
		xctrl.data[2] = ((int)BitRate & 0x000000FF);
	}	
	
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_H264_Set_BitRate ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	LOGV("XU_H264_Set_BitRate <== Success \n");
	return ret;
}

int XU_H264_Set_IFRAME(int fd)
{	
	LOGV("XU_H264_Set_IFRAME ==>\n");
	int ret = 0;
	int err = 0;
	__u8 ctrldata[11]={0};
	int BitRate_CtrlNum = 0;

	struct uvc_xu_control xctrl = {
			0,										/* bUnitID 				*/
			0,										/* function selector	*/
			11,										/* data size			*/
			(__u8*)&ctrldata								/* *data				*/
		};

	if(chip_id == CHIP_SNC291A)
	{
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_H264_CTRL;

		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x06;				// H264_IFRAME
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_H264_CTRL;

		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x04;				// H264_IFRAME
	}
	
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_H264_Set_IFRAME ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	// Set IFrame reset
	xctrl.data[0] = 1;
	
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_H264_Set_IFRAME ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}	

	LOGV("XU_H264_Set_IFRAME <== Success \n");
	return ret;
}

int XU_H264_Get_SEI(int fd, unsigned char *SEI)
{
	LOGV("XU_H264_Get_SEI ==>\n");
	int err = 0;
	__u8 ctrldata[11]={0};

	if(chip_id == CHIP_SNC291A)
	{
		LOGV(" ==SN9C290 no support get SEI==\n");	
		return 0;
	}
	struct uvc_xu_control xctrl = {
			XU_SONIX_USR_ID,						/* bUnitID 				*/
			XU_SONIX_USR_H264_CTRL,					/* function selector	*/
			11,										/* data size			*/
			(__u8*)&ctrldata								/* *data				*/
		};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x05;				// H264_SEI

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_H264_Get_SEI ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	// Get SEI
	memset(xctrl.data, 0, xctrl.size);
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) 
	{
		LOGV("XU_H264_Get_SEI ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	*SEI = xctrl.data[0];
	
	LOGV("      SEI : 0x%x\n",*SEI);
	LOGV("XU_H264_Get_SEI <== Success \n");

	return 0;
}

int XU_H264_Set_SEI(int fd, unsigned char SEI)
{	
	LOGV("XU_H264_Set_SEI ==>\n");
	int err = 0;
	__u8 ctrldata[11]={0};

	if(chip_id == CHIP_SNC291A)
	{
		LOGV(" ==SN9C290 no support Set SEI==\n");	
		return 0;
	}

	struct uvc_xu_control xctrl = {
			XU_SONIX_USR_ID,						/* bUnitID 				*/
			XU_SONIX_USR_H264_CTRL,					/* function selector	*/
			11,										/* data size			*/
			(__u8*)&ctrldata								/* *data				*/
		};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x05;				// H264_SEI
	
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_H264_Set_SEI ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Set SEI
	xctrl.data[0] = SEI;
	
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_H264_Set_SEI ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	LOGV("XU_H264_Set_SEI <== Success \n");
	return 0;
}

int XU_H264_Get_GOP(int fd, unsigned int *GOP)
{
	LOGV("XU_H264_Get_GOP ==>\n");
	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
			XU_SONIX_USR_ID,						/* bUnitID 				*/
			XU_SONIX_USR_H264_CTRL,					/* function selector	*/
			11,										/* data size			*/
			(__u8*)&ctrldata								/* *data				*/
		};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x03;				// H264_GOP

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_H264_Get_GOP ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	// Get GOP
	memset(xctrl.data, 0, xctrl.size);
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) 
	{
		LOGV("XU_H264_Get_GOP ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	*GOP = (xctrl.data[1] << 8) | xctrl.data[0];
	
	LOGV("      GOP : %d\n",*GOP);
	LOGV("XU_H264_Get_GOP <== Success \n");

	return 0;
}

int XU_H264_Set_GOP(int fd, unsigned int GOP)
{	
	LOGV("XU_H264_Set_GOP ==>\n");
	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
			XU_SONIX_USR_ID,						/* bUnitID 				*/
			XU_SONIX_USR_H264_CTRL,					/* function selector	*/
			11,										/* data size			*/
			(__u8*)&ctrldata								/* *data				*/
		};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x03;				// H264_GOP
	
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_H264_Set_GOP ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Set GOP
	xctrl.data[0] = (GOP & 0xFF);
	xctrl.data[1] = (GOP >> 8) & 0xFF;
	
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_H264_Set_GOP ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	LOGV("XU_H264_Set_GOP <== Success \n");
	return 0;
}

int XU_Get(int fd, struct uvc_xu_control *xctrl)
{
	LOGV("XU Get ==>\n");
	int i = 0;
	int ret = 0;
	int err = 0;

	// XU Set
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, xctrl)) < 0) 
	{
		LOGV("XU Get ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	// XU Get
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, xctrl)) < 0) 
	{
		LOGV("XU Get ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	LOGV("   == XU Get Success == \n");
	for(i=0; i<xctrl->size; i++)
			LOGV("      Get data[%d] : 0x%x\n", i, xctrl->data[i]);
	return ret;
}

int XU_Set(int fd, struct uvc_xu_control xctrl)
{
	LOGV("XU Set ==>\n");
	int i = 0;
	int ret = 0;
	int err = 0;

	// XU Set
	for(i=0; i<xctrl.size; i++)
			LOGV("      Set data[%d] : 0x%x\n", i, xctrl.data[i]);
	
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU Set ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	LOGV("   == XU Set Success == \n");
	return ret;
}

int XU_Asic_Read(int fd, unsigned int Addr, unsigned char *AsicData)
{
	LOGV("XU_Asic_Read ==>\n");
	int ret = 0;
	__u8 ctrldata[4];

	struct uvc_xu_control xctrl = {
		3,										/* bUnitID 				*/
		XU_SONIX_SYS_ASIC_RW,					/* function selector	*/
		4,										/* data size			*/
		(__u8*)&ctrldata								/* *data				*/
		};
	
	xctrl.data[0] = (Addr & 0xFF);
	xctrl.data[1] = ((Addr >> 8) & 0xFF);
	xctrl.data[2] = 0x0;
	xctrl.data[3] = 0xFF;		/* Dummy Write */
	
	/* Dummy Write */
	if ((ret=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("   ioctl(UVCIOC_CTRL_SET) FAILED (%i) \n",ret);
		if(ret==EINVAL)			LOGV("    Invalid arguments\n");		
		return ret;
	}
	
	/* Asic Read */
	xctrl.data[3] = 0x00;
	if ((ret=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) 
	{
		LOGV("   ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",ret);
		if(ret==EINVAL)			LOGV("    Invalid arguments\n");
		return ret;
	}
	*AsicData = xctrl.data[2];
	LOGV("   == XU_Asic_Read Success ==\n");
	LOGV("      Address:0x%x = 0x%x \n", Addr, *AsicData);
	LOGV("XU_Asic_Read <== Success\n");
	return ret;
}

int XU_Asic_Write(int fd, unsigned int Addr, unsigned char AsicData)
{
	LOGV("XU_Asic_Write ==>\n");
	int ret = 0;
	__u8 ctrldata[4];

	struct uvc_xu_control xctrl = {
		3,										/* bUnitID 				*/
		XU_SONIX_SYS_ASIC_RW,					/* function selector	*/
		4,										/* data size			*/
		(__u8*)&ctrldata								/* *data				*/
		};
	
	xctrl.data[0] = (Addr & 0xFF);			/* Addr Low */
	xctrl.data[1] = ((Addr >> 8) & 0xFF);	/* Addr High */
	xctrl.data[2] = AsicData;
	xctrl.data[3] = 0x0;					/* Normal Write */
	
	/* Normal Write */
	if ((ret=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("   ioctl(UVCIOC_CTRL_SET) FAILED (%i) \n",ret);
		if(ret==EINVAL)			LOGV("    Invalid arguments\n");		
	}
	
	LOGV("XU_Asic_Write <== %s\n",(ret<0?"Failed":"Success"));
	return ret;
}

int XU_Multi_Get_status(int fd, struct Multistream_Info *status)
{
	LOGV("XU_Multi_Get_status ==>\n");

	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_MULTI_STREAM_CTRL,			/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};
	
	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x01;				// Multi-Stream Status
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_Multi_Get_status ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Get status
	memset(xctrl.data, 0, xctrl.size);
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) 
	{
		LOGV("XU_Multi_Get_status ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	status->strm_type = xctrl.data[0];
	status->format = ( xctrl.data[1]<<8 )| (xctrl.data[2]) ;

	LOGV("   == XU_Multi_Get_status Success == \n");
	LOGV("      Get strm_type %d\n", status->strm_type);
	LOGV("      Get cur_format %d\n", status->format);

	return 0;
}

int XU_Multi_Get_Info(int fd, struct Multistream_Info *Info)
{	
	LOGV("XU_Multi_Get_Info ==>\n");

	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_MULTI_STREAM_CTRL,			/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x02;				// Multi-Stream Stream Info
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_Multi_Get_Info ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Get Info
	memset(xctrl.data, 0, xctrl.size);
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) 
	{
		LOGV("XU_Multi_Get_Info ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	Info->strm_type = xctrl.data[0];
	Info->format = ( xctrl.data[1]<<8 )| (xctrl.data[2]) ;

	LOGV("   == XU_Multi_Get_Info Success == \n");
	LOGV("      Get Support Stream %d\n", Info->strm_type);
	LOGV("      Get Support Resolution %d\n", Info->format);

	return 0;
}

int XU_Multi_Set_Type(int fd, unsigned int format)
{	
	LOGV("XU_Multi_Set_Type (%d) ==>\n",format);

	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_MULTI_STREAM_CTRL,			/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x02;
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_Multi_Set_Type ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	if(chip_id == CHIP_SNC291B && (format==4 ||format==8 ||format==16))
	{
		LOGV("Invalid arguments\n");
		return -1;
	}
	

	// Set format
	xctrl.data[0] = format;
	
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_Multi_Set_Type ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	LOGV("XU_Multi_Set_Type <== Success \n");
	return 0;

}

int XU_Multi_Set_Enable(int fd, unsigned char enable)
{	
	LOGV("XU_Multi_Set_Enable ==>\n");

	int err = 0;
	__u8 ctrldata[11]={0};
	int BitRate_CtrlNum = 0;

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_MULTI_STREAM_CTRL,			/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x03;				// Enable Multi-Stream Flag

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_Multi_Set_Enable ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Set enable / disable multistream
	xctrl.data[0] = enable;
	xctrl.data[1] = 0;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_Multi_Set_Enable ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	LOGV("Set H264_Multi_Enable = %d \n",(xctrl.data[0] &0x01));
	LOGV("Set MJPG_Multi_Enable = %d \n",((xctrl.data[0] >> 1) &0x01));
	LOGV("XU_Multi_Set_Enable <== Success \n");
	return 0;
}

int XU_Multi_Get_Enable(int fd, unsigned char *enable)
{	
	LOGV("XU_Multi_Get_Enable ==>\n");

	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_MULTI_STREAM_CTRL,			/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x03;				// Enable Multi-Stream Flag
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_Multi_Get_Enable ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Get Enable
	memset(xctrl.data, 0, xctrl.size);
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) 
	{
		LOGV("XU_Multi_Get_Enable ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	*enable = xctrl.data[0];

	LOGV("      Get H264 Multi Stream Enable = %d\n", xctrl.data[0] & 0x01);
	LOGV("      Get MJPG Multi Stream Enable =  %d\n", (xctrl.data[0] >> 1) & 0x01);	
	LOGV("XU_Multi_Get_Enable <== Success\n");
	return 0;
}

#if 0
int XU_Multi_Set_BitRate(int fd, unsigned int BitRate1, unsigned int BitRate2, unsigned int BitRate3)
{	
	LOGV("XU_Multi_Set_BitRate  BiteRate1=%d  BiteRate2=%d  BiteRate3=%d   ==>\n",BitRate1, BitRate2, BitRate3);

	int err = 0;
	__u8 ctrldata[11]={0};
	int BitRate_CtrlNum = 0;

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_MULTI_STREAM_CTRL,			/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;
	xctrl.data[1] = 0x04;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_Multi_Set_BitRate ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Set BitRate1~3  (unit:bps)
	xctrl.data[0] = (BitRate1 >> 16)&0xFF;
	xctrl.data[1] = (BitRate1 >> 8)&0xFF;
	xctrl.data[2] = BitRate1&0xFF;
	xctrl.data[3] = (BitRate2 >> 16)&0xFF;
	xctrl.data[4] = (BitRate2 >> 8)&0xFF;
	xctrl.data[5] = BitRate2&0xFF;
	xctrl.data[6] = (BitRate3 >> 16)&0xFF;
	xctrl.data[7] = (BitRate3 >> 8)&0xFF;
	xctrl.data[8] = BitRate3&0xFF;
	
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_Multi_Set_BitRate ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	LOGV("XU_Multi_Set_BitRate <== Success \n");
	return 0;
}

int XU_Multi_Get_BitRate(int fd)
{	
	LOGV("XU_Multi_Get_BitRate  ==>\n");

	int i = 0;
	int err = 0;
	int BitRate1 = 0;
	int BitRate2 = 0;
	int BitRate3 = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_MULTI_STREAM_CTRL,			/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;
	xctrl.data[1] = 0x04;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_Multi_Get_BitRate ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Get BitRate1~3  (unit:bps)
	memset(xctrl.data, 0, xctrl.size);
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) 
	{
		LOGV("XU_Multi_Get_BitRate ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	LOGV("XU_Multi_Get_BitRate <== Success \n");
	
	for(i=0; i<9; i++)
		LOGV("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

	BitRate1 =  ( xctrl.data[0]<<16 )| ( xctrl.data[1]<<8 )| (xctrl.data[2]) ;
	BitRate2 =  ( xctrl.data[3]<<16 )| ( xctrl.data[4]<<8 )| (xctrl.data[5]) ;
	BitRate3 =  ( xctrl.data[6]<<16 )| ( xctrl.data[7]<<8 )| (xctrl.data[8]) ;
	
	LOGV("  HD BitRate (%d)\n", BitRate1);
	LOGV("  QVGA BitRate (%d)\n", BitRate2);
	LOGV("  QQVGA BitRate (%d)\n", BitRate3);

	return 0;
}
#endif

int XU_Multi_Set_BitRate(int fd, unsigned int StreamID, unsigned int BitRate)
{	
	LOGV("XU_Multi_Set_BitRate  StreamID=%d  BiteRate=%d  ==>\n",StreamID ,BitRate);

	int err = 0;
	__u8 ctrldata[11]={0};
	int BitRate_CtrlNum = 0;

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_MULTI_STREAM_CTRL,			/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;
	xctrl.data[1] = 0x04;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_Multi_Set_BitRate ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Set BitRate  (unit:bps)
	xctrl.data[0] = StreamID;
	xctrl.data[1] = (BitRate >> 16)&0xFF;
	xctrl.data[2] = (BitRate >> 8)&0xFF;
	xctrl.data[3] = BitRate&0xFF;
	
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_Multi_Set_BitRate ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	LOGV("XU_Multi_Set_BitRate <== Success \n");
	return 0;
}

int XU_Multi_Get_BitRate(int fd, unsigned int StreamID, unsigned int *BitRate)
{	
	LOGV("XU_Multi_Get_BitRate  ==>\n");

	int i = 0;
	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_MULTI_STREAM_CTRL,			/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;
	xctrl.data[1] = 0x05;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_Multi_Get_BitRate ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Set Stream ID
	xctrl.data[0] = StreamID;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_Multi_Get_BitRate ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Get BitRate (unit:bps)
	memset(xctrl.data, 0, xctrl.size);
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) 
	{
		LOGV("XU_Multi_Get_BitRate ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	LOGV("XU_Multi_Get_BitRate <== Success \n");
	
	for(i=0; i<4; i++)
		LOGV("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

	*BitRate = ( xctrl.data[1]<<16 ) | ( xctrl.data[2]<<8 ) | xctrl.data[3];
	LOGV("  Stream= %d   BitRate= %d\n", xctrl.data[0], *BitRate);

	return 0;
}

int XU_Multi_Set_QP(int fd, unsigned int StreamID, unsigned int QP_Val)
{	
	LOGV("XU_Multi_Set_QP  StreamID=%d  QP_Val=%d  ==>\n",StreamID ,QP_Val);

	int err = 0;
	__u8 ctrldata[11]={0};
	int BitRate_CtrlNum = 0;

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_MULTI_STREAM_CTRL,			/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;
	xctrl.data[1] = 0x05;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_Multi_Set_QP ==> Switch cmd(5) : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Set Stream ID
	xctrl.data[0] = StreamID;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_Multi_Set_QP ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Switch command
	xctrl.data[0] = 0x9A;
	xctrl.data[1] = 0x06;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_Multi_Set_QP ==> Switch cmd(6) : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Set QP
	xctrl.data[0] = StreamID;
	xctrl.data[1] = QP_Val;

	
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_Multi_Set_QP ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	LOGV("XU_Multi_Set_QP <== Success \n");
	return 0;
}

int XU_Multi_Get_QP(int fd, unsigned int StreamID, unsigned int *QP_val)
{	
	LOGV("XU_Multi_Get_QP  ==>\n");

	int i = 0;
	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_MULTI_STREAM_CTRL,			/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;
	xctrl.data[1] = 0x05;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_Multi_Get_QP ==> Switch cmd(5) : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Set Stream ID
	xctrl.data[0] = StreamID;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_Multi_Get_QP ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Switch command
	xctrl.data[0] = 0x9A;
	xctrl.data[1] = 0x06;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_Multi_Get_QP ==> Switch cmd(6) : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Get QP
	memset(xctrl.data, 0, xctrl.size);
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) 
	{
		LOGV("XU_Multi_Get_QP ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	LOGV("XU_Multi_Get_QP <== Success \n");
	
	for(i=0; i<2; i++)
		LOGV("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

	*QP_val = xctrl.data[1];
	LOGV("  Stream= %d   QP_val = %d\n", xctrl.data[0], *QP_val);

	return 0;
}

int XU_OSD_Timer_Ctrl(int fd, unsigned char enable)
{	
	LOGV("XU_OSD_Timer_Ctrl  ==>\n");

	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_OSD_CTRL,					/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x00;				// OSD Timer control

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Timer_Ctrl ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Set enable / disable timer count
	xctrl.data[0] = enable;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Timer_Ctrl ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	LOGV("XU_OSD_Timer_Ctrl <== Success \n");

	return 0;
}

int XU_OSD_Set_RTC(int fd, unsigned int year, unsigned char month, unsigned char day, unsigned char hour, unsigned char minute, unsigned char second)
{	
	LOGV("XU_OSD_Set_RTC  ==>\n");

	int i = 0;
	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_OSD_CTRL,					/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x01;				// OSD RTC control

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Set_RTC ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Set RTC
	xctrl.data[0] = second;
	xctrl.data[1] = minute;
	xctrl.data[2] = hour;
	xctrl.data[3] = day;
	xctrl.data[4] = month;
	xctrl.data[5] = (year & 0xFF00) >> 8;
	xctrl.data[6] = (year & 0x00FF);

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Set_RTC ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	for(i=0; i<7; i++)
		LOGV("      Set data[%d] : 0x%x\n", i, xctrl.data[i]);
	
	LOGV("XU_OSD_Set_RTC <== Success \n");

	return 0;
}

int XU_OSD_Get_RTC(int fd, unsigned int *year, unsigned char *month, unsigned char *day, unsigned char *hour, unsigned char *minute, unsigned char *second)
{	
	LOGV("XU_OSD_Get_RTC  ==>\n");

	int i = 0;
	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_OSD_CTRL,					/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x01;				// OSD RTC control

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Get_RTC ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0)
	{
		LOGV("XU_OSD_Get_RTC ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	for(i=0; i<7; i++)
		LOGV("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

	LOGV("XU_OSD_Get_RTC <== Success \n");
	
	*year = (xctrl.data[5]<<8) | xctrl.data[6];
	*month = xctrl.data[4];
	*day = xctrl.data[3];
	*hour = xctrl.data[2];
	*minute = xctrl.data[1];
	*second = xctrl.data[0];

	LOGV(" year 	= %d \n",*year);
	LOGV(" month	= %d \n",*month);
	LOGV(" day 	= %d \n",*day);
	LOGV(" hour 	= %d \n",*hour);
	LOGV(" minute	= %d \n",*minute);
	LOGV(" second	= %d \n",*second);
	
	return 0;
}

int XU_OSD_Set_Size(int fd, unsigned char LineSize, unsigned char BlockSize)
{	
	LOGV("XU_OSD_Set_Size  ==>\n");

	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_OSD_CTRL,					/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x02;				// OSD Size control

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Set_Size ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	if(LineSize > 4)
		LineSize = 4;

	if(BlockSize > 4)
		BlockSize = 4;
		
	// Set data
	xctrl.data[0] = LineSize;
	xctrl.data[1] = BlockSize;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Set_Size ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	LOGV("XU_OSD_Set_Size <== Success \n");

	return 0;
}

int XU_OSD_Get_Size(int fd, unsigned char *LineSize, unsigned char *BlockSize)
{	
	LOGV("XU_OSD_Get_Size  ==>\n");

	int i = 0;
	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_OSD_CTRL,					/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x02;				// OSD Size control

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Get_Size ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0)
	{
		LOGV("XU_OSD_Get_Size ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	for(i=0; i<2; i++)
		LOGV("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

	*LineSize = xctrl.data[0];
	*BlockSize = xctrl.data[1];

	LOGV("OSD Size (Line) = %d\n",*LineSize);
	LOGV("OSD Size (Block) = %d\n",*BlockSize);
	LOGV("XU_OSD_Get_Size <== Success \n");
	
	return 0;
}

int XU_OSD_Set_Color(int fd, unsigned char FontColor, unsigned char BorderColor)
{	
	LOGV("XU_OSD_Set_Color  ==>\n");

	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_OSD_CTRL,					/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x03;				// OSD Color control

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Set_Color ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	if(FontColor > 4)
		FontColor = 4;

	if(BorderColor > 4)
		BorderColor = 4;
		
	// Set data
	xctrl.data[0] = FontColor;
	xctrl.data[1] = BorderColor;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Set_Color ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	LOGV("XU_OSD_Set_Color <== Success \n");

	return 0;
}

int XU_OSD_Get_Color(int fd, unsigned char *FontColor, unsigned char *BorderColor)
{	
	LOGV("XU_OSD_Get_Color  ==>\n");

	int i = 0;
	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_OSD_CTRL,					/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x03;				// OSD Color control

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Get_Color ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0)
	{
		LOGV("XU_OSD_Get_Color ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	for(i=0; i<2; i++)
		LOGV("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

	*FontColor = xctrl.data[0];
	*BorderColor = xctrl.data[1];

	LOGV("OSD Font Color = %d\n",*FontColor );
	LOGV("OSD Border Color = %d\n",*BorderColor);
	LOGV("XU_OSD_Get_Color <== Success \n");
	
	return 0;
}

int XU_OSD_Set_Enable(int fd, unsigned char Enable_Line, unsigned char Enable_Block)
{	
	LOGV("XU_OSD_Set_Enable  ==>\n");

	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_OSD_CTRL,					/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x04;				// OSD enable

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Set_Enable ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
		
	// Set data
	xctrl.data[0] = Enable_Line;
	xctrl.data[1] = Enable_Block;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Set_Enable ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	LOGV("XU_OSD_Set_Enable <== Success \n");

	return 0;
}

int XU_OSD_Get_Enable(int fd, unsigned char *Enable_Line, unsigned char *Enable_Block)
{	
	LOGV("XU_OSD_Get_Enable  ==>\n");

	int i = 0;
	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_OSD_CTRL,					/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x04;				// OSD Enable

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Get_Enable ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0)
	{
		LOGV("XU_OSD_Get_Enable ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	for(i=0; i<2; i++)
		LOGV("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

	*Enable_Line = xctrl.data[0];
	*Enable_Block = xctrl.data[1];
	
	LOGV("OSD Enable Line = %d\n",*Enable_Line);
	LOGV("OSD Enable Block = %d\n",*Enable_Block);

	LOGV("XU_OSD_Get_Enable <== Success \n");
	
	return 0;
}

int XU_OSD_Set_AutoScale(int fd, unsigned char Enable_Line, unsigned char Enable_Block)
{	
	LOGV("XU_OSD_Set_AutoScale  ==>\n");

	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_OSD_CTRL,					/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x05;				// OSD Auto Scale enable

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Set_AutoScale ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
		
	// Set data
	xctrl.data[0] = Enable_Line;
	xctrl.data[1] = Enable_Block;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Set_AutoScale ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	LOGV("XU_OSD_Set_AutoScale <== Success \n");

	return 0;
}

int XU_OSD_Get_AutoScale(int fd, unsigned char *Enable_Line, unsigned char *Enable_Block)
{	
	LOGV("XU_OSD_Get_AutoScale  ==>\n");

	int i = 0;
	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_OSD_CTRL,					/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x05;				// OSD Auto Scale enable

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Get_AutoScale ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0)
	{
		LOGV("XU_OSD_Get_AutoScale ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	for(i=0; i<2; i++)
		LOGV("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

	*Enable_Line = xctrl.data[0];
	*Enable_Block = xctrl.data[1];

	LOGV("OSD Enable Line  Auto Scale = %d\n",*Enable_Line);
	LOGV("OSD Enable Block Auto Scale = %d\n",*Enable_Block);
	LOGV("XU_OSD_Get_AutoScale <== Success \n");
	
	return 0;
}

int XU_OSD_Set_Multi_Size(int fd, unsigned char Stream0, unsigned char Stream1, unsigned char Stream2)
{	
	LOGV("XU_OSD_Set_Multi_Size  %d   %d   %d  ==>\n",Stream0 ,Stream1 , Stream2);

	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_OSD_CTRL,					/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x06;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Set_Multi_Size ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	// Set data
	xctrl.data[0] = Stream0;
	xctrl.data[1] = Stream1;
	xctrl.data[2] = Stream2;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Set_Multi_Size ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	LOGV("XU_OSD_Set_Multi_Size <== Success \n");

	return 0;
}

int XU_OSD_Get_Multi_Size(int fd, unsigned char *Stream0, unsigned char *Stream1, unsigned char *Stream2)
{	
	LOGV("XU_OSD_Get_Multi_Size  ==>\n");

	int i = 0;
	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_OSD_CTRL,					/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x06;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Get_Multi_Size ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0)
	{
		LOGV("XU_OSD_Get_Multi_Size ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	for(i=0; i<3; i++)
		LOGV("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

	*Stream0 = xctrl.data[0];
	*Stream1 = xctrl.data[1];
	*Stream2 = xctrl.data[2];
	
	LOGV("OSD Multi Stream 0 Size = %d\n",*Stream0);
	LOGV("OSD Multi Stream 1 Size = %d\n",*Stream1);
	LOGV("OSD Multi Stream 2 Size = %d\n",*Stream2);
	LOGV("XU_OSD_Get_Multi_Size <== Success \n");
	
	return 0;
}

int XU_OSD_Set_Start_Position(int fd, unsigned char OSD_Type, unsigned int RowStart, unsigned int ColStart)
{	
	LOGV("XU_OSD_Set_Start_Position  ==>\n");

	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_OSD_CTRL,					/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x08;				// OSD Start Position

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Set_Start_Position ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	if(OSD_Type > 3)
		OSD_Type = 0;
		
	// Set data
	xctrl.data[0] = OSD_Type;
	xctrl.data[1] = (RowStart & 0xFF00) >> 8;	//unit 16 lines
	xctrl.data[2] = RowStart & 0x00FF;
	xctrl.data[3] = (ColStart & 0xFF00) >> 8;	//unit 16 pixels
	xctrl.data[4] = ColStart & 0x00FF;	

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Set_Start_Position ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	LOGV("XU_OSD_Set_Start_Position <== Success \n");

	return 0;
}

int XU_OSD_Get_Start_Position(int fd, unsigned int *LineRowStart, unsigned int *LineColStart, unsigned int *BlockRowStart, unsigned int *BlockColStart)
{	
	LOGV("XU_OSD_Set_Start_Position  ==>\n");

	int i = 0;
	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_OSD_CTRL,					/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x08;				// OSD Start Position

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Set_Start_Position ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0)
	{
		LOGV("XU_OSD_Set_Start_Position ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	for(i=0; i<8; i++)
		LOGV("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

	*LineRowStart = (xctrl.data[0] << 8) | xctrl.data[1];
	*LineColStart = (xctrl.data[2] << 8) | xctrl.data[3];
	*BlockRowStart = (xctrl.data[4] << 8) | xctrl.data[5];
	*BlockColStart = (xctrl.data[6] << 8) | xctrl.data[7];
	
	LOGV("OSD Line Start Row =%d * 16lines\n",*LineRowStart);
	LOGV("OSD Line Start Col =%d * 16pixels\n",*LineColStart);
	LOGV("OSD Block Start Row =%d * 16lines\n",*BlockRowStart);
	LOGV("OSD Block Start Col =%d * 16pixels\n",*BlockColStart);
	LOGV("XU_OSD_Set_Start_Position <== Success \n");
	
	return 0;
}

int XU_OSD_Set_MS_Start_Position(int fd, unsigned char StreamID, unsigned char RowStart, unsigned char ColStart)
{	
	LOGV("XU_OSD_Set_MS_Start_Position  ==>\n");

	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_OSD_CTRL,					/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x09;				// OSD MS Start Position

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Set_MS_Start_Position ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	// Set data
	xctrl.data[0] = StreamID;
	xctrl.data[1] = RowStart & 0x00FF;
	xctrl.data[2] = ColStart & 0x00FF;	

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Set_MS_Start_Position ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	LOGV("XU_OSD_Set_MS_Start_Position  %d %d %d<== Success \n", StreamID, RowStart, ColStart);

	return 0;
}

int XU_OSD_Get_MS_Start_Position(int fd, unsigned char *S0_Row, unsigned char *S0_Col, unsigned char *S1_Row, unsigned char *S1_Col, unsigned char *S2_Row, unsigned char *S2_Col)
{	
	LOGV("XU_OSD_Get_MS_Start_Position  ==>\n");

	int i = 0;
	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_OSD_CTRL,					/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x09;				// OSD MS Start Position

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Get_MS_Start_Position ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0)
	{
		LOGV("XU_OSD_Get_MS_Start_Position ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	for(i=0; i<6; i++)
		LOGV("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

	*S0_Row = xctrl.data[0];
	*S0_Col = xctrl.data[1];
	*S1_Row = xctrl.data[2];
	*S1_Col = xctrl.data[3];
	*S2_Row = xctrl.data[4];
	*S2_Col = xctrl.data[5];
	
	LOGV("OSD Stream0 Start Row = %d * 16lines\n",*S0_Row);
	LOGV("OSD Stream0 Start Col = %d * 16pixels\n",*S0_Col);
	LOGV("OSD Stream1 Start Row = %d * 16lines\n",*S1_Row);
	LOGV("OSD Stream1 Start Col = %d * 16pixels\n",*S1_Col);
	LOGV("OSD Stream2 Start Row = %d * 16lines\n",*S2_Row);
	LOGV("OSD Stream2 Start Col = %d * 16pixels\n",*S2_Col);
	LOGV("XU_OSD_Get_MS_Start_Position <== Success \n");
	
	return 0;
}

int XU_OSD_Set_String(int fd, unsigned char group, char *String)
{	
	LOGV("XU_OSD_Set_String  ==>\n");

	int i = 0;
	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_OSD_CTRL,					/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x07;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Set_String ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Set data
	xctrl.data[0] = 1;
	xctrl.data[1] = group;

	for(i=0; i<8; i++)
		xctrl.data[i+2] = String[i];

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Set_String ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	LOGV("XU_OSD_Set_String <== Success \n");
	
	return 0;
}

int XU_OSD_Get_String(int fd, unsigned char group, char *String)
{	
	LOGV("XU_OSD_Get_String  ==>\n");

	int i = 0;
	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_OSD_CTRL,					/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x07;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Get_String ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Set read mode
	xctrl.data[0] = 0;
	xctrl.data[1] = group;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_OSD_Set_String ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0)
	{
		LOGV("XU_OSD_Get_String ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	group = xctrl.data[1];

	LOGV("      Get data[0] : 0x%x\n", xctrl.data[0]);
	LOGV("      Get data[1] : 0x%x\n", group);
	
	for(i=0; i<8; i++)
	{
		String[i] = xctrl.data[i+2];
		LOGV("      Get data[%d] : 0x%x\n", i+2, String[i]);
	}

	LOGV("OSD String = %s \n",String);
	LOGV("XU_OSD_Get_String <== Success \n");

	return 0;
}

int XU_MD_Set_Mode(int fd, unsigned char Enable)
{	
	LOGV("XU_MD_Set_Mode  ==>\n");

	int err = 0;
	__u8 ctrldata[24]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_MOTION_DETECTION,			/* function selector	*/
		24,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x01;				// Motion detection mode

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_MD_Set_Mode ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Set data
	xctrl.data[0] = Enable;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_MD_Set_Mode ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	LOGV("XU_MD_Set_Mode <== Success \n");

	return 0;
}

int XU_MD_Get_Mode(int fd, unsigned char *Enable)
{	
	LOGV("XU_MD_Get_Mode  ==>\n");

	int err = 0;
	__u8 ctrldata[24]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_MOTION_DETECTION,			/* function selector	*/
		24,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x01;				// Motion detection mode

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_MD_Get_Mode ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0)
	{
		LOGV("XU_MD_Get_Mode ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	*Enable = xctrl.data[0];

	LOGV("Motion Detect mode = %d\n",*Enable);
	LOGV("XU_MD_Get_Mode <== Success \n");
	
	return 0;
}

int XU_MD_Set_Threshold(int fd, unsigned int MD_Threshold)
{	
	LOGV("XU_MD_Set_Threshold  ==>\n");

	int err = 0;
	__u8 ctrldata[24]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_MOTION_DETECTION,			/* function selector	*/
		24,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x02;				// Motion detection threshold

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_MD_Set_Threshold ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Set data
	xctrl.data[0] = (MD_Threshold & 0xFF00) >> 8;
	xctrl.data[1] = MD_Threshold & 0x00FF;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_MD_Set_Threshold ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	LOGV("XU_MD_Set_Threshold <== Success \n");

	return 0;
}

int XU_MD_Get_Threshold(int fd, unsigned int *MD_Threshold)
{	
	LOGV("XU_MD_Get_Threshold  ==>\n");

	int err = 0;
	__u8 ctrldata[24]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_MOTION_DETECTION,			/* function selector	*/
		24,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x02;				// Motion detection threshold

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_MD_Get_Threshold ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0)
	{
		LOGV("XU_MD_Get_Threshold ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	*MD_Threshold = (xctrl.data[0] << 8) | xctrl.data[1];
	
	LOGV("Motion Detect threshold = %d\n",*MD_Threshold);
	LOGV("XU_MD_Get_Threshold <== Success \n");
	
	return 0;
}

int XU_MD_Set_Mask(int fd, unsigned char *Mask)
{	
	LOGV("XU_MD_Set_Mask  ==>\n");

	int err = 0;
	unsigned char i;
	__u8 ctrldata[24]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_MOTION_DETECTION,			/* function selector	*/
		24,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x03;				// Motion detection mask

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_MD_Set_Mask ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Set data
	for(i=0; i < 24; i++)
	{
		xctrl.data[i] = Mask[i];
	}

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_MD_Set_Mask ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	LOGV("XU_MD_Set_Mask <== Success \n");

	return 0;
}

int XU_MD_Get_Mask(int fd, unsigned char *Mask)
{	
	LOGV("XU_MD_Get_Mask  ==>\n");

	int err = 0;
	int i,j,k,l;
	__u8 ctrldata[24]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_MOTION_DETECTION,			/* function selector	*/
		24,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x03;				// Motion detection mask

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_MD_Get_Mask ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)  \n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0)
	{
		LOGV("XU_MD_Get_Mask ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i) \n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	for(i=0; i<24; i++)
	{
		LOGV("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);
		Mask[i] = xctrl.data[i];
	}
	
	LOGV("               ======   Motion Detect Mask   ======                \n");
	LOGV("     1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16 \n");
	
	for(k=0; k<12; k++)
	{
		LOGV("%2d   ",k+1);
		for(j=0; j<2; j++)
		{
			for(i=0; i<8; i++)
				LOGV("%d   ",(Mask[k*2+j]>>i)&0x01);
		}
		LOGV("\n");
	}

	LOGV("XU_MD_Get_Mask <== Success \n");
	
	return 0;
}

int XU_MD_Set_RESULT(int fd, unsigned char *Result)
{	
	//LOGV("XU_MD_Set_RESULT  ==>\n");

	int err = 0;
	unsigned char i;
	__u8 ctrldata[24]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_MOTION_DETECTION,			/* function selector	*/
		24,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x04;				// Motion detection Result

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_MD_Set_RESULT ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Set data
	for(i=0; i < 24; i++)
	{
		xctrl.data[i] = Result[i];
	}

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_MD_Set_RESULT ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	//LOGV("XU_MD_Set_RESULT <== Success \n");

	return 0;
}

int XU_MD_Get_RESULT(int fd, unsigned char *Result)
{	
	//LOGV("XU_MD_Get_RESULT  ==>\n");

	int err = 0;
	int i,j,k,l;
	__u8 ctrldata[24]={0};

	struct uvc_xu_control xctrl = {
		XU_SONIX_USR_ID,						/* bUnitID 				*/
		XU_SONIX_USR_MOTION_DETECTION,			/* function selector	*/
		24,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	// Switch command
	xctrl.data[0] = 0x9A;				// Tag
	xctrl.data[1] = 0x04;				// Motion detection Result

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_MD_Get_RESULT ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)  \n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0)
	{
		LOGV("XU_MD_Get_RESULT ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i) \n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	for(i=0; i<24; i++)
	{
		//LOGV("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);
		Result[i] = xctrl.data[i];
	}

	system("clear");
	LOGV("               ------   Motion Detect Result   ------                \n");
	LOGV("     1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16 \n");
	
	for(k=0; k<12; k++)
	{
		LOGV("%2d   ",k+1);
		for(j=0; j<2; j++)
		{
			for(i=0; i<8; i++)
				LOGV("%d   ",(Result[k*2+j]>>i)&0x01);
		}
		LOGV("\n");
	}

	//LOGV("XU_MD_Get_RESULT <== Success \n");
	
	return 0;
}

int XU_MJPG_Get_Bitrate(int fd, unsigned int *MJPG_Bitrate)
{
	LOGV("XU_MJPG_Get_Bitrate ==>\n");
	int i = 0;
	int ret = 0;
	int err = 0;
	__u8 ctrldata[11]={0};
	int BitRate_CtrlNum = 0;
	*MJPG_Bitrate = 0;

	struct uvc_xu_control xctrl = {
			0,										/* bUnitID 				*/
			0,										/* function selector	*/
			11,										/* data size			*/
			(__u8*)&ctrldata								/* *data				*/
		};

	if(chip_id == CHIP_SNC291A)
	{
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_MJPG_CTRL;
		
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x02;
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_MJPG_CTRL;

		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x01;
	}

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_MJPG_Get_Bitrate ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	// Get Bit rate ctrl number
	memset(xctrl.data, 0, xctrl.size);
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0) 
	{
		LOGV("XU_MJPG_Get_Bitrate ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	LOGV("   == XU_MJPG_Get_Bitrate Success == \n");

	if(chip_id == CHIP_SNC291A)
	{
		for(i=0; i<2; i++)
			LOGV("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

		BitRate_CtrlNum = ( xctrl.data[0]<<8 )| (xctrl.data[1]) ;

		// Bit Rate = BitRate_Ctrl_Num*256*fps*8 /1024(Kbps)
		*MJPG_Bitrate = (BitRate_CtrlNum*256.0*m_CurrentFPS*8)/1024.0;
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		for(i=0; i<4; i++)
			LOGV("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);

		*MJPG_Bitrate = (xctrl.data[0] << 24) | (xctrl.data[1] << 16) | (xctrl.data[2] << 8) | (xctrl.data[3]) ;
	}
	
	LOGV("XU_MJPG_Get_Bitrate (%x)<==\n", *MJPG_Bitrate);
	return ret;
}

int XU_MJPG_Set_Bitrate(int fd, unsigned int MJPG_Bitrate)
{	
	LOGV("XU_MJPG_Set_Bitrate (%x) ==>\n",MJPG_Bitrate);
	int ret = 0;
	int err = 0;
	__u8 ctrldata[11]={0};
	int BitRate_CtrlNum = 0;

	struct uvc_xu_control xctrl = {
			0,										/* bUnitID 				*/
			0,										/* function selector	*/
			11,										/* data size			*/
			(__u8*)&ctrldata								/* *data				*/
		};

	if(chip_id == CHIP_SNC291A)
	{
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_MJPG_CTRL;
		
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x02;
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_MJPG_CTRL;

		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x01;
	}
	
	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_MJPG_Set_Bitrate ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Set Bit Rate Ctrl Number
	if(chip_id == CHIP_SNC291A)
	{
		// Bit Rate = BitRate_Ctrl_Num*256*fps*8/1024 (Kbps)
		BitRate_CtrlNum = ((MJPG_Bitrate*1024)/(256*m_CurrentFPS*8));

		xctrl.data[0] = (BitRate_CtrlNum & 0xFF00) >> 8;
		xctrl.data[1] = (BitRate_CtrlNum & 0x00FF);
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		xctrl.data[0] = (MJPG_Bitrate & 0xFF000000) >> 24;
		xctrl.data[1] = (MJPG_Bitrate & 0x00FF0000) >> 16;
		xctrl.data[2] = (MJPG_Bitrate & 0x0000FF00) >> 8;
		xctrl.data[3] = (MJPG_Bitrate & 0x000000FF);
	}

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_MJPG_Set_Bitrate ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	LOGV("XU_MJPG_Set_Bitrate <== Success \n");
	return ret;
}

int XU_IMG_Set_Mirror(int fd, unsigned char Mirror)
{	
	LOGV("XU_IMG_Set_Mirror  ==>\n");

	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		0,										/* bUnitID 				*/
		0,										/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	if(chip_id == CHIP_SNC291A)
	{
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_IMG_SETTING;
		
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x01;
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_IMG_SETTING;

		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x01;
	}

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_IMG_Set_Mirror ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Set data
	xctrl.data[0] = Mirror;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_IMG_Set_Mirror ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	LOGV("XU_IMG_Set_Mirror  0x%x <== Success \n",Mirror);

	return 0;
}

int XU_IMG_Get_Mirror(int fd, unsigned char *Mirror)
{	
	LOGV("XU_IMG_Get_Mirror  ==>\n");

	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		0,										/* bUnitID 				*/
		0,										/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	if(chip_id == CHIP_SNC291A)
	{
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_IMG_SETTING;
		
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x01;
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_IMG_SETTING;

		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x01;
	}

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_IMG_Get_Mirror ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0)
	{
		LOGV("XU_IMG_Get_Mirror ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	*Mirror = xctrl.data[0];

	LOGV("Mirror = %d\n",*Mirror);
	LOGV("XU_IMG_Get_Mirror <== Success \n");
	
	return 0;
}

int XU_IMG_Set_Flip(int fd, unsigned char Flip)
{	
	LOGV("XU_IMG_Set_Flip  ==>\n");

	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		0,										/* bUnitID 				*/
		0,										/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	if(chip_id == CHIP_SNC291A)
	{
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_IMG_SETTING;
		
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x02;
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_IMG_SETTING;

		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x02;
	}

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_IMG_Set_Flip ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Set data
	xctrl.data[0] = Flip;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_IMG_Set_Flip ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	LOGV("XU_IMG_Set_Flip  0x%x <== Success \n",Flip);

	return 0;
}

int XU_IMG_Get_Flip(int fd, unsigned char *Flip)
{	
	LOGV("XU_IMG_Get_Flip  ==>\n");

	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		0,										/* bUnitID 				*/
		0,										/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	if(chip_id == CHIP_SNC291A)
	{
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_IMG_SETTING;
		
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x02;
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_IMG_SETTING;

		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x02;
	}

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_IMG_Get_Flip ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0)
	{
		LOGV("XU_IMG_Get_Flip ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	*Flip = xctrl.data[0];

	LOGV("Flip = %d\n",*Flip);
	LOGV("XU_IMG_Get_Flip <== Success \n");
	
	return 0;
}

int XU_IMG_Set_Color(int fd, unsigned char Color)
{	
	LOGV("XU_IMG_Set_Color  ==>\n");

	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		0,										/* bUnitID 				*/
		0,										/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	if(chip_id == CHIP_SNC291A)
	{
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_IMG_SETTING;
		
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x03;
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_IMG_SETTING;

		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x03;
	}

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_IMG_Set_Color ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Set data
	xctrl.data[0] = Color;

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_IMG_Set_Color ==> ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	LOGV("XU_IMG_Set_Color  0x%x <== Success \n",Color);

	return 0;
}

int XU_IMG_Get_Color(int fd, unsigned char *Color)
{	
	LOGV("XU_IMG_Get_Color  ==>\n");

	int err = 0;
	__u8 ctrldata[11]={0};

	struct uvc_xu_control xctrl = {
		0,										/* bUnitID 				*/
		0,										/* function selector	*/
		11,										/* data size			*/
		(__u8*)&ctrldata						/* *data				*/
	};

	if(chip_id == CHIP_SNC291A)
	{
		xctrl.unit = XU_SONIX_SYS_ID;
		xctrl.selector = XU_SONIX_SYS_IMG_SETTING;
		
		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x03;
	}
	else if((chip_id == CHIP_SNC291B)||(chip_id == CHIP_SNC292A))
	{
		xctrl.unit = XU_SONIX_USR_ID;
		xctrl.selector = XU_SONIX_USR_IMG_SETTING;

		// Switch command
		xctrl.data[0] = 0x9A;				// Tag
		xctrl.data[1] = 0x03;
	}

	if ((err=ioctl(fd, UVCIOC_CTRL_SET, &xctrl)) < 0) 
	{
		LOGV("XU_IMG_Get_Color ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}

	// Get data
	memset(xctrl.data, 0, xctrl.size);
	if ((err=ioctl(fd, UVCIOC_CTRL_GET, &xctrl)) < 0)
	{
		LOGV("XU_IMG_Get_Color ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",err);
		if(err==EINVAL)
			LOGV("Invalid arguments\n");
		return err;
	}
	
	*Color = xctrl.data[0];

	LOGV("Image Color = %d\n",*Color);
	LOGV("XU_IMG_Get_Color <== Success \n");
	
	return 0;
}
