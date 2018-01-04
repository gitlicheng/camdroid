#ifndef _BACKCAR_MONITOR_H
#define _BACKCAR_MONITOR_H

//#include <inttypes.h>
#include <sys/types.h>
#include <linux/videodev2.h>
#include <unistd.h>
#include "debug.h"
#include <utils/RefBase.h>
#include <utils/Thread.h>



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

#define V4L2_CID_BASE_EXTCTR_SONIX					0x0A0c4501
#define V4L2_CID_BASE_SONIX                       	V4L2_CID_BASE_EXTCTR_SONIX
#define V4L2_CID_ASIC_CTRL_SONIX                    V4L2_CID_BASE_SONIX+1
#define V4L2_CID_I2C_CTRL_SONIX						V4L2_CID_BASE_SONIX+2
#define V4L2_CID_SF_READ_SONIX                      V4L2_CID_BASE_SONIX+3
#define V4L2_CID_LAST_EXTCTR_SONIX					V4L2_CID_IMG_SETTING_SONIX

/* ---------------------------------------------------------------------------- */

#define UVC_GUID_SONIX_USER_HW_CONTROL       	{0x70, 0x33, 0xf0, 0x28, 0x11, 0x63, 0x2e, 0x4a, 0xba, 0x2c, 0x68, 0x90, 0xeb, 0x33, 0x40, 0x16}

// ----------------------------- XU Control Selector ------------------------------------
#define XU_SONIX_ASIC_CTRL 				0x01   // 与control 1 相对应
#define XU_SONIX_I2C_CTRL				0x02
#define XU_SONIX_SF_READ 				0x03

//#define XU_SONIX_H264_FMT  			0x06
//#define XU_SONIX_H264_QP   			0x07
//#define XU_SONIX_H264_BITRATE 		0x08
//#define XU_SONIX_FRAME_INFO  			0x06
//#define XU_SONIX_H264_CTRL   			0x07
//#define XU_SONIX_MJPG_CTRL		 		0x08
//#define XU_SONIX_OSD_CTRL	  			0x09
//#define XU_SONIX_MOTION_DETECTION		0x0A
//#define XU_SONIX_IMG_SETTING	 		0x0B

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



#define UVCIOC_CTRL_ADD		_IOW('U', 1, struct uvc_xu_control_info)
#define UVCIOC_CTRL_MAP		_IOWR('U', 2, struct uvc_xu_control_mapping)
#define UVCIOC_CTRL_GET		_IOWR('U', 3, struct uvc_xu_control)
#define UVCIOC_CTRL_SET		_IOW('U', 4, struct uvc_xu_control)

#define LENGTH_OF_SONIX_XU_CTR (3)
#define LENGTH_OF_SONIX_XU_MAP (3)
//include/uapi/linux/usb/video.h
#define UVC_RC_UNDEFINED                                0x00
#define UVC_SET_CUR                                     0x01
#define UVC_GET_CUR                                     0x81
#define UVC_GET_MIN                                     0x82
#define UVC_GET_MAX                                     0x83
#define UVC_GET_RES                                     0x84
#define UVC_GET_LEN                                     0x85
#define UVC_GET_INFO                                    0x86
#define UVC_GET_DEF                                     0x87

enum DumpType{
	Message_D1 = 0x01,
	Message_D2 = 0x02,
	Message_D3 = 0x04
};
//#define DumpMessage (Message_D1|Message_D2|Message_D3)
#define DumpMessage (Message_D1)


// Extension Unit AU38xx GUID {68bbd0b0-61a4-4b83-90b7-a6215f3c4f70}
#define UVC_GUID_AU38xx_XU       {0xb0, 0xd0, 0xbb, 0x68, 0xa4, 0x61, 0x83, 0x4b, 0x90, 0xb7, 0xa6, 0x21, 0x5f, 0x3c, 0x4f, 0x70}

