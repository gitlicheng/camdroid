#include <stdlib.h>

static int cubic_coeff[]={//32-taps
	0, 256,   0,   0,
	-3, 255,   4,   0,
	-6, 254,  10,   0,
	-9, 251,  16,   0,
	-11, 247,  23,  -1,
	-13, 242,  31,  -2,
	-15, 236,  39,  -3,
	-16, 229,  48,  -4,
	-17, 222,  58,  -5,
	-18, 214,  68,  -6,
	-18, 205,  78,  -8,
	-18, 196,  89,  -9,
	-18, 186, 100, -10,
	-17, 176, 111, -12,
	-17, 166, 122, -13,
	-16, 155, 133, -14,
	-15, 144, 144, -15,
	-14, 133, 155, -16,
	-13, 122, 166, -17,
	-12, 111, 176, -17,
	-10, 100, 186, -18,
	-9,  89, 196, -18,
	-8,  78, 205, -18,
	-6,  68, 214, -18,
	-5,  58, 222, -17,
	-4,  48, 229, -16,
	-3,  39, 236, -15,
	-2,  31, 242, -13,
	-1,  23, 247, -11,
	0,  16, 251,  -9,
	0,  10, 254,  -6,
	0,   4, 255,  -3,
};
#define  MAX(a,b)              (((a) > (b)) ? (a) : (b))
#define  CLIP(a,i,s)           (((a) > (s)) ? (s) : MAX(a,i))


typedef struct ISP_CTX 
{
	int SC_dst_format;			//0-420 1-422 2-444
	int SC_hor_filter_type;
	int width;
	int	height;
	int SC_dst_width;
	int SC_dst_height;
	int outpicsize;
	int SC_luma_hor_ratio;
	int SC_luma_ver_ratio;
	int SC_luma_ver_init_phase;
	int SC_luma_hor_init_phase;
	int SC_chroma_hor_init_phase;
	int SC_chroma_ver_init_phase;
	int SC_HOR_FILTER_COEFF0[16][4];//4-taps
	int SC_VER_FILTER_COEFF[32][2];//2-taps
}ISP_CTX;


void SC_hor_filter(ISP_CTX *ispctx,unsigned char *dst_buf,unsigned char *src_ptr,int src_width,int dst_width,int ratio,int init_phase,int filter_type,int isLuma)
{
	int j;
	int base_pixel,hphase,r = 0;
	unsigned char x[4],*td_buf;
	int *C;

	td_buf = (unsigned char *)malloc(dst_width+4);

	for(j=0;j<dst_width;j++)
	{
		base_pixel = (j*ratio+init_phase)>>8;
		hphase = j*ratio+init_phase - (base_pixel<<8);

		if(base_pixel-1<0)
			x[0] = src_ptr[0];
		else if(base_pixel-1>=src_width)
			x[0] = src_ptr[src_width-1];
		else
			x[0] = src_ptr[base_pixel-1];

		if(base_pixel<0)
			x[1] = src_ptr[0];
		else if(base_pixel>=src_width)
			x[1] = src_ptr[src_width-1];
		else
			x[1] = src_ptr[base_pixel];

		if(base_pixel+1<0)
			x[2] = src_ptr[0];
		else if(base_pixel+1>=src_width)
			x[2] = src_ptr[src_width-1];
		else
			x[2] = src_ptr[base_pixel+1];

		if(base_pixel+2<0)
			x[3] = src_ptr[0];
		else if(base_pixel+2>=src_width)
			x[3] = src_ptr[src_width-1];
		else
			x[3] = src_ptr[base_pixel+2];

		if(filter_type==0)
		{//4-taps
			C = ispctx->SC_HOR_FILTER_COEFF0[hphase>>4];
			r = x[0]*C[0] + x[1]*C[1] + x[2]*C[2] + x[3]*C[3];
			r += 128;
			r >>= 8;
		}
		td_buf[j+2] = CLIP(r,0,255);
	}
	td_buf[0] = td_buf[1] = td_buf[2];
	td_buf[2+dst_width] = td_buf[3+dst_width] = td_buf[1+dst_width];

	memcpy(dst_buf,td_buf+2,dst_width);

	free(td_buf);
}

