#ifndef _DISP_ROTATION_SW_H_
#define _DISP_ROTATION_SW_H_

#include "disp_private.h"

//#define WRITE_SEQUENTIAL_CONFIG

s32 disp_init_rotation_sw(disp_bsp_init_para * para);


//------------------- INLINE FUNCTIONS --------------------//

inline static void rot_0_u8(
	unsigned char *src_addr, const unsigned int src_widthstep,
    const unsigned int src_width, const unsigned int src_height,
	unsigned char *dst_addr, const unsigned int dst_widthstep)
{
	unsigned char *src_addr_end = src_addr
		 + src_widthstep * src_height;
	for(; src_addr != src_addr_end;) {
		memcpy((void *)dst_addr, (void *)src_addr, src_width);
		dst_addr += dst_widthstep;
		src_addr += src_widthstep;
	}
}

inline static void rot_0_u16(
	unsigned short *src_addr, const unsigned int src_widthstep,
    const unsigned int src_width, const unsigned int src_height,
	unsigned short *dst_addr, const unsigned int dst_widthstep)
{
	const unsigned int width_size = src_width << 1;
	rot_0_u8((unsigned char *)src_addr, src_widthstep, width_size,
		src_height, (unsigned char *)dst_addr, dst_widthstep);
}

inline static void rot_0_u32(
	unsigned int *src_addr, const unsigned int src_widthstep,
    const unsigned int src_width, const unsigned int src_height,
	unsigned int *dst_addr, const unsigned int dst_widthstep)
{
	const unsigned int width_size = src_width << 2;
	rot_0_u8((unsigned char *)src_addr, src_widthstep, width_size,
		src_height, (unsigned char *)dst_addr, dst_widthstep);
}

inline static void rot_180_u8(
	unsigned char *src_addr, const unsigned int src_widthstep,
    const unsigned int src_width, const unsigned int src_height,
	unsigned char *dst_addr, const unsigned int dst_widthstep)
{
	unsigned char *p_src, *p_src_end, *p_dst;
	unsigned char *dst_addr_end = dst_addr
		- dst_widthstep + src_width - 1;
	dst_addr = dst_addr_end + dst_widthstep * src_height;
	for(; dst_addr != dst_addr_end;) {
		p_src = src_addr;
		p_src_end = p_src + src_width;
		p_dst = dst_addr;
		for(; p_src != p_src_end;) {
			*p_dst-- = *p_src++;
		}
		src_addr += src_widthstep;
		dst_addr -= dst_widthstep;
	}
}

inline static void rot_180_u16(
	unsigned short *src_addr, const unsigned int src_widthstep,
    const unsigned int src_width, const unsigned int src_height,
	unsigned short *dst_addr, const unsigned int dst_widthstep)
{
	unsigned short *p_src, *p_src_end, *p_dst;
	unsigned short *dst_addr_end = dst_addr + src_width - 1;
	dst_addr_end = (unsigned short *)((char *)dst_addr_end
		 - dst_widthstep);
	dst_addr  = (unsigned short *)((char *)dst_addr_end
		 + dst_widthstep * src_height);
	for(; dst_addr != dst_addr_end;) {
		p_src = src_addr;
		p_src_end = p_src + src_width;
		p_dst = dst_addr;
		for(; p_src != p_src_end;) {
			*p_dst-- = *p_src++;
		}
		src_addr = (unsigned short *)((char *)src_addr + src_widthstep);
		dst_addr = (unsigned short *)((char *)dst_addr - dst_widthstep);
	}
}

inline static void rot_180_u32(
	unsigned int *src_addr, const unsigned int src_widthstep,
    const unsigned int src_width, const unsigned int src_height,
	unsigned int *dst_addr, const unsigned int dst_widthstep)
{
	unsigned int *p_src, *p_src_end, *p_dst;
	unsigned int *dst_addr_end = dst_addr + src_width - 1;
	dst_addr_end  = (unsigned int *)((char *)dst_addr_end
		 - dst_widthstep);
	dst_addr = (unsigned int *)((char *)dst_addr_end
		 + dst_widthstep * src_height);
	for(; dst_addr != dst_addr_end;) {
		p_src = src_addr;
		p_src_end = p_src + src_width;
		p_dst = dst_addr;
		for(; p_src != p_src_end;) {
			*p_dst-- = *p_src++;
		}
		src_addr = (unsigned int *)((char *)src_addr + src_widthstep);
		dst_addr = (unsigned int *)((char *)dst_addr - dst_widthstep);
	}
}