//Extension Unit M560xx GUID  {A7974C56-A77E-4B90-8CBF-1C71EC303000}
#define UVC_GUID_M56xx_XU	       {0x56, 0x4C, 0x97, 0xA7, 0x7E, 0xA7, 0x90, 0x4B, 0x8C, 0xBF, 0x1C, 0x71, 0xEC, 0x30, 0x30, 0x00}
	
#define XU_UNIT_ID		(6)	//AU38xx and M56xx
	
	//===================================
	//=============   AU38xx ===============
	
#define XU_I2C_ADDR					(0x01)
#define XU_I2C_DATA					(0x02)
#define XU_EEPROM_RW    			(0x03)
#define XU_MIRROR_CMD				(0x04)
#define XU_FLIP_CMD					(0x05)
#define XU_REG_SEQUENCIAL_RW		(0x06)
#define XU_SENSOR_SEQUENCIAL_RW	    (0x07)
#define XU_8051_SEQUENCIAL_RW		(0x08)
#define XU_UPGRADE_INTER_ROM		(0x09)
#define XU_GET_HW_VER				(0x0A)
#define XU_EEPROM_TYPE				(0x0B)
#define XU_GET_SET_VER				(0x0C)
#define XU_GET_FW_VER				(0x0D)
#define XU_WRITE_PROTECT			(0x0E)
#define XU_EEPROM_CHECKSUM		    (0x11)
#define XU_SPIFLASH_STATUS			(0x12)
	
	//=============   AU38xx end ============
	//===================================
	
	//===================================
	//=============   M56xx ================
	
#define XU_CTRL_M56_offset			0x30
	//100923 start: CP
#define XU_CTRL_M56_ADDR			8
#define XU_CTRL_M56_DATA			9	// not use
#define XU_CTRL_M56_ASIC_RW		    10
#define XU_CTRL_M56_EEPROM_RW	    11
#define XU_CTRL_M56_DATA_RW		    12
#define XU_CTRL_M56_SENSOR_RW	    13
#define XU_CTRL_M56_SPI			    14
#define XU_CTRL_M56_FW			    15
	//100923 end
	
	//-------  XU_CTRL_M56_FW command ---------
#define FW_REG_CMD_SET			    0
#define FW_REG_RW_CNT				1
#define FW_REG_MEMSRC				2
#define FW_REG_MEMSRC_EXT			3
#define FW_REG_ERASE_CMD			4
#define FW_REG_DMA_EN				5
#define FW_REG_EXT_ADDR_H			6
#define FW_REG_EXT_ADDR_L			7
	//SPI
#define FW_REG_SPI_SDAT			8
#define FW_REG_SPI_RDAT			9
#define FW_REG_SPI_CTRL			10
#define FW_REG_SPI_READ			11
	//CRC
#define FW_REG_CRC_CTRL			12
#define FW_REG_CRC_CHECKSUM_H	13
#define FW_REG_CRC_CHECKSUM_L	14
#define FW_REG_CRC_BYTECNT_H		15
#define FW_REG_CRC_BYTECNT_L		16
#define FW_REG_CRC_STATUS			17
	
#define FW_REG_PAGE_SIZE			18
#define FW_REG_I2C_RETRY			19
	
#define FW_REG_SPI_CLK				20
#define FW_REG_I2CE_CLK			21
#define FW_REG_I2CS_CLK			22
#define FW_REG_I2CS_TYPE			23
#define FW_REG_I2CS_ID				24
#define FW_REG_I2CS_MODE			25
#define FW_REG_5608_EN				26
#define FW_REG_WRITE_CYCLE_MS	27
	
#define FW_CMD_SET_NULL_ROMFIX	0xF5
#define FW_CMD_GET_ROM_VERSION	0xF6
#define FW_CMD_SET_NULL_FUNC		0xF7
#define FW_CMD_SNAPSHOT_PRESS	0xF8
#define FW_CMD_SNAPSHOT_RELEASE	0xF9
#define FW_CMD_MCU_CLOCK			0xFA
#define FW_REG_SYS_FLAG			0xFB
#define FW_CMD_WRITE_ENABLE		0xFC
#define FW_CMD_WRITE_DISABLE		0xFD
#define FW_CMD_CRC_CHECKING		0xFE
#define FW_CMD_REBOOT				0xFF
	//-------  XU_CTRL_M56_FW command end ---------
	
