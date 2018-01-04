#define LOG_TAG "Water_mark_interface"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <cutils/log.h>
#include <include_watermark/water_mark_interface.h>

#include <linux/preempt.h>

#include "water_mark.h"


static void argb2yuv420sp(unsigned char* src_p, unsigned char* alph, 
				int width, int height,
				unsigned char* dest_y, unsigned char* dest_c)
{
	int i,j;
	for(i = 0; i < (int)height; i++)
    {
		if((i&1) == 0)
		{
	    	for(j= 0; j< (int)width; j++)
			{
				*dest_y = (299*src_p[2]+587*src_p[1]+114*src_p[0])/1000;

				if((j&1) == 0)
				{
				   //cb
				   *dest_c++ = 128+(564*(src_p[0]-*dest_y)/1000);	
				}
				else
				{
				   // cr
				   *dest_c++ = 128+(713*(src_p[2]-*dest_y)/1000);
				}

				*alph++ = src_p[3];
				src_p +=4;
				dest_y++;
			}		
		}
		else
		{
			for(j= 0; j< (int)width; j++)
			{
				*dest_y = (299*src_p[2]+587*src_p[1]+114*src_p[0])/1000;
				*alph++ = src_p[3];
				src_p +=4;
				dest_y++;
			}	
		}
    }
	
	return;
}

static int getWordCharNum(char val)
{
	int i;
	int cnt = 0;

	for (i=7; i>=0; i--) {
		if (!(val & (1<<i))) {
			break;
		} else {
			cnt++;
		}
	}

	if (cnt == 0) {
		return 1;
	}
	return cnt;
}

static int getWordIndex(char *data, int len)
{
	if (len == 1) {
		if (*data >= '0' && *data <= '9') return (*data - 0x30);
        else if (*data >= 'A' && *data <= 'Z') return (*data - 0x2B);
		else if (*data == ' ') return 19;
		else if (*data == '-') return 20;
		else if (*data == ':') return 21;
        else if (*data == '/') return 48;
		else return 19;
	} else if (len == 2) {
		return 19;
	} else if (len == 3) {
		if(!memcmp(data, "京", 3)) return 49;
		else if (!memcmp(data, "湘", 3)) return 50;
		else if (!memcmp(data, "津", 3)) return 51;
		else if (!memcmp(data, "鄂", 3)) return 52;
		else if (!memcmp(data, "沪", 3)) return 53;
		else if (!memcmp(data, "粤", 3)) return 54;
		else if (!memcmp(data, "渝", 3)) return 55;
		else if (!memcmp(data, "琼", 3)) return 56;
		else if (!memcmp(data, "冀", 3)) return 57;
		else if (!memcmp(data, "川", 3)) return 58;
		else if (!memcmp(data, "晋", 3)) return 59;
		else if (!memcmp(data, "黔", 3)) return 60;
		else if (!memcmp(data, "辽", 3)) return 61;
		else if (!memcmp(data, "云", 3)) return 62;
		else if (!memcmp(data, "吉", 3)) return 63;
		else if (!memcmp(data, "陕", 3)) return 64;
		else if (!memcmp(data, "黑", 3)) return 65;
		else if (!memcmp(data, "甘", 3)) return 66;
		else if (!memcmp(data, "苏", 3)) return 67;
		else if (!memcmp(data, "青", 3)) return 68;
		else if (!memcmp(data, "浙", 3)) return 69;
		else if (!memcmp(data, "台", 3)) return 70;
		else if (!memcmp(data, "皖", 3)) return 71;
		else if (!memcmp(data, "藏", 3)) return 72;
		else if (!memcmp(data, "闽", 3)) return 73;
		else if (!memcmp(data, "蒙", 3)) return 74;
		else if (!memcmp(data, "赣", 3)) return 75;
		else if (!memcmp(data, "桂", 3)) return 76;
		else if (!memcmp(data, "鲁", 3)) return 77;
		else if (!memcmp(data, "宁", 3)) return 78;
		else if (!memcmp(data, "豫", 3)) return 79;
		else if (!memcmp(data, "新", 3)) return 80;
		else if (!memcmp(data, "港", 3)) return 81;
		else if (!memcmp(data, "澳", 3)) return 82;
		else return 19;
	} else {
		return 19;
	}
	return 19;
}

