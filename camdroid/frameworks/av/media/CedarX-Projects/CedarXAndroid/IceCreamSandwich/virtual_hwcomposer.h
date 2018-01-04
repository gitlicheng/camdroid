/*
********************************************************************************
*                                    Android
*                               CedarX sub-system
*                               CedarXAndroid module
*
*          (c) Copyright 2010-2013, Allwinner Microelectronic Co., Ltd.
*                              All Rights Reserved
*
* File   : virtual_hwcomposer.h
* Version: V1.0
* By     : Eric_wang
* Date   : 2012-2-26
* Description:
********************************************************************************
*/
#ifndef _VIRTUAL_HWCOMPOSER_H_
#define _VIRTUAL_HWCOMPOSER_H_

#include <CDX_Common.h>
#include <CDX_PlayerAPI.h>
//#include <libcedarv.h>
//#include <OMX_Video.h>
//#include <hardware/hwcomposer.h>
#include <vdecoder.h>

namespace android {

#if 1
#if (CEDARX_ANDROID_VERSION >= 9)
  #if (defined(__CHIP_VERSION_F81))
    typedef enum tag_VirtualHWCRenderFormat
    {
        VIRTUAL_HWC_FORMAT_MINVALUE     = HWC_FORMAT_MINVALUE,
        VIRTUAL_HWC_FORMAT_RGBA_8888    = HWC_FORMAT_RGBA_8888,
        VIRTUAL_HWC_FORMAT_RGB_565      = HWC_FORMAT_RGB_565,
        VIRTUAL_HWC_FORMAT_BGRA_8888    = HWC_FORMAT_BGRA_8888,
        VIRTUAL_HWC_FORMAT_YCbYCr_422_I = HWC_FORMAT_YCbYCr_422_I,
        VIRTUAL_HWC_FORMAT_CbYCrY_422_I = HWC_FORMAT_CbYCrY_422_I,
        VIRTUAL_HWC_FORMAT_MBYUV420		= HWC_FORMAT_MBYUV420,
        VIRTUAL_HWC_FORMAT_MBYUV422		= HWC_FORMAT_MBYUV422,
        VIRTUAL_HWC_FORMAT_YUV420PLANAR	= HWC_FORMAT_YUV420PLANAR,
        VIRTUAL_HWC_FORMAT_YUV420VUC    = HWC_FORMAT_YUV420VUC,
        VIRTUAL_HWC_FORMAT_DEFAULT      = HWC_FORMAT_DEFAULT,    // The actual color format is determined
        VIRTUAL_HWC_FORMAT_MAXVALUE     = HWC_FORMAT_MAXVALUE,
        VIRTUAL_HAL_PIXEL_FORMAT_YV12   = HAL_PIXEL_FORMAT_YV12,
    }VirtualHWCRenderFormat;
    typedef enum tag_VirtualHWC3DSrcMode  
    {
        VIRTUAL_HWC_3D_SRC_MODE_TB  = 0x0,  //HWC_3D_SRC_MODE_TB,//top bottom, 
        VIRTUAL_HWC_3D_SRC_MODE_FP  = 0x1,  //HWC_3D_SRC_MODE_FP,//frame packing
        VIRTUAL_HWC_3D_SRC_MODE_SSF = 0x2,  //HWC_3D_SRC_MODE_SSF,//side by side full
        VIRTUAL_HWC_3D_SRC_MODE_SSH = 0x3,  //HWC_3D_SRC_MODE_SSH,//side by side half
        VIRTUAL_HWC_3D_SRC_MODE_LI  = 0x4,  //HWC_3D_SRC_MODE_LI,//line interleaved

        VIRTUAL_HWC_3D_SRC_MODE_NORMAL = 0xFF,  //HWC_3D_SRC_MODE_NORMAL, //2d
        VIRTUAL_HWC_3D_SRC_MODE_UNKNOWN = -1,
    }VirtualHWC3DSrcMode;   //virtual to HWC_3D_SRC_MODE_TB
    typedef enum tag_VirtualHWCDisplayMode  
    {
        VIRTUAL_HWC_3D_OUT_MODE_2D                  = 0x0,  //HWC_3D_OUT_MODE_2D,//left picture
        VIRTUAL_HWC_3D_OUT_MODE_HDMI_3D_1080P24_FP  = 0x1,  //HWC_3D_OUT_MODE_HDMI_3D_1080P24_FP,
        VIRTUAL_HWC_3D_OUT_MODE_ANAGLAGH            = 0x2,  //HWC_3D_OUT_MODE_ANAGLAGH,//
        VIRTUAL_HWC_3D_OUT_MODE_ORIGINAL            = 0x3,  //HWC_3D_OUT_MODE_ORIGINAL,//original pixture

        VIRTUAL_HWC_3D_OUT_MODE_LI                  = 0x4,  //HWC_3D_OUT_MODE_LI,//line interleaved
        VIRTUAL_HWC_3D_OUT_MODE_CI_1                = 0x5,  //HWC_3D_OUT_MODE_CI_1,//column interlaved 1
        VIRTUAL_HWC_3D_OUT_MODE_CI_2                = 0x6,  //HWC_3D_OUT_MODE_CI_2,//column interlaved 2
        VIRTUAL_HWC_3D_OUT_MODE_CI_3                = 0x7,  //HWC_3D_OUT_MODE_CI_3,//column interlaved 3
        VIRTUAL_HWC_3D_OUT_MODE_CI_4                = 0x8,  //HWC_3D_OUT_MODE_CI_4,//column interlaved 4

        VIRTUAL_HWC_3D_OUT_MODE_HDMI_3D_720P50_FP   = 0x9,  //HWC_3D_OUT_MODE_HDMI_3D_720P50_FP,
        VIRTUAL_HWC_3D_OUT_MODE_HDMI_3D_720P60_FP   = 0xa,  //HWC_3D_OUT_MODE_HDMI_3D_720P60_FP,
        VIRTUAL_HWC_3D_OUT_MODE_UNKNOWN     = -1,
    }VirtualHWCDisplayMode;   //virtual to HWC_3D_OUT_MODE_ANAGLAGH
  #else
    #error "Unknown chip type2!"
  #endif
#else
    #error "Unknown chip type!"
#endif

//typedef struct tag_VirtualVideo3DInfo
//{
//	unsigned int        width;
//	unsigned int        height;
	//unsigned int        format; // HWC_FORMAT_MBYUV422(hwcomposer.h) or HAL_PIXEL_FORMAT_YV12(graphics.h).
//	int  format; // VirtualHWCRenderFormat, virtual to HWC_FORMAT_MBYUV422(hwcomposer.h) or HAL_PIXEL_FORMAT_YV12(graphics.h).
//	int     src_mode;   //VirtualHWC3DSrcMode, virtual to HWC_3D_SRC_MODE_TB
//	int   display_mode;   //VirtualHWCDisplayMode, virtual to HWC_3D_OUT_MODE_ANAGLAGH(hwcomposer.h)
//}VirtualVideo3DInfo;    //virtual to video3Dinfo_t


#if (CEDARX_ANDROID_VERSION >= 9)
  #if (defined(__CHIP_VERSION_F81))
    enum VIDEORENDER_CMD
    {
    	VIDEORENDER_CMD_UNKOWN = 0,
        VIDEORENDER_CMD_ROTATION_DEG    = 1,    //HWC_LAYER_ROTATION_DEG, //HWC_LAYER_ROTATION_DEG    ,
        VIDEORENDER_CMD_DITHER          = 3,    //HWC_LAYER_DITHER, //HWC_LAYER_DITHER          ,
        VIDEORENDER_CMD_SETINITPARA     = 4,    //HWC_LAYER_SETINITPARA, //HWC_LAYER_SETINITPARA     ,
        VIDEORENDER_CMD_SETVIDEOPARA    = 5,    //HWC_LAYER_SETVIDEOPARA, //HWC_LAYER_SETVIDEOPARA    ,
        VIDEORENDER_CMD_SETFRAMEPARA    = 6,    //HWC_LAYER_SETFRAMEPARA, //HWC_LAYER_SETFRAMEPARA    ,
        VIDEORENDER_CMD_GETCURFRAMEPARA = 7,    //HWC_LAYER_GETCURFRAMEPARA, //HWC_LAYER_GETCURFRAMEPARA ,
        VIDEORENDER_CMD_QUERYVBI        = 8,    //HWC_LAYER_QUERYVBI, //HWC_LAYER_QUERYVBI        ,
        VIDEORENDER_CMD_SETSCREEN       = 9,    //HWC_LAYER_SETMODE,
        VIDEORENDER_CMD_SHOW            = 0xa, //HWC_LAYER_SHOW, //HWC_LAYER_SHOW            ,
        VIDEORENDER_CMD_RELEASE         = 0xb, //HWC_LAYER_RELEASE, //HWC_LAYER_RELEASE         ,
        VIDEORENDER_CMD_SET3DMODE       = 0xc,    //HWC_LAYER_SET3DMODE, //HWC_LAYER_SET3DMODE       ,
        VIDEORENDER_CMD_SETFORMAT       = 0xd,    //HWC_LAYER_SETFORMAT, //HWC_LAYER_SETFORMAT       ,
        VIDEORENDER_CMD_VPPON           = 0xe,    //HWC_LAYER_VPPON, //HWC_LAYER_VPPON           ,
        VIDEORENDER_CMD_VPPGETON        = 0xf,    //HWC_LAYER_VPPGETON, //HWC_LAYER_VPPGETON        ,
        VIDEORENDER_CMD_SETLUMASHARP    = 0x10,    //HWC_LAYER_SETLUMASHARP,//HWC_LAYER_SETLUMASHARP    ,
        VIDEORENDER_CMD_GETLUMASHARP    = 0x11,    //HWC_LAYER_GETLUMASHARP, //HWC_LAYER_GETLUMASHARP    ,
        VIDEORENDER_CMD_SETCHROMASHARP  = 0x12,    //HWC_LAYER_SETCHROMASHARP, //HWC_LAYER_SETCHROMASHARP  ,
        VIDEORENDER_CMD_GETCHROMASHARP  = 0x13,    //HWC_LAYER_GETCHROMASHARP,//HWC_LAYER_GETCHROMASHARP  ,
        VIDEORENDER_CMD_SETWHITEEXTEN   = 0x14,    //HWC_LAYER_SETWHITEEXTEN, //HWC_LAYER_SETWHITEEXTEN   ,
        VIDEORENDER_CMD_GETWHITEEXTEN   = 0x15,    //HWC_LAYER_GETWHITEEXTEN, //HWC_LAYER_GETWHITEEXTEN   ,
        VIDEORENDER_CMD_SETBLACKEXTEN   = 0x16,    //HWC_LAYER_SETBLACKEXTEN, //HWC_LAYER_SETBLACKEXTEN   ,
        VIDEORENDER_CMD_GETBLACKEXTEN   = 0x17,    //HWC_LAYER_GETBLACKEXTEN, //HWC_LAYER_GETBLACKEXTEN   ,

