#ifndef __BUFFER_BITMAP_SOURCE_H__
#define __BUFFER_BITMAP_SOURCE_H__
#include <zxing/LuminanceSource.h>
#include <stdio.h>
#include <stdlib.h>

namespace zxing {

class BufferBitmapSource : public LuminanceSource {
  typedef LuminanceSource Super;
private:
  int width, height; 
  unsigned char * buffer; 

public:
  BufferBitmapSource(int inWidth, int inHeight, unsigned char * inBuffer); 
  ~BufferBitmapSource(); 

  int getWidth() const; 
  int getHeight() const; 
  ArrayRef<char> getRow(int y, ArrayRef<char> row) const; 
  ArrayRef<char> getMatrix() const; 
}; 
}
#endif
