/*
**
** Copyright (C) 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <cutils/log.h>
#include <cutils/properties.h>
#include <string.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
 
#include "hwdisplay.h"

#ifdef LOG_TAG  
#undef LOG_TAG
#define LOG_TAG "HwDisplay"
#endif

#define gLogLevel 1
#define LOG1(...) ALOGD_IF(gLogLevel >= 1, __VA_ARGS__);
#define LOG2(...) ALOGD_IF(gLogLevel >= 2, __VA_ARGS__);

#define _p1() LOG1("_f(%s), _l(%d)", __FUNCTION__, __LINE__)
#define _p2() LOG2("_f(%s), _l(%d)", __FUNCTION__, __LINE__)

namespace android {

Mutex HwDisplay::sLock;
bool HwDisplay::mInitialized = false;
HwDisplay *HwDisplay::sHwDisplay = NULL;

HwDisplay::HwDisplay()
	: mDisp_fd(-1)
{
	LOG1("(%s %d)\n", __FUNCTION__, __LINE__);
	memset(mLayerStatus, 0, sizeof(mLayerStatus));
	hwd_init();
}

HwDisplay::~HwDisplay()
{
	if (mInitialized) {
		hwd_exit();
		mInitialized = false;
	}
}

#ifdef DE_2

int HwDisplay::layer_cmd(unsigned int hlay)	//finish
{
	return layer_cmd(mScreen, hlay);
}

int HwDisplay::layer_cmd(int screen, unsigned int hlay)
{
/*
	LOG1("(%s %d) %d\n", __FUNCTION__, __LINE__, hlay);
	unsigned long args[4] = {0};

	switch(cmd) {
		case DISP_CMD_LAYER_TOP:
			break;
		case DISP_CMD_LAYER_OPEN:
			break;
		case DISP_CMD_LAYER_CLOSE:
			break;
		case DISP_CMD_LAYER_CK_ON:
			break;
		case DISP_CMD_LAYER_CK_OFF:
			break;
		case DISP_CMD_LAYER_OPEN:
			break;
		case DISP_CMD_LAYER_CLOSE:
			break;
	}
	args[0] = mScreen;
	args[1] = hlay;
	return ioctl(mDisp_fd, cmd, args);
*/
	return 0;
}

int HwDisplay::layer_request(int *pCh, int *pId)	//finish
{
	LOG1("(%s %d)\n", __FUNCTION__, __LINE__);
	int ch;
	int id;
	{
		Mutex::Autolock lock(sLock);
		for(id=0; id<LYL_NUM; id++) {
			for (ch=0; ch<CHN_NUM; ch++) {
				if (!(mLayerStatus[ch][id] & HWD_STATUS_REQUESTED)) {
					mLayerStatus[ch][id] |= HWD_STATUS_REQUESTED;
					goto out;
				}
			}
		}
	}
out:
	if ((ch==CHN_NUM) && (id==LYL_NUM)) {
		ALOGE("all layer used.");
		return RET_FAIL;
	}
	*pCh = ch;
	*pId = id;
	mCurLayerCnt++;
	LOG1("requested: ch:%d, id:%d\n", ch, id);
	return HLAY(ch, id);
}

int HwDisplay::layer_request(int screen)
{
	return 0;
	//return layer_cmd(screen, DISP_LAYER_WORK_MODE_SCALER, DISP_CMD_LAYER_REQUEST);
}

int HwDisplay::layer_release(int hlay)
{
	return hwd_layer_close(hlay);
	//return layer_cmd(hlay, DISP_CMD_LAYER_RELEASE);
}

int HwDisplay::layer_config(__DISP_t cmd, disp_layer_config *pinfo)
{
	LOG2("(%s %d)\n", __FUNCTION__, __LINE__);
	unsigned long args[4] = {0};
	unsigned int ret = RET_OK;
	
	args[0] = mScreen;
    args[1] = (unsigned long)pinfo;
    args[2] = 1;
    ret = ioctl(mDisp_fd, cmd, args);
	if(RET_OK != ret) {
		ALOGE("fail to get para\n");
		ret = RET_FAIL;
	}
	return ret;
}

int HwDisplay::layer_get_para(disp_layer_config *pinfo)
{
	return layer_config(DISP_LAYER_GET_CONFIG, pinfo);
}

int HwDisplay::layer_set_para(disp_layer_config *pinfo)
{
	return layer_config(DISP_LAYER_SET_CONFIG, pinfo);
}

#define ALIGN_16B(x) (((x) + (15)) & ~(15))

