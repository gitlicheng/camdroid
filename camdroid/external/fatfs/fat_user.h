#ifndef _FAT_USER_H_
#define _FAT_USER_H_

#define	FA_READ				0x01
#define	FA_OPEN_EXISTING	0x00
#define	FA_WRITE			0x02
#define	FA_CREATE_NEW		0x04
#define	FA_CREATE_ALWAYS	0x08
#define	FA_OPEN_ALWAYS		0x10
#define FA__WRITTEN			0x20
#define FA__DIRTY			0x40

#define	AM_RDO	0x01	/* Read only */
#define	AM_HID	0x02	/* Hidden */
#define	AM_SYS	0x04	/* System */
#define	AM_VOL	0x08	/* Volume label */
#define AM_LFN	0x0F	/* LFN entry */
#define AM_DIR	0x10	/* Directory */
#define AM_ARC	0x20	/* Archive */
#define AM_MASK	0x3F	/* Mask of defined bits */

//__BEGIN_DECLS
#ifdef __cplusplus
extern "C"
{
#endif

/* This type MUST be 8 bit */
typedef unsigned char	BYTE;

/* These types MUST be 16 bit */
typedef short			SHORT;
typedef unsigned short	WORD;
typedef unsigned short	WCHAR;

/* These types MUST be 16 bit or 32 bit */
typedef int				INT;
typedef unsigned int	UINT;

/* These types MUST be 32 bit */
typedef long			LONG;
typedef unsigned long	DWORD;

/* File information structure (FILINFO) */
typedef struct {
	DWORD	fsize;			/* File size */
	WORD	fdate;			/* Last modified date */
	WORD	ftime;			/* Last modified time */
	BYTE	fattrib;		/* Attribute */
	char	fname[13];		/* Short file name (8.3 format) */
	char 	lfname[128];	/* Pointer to the LFN buffer */
	UINT 	lfsize;			/* Size of LFN buffer in TCHAR */
	
} FAT_FILINFO;

typedef struct {
	UINT fd;
	FAT_FILINFO fileinfo;
	char path[128];
}FAT_FILEINFO_FD;

typedef struct {
	char path[128];
	BYTE mode;
	BYTE revert[3];
}FAT_OPEN_ARGS;

#define FAT_BUFFER_SIZE   64*1024
typedef struct {
	UINT fd;
	UINT len;
	char buf[FAT_BUFFER_SIZE+128];
}FAT_BUFFER;

typedef struct {
	int fd;
	DWORD ofs;
}FAT_SEEK;

typedef struct {
	BYTE attr;
	BYTE mask;
	char path[128];	
}FAT_FILE_ATTR;

typedef struct {
	unsigned int        f_type;
	unsigned int        f_bsize;
	unsigned long long  f_blocks;
	unsigned long long  f_bfree;
} FAT_STATFS;

typedef struct {
	int fd;
	long long len;
} FAT_FILE_LEN;


/*******************************************************************/
/* Open or create a file */
int fat_open (const char* path, BYTE mode);

/* Close an open file object */
int fat_close (int fd);

/* Read data from a file */
int fat_read (int fd, void* buff, UINT btr);
			
/* Write data to a file */
int fat_write (int fd, const void* buff, UINT btw);

/* Move file pointer of a file object */
int fat_lseek (int fd, DWORD ofs);			

/* Truncate file */
int fat_truncate (int fd);

/* Flush cached data of a writing file */
int fat_sync (int fd);

/* Get position of a file */
long long fat_tell(int fd);

/* Get length of a file */
long long fat_size(int fd);

/*******************************************************************/
/* Open a directory */
int fat_opendir (const char* path);	

/* Close an open directory */
int fat_closedir (int fd);			

/* Read a directory item */
int fat_readdir (int fd, FAT_FILINFO* fno);	

/*******************************************************************/
/* Create a sub directory */
int fat_mkdir (const char* path);	

/* Delete an existing file or directory */
int fat_unlink (const char* path);								

/* Rename/Move a file or directory */
int fat_rename (const char* path_old, const char* path_new);	

/* Get file status */
int fat_stat (const char* path, FAT_FILINFO* fno);					

/* Set file attribution */
int fat_chmod(const char* path, BYTE attr, BYTE mask);

/* Get number of free clusters on the drive */
int fat_getfree (const char* path);

/* Create a file system on the volume */
int fat_mkfs (const char* path);			

/*******************************************************************/
int fat_statfs(const char* path, FAT_STATFS *buf);

/*******************************************************************/
/* Detect card whether it exist or not */
/* @return: 0-not exist; 1-exist*/
int fat_card_status(void);

/* Prepare before read/write */
/* @return: 0-everything is ok; 1-card NOT exist; 2-card NOT fatfs; 3-card NOT 64KB/cluster; */
int fat_card_prepare(void);

/* Get card's fs-type */
/* @return: 1-fat12; 2-fat16; 3-fat32; 4-other filesystem */
int fat_card_fstype(void);

/* Remount card when switch from vfs or plug in */
/* @return: 0-ok; !0-remount fail */
int fat_card_remount(void);

/* Judge whether need to format card or not */
/* @return: 0-not need(fatfs+64KB/cluster); 1-need format */
int fat_card_format(void);

//__END_DECLS
#ifdef __cplusplus
}
#endif


#endif
