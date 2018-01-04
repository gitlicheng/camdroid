#ifndef __BMP_H__
#define __BMP_H__

#include <stdlib.h>
#include <stdio.h>

#ifndef __cplusplus
typedef char bool;
#define false 0
#define true 1
#endif

#pragma pack(1)
typedef struct
{
	//unsigned short    bfType;
	unsigned bfSize;
	unsigned short    bfReserved1;
	unsigned short    bfReserved2;
	unsigned bfOffBits;
} ClBitMapFileHeader;

typedef struct
{
	unsigned biSize; 
	int   biWidth; 
	int   biHeight; 
	unsigned short   biPlanes; 
	unsigned short   biBitCount;
	unsigned biCompression; 
	unsigned biSizeImage; 
	int   biXPelsPerMeter; 
	int   biYPelsPerMeter; 
	unsigned biClrUsed; 
	unsigned biClrImportant; 
} ClBitMapInfoHeader;

typedef struct 
{
	unsigned char rgbBlue;
	unsigned char rgbGreen;
	unsigned char rgbRed;
	unsigned char rgbReserved;
} ClRgbQuad;

typedef struct
{
	int width;
	int height;
	int channels;
	unsigned char* imageData;
}ClImage;

#pragma pack()

#ifdef __cplusplus 
extern "C"{
#endif

	ClImage* clLoadImage(char* path);
	bool clSaveImage(char* path, ClImage* bmpImg);

#ifdef __cplusplus 
}
#endif

#endif // JPEG_DECODER_H