int HwDisplay::hwd_layer_set_src(unsigned int hlay, struct src_info *src)
{
	LOG1("(%s %d)\n", __FUNCTION__, __LINE__);
	unsigned long args[4] = {0};
	disp_layer_config config;
	
	memset(&config, 0, sizeof(disp_layer_config));
	config.channel  = HD2CHN(hlay);
	config.layer_id = HD2LYL(hlay);
	layer_get_para(&config);

	config.info.fb.crop.x = src->crop_x;
	config.info.fb.crop.y = src->crop_y;
	config.info.fb.crop.width  = (src->crop_w);
	config.info.fb.crop.height = (src->crop_h);

    	LOG1("width: 0x%llx, height: 0x%llx", config.info.fb.crop.width, config.info.fb.crop.height);
    
    	config.info.fb.crop.x = config.info.fb.crop.x << 32;
	config.info.fb.crop.y = config.info.fb.crop.y << 32;
    	config.info.fb.crop.width  = config.info.fb.crop.width << 32;
    	config.info.fb.crop.height = config.info.fb.crop.height << 32;

    	LOG1("width: 0x%llx, height: 0x%llx", config.info.fb.crop.width, config.info.fb.crop.height);
    
	config.info.fb.size[0].width  = src->w;
	config.info.fb.size[0].height = src->h; 
	switch(src->format) {
		case HWC_FORMAT_YUV420PLANAR:
			config.info.fb.format = DISP_FORMAT_YUV420_P;
			/*
			config.info.fb.addr[0] = (int)(config.info.fb.addr[0] );
			config.info.fb.addr[1] = (int)(config.info.fb.addr[0] + fb_width*fb_height);
			config.info.fb.addr[2] = (int)(config.info.fb.addr[0] + fb_width*fb_height*5/4);
			*/
			config.info.fb.size[1].width    = config.info.fb.size[0].width / 2;
			config.info.fb.size[1].height   = config.info.fb.size[0].height / 2;
			config.info.fb.size[2].width    = config.info.fb.size[0].width / 2;
			config.info.fb.size[2].height   = config.info.fb.size[0].height / 2;
			break;
		case HWC_FORMAT_YUV420UVC:
			config.info.fb.format = DISP_FORMAT_YUV420_SP_UVUV;
			config.info.fb.size[1].width    = config.info.fb.size[0].width / 2;
			config.info.fb.size[1].height   = config.info.fb.size[0].height / 2;
			//config.info.fb.size[2].width    = config.info.fb.size[0].width / 2;
			//config.info.fb.size[2].height   = config.info.fb.size[0].height / 2;
			break;
		case HWC_FORMAT_YUV420VUC:
			config.info.fb.format = DISP_FORMAT_YUV420_SP_VUVU;
			config.info.fb.size[1].width  = config.info.fb.size[0].width/2;
			config.info.fb.size[1].height = config.info.fb.size[0].height/2;
			//config.info.fb.size[2].width  = config.info.fb.size[0].width/2;
			//config.info.fb.size[2].height = config.info.fb.size[0].height/2;
			break;
		case HWC_FORMAT_YUV444PLANAR:
			config.info.fb.format = DISP_FORMAT_YUV444_P;
			/*
			config.info.fb.addr[0] = (int)(config.info.fb.addr[0]);
			config.info.fb.addr[1] = (int)(config.info.fb.addr[0] + fb_width*fb_height);
			config.info.fb.addr[2] = (int)(config.info.fb.addr[0] + fb_width*fb_height*2);
			*/
			config.info.fb.size[1].width    = config.info.fb.size[0].width;
			config.info.fb.size[1].height   = config.info.fb.size[0].height;
			config.info.fb.size[2].width    = config.info.fb.size[0].width;
			config.info.fb.size[2].height   = config.info.fb.size[0].height;
			break;
		case HWC_FORMAT_YUV422PLANAR:
			config.info.fb.format = DISP_FORMAT_YUV422_P;
			/*
			config.info.fb.addr[0] = (int)(config.info.fb.addr[0] );
			config.info.fb.addr[1] = (int)(config.info.fb.addr[0] + fb_width*fb_height);
			config.info.fb.addr[2] = (int)(config.info.fb.addr[0] + fb_width*fb_height*3/2);
			*/
			config.info.fb.size[1].width    = config.info.fb.size[0].width / 2;
			config.info.fb.size[1].height   = config.info.fb.size[0].height;
			config.info.fb.size[2].width    = config.info.fb.size[0].width / 2;
			config.info.fb.size[2].height   = config.info.fb.size[0].height;
			break;
		case HWC_FORMAT_YUV422UVC:
			config.info.fb.format = DISP_FORMAT_YUV422_SP_UVUV;
			config.info.fb.size[1].width    = config.info.fb.size[0].width / 2;
			config.info.fb.size[1].height   = config.info.fb.size[0].height;
			config.info.fb.size[2].width    = config.info.fb.size[0].width / 2;
			config.info.fb.size[2].height   = config.info.fb.size[0].height;
			break;
		case HWC_FORMAT_YUV422VUC:
			config.info.fb.format = DISP_FORMAT_YUV422_SP_VUVU;
			config.info.fb.size[1].width    = config.info.fb.size[0].width / 2;
			config.info.fb.size[1].height   = config.info.fb.size[0].height;
			config.info.fb.size[2].width    = config.info.fb.size[0].width / 2;
			config.info.fb.size[2].height   = config.info.fb.size[0].height;
			break;
		case HWC_FORMAT_YUV411PLANAR:
			config.info.fb.format = DISP_FORMAT_YUV411_P;
			/*
			config.info.fb.addr[0] = (int)(config.info.fb.addr[0] );
			config.info.fb.addr[1] = (int)(config.info.fb.addr[0] + fb_width*fb_height);
			config.info.fb.addr[2] = (int)(config.info.fb.addr[0] + fb_width*fb_height*5/4);
			*/
			config.info.fb.size[1].width    = config.info.fb.size[0].width / 4;
			config.info.fb.size[1].height   = config.info.fb.size[0].height;
			config.info.fb.size[2].width    = config.info.fb.size[0].width / 4;
			config.info.fb.size[2].height   = config.info.fb.size[0].height;
			break;
		case HWC_FORMAT_YUV411UVC:
			config.info.fb.format = DISP_FORMAT_YUV411_SP_UVUV;
			config.info.fb.size[1].width    = config.info.fb.size[0].width / 4;
			config.info.fb.size[1].height   = config.info.fb.size[0].height;
			config.info.fb.size[2].width    = config.info.fb.size[0].width / 4;
			config.info.fb.size[2].height   = config.info.fb.size[0].height;
			break;
		case HWC_FORMAT_YUV411VUC:
			config.info.fb.format = DISP_FORMAT_YUV411_SP_VUVU;
			config.info.fb.size[1].width    = config.info.fb.size[0].width / 4;
			config.info.fb.size[1].height   = config.info.fb.size[0].height;
			config.info.fb.size[2].width    = config.info.fb.size[0].width / 4;
			config.info.fb.size[2].height   = config.info.fb.size[0].height;
			break;
		default:
			/* INTERLEAVED */
			/*
			config.info.fb.addr[0] = (int)(config.info.fb.addr[0] + fb_width*fb_height/3*0);
			config.info.fb.addr[1] = (int)(config.info.fb.addr[0] + fb_width*fb_height/3*1);
			config.info.fb.addr[2] = (int)(config.info.fb.addr[0] + fb_width*fb_height/3*2);
			*/
			config.info.fb.format = DISP_FORMAT_ARGB_8888;
			config.info.fb.size[1].width    = config.info.fb.size[0].width;
			config.info.fb.size[1].height   = config.info.fb.size[0].height;
			config.info.fb.size[2].width    = config.info.fb.size[0].width;
			config.info.fb.size[2].height   = config.info.fb.size[0].height;
			break;
	}
	LOG1("set fb.format %d %d\n", src->format, config.info.fb.format);
	return layer_set_para(&config);
}


