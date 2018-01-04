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

//#define ALOG_NDEBUG 0
#define ALOG_TAG "MediaPlayerServiceMediaFormatProbe"
#include <utils/Log.h>

#include "SimpleMediaFormatProbe.h"

namespace android {

#define AVPROBE_SCORE_MAX 100


#define AV_RB32(x)  ((((const unsigned char*)(x))[0] << 24) | \
    (((const unsigned char*)(x))[1] << 16) | \
    (((const unsigned char*)(x))[2] <<  8) | \
                      ((const unsigned char*)(x))[3])

#define AV_RL32(x) ((((const unsigned char*)(x))[3] << 24) | \
    (((const unsigned char*)(x))[2] << 16) | \
    (((const unsigned char*)(x))[1] <<  8) | \
                     ((const unsigned char*)(x))[0])

#define FFMAX(a,b) ((a) > (b) ? (a) : (b))

#define MKTAG(a,b,c,d) (a | (b << 8) | (c << 16) | (d << 24))

//---------------------------- Audio stream format probe -----------------------------------
#define ID3v2_HEADER_SIZE 10
#define MPA_STEREO  0
#define MPA_JSTEREO 1
#define MPA_DUAL    2
#define MPA_MONO    3

const uint16_t ff_mpa_freq_tab[3] = { 44100, 48000, 32000 };