        VIDEORENDER_CMD_SETLAYERORDER   = 0x100,                   //DISP_CMD_LAYER_TOP, DISP_CMD_LAYER_BOTTOM ,
        VIDEORENDER_CMD_SET_CROP		= 0x1000         ,
    };
  #else
    #error "Unknown chip type2!"
  #endif
#else
    #error "Unknown chip type!"
#endif

#endif

typedef struct tag_VIRTUALLIBHWCLAYERPARA
{
    unsigned long   number;             //frame_id

    cedarv_3d_mode_e            source3dMode;   //cedarv_3d_mode_e, CEDARV_3D_MODE_DOUBLE_STREAM
    //cedarx_display_3d_mode_e    displayMode;    //cedarx_display_3d_mode_e, CEDARX_DISPLAY_3D_MODE_3D
    //cedarv_pixel_format_e       pixel_format;   //cedarv_pixel_format_e, CEDARV_PIXEL_FORMAT_MB_UV_COMBINE_YUV420
    EPIXELFORMAT    pixel_format;
    
    unsigned long   top_y;              // the address of frame buffer, which contains top field luminance
    unsigned long   top_u;  //top_c;              // the address of frame buffer, which contains top field chrominance
    unsigned long   top_v;
    unsigned long   top_y2;
    unsigned long   top_u2;
    unsigned long   top_v2;

    unsigned long   size_top_y;
    unsigned long   size_top_u;
    unsigned long   size_top_v;
    unsigned long   size_top_y2;
    unsigned long   size_top_u2;
    unsigned long   size_top_v2;
    
    signed char     bProgressiveSrc;    // Indicating the source is progressive or not
    signed char     bTopFieldFirst;     // VPO should check this flag when bProgressiveSrc is FALSE
    unsigned long   flag_addr;          //dit maf flag address
    unsigned long   flag_stride;        //dit maf flag line stride
    unsigned char   maf_valid;
    unsigned char   pre_frame_valid;
}Virtuallibhwclayerpara;   //libhwclayerpara_t, [hwcomposer.h]

//#if (defined(__CHIP_VERSION_F23) || defined(__CHIP_VERSION_F51) || (defined(__CHIP_VERSION_F33) && CEDARX_ANDROID_VERSION<8) || (defined(__CHIP_VERSION_F50)))
//extern int convertlibhwclayerpara_NativeRendererVirtual2Arch(libhwclayerpara_t *pDes, Virtuallibhwclayerpara *pSrc);
//extern int convertlibhwclayerpara_SoftwareRendererVirtual2Arch(libhwclayerpara_t *pDes, Virtuallibhwclayerpara *pSrc);
//#elif ((defined(__CHIP_VERSION_F33) && CEDARX_ANDROID_VERSION>=9))
//#else
//    #error "Unknown chip type!"
//#endif
}
#endif  /* _VIRTUAL_HWCOMPOSER_H_ */