void *initialwaterMark(int wm_height)
{
	FILE *fp = NULL;
	int watermark_pic_num = MAX_ICON_PIC;
	int i;
	int width = 0, height = 0;
	char filename[256];
	unsigned char *buf = NULL;
	WATERMARK_CTRL *wm_ctrl = NULL;

	ALOGD("<initialwaterMark>water mark initialize, height=%d", wm_height);
	wm_ctrl = (WATERMARK_CTRL*)malloc(sizeof(WATERMARK_CTRL));
	if (wm_ctrl == NULL) {
		ALOGE("<initialwaterMark> Alloc wm_ctrl error(%s)", strerror(errno));
		return NULL;
	}
	memset(wm_ctrl, 0, sizeof(WATERMARK_CTRL));
	wm_ctrl->ADBright.recycle_frame = 30;
    wm_ctrl->multi.singleWaterMark[0].positionX = 32;
    wm_ctrl->multi.singleWaterMark[0].positionY = 32;
    wm_ctrl->multi.waterMarkNum = 1;

	for(i = 0; i < watermark_pic_num; ++i) {
		sprintf(filename, "/system/watermark/wm_%dp_%d.bmp", wm_height, i);

		fp = fopen(filename, "r");
		if (NULL == fp) {
			ALOGW("<initialwaterMark>Fail to open file %s(%s)!", filename, strerror(errno));
			break;
		}

		//get watermark picture size
		fseek(fp, 18, SEEK_SET);
		fread(&width, 1, 4, fp);
		fread(&height, 1, 4, fp);
		wm_ctrl->wminfo.single_pic[i].width = abs(width);
		wm_ctrl->wminfo.single_pic[i].height = abs(height);

		fseek(fp, 54, SEEK_SET);

		wm_ctrl->wminfo.single_pic[i].y = (unsigned char*)malloc(wm_ctrl->wminfo.single_pic[i].width * wm_ctrl->wminfo.single_pic[i].height*5/2);
		if (NULL == wm_ctrl->wminfo.single_pic[i].y) {
			fclose(fp);
			goto ALLOC_Y_ERR;
		}
		wm_ctrl->wminfo.single_pic[i].alph = wm_ctrl->wminfo.single_pic[i].y + wm_ctrl->wminfo.single_pic[i].width * wm_ctrl->wminfo.single_pic[i].height;
		wm_ctrl->wminfo.single_pic[i].c = wm_ctrl->wminfo.single_pic[i].alph + wm_ctrl->wminfo.single_pic[i].width * wm_ctrl->wminfo.single_pic[i].height;
		wm_ctrl->wminfo.single_pic[i].id = i;

		buf = (unsigned char *)malloc(wm_ctrl->wminfo.single_pic[i].width * wm_ctrl->wminfo.single_pic[i].height * 4);
		if (NULL == buf) {
			fclose(fp);
			goto ALLOC_BUF_ERR;
		}

		fread(buf, wm_ctrl->wminfo.single_pic[i].width * wm_ctrl->wminfo.single_pic[i].height * 4, 1, fp);

		argb2yuv420sp(buf, wm_ctrl->wminfo.single_pic[i].alph, 
				wm_ctrl->wminfo.single_pic[i].width, wm_ctrl->wminfo.single_pic[i].height,
				wm_ctrl->wminfo.single_pic[i].y, wm_ctrl->wminfo.single_pic[i].c);

		fclose(fp);
		free(buf);
	}
    if (i == 0) {
        free(wm_ctrl);
        return NULL;
    } else {
    	wm_ctrl->wminfo.picture_number = i;
    	ALOGD("<initialwaterMark>picture_number=%d", i);
    }

	return (void*)wm_ctrl;

ALLOC_BUF_ERR:
ALLOC_Y_ERR:
OPEN_FILE_ERR:
	for (i = 0; i < wm_ctrl->wminfo.picture_number; ++i) {
		if (wm_ctrl->wminfo.single_pic[i].y != NULL) {
			free(wm_ctrl->wminfo.single_pic[i].y);
			wm_ctrl->wminfo.single_pic[i].y = NULL;
		}
	}
	free(wm_ctrl);
	return NULL;
}

int releaseWaterMark(void *ctrl)
{
	int i;
	WATERMARK_CTRL *wm_ctrl = (WATERMARK_CTRL*)ctrl;

    if (ctrl == NULL) {
        ALOGE("<F:%s, L:%d>Invalid parameters!!", __FUNCTION__, __LINE__);
        return -1;
    }
	
	ALOGD("<releaseWaterMark>water mark release");
	for (i = 0; i < wm_ctrl->wminfo.picture_number; ++i) {
		if (wm_ctrl->wminfo.single_pic[i].y != NULL) {
			free(wm_ctrl->wminfo.single_pic[i].y);
			wm_ctrl->wminfo.single_pic[i].y = NULL;
		}
	}
	free(wm_ctrl);
	return 0;
}

