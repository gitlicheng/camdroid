#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdint.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <utils/String8.h>

#include "BackCarMonitor.h" 
#include "ConfigData.h"
#include "debug.h"

#undef LOG_TAG
#define LOG_TAG "BackCarMonitor"


using namespace android;

UVC_DEV_INFO uvc_devs[] = {
	{0x0c45, 0x64ab, DEV_SONGHAN},
	{0x058f, 0x3842, DEV_ANGUO},
};

static struct uvc_xu_control_info sonix_xu_ctrls[] =
{
    {
        UVC_GUID_SONIX_USER_HW_CONTROL,
        0,
        XU_SONIX_ASIC_CTRL,
        4,
        UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX |
                    UVC_CONTROL_GET_DEF | UVC_CONTROL_AUTO_UPDATE | UVC_CONTROL_GET_CUR
    },
    {
        UVC_GUID_SONIX_USER_HW_CONTROL,
        1,
        XU_SONIX_I2C_CTRL,
        8,
        UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
    },
    {
        UVC_GUID_SONIX_USER_HW_CONTROL,
        2,
        XU_SONIX_SF_READ,
        11,
        UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
    },

};

//  SONiX XU Ctrls Mapping
static struct uvc_xu_control_mapping sonix_xu_mappings[] =
{
    {
        V4L2_CID_ASIC_CTRL_SONIX,
        "SONiX: Asic Control",
        UVC_GUID_SONIX_USER_HW_CONTROL,
        XU_SONIX_ASIC_CTRL,
        4,
        0,
        V4L2_CTRL_TYPE_INTEGER,
        UVC_CTRL_DATA_TYPE_SIGNED
    },
    {
        V4L2_CID_I2C_CTRL_SONIX,
        "SONiX: I2C Control",
        UVC_GUID_SONIX_USER_HW_CONTROL,
        XU_SONIX_I2C_CTRL,
        8,
        0,
        V4L2_CTRL_TYPE_INTEGER,
        UVC_CTRL_DATA_TYPE_UNSIGNED
    },
    {
        V4L2_CID_SF_READ_SONIX,
        "SONiX: Serial Flash Read",
        UVC_GUID_SONIX_USER_HW_CONTROL,
        XU_SONIX_SF_READ,
        11,
        0,
        V4L2_CTRL_TYPE_INTEGER,
        UVC_CTRL_DATA_TYPE_UNSIGNED
    },


};
static int file_exist(char *path)
{
	if (access(path, F_OK) == 0) {
		return 1;
	} else
		return -1;
}

static void get_oneline(const char *pathname, String8 &val)
{
	FILE *fp;
	char buf[256];	

	if (access(pathname, R_OK) != 0) {
		db_error("%s can not read: %s\n", pathname, strerror(errno));
		val = "";
		return ;
	}		

	fp = fopen(pathname, "r");

	if (fp == NULL) {
		db_error("open %s failed: %s\n", pathname, strerror(errno));
		val = "";
		return ;
	}
	if (fgets(buf, sizeof(buf), fp) != NULL) {
		fclose(fp);
		val = buf;
		return ;
	}
	val = "";
	fclose(fp);
}

int BackCarMonitor::getDevId()
{
	int i ;
	char pathname[64];
	String8 val;
	for (i = 0; i < 20; i++) {
		sprintf(pathname, "%s%d%s", UVC_DEVINFO_FILE, i, ID_VENDOR);
		if (file_exist(pathname) != -1) {
			get_oneline(pathname, val);
			db_msg("val_vendor:%s\n", val.string());
			mDevInfo->dev_vendor  = strtol(val.string(), NULL, 16);
			memset(pathname, 0, strlen(pathname));
			sprintf(pathname, "%s%d%s", UVC_DEVINFO_FILE, i, ID_PRODUCT);
			get_oneline(pathname, val);
			db_msg("val_product:%s\n", val.string());
			mDevInfo->dev_product = strtol(val.string(), NULL, 16);
		} else {
			memset(pathname, 0, strlen(pathname));
			continue;
		}
	}
	for (i = 0; i < sizeof(uvc_devs) / sizeof(uvc_devs[0]); i++) {
		if (uvc_devs[i].dev_vendor == mDevInfo->dev_vendor &&
		uvc_devs[i].dev_product == mDevInfo->dev_product) {
			return i;
		}
	}
	return -1;
}

static int xioctl(int  fd, int  request,void *arg)
{
        int r;
        do r = ioctl (fd, request, arg);
        while (-1 == r && EINTR == errno);

        return r;
}
// Init camera device
int BackCarMonitor::initCamera()
{
	mDevFd = openCamera();
	if (mDevFd < 0) {
		return mDevFd;
	}
	mDevInfo = (pUVC_DEV_INFO)malloc(sizeof(UVC_DEV_INFO));
	mDevInfo->dev_id = getDevId();
	if (mDevInfo->dev_id == 0) {
		db_msg("use songhan's uvc\n");
	} else if (mDevInfo->dev_id == 1) {
		db_msg("use anguo's uvc\n");
	}
	return 0;
}