inline static void rot_270_u8(
	unsigned char *src_addr, const unsigned int src_widthstep,
    const unsigned int src_width, const unsigned int src_height,
	unsigned char *dst_addr, const unsigned int dst_widthstep)
{
	unsigned char *p_src, *p_src_end, *p_dst, *dst_addr_end;
	dst_addr += (dst_widthstep * (src_width - 1));
	dst_addr_end = dst_addr + src_height;
	for(; dst_addr != dst_addr_end; dst_addr++) {
		p_src = src_addr;
		p_src_end = p_src + src_width;
		p_dst = dst_addr;
		for(; p_src != p_src_end;) {
			*p_dst = *p_src++;
			p_dst -= dst_widthstep;
		}
		src_addr += src_widthstep;
	}
}

inline static void rot_270_u16(
	unsigned short *src_addr, const unsigned int src_widthstep,
    const unsigned int src_width, const unsigned int src_height,
	unsigned short *dst_addr, const unsigned int dst_widthstep)
{
	unsigned short *p_src, *p_src_end, *p_dst, *dst_addr_end;
	dst_addr = (unsigned short *)((char *)dst_addr
		 + dst_widthstep * (src_width - 1));
	dst_addr_end = dst_addr + src_height;
	for(; dst_addr != dst_addr_end; dst_addr++) {
		p_src = src_addr;
		p_src_end = p_src + src_width;
		p_dst = dst_addr;
		for(; p_src != p_src_end;) {
			*p_dst = *p_src++;
			p_dst = (unsigned short *)((char *)p_dst - dst_widthstep);
		}
		src_addr = (unsigned short *)((char *)src_addr + src_widthstep);
	}
}

inline static void rot_270_u32(
	unsigned int *src_addr, const unsigned int src_widthstep,
    const unsigned int src_width, const unsigned int src_height,
	unsigned int *dst_addr, const unsigned int dst_widthstep)
{
	unsigned int *p_src, *p_src_end, *p_dst, *dst_addr_end;
	dst_addr = (unsigned int *)((char *)dst_addr
		 + dst_widthstep * (src_width - 1));
	dst_addr_end = dst_addr + src_height;
	for(; dst_addr != dst_addr_end; dst_addr++) {
		p_src = src_addr;
		p_src_end = p_src + src_width;
		p_dst = dst_addr;
		for(; p_src != p_src_end;) {
			*p_dst = *p_src++;
			p_dst = (unsigned int *)((char *)p_dst - dst_widthstep);
		}
		src_addr = (unsigned int *)((char *)src_addr + src_widthstep);
	}
}

inline static void rot_90_u8(
	unsigned char *src_addr, const unsigned int src_widthstep,
	const unsigned int src_width, const unsigned int src_height,
	unsigned char *dst_addr, const unsigned int dst_widthstep)
{
	unsigned char *p_src, *p_src_end, *p_dst;
	unsigned char *dst_addr_end = dst_addr - 1;
	dst_addr = dst_addr_end + src_height;
	for(; dst_addr != dst_addr_end; dst_addr--) {
		p_src = src_addr;
		p_src_end = p_src + src_width;
		p_dst = dst_addr;
		for(; p_src != p_src_end;) {
			*p_dst = *p_src++;
			p_dst += dst_widthstep;
		}
		src_addr += src_widthstep;
	}
}

inline static void rot_90_u16(
	unsigned short *src_addr, const unsigned int src_widthstep,
	const unsigned int src_width, const unsigned int src_height,
	unsigned short *dst_addr, const unsigned int dst_widthstep)
{
	unsigned short *p_src, *p_src_end, *p_dst;
	unsigned short *dst_addr_end = dst_addr - 1;
	dst_addr = dst_addr_end + src_height;
	for(; dst_addr != dst_addr_end; dst_addr--) {
		p_src = src_addr;
		p_src_end = p_src + src_width;
		p_dst = dst_addr;
		for(; p_src != p_src_end;) {
			*p_dst = *p_src++;
			p_dst = (unsigned short *)((char *)p_dst + dst_widthstep);
		}
		src_addr = (unsigned short *)((char *)src_addr + src_widthstep);
	}
}

