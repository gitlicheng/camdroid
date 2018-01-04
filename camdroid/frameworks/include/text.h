#ifndef _TEXT_H
#define _TEXT_H

#define COLOR_white	(0xffffff)
#define COLOR_red	(0xff0000)
#define COLOR_green	(0x00ff00)
#define COLOR_blue	(0x0000ff)

/**********************************************************************
 * name	:	DrawInit
 * desc	:	
 * args	:	addr: buffer, w: width, h: height, bpp: bit_per_pixel
 * retn	:	0: successs <0: fail
 ***********************************************************************/
int DrawInit(unsigned char *addr, unsigned int w, unsigned int h, unsigned int bpp);

/**********************************************************************
 * name	:	DrawString
 * desc	:	draw text at (x, y)
 * args	:	str_utf8-utf8: utf8 string, x,y, color
 * retn	:	0
 ***********************************************************************/
int DrawString(char *str_utf8, unsigned int x, unsigned int y, int color);

#endif	//ifndef _TEXT_H