int waterMarkSetMultiple(void *ctrl, const char *waterMark)
{
    int i;
    char buf[256];
    char *pStr;
    WATERMARK_CTRL *wm_ctrl;

    ALOGV("waterMarkSetMultiple(%s)", waterMark);
    if (ctrl == NULL || waterMark == NULL) {
        ALOGE("<F:%s, L:%d>Invalid parameters!!", __FUNCTION__, __LINE__);
        return -1;
    }

    wm_ctrl = (WATERMARK_CTRL*)ctrl;
    strncpy(buf, waterMark, 256);
    for (i = 0; i < MAX_WATERMARK_NUM; i++) {
        if (i == 0) {
            pStr = strtok(buf, ",");
        } else {
            pStr = strtok(NULL, ",");
        }
        if (pStr == NULL) break;
        wm_ctrl->multi.singleWaterMark[i].positionX = strtol(pStr, NULL, 0);

        pStr = strtok(NULL, ",");
        if (pStr == NULL) break;
        wm_ctrl->multi.singleWaterMark[i].positionY = strtol(pStr, NULL, 0);

        pStr = strtok(NULL, ",");
        if (pStr == NULL) break;
        strncpy(wm_ctrl->multi.singleWaterMark[i].content, pStr, DISP_ICON_NUM);
        ALOGV("watermark_%d:(%d,%d)-%s", i, wm_ctrl->multi.singleWaterMark[i].positionX,
            wm_ctrl->multi.singleWaterMark[i].positionY, wm_ctrl->multi.singleWaterMark[i].content);
    }
    wm_ctrl->multi.waterMarkNum = i;
    return 0;
}

static int doWaterMark(WaterMarkInData *data, void *ctrl)
{
	BackGroudLayerInfo BGInfo;
	ShowWaterMarkParam WMPara;
	AjustBrightnessParam *ADBright;
	WATERMARK_CTRL *wm_ctrl;
	WaterMarkInfo *wm_info;
	int buflen, wordlen;
	int i;
	char *dispBuf;

	if (ctrl == NULL || data == NULL) {
		ALOGE("Input parameters error!");
		return -1;
	}
	if (data->display == NULL || (buflen = strlen(data->display)) == 0) {
		ALOGE("invalid water mark display buffer!");
		return -1;
	}

	wm_ctrl = (WATERMARK_CTRL*)ctrl;
	BGInfo.width = data->width;
	BGInfo.height = data->height;
	BGInfo.y = data->y;
	BGInfo.c = data->c;
	wm_info = &wm_ctrl->wminfo;
	ADBright = &wm_ctrl->ADBright;
	dispBuf = data->display;

	for (i = 0; i < DISP_ICON_NUM; ++i) {
		wordlen = getWordCharNum(dispBuf[0]);
		buflen -= wordlen;
		if (buflen < 0) {
			break;
		}
		WMPara.id_list[i] = getWordIndex(dispBuf, wordlen);
        if (WMPara.id_list[i] >= wm_ctrl->wminfo.picture_number) {
            ALOGE("<F:%s, L:%d> getWordIndex[%d] error! wm string '%s'", __FUNCTION__, __LINE__, WMPara.id_list[i], data->display);
            return -1;
        }
		dispBuf += wordlen;
	}

	WMPara.number = i;

	WMPara.pos.x =  data->posx;
	WMPara.pos.y = data->posy;

	//watermark_blending_ajust_brightness(&BGInfo, wm_info, &WMPara, ADBright);
	watermark_blending(&BGInfo, wm_info, &WMPara);

	return 0;
}

int doWaterMarkMultiple(WaterMarkInData *data, void *ctrl, const char *time_str)
{
    WATERMARK_CTRL *wm_ctrl = (WATERMARK_CTRL*)ctrl;
    int i;

    if (data == NULL || ctrl == NULL || time_str == NULL) {
        ALOGE("<F:%s, L:%d>Invalid parameters!!", __FUNCTION__, __LINE__);
        return -1;
    }

    //preempt_disable();
    if (data->main_channel == 1) {
        if (wm_ctrl->multi.singleWaterMark[0].content[0] == '1') {
            data->posx = wm_ctrl->multi.singleWaterMark[0].positionX;
            data->posy = wm_ctrl->multi.singleWaterMark[0].positionY;
            data->display = (char*)time_str;
            doWaterMark(data, ctrl);
        }
        for (i = 1; i < wm_ctrl->multi.waterMarkNum; i++) {
            data->posx = wm_ctrl->multi.singleWaterMark[i].positionX;
            data->posy = wm_ctrl->multi.singleWaterMark[i].positionY;
            data->display = wm_ctrl->multi.singleWaterMark[i].content;
            doWaterMark(data,ctrl);
        }
    } else {
        if (wm_ctrl->multi.singleWaterMark[0].content[0] == '1') {
            data->posx = wm_ctrl->multi.singleWaterMark[0].positionX * data->scale;
            data->posy = wm_ctrl->multi.singleWaterMark[0].positionY * data->scale;
            data->display = (char*)time_str;
            doWaterMark(data, ctrl);
        }
        for (i = 1; i < wm_ctrl->multi.waterMarkNum; i++) {
            data->posx = wm_ctrl->multi.singleWaterMark[i].positionX * data->scale;
            data->posy = wm_ctrl->multi.singleWaterMark[i].positionY * data->scale;
            data->display = wm_ctrl->multi.singleWaterMark[i].content;
            doWaterMark(data,ctrl);
        }
    }
    //preempt_enable();
    return 0;
}