int HwDisplay::hwd_layer_set_rect(unsigned int hlay, struct view_info *view)
{
	LOG1("(%s %d)\n", __FUNCTION__, __LINE__);
	disp_layer_config config;
		
	memset(&config, 0, sizeof(disp_layer_config));
	config.channel	= HD2CHN(hlay);
	config.layer_id = HD2LYL(hlay);
	layer_get_para(&config);

	config.info.screen_win.x      = view->x;
	config.info.screen_win.y      = view->y;
	config.info.screen_win.width  = view->w;
	config.info.screen_win.height = view->h;
	
	return layer_set_para(&config);
}

int HwDisplay::hwd_layer_top(unsigned int hlay)
{
	_p1();
	disp_layer_config config;
	//LOG1("[debug_jaosn]:the ZORDER_MAX \n",ZORDER_MAX);
	memset(&config, 0, sizeof(disp_layer_config));
	config.channel	= HD2CHN(hlay);
	config.layer_id = HD2LYL(hlay);
	layer_get_para(&config);

	config.info.zorder = ZORDER_MAX;
	return layer_set_para(&config);
}

int HwDisplay::hwd_layer_bottom(unsigned int hlay)
{
	_p1();
	//LOG1("[debug_jaosn]:the ZORDER_MIN \n");
	disp_layer_config config;
	memset(&config, 0, sizeof(disp_layer_config));
	config.channel	= HD2CHN(hlay);
	config.layer_id = HD2LYL(hlay);
	layer_get_para(&config);

	config.info.zorder = ZORDER_MIN;
	return layer_set_para(&config);
}

int HwDisplay::hwd_layer_open(unsigned int hlay)
{
	disp_layer_config config;
	memset(&config, 0, sizeof(disp_layer_config));
	config.channel	= HD2CHN(hlay);
	config.layer_id = HD2LYL(hlay);
	layer_get_para(&config);

	config.enable = 1;
	return layer_set_para(&config);
}

int HwDisplay::hwd_layer_close(unsigned int hlay)
{
	disp_layer_config config;
	memset(&config, 0, sizeof(disp_layer_config));
	config.channel	= HD2CHN(hlay);
	config.layer_id = HD2LYL(hlay);
	layer_get_para(&config);

	config.enable = 0;
	return layer_set_para(&config);
}

int HwDisplay::hwd_layer_ck_on(unsigned int hlay)
{
	return 0;
}

int HwDisplay::hwd_layer_ck_off(unsigned int hlay)
{
	return 0;
}

int HwDisplay::hwd_layer_ck_value(unsigned int hlay, unsigned int color)
{
#if 0
	LOG1("(%s %d) %d\n", __FUNCTION__, __LINE__, hlay);
	unsigned int args[4]={0};
	int ret = RET_OK;
	disp_colorkey ck;
	unsigned int r;
	unsigned int g;
	unsigned int b;
	r = (color>>16)&0xff;
	g = (color>>8)&0xff;
	b = (color>>0)&0xff;
	ck.ck_min.alpha 	= 0xff;
	ck.ck_min.red		= r;
	ck.ck_min.green 	= g;
	ck.ck_min.blue		= b;
	ck.ck_max.alpha 	= 0xff;
	ck.ck_max.red		= r;
	ck.ck_max.green 	= g;
	ck.ck_max.blue		= b;
	ck.red_match_rule	= 2;
	ck.green_match_rule = 2;
	ck.blue_match_rule	= 2;

	args[0]	= mScreen;
    args[1]	= (unsigned int)&ck;
    ioctl(mDisp_fd,DISP_SET_COLORKEY,(void*)args);
	if(RET_OK != ret) {
		ALOGE("fail to set colorkey\n");
		ret = RET_FAIL;
	}
#endif
	return 0;
}

int HwDisplay::layer_set_normal(unsigned int hlay)
{
	return 0;
}