void do_scaler(ISP_CTX *ispctx,unsigned char * psrc, unsigned char * pdst, int src_w, int src_h, int dst_w, int dst_h, int fmt, int align)
{
	int i,j,*C,r;
	unsigned char *scline_buf[2],*lastline,*curline;
	int line_num[2],base_line,base_linep1;
	unsigned char *dst_ptr,*CbPtr,*CrPtr,*DstBuf;
	int vphase,hratio = 0,vratio,init_vphase;
	int src_height = 0, src_width = 0, dst_width = 0, dst_height = 0;
	int align_width,sub_num,cro_align_width,cro_sub_num;

	scline_buf[0] = (unsigned char *)malloc(ispctx->SC_dst_width*2);
	scline_buf[1] = scline_buf[0] + ispctx->SC_dst_width;
	DstBuf = (unsigned char *)malloc(ispctx->outpicsize*3);
	//DstBuf = (unsigned char *)VirtualAlloc(0,ispctx->outpicsize*3,MEM_COMMIT,PAGE_READWRITE);
	dst_ptr = DstBuf;

	//calculate luminance 
	line_num[0] = line_num[1] = -1;
	vratio = ispctx->SC_luma_ver_ratio;
	init_vphase = (ispctx->SC_luma_ver_init_phase);
	align_width = (src_w + align - 1) & (~(align - 1));
	sub_num = align_width - src_w;
	cro_align_width = ((src_w>>1) + align/2 - 1) & (~(align/2 - 1));
	cro_sub_num = cro_align_width  - (src_w>>1);

	for(i=0; i<ispctx->SC_dst_height; i++)
	{
		base_line = (i*vratio+init_vphase)>>8;
		vphase = (i*vratio+init_vphase) - (base_line<<8);

		{
			if(base_line >= ispctx->height)
				base_line = ispctx->height-1;

			base_linep1 = base_line+1;
			if(base_linep1 >= ispctx->height)
				base_linep1 = ispctx->height-1;
		}

		if(base_line == line_num[0])
		{
			lastline = scline_buf[0];
		}
		else if(base_line == line_num[1])
		{
			lastline = scline_buf[1];
		}
		else
		{	
			lastline = scline_buf[0];
			line_num[0] = base_line;

			//need to fill
			SC_hor_filter(ispctx,lastline,psrc+base_line*align_width,ispctx->width,ispctx->SC_dst_width,ispctx->SC_luma_hor_ratio,ispctx->SC_luma_hor_init_phase,0,1);
		}

		if(base_linep1 == line_num[0])
		{
			curline = scline_buf[0];
		}
		else if(base_linep1 == line_num[1])
		{
			curline = scline_buf[1];
		}
		else
		{
			if(base_line == line_num[0])
			{
				line_num[1] = base_linep1;
				curline = scline_buf[1];
			}
			else
			{
				line_num[0] = base_linep1;
				curline = scline_buf[0];
			}

			//need to fill

			SC_hor_filter(ispctx,curline,psrc+base_linep1*align_width,ispctx->width,ispctx->SC_dst_width,ispctx->SC_luma_hor_ratio,ispctx->SC_luma_hor_init_phase,0,1);
		}

		//vertical filter
		C = ispctx->SC_VER_FILTER_COEFF[vphase>>3];
		for(j=0;j<ispctx->SC_dst_width;j++)
		{
			r = lastline[j]*C[0] + curline[j]*C[1];
			r += 128;
			r >>= 8;
			*dst_ptr++ = CLIP(r,0,255);
		}
		dst_ptr += sub_num;
	}

	//decide the chroma aspect ratio
	switch(fmt)
	{//0-yuv420, 1-yuv422, 2-yvu420, 3-yvu422,4-yuyv422, 5-32x32,8-argb8888, 9-rgba8888, 10-abgr8888, 11-bgra8888
	case 0:
		{//src is 4:2:0, dst is 4:2:0
			hratio = ispctx->SC_luma_hor_ratio;
			vratio = ispctx->SC_luma_ver_ratio;
			dst_width = ispctx->SC_dst_width>>1;
			dst_height = ispctx->SC_dst_height>>1;
		}
		src_width = ispctx->width>>1;
		src_height = ispctx->height>>1;
		break;
	default:
		break;
	}

	//calculate chrominance Cb
	CbPtr = psrc + align_width*ispctx->height;
	line_num[0] = line_num[1] = -1;
	vratio = ispctx->SC_luma_ver_ratio;
	init_vphase = (ispctx->SC_chroma_ver_init_phase);
	for(i=0;i<dst_height;i++)
	{
		base_line = (i*vratio+init_vphase)>>8;
		vphase = (i*vratio+init_vphase) - (base_line<<8);

		{
			if(base_line >= src_height)
				base_line = src_height-1;

			base_linep1 = base_line+1;
			if(base_linep1 >= src_height)
				base_linep1 = src_height-1;
		}

		if(base_line == line_num[0])
		{
			lastline = scline_buf[0];
		}
		else if(base_line == line_num[1])
		{
			lastline = scline_buf[1];
		}
		else
		{	
			lastline = scline_buf[0];
			line_num[0] = base_line;

			//need to fill
			SC_hor_filter(ispctx,lastline,CbPtr+base_line*cro_align_width,src_width,dst_width,hratio,ispctx->SC_chroma_hor_init_phase,0,0);
		}

		if(base_linep1 == line_num[0])
		{
			curline = scline_buf[0];
		}
		else if(base_linep1 == line_num[1])
		{
			curline = scline_buf[1];
		}
		else
		{
			if(base_line == line_num[0])
			{
				line_num[1] = base_linep1;
				curline = scline_buf[1];
			}
			else
			{
				line_num[0] = base_linep1;
				curline = scline_buf[0];
			}

			//need to fill
			SC_hor_filter(ispctx,curline,CbPtr+base_linep1*cro_align_width,src_width,dst_width,hratio,ispctx->SC_chroma_hor_init_phase,0,0);
		}

		//vertical filter
		C = ispctx->SC_VER_FILTER_COEFF[vphase>>3];
		for(j=0;j<dst_width;j++)
		{
			r = lastline[j]*C[0] + curline[j]*C[1];
			r += 128;
			r >>= 8;
			*dst_ptr++ = CLIP(r,0,255);
		}
		dst_ptr += cro_sub_num;
	}

	CrPtr = CbPtr + cro_align_width*src_height;
	line_num[0] = line_num[1] = -1;
	vratio = ispctx->SC_luma_ver_ratio;
	init_vphase = (ispctx->SC_chroma_ver_init_phase);
	for(i=0;i<dst_height;i++)
	{
		base_line = (i*vratio+init_vphase)>>8;
		vphase = (i*vratio+init_vphase) - (base_line<<8);


		{
			if(base_line >= src_height)
				base_line = src_height-1;

			base_linep1 = base_line+1;
			if(base_linep1 >= src_height)
				base_linep1 = src_height-1;
		}

		if(base_line == line_num[0])
		{
			lastline = scline_buf[0];
		}
		else if(base_line == line_num[1])
		{
			lastline = scline_buf[1];
		}
		else
		{	
			lastline = scline_buf[0];
			line_num[0] = base_line;

			//need to fill
			SC_hor_filter(ispctx,lastline,CrPtr+base_line*cro_align_width,src_width,dst_width,hratio,ispctx->SC_chroma_hor_init_phase,0,0);
		}

		if(base_linep1 == line_num[0])
		{
			curline = scline_buf[0];
		}
		else if(base_linep1 == line_num[1])
		{
			curline = scline_buf[1];
		}
		else
		{
			if(base_line == line_num[0])
			{
				line_num[1] = base_linep1;
				curline = scline_buf[1];
			}
			else
			{
				line_num[0] = base_linep1;
				curline = scline_buf[0];
			}

			//need to fill

			SC_hor_filter(ispctx,curline,CrPtr+base_linep1*cro_align_width,src_width,dst_width,hratio,ispctx->SC_chroma_hor_init_phase,0,0);
		}

		//vertical filter
		C = ispctx->SC_VER_FILTER_COEFF[vphase>>3];
		for(j=0;j<dst_width;j++)
		{
			r = lastline[j]*C[0] + curline[j]*C[1];
			r += 128;
			r >>= 8;
			*dst_ptr++ = CLIP(r,0,255);
		}
		dst_ptr += cro_sub_num;
	}

	memcpy(pdst,DstBuf,ispctx->outpicsize);

	free(scline_buf[0]);
	free(DstBuf);
}

