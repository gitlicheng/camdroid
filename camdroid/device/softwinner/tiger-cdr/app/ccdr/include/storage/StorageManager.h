#ifndef _STORAGE_MANAGER_H
#define _STORAGE_MANAGER_H

#include <sys/vfs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include <fcntl.h>

#include <utils/String8.h>
#include <StorageMonitor.h>
#include <DBCtrl.h>
#include <private/media/VideoFrame.h>

#include <minigui/common.h>
#include <minigui/gdi.h>

#include "Process.h"
#include "misc.h"
#include "cdr_res.h"
#include "camera.h"

#include "Rtc.h"
#include "EventManager.h"
#include <fat_user.h>

using namespace android;

long long totalSize(const char *path);
long long availSize(const char *path);

#define MOUNT_POINT STORAGE_POINT
#define MOUNT_PATH STORAGE_PATH
#define SZ_1M ((unsigned long long)(1<<20))
#define SZ_1G ((unsigned long long)(1<<30))
#define MAX_CARD_SIZE_MB ((unsigned long long)1<<17)	//1024*128 MB
//#define RESERVED_SIZE (SZ_1M * 300)
#define RESERVED_SIZE ((unsigned long long)((SZ_1M*500)/SZ_1M))
#define LOOP_COVERAGE_SIZE (RESERVED_SIZE + ((SZ_1M*500)/SZ_1M))
#define MIN_AVILIABLE_SPCE ((unsigned long long)((SZ_1M*50)/SZ_1M))

#define FILE_SZ_1M ((uint32_t)(1*1024*1024))

#define ALIGN( num, to ) (((num) + (to-1)) & (~(to-1)))
#define ALIGN32M(num) (ALIGN(num, (1<<25)))

#define MINUTE(ms) (ms/1000)
#define CSI_PREALLOC_SIZE(ms) (ALIGN32M((MINUTE(ms)*FRONT_CAM_BITRATE/8)))
#define UVC_PREALLOC_SIZE(ms) (ALIGN32M((MINUTE(ms)*BACK_CAM_BITRATE/8)))
#define AWMD_PREALLOC_SIZE(ms) CSI_PREALLOC_SIZE(ms)


#define CDR_TABLE "CdrFile"

typedef unsigned short WORD;
typedef unsigned char BYTE;

#define WININFOHEADERSIZE  40

#define SIZEOF_BMFH     14
#define SIZEOF_BMIH     40


#define BI_RGB          0
#define BI_RLE8         1
#define BI_RLE4         2
#define BI_BITFIELDS    3

#define SIZEOF_RGBQUAD      4

typedef struct tagRGBQUAD
{
    BYTE    rgbBlue;
    BYTE    rgbGreen;
    BYTE    rgbRed;
    BYTE    rgbReserved;
} RGBQUAD;


typedef struct BITMAPFILEHEADER
{
   unsigned short bfType;
   unsigned long  bfSize;
   unsigned short bfReserved1;
   unsigned short bfReserved2;
   unsigned long  bfOffBits;
} BITMAPFILEHEADER;

typedef struct WINBMPINFOHEADER  /* size: 40 */
{
   unsigned long  biSize;
   unsigned long  biWidth;
   unsigned long  biHeight;
   unsigned short biPlanes;
   unsigned short biBitCount;
   unsigned long  biCompression;
   unsigned long  biSizeImage;
   unsigned long  biXPelsPerMeter;
   unsigned long  biYPelsPerMeter;
   unsigned long  biClrUsed;
   unsigned long  biClrImportant;
} WINBMPINFOHEADER;

typedef struct abgrColor
{
    /**
     * The red component of a RGBA quarter.
     */
    unsigned char r;
    /**
     * The green component of a RGBA quarter.
     */
    unsigned char g;
    /**
     * The blue component of a RGBA quarter.
     */
    unsigned char b;
    /**
     * The alpha component of a RGBA quarter.
     */
    unsigned char a;
} abgrColor;

extern int SaveBmpToFatFs (PMYBITMAP my_bmp, RGB* pal, const char* spFileName);

static inline int fatFileExist(const char *path)
{
	int fd = fat_open(path, FA_OPEN_EXISTING);
	if (fd >= 0) {
		fat_close(fd);
		return 1;
	}
	return 0;
}

#ifdef FATFS
	#define FILE_EXSIST(PATH)	(fatFileExist(PATH))
#else
	#define FILE_EXSIST(PATH)	(!access(PATH, F_OK))
#endif

#define FLAG_NONE ""
#define FLAG_SNAP "_snap"
#define FLAG_SOS "_SOS"

#define FILE_FRONT "A"
#define FILE_BACK "B"

#ifdef TS_MUXER
#define EXT_VIDEO_MP4 ".mov"
#else
#define EXT_VIDEO_MP4 ".mp4"
#endif
#define EXT_PIC_JPG ".jpg"
#define EXT_PIC_BMP ".bmp"

#define GENE_IMPACT_TIME_FILE  "/data/impcttime.db"
#define IMPACT_TIME_TEXT_LENGTH   20
#define IMPACT_TIME_TEXT  128

enum {
	RET_IO_OK = 0,
	RET_IO_NO_RECORD = -150,
	RET_IO_DB_READ = -151,
	RET_IO_DEL_FILE = -152,
	RET_IO_OPEN_FILE = -153,
	RET_NOT_MOUNT = -154,
	RET_DISK_FULL = -155,
	RET_NEED_LOOP_COVERAGE = -156
};

typedef struct elem
{
	char *file;
	long unsigned int time;
	char *type;
	char *info;
}Elem;

