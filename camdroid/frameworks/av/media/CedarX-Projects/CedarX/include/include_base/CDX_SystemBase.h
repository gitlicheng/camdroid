/*******************************************************************************
--                                                                            --
--                    CedarX Multimedia Framework                             --
--                                                                            --
--          the Multimedia Framework for Linux/Android System                 --
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                         Softwinner Products.                               --
--                                                                            --
--                   (C) COPYRIGHT 2011 SOFTWINNER PRODUCTS                   --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
*******************************************************************************/

#ifndef CDX_SYSTEMBASE_H
#define CDX_SYSTEMBASE_H

#include <CDX_Types.h>

#ifdef  __cplusplus
extern "C" {
#endif

CDX_S64 CDX_GetNowUs();
CDX_S64 CDX_GetSysTimeUsMonotonic();
void hexdump(void * buf, CDX_U32  size);
typedef struct
{
    unsigned int totalKB;
	unsigned int freeKB;
}IonMemInfo;
int getIonMemInfo(IonMemInfo *pIonMemInfo);

#ifdef  __cplusplus
}
#endif
#endif