void HwDisplay::hwd_set_rot(int screen, int rot)
{
       unsigned long args[4]={0};
       args[0] = screen;
       args[1] = rot;
       ioctl(mDisp_fd, DISP_ROTATION_SW_SET_ROT, args);
}

void HwDisplay::openHdmi(int screen, int val)
{
	int type;
	if (val) {
        type = DISP_OUTPUT_TYPE_HDMI;
	LOG1("+++++=(%s %d %d)\n", __FUNCTION__, __LINE__,screen);
        hwd_set_rot(screen, ROTATION_SW_0);
		LOG1("+++++=(%s %d)\n", __FUNCTION__, __LINE__,screen);
	} else {
		LOG1("+++++=(%s %d)\n", __FUNCTION__, __LINE__);
        type = DISP_OUTPUT_TYPE_LCD;
        hwd_set_rot(screen, ROTATION_SW_90);
	}
	unsigned long args[4]={0};
	args[0] = screen;
	args[1] = type;
	ioctl(mDisp_fd, DISP_DEVICE_SWITCH, args);
	return ;
}

int HwDisplay::hwd_layer_other_screen(int arg, unsigned int hlay1, unsigned int hlay2)
{
	disp_layer_config ui_config, csi_config;
	csi_config.channel = 0;
	csi_config.layer_id = 0;
	layer_get_para(&csi_config);
	if (arg == SCREEN_1) {	//change the ui from (2,0) to (1, 0), so that ui can be scaled
		ui_config.channel = 2;
		ui_config.layer_id = 0;
		layer_get_para(&ui_config);
		openHdmi(SCREEN_0, 1);
		ui_config.enable = 0;
		layer_set_para(&ui_config);
		ui_config.info.screen_win.width  = 1280;
		ui_config.info.screen_win.height = 720;
		ui_config.channel = 1;
		ui_config.enable = 1;
		ui_config.info.zorder = 1;
		layer_set_para(&ui_config);
		csi_config.info.zorder = 2;	//set the csi layer to the top to avoid be covered by ui
		layer_set_para(&csi_config);
	} else {
		ui_config.channel = 1;
		ui_config.layer_id = 0;
		layer_get_para(&ui_config);
		ui_config.enable = 0;
		layer_set_para(&ui_config);
		ui_config.info.screen_win.width  =  320;
		ui_config.info.screen_win.height =  240;
		ui_config.channel = 2;
		ui_config.enable = 1;
		ui_config.info.zorder = 8;
		layer_set_para(&ui_config);	//restore ui to (1,0)
		csi_config.info.zorder = 0;	//set the csi to the bottom, because ui can do alpha with it
		layer_set_para(&csi_config);
		openHdmi(SCREEN_0, 0);
	}
	return 0;
}

int HwDisplay::hwd_layer_clear(unsigned int hlay)
{
	int channel	= HD2CHN(hlay);
	int layer_id = HD2LYL(hlay);
	hwd_layer_close(hlay);
	mLayerStatus[channel][layer_id] &= ~HWD_STATUS_OPENED;
	return 0;
}

int HwDisplay::hwd_layer_render(unsigned int hlay, libhwclayerpara_t *picture)
{
	LOG2("(%s %d)\n", __FUNCTION__, __LINE__);
	int ret;
	disp_layer_config config;
	
	memset(&config, 0, sizeof(disp_layer_config));
	config.channel	= HD2CHN(hlay);
	config.layer_id = HD2LYL(hlay);
	layer_get_para(&config);
	//LOG1("++++++++++[debug_jaosn]:this zorder is %d+++++++++\n",config.info.zorder);
	config.info.fb.addr[0] = picture->top_y;
	config.info.fb.addr[1] = picture->top_c;
	config.info.fb.addr[2] = picture->bottom_y;
	ret = layer_set_para(&config);
	if(!(mLayerStatus[config.channel][config.layer_id] & HWD_STATUS_OPENED)) {
		LOG2("(%s %d) %d\n", __FUNCTION__, __LINE__, hlay);
		hwd_layer_open(hlay); //to avoid that the first frame is not initialized 
		mLayerStatus[config.channel][config.layer_id] |= HWD_STATUS_OPENED;
	}
	return ret;
}

int HwDisplay::hwd_layer_exchange(unsigned int hlay1, unsigned int hlay2, int otherOnTop)
{
	disp_layer_config config1;
	disp_layer_config config2;
	disp_rect rect_tmp;
	unsigned char zorder_tmp;
    
	memset(&config1, 0, sizeof(disp_layer_config));
	memset(&config2, 0, sizeof(disp_layer_config));
	config1.channel	= HD2CHN(hlay1);
	config1.layer_id = HD2LYL(hlay1);
	config2.channel	= HD2CHN(hlay2);
	config2.layer_id = HD2LYL(hlay2);
	//LOG1("++++++[debug_jason]:%d,%d,%d,%d+++++\n",config1.channel,config1.layer_id,config2.channel,config2.layer_id);
	layer_get_para(&config1);
	layer_get_para(&config2);

	rect_tmp = config1.info.screen_win;
	config1.info.screen_win = config2.info.screen_win;
	config2.info.screen_win = rect_tmp;

    zorder_tmp = config1.info.zorder;
    config1.info.zorder = config2.info.zorder;
    config2.info.zorder = zorder_tmp;
    
	layer_set_para(&config1);
	layer_set_para(&config2);
	return RET_OK;
}

