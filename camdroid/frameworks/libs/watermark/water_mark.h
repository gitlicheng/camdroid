#ifndef _WATER_MARK_H_
#define _WATER_MARK_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MAX_ICON_PIC	83
#define DISP_ICON_NUM	23
#define MAX_WATERMARK_NUM 5

typedef struct BackGroudLayerInfo
{
	int width;
	int height;
	unsigned char* y;
	unsigned char* c;
}BackGroudLayerInfo;

typedef struct SinglePicture
{
	int id; //picture id
	int width; //pic width
	int height; //pic height
	unsigned char* y;
	unsigned char* c;
	unsigned char* alph;
}SinglePicture;

typedef struct WaterMarkInfo
{
	int picture_number; 
	SinglePicture single_pic[MAX_ICON_PIC];
}WaterMarkInfo;

typedef struct WaterMarkPositon
{
	int x;
	int y;
}WaterMarkPositon;

typedef struct ShowWaterMarkParam
{
    WaterMarkPositon  pos;     //the position of the waterMark
    int               number;
    int               id_list[DISP_ICON_NUM];  //the index of the picture of the waterMark
}ShowWaterMarkParam;

typedef struct AjustBrightnessParam
{
	int     recycle_frame;  // brightness recycle frame
	int     cur_frame;  // use this to decide if ajust brightness 
	int     number;     // number as same as ShowWaterMarkParam.number
	int     Brightness[DISP_ICON_NUM];  //the index of the picture of the waterMark
}AjustBrightnessParam;

typedef struct {
	int positionX;
	int positionY;
	char content[DISP_ICON_NUM];
}SingleWaterMark;

typedef struct {
	int waterMarkNum;
	SingleWaterMark singleWaterMark[MAX_WATERMARK_NUM];
}WaterMarkMultiple;

typedef struct _WATERMARK_CTRL
{
	AjustBrightnessParam ADBright;
	WaterMarkInfo wminfo;
    WaterMarkMultiple multi;
} WATERMARK_CTRL;


int watermark_blending(BackGroudLayerInfo *bg_info, WaterMarkInfo *wm_info, ShowWaterMarkParam *wm_Param);
int watermark_blending_ajust_brightness(BackGroudLayerInfo *bg_info, WaterMarkInfo *wm_info, ShowWaterMarkParam *wm_Param, 
	AjustBrightnessParam *ajust_Param);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif



