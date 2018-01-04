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

#ifndef _ABS_HEADER_H_
#define _ABS_HEADER_H_

#include <CDX_Types.h>

/* Windows WAVE File Encoding Tags */
#define WAVE_FORMAT_UNKNOWN                 0x0000  /* Unknown Format */
#define WAVE_FORMAT_PCM                     0x0001  /* PCM */
#define WAVE_FORMAT_ADPCM                   0x0002  /* Microsoft ADPCM Format */
#define WAVE_FORMAT_IEEE_FLOAT              0x0003  /* IEEE Float */
#define WAVE_FORMAT_VSELP                   0x0004  /* Compaq Computer's VSELP */
#define WAVE_FORMAT_IBM_CSVD                0x0005  /* IBM CVSD */
#define WAVE_FORMAT_ALAW                    0x0006  /* ALAW */
#define WAVE_FORMAT_MULAW                   0x0007  /* MULAW */
#define WAVE_FORMAT_OKI_ADPCM               0x0010  /* OKI ADPCM */
#define WAVE_FORMAT_DVI_ADPCM               0x0011  /* Intel's DVI ADPCM */
#define WAVE_FORMAT_MEDIASPACE_ADPCM        0x0012  /* Videologic's MediaSpace ADPCM*/
#define WAVE_FORMAT_SIERRA_ADPCM            0x0013  /* Sierra ADPCM */
#define WAVE_FORMAT_G723_ADPCM              0x0014  /* G.723 ADPCM */
#define WAVE_FORMAT_DIGISTD                 0x0015  /* DSP Solution's DIGISTD */
#define WAVE_FORMAT_DIGIFIX                 0x0016  /* DSP Solution's DIGIFIX */
#define WAVE_FORMAT_DIALOGIC_OKI_ADPCM      0x0017  /* Dialogic OKI ADPCM */
#define WAVE_FORMAT_MEDIAVISION_ADPCM       0x0018  /* MediaVision ADPCM */
#define WAVE_FORMAT_CU_CODEC                0x0019  /* HP CU */
#define WAVE_FORMAT_YAMAHA_ADPCM            0x0020  /* Yamaha ADPCM */
#define WAVE_FORMAT_SONARC                  0x0021  /* Speech Compression's Sonarc */
#define WAVE_FORMAT_TRUESPEECH              0x0022  /* DSP Group's True Speech */
#define WAVE_FORMAT_ECHOSC1                 0x0023  /* Echo Speech's EchoSC1 */
#define WAVE_FORMAT_AUDIOFILE_AF36          0x0024  /* Audiofile AF36 */
#define WAVE_FORMAT_APTX                    0x0025  /* APTX */
#define WAVE_FORMAT_AUDIOFILE_AF10          0x0026  /* AudioFile AF10 */
#define WAVE_FORMAT_PROSODY_1612            0x0027  /* Prosody 1612 */
#define WAVE_FORMAT_LRC                     0x0028  /* LRC */
#define WAVE_FORMAT_AC2                     0x0030  /* Dolby AC2 */
#define WAVE_FORMAT_GSM610                  0x0031  /* GSM610 */
#define WAVE_FORMAT_MSNAUDIO                0x0032  /* MSNAudio */
#define WAVE_FORMAT_ANTEX_ADPCME            0x0033  /* Antex ADPCME */
#define WAVE_FORMAT_CONTROL_RES_VQLPC       0x0034  /* Control Res VQLPC */
#define WAVE_FORMAT_DIGIREAL                0x0035  /* Digireal */
#define WAVE_FORMAT_DIGIADPCM               0x0036  /* DigiADPCM */
#define WAVE_FORMAT_CONTROL_RES_CR10        0x0037  /* Control Res CR10 */
#define WAVE_FORMAT_VBXADPCM                0x0038  /* NMS VBXADPCM */
#define WAVE_FORMAT_ROLAND_RDAC             0x0039  /* Roland RDAC */
#define WAVE_FORMAT_ECHOSC3                 0x003A  /* EchoSC3 */
#define WAVE_FORMAT_ROCKWELL_ADPCM          0x003B  /* Rockwell ADPCM */
#define WAVE_FORMAT_ROCKWELL_DIGITALK       0x003C  /* Rockwell Digit LK */
#define WAVE_FORMAT_XEBEC                   0x003D  /* Xebec */
#define WAVE_FORMAT_G721_ADPCM              0x0040  /* Antex Electronics G.721 */
#define WAVE_FORMAT_G728_CELP               0x0041  /* G.728 CELP */
#define WAVE_FORMAT_MSG723                  0x0042  /* MSG723 */
#define WAVE_FORMAT_MPEG                    0x0050  /* MPEG Layer 1,2 */
#define WAVE_FORMAT_RT24                    0x0051  /* RT24 */
#define WAVE_FORMAT_PAC                     0x0051  /* PAC */
#define WAVE_FORMAT_MPEGLAYER3              0x0055  /* MPEG Layer 3 */
#define WAVE_FORMAT_CIRRUS                  0x0059  /* Cirrus */
#define WAVE_FORMAT_ESPCM                   0x0061  /* ESPCM */
#define WAVE_FORMAT_VOXWARE                 0x0062  /* Voxware (obsolete) */
#define WAVE_FORMAT_CANOPUS_ATRAC           0x0063  /* Canopus Atrac */
#define WAVE_FORMAT_G726_ADPCM              0x0064  /* G.726 ADPCM */
#define WAVE_FORMAT_G722_ADPCM              0x0065  /* G.722 ADPCM */
#define WAVE_FORMAT_DSAT                    0x0066  /* DSAT */
#define WAVE_FORMAT_DSAT_DISPLAY            0x0067  /* DSAT Display */
#define WAVE_FORMAT_VOXWARE_BYTE_ALIGNED    0x0069  /* Voxware Byte Aligned (obsolete) */
#define WAVE_FORMAT_VOXWARE_AC8             0x0070  /* Voxware AC8 (obsolete) */
#define WAVE_FORMAT_VOXWARE_AC10            0x0071  /* Voxware AC10 (obsolete) */
#define WAVE_FORMAT_VOXWARE_AC16            0x0072  /* Voxware AC16 (obsolete) */
#define WAVE_FORMAT_VOXWARE_AC20            0x0073  /* Voxware AC20 (obsolete) */
#define WAVE_FORMAT_VOXWARE_RT24            0x0074  /* Voxware MetaVoice (obsolete) */
#define WAVE_FORMAT_VOXWARE_RT29            0x0075  /* Voxware MetaSound (obsolete) */
#define WAVE_FORMAT_VOXWARE_RT29HW          0x0076  /* Voxware RT29HW (obsolete) */
#define WAVE_FORMAT_VOXWARE_VR12            0x0077  /* Voxware VR12 (obsolete) */
#define WAVE_FORMAT_VOXWARE_VR18            0x0078  /* Voxware VR18 (obsolete) */
#define WAVE_FORMAT_VOXWARE_TQ40            0x0079  /* Voxware TQ40 (obsolete) */
#define WAVE_FORMAT_SOFTSOUND               0x0080  /* Softsound */
#define WAVE_FORMAT_VOXWARE_TQ60            0x0081  /* Voxware TQ60 (obsolete) */
#define WAVE_FORMAT_MSRT24                  0x0082  /* MSRT24 */
#define WAVE_FORMAT_G729A                   0x0083  /* G.729A */
#define WAVE_FORMAT_MVI_MV12                0x0084  /* MVI MV12 */
#define WAVE_FORMAT_DF_G726                 0x0085  /* DF G.726 */
#define WAVE_FORMAT_DF_GSM610               0x0086  /* DF GSM610 */
#define WAVE_FORMAT_ISIAUDIO                0x0088  /* ISIAudio */
#define WAVE_FORMAT_ONLIVE                  0x0089  /* Onlive */
#define WAVE_FORMAT_SBC24                   0x0091  /* SBC24 */
#define WAVE_FORMAT_DOLBY_AC3_SPDIF         0x0092  /* Dolby AC3 SPDIF */
#define WAVE_FORMAT_ZYXEL_ADPCM             0x0097  /* ZyXEL ADPCM */
#define WAVE_FORMAT_PHILIPS_LPCBB           0x0098  /* Philips LPCBB */
#define WAVE_FORMAT_PACKED                  0x0099  /* Packed */
#define WAVE_FORMAT_RHETOREX_ADPCM          0x0100  /* Rhetorex ADPCM */
#define WAVE_FORMAT_IRAT                    0x0101  /* BeCubed Software's IRAT */
#define WAVE_FORMAT_VIVO_G723               0x0111  /* Vivo G.723 */
#define WAVE_FORMAT_VIVO_SIREN              0x0112  /* Vivo Siren */
#define WAVE_FORMAT_DIGITAL_G723            0x0123  /* Digital G.723 */
#define WAVE_FORMAT_CREATIVE_ADPCM          0x0200  /* Creative ADPCM */
#define WAVE_FORMAT_CREATIVE_FASTSPEECH8    0x0202  /* Creative FastSpeech8 */
#define WAVE_FORMAT_CREATIVE_FASTSPEECH10   0x0203  /* Creative FastSpeech10 */
#define WAVE_FORMAT_QUARTERDECK             0x0220  /* Quarterdeck */
#define WAVE_FORMAT_FM_TOWNS_SND            0x0300  /* FM Towns Snd */
#define WAVE_FORMAT_BTV_DIGITAL             0x0400  /* BTV Digital */
#define WAVE_FORMAT_VME_VMPCM               0x0680  /* VME VMPCM */
#define WAVE_FORMAT_OLIGSM                  0x1000  /* OLIGSM */
#define WAVE_FORMAT_OLIADPCM                0x1001  /* OLIADPCM */
#define WAVE_FORMAT_OLICELP                 0x1002  /* OLICELP */
#define WAVE_FORMAT_OLISBC                  0x1003  /* OLISBC */
#define WAVE_FORMAT_OLIOPR                  0x1004  /* OLIOPR */
#define WAVE_FORMAT_LH_CODEC                0x1100  /* LH Codec */
#define WAVE_FORMAT_NORRIS                  0x1400  /* Norris */
#define WAVE_FORMAT_SOUNDSPACE_MUSICOMPRESS 0x1500  /* Soundspace Music Compression */
#define WAVE_FORMAT_DVM                     0x2000  /* DVM */
#define WAVE_FORMAT_EXTENSIBLE              0xFFFE  /* SubFormat */
#define WAVE_FORMAT_DEVELOPMENT             0xFFFF  /* Development */

//user define adpcm codec type from video file
#define ADPCM_CODEC_ID_IMA_QT               0xE000
#define ADPCM_CODEC_ID_IMA_WAV              0xE001  /* from avi file format */
#define ADPCM_CODEC_ID_IMA_DK3              0xE002
#define ADPCM_CODEC_ID_IMA_DK4              0xE003
#define ADPCM_CODEC_ID_IMA_WS               0xE004
#define ADPCM_CODEC_ID_IMA_SMJPEG           0xE005
#define ADPCM_CODEC_ID_MS                   0xE006
#define ADPCM_CODEC_ID_4XM                  0xE007
#define ADPCM_CODEC_ID_XA                   0xE008
#define ADPCM_CODEC_ID_ADX                  0xE009
#define ADPCM_CODEC_ID_EA                   0xE00A
#define ADPCM_CODEC_ID_G726                 0xE00B
#define ADPCM_CODEC_ID_CT                   0xE00C
#define ADPCM_CODEC_ID_SWF                  0xE00D  /* from flv/swf file format */
#define ADPCM_CODEC_ID_YAMAHA               0xE00E
#define ADPCM_CODEC_ID_SBPRO_4              0xE00F
#define ADPCM_CODEC_ID_SBPRO_3              0xE010
#define ADPCM_CODEC_ID_SBPRO_2              0xE011


