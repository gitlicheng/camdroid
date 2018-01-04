//#define ALOG_NDEBUG 0
#define ALOG_TAG "MovAvInfoDetect"
#include <utils/Log.h>

#include "string.h"

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <ConfigOption.h>
#include <cedarx_stream.h>

namespace android {

//#define INT_MAX      2147483647
#define MKTAG(a,b,c,d) (a | (b << 8) | (c << 16) | (d << 24))

typedef struct {
    uint32_t			type;
    int32_t			offset;
    int32_t			size;				/* total size (excluding the size and type fields) */
} MOV_atom_t;

typedef struct MOVContext {
	//FILE			*fp;
    struct cdx_stream_info *fp;
    int32_t			file_size;
	
    int32_t			time_scale;
    int32_t			duration;			/* duration of the longest track */
    int32_t			found_moov;			/* when both 'moov' and 'mdat' sections has been found */
    int32_t			found_mdat;			/* we suppose we have enough data to read the file */

    int32_t		has_video;
	int32_t		has_audio;

    int32_t			mdat_count;
    int32_t			isom;				/* 1 if file is ISO Media (mp4/3gp) */

	const struct MOVParseTableEntry *parse_table; /* could be eventually used to change the table */
} MOVContext;

typedef int32_t (*mov_parse_function)(MOVContext *ctx, MOV_atom_t atom);

typedef struct MOVParseTableEntry {
    uint32_t				type;
    mov_parse_function	func;
} MOVParseTableEntry;


static int32_t get_byte(struct cdx_stream_info *s)
{
    __u8 t;
    int32_t t1;
    s->read(&t, 1, 1, s);
    t1 = (int32_t)t;
    return t1;
}

static void get_buffer(struct cdx_stream_info *s,__u8 *data, uint32_t len)
{
    s->read(data, 1, len, s);
	return;
}

static uint32_t get_le16(struct cdx_stream_info *s)
{
    uint32_t val;
    val = get_byte(s);
    val |= get_byte(s) << 8;
    return val;
}

static uint32_t get_le24(struct cdx_stream_info *s)
{
    uint32_t val;
    val = get_le16(s);
    val |= get_byte(s) << 16;
    return val;
}

static uint32_t get_le32(struct cdx_stream_info *s)
{
    uint32_t val;
    val = get_le16(s);
    val |= get_le16(s) << 16;
    return val;
}

//static int32_t get_le64(FILE *s)
//{
//    int32_t val,val_h;
//    val = get_le32(s);
//    val_h = get_le32(s) << 32;
//
//    if(val_h!=0){
//		val = INT_MAX;
//	}
//    return val;
//}

static uint32_t get_be16(struct cdx_stream_info *s)
{
    uint32_t val;
    val = get_byte(s) << 8;
    val |= get_byte(s);
    return val;
}

static uint32_t get_be24(struct cdx_stream_info *s)
{
    uint32_t val;
    val = get_be16(s) << 8;
    val |= get_byte(s);
    return val;
}

static uint32_t get_be32(struct cdx_stream_info *s)
{
    uint32_t val;
    val = get_be16(s) << 16;
    val |= get_be16(s);
    return val;
}

static int32_t get_be64(struct cdx_stream_info *s)
{
    int32_t val;
	val = get_be32(s);
    if(val==0){
		val = (int32_t)get_be32(s);
	}
	else{
		get_be32(s);
		val = INT_MAX;
	}
    return val;
}

static void FSKIP(struct cdx_stream_info *fp,int32_t size){
    fp->seek(fp, size, SEEK_CUR);
}

int32_t fsize(struct cdx_stream_info *s)
{
    int32_t size;
    size = (int32_t)s->getsize(s);
    return size;
}

//--------------------------------------------------------
static int32_t mov_read_default(MOVContext *c, MOV_atom_t atom)
{
    int32_t total_size = 0;
    MOV_atom_t a;
    int32_t i;
    int32_t err = 0;
    //FILE *pb;
    struct cdx_stream_info *pb;

    pb = c->fp;
    a.offset = atom.offset;

    if (atom.size < 0)
        atom.size = 0x7fffffff;
    while(((total_size + 8) < atom.size) /*&& !feof(pb)*/ && !err) {
        a.size = atom.size;
        a.type=0L;
        if(atom.size >= 8) {
            a.size = get_be32(pb);
            a.type = get_le32(pb);
        }
        total_size += 8;
        a.offset += 8;
        if (a.size == 1) { /* 64 bit extended size */
            ALOGV("length may error!\n");
            a.size = get_be64(pb) - 8;
            a.offset += 8;
            total_size += 8;
        }
        if (a.size == 0) {
            a.size = atom.size - total_size;
            if (a.size <= 8)
                break;
        }
        a.size -= 8;
        if(a.size < 0 || a.size > atom.size - total_size)
            break;

        for (i = 0; c->parse_table[i].type != 0L
             && c->parse_table[i].type != a.type; i++)
            /* empty */;

        ALOGV("type:%08x size:%x\n",a.type,a.size);

        if (c->parse_table[i].type == 0) { /* skip leaf atoms data */
            FSKIP(pb, a.size);
        } else {
            //int32_t start_pos = ftell(pb);
            int32_t start_pos = pb->tell(pb);
            int32_t left;
            err = (c->parse_table[i].func)(c, a);
            left = a.size - pb->tell(pb) + start_pos;
            if (left > 0) /* skip garbage at atom end */
                FSKIP(pb, left);
        }
        a.offset += a.size;
        total_size += a.size;
    }

    if (!err && total_size < atom.size && atom.size < 0x7ffff) {
        FSKIP(pb, atom.size - total_size);
    }

    return err;
}

static int32_t mov_read_ftyp(MOVContext *c, MOV_atom_t atom)
{
    //FILE *pb;
    struct cdx_stream_info *pb;
    uint32_t type;
    
    pb = c->fp;
    type = get_le32(pb);
    
    if (type != MKTAG('q','t',' ',' '))
        c->isom = 1;
    get_be32(pb); /* minor version */
    FSKIP(pb, atom.size - 8);
    return 0;
}

static int32_t mov_read_mdat(MOVContext *c, MOV_atom_t atom)
{
    //FILE *pb; 
    struct cdx_stream_info *pb;
    
    pb = c->fp;
    if(atom.size == 0) /* wrong one (MP4) */
        return 0;

    c->mdat_count++;
    c->found_mdat=1;

    if(c->found_moov)
        return 1; /* found both, just go */
    FSKIP(pb, atom.size);
    return 0; /* now go for moov */
}

static int32_t mov_read_wide(MOVContext *c, MOV_atom_t atom)
{
    int32_t err;
    //FILE *pb;
    struct cdx_stream_info *pb;
    
    pb = c->fp;

    if (atom.size < 8)
        return 0; /* continue */
    if (get_be32(pb) != 0) { /* 0 sized mdat atom... use the 'wide' atom size */
        FSKIP(pb, atom.size - 4);
        return 0;
    }
    atom.type = get_le32(pb);
    atom.offset += 8;
    atom.size -= 8;
    if (atom.type != MKTAG('m', 'd', 'a', 't')) {
        FSKIP(pb, atom.size);
        return 0;
    }
    err = mov_read_mdat(c, atom);
    return err;
}

static int32_t mov_read_moov(MOVContext *c, MOV_atom_t atom)
{
    int32_t err;

    err = mov_read_default(c, atom);
    c->found_moov=1;
    if(c->found_mdat)
        return 1; /* found both, just go */
    return 0;     /* now go for mdat */
}

static int32_t mov_read_mvhd(MOVContext *c, MOV_atom_t atom)
{
    //FILE *pb = c->fp;
    struct cdx_stream_info *pb = c->fp;
    int32_t version = get_byte(pb); /* version */
    get_byte(pb); get_byte(pb); get_byte(pb); /* flags */

    if (version == 1) {
        get_be64(pb);
        get_be64(pb);
    } else {
        get_be32(pb); /* creation time */
        get_be32(pb); /* modification time */
    }
    c->time_scale = get_be32(pb); /* time scale */

    c->duration =(int32_t) ( (version == 1) ? get_be64(pb) : get_be32(pb) ); /* duration */
	
	FSKIP(pb,6+10+36+28);

    return 0;
}

static int32_t mov_read_trak(MOVContext *c, MOV_atom_t atom)
{
    return mov_read_default(c, atom);
}

static int32_t mov_read_tkhd(MOVContext *c, MOV_atom_t atom)
{
    //FILE *pb = c->fp;
    struct cdx_stream_info *pb = c->fp;
    int32_t version = get_byte(pb);

    get_byte(pb); get_byte(pb);
    get_byte(pb);		/* flags */

    if (version == 1) {
        get_be64(pb);
        get_be64(pb);
    } else {
        get_be32(pb);	/* creation time */
        get_be32(pb);	/* modification time */
    }
    get_be32(pb); /* track id (NOT 0 !)*/
    get_be32(pb);		/* reserved */
    //st->start_time = 0; /* check */
    (version == 1) ? get_be64(pb) : get_be32(pb); /* highlevel (considering edits) duration in movie timebase */

    FSKIP(pb, 16+36+8); /* display matrix */

    return 0;
}

static int32_t mov_read_mdhd(MOVContext *c, MOV_atom_t atom)
{
    //FILE *pb = c->fp;
    struct cdx_stream_info *pb = c->fp;
    int32_t version = get_byte(pb);
    int32_t lang;
    
    if (version > 1)
        return 1;	/* unsupported */

    get_byte(pb); get_byte(pb);
    get_byte(pb);	/* flags */

    if (version == 1) {
        get_be64(pb);
        get_be64(pb);
    } else {
        get_be32(pb); /* creation time */
        get_be32(pb); /* modification time */
    }

    get_be32(pb);
    (version == 1) ? get_be64(pb) : get_be32(pb); /* duration */

    lang = get_be16(pb); /* language */
    get_be16(pb); /* quality */

    return 0;
}

static int32_t mov_read_hdlr(MOVContext *c, MOV_atom_t atom)
{
    uint32_t type;
    uint32_t ctype;
    //FILE *pb = c->fp;
    struct cdx_stream_info *pb = c->fp;

    get_byte(pb);		/* version */
    get_byte(pb); get_byte(pb); get_byte(pb); /* flags */

    ctype = get_le32(pb);
    type = get_le32(pb); /* component subtype */

    if(!ctype)
        c->isom = 1;

    if(type == MKTAG('v', 'i', 'd', 'e')){
		c->has_video = 1;

	}
    else if(type == MKTAG('s', 'o', 'u', 'n')){
		c->has_audio = 1;
	}

    get_be32(pb); /* component  manufacture */
    get_be32(pb); /* component flags */
    get_be32(pb); /* component flags mask */

    if(atom.size <= 24)
        return 0; /* nothing left to read */

    FSKIP(pb, atom.size - (pb->tell(pb) - atom.offset));
    return 0;
}

static int32_t mov_read_cmov(MOVContext *c, MOV_atom_t atom)
{
    return -1; //Not support for compressed mode
}

static const MOVParseTableEntry mov_default_parse_table[] = {
/* mp4 atoms */

{ MKTAG( 'e', 'd', 't', 's' ), mov_read_default },
{ MKTAG( 'f', 't', 'y', 'p' ), mov_read_ftyp },
{ MKTAG( 'h', 'd', 'l', 'r' ), mov_read_hdlr },
{ MKTAG( 'm', 'd', 'a', 't' ), mov_read_mdat },
{ MKTAG( 'm', 'd', 'h', 'd' ), mov_read_mdhd },
{ MKTAG( 'm', 'd', 'i', 'a' ), mov_read_default },
{ MKTAG( 'm', 'i', 'n', 'f' ), mov_read_default },
{ MKTAG( 'm', 'o', 'o', 'v' ), mov_read_moov },
{ MKTAG( 'm', 'v', 'h', 'd' ), mov_read_mvhd },

{ MKTAG( 's', 't', 'b', 'l' ), mov_read_default },
{ MKTAG( 't', 'k', 'h', 'd' ), mov_read_tkhd }, /* track header */
{ MKTAG( 't', 'r', 'a', 'k' ), mov_read_trak },

{ MKTAG( 'w', 'i', 'd', 'e' ), mov_read_wide }, /* place holder */
{ MKTAG( 'c', 'm', 'o', 'v' ), mov_read_cmov },

{ MKTAG( 'a', 'v', 'i', 'd' ), mov_read_mdat }, //3dv
{ MKTAG( '3', 'd', 'v', 'f' ), mov_read_moov }, //3dv
{ 0L, NULL }
};

//--------------------------------------------------------

static int32_t mov_read_header(MOVContext *s, int content_size)
{
    int32_t err;
    MOV_atom_t atom = { 0, 0, 0 };
    //FILE *pb;
    struct cdx_stream_info *pb;

    pb = s->fp;
    s->parse_table = mov_default_parse_table;

    if(content_size == 0)
    	atom.size = fsize(pb);
    else
    	atom.size = content_size;

    /* check MOV header */
    err = mov_read_default(s, atom);
    if (err<0 || (!s->found_moov && !s->found_mdat)) {
        return -1;
    }

    return 0;
}

int MovAudioOnlyDetect0(const char *url)
{
	MOVContext *c;
    
	c = (MOVContext *)malloc(sizeof(MOVContext));
	memset(c,0,sizeof(MOVContext));
	CedarXDataSourceDesc dataSourceDesc;
    memset(&dataSourceDesc, 0, sizeof(CedarXDataSourceDesc));
    dataSourceDesc.source_url = (char*)url;
    dataSourceDesc.source_type = CEDARX_SOURCE_FILEPATH;
    dataSourceDesc.stream_type = CEDARX_STREAM_LOCALFILE;
    c->fp = create_stream_handle(&dataSourceDesc);
    if (c->fp == NULL) {
        ALOGE("(f:%s, l:%d) create_stream_handle error, path=%s", __FUNCTION__, __LINE__, url);
        free(c);
        return 0;
    }
    
	if(mov_read_header(c, 0) != 0)
		return 0;

	ALOGD("has_audio:%d has_video:%d\n", c->has_audio, c->has_video);
    destory_stream_handle(c->fp);
    c->fp = NULL;
	free(c);
	
	return (!c->has_video && c->has_audio);
}

int MovAudioOnlyDetect1(int fd, int64_t offset, int64_t length)
{
	MOVContext *c;

	c = (MOVContext *)malloc(sizeof(MOVContext));
	memset(c,0,sizeof(MOVContext));
//    int tmpfd = dup(fd);
//    if(tmpfd < 0)
//    {
//        free(c);
//        return 0;
//    }
//	c->fp = fdopen(tmpfd,"rb");
//	if(!c->fp) {
//		free(c);
//        close(tmpfd);
//		return 0;
//	}

    CedarXDataSourceDesc dataSourceDesc;
    memset(&dataSourceDesc, 0, sizeof(CedarXDataSourceDesc));
    dataSourceDesc.ext_fd_desc.fd = fd;
    dataSourceDesc.ext_fd_desc.offset = offset;
    dataSourceDesc.ext_fd_desc.length = length;
    dataSourceDesc.source_type = CEDARX_SOURCE_FD;
    dataSourceDesc.stream_type = CEDARX_STREAM_LOCALFILE;
    c->fp = create_stream_handle(&dataSourceDesc);
    if (c->fp == NULL) {
        ALOGE("(f:%s, l:%d) create_stream_handle error, fd=%d", __FUNCTION__, __LINE__, fd);
        free(c);
        return 0;
    }

	//fseek(c->fp, offset, SEEK_SET);

	if(mov_read_header(c, length) != 0)
	{
        destory_stream_handle(c->fp);
        c->fp = NULL;
        free(c);
        //close(tmpfd);
		return 0;
	}

	ALOGD("has_audio:%d has_video:%d\n", c->has_audio, c->has_video);

	//fclose(c->fp);
	destory_stream_handle(c->fp);
    c->fp = NULL;
	free(c);
    
    //close(tmpfd);
	return (!c->has_video && c->has_audio);
}

}
