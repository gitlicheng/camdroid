#ifndef CEDARX_STREAM_H_
#define CEDARX_STREAM_H_

//#define _FILE_OFFSET_BITS 64
//#define __USE_FILE_OFFSET64
//#define __USE_LARGEFILE64
//#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include <unistd.h>

#include <CDX_Common.h>
#include <include_base/tsemaphore.h>

__BEGIN_DECLS

#if (_FILE_OFFSET_BITS==64)
#define cdx_off_t long long
#else
#define cdx_off_t long long
#endif

/*   stream.h */
#define STREAM_BUFFER_SIZE 2048
#define VCD_SECTOR_SIZE 2352
#define VCD_SECTOR_OFFS 24
#define VCD_SECTOR_DATA 2324

#define DRAM_BUF_LEN  (1024*256)
#define READ_CACHE_LEN  (1024*64)

typedef struct cdx_data_buf {
	char *buf;
	size_t buf_len;
	char *buf_pos;
	size_t data_len;
} cdx_data_buf_t;

typedef struct cdx_write_callback {
    void *hComp;
    int (*cb)(void *hComp, int event);
} cdx_write_callback_t;

typedef struct cdx_stream_info {
  const char *info;
  const char *name;
  const char *comment;

  int 					quitFlag;
  cdx_sem_t            	sem_data_ready;
  reqdata_from_dram  	request_data;
  cdx_data_buf_t    	data_buf;
  cdx_data_buf_t        read_cache;
  int					isReqData;
  CedarXDataSourceDesc  data_src_desc;

  cdx_write_callback_t  callback;
  int writeErrcnt;
  int writeError;

  //TODO:merge with data_src_desc above;
  //This variable is holy shit.
  //CedarXDataSourceDesc  *another_data_src_desc;

  //below reserved are only used by cedarx internal
  void *reserved_0;
  void *reserved_1;

  //below reserved are used by customer
  void *reserved_usr_0;
  void *reserved_usr_1;

  char *mpFilePath;
  FILE  *file_handle;
  char *mpAlignBuf;
  int   mAlignBufSize;
  int   mIODirectionFlag;   //0:in, 1:out; in=read, out=write,
  CedarXExternFdDesc fd_desc;
  long long mFileEndOffset;   //used in fwrite, logical file end position
  long long mFileSize;    //used in fwrite, physical file end position, may be bigger than mEndOffset because extend to blockSize for directIO.
  int   mFtruncateFlag;  //private member, for directIO, when use write() instead of ftruncate(), set this flag to 1.

  void* m3u8_context;

  int  (*seek)(struct cdx_stream_info *stream, cdx_off_t offset, int whence);
  cdx_off_t (*tell)(struct cdx_stream_info *stream);
  ssize_t  (*read)(void *ptr, size_t size, size_t nmemb, struct cdx_stream_info *stream);
  ssize_t  (*write)(const void *ptr, size_t size, size_t nmemb, struct cdx_stream_info *stream);
  int  (*write2)(void *bs_info, struct cdx_stream_info *stream);
  int  (*truncate)(struct cdx_stream_info *stream, cdx_off_t length);
  int  (*fallocate)(struct cdx_stream_info *stream, int mode, int64_t offset, int64_t len);
  long long (*getsize)(struct cdx_stream_info *stream);
  int (*destory)(struct cdx_stream_info *stm_info);
  int (*decrypt)(void *ptr, size_t size, int pkt_type, struct cdx_stream_info *stream);
  //below two function used for m3u/ts
  long long (*seek_to_time)(struct cdx_stream_info *stream, long long us);
  long long (*get_total_duration)(struct cdx_stream_info *stream);

  void (*reset_stream)(struct cdx_stream_info *stream);
  int (*control_stream)(struct cdx_stream_info * stream, void *arg, int cmd);
  int (*extern_writer)(void *parent, void *bs_info);
} cdx_stream_info_t;

extern struct cdx_stream_info *create_stream_handle(CedarXDataSourceDesc *datasource_desc);
extern void destory_stream_handle(struct cdx_stream_info *stm_info);
extern struct cdx_stream_info *create_outstream_handle(CedarXDataSourceDesc *datasource_desc);
extern void destroy_outstream_handle(struct cdx_stream_info *stm_info);

extern int stream_remove_file(char* fileName);
extern int stream_mkdir(char *dirName, int mode);

static inline int cdx_seek(struct cdx_stream_info *stream, cdx_off_t offset, int whence)
{
	return stream->seek(stream, offset, whence);
}

static inline cdx_off_t cdx_tell(struct cdx_stream_info *stream)
{
	return stream->tell(stream);
}

static inline int cdx_read(void *ptr, size_t size, size_t nmemb, struct cdx_stream_info *stream)
{
	return stream->read(ptr, size, nmemb,stream);
}

static inline int cdx_write(const void *ptr, size_t size, size_t nmemb, struct cdx_stream_info *stream)
{
	return stream->write(ptr, size, nmemb,stream);
}

static inline int cdx_write2(void *bs_info, struct cdx_stream_info *stream)
{
	return stream->write2(bs_info,stream);
}

__END_DECLS

#endif /* CEDAR_DEMUX_H_ */
