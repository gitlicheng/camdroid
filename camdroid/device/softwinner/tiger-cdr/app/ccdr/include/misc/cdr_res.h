#ifndef __CDR_RES_H__
#define __CDR_RES_H__

/***************** file define *******************/
#define CON_2STRING(a, b)		a "" b
#define CON_3STRING(a, b, c)	a "" b "" c

#define TYPE_VIDEO "video"
#define TYPE_PIC "photo"
#define TYPE_LOCK "lock"
#define TYPE_THUMB ".thumb"

#define INFO_NORMAL "normal"
#define INFO_LOCK	"lock"

#define LOCK_DIR  TYPE_LOCK"/"
#define VIDEO_DIR TYPE_VIDEO"/"
#define PIC_DIR TYPE_PIC"/"
#define THUMBNAIL_DIR TYPE_THUMB"/"

/* -------------------- hints index --------------------------------- */
#define HINT_INDEX_SAVING_SCREEN		0

#endif	/* __CDR_RES_H__ */
