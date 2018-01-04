#ifndef _MISC_H
#define _MISC_H

#include <minigui/common.h>
#include <minigui/gdi.h>
#include <utils/String8.h>
#include <media/AudioTrack.h> 
#include <binder/MemoryHeapBase.h>
#include <binder/MemoryBase.h>

#include <platform.h>

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164
using namespace android;


typedef struct pos_t
{
	int x;
	int y;
}pos_t;

struct riff_wave_header {
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t wave_id;
};

struct chunk_header {
    uint32_t id;
    uint32_t sz;
};

struct chunk_fmt {
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;

};

extern void switchKeyVoice(bool flag);
extern void setKeySoundVol(float val);
extern void playKeySound();
extern int initKeySound(const char *path);

extern void Line2 (HDC hdc, int x1, int y1, int x2, int y2);
extern const char* removeSuffix(const char *file, char *buf);
extern const char* getBaseName(const char *file, char *buf);
extern int copyFile(const char* from, const char* to);
extern void ck_off(int hlay);
extern int set_ui_alpha(unsigned char alpha);
extern void getScreenInfo(int *w, int *h);
enum CAMERA_DEV {
	CAMERA_UNKNOWN = 0,
	CAMERA_CSI,
	CAMERA_USB,
	CAMERA_TVD
};

typedef struct {
	int x,y;	/* 矩形的左上角的坐标 */
	int w,h;	/* 矩形的宽和高 */
}CDR_RECT;

typedef enum {
	CSI_ONLY = 1,
	UVC_ONLY,
	UVC_ON_CSI,
	CSI_ON_UVC,
	NO_PREVIEW
}pipMode_t;

typedef enum {
	VIDEO_TYPE_NORMAL = 1,
	VIDEO_TYPE_IMPACT,
	PHOTO_TYPE_NORMAL,
	PHOTO_TYPE_SNAP,
}fileType_t;

typedef struct {
	android::String8 fileString;
	time_t time;
	android::String8 infoString;
}recordDBInfo_t;
typedef struct {
	unsigned int curIdx;
	unsigned int fileCounts;
	android::String8 curTypeString;	/* video or picture */
	android::String8 curInfoString;	/* normal or impact */
	android::String8 curFileName;
}PLBPreviewFileInfo_t;
#define ARRYSIZE(table)    (sizeof(table)/sizeof(table[0]))

#define PREALLOC_SIZE_100M (100*1024*1024)
#define PREALLOC_SIZE_200M (PREALLOC_SIZE_100M * 2)
#endif