typedef struct {
	String8 file;
	int time;
	String8 type;
	String8 info;
}dbRecord_t;

class CommonListener
{
public:
	CommonListener(){};
	virtual ~CommonListener(){}
	virtual void finish(int what, int extra) = 0;
};


#ifdef FATFS
	class FatBuffer
	{
		public:
			 FatBuffer(int alignSize);
			 ~FatBuffer();
			 int write(const void *ptr, int size, int num);
			 void syncFile(int fd);
		private:
			int mUnitSize;
			char *mBuf;
			int mWriteSize;
			int mTotalSize;
	};
#endif

class StorageManager : public StorageMonitorListener
{
private:
	void init();
	StorageManager();
	~StorageManager();
	StorageManager& operator=(const StorageManager& o);
public:
	static StorageManager* getInstance();
	void exit();
	void notify(int what, int status, char *msg);
	bool isInsert();
	bool isMount();
	int doMount(const char *fsPath, const char *mountPoint,
	             bool ro, bool remount, bool executable,
	             int ownerUid, int ownerGid, int permMask, bool createLost);
	int format(CommonListener *listener);
	int reNameFile(const char *oldpath, const char *newpath);
	int dbInit();
	void dbExit();
	void dbReset();
	int dbCount(char *item);
	void dbGetFiles(char *item, Vector<String8>&file);
	void dbGetFile(char *item, String8 &file );
	int dbGetSortByTimeRecord(dbRecord_t &record, int index);
	int dbAddFile(Elem *elem);
	int dbDelFile(char const *file);
	int generateFile(time_t &now, String8 &file, int cam_type, fileType_t type, bool cyclic_flag=true, bool next_fd=false);
	int saveVideoFrameToBmpFile(VideoFrame* videoFrame, const char* bmpFile);
	int savePicture(void* data, int size, String8 &file);
	int checkDirs();
	int dbUpdate();
	int getSoS_fileCapitly();
	int dbUpdate(char *type);
	int deleteFile(const char *file);
	bool dbIsFileExist(String8 file);
	int shareVol();
	int unShareVol();
	bool isDiskFull();
	unsigned long fileSize(String8 path);
	void dbUpdateFile(const char* oldname, const char *newname);

	void dbUpdateFileType(const char* name, const char *type) ;
	void setFileInfo(const char* oldName, bool isLock1 ,String8& nfile);

	String8 lockFile(const char *path);
	int getCapacity(unsigned long long *as, unsigned long long *ts);
	void needCheckAgain(bool val);
	class UpdateDbThread : public Thread
	{
	public:
		UpdateDbThread(StorageManager *storage): Thread(false),sm(storage){	}
		status_t start() {
			return run("InitDbThread", PRIORITY_NORMAL);
		}
		virtual bool threadLoop() {
			int ret = 0;
			sm->dblock(true);
			ret = sm->dbInit();
			sm->dblock(false);
			if (ret < 0) {	//check once again
				sm->dblock(true);
				ret = sm->dbInit();
				sm->dblock(false);
			}
			return false;
		}

	private:
		StorageManager *sm;
	};
	void stopUpdate();
	void dblock(bool flag);
#ifdef FATFS
	class FatCardDetectThread : public Thread
	{
	public:
		FatCardDetectThread(StorageManager *storage): Thread(false),sm(storage){	}
		status_t start() {
			return run("FatCardDetectThread", PRIORITY_NORMAL);
		}
		virtual bool threadLoop() {
			sm->detectTfCard();
			return true;
		}

	private:
		StorageManager *sm;
	};
	void detectTfCard();
#endif
	class FormatThread : public Thread
	{
	public:
		virtual bool threadLoop() {
			mSM->format();
			mSM->dbReset();
			mListener->finish(0, 0);
			return false;
		}
		status_t start() {
			return run("FormatThread", PRIORITY_NORMAL);
		}
		FormatThread(StorageManager *sm, CommonListener *listener) {mSM = sm; mListener = listener;};
	private:
		StorageManager *mSM;
		CommonListener *mListener;
	};
	void setEventListener(EventListener *pListener);
	int  readCpuInfo(char ret[]);
	int geneImpactTimeFile();
	int readImpactTimeFile(char ret[]);
	#ifdef FATFS
	bool reMountFATFS();
	void setFATFSMountValue(bool val);
	#endif


	class StorageSyncThread : public Thread
	{
	public:
		virtual bool threadLoop() {
			storageSyncLoop();
			return false;
		}
		status_t start() {
			return run("StorageSyncThread", PRIORITY_NORMAL);
		}
		void stopThread()
		{
			mLoop = false;
		}
		void storageSyncLoop();
		StorageSyncThread(StorageManager *sm, bool loop=false);
	private:
		StorageManager *mSM;
		bool mLoop;
	};
	void startSyncThread(bool loop=false);
	void stopSyncThread();
	void storageSync();
	int  deleteRecentlyFile();


private:
	int isInsert(const char *checkPath);
	int isMount(const char *checkPath);
	int deleteFiles(const char *type, const char *info);
	int deleteFiles();
	int format();
	StorageMonitor *mStorageMonitor;
	DBCtrl *mDBC;
	Mutex mDBLock;
	sp <FormatThread> mFT;
	sp <StorageSyncThread> mSST;
	bool mInsert;
	bool mMounted;
	EventListener *mListener;
	bool mDBInited;
	Mutex mDBInitL;
#ifdef FATFS
	sp<FatCardDetectThread> mFCDT;
#endif
	sp<UpdateDbThread> mUpdateDbTh;
	bool mStopDBUpdate;
	bool mNeedCheckAgain;
};
#endif
