#ifndef _CVBSDISP_H
#define _CVBSDISP_H

typedef enum
{
    DISP_MOD_INTERLEAVED        =0x1,   //interleaved,1个地址
    DISP_MOD_NON_MB_PLANAR      =0x0,   //无宏块平面模式,3个地址,RGB/YUV每个channel分别存放
    DISP_MOD_NON_MB_UV_COMBINED =0x2,   //无宏块UV打包模式,2个地址,Y和UV分别存放
    DISP_MOD_MB_PLANAR          =0x4,   //宏块平面模式,3个地址,RGB/YUV每个channel分别存放
    DISP_MOD_MB_UV_COMBINED     =0x6,   //宏块UV打包模式 ,2个地址,Y和UV分别存放
}__disp_pixel_mod_t;

typedef enum
{
    DISP_FORMAT_1BPP        =0x0,
    DISP_FORMAT_2BPP        =0x1,
    DISP_FORMAT_4BPP        =0x2,
    DISP_FORMAT_8BPP        =0x3,
    DISP_FORMAT_RGB655      =0x4,
    DISP_FORMAT_RGB565      =0x5,
    DISP_FORMAT_RGB556      =0x6,
    DISP_FORMAT_ARGB1555    =0x7,
    DISP_FORMAT_RGBA5551    =0x8,
    DISP_FORMAT_ARGB888     =0x9,//alpha padding to 0xff
    DISP_FORMAT_ARGB8888    =0xa,
    DISP_FORMAT_RGB888      =0xb,
    DISP_FORMAT_ARGB4444    =0xc,

    DISP_FORMAT_YUV444      =0x10,
    DISP_FORMAT_YUV422      =0x11,
    DISP_FORMAT_YUV420      =0x12,
    DISP_FORMAT_YUV411      =0x13,
    DISP_FORMAT_CSIRGB      =0x14,
}__disp_pixel_fmt_t;

typedef enum
{
//for interleave argb8888
    DISP_SEQ_ARGB   =0x0,//A在高位
    DISP_SEQ_BGRA   =0x2,
    
//for nterleaved yuv422
    DISP_SEQ_UYVY   =0x3,  
    DISP_SEQ_YUYV   =0x4,
    DISP_SEQ_VYUY   =0x5,
    DISP_SEQ_YVYU   =0x6,
    
//for interleaved yuv444
    DISP_SEQ_AYUV   =0x7,  
    DISP_SEQ_VUYA   =0x8,
    
//for uv_combined yuv420
    DISP_SEQ_UVUV   =0x9,  
    DISP_SEQ_VUVU   =0xa,
    
//for 16bpp rgb
    DISP_SEQ_P10    = 0xd,//p1在高位
    DISP_SEQ_P01    = 0xe,//p0在高位
    
//for planar format or 8bpp rgb
    DISP_SEQ_P3210  = 0xf,//p3在高位
    DISP_SEQ_P0123  = 0x10,//p0在高位
    
//for 4bpp rgb
    DISP_SEQ_P76543210  = 0x11,
    DISP_SEQ_P67452301  = 0x12,
    DISP_SEQ_P10325476  = 0x13,
    DISP_SEQ_P01234567  = 0x14,
    
//for 2bpp rgb
    DISP_SEQ_2BPP_BIG_BIG       = 0x15,//15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
    DISP_SEQ_2BPP_BIG_LITTER    = 0x16,//12,13,14,15,8,9,10,11,4,5,6,7,0,1,2,3
    DISP_SEQ_2BPP_LITTER_BIG    = 0x17,//3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12
    DISP_SEQ_2BPP_LITTER_LITTER = 0x18,//0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
    
//for 1bpp rgb
    DISP_SEQ_1BPP_BIG_BIG       = 0x19,//31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
    DISP_SEQ_1BPP_BIG_LITTER    = 0x1a,//24,25,26,27,28,29,30,31,16,17,18,19,20,21,22,23,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7
    DISP_SEQ_1BPP_LITTER_BIG    = 0x1b,//7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8,23,22,21,20,19,18,17,16,31,30,29,28,27,26,25,24
    DISP_SEQ_1BPP_LITTER_LITTER = 0x1c,//0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
}__disp_pixel_seq_t;

int cvbs_init(void);
int cvbs_exit(void);
void set_para(unsigned int src_w, unsigned int src_h, unsigned int scn_w, unsigned int scn_h,
	__disp_pixel_mod_t mode, __disp_pixel_fmt_t format, __disp_pixel_seq_t seq);
int set_addr(void *phy_addr0,  void *phy_addr1, void *phy_addr2);

#endif



