/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _SUBTITLERENDER_H_
#define _SUBTITLERENDER_H_

namespace android {

//ref to [sub_reader.h], SUB_TEXT_SIZE(256) SUB_DVDSUB_SIZE(1024*1024) SUB_PGS_SIZE(1*1024*1024)
#define SUB_TEXT_BUFSIZE   (1024)
#define SUB_FRAME_BUFSIZE   (1*1024*1024)
//enum SUB_MODE
//{
//    SUB_MODE_TEXT   = 0,
//    SUB_MODE_BITMAP = 1,
//    SUB_MODE_
//};
class CedarXTimedText : public RefBase
{
    public:
        CedarXTimedText();
	    ~CedarXTimedText();
		int                 setSubShowFlag(int subShowFlag);
        int                 getSubShowFlag();
        int  				setCharset(int Charset);
        int  				getCharset();
        int                 setSubInf(void *pInf);
        unsigned char subMode;            //0: SUB_MODE_TEXT; 1: SUB_MODE_BITMAP
        int  startx;             // the invalid value is -1
        int  starty;             // the invalid value is -1
        int  endx;               // the invalid value is -1
        int  endy;               // the invalid value is -1
        int  subDispPos;         // the disply position of the subItem, SUB_DISPPOS_BOT_LEFT
        int  nScreenWidth;          //for ssa, sub position need a whole screen size as a reference.
        int  nScreenHeight;
        unsigned int  startTime;          // the start display time of the subItem
        unsigned int  endTime;            // the end display time of the subItem, 0:unknown endTime, so display until next subtitle item(maybe null item).
        unsigned char*  subTextBuf;         // the data buffer of the text subtitle
        unsigned char*  subBitmapBuf;       // the data buffer of the bitmap subtitle
        unsigned int  subTextLen;         // the length of the text subtitle
        unsigned int  subPicWidth;        // the width of the bitmap subtitle
        unsigned int  subPicHeight;       // the height of the bitmap subtitle
        unsigned int  mSubPicPixelFormat;  //PIXEL_FORMAT_RGBA_8888
        //OMX_U8   alignment;          // the alignment of the subtitle
        char   encodingType;       // the encoding tyle of the text subtitle, SUB_ENCODING_UTF8
//    void*    nextSubItem;        // the information of the next subItem 
//    OMX_U8   dispBufIdx;         // the diplay index of the sub 
//
//    OMX_U32  subScaleWidth;      // the scaler width of the bitmap subtitle
//    OMX_U32  subScaleHeight;     // the scaler height of the bitmap subtitle
//    
        unsigned char   subHasFontInfFlag;  //indicate font inf valid.
        unsigned char   fontStyle;          // the font style of the text subtitle
        char   fontSize;           // the font size of the text subtile
        unsigned int  primaryColor;     // ARGB, int32 high->low.
        unsigned int  secondaryColor;   // ARGB, int32 high->low. the second color of main color.
  //OMX_U32  outlineColour;
  //OMX_U32  backColour;
        unsigned int  subStyle;          // the bold flag,the italic flag, or the underline flag, SUB_STYLE_BOLD
//    OMX_S32  subEffectFlag;
//    OMX_U32  effectStartxPos;
//    OMX_U32  effectEndxPos;
//    OMX_U32  effectStartyPos;
//    OMX_U32  effectEndyPos;
//    OMX_U32  effectTimeDelay;
//    sub_karako_effect_inf *subKarakoEffectInf;
    private:
        int                 convertUniCode(void *pInf);
		int                 mSubShowFlag;   //SUB_SHOW_INVALID
		int  				mCharset;   //SUB_CHARSET_GBK. record app's indication to subtitle text charset. If subtitle decoder don't know text charset, then use this as charset.
       //sub_item_inf *      mDispSubInfo;
};


}  // namespace android

#endif  /* _SUBTITLERENDER_H_ */


