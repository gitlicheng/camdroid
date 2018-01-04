#ifndef __QRDECODE_INTERFACE_H__
#define __QRDECODE_INTERFACE_H__

#ifdef __cplusplus
extern "C" {
#endif

int zbar_dec(int width, int height, char* greyData, char* result, int result_len);

#ifdef __cplusplus
}
#endif

#endif
