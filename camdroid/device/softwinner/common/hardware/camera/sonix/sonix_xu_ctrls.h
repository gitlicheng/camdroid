#ifndef SONIX_XU_CTRLS_H
#define SONIX_XU_CTRLS_H

#include <linux/videodev2.h>


/*
 * Dynamic controls
 */

#define UVC_CTRL_DATA_TYPE_RAW		0
#define UVC_CTRL_DATA_TYPE_SIGNED	1
#define UVC_CTRL_DATA_TYPE_UNSIGNED	2
#define UVC_CTRL_DATA_TYPE_BOOLEAN	3
#define UVC_CTRL_DATA_TYPE_ENUM		4
#define UVC_CTRL_DATA_TYPE_BITMASK	5

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

#define V4L2_CID_BASE_EXTCTR_SONIX				0x0A0c4501
#define V4L2_CID_BASE_SONIX						V4L2_CID_BASE_EXTCTR_SONIX
#define V4L2_CID_ASIC_RW_SONIX					V4L2_CID_BASE_SONIX+1
#define V4L2_CID_FRAME_INFO_SONIX				V4L2_CID_BASE_SONIX+2
#define V4L2_CID_H264_CTRL_SONIX				V4L2_CID_BASE_SONIX+3
#define V4L2_CID_MJPG_CTRL_SONIX				V4L2_CID_BASE_SONIX+4
#define V4L2_CID_OSD_CTRL_SONIX					V4L2_CID_BASE_SONIX+5
#define V4L2_CID_MOTION_DETECTION_SONIX			V4L2_CID_BASE_SONIX+6
#define V4L2_CID_IMG_SETTING_SONIX				V4L2_CID_BASE_SONIX+7
#define V4L2_CID_MULTI_STREAM_CTRL_SONIX		V4L2_CID_BASE_SONIX+8
#define V4L2_CID_LAST_EXTCTR_SONIX				V4L2_CID_MULTI_STREAM_CTRL_SONIX

/* ---------------------------------------------------------------------------- */

#define UVC_GUID_SONIX_SYS_HW_CTRL				{0x70, 0x33, 0xf0, 0x28, 0x11, 0x63, 0x2e, 0x4a, 0xba, 0x2c, 0x68, 0x90, 0xeb, 0x33, 0x40, 0x16}	//291A
#define UVC_GUID_SONIX_USR_HW_CTRL				{0x94, 0x73, 0xDF, 0xDD, 0x3E, 0x97, 0x27, 0x47, 0xBE, 0xD9, 0x04, 0xED, 0x64, 0x26, 0xDC, 0x67}	//292
//#define UVC_GUID_SONIX_USR_HW_CTRL				{0x3F, 0xAE, 0x12, 0x28, 0xD7, 0xBC, 0x11, 0x4E, 0xA3, 0x57, 0x6F, 0x1E, 0xDE, 0xF7, 0xD6, 0x1D}	//292

#define XU_SONIX_SYS_ID							0x03
#define XU_SONIX_USR_ID							0x04

// ----------------------------- XU Control Selector ------------------------------------
#define XU_SONIX_SYS_ASIC_RW	      			0x01
#define XU_SONIX_SYS_FRAME_INFO					0x06
#define XU_SONIX_SYS_H264_CTRL					0x07
#define XU_SONIX_SYS_MJPG_CTRL					0x08
#define XU_SONIX_SYS_OSD_CTRL					0x09
#define XU_SONIX_SYS_MOTION_DETECTION			0x0A
#define XU_SONIX_SYS_IMG_SETTING				0x0B

#define XU_SONIX_USR_FRAME_INFO					0x01
#define XU_SONIX_USR_H264_CTRL					0x02
#define XU_SONIX_USR_MJPG_CTRL					0x03
#define XU_SONIX_USR_OSD_CTRL					0x04
#define XU_SONIX_USR_MOTION_DETECTION			0x05
#define XU_SONIX_USR_IMG_SETTING				0x06
#define XU_SONIX_USR_MULTI_STREAM_CTRL			0x07