inline static void rot_90_u32(
	unsigned int *src_addr, const unsigned int src_widthstep,
	const unsigned int src_width, const unsigned int src_height,
	unsigned int *dst_addr, const unsigned int dst_widthstep)
{
	unsigned int *p_src, *p_src_end, *p_dst;
	unsigned int *dst_addr_end = dst_addr - 1;
	dst_addr = dst_addr_end + src_height;
	for(; dst_addr != dst_addr_end; dst_addr--) {
		p_src = src_addr;
		p_src_end = p_src + src_width;
		p_dst = dst_addr;
		for(; p_src != p_src_end;) {
			*p_dst = *p_src++;
			p_dst = (unsigned int *)((char *)p_dst + dst_widthstep);
		}
		src_addr = (unsigned int *)((char *)src_addr + src_widthstep);
	}
}

#ifdef WRITE_SEQUENTIAL_CONFIG

inline static void rot_180_seq_w_u8(
	unsigned char *src_addr, const unsigned int src_widthstep,
    const unsigned int src_width, const unsigned int src_height,
	unsigned char *dst_addr, const unsigned int dst_widthstep)
{
	unsigned char *p_src, *p_dst, *p_dst_end;
	unsigned char *src_addr_end = src_addr
		 - src_widthstep + src_width - 1;
	src_addr = src_addr_end + src_widthstep * src_height;
	for(; src_addr != src_addr_end;) {
		p_src = src_addr;
		p_dst = dst_addr;
		p_dst_end = p_dst + src_width;
		for(; p_dst != p_dst_end;) {
			*p_dst++ = *p_src--;
		}
		dst_addr += dst_widthstep;
		src_addr -= src_widthstep;
	}
}

inline static void rot_180_seq_w_u16(
	unsigned short *src_addr, const unsigned int src_widthstep,
    const unsigned int src_width, const unsigned int src_height,
	unsigned short *dst_addr, const unsigned int dst_widthstep)
{
	unsigned short *p_src, *p_dst, *p_dst_end;
	unsigned short *src_addr_end = src_addr + src_width - 1;
	src_addr_end = (unsigned short *)((char *)src_addr_end
		- src_widthstep);
	src_addr = (unsigned short *)((char *)src_addr_end
		 + src_widthstep * src_height);
	for(; src_addr != src_addr_end;) {
		p_src = src_addr;
		p_dst = dst_addr;
		p_dst_end = p_dst + src_width;
		for(; p_dst != p_dst_end;) {
			*p_dst++ = *p_src--;
		}
		src_addr = (unsigned short *)((char *)src_addr - src_widthstep);
		dst_addr = (unsigned short *)((char *)dst_addr + dst_widthstep);
	}
}

inline static void rot_180_seq_w_u32(
	unsigned int *src_addr,	const unsigned int src_widthstep,
    const unsigned int src_width, const unsigned int src_height,
	unsigned int *dst_addr, const unsigned int dst_widthstep)
{
	unsigned int *p_src, *p_dst, *p_dst_end;
	unsigned int *src_addr_end = src_addr + src_width - 1;
	src_addr_end  = (unsigned int *)((char *)src_addr_end
		 - src_widthstep);
	src_addr = (unsigned int *)((char *)src_addr_end
		 + src_widthstep * src_height);
	for(; src_addr != src_addr_end;) {
		p_src = src_addr;
		p_dst = dst_addr;
		p_dst_end = p_dst + src_width;
		for(; p_dst != p_dst_end;) {
			*p_dst++ = *p_src--;
		}
		src_addr = (unsigned int *)((char *)src_addr - src_widthstep);
		dst_addr = (unsigned int *)((char *)dst_addr + dst_widthstep);
	}
}

inline static void rot_90_seq_w_u8(
	unsigned char *src_addr, const unsigned int src_widthstep,
	const unsigned int src_width, const unsigned int src_height,
	unsigned char *dst_addr, const unsigned int dst_widthstep)
{
	unsigned char *p_src, *src_addr_end, *p_dst, *p_dst_end;
	src_addr += (src_widthstep * (src_height - 1));
	src_addr_end = src_addr + src_width;
	for(; src_addr != src_addr_end; src_addr++) {
		p_src = src_addr;
		p_dst = dst_addr;
		p_dst_end = p_dst + src_height;
		for(; p_dst != p_dst_end;) {
			*p_dst++ = *p_src;
			p_src -= src_widthstep;
		}
		dst_addr += dst_widthstep;
	}
}