//define wave header, for decode pcm data
typedef struct __WAVE_HEADER
{
    CDX_U32       uRiffFcc;       // four character code, "RIFF"
    CDX_U32       uFileLen;       // file total length, don't care it

    CDX_U32       uWaveFcc;       // four character code, "WAVE"

    CDX_U32       uFmtFcc;        // four character code, "fmt "
    CDX_U32       uFmtDataLen;    // Length of the fmt data (=16)
    CDX_U16       uWavEncodeTag;  // WAVE File Encoding Tag
    CDX_U16       uChannels;      // Channels: 1 = mono, 2 = stereo
    CDX_U32       uSampleRate;    // Samples per second: e.g., 44100
    CDX_U32       uBytesPerSec;   // sample rate * block align
    CDX_U16       uBlockAlign;    // channels * bits/sample / 8
    CDX_U16       uBitsPerSample; // 8 or 16

    CDX_U32       uDataFcc;       // four character code "data"
    CDX_U32       uSampDataSize;  // Sample data size(n)

} __wave_header_t;


//define adpcm header
typedef struct __WAVE_HEADER_ADPCM
{
    CDX_U32       uRiffFcc;       // four character code, "RIFF"
    CDX_U32       uFileLen;       // file total length,  =totalfilelength-8

    CDX_U32       uWaveFcc;       // four character code, "WAVE"

    CDX_U32       uFmtFcc;        // four character code, "fmt "
    CDX_U32       uFmtDataLen;    // Length of the fmt data min(adpcm=0x14,pcm =0x10)
    CDX_U16       uWavEncodeTag;  // WAVE File Encoding Tag adpcm =0x11,pcm =0x01
    CDX_U16       uChannels;      // Channels: 1 = mono, 2 = stereo
    CDX_U32       uSampleRate;    // Samples per second: e.g., 44100
    CDX_U32       uBytesPerSec;   // pcm = sample rate * block align,adpcm= sample rate* uBlockAlign/nSamplesperBlock;
    CDX_U16       uBlockAlign;    // pcm=channels * bits/sample / 8,adpcm =block lenth;
    CDX_U16       uBitsPerSample; // pcm=8 or 16 or 24,adpcm =4,now

    CDX_U16       uWordsbSize;    // for only adpcm =0x0002;
    CDX_U16       uNSamplesPerBlock;//for only adpcm = ((uBlockAlign-4*uChannels)*2+channels)/channels

    CDX_U32       uDataFcc;       // four character code "data"
    CDX_U32       uSampDataSize;  // Sample data size(n)

} __wave_header_adpcm_t;


