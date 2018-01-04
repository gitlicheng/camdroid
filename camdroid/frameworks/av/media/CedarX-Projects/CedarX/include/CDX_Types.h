
#ifndef CDX_Types_H_
#define CDX_Types_H_

typedef unsigned char CDX_U8;
typedef signed char CDX_S8;
typedef unsigned short CDX_U16;
typedef short CDX_S16;
typedef unsigned int CDX_U32;
typedef int CDX_S32;
typedef unsigned long long CDX_U64;
typedef signed long long CDX_S64;
typedef char CDX_BOOL;
typedef void* CDX_PTR;

#ifndef NULL
#define NULL ((void*)0)
#endif

#define CDX_FALSE  0
#define CDX_TRUE   1

struct list_head {
	struct list_head *next, *prev;
};

#include "CDX_ErrorType.h"
#include<CDX_MemWatch.h>

#endif
/* File EOF */