#define _EXT_I2C_MODE_0_				(0x40)	//mode 0		//EEPROM 256 ~ 2K
#define _EXT_I2C_MODE_1_				(0x41)	//mode 1		//EEPROM 4K ~ 16K
	
	
#define EEPROM_HEADER			0x560855AA
#define EE_ADDR_HEADER_0		0x0000		//FIX
#define EE_ADDR_HEADER_1		0x0004
#define EE_HEADER_LENGTH		0x0008
	
	//=============   M56xx end =============
	
	
	//===================================
	
	//===================================
	//======== XU mapping id =================
#define V4L2_CID_BASE_EXTCTR					0x0A046D01
#define V4L2_CID_BASE_ALCOR					V4L2_CID_BASE_EXTCTR
	//-------------- AU38xx ----------------
#define V4L2_CID_XU_I2C_ADDR					V4L2_CID_BASE_ALCOR+XU_I2C_ADDR
#define V4L2_CID_XU_I2C_DATA					V4L2_CID_BASE_ALCOR+XU_I2C_DATA
#define V4L2_CID_XU_EEPROM_RW					V4L2_CID_BASE_ALCOR+XU_EEPROM_RW
#define V4L2_CID_XU_MIRROR_CMD				V4L2_CID_BASE_ALCOR+XU_MIRROR_CMD
#define V4L2_CID_XU_FLIP_CMD					V4L2_CID_BASE_ALCOR+XU_FLIP_CMD
#define V4L2_CID_XU_REG_SEQUENCIAL_RW		V4L2_CID_BASE_ALCOR+XU_REG_SEQUENCIAL_RW
#define V4L2_CID_XU_SENSOR_SEQUENCIAL_RW	V4L2_CID_BASE_ALCOR+XU_SENSOR_SEQUENCIAL_RW
#define V4L2_CID_XU_8051_SEQUENCIAL_RW		V4L2_CID_BASE_ALCOR+XU_8051_SEQUENCIAL_RW
#define V4L2_CID_XU_UPGRADE_INTER_ROM		V4L2_CID_BASE_ALCOR+XU_UPGRADE_INTER_ROM
#define V4L2_CID_XU_GET_HW_VER				V4L2_CID_BASE_ALCOR+XU_GET_HW_VER
#define V4L2_CID_XU_EEPROM_TYPE				V4L2_CID_BASE_ALCOR+XU_EEPROM_TYPE
#define V4L2_CID_XU_GET_SET_VER				V4L2_CID_BASE_ALCOR+XU_GET_SET_VER
#define V4L2_CID_XU_GET_FW_VER				V4L2_CID_BASE_ALCOR+XU_GET_FW_VER
#define V4L2_CID_XU_WRITE_PROTECT				V4L2_CID_BASE_ALCOR+XU_WRITE_PROTECT
#define V4L2_CID_XU_EEPROM_CHECKSUM			V4L2_CID_BASE_ALCOR+XU_EEPROM_CHECKSUM
#define V4L2_CID_XU_SPIFLASH_STATUS			V4L2_CID_BASE_ALCOR+XU_SPIFLASH_STATUS
	//-------------- M56xx ----------------
