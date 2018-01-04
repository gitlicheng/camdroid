#include "BufferBitmapSource.h"
#include <iostream>

using zxing::Ref;
using zxing::ArrayRef;
using zxing::LuminanceSource;
using zxing::BufferBitmapSource;

BufferBitmapSource::BufferBitmapSource(int inWidth, int inHeight, unsigned char * inBuffer)	
	:Super(inWidth, inHeight)
{
	width = inWidth;
	height = inHeight; 
	buffer = inBuffer; 
}

BufferBitmapSource::~BufferBitmapSource()
{
}

int BufferBitmapSource::getWidth() const
{
	return width; 
}

int BufferBitmapSource::getHeight() const
{
	return height; 
}

ArrayRef<char> BufferBitmapSource::getRow(int y, ArrayRef<char> row) const
{
	if (y < 0 || y >= height) 
	{
		fprintf(stderr, "ERROR, attempted to read row %d of a %d height image.\n", y, height); 
		return NULL; 
	}
	// WARNING: NO ERROR CHECKING! You will want to add some in your code. 
	if (!row || row->size() < width) {
		ArrayRef<char> temp (width);
		row = temp;
	}
	for (int x = 0; x < width; x ++)
	{
		row[x] = buffer[y*width+x]; 
	}
	return row; 
}

ArrayRef<char> BufferBitmapSource::getMatrix() const
{
	ArrayRef<char> ret = ArrayRef<char>((char*)buffer, width*height); 
	return ret; 
}