typedef struct __AAC_HEADER_INFO
{
    CDX_U32       ulBsHdrFlag;        // bitstream header flag, = 0xffffffff, for mask bitstream header
    CDX_U32       ulSampleRate;       // sample rate
    CDX_U32       ulActualRate;       // sample rate
    CDX_U16       usBitsPerSample;    // bits per sample
    CDX_U16       usNumChannels;      // channel count
    CDX_U16       usAudioQuality;     // audio quality, don't care(default 100)
    CDX_U16       usFlavorIndex;      // flavor index, don't care(default 0)
    CDX_U32       ulBitsPerFrame;     // bits per frame,don't care(default 0x100)
    CDX_U32       ulGranularity;      // default=0x100;ulFramesPerBlock*ulFrameSize传过来block的大小，一个block包含1个或者多个frames
    CDX_U32       ulOpaqueDataSize;   // size of opaque data
                                    // just put opaque data after 'ulOpaqueDataSize'
} __aac_header_info_t;


//define aac audio sub type
typedef enum __AAC_BITSTREAM_SUB
{
    AAC_SUB_TYPE_MAIN       = 1,
    AAC_SUB_TYPE_LC         = 2,
    AAC_SUB_TYPE_SSR        = 3,
    AAC_SUB_TYPE_LTP        = 4,
    AAC_SUB_TYPE_SBR        = 5,
    AAC_SUB_TYPE_SCALABLE   = 6,
    AAC_SUB_TYPE_TWINVQ     = 7,
    AAC_SUB_TYPE_CELP       = 8,
    AAC_SUB_TYPE_HVXC       = 9,
    AAC_SUB_TYPE_TTSI       = 12,
    AAC_SUB_TYPE_GENMIDI    = 15,
    AAC_SUB_TYPE_AUDIOFX    = 16,

    AAC_SUB_TYPE_

} __aac_bitstream_sub_t;


typedef enum __AAC_PROFILE_TYPE
{
    AAC_PROFILE_MP  = 0,
    AAC_PROFILE_LC  = 1,
    AAC_PROFILE_SSR = 2,

    AAC_PROFILE_

} __aac_profile_type_t;




// __AAC_OpaqueData structure define
// Note : we can't use this struct, because the big ending,
// this structure just mark that the items need by decode lib
// | 7 6 5 4 3 2 1 0 | 7 6 5 4 3 2 1 0 | 7 6 5 4 3 2 1 0 |7-0 | 7-0 | 7 6 5 4 3     2     1 0  |
// |---uAACFrmType---|--AACOBJ--|--SFIdx--|---------sample rate--------|--chn--|-frmlen-|-flag-|

typedef struct __AAC_OpaqueData
{
    CDX_U64       uAACFrmType : 8;        // 1:ADTS Frame; 2:MP4 Audio Specific Config Data
	CDX_U64       uAACObjType : 5;        // __aac_bitstream_sub_t
    CDX_U64       uSFIdxType  : 4;        // sample frequence type, 0:96000, 0xf:follow 24 bits
    CDX_U64       uSampleRate : 24;       // sample frequence, just exist when uSFIdxType = 0xf
    CDX_U64       uChannels   : 4;        // audio channel count
    CDX_U64       uFrmLength  : 1;        // 0:960, 1:1024; default set to 1
    CDX_U64       Flag0       : 1;        // flag reserved 0,
    CDX_U64       Flag1       : 1;        // flag reserved 1,
    CDX_U64       Reserved    : 16;       // reserved, no use

} __aac_opaquedata_t;


typedef enum __AMR_FORMAT_SUB_TYPE
{
    AMR_FORMAT_NONE = 0,                // undefine amr format
    AMR_FORMAT_NARROWBAND,              // narrow band amr format
    AMR_FORMAT_WIDEBAND,                // wide band amr format
    AMR_FORMAT_

} __amr_format_sub_type_t;


typedef struct __WMA_HEADER_INFO
{
    CDX_S16       FmtTag;                 //wma format tag, should be 0x160/0x162/0x163
    CDX_S16       Channel;                //audio channel count
    CDX_S32       SampleRate;             //audio sample rate
    CDX_S32       AvgBytePerSec;          //average bytes per seconds
    CDX_S16       BlockAlign;             //block align
    CDX_S16       reserved0;              //reserved
    CDX_U8        reserved1[8];           //reserved
    CDX_S16       EncodeOpt;              //encode options
    CDX_S16       reserved2;              //reserved
    CDX_S16       reserved3;              //reserved

} __wma_header_info_t;


#endif /* _ABS_HEADER_H_ */