static int getNode()
{
	char dev[32];
	int i = 0;
	int ret;
	struct v4l2_capability cap; 
	for (i = 0; i < 8; ++i) {
		sprintf(dev, "/dev/video%d", i);
		if (!access(dev, F_OK)) {
			int fd = open(dev, O_RDWR | O_NONBLOCK, 0);
			if (fd < 0) {
				continue;
			}
			ret = ioctl (fd, VIDIOC_QUERYCAP, &cap);
			if (ret < 0) {
				close(fd);
				continue;
			}
			if (strcmp((char*)cap.driver, "uvcvideo") == 0) {
				close(fd);
				return i;
			}
			close(fd);
			fd = -1;
		}
	}
	return 1;
}

// Open camera device
int BackCarMonitor::openCamera()
{
	char devicename[32];
	int fd;
	int idx = getNode();
	sprintf(devicename, "/dev/video%d", idx);
	fd = open(devicename, O_RDWR | O_NONBLOCK, 0);
	if (fd == -1)
		db_msg("can't open /dev/video%d: %s", idx, strerror(errno));
	else 
		return fd;
	return -1;
}
// use for songhan uvc
int BackCarMonitor::XUInitCtrl()
{
    int i=0;
    int err=0;
    /* try to add all controls listed above */
    for ( i=0; i<LENGTH_OF_SONIX_XU_CTR; i++ )
    {
        db_msg("Adding XU Ctrls - %s\n", sonix_xu_mappings[i].name);
        if ((err=ioctl(mDevFd, UVCIOC_CTRL_ADD, &sonix_xu_ctrls[i])) < 0 )
        {
            if ((errno == EEXIST) || (errno != EACCES))
            {
                db_msg("UVCIOC_CTRL_ADD - Ignored, uvc driver had already defined\n");
               // return (-EEXIST);
               continue;
            }
            else if (errno == EACCES)
            {
                db_msg("Need admin previledges for adding extension unit(XU) controls\n");
                db_msg("please run 'SONiX_UVC_TestAP --add_ctrls' as root (or with sudo)\n");
            //    return  (-1);
				continue;
            }
            else db_msg("Control exists");
        }
    }
    /* after adding the controls, add the mapping now */
    for ( i=0; i<LENGTH_OF_SONIX_XU_MAP; i++ )
    {
        db_msg("Mapping XU Ctrls - %s\n", sonix_xu_mappings[i].name);
        if ((err=ioctl(mDevFd, UVCIOC_CTRL_MAP, &sonix_xu_mappings[i])) < 0)
        {
            if ((errno!=EEXIST) || (errno != EACCES))
            {
                db_msg("UVCIOC_CTRL_MAP - Error");
             //   return (-2);
				continue;
            }
            else if (errno == EACCES)
            {
                db_msg("Need admin previledges for adding extension unit(XU) controls\n");
                db_msg("please run 'SONiX_UVC_TestAP --add_ctrls' as root (or with sudo)\n");
             //   return  (-1);
				continue;
            }
            else db_msg("Mapping exists");
        }
    }
    return 0;
}

// use for songhan uvc
int BackCarMonitor::XUAsicGetValue(int addr, uint8_t *uData)
{
	//    db_info("XU_ASIC_Get_Data ==>\n");
	int i = 0;
	int ret = 0;
	int err = 0;
	uint8_t ctrldata[4]={0};

	struct uvc_xu_control xctrl = {
		//3,										/* bUnitID 				*/
		unit,
		XU_SONIX_ASIC_CTRL,					   /* function selector	*/
		4,										/* data size			*/
		(__u8*)&ctrldata								/* *data				*/
	};

	// Switch command
	xctrl.data[0] = addr & 0xFF;				// Tag
	xctrl.data[1] = (addr & 0xFF00)>>8;
	xctrl.data[2] = 0x0;
	xctrl.data[3] = 0xff;                       // Dummy

	if ((err=ioctl(mDevFd, UVCIOC_CTRL_SET, &xctrl)) < 0)
	{
		db_msg("XU_ASIC_Get_Data ==> Switch cmd : ioctl(UVCIOC_CTRL_SET) FAILED (%i)11 == \n",err);
		if (err==EINVAL) db_msg("Invalid arguments\n");
		return err;
	}

	if ((err=ioctl(mDevFd, UVCIOC_CTRL_GET, &xctrl)) < 0)
	{
		db_msg("XU_ASIC_Get_Data ==> ioctl(UVCIOC_CTRL_GET) FAILED (%i) == \n",err);
		if(err==EINVAL) db_msg("Invalid arguments\n");
		return err;
}

//    db_info("   == XU_ASIC_Get_Data Success == \n");
//    for(i=0; i<4; i++)
//        db_info("      Get data[%d] : 0x%x\n", i, xctrl.data[i]);
	*uData = xctrl.data[2];
	return ret;
}