const uint16_t ff_mpa_bitrate_tab[2][3][15] = {
    { {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448 },
      {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384 },
      {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 } },
    { {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
      {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
      {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
    }
};

#define MPA_DECODE_HEADER \
    int frame_size; \
    int error_protection; \
    int layer; \
    int sample_rate; \
    int sample_rate_index; /* between 0 and 8 */ \
    int bit_rate; \
    int nb_channels; \
    int mode; \
    int mode_ext; \
    int lsf;

typedef struct MPADecodeHeader {
  MPA_DECODE_HEADER
} MPADecodeHeader;


static int ff_id3v2_match(const uint8_t *buf)
{
    return  buf[0]         ==  'I' &&
            buf[1]         ==  'D' &&
            buf[2]         ==  '3' &&
            buf[3]         != 0xff &&
            buf[4]         != 0xff &&
           (buf[6] & 0x80) ==    0 &&
           (buf[7] & 0x80) ==    0 &&
           (buf[8] & 0x80) ==    0 &&
           (buf[9] & 0x80) ==    0;
}

static int ff_id3v2_tag_len(const uint8_t * buf)
{
    int len = ((buf[6] & 0x7f) << 21) +
              ((buf[7] & 0x7f) << 14) +
              ((buf[8] & 0x7f) << 7) +
               (buf[9] & 0x7f) +
              ID3v2_HEADER_SIZE;
    if (buf[5] & 0x10)
        len += ID3v2_HEADER_SIZE;
    return len;
}

/* fast header check for resync */
static inline int ff_mpa_check_header(uint32_t header){
    /* header */
    if ((header & 0xffe00000) != 0xffe00000)
        return -1;
    /* layer check */
    if ((header & (3<<17)) == 0)
        return -1;
    /* bit rate */
    if ((header & (0xf<<12)) == 0xf<<12)
        return -1;
    /* frequency */
    if ((header & (3<<10)) == 3<<10)
        return -1;
    return 0;
}

static int ff_mpegaudio_decode_header(MPADecodeHeader *s, uint32_t header)
{
    int sample_rate, frame_size, mpeg25, padding;
    int sample_rate_index, bitrate_index;
    if (header & (1<<20)) {
        s->lsf = (header & (1<<19)) ? 0 : 1;
        mpeg25 = 0;
    } else {
        s->lsf = 1;
        mpeg25 = 1;
    }

    s->layer = 4 - ((header >> 17) & 3);
    /* extract frequency */
    sample_rate_index = (header >> 10) & 3;
    sample_rate = ff_mpa_freq_tab[sample_rate_index] >> (s->lsf + mpeg25);
    sample_rate_index += 3 * (s->lsf + mpeg25);
    s->sample_rate_index = sample_rate_index;
    s->error_protection = ((header >> 16) & 1) ^ 1;
    s->sample_rate = sample_rate;

    bitrate_index = (header >> 12) & 0xf;
    padding = (header >> 9) & 1;
    //extension = (header >> 8) & 1;
    s->mode = (header >> 6) & 3;
    s->mode_ext = (header >> 4) & 3;
    //copyright = (header >> 3) & 1;
    //original = (header >> 2) & 1;
    //emphasis = header & 3;

    if (s->mode == MPA_MONO)
        s->nb_channels = 1;
    else
        s->nb_channels = 2;

    if (bitrate_index != 0) {
        frame_size = ff_mpa_bitrate_tab[s->lsf][s->layer - 1][bitrate_index];
        s->bit_rate = frame_size * 1000;
        switch(s->layer) {
        case 1:
            frame_size = (frame_size * 12000) / sample_rate;
            frame_size = (frame_size + padding) * 4;
            break;
        case 2:
            frame_size = (frame_size * 144000) / sample_rate;
            frame_size += padding;
            break;
        default:
        case 3:
            frame_size = (frame_size * 144000) / (sample_rate << s->lsf);
            frame_size += padding;
            break;
        }
        s->frame_size = frame_size;
    } else {
        /* if no frame size computed, signal it */
        return 1;
    }

    return 0;
}

/* useful helper to get mpeg audio stream infos. Return -1 if error in
   header, otherwise the coded frame size in bytes */
static int ff_mpa_decode_header(uint32_t head, int *sample_rate, int *channels, int *frame_size, int *bit_rate,int *layer)
{
    MPADecodeHeader s1, *s = &s1;

    if (ff_mpa_check_header(head) != 0)
        return -1;

    if (ff_mpegaudio_decode_header(s, head) != 0) {
        return -1;
    }

    switch(s->layer) {
    case 1:
        //avctx->codec_id = CODEC_ID_MP1;
        *frame_size = 384;
        break;
    case 2:
        //avctx->codec_id = CODEC_ID_MP2;
        *frame_size = 1152;
        break;
    default:
    case 3:
        //avctx->codec_id = CODEC_ID_MP3;
        if (s->lsf)
            *frame_size = 576;
        else
            *frame_size = 1152;
        break;
    }

    *sample_rate = s->sample_rate;
    *channels = s->nb_channels;
    *bit_rate = s->bit_rate;
    *layer = s->layer;
    //avctx->sub_id = s->layer;
    return s->frame_size;
}

static int mp3_probe(media_probe_data_t *p)
{
    int max_frames, first_frames = 0;
    int fsize, frames, sample_rate;
    uint32_t header;
    uint8_t *buf, *buf0, *buf2, *end;
    int layer = 0;

    buf0 = p->buf;
    if(ff_id3v2_match(buf0)) {
        buf0 += ff_id3v2_tag_len(buf0);
    }
    end = p->buf + p->buf_size - sizeof(uint32_t);
    while(buf0 < end && !*buf0)
        buf0++;

    max_frames = 0;
    buf = buf0;

    for(; buf < end; buf= buf2+1) {
        buf2 = buf;

        for(frames = 0; buf2 < end; frames++) {
            header = AV_RB32(buf2);
            fsize = ff_mpa_decode_header(header, &sample_rate, &sample_rate, &sample_rate, &sample_rate,&layer);
            if(fsize < 0)
                return 0;//break;//for error check aac->mp3;
            buf2 += fsize;
        }
        max_frames = FFMAX(max_frames, frames);
        if(buf == buf0)
            first_frames= frames;
    }

    //ALOGV("first_frames:%d max_frames:%d",first_frames,max_frames);

    // keep this in sync with ac3 probe, both need to avoid
    // issues with MPEG-files!
    if((layer==1)||(layer==2))
        return AVPROBE_SCORE_MAX -3;
    if   (first_frames>=4) return AVPROBE_SCORE_MAX/2+1;
    else if(max_frames>500)return AVPROBE_SCORE_MAX/2;
    else if(max_frames>=2) return AVPROBE_SCORE_MAX/4;
    else if(buf0!=p->buf)  return AVPROBE_SCORE_MAX/4-1;
    //else if(max_frames>=1) return 1;
    else                   return 0;
//mpegps_mp3_unrecognized_format.mpg has max_frames=3
}

static int ogg_probe(media_probe_data_t *p)
{
    if (p->buf[0] == 'O' && p->buf[1] == 'g' &&
        p->buf[2] == 'g' && p->buf[3] == 'S' &&
        p->buf[4] == 0x0 && p->buf[5] <= 0x7 )
        return AVPROBE_SCORE_MAX;
    else
        return 0;
}

static int wav_probe(media_probe_data_t *p)
{
    /* check file header */
    if (p->buf_size <= 32)
        return 0;
    if (!memcmp(p->buf + 8, "WAVE", 4)) {
        if (!memcmp(p->buf, "RIFF", 4)) {
            /*
              Since ACT demuxer has standard WAV header at top of it's own,
              returning score is decreased to avoid probe conflict
              between ACT and WAV.
            */
        	int i, buflen = p->buf_size;
        	unsigned char *ptr = p->buf;

        	for(i=0x10; i<(buflen > 512 ? 512 : buflen); i++)
        	{
        	    if((ptr[i] == 0x64)&&(ptr[i+1] == 0x61)&&(ptr[i+2] == 0x74)&&(ptr[i+3] == 0x61))
        			break;
        	}
        	i+=8;
        	if(i<buflen)
        	{
        		for(;i<buflen;i++)
        		{
        			if(ptr[i]!=0)
        				break;
        		}
        	}
        	if(i<buflen-5)
        	{
        		if ((ptr[i + 0] == 0xff && ptr[i + 1] == 0x1f &&
					  ptr[i + 2] == 0x00 && ptr[i + 3] == 0xe8 &&
					  (ptr[i + 4] & 0xf0) == 0xf0 && ptr[i + 5] == 0x07)
				   ||(ptr[i + 0] == 0x1f && ptr[i + 1] == 0xff &&
					  ptr[i + 2] == 0xe8 && ptr[i + 3] == 0x00 &&
					  ptr[i + 4] == 0x07 &&(ptr[i + 5] & 0xf0) == 0xf0)
				   ||(ptr[i + 0] == 0xfe && ptr[i + 1] == 0x7f &&
					  ptr[i + 2] == 0x01 && ptr[i + 3] == 0x80)
				   ||(ptr[i + 0] == 0x7f && ptr[i + 1] == 0xfe &&
					  ptr[i + 2] == 0x80 && ptr[i + 3] == 0x01))
        		{
        			return AVPROBE_SCORE_MAX - 2; //DTS
        		}
        	}
            //add for adpcm call  CedarA
            for(i=12; i<(buflen > 512 ? 512 : buflen); i++)
        	{
        	    if((ptr[i] == 0x66)&&(ptr[i+1] == 0x6D)&&(ptr[i+2] == 0x74)&&(ptr[i+3] == 0x20))//fmt
        			break;
        	}
            if(i<buflen-10)
            {
                unsigned int tempWavTag = ptr[i+8]|((unsigned int)(ptr[i+9])<<8);
                ALOGV("WAV FOR tag =%d\n",tempWavTag);
                if(tempWavTag == 0x55)/* MPEG Layer 3 */
                {
	                return MEDIA_FORMAT_MP3;
                }
                else if(tempWavTag != 1)//if((tempWavTag != 1)&&(tempWavTag != 6)&&(tempWavTag != 7))
                {
                    return AVPROBE_SCORE_MAX -3;
                }
            }
            

            return AVPROBE_SCORE_MAX - 1;
        }
        else if (!memcmp(p->buf,      "RF64", 4) &&
                 !memcmp(p->buf + 12, "ds64", 4))
            return AVPROBE_SCORE_MAX;
    }

    return 0;
}

static const char AMR_header [] = "#!AMR\n";

static int amr_probe(media_probe_data_t *p)
{
    //Only check for "#!AMR" which could be amr-wb, amr-nb.
    //This will also trigger multichannel files: "#!AMR_MC1.0\n" and
    //"#!AMR-WB_MC1.0\n" (not supported)

    if(memcmp(p->buf,AMR_header,5)==0)
        return AVPROBE_SCORE_MAX;
    else
        return 0;
}

static int ape_probe(media_probe_data_t * p)
{
    if (p->buf[0] == 'M' && p->buf[1] == 'A' && p->buf[2] == 'C' && p->buf[3] == ' ')
        return AVPROBE_SCORE_MAX;

    return 0;
}

static int flac_probe(media_probe_data_t *p)
{
    unsigned char *bufptr = p->buf;
    unsigned char *end    = p->buf + p->buf_size;

    if (p->buf[0] == 'f' && p->buf[1] == 'L' && p->buf[2] == 'a' && p->buf[3] == 'C')
       return AVPROBE_SCORE_MAX;

    if(ff_id3v2_match(bufptr))
        bufptr += ff_id3v2_tag_len(bufptr);

    if(bufptr > end-4 || memcmp(bufptr, "fLaC", 4)) return 0;
    else                                            return AVPROBE_SCORE_MAX/2;
}

#if 0
static int wma_probe(media_probe_data_t *p)
{
	unsigned char asf_header[] = { 0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11,
								   0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C };

	if(memcmp(p->buf, asf_header, 16) == 0)
	{
		return AVPROBE_SCORE_MAX;
	}

	return 0;
}
#endif
typedef unsigned char GUID[16];
static const GUID stream_bitrate_guid = { /* (http://get.to/sdp) */
    0xce, 0x75, 0xf8, 0x7b, 0x8d, 0x46, 0xd1, 0x11, 0x8d, 0x82, 0x00, 0x60, 0x97, 0xc9, 0xa2, 0xb2
};

static const GUID asf_header = {
    0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11, 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C
};

static const GUID file_header = {
    0xA1, 0xDC, 0xAB, 0x8C, 0x47, 0xA9, 0xCF, 0x11, 0x8E, 0xE4, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65
};

static const GUID stream_header = {
    0x91, 0x07, 0xDC, 0xB7, 0xB7, 0xA9, 0xCF, 0x11, 0x8E, 0xE6, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65
};

static const GUID ext_stream_header = {
    0xCB, 0xA5, 0xE6, 0x14, 0x72, 0xC6, 0x32, 0x43, 0x83, 0x99, 0xA9, 0x69, 0x52, 0x06, 0x5B, 0x5A
};

static const GUID audio_stream = {
    0x40, 0x9E, 0x69, 0xF8, 0x4D, 0x5B, 0xCF, 0x11, 0xA8, 0xFD, 0x00, 0x80, 0x5F, 0x5C, 0x44, 0x2B
};

static const GUID audio_conceal_none = {
    // 0x40, 0xa4, 0xf1, 0x49, 0x4ece, 0x11d0, 0xa3, 0xac, 0x00, 0xa0, 0xc9, 0x03, 0x48, 0xf6
    // New value lifted from avifile
    0x00, 0x57, 0xfb, 0x20, 0x55, 0x5B, 0xCF, 0x11, 0xa8, 0xfd, 0x00, 0x80, 0x5f, 0x5c, 0x44, 0x2b
};

static const GUID audio_conceal_spread = {
	0x50, 0xCD, 0xC3, 0xBF, 0x8F, 0x61, 0xCF, 0x11, 0x8B, 0xB2, 0x00, 0xAA, 0x00, 0xB4, 0xE2, 0x20
};

static const GUID video_stream = {
    0xC0, 0xEF, 0x19, 0xBC, 0x4D, 0x5B, 0xCF, 0x11, 0xA8, 0xFD, 0x00, 0x80, 0x5F, 0x5C, 0x44, 0x2B
};

static const GUID video_conceal_none = {
    0x00, 0x57, 0xFB, 0x20, 0x55, 0x5B, 0xCF, 0x11, 0xA8, 0xFD, 0x00, 0x80, 0x5F, 0x5C, 0x44, 0x2B
};

static const GUID command_stream = {
    0xC0, 0xCF, 0xDA, 0x59, 0xE6, 0x59, 0xD0, 0x11, 0xA3, 0xAC, 0x00, 0xA0, 0xC9, 0x03, 0x48, 0xF6
};

static const GUID comment_header = {
    0x33, 0x26, 0xb2, 0x75, 0x8E, 0x66, 0xCF, 0x11, 0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c
};

static const GUID codec_comment_header = {
    0x40, 0x52, 0xD1, 0x86, 0x1D, 0x31, 0xD0, 0x11, 0xA3, 0xA4, 0x00, 0xA0, 0xC9, 0x03, 0x48, 0xF6
};
static const GUID codec_comment1_header = {
    0x41, 0x52, 0xd1, 0x86, 0x1D, 0x31, 0xD0, 0x11, 0xa3, 0xa4, 0x00, 0xa0, 0xc9, 0x03, 0x48, 0xf6
};

static const GUID data_header = {
    0x36, 0x26, 0xb2, 0x75, 0x8E, 0x66, 0xCF, 0x11, 0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c
};

static const GUID head1_guid = {
    0xb5, 0x03, 0xbf, 0x5f, 0x2E, 0xA9, 0xCF, 0x11, 0x8e, 0xe3, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65
};

static const GUID head2_guid = {
    0x11, 0xd2, 0xd3, 0xab, 0xBA, 0xA9, 0xCF, 0x11, 0x8e, 0xe6, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65
};

static const GUID extended_content_header = {
	0x40, 0xA4, 0xD0, 0xD2, 0x07, 0xE3, 0xD2, 0x11, 0x97, 0xF0, 0x00, 0xA0, 0xC9, 0x5E, 0xA8, 0x50
};

static const GUID simple_index_header = {
	0x90, 0x08, 0x00, 0x33, 0xB1, 0xE5, 0xCF, 0x11, 0x89, 0xF4, 0x00, 0xA0, 0xC9, 0x03, 0x49, 0xCB
};

static const GUID ext_stream_embed_stream_header = {
	0xe2, 0x65, 0xfb, 0x3a, 0xEF, 0x47, 0xF2, 0x40, 0xac, 0x2c, 0x70, 0xa9, 0x0d, 0x71, 0xd3, 0x43
};

static const GUID ext_stream_audio_stream = {
	0x9d, 0x8c, 0x17, 0x31, 0xE1, 0x03, 0x28, 0x45, 0xb5, 0x82, 0x3d, 0xf9, 0xdb, 0x22, 0xf5, 0x03
};

static const GUID metadata_header = {
	0xea, 0xcb, 0xf8, 0xc5, 0xaf, 0x5b, 0x77, 0x48, 0x84, 0x67, 0xaa, 0x8c, 0x44, 0xfa, 0x4c, 0xca
};

static int get_buffer(int fd,unsigned char *data, unsigned int len)
{
    unsigned int r_size;
    r_size = read(fd, data, len);
    if(r_size != len)
    {
        ALOGW("fd read error! r_size=%d, len=%d", r_size, len);
        memset(data, 0, len);
        return -1;
    }
    return 0;
}
static int get_byte(int fd)
{
    unsigned char t = 0;
    int t1;
    int ret = read(fd, &t, 1);
    if(ret != 1)
    {
    	ALOGI("read failed, may be end of stream.");
    	return -1;
    }
    t1 = (int)t;
    return t1;
}
static unsigned int get_le16(int fd)
{
    unsigned int val;
    val = get_byte(fd);
    val |= get_byte(fd) << 8;
    return val;
}

static unsigned int get_le24(int fd)
{
    unsigned int val;
    val = get_le16(fd);
    val |= get_byte(fd) << 16;
    return val;
}

static unsigned int get_le32(int fd)
{
    unsigned int val;
    val = get_le16(fd);
    val |= get_le16(fd) << 16;
    return val;
}

static uint64_t get_le64(int fd)
{
    uint64_t val;
    val = (uint64_t)get_le32(fd);
    val |= (uint64_t)get_le32(fd) << 32;
    return val;
}
static int get_guid(int fd, GUID *g)
{
    if(0 != get_buffer(fd, *g, sizeof(*g)))
    {
        return -1;
    }
    return 0;
}
static void print_guid(const GUID *g)
{
//    CDX_S32 i;
//    for(i=0;i<16;i++)
//        printf(" 0x%02x,", (*g)[i]);
//    printf("}\n");
    ALOGD("GUID: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", 
        (*g)[0], (*g)[1], (*g)[2], (*g)[3], (*g)[4], (*g)[5], (*g)[6], (*g)[7], (*g)[8], (*g)[9], (*g)[10], (*g)[11], (*g)[12], (*g)[13], (*g)[14], (*g)[15]);
}
enum CodecType {
    CODEC_TYPE_UNKNOWN = -1,
	CODEC_TYPE_VIDEO,
	CODEC_TYPE_AUDIO,
	CODEC_TYPE_DATA,
	CODEC_TYPE_SUBTITLE,
	CODEC_TYPE_ATTACHMENT,
	CODEC_TYPE_NB
};

static int wma_probe(media_probe_data_t *p, int fd, int64_t offset)
{
    int audio_stream_num = 0;
    int video_stream_num = 0;
    int wmv_file_flag = 0;  //0:not wmv, 1:wmv, 2:wma
    GUID g;
    int64_t gsize;
    int64_t read_size = 0;
    ALOGV("(f:%s, l:%d), fd=%d, offset=%lld", __FUNCTION__, __LINE__, fd, offset);
//	if(memcmp(p->buf, asf_header, 16) == 0)
//	{
//		return AVPROBE_SCORE_MAX;
//	}

    //seek to original position
    lseek(fd, offset, SEEK_SET);

    get_guid(fd, &g);
    read_size += 16;
    if (memcmp(&g, &asf_header, sizeof(GUID)))
    {
        goto fail;
    }
    get_le64(fd);
    get_le32(fd);
    get_byte(fd);
    get_byte(fd);
    read_size += 14;
    for(;;) {
        if(read_size > 8*1024)
        {
            ALOGV("(f:%s, l:%d), file has read %lld bytes, break!", __FUNCTION__, __LINE__, read_size);
            break;
        }
        if(0!=get_guid(fd, &g))
        {
            break;
        }
        gsize = get_le64(fd);
        read_size += 24;
#if 0
        print_guid(&g);
        ALOGD("size=0x%d\n", gsize);
#endif
        if (!memcmp(&g, &data_header, sizeof(GUID))) {
            ALOGV("(f:%s, l:%d), meet ASF_Data_Object_GUID", __FUNCTION__, __LINE__);
            break;
        }
        if (gsize < 24)
        {
            ALOGV("(f:%s, l:%d), gsize[%lld] < 24, fatal error!", __FUNCTION__, __LINE__, gsize);
            goto fail;
        }
            
        if (!memcmp(&g, &file_header, sizeof(GUID))) {
            ALOGV("(f:%s, l:%d), meet ASF_File_Properties_Object_GUID, gsize=%lld, skip", __FUNCTION__, __LINE__, gsize);
            lseek(fd, gsize-24, SEEK_CUR);
            read_size += gsize-24;
        } else if (!memcmp(&g, &stream_header, sizeof(GUID))) {
            ALOGV("(f:%s, l:%d), meet ASF_Stream_Properties_Object_GUID, gsize=%lld", __FUNCTION__, __LINE__, gsize);
            enum CodecType type;

            get_guid(fd, &g);
            read_size += 16;

            if (!memcmp(&g, &audio_stream, sizeof(GUID))) {
                type = CODEC_TYPE_AUDIO;
            } else if (!memcmp(&g, &video_stream, sizeof(GUID))) {
                type = CODEC_TYPE_VIDEO;
            } else if (!memcmp(&g, &command_stream, sizeof(GUID))) {
                type = CODEC_TYPE_UNKNOWN;
            } else if (!memcmp(&g, &ext_stream_embed_stream_header, sizeof(GUID))) {
                type = CODEC_TYPE_UNKNOWN;
            } else {
                ALOGW("(f:%s, l:%d), ASF_Stream_Properties_Object, Stream Type GUID is error!", __FUNCTION__, __LINE__);
                //goto fail;
                type = CODEC_TYPE_UNKNOWN;
            }
            if(type == CODEC_TYPE_AUDIO)
            {
                audio_stream_num++;
                ALOGV("(f:%s, l:%d), ASF_Stream_Properties_Object, detect audio stream, count[%d]!", __FUNCTION__, __LINE__, audio_stream_num);
            }
            else if(type == CODEC_TYPE_VIDEO)
            {
                video_stream_num++;
                ALOGV("(f:%s, l:%d), ASF_Stream_Properties_Object, detect video stream, count[%d], this is a wmv file!", __FUNCTION__, __LINE__, video_stream_num);
                break;
            }
            lseek(fd, gsize-40, SEEK_CUR);
            read_size += gsize-40;
        }else if (!memcmp(&g, &comment_header, sizeof(GUID))) {
            ALOGV("(f:%s, l:%d), meet ASF_Content_Description_Object_GUID, gsize=%lld, skip", __FUNCTION__, __LINE__, gsize);
            lseek(fd, gsize-24, SEEK_CUR);
            read_size += gsize-24;
        } else if (!memcmp(&g, &stream_bitrate_guid, sizeof(GUID))) {
            ALOGV("(f:%s, l:%d), meet ASF_Stream_Bitrate_Properties_Object_GUID, gsize=%lld, skip", __FUNCTION__, __LINE__, gsize);
            lseek(fd, gsize-24, SEEK_CUR);            
            read_size += gsize-24;
        } else if (!memcmp(&g, &extended_content_header, sizeof(GUID))) {
            ALOGV("(f:%s, l:%d), meet ASF_Extended_Content_Description_Object_GUID, gsize=%lld, skip", __FUNCTION__, __LINE__, gsize);
            lseek(fd, gsize-24, SEEK_CUR);  
            read_size += gsize-24;
        } else if (!memcmp(&g, &metadata_header, sizeof(GUID))) {
            ALOGV("(f:%s, l:%d), meet ASF_Metadata_Object_GUID, gsize=%lld, skip", __FUNCTION__, __LINE__, gsize);
            lseek(fd, gsize-24, SEEK_CUR);
            read_size += gsize-24;
        } else if (!memcmp(&g, &ext_stream_header, sizeof(GUID))) {
            ALOGV("(f:%s, l:%d), meet ASF_Extended_Stream_Properties_Object_GUID, gsize=%lld, skip", __FUNCTION__, __LINE__, gsize);
            lseek(fd, gsize-24, SEEK_CUR);
            read_size += gsize-24;
        } else if (!memcmp(&g, &head1_guid, sizeof(GUID))) {
            ALOGV("(f:%s, l:%d), meet ASF_Header_Extension_Object_GUID, gsize=%lld, skip", __FUNCTION__, __LINE__, gsize);
            lseek(fd, gsize-24, SEEK_CUR);
            read_size += gsize-24;
        } else {
            lseek(fd, gsize - 24, SEEK_CUR);
            read_size += gsize-24;
        }
    }

    ALOGV("(f:%s, l:%d), video_num=%d, audio_num=%d, read_size=%lld", __FUNCTION__, __LINE__, video_stream_num, audio_stream_num, read_size);
    if(video_stream_num > 0)
    {
        wmv_file_flag = 1;
    }
    else if(audio_stream_num > 0)
    {
        wmv_file_flag = 2;
    }
    else
    {
        ALOGW("(f:%s, l:%d), detect none audio or video, we think it's wmv file!", __FUNCTION__, __LINE__);
        wmv_file_flag = 1;
    }

    //restore file pos
    lseek(fd, offset, SEEK_SET);

    if(2 == wmv_file_flag)
    {
        return AVPROBE_SCORE_MAX;
    }
    else
    {
	    return 0;
    }

fail:
    lseek(fd, offset, SEEK_SET);
    return 0;
}

static int ac3_probe(media_probe_data_t *p)
{
	unsigned char *ptr = p->buf;

	if(((ptr[0] == 0x77)&&(ptr[1] == 0x0b))
		||((ptr[0] == 0x0b)&&(ptr[1] == 0x77))
		||((ptr[0] == 0x72)&&(ptr[1] == 0xf8)&&(ptr[2] == 0xbb)&&(ptr[3] == 0x6f))
		||((ptr[0] == 0xf8)&&(ptr[1] == 0x72)&&(ptr[2] == 0x6f)&&(ptr[3] == 0xbb))
		||((ptr[0] == 0x72)&&(ptr[1] == 0xf8)&&(ptr[2] == 0xba)&&(ptr[3] == 0x6f))
		||((ptr[0] == 0xf8)&&(ptr[1] == 0x72)&&(ptr[2] == 0x6f)&&(ptr[3] == 0xba)))
		//0x72f8bb6f//0xf8726fbb//0x72f8ba6f//0xf8726fba
	{
		return AVPROBE_SCORE_MAX;
	}

	return 0;
}

static int dts_probe(media_probe_data_t *p)
{
	unsigned char *ptr = p->buf;

	if ((ptr[0] == 0xff && ptr[1] == 0x1f &&
              ptr[2] == 0x00 && ptr[3] == 0xe8 &&
              (ptr[4] & 0xf0) == 0xf0 && ptr[5] == 0x07)
           ||(ptr[0] == 0x1f && ptr[1] == 0xff &&
              ptr[2] == 0xe8 && ptr[3] == 0x00 &&
              ptr[4] == 0x07 && (ptr[5] & 0xf0) == 0xf0)
		   ||(ptr[0] == 0xfe && ptr[1] == 0x7f &&
			  ptr[2] == 0x01 && ptr[3] == 0x80)
		   ||(ptr[0] == 0x7f && ptr[1] == 0xfe &&
              ptr[2] == 0x80 && ptr[3] == 0x01))
	{
		return AVPROBE_SCORE_MAX;
	}

	return 0;
}

static int aac_probe(media_probe_data_t *p)
{
	unsigned char *ptr = p->buf;

	if((((ptr[0]&0xff)==0xff)&&((ptr[1]&0xf0)==0xf0))
		||(((ptr[0]&0xff)==0x56)&&((ptr[1]&0xe0)==0xe0))
		||(((ptr[0]&0xff)=='A')&&((ptr[1]&0xff)=='D')&&((ptr[2]&0xff)=='I')&&((ptr[3]&0xff)=='F')))
	{
		return AVPROBE_SCORE_MAX;
	}

	return 0;
}

#define MAX_FTYPE_SIZE 5

static int mp4_probe(media_probe_data_t *p) {
	unsigned int offset;
	unsigned int tag;
	int score = 0;
	char ftype_data[MAX_FTYPE_SIZE];

	/* check file header */
	offset = 0;
	for (;;) {
		/* ignore invalid offset */
		if ((offset + 8) > (unsigned int) p->buf_size)
			return score;
		tag = AV_RL32(p->buf + offset + 4);
		switch (tag) {
		/* check for obvious tags */
		case MKTAG('j','P',' ',' '): /* jpeg 2000 signature */
		case MKTAG('m','o','o','v'):
		case MKTAG('m','d','a','t'):
		case MKTAG('p','n','o','t'): /* detect movs with preview pics like ew.mov and april.mov */
		case MKTAG('u','d','t','a'): /* Packet Video PVAuthor adds this and a lot of more junk */
			return AVPROBE_SCORE_MAX;
			/* those are more common words, so rate then a bit less */
		case MKTAG('e','d','i','w'): /* xdcam files have reverted first tags */
		case MKTAG('w','i','d','e'):
		case MKTAG('f','r','e','e'):
		case MKTAG('j','u','n','k'):
		case MKTAG('p','i','c','t'):
			return AVPROBE_SCORE_MAX - 5;
		case MKTAG('f','t','y','p'):

			memcpy(ftype_data, p->buf+offset+8, MAX_FTYPE_SIZE-1);
			ftype_data[MAX_FTYPE_SIZE-1] = '\0';
//			ALOGV("ftype_data:%s",ftype_data);
//			if(strcasestr(ftype_data,"3gp") != NULL
//					|| strcasestr(ftype_data,"isom") != NULL
//					||strcasestr(ftype_data,"3gg6") != NULL) {
//				return 77;
//			}
			if((strcasestr(ftype_data,"3gg6") != NULL)
					|| (strcasestr(ftype_data,"m4a") != NULL)) {
				//cts test switch subtitle seamlessly with a 3gg5 video
				//and test setnextplayer with m4a audio, use stagefright player
				return 77;
			}
			return AVPROBE_SCORE_MAX;
		case MKTAG(0x82,0x82,0x7f,0x7d ):
		case MKTAG('s','k','i','p'):
		case MKTAG('u','u','i','d'):
		case MKTAG('p','r','f','l'):
			offset = AV_RB32(p->buf+offset) + offset;
			/* if we only find those cause probedata is too small at least rate them */
			score = AVPROBE_SCORE_MAX - 50;
			break;
		default:
			/* unrecognized tag */
			return score;
		}
	}

	return score;
}

int audio_format_detect(unsigned char *buf, int buf_size, int fd, int64_t offset)
{
	media_probe_data_t prob;
	int file_format = MEDIA_FORMAT_UNKOWN;
	int ret = 0;
    
	prob.buf = buf;
	prob.buf_size = buf_size;

	if(ogg_probe(&prob) > 0){
		return MEDIA_FORMAT_OGG;
	}
    
	if(amr_probe(&prob) > 0){
		return MEDIA_FORMAT_AMR;
	}
    
	if((ret = mp3_probe(&prob)) >= AVPROBE_SCORE_MAX/4-1){
        if(ret == (AVPROBE_SCORE_MAX - 3)) {
			return MEDIA_FORMAT_CEDARA;//mp1,mp2
		}
        else
        {
		    return MEDIA_FORMAT_MP3;
        }
	}

	if(ff_id3v2_match(buf)) {
        ret = ff_id3v2_tag_len(buf);
        prob.buf = buf + ret;
		prob.buf_size = buf_size - ret;
    }

	if( (ret = wav_probe(&prob)) > 0){
		if(ret == (AVPROBE_SCORE_MAX - 2)) {
			return MEDIA_FORMAT_DTS;
		}
        else if(ret == (AVPROBE_SCORE_MAX - 3)) {
        	ALOGD("adpcm probe!");
			return MEDIA_FORMAT_CEDARA;//adpcm
		}
		else if(ret == MEDIA_FORMAT_MP3){
			ALOGD("mp3 probe!");
			return MEDIA_FORMAT_MP3;
		}	
		else {
			return MEDIA_FORMAT_WAV;
		}
	}

	if((ret = mp4_probe(&prob)) > 0) {
		if(ret == 77) {
			return MEDIA_FORMAT_3GP;
		}
		else {
			return MEDIA_FORMAT_M4A;
		}

//		return MEDIA_FORMAT_3GP;
	}

	if(wma_probe(&prob, fd, offset) > 0){
		return MEDIA_FORMAT_WMA;
	}

	if(ape_probe(&prob) > 0){
		return MEDIA_FORMAT_APE;
	}

	if(flac_probe(&prob) > 0){
		return MEDIA_FORMAT_FLAC;
	}

	if(ac3_probe(&prob) > 0){
		return MEDIA_FORMAT_AC3;
	}

	if(dts_probe(&prob) > 0){
		return MEDIA_FORMAT_DTS;
	}

	if(aac_probe(&prob) > 0){
		return MEDIA_FORMAT_AAC;
	}

	return file_format;
}

int stagefright_system_format_detect(unsigned char *buf, int buf_size)
{
	media_probe_data_t prob;
	int file_format = MEDIA_FORMAT_UNKOWN;
	int ret = 0;

	prob.buf = buf;
	prob.buf_size = buf_size;

	if(ogg_probe(&prob) > 0){
		return MEDIA_FORMAT_OGG;
	}

	if(amr_probe(&prob) > 0){
		return MEDIA_FORMAT_AMR;
	}

	return file_format;
}

} // android namespace