// ----------------------------- XU Control Selector ------------------------------------
#define UVC_CONTROL_SET_CUR	(1 << 0)
#define UVC_CONTROL_GET_CUR	(1 << 1)
#define UVC_CONTROL_GET_MIN	(1 << 2)
#define UVC_CONTROL_GET_MAX	(1 << 3)
#define UVC_CONTROL_GET_RES	(1 << 4)
#define UVC_CONTROL_GET_DEF	(1 << 5)
/* Control should be saved at suspend and restored at resume. */
#define UVC_CONTROL_RESTORE	(1 << 6)
/* Control can be updated by the camera. */
#define UVC_CONTROL_AUTO_UPDATE	(1 << 7)

#define UVC_CONTROL_GET_RANGE   (UVC_CONTROL_GET_CUR | UVC_CONTROL_GET_MIN | \
                                 UVC_CONTROL_GET_MAX | UVC_CONTROL_GET_RES | \
                                 UVC_CONTROL_GET_DEF)

#ifdef __cplusplus
extern "C" {
#endif

struct uvc_xu_control_info {
	__u8 entity[16];
	__u8 index;
	__u8 selector;
	__u16 size;
	__u32 flags;
};

struct uvc_xu_control_mapping {
	__u32 id;
	__u8 name[32];
	__u8 entity[16];
	__u8 selector;

	__u8 size;
	__u8 offset;
	enum v4l2_ctrl_type v4l2_type;
	__u32 data_type;
};

struct uvc_xu_control {
	__u8 unit;
	__u8 selector;
	__u16 size;
	__u8 *data;
};

struct H264Format
{
	unsigned short  wWidth;
	unsigned short  wHeight;
	int   		fpsCnt;
	unsigned int	FrameSize;
	unsigned int	*FrPay;		// FrameInterval[0|1]Payloadsize[2|3]
};

struct Cur_H264Format
{	
	int FmtId;
	unsigned short wWidth;
	unsigned short wHeight;
	int FrameRateId;
	unsigned char framerate;
	unsigned int FrameSize;
};

struct Multistream_Info
{
	unsigned char strm_type;
	unsigned int format; 
};

typedef enum{
	CHIP_NONE = -1,
	CHIP_SNC291A = 0,
	CHIP_SNC291B,
	CHIP_SNC292A
}CHIP_SNC29X;

#define UVCIOC_CTRL_ADD		_IOW('U', 1, struct uvc_xu_control_info)
#define UVCIOC_CTRL_MAP		_IOWR('U', 2, struct uvc_xu_control_mapping)
#define UVCIOC_CTRL_GET		_IOWR('U', 3, struct uvc_xu_control)
#define UVCIOC_CTRL_SET		_IOW('U', 4, struct uvc_xu_control)

extern unsigned int  chip_id;

int XU_Init_Ctrl(int fd);
int XU_Ctrl_ReadChipID(int fd);

int H264_GetFormat(int fd);
int H264_CountFormat(unsigned char *Data, int len);
int H264_ParseFormat(unsigned char *Data, int len, struct H264Format *fmt);
int H264_GetFPS(unsigned int FrPay);

// H.264 XU +++++

int XU_Set(int fd, struct uvc_xu_control xctrl);
int XU_Get(int fd, struct uvc_xu_control *xctrl);

int XU_H264_InitFormat(int fd);
int XU_H264_GetFormatLength(int fd, unsigned short *fwLen);
int XU_H264_GetFormatData(int fd, unsigned char *fwData, unsigned short fwLen);
int XU_H264_SetFormat(int fd, struct Cur_H264Format fmt);

int XU_H264_Get_QP_Limit(int fd, int *QP_Min, int *QP_Max);
int XU_H264_Get_Mode(int fd, int *Mode);
int XU_H264_Set_Mode(int fd, int Mode);

int XU_H264_Get_QP(int fd, int *QP_Val);
int XU_H264_Set_QP(int fd, int QP_Val);

int XU_H264_Get_BitRate(int fd, double *BitRate);
int XU_H264_Set_BitRate(int fd, double BitRate);

int XU_H264_Set_IFRAME(int fd);

int XU_H264_Get_SEI(int fd, unsigned char *SEI);
int XU_H264_Set_SEI(int fd, unsigned char SEI);

int XU_H264_Get_GOP(int fd, unsigned int *GOP);
int XU_H264_Set_GOP(int fd, unsigned int GOP);

int XU_Asic_Read(int fd, unsigned int Addr, unsigned char *AsicData);
int XU_Asic_Write(int fd, unsigned int Addr, unsigned char AsicData);

int XU_Multi_Get_status(int fd, struct Multistream_Info *status);
int XU_Multi_Get_Info(int fd, struct Multistream_Info *Info);
int XU_Multi_Set_Type(int fd, unsigned int format);

int XU_Multi_Set_Enable(int fd, unsigned char enable);
int XU_Multi_Get_Enable(int fd, unsigned char *enable);

int XU_Multi_Set_BitRate(int fd, unsigned int StreamID, unsigned int BitRate);
int XU_Multi_Get_BitRate(int fd, unsigned int StreamID, unsigned int *BitRate);
int XU_Multi_Set_QP(int fd, unsigned int StreamID, unsigned int QP_Val);
int XU_Multi_Get_QP(int fd, unsigned int StreamID, unsigned int *QP_val);

int XU_OSD_Timer_Ctrl(int fd, unsigned char enable);
int XU_OSD_Set_RTC(int fd, unsigned int year, unsigned char month, unsigned char day, unsigned char hour, unsigned char minute, unsigned char second);
int XU_OSD_Get_RTC(int fd, unsigned int *year, unsigned char *month, unsigned char *day, unsigned char *hour, unsigned char *minute, unsigned char *second);

int XU_OSD_Set_Size(int fd, unsigned char LineSize, unsigned char BlockSize);
int XU_OSD_Get_Size(int fd, unsigned char *LineSize, unsigned char *BlockSize);

int XU_OSD_Set_Color(int fd, unsigned char FontColor, unsigned char BorderColor);
int XU_OSD_Get_Color(int fd, unsigned char *FontColor, unsigned char *BorderColor);

int XU_OSD_Set_Enable(int fd, unsigned char Enable_Line, unsigned char Enable_Block);
int XU_OSD_Get_Enable(int fd, unsigned char *Enable_Line, unsigned char *Enable_Block);

int XU_OSD_Set_AutoScale(int fd, unsigned char Enable_Line, unsigned char Enable_Block);
int XU_OSD_Get_AutoScale(int fd, unsigned char *Enable_Line, unsigned char *Enable_Block);

int XU_OSD_Set_Multi_Size(int fd, unsigned char Stream0, unsigned char Stream1, unsigned char Stream2);
int XU_OSD_Get_Multi_Size(int fd, unsigned char *Stream0, unsigned char *Stream1, unsigned char *Stream2);

int XU_OSD_Set_Start_Position(int fd, unsigned char OSD_Type, unsigned int RowStart, unsigned int ColStart);
int XU_OSD_Get_Start_Position(int fd, unsigned int *LineRowStart, unsigned int *LineColStart, unsigned int *BlockRowStart, unsigned int *BlockColStart);

int XU_OSD_Set_MS_Start_Position(int fd, unsigned char StreamID, unsigned char RowStart, unsigned char ColStart);
int XU_OSD_Get_MS_Start_Position(int fd, unsigned char *S0_Row, unsigned char *S0_Col, unsigned char *S1_Row, unsigned char *S1_Col, unsigned char *S2_Row, unsigned char *S2_Col);

int XU_OSD_Set_String(int fd, unsigned char group, char *String);
int XU_OSD_Get_String(int fd, unsigned char group, char *String);

int XU_MD_Set_Mode(int fd, unsigned char Enable);
int XU_MD_Get_Mode(int fd, unsigned char *Enable);

int XU_MD_Set_Threshold(int fd, unsigned int MD_Threshold);
int XU_MD_Get_Threshold(int fd, unsigned int *MD_Threshold);

int XU_MD_Set_Mask(int fd, unsigned char *Mask);
int XU_MD_Get_Mask(int fd, unsigned char *Mask);

int XU_MD_Set_RESULT(int fd, unsigned char *Result);
int XU_MD_Get_RESULT(int fd, unsigned char *Result);

int XU_MJPG_Set_Bitrate(int fd, unsigned int MJPG_Bitrate);
int XU_MJPG_Get_Bitrate(int fd, unsigned int *MJPG_Bitrate);

int XU_IMG_Set_Mirror(int fd, unsigned char Mirror);
int XU_IMG_Get_Mirror(int fd, unsigned char *Mirror);

int XU_IMG_Set_Flip(int fd, unsigned char Flip);
int XU_IMG_Get_Flip(int fd, unsigned char *Flip);

int XU_IMG_Set_Color(int fd, unsigned char Color);
int XU_IMG_Get_Color(int fd, unsigned char *Color);
// H.264 XU -----
#ifdef __cplusplus
}
#endif

#endif