int BackCarMonitor::XUCtrlGetValue(int32_t controlId)
{
	//an array of v4l2_ext_control
	struct v4l2_ext_control clist[1];
	clist[0].id      = controlId;
	clist[0].value = 0;

	//v4l2_ext_controls with list of v4l2_ext_control
	struct v4l2_ext_controls ctrls = {0};
	ctrls.ctrl_class = controlId & 0xFFFF0000;
	ctrls.count = 1;
	ctrls.controls = clist;

	//read back the value
	if (-1 == ioctl (mDevFd, VIDIOC_G_EXT_CTRLS, &ctrls))
	{
		//..... error handling
		fprintf(stderr, "[V4L2]::%s:%d : VIDIOC_G_EXT_CTRLS id=%08x FAILED\n", __FUNCTION__, __LINE__,
			controlId);
		return -1;
	}

	fprintf(stderr, "[V4L2]::%s:%d : VIDIOC_G_EXT_CTRLS id=%08x => val=%d\n", __FUNCTION__, __LINE__,
		controlId, clist[0].value);

	return clist[0].value;
}


int BackCarMonitor::getBackCarState()
{
	uint8_t uData;
	int ret = -1;
	int value = -1;
	if (mDevFd == -1) {
		db_msg("can't open uvc device\n");
		return value;
	}
	switch (mDevInfo->dev_id) {
	case 0:
		ret = XUAsicGetValue(0x101f, &uData);
		if(ret < 0){
			//db_msg("change uint read again\n");
			unit = 4;
			ret = XUAsicGetValue(0x101f, &uData);
		}
		//db_msg("UVC chip ID is %x\n", uData);
		ret = XUAsicGetValue(0x1005, &uData);
		if(ret < 0){
			//db_msg("change uint read again	\r\n");
			if (unit == 3) unit = 4;
			else unit = 3;
		    ret = XUAsicGetValue(0x1005, &uData);
			if(ret < 0 )
				return -1;
		}
		value = (uData & 0x01);
		break;
	case 1:
		value = XUCtrlGetValue(V4L2_CID_BACKLIGHT_COMPENSATION);
		break;
	default:
		break;
	}
	return value;
}

void BackCarMonitor::setListener(ReverseEvent *re)
{
	mRE = re;
}

void BackCarMonitor::TestLoop()
{
	int on_flag = 0;
	int ret = -1;
	char reverseDebug[4]={0};
	cfgDataGetString("persist.reverse.mode",reverseDebug,"0");
	int debug_flag = (atoi(reverseDebug) == 1);
	while(1) {
		usleep(500 * 1000);
		ret = getBackCarState();
		if (debug_flag) {
			db_msg("back car state is:%d\n", ret);
		}
		if (ret == 1 && on_flag == 0) {
			mRE->reverseEvent(REVERSE_EVENT_ON, 0);
			on_flag = 1;
		} else if (ret == 0 && on_flag == 1) {
			mRE->reverseEvent(REVERSE_EVENT_OFF, 0);
			on_flag = 0;
		}
		if (!mEnable) {
			break;
		}
	}
}

BackCarMonitor::BackCarMonitor():unit(3), mEnable(false)
{
	db_msg("constructor\n");
	mTT = new TestThread(this);
	
}

void BackCarMonitor::releaseCamera()
{
	if(mDevFd != NULL){
	db_msg("close uvc device\n");
		close(mDevFd);
	}
	free(mDevInfo);
}

int BackCarMonitor::setEnable(bool bEnable)
{
	int ret = 0;
	mEnable = bEnable;
	if (bEnable) {
		ret = initCamera();
		if (ret < 0) {
			return ret;
		}
		mTT->startThread();
	} else {
		mTT->stopThread();
		releaseCamera();
	}
	return ret;
}

//void BackCarMonitor::TestLoop()
//{
//	int on_flag = 0;
//	char reverseConfig[4]={0};
//	sleep(5);
//	while(1) {
//		usleep(500 * 1000);
//		cfgDataGetString("persist.reverse.mode",reverseConfig,"0");
//		if (atoi(reverseConfig) == 1 && on_flag == 0) {
//			mRE->reverseEvent(REVERSE_EVENT_ON, 0);
//			on_flag = 1;
//		} else if (atoi(reverseConfig) == 0 && on_flag == 1) {
//			mRE->reverseEvent(REVERSE_EVENT_OFF, 0);
//			on_flag = 0;
//		}
//	}
//}