int HwDisplay::hwd_layer_switch(unsigned int hlay, int bOpen)
{
	if (1 == bOpen) {
		return hwd_layer_open(hlay);
	} else if (0 == bOpen) {
		return hwd_layer_close(hlay);
	}
	return RET_FAIL;
}

#else
int HwDisplay::layer_cmd(unsigned int hlay, __disp_cmd_t cmd)
{
	LOG1("(%s %d) %d\n", __FUNCTION__, __LINE__, hlay);
	unsigned long args[4] = {0};

	args[0] = mScreen;
	args[1] = hlay;
	
	return ioctl(mDisp_fd, cmd, args);
}

int HwDisplay::layer_cmd(int screen, unsigned int hlay, __disp_cmd_t cmd)
{
	LOG1("(%s %d) %d\n", __FUNCTION__, __LINE__, hlay);
	unsigned long args[4] = {0};

	args[0] = screen;
	args[1] = hlay;
	
	return ioctl(mDisp_fd, cmd, args);
}

int HwDisplay::layer_request()
{
	return layer_cmd(DISP_LAYER_WORK_MODE_SCALER, DISP_CMD_LAYER_REQUEST);
}

int HwDisplay::layer_request(int screen)
{
	return layer_cmd(screen, DISP_LAYER_WORK_MODE_SCALER, DISP_CMD_LAYER_REQUEST);
}

int HwDisplay::layer_release(int hlay)
{
	return layer_cmd(hlay, DISP_CMD_LAYER_RELEASE);
}

int HwDisplay::layer_get_para(unsigned int hlay, __disp_layer_info_t *pinfo)
{
	LOG1("(%s %d) %d\n", __FUNCTION__, __LINE__, hlay);
	unsigned long args[4] = {0};
	unsigned int ret = RET_OK;
	
	args[0] = mScreen;
    args[1] = hlay;
    args[2] = (unsigned int)pinfo;
    ret = ioctl(mDisp_fd, DISP_CMD_LAYER_GET_PARA, args);
	if(RET_OK != ret) {
		ALOGE("fail to get para\n");
		ret = RET_FAIL;
	}
	
	return ret;
}

int HwDisplay::layer_set_para(unsigned int hlay, __disp_layer_info_t *pinfo)
{
	LOG1("(%s %d) %d\n", __FUNCTION__, __LINE__, hlay);
	unsigned int args[4] = {0};
	int ret = RET_OK;
	
	args[0] = mScreen;
    args[1] = hlay;
    args[2] = (unsigned int)pinfo;
	ret = ioctl(mDisp_fd, DISP_CMD_LAYER_SET_PARA, args);
	if(RET_OK != ret) {
		ALOGE("fail to set para\n");
		ret = RET_FAIL;
	}
	return ret;
}

int HwDisplay::hwd_layer_set_src(unsigned int hlay, struct src_info *src)
{
	LOG1("(%s %d) %d\n", __FUNCTION__, __LINE__, hlay);
	unsigned long args[4] = {0};
	__disp_layer_info_t layer_info;
	
	memset(&layer_info, 0, sizeof(__disp_layer_info_t));
	layer_get_para(hlay, &layer_info);

	layer_info.src_win.x      = 0;
	layer_info.src_win.y      = 0;
	layer_info.src_win.width  = src->w;
	layer_info.src_win.height = src->h;
	layer_info.fb.size.width  = src->w;
	layer_info.fb.size.height = src->h;
	switch(src->format) {
		case HWC_FORMAT_YUV411PLANAR:
			layer_info.fb.format = DISP_FORMAT_YUV411;
			layer_info.fb.mode   = DISP_MOD_NON_MB_PLANAR;
			layer_info.fb.seq	   = DISP_SEQ_P3210;
			break;
		case HWC_FORMAT_YUV422PLANAR:
			layer_info.fb.format = DISP_FORMAT_YUV422;
			layer_info.fb.mode   = DISP_MOD_NON_MB_PLANAR;
			layer_info.fb.seq    = DISP_SEQ_P3210;
			break;
		case HWC_FORMAT_YUV444PLANAR:
			layer_info.fb.format = DISP_FORMAT_YUV444;
			layer_info.fb.mode   = DISP_MOD_NON_MB_PLANAR;
			layer_info.fb.seq	   = DISP_SEQ_P3210;
			break;
		case HWC_FORMAT_YUV420PLANAR:
			layer_info.fb.format = DISP_FORMAT_YUV420;
			layer_info.fb.mode   = DISP_MOD_NON_MB_PLANAR;
			layer_info.fb.seq	   = DISP_SEQ_P3210;
			break;
		case HWC_FORMAT_YUV420UVC:
			layer_info.fb.format = DISP_FORMAT_YUV420;
			layer_info.fb.mode   = DISP_MOD_NON_MB_UV_COMBINED;
			layer_info.fb.seq	   = DISP_SEQ_UVUV;
			break;
		case HWC_FORMAT_YUV420VUC:
			layer_info.fb.format = DISP_FORMAT_YUV420;
			layer_info.fb.mode   = DISP_MOD_NON_MB_UV_COMBINED;
			layer_info.fb.seq	   = DISP_SEQ_VUVU;
			break;
		case HWC_FORMAT_MBYUV420:
			layer_info.fb.format = DISP_FORMAT_YUV420;
			layer_info.fb.mode   = DISP_MOD_MB_UV_COMBINED;
			layer_info.fb.seq    = DISP_SEQ_UVUV;
			break;
		case HWC_FORMAT_MBYUV422:
			layer_info.fb.format = DISP_FORMAT_YUV422;
			layer_info.fb.mode   = DISP_MOD_MB_UV_COMBINED;
			layer_info.fb.seq    = DISP_SEQ_UVUV;
			break;
		default:
			layer_info.fb.format = DISP_FORMAT_ARGB8888;
			layer_info.fb.mode   = DISP_MOD_INTERLEAVED;
			layer_info.fb.seq    = DISP_SEQ_ARGB;
			break;
	}
	return layer_set_para(hlay, &layer_info);
}