#define V4L2_CID_XU_CTRL_M56_ADDR				V4L2_CID_BASE_ALCOR+XU_CTRL_M56_offset+XU_CTRL_M56_ADDR
#define V4L2_CID_XU_CTRL_M56_DATA				V4L2_CID_BASE_ALCOR+XU_CTRL_M56_offset+XU_CTRL_M56_DATA	// not use
#define V4L2_CID_XU_CTRL_M56_ASIC_RW			V4L2_CID_BASE_ALCOR+XU_CTRL_M56_offset+XU_CTRL_M56_ASIC_RW
#define V4L2_CID_XU_CTRL_M56_EEPROM_RW		V4L2_CID_BASE_ALCOR+XU_CTRL_M56_offset+XU_CTRL_M56_EEPROM_RW
#define V4L2_CID_XU_CTRL_M56_DATA_RW			V4L2_CID_BASE_ALCOR+XU_CTRL_M56_offset+XU_CTRL_M56_DATA_RW
#define V4L2_CID_XU_CTRL_M56_SENSOR_RW		V4L2_CID_BASE_ALCOR+XU_CTRL_M56_offset+XU_CTRL_M56_SENSOR_RW
#define V4L2_CID_XU_CTRL_M56_SPI				V4L2_CID_BASE_ALCOR+XU_CTRL_M56_offset+XU_CTRL_M56_SPI
#define V4L2_CID_XU_CTRL_M56_FW				V4L2_CID_BASE_ALCOR+XU_CTRL_M56_offset+XU_CTRL_M56_FW
	//======== XU mapping id end ==============
	//===================================
	
	
#define BACKEND_TYPE_NULL		0
#define BACKEND_TYPE_M5608D		2
#define BACKEND_TYPE_S5608U		3
#define BACKEND_TYPE_M5608U		4
#define BACKEND_TYPE_CK			7
#define BACKEND_TYPE_OTHER		10
#define BACKEND_TYPE_SEGFW		11
#define BACKEND_TYPE_MORGAN		12
#define BACKEND_TYPE_M5608T		13
	
#define ExtMemPageSize  		8	// don't modify , only one vale
#define BURST_WRITE_LEN			8
	
#define UVC_DEVINFO_FILE "/sys/class/video4linux/video1/device/input/input"
#define ID_VENDOR "/id/vendor"
#define ID_PRODUCT "/id/product"

typedef struct {
	int dev_vendor;
	int dev_product;
	int dev_id;
} UVC_DEV_INFO, *pUVC_DEV_INFO;

enum {
	DEV_SONGHAN = 1,
	DEV_ANGUO,
};

enum
{
	REVERSE_EVENT_OFF = 0,
	REVERSE_EVENT_ON
};

typedef struct{
	int 		fd;
	char*	devname;
}V4L_DEVINFO, *pV4L_DEVINFO;

// for M56
typedef struct _EXTENSION_DATA_ITEM {
	uint8_t data0;	//LO Byte
	uint8_t data1;	//HI Byte
} EXTENSION_DATA_ITEM, *PEXTENSION_DATA_ITEM;

using namespace android;

class ReverseEvent
{
public:
	virtual ~ReverseEvent(){}
	virtual void reverseEvent(int cmd, int arg)=0;
};

class ReverseBase
{
public:
       virtual ~ReverseBase(){}
       virtual void setListener(ReverseEvent *re)=0;
	   virtual int setEnable(bool bEnable)=0;
};

class BackCarMonitor:public ReverseBase
{
public:
       BackCarMonitor();
       ~BackCarMonitor() {
			db_msg("~~BackCarMonitor");
			mTT->stopThread();
			mTT.clear();
			mTT = NULL;
       }
       int XUCtrlGetValue(int32_t controlId);
       int XUAsicGetValue(int addr, uint8_t *uData);
       int getBackCarState();
       int openCamera();
       bool inited;
       void setListener(ReverseEvent *re);
       void TestLoop();
       int XUInitCtrl();
       int getDevId();
       int initCamera();
	   int setEnable(bool bEnable);
	   void releaseCamera();
       class TestThread : public Thread
       {
       public: TestThread(BackCarMonitor *bm) : Thread(false), mBM(bm) { }
	       void startThread()
	       {
		       run("UeventThread", PRIORITY_NORMAL);
	       }
	       void stopThread()
	       {
		       requestExitAndWait();
	       }
	       bool threadLoop()
	       {
		       mBM->TestLoop();
			       return false;
	       }
       private:
	       BackCarMonitor *mBM;
       };
private:
	pUVC_DEV_INFO mDevInfo;
	int mDevFd;	
	int unit;
	ReverseEvent *mRE;
	sp<TestThread> mTT;
	bool mEnable;
};
#endif