// fmt: only support YV12
int scaler(unsigned char * psrc, unsigned char * pdst, int src_w, int src_h, int dst_w, int dst_h, int fmt, int align)
{
	int i;
	ISP_CTX isp_ctx,*ispctx;
	ispctx = &isp_ctx;
	ispctx->width = src_w;
	ispctx->height = src_h;
	ispctx->SC_dst_width = dst_w;
	ispctx->SC_dst_height = dst_h;
	ispctx->SC_luma_ver_init_phase = 0;
	ispctx->SC_luma_hor_init_phase = 0;
	ispctx->SC_chroma_hor_init_phase = 0;
	ispctx->SC_chroma_ver_init_phase = 0;
	if(fmt == 0)
		ispctx->outpicsize = dst_w*dst_h*1.5;

	for(i=0;i<16;i++)
	{
		ispctx->SC_HOR_FILTER_COEFF0[i][0] = cubic_coeff[i*4*2];
		ispctx->SC_HOR_FILTER_COEFF0[i][1] = cubic_coeff[i*4*2+1];
		ispctx->SC_HOR_FILTER_COEFF0[i][2] = cubic_coeff[i*4*2+2];
		ispctx->SC_HOR_FILTER_COEFF0[i][3] = cubic_coeff[i*4*2+3];
	}

	for(i=0;i<32;i++)
	{
		ispctx->SC_VER_FILTER_COEFF[i][0] = (32-i)<<3;
		ispctx->SC_VER_FILTER_COEFF[i][1] = i<<3;
	}


	ispctx->SC_luma_hor_ratio = (src_w*256 + (dst_w/2))/dst_w;
	ispctx->SC_luma_ver_ratio = (src_h*256 + (dst_h/2))/dst_h;
	do_scaler(ispctx,psrc,pdst,src_w,src_h,dst_w,dst_h,fmt,align);
	return 0;
}