int HwDisplay::hwd_layer_set_rect(unsigned int hlay, struct view_info *view)
{
	LOG1("(%s %d) %d\n", __FUNCTION__, __LINE__, hlay);
	unsigned long args[4] = {0};
	__disp_layer_info_t layer_info;
	
	memset(&layer_info, 0, sizeof(__disp_layer_info_t));
	layer_get_para(hlay, &layer_info);

	layer_info.scn_win.x      = view->x;
	layer_info.scn_win.y      = view->y;
	layer_info.scn_win.width  = view->w;
	layer_info.scn_win.height = view->h;
	
	return layer_set_para(hlay, &layer_info);
}

int HwDisplay::hwd_layer_top(unsigned int hlay)
{
	LOG1("(%s %d) %d\n", __FUNCTION__, __LINE__, hlay);
	return layer_cmd(hlay, DISP_CMD_LAYER_TOP);
}

int HwDisplay::hwd_layer_bottom(unsigned int hlay)
{
	LOG1("(%s %d) %d\n", __FUNCTION__, __LINE__, hlay);
	return layer_cmd(hlay, DISP_CMD_LAYER_BOTTOM);
}

int HwDisplay::hwd_layer_open(unsigned int hlay)
{
	int idx = HDL2ID(hlay);
	int ret = RET_OK;
	LOG1("(%s %d) %d\n", __FUNCTION__, __LINE__, hlay);
	if (layer_cmd(hlay, DISP_CMD_LAYER_OPEN) < 0) {
		ret = RET_FAIL;
	}
	return ret;
}

int HwDisplay::hwd_layer_close(unsigned int hlay)
{
	int idx = HDL2ID(hlay);
	int ret = RET_OK;
	LOG1("(%s %d) %d\n", __FUNCTION__, __LINE__, hlay);
	if (layer_cmd(hlay, DISP_CMD_LAYER_CLOSE) < 0) {
		ret = RET_FAIL;
	}
	return ret;
}

int HwDisplay::hwd_layer_ck_on(unsigned int hlay)
{
	LOG1("(%s %d) %d\n", __FUNCTION__, __LINE__, hlay);
	return layer_cmd(hlay, DISP_CMD_LAYER_CK_ON);
}

int HwDisplay::hwd_layer_ck_off(unsigned int hlay)
{
	LOG1("(%s %d) %d\n", __FUNCTION__, __LINE__, hlay);
	return layer_cmd(hlay, DISP_CMD_LAYER_CK_OFF);
}

int HwDisplay::hwd_layer_ck_value(unsigned int hlay, unsigned int color)
{
	LOG1("(%s %d) %d\n", __FUNCTION__, __LINE__, hlay);
	unsigned int args[4]={0};
	int ret = RET_OK;
	__disp_colorkey_t ck;
	unsigned int r;
	unsigned int g;
	unsigned int b;
	r = (color>>16)&0xff;
	g = (color>>8)&0xff;
	b = (color>>0)&0xff;
	ck.ck_min.alpha 	= 0xff;
	ck.ck_min.red		= r;
	ck.ck_min.green 	= g;
	ck.ck_min.blue		= b;
	ck.ck_max.alpha 	= 0xff;
	ck.ck_max.red		= r;
	ck.ck_max.green 	= g;
	ck.ck_max.blue		= b;
	ck.red_match_rule	= 2;
	ck.green_match_rule = 2;
	ck.blue_match_rule	= 2;
	
	args[0]	= mScreen;
    args[1]	= (unsigned int)&ck;
    ioctl(mDisp_fd,DISP_CMD_SET_COLORKEY,(void*)args);
	if(RET_OK != ret) {
		ALOGE("fail to set colorkey\n");
		ret = RET_FAIL;
	}
	return ret;
}

int HwDisplay::layer_set_normal(unsigned int hlay)
{
	__disp_layer_info_t layer_info;
	layer_get_para(hlay, &layer_info);
	layer_info.src_win.x      = 0;
	layer_info.src_win.y      = 0;
	layer_info.src_win.width  = 1;
	layer_info.src_win.height = 1;
	layer_info.fb.size.width  = 1;
	layer_info.fb.size.height = 1;
	layer_info.scn_win.x      = 0;
	layer_info.scn_win.y      = 0;
	layer_info.scn_win.width  = 1;
	layer_info.scn_win.height = 1;
	layer_info.mode           = DISP_LAYER_WORK_MODE_NORMAL;
	layer_set_para(hlay, &layer_info);
	return 0;
}
void HwDisplay::openHdmi(int screen, int val)
{
	int disp_fd = open("/dev/disp", O_RDWR);
	unsigned long arg[4]={0};
	int cmd = val?DISP_CMD_HDMI_ON:DISP_CMD_HDMI_OFF;
	arg[0] = screen;
	arg[1] = DISP_TV_MOD_720P_60HZ;
	ioctl(disp_fd,DISP_CMD_HDMI_SET_MODE,arg);
	ioctl(disp_fd,cmd,arg);
	close(disp_fd);
}