inline static void rot_90_seq_w_u16(
	unsigned short *src_addr, const unsigned int src_widthstep,
	const unsigned int src_width, const unsigned int src_height,
	unsigned short *dst_addr, const unsigned int dst_widthstep)
{
	unsigned short *p_src, *src_addr_end, *p_dst, *p_dst_end;
	src_addr = (unsigned short *)((char *)src_addr
		 + (src_widthstep * (src_height - 1)));
	src_addr_end = src_addr + src_width;
	for(; src_addr != src_addr_end; src_addr++) {
		p_src = src_addr;
		p_dst = dst_addr;
		p_dst_end = p_dst + src_height;
		for(; p_dst != p_dst_end;) {
			*p_dst++ = *p_src;
			p_src = (unsigned short *)((char *)p_src - src_widthstep);
		}
		dst_addr = (unsigned short *)((char *)dst_addr + dst_widthstep);
	}
}

inline static void rot_90_seq_w_u32(
	unsigned int *src_addr, const unsigned int src_widthstep,
	const unsigned int src_width, const unsigned int src_height,
	unsigned int *dst_addr, const unsigned int dst_widthstep)
{
	unsigned int *p_src, *src_addr_end, *p_dst, *p_dst_end;
	src_addr = (unsigned int *)((char *)src_addr
		 + src_widthstep * (src_height - 1));
	src_addr_end = src_addr + src_width;
	for(; src_addr != src_addr_end; src_addr++) {
		p_src = src_addr;
		p_dst = dst_addr;
		p_dst_end = p_dst + src_height;
		for(; p_dst != p_dst_end;) {
			*p_dst++ = *p_src;
			p_src = (unsigned int *)((char *)p_src - src_widthstep);
		}
		dst_addr = (unsigned int *)((char *)dst_addr + dst_widthstep);
	}
}

inline static void rot_270_seq_w_u8(
	unsigned char *src_addr, const unsigned int src_widthstep,
    const unsigned int src_width, const unsigned int src_height,
	unsigned char *dst_addr, const unsigned int dst_widthstep)
{
	unsigned char *p_src, *p_dst, *p_dst_end;
	unsigned char *src_addr_end = src_addr - 1;
	src_addr = src_addr_end + src_width;
	for(; src_addr != src_addr_end; src_addr--) {
		p_src = src_addr;
		p_dst = dst_addr;
		p_dst_end = p_dst + src_height;
		for(; p_dst != p_dst_end;) {
			*p_dst++ = *p_src;
			p_src += src_widthstep;
		}
		dst_addr += dst_widthstep;
	}
}

inline static void rot_270_seq_w_u16(
	unsigned short *src_addr, const unsigned int src_widthstep,
    const unsigned int src_width, const unsigned int src_height,
	unsigned short *dst_addr, const unsigned int dst_widthstep)
{
	unsigned short *p_src, *p_dst, *p_dst_end;
	unsigned short *src_addr_end = src_addr - 1;
	src_addr = src_addr_end + src_width;
	for(; src_addr != src_addr_end; src_addr--) {
		p_src = src_addr;
		p_dst = dst_addr;
		p_dst_end = p_dst + src_height;
		for(; p_dst != p_dst_end;) {
			*p_dst++ = *p_src;
			p_src = (unsigned short *)((char *)p_src + src_widthstep);
		}
		dst_addr = (unsigned short *)((char *)dst_addr + dst_widthstep);
	}
}

inline static void rot_270_seq_w_u32(
	unsigned int *src_addr,	const unsigned int src_widthstep,
    const unsigned int src_width, const unsigned int src_height,
	unsigned int *dst_addr, const unsigned int dst_widthstep)
{
	unsigned int *p_src, *p_dst, *p_dst_end;
	unsigned int *src_addr_end = src_addr - 1;
	src_addr = src_addr_end + src_width;
	for(; src_addr != src_addr_end; src_addr--) {
		p_src = src_addr;
		p_dst = dst_addr;
		p_dst_end = dst_addr + src_height;
		for(; p_dst != p_dst_end;) {
			*p_dst++ = *p_src;
			p_src = (unsigned int *)((char *)p_src + src_widthstep);
		}
		dst_addr = (unsigned int *)((char *)dst_addr + dst_widthstep);
	}
}

#endif //#ifdef WRITE_SEQUENTIAL_CONFIG

#endif // #ifndef _DISP_ROTATION_SW_H_