int HwDisplay::hwd_layer_other_screen(int screen, unsigned int hlay1, unsigned int hlay2)
{
	__disp_layer_info_t layer_info1, layer_info2;

	int cnt = 0;
	int lay = 0;
	if (hlay1 != 0) {
		layer_get_para(hlay1, &layer_info1);
		hwd_layer_close(hlay1);
		hwd_layer_release(hlay1);
		cnt++;
	}
	if (hlay2 != 0) {
		layer_get_para(hlay2, &layer_info2);
		hwd_layer_close(hlay2);
		hwd_layer_release(hlay2);
		cnt++;
	}
		lay = layer_request(screen);
		mLayerStatus[HDL2ID(lay)] |= HWD_STATUS_REQUESTED;
	ALOGD("\nline %d---------------lay:%d\n", __LINE__, lay);
		lay = layer_request(screen);
		mLayerStatus[HDL2ID(lay)] |= HWD_STATUS_REQUESTED;
	ALOGD("\nline %d---------------lay:%d\n", __LINE__, lay);
	if (hlay1 == 0) {
		mLayerStatus[HDL2ID(lay)] |= HWD_STATUS_NOTUSED;
	}
	if (hlay2 == 0) {
		mLayerStatus[HDL2ID(lay)] |= HWD_STATUS_NOTUSED;
	}
	mScreen = screen;
	if (hlay1 != 0) {
		layer_set_para(hlay1, &layer_info1);
	}
	if (hlay2 != 0) {
		layer_set_para(hlay2, &layer_info2);
	}
	if (screen == SCREEN_1) {
		openHdmi(SCREEN_1, 1);
	} else {
		openHdmi(SCREEN_1, 0);
	}
	return 0;
}

int HwDisplay::hwd_layer_clear(unsigned int hlay)
{
	int idx = HDL2ID(hlay);
	hwd_layer_close(hlay);
	mLayerStatus[idx] &= ~HWD_STATUS_OPENED;
	return 0;
}

int HwDisplay::hwd_layer_render(unsigned int hlay, libhwclayerpara_t *picture)
{
	unsigned long args[4] = {0};
	__disp_layer_info_t layer_info;
	int idx = HDL2ID(hlay);
	int ret = RET_OK;
	
	memset(&layer_info, 0, sizeof(__disp_layer_info_t));
	layer_get_para(hlay, &layer_info);
	
	layer_info.fb.addr[0] = picture->top_y;
	layer_info.fb.addr[1] = picture->top_c;
	layer_info.fb.addr[2] = picture->bottom_y;
	ret = layer_set_para(hlay, &layer_info);
	
	if(!(mLayerStatus[idx] & HWD_STATUS_OPENED)) {
		LOG1("(%s %d) %d\n", __FUNCTION__, __LINE__, hlay);
		hwd_layer_open(hlay); //to avoid that the first frame is not initialized 
		mLayerStatus[idx] |= HWD_STATUS_OPENED;
	}

	return ret;
}

int HwDisplay::hwd_layer_exchange(unsigned int hlay1, unsigned int hlay2, int otherOnTop)
{
	if (hlay1 < VALID_LAYER || hlay2 < VALID_LAYER) {
		ALOGE("hlay invalid\n");
		return RET_FAIL;
	}
	
	if (otherOnTop != 1 && otherOnTop != -1) {
		ALOGE("hwd_layer_exchange, the third args should be 1 or -1");
		return RET_FAIL;
	}
	__disp_layer_info_t layer_info1;
	__disp_layer_info_t layer_info2;
	__disp_rect_t rect_tmp;
	memset(&layer_info1, 0, sizeof(__disp_layer_info_t));
	memset(&layer_info2, 0, sizeof(__disp_layer_info_t));

	layer_get_para(hlay1, &layer_info1);
	layer_get_para(hlay2, &layer_info2);
	LOG1("prio1:%d, prio2:%d\n", layer_info1.prio, layer_info2.prio);
	rect_tmp = layer_info1.scn_win;
	layer_info1.scn_win = layer_info2.scn_win;
	layer_info2.scn_win = rect_tmp;
	
	layer_set_para(hlay1, &layer_info1);
	layer_set_para(hlay2, &layer_info2);
	if (1 == otherOnTop) { //has other layer on the top
		if(layer_info1.prio < layer_info2.prio) {
			hwd_layer_bottom(hlay1);
			hwd_layer_bottom(hlay2);
		} else {
			hwd_layer_bottom(hlay2);
			hwd_layer_bottom(hlay1);
		}
	} else if (-1 == otherOnTop){ //has other layer on the bottom
		if(layer_info1.prio < layer_info2.prio) {
			hwd_layer_top(hlay2);
			hwd_layer_top(hlay1);
		} else {
			hwd_layer_top(hlay1);
			hwd_layer_top(hlay2);
		}
	}
	return RET_OK;
}

int HwDisplay::hwd_layer_switch(unsigned int hlay, int bOpen)
{
	if (1 == bOpen) {
		return layer_cmd(hlay, DISP_CMD_LAYER_OPEN);
	} else if (0 == bOpen) {
		return layer_cmd(hlay, DISP_CMD_LAYER_CLOSE);
	}
	return RET_FAIL;
}

#endif

int HwDisplay::hwd_layer_request(struct view_info* surface)
{
#ifdef DE_2
	int hlay;
	int ch, id;
	disp_layer_config config;
	memset(&config, 0, sizeof(disp_layer_config));
	hlay = layer_request(&ch, &id);
	config.channel = ch;
	config.layer_id = id;
	config.enable = 0;
	config.info.screen_win.x = surface->x;
	config.info.screen_win.y = surface->y;
	config.info.screen_win.width	= surface->w;
	config.info.screen_win.height	= surface->h;
	config.info.mode = LAYER_MODE_BUFFER;
	config.info.alpha_mode = 0;
	config.info.alpha_value = 128;
	config.info.fb.flags = DISP_BF_NORMAL;
	config.info.fb.scan = DISP_SCAN_PROGRESSIVE;
	config.info.fb.color_space = (surface->h<720)?DISP_BT601:DISP_BT709;
	config.info.zorder = HLAY(ch, id);
    ALOGI("hlay:%d, zorder=%d, cnt:%d", hlay, config.info.zorder, mCurLayerCnt);
	layer_set_para(&config);
	return hlay;
#else
	LOG1("(%s %d)\n", __FUNCTION__, __LINE__);
	int idx = -1;
	int i = 0;
	int hlay;
	unsigned int ret;
	unsigned long args[4] = {0};
	__disp_layer_info_t layer_info;

	struct view_info *para = surface;
	{
		Mutex::Autolock lock(sLock);
		for (i=0; i<MAX_LAYER; i++) {
			if (!(mLayerStatus[i]&HWD_STATUS_REQUESTED)) {
				break;
			} else if(mLayerStatus[i]&HWD_STATUS_NOTUSED){
				hlay = ID2HDL(i);
				goto set_para;
			}
		}
		if (MAX_LAYER == i) {
			ALOGE("all layers used.");
			return RET_FAIL;
		}
		hlay = layer_request();
		LOG1("~~~~~~~~~~~~~hlay_hdl:%d", hlay);
		if(hlay < 0) {
			ALOGE("fail to request layer");
			ret = RET_FAIL;
			return ret;
		} else {
			mLayerStatus[HDL2ID(hlay)] |= HWD_STATUS_REQUESTED;
		}
	}
set_para:
	memset(&layer_info, 0, sizeof(__disp_layer_info_t));

	layer_info.scn_win.x      = para->x;
	layer_info.scn_win.y      = para->y;
	layer_info.scn_win.width  = para->w;
	layer_info.scn_win.height = para->h;

	layer_info.mode           = DISP_LAYER_WORK_MODE_SCALER;
	layer_info.fb.cs_mode     = (para->h<720)?DISP_BT601:DISP_BT709;
	layer_info.pipe           = 1;	//fb0's layer in pipe 0, set video layer in pipe 1 to use colorkey
	layer_info.fb.br_swap     = 0;
	layer_info.fb.b_trd_src   = 0;
	layer_info.ck_enable      = 1;
	hwd_layer_ck_value(hlay, CK_COLOR);
	
	layer_set_para(hlay, &layer_info);
	
	return hlay;
#endif
}

int HwDisplay::hwd_layer_release(unsigned int hlay)
{
#ifdef DE_2
	int chn = HD2CHN(hlay);
	int lyl = HD2LYL(hlay);
	int ret = layer_release(hlay);
	if (RET_OK == ret) {
		Mutex::Autolock lock(sLock);
		if (chn >=0 && lyl >=0 && mLayerStatus[chn][lyl]) {
			mLayerStatus[chn][lyl] = 0;
			mCurLayerCnt--;
		}
	}
	return ret;
#else
	LOG1("(%s %d) %d\n", __FUNCTION__, __LINE__, hlay);
	int idx = HDL2ID(hlay);
	int ret = layer_release(hlay);
	if (RET_OK == ret)
	{
		Mutex::Autolock lock(sLock);
		if (idx >= 0)
			mLayerStatus[idx] = 0;
	}
	return ret;
#endif
}

int HwDisplay::hwd_init(void)
{
	LOG1("(%s %d)\n", __FUNCTION__, __LINE__);
	int ret = RET_OK;
	mScreen = SCREEN_0;
	if (mInitialized) {
		return RET_FAIL;
	}
	
    mDisp_fd = open(DISP_DEV, O_RDWR);
	if (mDisp_fd < 0)
    {
        ALOGE("Failed to open disp device, ret:%d, errno: %d\n", mDisp_fd, errno);
        return  RET_FAIL;
    }

#ifdef DE_2
	mLayerStatus[2][0] = HWD_STATUS_REQUESTED;	//used by fb0
#else
	layer_request(SCREEN_1);
#endif
#ifdef DE_2
//display resume
	int args[4]={0};
	args[0] = 0;
	args[1] = 0;
	ioctl(mDisp_fd, DISP_BLANK, args);

#endif
	mInitialized = true;
	mCurLayerCnt = 1;
    return ret;
}

int HwDisplay::hwd_exit(void)
{
	LOG1("(%s %d)\n", __FUNCTION__, __LINE__);
	int ret = RET_OK;
			
    close(mDisp_fd);
	
    return ret;
}

HwDisplay* HwDisplay::getInstance()
{
	LOG1("(%s %d)\n", __FUNCTION__, __LINE__);
    Mutex::Autolock lock(sLock);
    if (sHwDisplay == NULL) {
		sHwDisplay = new HwDisplay;
	}
	return sHwDisplay;
}

}
