#include "StorageManager.h"
#undef LOG_TAG
#define LOG_TAG "StorageManager"
#include "debug.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>

#define SYNC_THREAD

static StorageManager* gInstance;
static Mutex gInstanceMutex;

static DBC_TYPE gDBType[] = {
	DB_TEXT,
	DB_UINT32, 
	DB_TEXT,
	DB_TEXT,
};

static struct sql_tb gSqlTable[] = {
	{"file", "VARCHAR(128)"},
	{"time", "INTEGER"},
	{"type", "VARCHAR(32)"},		/* "video" "pic" */
	{"info", "VARCHAR(32)"},		/* "normal"  */
};

void StorageManager::init()
{
	mStorageMonitor = new StorageMonitor;
	mStorageMonitor->init();
	mStorageMonitor->setMonitorListener(this);
	if(isMount(MOUNT_POINT)) {
		mMounted = true;
		checkDirs();
	}
	else
		mMounted = false;
	mUpdateDbTh = new UpdateDbThread(this);
	mUpdateDbTh->start();
//	mDBInitL.lock();
//	dbInit();
//	mDBInitL.unlock();
	db_info("%s is %s\n", MOUNT_PATH, (mMounted == true) ? "mounted" : "not mount");
#ifdef FATFS
	mFCDT = NULL;
	//mFCDT = new FatCardDetectThread(this);
	//mFCDT->start();
#endif
}

void StorageManager::exit()
{
	delete mStorageMonitor;
	mStorageMonitor = NULL;
	dbExit();
}

int StorageManager::shareVol()
{
	if (mStorageMonitor) {
		return mStorageMonitor->mountToPC();
	}
	return -1;
}
int StorageManager::unShareVol()
{
	if (mStorageMonitor) {
		return mStorageMonitor->unmountFromPC();
	}
	return -1;
}
StorageManager::StorageManager()
	: mStorageMonitor(NULL)
	, mDBC(NULL)
	, mSST(NULL)
	, mListener(NULL)
	, mDBInited(false)
	, mStopDBUpdate(false)
	, mNeedCheckAgain(false)
{
	init();
}

StorageManager::~StorageManager()
{
#ifdef FATFS
	if(mFCDT != NULL) {
		mFCDT.clear();
		mFCDT = NULL;
	}
#endif
	if(mUpdateDbTh != NULL){
		mUpdateDbTh.clear();
		mUpdateDbTh = NULL;
	}
	exit();
}
StorageManager* StorageManager::getInstance(void)
{
	Mutex::Autolock _l(gInstanceMutex);
	if(gInstance != NULL) {
		return gInstance;
	}
	gInstance = new StorageManager();
	return gInstance;
}

void StorageManager::notify(int what, int status, char *msg)
{
	switch(status){
	case STORAGE_STATUS_UNMOUNTING:
	case STORAGE_STATUS_NO_MEDIA:
		{
			db_msg("~~unmounting");
			//dbUpdate();
			if (mStorageMonitor->isFormated() || mStorageMonitor->isShare()){
				break;
			}
			if (mMounted == true) {
				if (mNeedCheckAgain) {
					mMounted = isInsert("/dev/block/mmcblk0p1");
					if (mMounted == true) {
						mNeedCheckAgain = false;
						return notify(0, STORAGE_STATUS_MOUNTED, NULL);
					}
				}
				mMounted = false;
				if(mListener)
					mListener->notify(EVENT_MOUNT, 0);
			}
			stopUpdate();
			break;
		}
	case STORAGE_STATUS_MOUNTED:
		{
			db_msg("~~mounted");
			#ifdef FATFS
				fat_card_remount();
			#endif
			mMounted = true;
			checkDirs();
			dbReset();
			if (mStorageMonitor->isFormated() || mStorageMonitor->isShare()){
				break;
			}
			if(mListener)
			mListener->notify(EVENT_MOUNT, 1);
			break;
		}
#ifdef  FATFS
	case STORAGE_STATUS_REMOVE:
		if(mListener)
			mListener->notify(EVENT_REMOVESD, 1);
		break;
	case STORAGE_STATUS_PENDING:
		if(mListener)
			mListener->notify(EVENT_INSERTSD, 1);
		break;
#endif
	}
}
int StorageManager::isInsert(const char *checkPath)
{
#ifdef FATFS
	return isMount(checkPath);
#else
	FILE *fp;
	db_msg("------------checkPath = %s \n", checkPath);
	if (!(fp = fopen(checkPath, "r"))) {
		db_msg(" isInsert fopen fail, checkPath = %s\n", checkPath);
		return 0;
	}
	if(fp){
		fclose(fp);
	}
	return 1;
#endif
}

int StorageManager::isMount(const char *checkPath)
{
#ifdef FATFS
	if (fat_card_status() > 0) {
        return 1;
	}
	return 0;
#else
	const char *path = "/proc/mounts";
	FILE *fp;
	char line[255];
	const char *delim = " \t";
	ALOGV(" isMount checkPath=%s \n",checkPath);
	if (!(fp = fopen(path, "r"))) {
		ALOGV(" isMount fopen fail,path=%s\n",path);
		return 0;
	}
	memset(line,'\0',sizeof(line));
	while(fgets(line, sizeof(line), fp))
	{
		char *save_ptr = NULL;
		char *p        = NULL;
		//ALOGV(" isMount line=%s \n",line);
		if (line[0] != '/' || strncmp("/dev/block/vold",line,strlen("/dev/block/vold")) != 0)
		{
			memset(line,'\0',sizeof(line));
			continue;
		}       
		if ((p = strtok_r(line, delim, &save_ptr)) != NULL) {		    			
			if ((p = strtok_r(NULL, delim, &save_ptr)) != NULL){
				ALOGV(" isMount p=%s \n",p);
				if(strncmp(checkPath,p,strlen(checkPath) ) == 0){
					fclose(fp);
					ALOGV(" isMount return 1\n");
					return 1;				
				}				
			}					
		}		
	}//end while(fgets(line, sizeof(line), fp))	
	if(fp){
		fclose(fp);
	}
	return 0;
#endif
}

bool StorageManager::isInsert(void)
{
#ifdef FATFS
	mInsert = fat_card_status();
	return mInsert;
#else
	mInsert = mStorageMonitor->isInsert();
	db_msg("------------mInsert = %d \n", mInsert);
	return mInsert;
#endif
}
bool StorageManager::isMount(void)
{
	//mMounted = mStorageMonitor->isMount();
	return mMounted;
}

#ifdef FATFS
bool StorageManager::reMountFATFS()
{
	int ret = fat_card_remount();
	if(ret == 0)
		mMounted = true;
	else
		mMounted = false;
	return mMounted;
}

void StorageManager::setFATFSMountValue(bool val)
{
	mMounted = val;
}
#endif


void StorageManager::needCheckAgain(bool val)
{
	mNeedCheckAgain = val;
}

String8 StorageManager::lockFile(const char *path)
{
	String8 file(path);
	// /mnt/extsd/video/time_A.mp4 -> /mnt/extsd/video/time_A_SOS.mp4
	file = file.getBasePath();
	file += FLAG_SOS;
	file += EXT_VIDEO_MP4;
	db_msg("lockFile:%s --> %s", path, file.string());
	file = String8(CON_2STRING(MOUNT_PATH,LOCK_DIR))+file.getPathLeaf();
	reNameFile(path, file.string());
	return file;
}

int StorageManager::reNameFile(const char *oldpath, const char *newpath)
{
	int ret = 0;
#ifdef FATFS
	ret = fat_rename(oldpath,newpath);
#else
	ret = rename(oldpath,newpath);
#endif
	if(ret != 0){
		db_error("reNameFile fail oldpath=%s, newpath = %s,ret=%d\n",oldpath,newpath,ret);
	}

	return ret;
}

int needDeleteFiles()
{
	unsigned long long as = availSize(MOUNT_PATH);
	unsigned long long ts = totalSize(MOUNT_PATH);
	db_msg("[Size]:%lluMB/%lluMB, reserved:%lluMB", as, ts, RESERVED_SIZE);

	if (as <= RESERVED_SIZE) {
		db_msg("!Disk FULL");
		return RET_DISK_FULL;
	}
	
	if (as <= LOOP_COVERAGE_SIZE) {
		db_msg("!Loop Coverage");
		return RET_NEED_LOOP_COVERAGE;
	}
	
	return 0;
}

int StorageManager::getCapacity(unsigned long long *as, unsigned long long *ts)
{
	*as = availSize(MOUNT_PATH);
	*ts = totalSize(MOUNT_PATH);
	return 0;
}

int StorageManager::deleteFile(const char *file)
{
	int ret = 0;
	String8 path(file);
	if (FILE_EXSIST(file)) {
#ifdef FATFS
	if (fat_unlink(file) < 0) {
#else
	if (remove(file) != 0) {
#endif
			ret = -1;
		}
	} else {
		db_msg("file not existed:%s", file);
	}
	/* delete thumbnail */
	if (path.getPathExtension() == EXT_VIDEO_MP4 ||
		path.getPathExtension() == EXT_PIC_JPG) {
		String8 stime;
		stime = path.getBasePath().getPathLeaf();
		path = (path.getPathDir())+"/"+THUMBNAIL_DIR+stime+EXT_PIC_BMP;// /mnt/extst/video/time_A.mp4 ->/mnt/extsd/video->/mnt/extsd/video/.thumb/time_A.bmp
		db_msg("delete pic :%s", path.string());
		return deleteFile(path.string());
	}
	return ret;
}

int StorageManager::deleteFiles(const char *type, const char *info)
{

	Mutex::Autolock _l(mDBLock);

	void *ptr;

	int count = 0;
	int ret = 0;
	String8 filter;
	String8 filter_type("WHERE TYPE ='");
	String8 tmp;
	String8 time;

	db_debug("delete files in /mnt/extsd/%s", type);
	if (strcmp(type, TYPE_PIC) == 0) {
		filter_type += type;
		filter_type += "'";
	} else {
		filter_type += type;
		filter_type += "' AND info ='";
		filter_type += info;
		filter_type += "'";
	}
	db_debug("%s %d, %s\n", __FUNCTION__, __LINE__, filter_type.string());
	mDBC->setFilter(filter_type);

	count = mDBC->getRecordCnt();
	db_debug("%s recordcnt:%d\n", __FUNCTION__, count);
	if (count <= 0) {
		/* check datebase */
		system("ls -l /data");
		system("df /data");
		return RET_IO_NO_RECORD;
	}

	mDBC->bufPrepare();
	do {
		if ((count--) == 0) {
			return RET_IO_NO_RECORD;
		}
		//system("ls -l /mnt/extsd/video");
		filter = filter_type;
		filter += " ORDER BY time ASC";
		mDBC->setFilter(filter);
		tmp = "file";
		ptr = mDBC->getFilterFirstElement(tmp, 0);
		if (!ptr) {
			db_debug("fail to read db");
			return RET_IO_DB_READ;
		}
		ret = deleteFile((const char*)ptr);
		if (ret < 0) {
			db_debug("fail to delete file %s", (const char *)ptr);
			return RET_IO_DEL_FILE;
		}
		db_debug("%s has been deleted\n", (char *)ptr);
		tmp = String8::format("DELETE FROM %s WHERE file='%s'", mDBC->getTableName().string(), (char *)ptr);
		db_debug("execute sql %s", tmp.string());
		mDBC->setSQL(tmp);
		mDBC->executeSQL();
		unsigned long long as  = availSize(MOUNT_PATH);
		if (as == 0) {
			db_msg("availSize is 0");
			break;
		}
		if (as > LOOP_COVERAGE_SIZE) {
			db_msg("availSize %lluMB > loop size %lluMB", as, LOOP_COVERAGE_SIZE);
			break;
		}
	} while(1);
	mDBC->bufRelease();

	return RET_IO_OK;
}

int StorageManager::deleteFiles()
{
	int ret = 0;

	ret = deleteFiles(TYPE_VIDEO, INFO_NORMAL);
	if (ret != RET_IO_OK) {
		ret = deleteFiles(TYPE_VIDEO, INFO_LOCK);
	}
	return ret;
}

int StorageManager::generateFile(time_t &now, String8 &file, int cam_type, fileType_t type, bool cyclic_flag, bool next_fd)
{
	int fd;
	int ret;
	struct tm *tm=NULL;
	char buf[128]= {0};
	const char *camType = FILE_FRONT;
	const char *suffix = FLAG_NONE;
	const char *fileType = EXT_VIDEO_MP4;
	const char *path = CON_2STRING(MOUNT_PATH, VIDEO_DIR);
	if(isMount(MOUNT_POINT)== false)
		return RET_NOT_MOUNT;
	db_msg("%s %d", __FUNCTION__, __LINE__);
	if((ret = needDeleteFiles())) {
		if((type == PHOTO_TYPE_NORMAL || type == PHOTO_TYPE_SNAP ) && (ret == RET_DISK_FULL))
			return RET_DISK_FULL;
		else {
			if (!cyclic_flag) {
				if (ret == RET_DISK_FULL){
					unsigned long long as = availSize(MOUNT_PATH);
					db_msg("    as:%lluMB  MIN_AVILIABLE_SPCE:%lluMB ",as,MIN_AVILIABLE_SPCE);
					if(as< MIN_AVILIABLE_SPCE){   //left space <50M can not startrecord
						return RET_DISK_FULL;
					}
				}
			} else {
				ret = deleteFiles();
				if(ret != RET_IO_OK && ret != RET_IO_NO_RECORD) {
					db_error("deleteFiles ret: RET_IO:%d\n", ret);
					return ret;
				}
			}
		}
	}

	db_msg("%s %d", __FUNCTION__, __LINE__);
	now = getDateTime(&tm);

	if(next_fd){
		time_t ptime = mktime(tm);
		ptime+=10;
		tm = localtime(&ptime);
		now = ptime;
	}
	db_msg("%s %d", __FUNCTION__, __LINE__);
	if (type == VIDEO_TYPE_IMPACT) {
		suffix = FLAG_SOS;
//		path = CON_2STRING(MOUNT_PATH, LOCK_DIR);
		path = CON_2STRING(MOUNT_PATH, LOCK_DIR);
	}
	if (type == PHOTO_TYPE_NORMAL || type == PHOTO_TYPE_SNAP) {
		fileType = EXT_PIC_JPG;
		path = CON_2STRING(MOUNT_PATH, PIC_DIR);
	}
	if(type == PHOTO_TYPE_SNAP)
		suffix=FLAG_SNAP;
	if (cam_type == CAM_UVC) {
		camType = FILE_BACK;
	}
	db_msg("--------***************------------");
	db_msg("%s %d", __FUNCTION__, __LINE__);
	sprintf(buf, "%s%04d%02d%02d_%02d%02d%02d%s%s%s",path,tm->tm_year + 1900, tm->tm_mon + 1,
		tm->tm_mday, tm->tm_hour,tm->tm_min, tm->tm_sec, camType, suffix, fileType);
	db_msg("--time=%s-----%s %d",buf, __FUNCTION__, __LINE__);
	file = buf;
//	fd = open(buf, O_RDWR | O_CREAT, 0666);
//	if (fd < 0) {
//		db_msg("failed to open file '%s'!!\n", buf);
//		return RET_IO_OPEN_FILE;
//	}
	db_debug("generate file %s", file.string());
	return RET_IO_OK;
}

int  StorageManager::readCpuInfo(char ret[]){
       char line[128];
       FILE *fp = fopen("/proc/cpuinfo", "r");
       if(fp==NULL){
               db_msg("open file failed.");
               return -1;
       }
       memset(ret,0,sizeof(ret));
       while(fgets(line, sizeof(line), fp)) {
               if (strncmp(line, "Serial", strlen("Serial")) == 0) {
                       char *str = strstr(line, ":");
                       str += 2;
               //      int num = atoi(str);
               //      printf("serial is %d\n", num);
	               if(strlen(str)>4){
	                       str +=(strlen(str)-5);
						   char* str1 = strstr(str, "\n");
						   if (str1 != NULL) {
						       *str1 = '\0';
						   }
	                       strcpy(ret,str);
	                       db_msg("str=%s,ret=%s",str,ret);
	               	}
               }
       }
       fclose(fp);
       return 0;

}


int StorageManager::geneImpactTimeFile(){
	int fd;
	int ret;
	struct tm *tm=NULL;
	char buf[IMPACT_TIME_TEXT_LENGTH]= {0};
	memset(buf,0,sizeof(buf));
	if(isMount()== false)
		return 0;
	db_msg("%s %d", __FUNCTION__, __LINE__);

	if (!access(GENE_IMPACT_TIME_FILE, F_OK)){
		db_msg("file in -----------");
	}
	fd = open(GENE_IMPACT_TIME_FILE, O_RDWR|O_APPEND | O_CREAT, 0666);
	if (fd < 0) {
		db_msg("failed to open file '%s'!!\n", buf);
		return 0;
	}
	getDateTime(&tm);

	sprintf(buf, "%04d/%02d/%02d %02d:%02d",tm->tm_year + 1900, tm->tm_mon + 1,
		tm->tm_mday, tm->tm_hour,tm->tm_min);
	db_msg("%s %d", __FUNCTION__, __LINE__);
	buf[16]='\n';
	ret = write(fd,buf,sizeof(buf));
	close(fd);
	if(ret<0){
		db_msg("write file error!, %s,%d",__FUNCTION__, __LINE__);
		return 0;
	}
	
	return 1;


}

int StorageManager::readImpactTimeFile(	char ret[]){
	int fd,size,count=0;
	char buf[IMPACT_TIME_TEXT_LENGTH];
	db_msg("****--------------------%s,%d***",__FUNCTION__,__LINE__);
	memset(buf,0,sizeof(buf));
	memset(ret,0,sizeof(ret));
    if (access(GENE_IMPACT_TIME_FILE, F_OK)) {
		db_msg("impct_time_file not found");
		return 0;
	}
	fd = open(GENE_IMPACT_TIME_FILE, O_RDONLY , 0666);
	if (fd < 0) {
		db_msg("open file false'%s'!!\n", buf);
		return 0;
	}
	do{
		if(count<10)
			count++;
		else
			break;
		size=read(fd,buf,sizeof(buf));
		db_msg("******buff=%s----size=%d",buf,size);
		if(size==0)
			break;
		strcat(ret,buf);
		memset(buf,0,sizeof(buf));
	}while(size>1);
	close(fd);
	if(size<0){
		db_msg("read file failed!");
		db_msg("%s,%d",__FUNCTION__,__LINE__);
		return 0;
	}
	return 1;
}

int StorageManager::doMount(const char *fsPath, const char *mountPoint,
		bool ro, bool remount, bool executable,
		int ownerUid, int ownerGid, int permMask, bool createLost) {
	int rc;
	unsigned long flags;
	char mountData[255];
	flags = MS_NODEV | MS_NOSUID | MS_DIRSYNC;
	flags |= (executable ? 0 : MS_NOEXEC);
	flags |= (ro ? MS_RDONLY : 0);
	flags |= (remount ? MS_REMOUNT : 0);

	sprintf(mountData,"utf8,uid=%d,gid=%d,fmask=%o,dmask=%o,shortname=mixed",
			ownerUid, ownerGid, permMask, permMask); 
	rc = mount(fsPath, mountPoint, "vfat", flags, mountData);
	if (rc && errno == EROFS) {
		db_error("%s mount fail try again", fsPath);
		rc = mount(fsPath, mountPoint, "vfat", flags, mountData);
		db_error(" mount rc=%d", rc);
	}

	if (rc == 0 && createLost) {
		char *lost_path;
		asprintf(&lost_path, "%s/LOST.DIR", mountPoint);
		if (access(lost_path, F_OK)) {
			if (mkdir(lost_path, 0755)) {
				db_error("Unable to create LOST.DIR (%s)", strerror(errno));
			}
		}
		free(lost_path);
	}
	return rc;
}

int StorageManager::format(CommonListener *listener)
{

#ifdef ASYNC_FORMAT
	mFT = new FormatThread(this, listener);
	status_t err = mFT->start();
	if (err != OK) {
		mFT.clear();
	}
#else
	format();
	dbReset();
//	listener->finish(0, 0);	
#endif
	return 0;
}

int StorageManager::format()
{
	int ret = -1;
	/*
	db_msg("format start\n");
	int retries = 5;
	if(isMount()== true) {
		while((ret = umount(MOUNT_PATH)) != 0) {
			usleep(500*1000);
			if (!(--retries)) { //retry 5 times
				break;
			}
		}
		if (retries == 0) {
			if((ret = umount(MOUNT_PATH)) != 0) { //the 6th time, total time is 3s
				usleep(500*1000);
				ALOGD("format umount ret=%d,try again\n",ret);
				Process::killProcessesWithOpenFiles(MOUNT_PATH, 2);
				if((ret = umount(MOUNT_PATH)) != 0) {
					ALOGV("format umount again fail ret=%d\n",ret);
					return -1;
				}
			}
		}
	}
	ret = system("newfs_msdos -F 32 -O android -b 65536 -c 128 /dev/block/vold/179:0");
	if(ret < 0) {
		db_error("execute command failed: %s\n", strerror(errno));
		return -1;
	} else {
        if (WIFEXITED(ret)) { //success to exit
            if (0 == WEXITSTATUS(ret)) { //success to execute command
                db_msg("run shell script successfully.\n");
            } else { //fail
                db_error("run shell script fail, script exit code: %d\n", WEXITSTATUS(ret));
				return -1;
            }
        }  else { //exit error
            db_error("exit status = [%d]\n", WEXITSTATUS(ret));
			return -1;
        }
    }
	ret = doMount("/dev/block/vold/179:0", MOUNT_PATH, false, false, true, 0, 0, 0, true);
	if(ret != 0){
		ALOGV("__doMount erro ret=%d,error=%s\n",ret,strerror(errno));
		return -1;
	}
	db_msg("format end ret=%d\n",ret);
	*/
#ifdef FATFS
	fat_mkfs("0");
#else
	if (mStorageMonitor) {
		return mStorageMonitor->formatSDcard();
    }
#endif
	checkDirs();
	dbReset();
	return ret;
}

long long totalSize(const char *path)
{
	unsigned long long totalSize = 0;
#ifdef FATFS
    FAT_STATFS  diskinfo;
	int res;
	res = fat_statfs(MOUNT_POINT, &diskinfo);
	db_msg("[app] res: %d\n", res);
	if (res >= 0) {
		db_msg("[app] f_bsize: %d, f_blocks: %llu!!!, f_bfree: %llu\n", diskinfo.f_bsize, (long long)diskinfo.f_blocks, diskinfo.f_bfree);
		totalSize = diskinfo.f_blocks * diskinfo.f_bsize;
		totalSize = totalSize >> 20;
		if (totalSize > MAX_CARD_SIZE_MB) {
			totalSize = 0;
		}
		return totalSize; //MB
	}
#else
    struct statfs diskinfo;
	if(statfs(path, &diskinfo) != -1){
		totalSize = diskinfo.f_blocks * diskinfo.f_bsize;
		totalSize = totalSize >> 20;
		if (totalSize > MAX_CARD_SIZE_MB) {
			totalSize = 0;
		}
		return totalSize; //MB
	}
#endif
	return 0;
}

long long availSize(const char *path)
{
	unsigned long long availSize = 0;
#ifdef FATFS
    FAT_STATFS  diskinfo;
	int res;
	res = fat_statfs(MOUNT_POINT, &diskinfo);
	db_msg("[app] res: %d\n", res);
	if (res >= 0) {
		db_msg("[app] f_bsize: %d, f_blocks: %llu, f_bfree: %llu\n", diskinfo.f_bsize, diskinfo.f_blocks, diskinfo.f_bfree);
		availSize = diskinfo.f_bfree * diskinfo.f_bsize;
		db_msg("availSize:%llu", availSize);
		availSize = availSize >> 20;
		if (availSize > MAX_CARD_SIZE_MB) {
			availSize = 0;
		}
		return availSize; //MB
	}
#else
    struct statfs diskinfo;
	if(statfs(path, &diskinfo) != -1){
		availSize = diskinfo.f_bfree * diskinfo.f_bsize;
		availSize = availSize >> 20;
		if (availSize > MAX_CARD_SIZE_MB) {
			availSize = 0;
		}
		return availSize; //MB
	}
#endif
	return 0;
}

int getFiles(char *dir, char *filetype, Vector<String8> &files)
{
	String8 file(dir);
#ifdef FATFS
	int dir_fd = -1;
	FAT_FILINFO Finfo;
	int res = -1;
	char *fn = NULL;
	if ((dir_fd = fat_opendir(dir)) >= 0) {
		while ((res = fat_readdir(dir_fd, &Finfo)) == 0)  {
			if (Finfo.fname[0] == '.')
				continue;
			if(Finfo.fname[0] == 0)
				break;
			fn = *Finfo.lfname? Finfo.lfname: Finfo.fname;
			file = dir;
			file += "/";
			file += fn;
			if (Finfo.fattrib & AM_DIR) { //dir

			} else {	//file
				String8 ext = file.getPathExtension();
				if(ext.isEmpty()) {
					continue;
				}
				if(strcasecmp(ext.string(), filetype) == 0) {
					files.add(file);
				} else {
					db_debug("file is not type:%s", filetype);
				}
			}
		}
	}
	fat_closedir(dir_fd);
	return 0;
#else
	DIR *p_dir;
	struct stat stat;
	struct dirent *p_dirent;
	p_dir = opendir(dir);
	if (p_dir == NULL) {
		db_error("fail to opendir %s", dir);
		return -1;
	}
	while( (p_dirent = readdir(p_dir)) ) {
		if (strcmp(p_dirent->d_name, ".") == 0 ||
				strcmp(p_dirent->d_name, "..") == 0) {
			continue;
		}
		file = dir;
		file += p_dirent->d_name;
		if(lstat(file.string(), &stat) == -1) {
			db_error("fail to get file stat %s", file.string());
			continue;
		}
		if (!S_ISREG(stat.st_mode)) {
			db_error("%s is not a regular file", file.string());
			continue;
		}
		String8 ext = file.getPathExtension();
		if(ext.isEmpty()) {
			continue;
		}
		if(strcasecmp(ext.string(), filetype) == 0) {
			files.add(file);
		} else {
			db_debug("file is not type:%s", filetype);
		}
	}
	closedir(p_dir);
#endif
	return 0;
}

void StorageManager::dbReset()
{
	mDBInitL.lock();
	if (mDBInited == true) {
		db_debug("%s %d %p\n", __FUNCTION__, __LINE__, mDBC);
		mDBC->deleteTable(String8(CDR_TABLE));
		mDBInited = false;
	}
	mDBInitL.unlock();
	if (mUpdateDbTh != NULL)
		mUpdateDbTh->start();
}

int StorageManager::dbInit()
{
	int ret = 0;
	if (mDBInited) {
		return 0;
	}
	if (mDBC == NULL) {
		system("rm /data/sunxi.db");
		mDBC = new DBCtrl();
		db_debug("%s %d %p\n", __FUNCTION__, __LINE__, mDBC);
	}
	mDBC->createTable(String8(CDR_TABLE), gSqlTable, ARRYSIZE(gSqlTable));	
	mDBC->setColumnType(gDBType, 0);
	ret = dbUpdate();
	if (ret < 0) {
		return ret;
	}
	mDBInited = true;
	return ret;
}

time_t parase_time(char *ptr1)
{
	char *ptr;
	char chr;
	int value;
	struct tm my_tm;
	time_t my_time_t;
	/*
	   ptr = strchr(filename, '.');
	   if(ptr == NULL) {
	   printf("not found the delim\n");
	   return 0;
	   }
	   db_msg("filename is %s\n", filename);
	   ptr1 = strndup(filename, ptr - filename - 1);
	   */
	memset(&my_tm, 0, sizeof(my_tm));
	if (strlen(ptr1) >= strlen("20141201_121212")) {
	/************ year ************/
	ptr = ptr1;
	chr = ptr[4];
	ptr[4] = '\0';
	value = atoi(ptr);
	my_tm.tm_year = value - 1900;
	ptr[4] = chr;

	/************ month ************/
	ptr = ptr + 4;
	chr = ptr[2]; 
	ptr[2] = '\0';
	value = atoi(ptr);
	my_tm.tm_mon = value - 1;
	ptr[2] = chr;

	/************ day ************/
	ptr = ptr + 2;
	chr = ptr[2]; 
	ptr[2] = '\0';
	value = atoi(ptr);
	my_tm.tm_mday = value;

	/************ hour ************/
	ptr += 3;
	chr = ptr[2]; 
	ptr[2] = '\0';
	value = atoi(ptr);
	my_tm.tm_hour = value;
	ptr[2] = chr;

	/************ minite ************/
	ptr += 2;
	chr = ptr[2]; 
	ptr[2] = '\0';
	value = atoi(ptr);
	my_tm.tm_min = value;
	ptr[2] = chr;

	/************ second ************/
	ptr += 2;
	chr = ptr[2]; 
	ptr[2] = '\0';
	value = atoi(ptr);
	my_tm.tm_sec = value;
	} else {
		my_tm.tm_year = 2014;
		my_tm.tm_mon  = 12;
		my_tm.tm_mday = 1;
		my_tm.tm_hour = 1;
		my_tm.tm_min  = 1;
		my_tm.tm_sec  = 1;
	}
//	db_msg("year: %d, month: %d, day: %d, hour: %d, minute: %d, second: %d\n", 
//			my_tm.tm_year, 
//			my_tm.tm_mon, 
//			my_tm.tm_mday, 
//			my_tm.tm_hour, 
//			my_tm.tm_min, 
//			my_tm.tm_sec);
	my_time_t = mktime(&my_tm);
//	db_msg("time_t is %lu, time str is %s\n", my_time_t, ctime(&my_time_t));
	return my_time_t;
}

unsigned long StorageManager::fileSize(String8 path)
{
#ifdef FATFS
	FAT_FILINFO Finfo;

	int res = fat_stat(path.string(), &Finfo);
	if (res < 0 || Finfo.fsize == 0) {
		db_msg("fail to get %s'size, size is %lu", path.string(), Finfo.fsize);
		Finfo.fsize = 0;
	}
	return Finfo.fsize;
#else
	struct stat	buf; 
	stat(path.string(),&buf);
	if (buf.st_size == 0) {
		db_msg("%s size is 0", path.string());
	}
	return buf.st_size;
#endif
}

int StorageManager::dbUpdate(char *type)
{
	Vector<String8> files_db, files;
	String8 file, file_db, tmp, stime;
	//	Vector<String8>::iterator result;
	Elem elem;
	time_t time;
	int size, size_db, i, ret=0;
	char *ext = NULL;
	char *video_dir = (char*)CON_2STRING(MOUNT_PATH, VIDEO_DIR);
	char *lock_dir  = (char*)CON_2STRING(MOUNT_PATH, LOCK_DIR);
	char *pic_dir   = (char*)CON_2STRING(MOUNT_PATH, PIC_DIR);
#ifdef FATFS
	video_dir = (char*)CON_2STRING(MOUNT_PATH, TYPE_VIDEO);
	lock_dir  = (char*)CON_2STRING(MOUNT_PATH, TYPE_LOCK);
	pic_dir   = (char*)CON_2STRING(MOUNT_PATH, TYPE_PIC);
#endif
	if (0 == strcmp(type, TYPE_VIDEO)) {
		ext = EXT_VIDEO_MP4;
		ret = getFiles(video_dir, (char*)ext, files);
		ret = getFiles(lock_dir, (char*)ext, files);
	} else if (0 == strcmp(type, TYPE_PIC)) {
		ext = EXT_PIC_JPG;
		ret = getFiles(pic_dir, (char*)ext, files);
	}
	/*
	dbGetFiles(type, files_db);
	size_db = files_db.size();
	for(i=0; i<size_db; i++) {
		file_db = files_db.itemAt(i);
		if (!FILE_EXSIST(file_db.string())) {	//File is in the database but does not exist
			db_msg("has not found %s, delete from database", file_db.string());
			dbDelFile(file_db.string());	//delete file record in the database			
			ret = deleteFile(file_db.string());
			if (ret < 0) {
				db_debug("fail to delete file %s", (const char *)file_db.string());
				return RET_IO_DEL_FILE;
			}
		} else {
			db_msg("has found %s in the tfcard", file_db.string());
		}
	}
	*/
	size = files.size();
	for(i=0; i<size; i++)
	{
		if (mStopDBUpdate) {
			return 0;
		}
		//isValidVideoFile(files.itemAt(i));
		file = files.itemAt(i);
		if (!dbIsFileExist(file)) {	//file exsit but there is no record in the database
			if (fileSize(file) == 0) {	//file size is 0
				deleteFile(file.string());
				continue;
			}
			time = parase_time((char*)file.getBasePath().getPathLeaf().string());
			elem.file = (char *)file.string();
			elem.time = (long unsigned int)time;
			elem.type = type;
			elem.info = (char*)INFO_NORMAL;
			if (0 == strcmp(type, TYPE_VIDEO)) {
				if(strstr(file.string(), FLAG_SOS) != NULL) {
					db_msg("file type is %s\n", FLAG_SOS);
					elem.info = (char*)INFO_LOCK;
				}
			}
			ret = dbAddFile(&elem);	//new file insert to the database
			if (ret < 0) {
				db_error("xxxx need to reset database xxx");
				delete mDBC;
				mDBC = NULL;
				return -1;
			}
		}
//		db_msg("file %s exists in the database", files.itemAt(i).string());
	}
	return 0;
}

int StorageManager::dbUpdate()
{
//	db_msg("------dbUpate");
	int ret = 0;
	if(isMount()== false) {
		db_error("not mount\n");
		return -1;
	}
	mStopDBUpdate = false;
	ret = dbUpdate((char*)TYPE_VIDEO);
	if (ret >= 0) {
		ret = dbUpdate((char*)TYPE_PIC);
	}
	return ret;
}


int StorageManager::getSoS_fileCapitly()
{
	Vector<String8> files;
	String8 file;
	int size, i, ret=0;
	unsigned long int file_size=0;
	unsigned long int tmp,file_size_total=0;
	const char *ext = NULL;
	ext = EXT_VIDEO_MP4;
	char *lock_dir = (char*)CON_2STRING(MOUNT_PATH, LOCK_DIR);
	#ifdef FATFS
		lock_dir = (char*)CON_2STRING(MOUNT_PATH, TYPE_LOCK);
	#endif
	ret = getFiles(lock_dir, (char*)ext, files);
	size = files.size();
	db_msg("--------[debug_jason]:the file cont is :%d------------\n",size);
	for (i= 0; i<size;i++)
	{
		file = files.itemAt(i);
		db_msg("----the file name is :%s---------\n",(char *)file.string());
		if(strstr(file.string(), FLAG_SOS) != NULL) {
			file_size = fileSize(file);
			tmp = file_size;
			file_size_total += tmp;
			db_msg("file type is %s\n", FLAG_SOS);
			db_msg("----the file size is :%ld---------\n",file_size);
			db_msg("----the file file_size_total is :%ld---------\n",file_size_total);
		}
	}
	return file_size_total;
	
}

void StorageManager::dbExit()
{
	if (mDBC) {
		delete mDBC;
		mDBC = NULL;
	}
}

/* create dirs
 * /mnt/extsd/video
 * /mnt/extsd/photo
 * /mnt/extsd/video/.thumb
 * /mnt/extsd/photo/.thumb
 */

int _mkdirs(const char *dir)
{
	int ret = 0;
#ifdef FATFS
	int dir_fd = -1;
	if ((dir_fd = fat_opendir(dir)) >= 0) {
		db_msg("success to open %s",dir);
		fat_closedir(dir_fd);
	} else {
		db_msg("fail to open %s, now create",dir);
		if (fat_mkdir(dir) < 0) {
			ret = -1;
			db_msg("fail to mkdir %s, error:%s", dir, strerror(errno));
		}
	}
#endif
	return ret;
}

int StorageManager::checkDirs()
{
	int retval = 0;

#ifdef FATFS
	retval = _mkdirs(CON_2STRING(MOUNT_PATH, TYPE_VIDEO));
	retval = _mkdirs(CON_2STRING(MOUNT_PATH, TYPE_PIC));
	retval = _mkdirs(CON_3STRING(MOUNT_PATH, VIDEO_DIR, TYPE_THUMB));
	retval = _mkdirs(CON_3STRING(MOUNT_PATH, PIC_DIR, TYPE_THUMB));
	retval = _mkdirs(CON_2STRING(MOUNT_PATH, TYPE_LOCK));
#else
	if(access(CON_2STRING(MOUNT_PATH, VIDEO_DIR), F_OK) != 0) {
		if(mkdir(CON_2STRING(MOUNT_PATH, VIDEO_DIR), 0777) == -1) {
			db_msg("mkdir failed: %s\n", strerror(errno));
			retval = -1;
		}
	}
	if(access(CON_2STRING(MOUNT_PATH, PIC_DIR), F_OK) != 0) {

		if(mkdir(CON_2STRING(MOUNT_PATH, PIC_DIR), 0777) == -1) {
			db_msg("mkdir failed: %s\n", strerror(errno));
			retval = -1;
		}
	}
	if(access(CON_3STRING(MOUNT_PATH, VIDEO_DIR, THUMBNAIL_DIR), F_OK) != 0) {

			if(mkdir(CON_3STRING(MOUNT_PATH, VIDEO_DIR, THUMBNAIL_DIR), 0777) == -1) {
			db_msg("mkdir failed: %s\n", strerror(errno));
			retval = -1;
		}
	}
	if(access(CON_3STRING(MOUNT_PATH, PIC_DIR, THUMBNAIL_DIR), F_OK) != 0) {

		if(mkdir(CON_3STRING(MOUNT_PATH, PIC_DIR, THUMBNAIL_DIR), 0777) == -1) {
			db_msg("mkdir failed: %s\n", strerror(errno));
			retval = -1;
		}
	}
	if(access(CON_2STRING(MOUNT_PATH, LOCK_DIR), F_OK) != 0) {
		if(mkdir(CON_2STRING(MOUNT_PATH, LOCK_DIR), 0777) == -1) {
		db_msg("mkdir failed: %s\n", strerror(errno));
                         retval = -1;
    	}
	}

#endif
	return retval;
}

inline void addSingleQuotes(String8 &str)
{
	String8 tmp("'");
	tmp += str;
	tmp += ("'");
	str = tmp;
}

bool StorageManager::dbIsFileExist(String8 file)
{
	String8 filter("where file=");
	int cnt = 0;
	addSingleQuotes(file);
	filter += file;
	Mutex::Autolock _l(mDBLock);
	mDBC->setFilter(filter);
	cnt = mDBC->getRecordCnt();
	return (cnt>0)?true:false;
}

/* Count item, such as file, time, type, info etc.*/
int StorageManager::dbCount(char *item)
{
	Mutex::Autolock _l(mDBLock);
	String8 str("where type=");
	String8 item_str(item);
	addSingleQuotes(item_str);
	mDBC->setFilter(str+item_str);
	return mDBC->getRecordCnt();
}

void StorageManager::dbGetFiles(char *item, Vector<String8>&file)
{
	int total = dbCount(item);
	int i = 0;
	char *ptr = NULL;
	Mutex::Autolock _l(mDBLock);
	String8 str("where type=");
	String8 item_str(item);
	addSingleQuotes(item_str);
	mDBC->setFilter(str+item_str);
	mDBC->bufPrepare();
	while(i < total) {
		ptr = (char *)mDBC->getElement(String8("file"), 0, i);
		if (ptr) {
			file.add(String8(ptr));
		}
		i++;
	}
	mDBC->bufRelease();
}

void StorageManager::dbGetFile(char* item, String8 &file)
{
	char *ptr = NULL;
	Mutex::Autolock _l(mDBLock);
	String8 str("where type=");
	String8 item_str(item);
	addSingleQuotes(item_str);
	mDBC->setFilter(str+item_str);
	mDBC->bufPrepare();
	ptr = (char *)mDBC->getElement(String8("file"), 0, 0);
	if (ptr) {
		file = String8(ptr);
	}
	mDBC->bufRelease();
}

int StorageManager::deleteRecentlyFile()
{
	char *ptr;
	int total, retval;
	retval = dbCount((char*)TYPE_VIDEO);
	if(retval < 0) {
		db_error("get db count failed\n");
		return -1;
	}
	total = retval;
	retval = dbCount((char*)TYPE_PIC);
	if(retval < 0) {
		db_error("get db count failed\n");
		return -1;
	}
	total += retval;
	if(total<0){
		db_msg("no files to delete");
		return -1;
	}
	Mutex::Autolock _l(mDBLock);
	mDBC->bufPrepare();
	ptr = (char *)mDBC->getElement(String8("file"), 0, total-1);
	mDBLock.unlock();
	dbDelFile(ptr);
	deleteFile(ptr);
	mDBC->bufRelease();
	return 0;
}

int StorageManager::dbGetSortByTimeRecord(dbRecord_t &record, int index)
{
	char *ptr = NULL;
	int total, retval;

	retval = dbCount((char*)TYPE_VIDEO);
	if(retval < 0) {
		db_error("get db count failed\n");
		return -1;
	}
	total = retval;

	retval = dbCount((char*)TYPE_PIC);
	if(retval < 0) {
		db_error("get db count failed\n");
		return -1;
	}
	total += retval;

	Mutex::Autolock _l(mDBLock);
	if(index >= total) {
		db_error("invalid index: %d, total is %d\n", index, total);
		return -1;
	}
	mDBC->setFilter(String8("ORDER BY time ASC") );
	mDBC->bufPrepare();
	ptr = (char *)mDBC->getElement(String8("file"), 0, index);
	if (ptr) {
		record.file = String8(ptr);
	}
	ptr = (char *)mDBC->getElement(String8("type"), 0, index);
	if (ptr) {
		record.type = String8(ptr);
	}
	ptr = (char *)mDBC->getElement(String8("info"), 0, index);
	if (ptr) {
		record.info = String8(ptr);
	}
	mDBC->bufRelease();

	return 0;
}
int StorageManager::dbAddFile(Elem *elem)
{
	char str[128];
	int ret;
	sprintf(str, "INSERT INTO %s VALUES('%s',%lu,'%s','%s')",mDBC->getTableName().string(),
			elem->file,elem->time,elem->type,elem->info);
//	db_msg("sql is %s\n", str);
	Mutex::Autolock _l(mDBLock);
	mDBC->setSQL(String8(str));
	ret = mDBC->executeSQL();
	if (ret != SQLITE_OK) {
		db_msg("error to add file:%d\n", ret);
		return -1;
	}
	return 0;
}

int StorageManager::dbDelFile(char const *file)
{
	Mutex::Autolock _l(mDBLock);
	char str[128];
	int ret = 0;
	
	sprintf(str, "DELETE from %s WHERE file='%s'",mDBC->getTableName().string(), file);
	mDBC->setSQL(String8(str));
	ret = mDBC->executeSQL();
	return ret;
}

void StorageManager::dbUpdateFile(const char* oldname, const char *newname)
{
	Mutex::Autolock _l(mDBLock);
	const String8 sql(String8::format("UPDATE %s SET %s = '%s' WHERE %s = '%s' ",
		mDBC->getTableName().string(), gSqlTable[0].item, newname, gSqlTable[0].item, oldname));
	mDBC->setSQL(sql);
	mDBC->executeSQL();
}


void StorageManager::dbUpdateFileType(const char* name, const char *type)
{
	Mutex::Autolock _l(mDBLock);
	const String8 sql(String8::format("UPDATE %s SET %s = '%s' WHERE %s = '%s' ",
		mDBC->getTableName().string(), gSqlTable[3].item, type, gSqlTable[0].item, name));
	mDBC->setSQL(sql);
	mDBC->executeSQL();
}

void StorageManager::setFileInfo(const char* oldName, bool isLock1 ,String8& nfile)
{
	db_msg(">>>>>>>>>>>>oldname:%s ,isLock: %d  ",oldName ,isLock1 );
	char fileName[40]={0};
	String8 file(oldName) ;
	int ret=0 ;

	if(isLock1){
		file = file.getBasePath();
		file += FLAG_SOS;
		file += EXT_VIDEO_MP4;
		db_msg("lockFile:%s --> %s", oldName, (char*)file.string());

		String8 newfile = String8(CON_2STRING(MOUNT_PATH,LOCK_DIR))+file.getPathLeaf();
		nfile = newfile;

		dbUpdateFile(oldName ,(char*)newfile.string());
		dbUpdateFileType((char*)newfile.string(),(char*)INFO_LOCK );
		reNameFile(oldName , newfile.string());
	}else{
		file = file.getBasePath();
		ret = strlen(file.string());
		strncpy(fileName ,(char*)file.string() ,ret-strlen("_SOS"));//"_SOS" 4λ
		if( !strncmp(EXT_VIDEO_MP4 ,".mov",4)){
			fileName[ret-4]='.';
			fileName[ret-3]='m';
			fileName[ret-2]='o';
			fileName[ret-1]='v';
			fileName[ret-0]= 0 ;
		}else{
		fileName[ret-4]='.';
		fileName[ret-3]='m';
		fileName[ret-2]='p';
		fileName[ret-1]='4';
		fileName[ret-0]= 0 ;
		}

		char NewFile[40]={0};
		strncpy(NewFile,(char*)file.string(),strlen(MOUNT_PATH));
		strcat(NewFile, TYPE_VIDEO);
		strcat(NewFile, fileName+strlen(MOUNT_PATH)+strlen(TYPE_LOCK));
		db_msg("unlockFile:%s --> %s", oldName, NewFile);	
		dbUpdateFile(oldName ,(char*)NewFile);
		dbUpdateFileType((char*)NewFile,(char*)INFO_NORMAL);
		reNameFile(oldName , (char*)NewFile);
		nfile = String8(NewFile);
	}
	db_msg(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>lock or unlock  ok! NewFile:%s",nfile.string());
}

int StorageManager::saveVideoFrameToBmpFile(VideoFrame* videoFrame, const char* bmpFile)
{
	unsigned int width, height, size;
	unsigned char* data;
	MYBITMAP myBitmap;
	RGB pal[256] = {{0,0,0,0}};
	int Bpp;	/* Bytes per pixel */
	int depth;
	data = (unsigned char*)videoFrame + sizeof(VideoFrame);
	width = videoFrame->mWidth;
	height = videoFrame->mHeight;
	size = videoFrame->mSize;
	Bpp = size / (width * height);
	depth = Bpp * 8;
	db_msg("Bpp is %d\n", Bpp);
	db_msg("depth is %d\n", depth);
	myBitmap.flags  = MYBMP_TYPE_NORMAL;
	myBitmap.frames = 1;
	myBitmap.depth  = depth;
	myBitmap.w      = width;
	myBitmap.h      = height;
	myBitmap.pitch  = width * Bpp;
	myBitmap.size   = size;
	myBitmap.bits   = data;
#ifdef FATFS
	return SaveBmpToFatFs(&myBitmap, pal, bmpFile);
#else
	return SaveMyBitmapToFile(&myBitmap, pal, bmpFile);
#endif
}

int StorageManager::savePicture(void* data, int size, String8 &file)
{
	int bytes_write = 0;
	int bytes_to_write = size;
	void *ptr = data;

#ifdef FATFS
	int fd = fat_open(file.string(), FA_WRITE|FA_CREATE_NEW);
#else
	int fd = open(file.string(), O_RDWR | O_CREAT, 0666);
#endif
	if (fd < 0) {
		db_msg("failed to open file '%s'!!\n", file.string());
		return RET_IO_OPEN_FILE;
	}
#ifdef FATFS
	while((bytes_write = fat_write(fd, ptr, bytes_to_write)))
#else
	while((bytes_write = write(fd, ptr, bytes_to_write)))
#endif
	{
		if((bytes_write == -1) && (errno != EINTR)) {
			db_msg("wirte error: %s\n", strerror(errno));
			break;
		} else if(bytes_write == bytes_to_write) {
			break;
		} else if( bytes_write > 0 ) {
			ptr = (void *)((char *)ptr + bytes_write);
			bytes_to_write -= bytes_write;
		}
	}
	if(bytes_write == -1) {
		db_error("save picture failed: %s\n", strerror(errno));
		return -1;
	}
#ifdef FATFS
	fat_sync(fd);
	fat_close(fd);
#else
	fsync(fd);
	close(fd);
#endif
	return 0;
}

bool StorageManager::isDiskFull(void)
{
	if(needDeleteFiles() == RET_DISK_FULL)
		return true;
	return false;
}

void StorageManager::setEventListener(EventListener* pListener)
{
	mListener = pListener;
}

StorageManager::StorageSyncThread::StorageSyncThread(StorageManager *sm, bool loop)
{
	mSM = sm;
#if 0
	system("echo 1 > /proc/sys/vm/dirty_ratio");
	system("echo 1 > /proc/sys/vm/dirty_background_ratio");
	system("echo 100 > /proc/sys/vm/dirty_writeback_centisecs");
	system("echo 1500 > /proc/sys/vm/dirty_expire_centisecs");
	system("echo 500 > /proc/sys/vm/vfs_cache_pressure");
#endif
	mLoop = loop;
}

void StorageManager::storageSync()
{
	system("sync "STORAGE_POINT);
}

void StorageManager::StorageSyncThread::storageSyncLoop()
{
	while(mLoop) {
		mSM->storageSync();
		usleep(3*1000*1000);
	}
	mSM->storageSync();
}

void StorageManager::startSyncThread(bool loop)
{
#ifdef SYNC_THREAD
	if (mSST == NULL) {
		mSST = new StorageSyncThread(this, loop);
	}
	mSST->start();
	db_msg("!!mSST is not null");
#endif
}

void StorageManager::stopSyncThread()
{
#ifdef SYNC_THREAD
	if (mSST != NULL) {
		mSST->stopThread();
		mSST.clear();
		mSST = NULL;
		return ;
	}
	db_msg("!!mSST is null");
#else
	storageSync();
#endif
}


#ifdef FATFS

const char*
get_extension (const char* filename)
{
    const char* ext;
    ext = strrchr (filename, '.');
    if (ext)
        return ext + 1;
    return NULL;
}

int getMemFromFatFile(const char *file, char **data, int *size)
{
	int ret = -1;
	int lSize = 0;
	char *buffer = NULL;
	FAT_FILINFO FInfo;
	int read_fd = fat_open(file, FA_READ);
	if (read_fd < 0) {
		db_msg("fail to open file %s", file);
		return -1;
	}
	fat_stat (file, &FInfo);
	lSize = FInfo.fsize;
    buffer = (char*) malloc (sizeof(char)*lSize);
	char *tmp = buffer;
    if (buffer == NULL)
    {
        db_msg ("Memory error");
		return -1;
    }
	while(1) {
	    ret = fat_read (read_fd,tmp,lSize);
	    if (ret == FAT_BUFFER_SIZE)
	    {
			tmp += FAT_BUFFER_SIZE;
	        continue;
	    } else if (ret < 0) {
	        db_msg ("Reading error, ret %d", ret);
			return -1;
		}
		break;
	}
	fat_close(read_fd);
	*size = lSize;
	*data = buffer;
    return 0;
}

int releaseMemFromFatFile(char **buffer)
{
	if (*buffer) {
		free(*buffer);
		*buffer = NULL;
	}
	return 0;
}

inline static int depth2Bpp (int depth)
{
    switch (depth) {
    case 4:
    case 8:
        return 1;
    case 15:
    case 16:
        return 2;
    case 24:
        return 3;
    case 32:
        return 4;
    }

    return 1;
}

FatBuffer::FatBuffer(int alignSize)
{
	mUnitSize = alignSize;
	mBuf = (char *)malloc(sizeof(char) * mUnitSize);
	memset(mBuf, 0, sizeof(char)*mUnitSize);
	mWriteSize = 0;
	mTotalSize = mUnitSize;
}


int FatBuffer::write(const void *ptr, int size, int num)
{
	int acquireSize = size * num;
	if (acquireSize + mWriteSize > mTotalSize) {
		mBuf = (char *)realloc(mBuf, mTotalSize + mUnitSize);
		if (mBuf == NULL) {
			return -1;
		}
		mTotalSize += mUnitSize;
	}
	memcpy(mBuf + mWriteSize, ptr, acquireSize);
	mWriteSize += acquireSize;
	return acquireSize;
}

FatBuffer::~FatBuffer()
{
	free(mBuf);
}

void FatBuffer::syncFile(int fd)
{
	int off = 0;
	while(mWriteSize > mUnitSize)  {
		fat_write(fd, mBuf + off, mUnitSize);
		mWriteSize -= mUnitSize;
		off += mUnitSize;
	}
	fat_write(fd, mBuf+off, mWriteSize);
}

static int RWrite(int fd, const void *ptr, int size, int num)
{
	return fat_write(fd, ptr, size*num);
}
inline void pixel2rgb (unsigned int pixel, abgrColor* color, int depth)
{
    switch (depth) {
    case 24:
    case 32:
        color->r = (unsigned char) ((pixel >> 16) & 0xFF);
        color->g = (unsigned char) ((pixel >> 8) & 0xFF);
        color->b = (unsigned char) (pixel & 0xFF);
        break;

    case 15:
        color->r = (unsigned char)((pixel & 0x7C00) >> 7) | 0x07;
        color->g = (unsigned char)((pixel & 0x03E0) >> 2) | 0x07;
        color->b = (unsigned char)((pixel & 0x001F) << 3) | 0x07;
        break;

    case 16:
        color->r = (unsigned char)((pixel & 0xF800) >> 8) | 0x07;
        color->g = (unsigned char)((pixel & 0x07E0) >> 3) | 0x03;
        color->b = (unsigned char)((pixel & 0x001F) << 3) | 0x07;
        break;
    }
}

static void bmpGetHighCScanline (BYTE* bits, BYTE* scanline,
                        int pixels, int bpp, int depth)
{
    int i;
    unsigned int c;
    abgrColor color;
    memset (&color, 0, sizeof(abgrColor));

    for (i = 0; i < pixels; i++) {
        c = *((unsigned int*)bits);

        pixel2rgb (c, &color, depth);
        *(scanline)     = color.b;
        *(scanline + 1) = color.g;
        *(scanline + 2) = color.r;

        bits += bpp;
        scanline += 3;
    }
}

static int __fat_save_bmp (int fd, MYBITMAP* bmp, RGB* pal)
{
    BYTE* scanline = NULL;
    int i, bpp;
    int scanlinebytes;
	FatBuffer *fat_buf = new FatBuffer(FAT_BUFFER_SIZE);	//it must use 64kb unit to read/write on fatfs to accelerate.
	int write_size = 0;
    BITMAPFILEHEADER bmfh;
    WINBMPINFOHEADER bmih;
    memset (&bmfh, 0, sizeof (BITMAPFILEHEADER));
    bmfh.bfType         = MAKEWORD ('B', 'M');
    bmfh.bfReserved1    = 0;
    bmfh.bfReserved2    = 0;
    memset (&bmih, 0, sizeof (WINBMPINFOHEADER));
    bmih.biSize         = (DWORD)(WININFOHEADERSIZE);
    bmih.biWidth        = (DWORD)(bmp->w);
    bmih.biHeight       = (DWORD)(bmp->h);
    bmih.biPlanes       = 1;
    bmih.biCompression  = BI_RGB;
	bpp = depth2Bpp (bmp->depth);
	//depth 32
    scanlinebytes       = bmih.biWidth*3;
    scanlinebytes       = ((scanlinebytes + 3)>>2)<<2;
    if (!(scanline = (BYTE*)malloc (scanlinebytes))) return ERR_BMP_MEM;
    memset (scanline, 0, scanlinebytes);
    bmih.biSizeImage    = bmih.biHeight*scanlinebytes;
    bmfh.bfOffBits      = SIZEOF_BMFH + SIZEOF_BMIH;
    bmfh.bfSize         = bmfh.bfOffBits + bmih.biSizeImage;
    bmih.biBitCount     = 24;

#if 0
    RWrite (fd, &bmfh.bfType, sizeof (WORD), 1);
    RWrite (fd, &bmfh.bfSize, sizeof (DWORD), 1);
    RWrite (fd, &bmfh.bfReserved1, sizeof (WORD), 1);
    RWrite (fd, &bmfh.bfReserved2, sizeof (WORD), 1);
    RWrite (fd, &bmfh.bfOffBits, sizeof (DWORD), 1);

    RWrite (fd, &bmih.biSize, sizeof (DWORD), 1);
    RWrite (fd, &bmih.biWidth, sizeof (LONG), 1);
    RWrite (fd, &bmih.biHeight, sizeof (LONG), 1);
    RWrite (fd, &bmih.biPlanes, sizeof (WORD), 1);
    RWrite (fd, &bmih.biBitCount, sizeof (WORD), 1);
    RWrite (fd, &bmih.biCompression, sizeof (DWORD), 1);
    RWrite (fd, &bmih.biSizeImage, sizeof (DWORD), 1);
    RWrite (fd, &bmih.biXPelsPerMeter, sizeof (LONG), 1);
    RWrite (fd, &bmih.biYPelsPerMeter, sizeof (LONG), 1);
    RWrite (fd, &bmih.biClrUsed, sizeof (DWORD), 1);
    RWrite (fd, &bmih.biClrImportant, sizeof (DWORD), 1);
    for (i = bmp->h - 1; i >= 0; i--) {
        bmpGetHighCScanline (bmp->bits + i * bmp->pitch, scanline,
                        bmp->w, bpp, bmp->depth);
        RWrite (fd, scanline, sizeof (char), scanlinebytes);
    }
#else
	fat_buf->write (&bmfh.bfType, sizeof (WORD), 1);
	fat_buf->write (&bmfh.bfSize, sizeof (DWORD), 1);
    fat_buf->write (&bmfh.bfReserved1, sizeof (WORD), 1);
    fat_buf->write (&bmfh.bfReserved2, sizeof (WORD), 1);
    fat_buf->write (&bmfh.bfOffBits, sizeof (DWORD), 1);

    fat_buf->write (&bmih.biSize, sizeof (DWORD), 1);
    fat_buf->write (&bmih.biWidth, sizeof (LONG), 1);
    fat_buf->write (&bmih.biHeight, sizeof (LONG), 1);
    fat_buf->write (&bmih.biPlanes, sizeof (WORD), 1);
    fat_buf->write (&bmih.biBitCount, sizeof (WORD), 1);
    fat_buf->write (&bmih.biCompression, sizeof (DWORD), 1);
    fat_buf->write (&bmih.biSizeImage, sizeof (DWORD), 1);
    fat_buf->write (&bmih.biXPelsPerMeter, sizeof (LONG), 1);
    fat_buf->write (&bmih.biYPelsPerMeter, sizeof (LONG), 1);
    fat_buf->write (&bmih.biClrUsed, sizeof (DWORD), 1);
    fat_buf->write (&bmih.biClrImportant, sizeof (DWORD), 1);
    for (i = bmp->h - 1; i >= 0; i--) {
        bmpGetHighCScanline (bmp->bits + i * bmp->pitch, scanline,
                        bmp->w, bpp, bmp->depth);
        fat_buf->write (scanline, sizeof (char), scanlinebytes);
    }
#endif
    free (scanline);
	fat_buf->syncFile(fd);
	delete fat_buf;
    return ERR_BMP_OK;
}

int SaveBmpToFatFs (PMYBITMAP my_bmp, RGB* pal, const char* spFileName)
{
    const char* ext;
    int save_ret;
	int fd = fat_open(spFileName, FA_WRITE | FA_OPEN_ALWAYS);
    save_ret = __fat_save_bmp(fd, my_bmp, pal);
	fat_close(fd);
    return save_ret;
}

void StorageManager::detectTfCard()
{
	if (mMounted == true && !isMount(MOUNT_POINT)) {	//umount
		//notify(0, STORAGE_STATUS_REMOVE, NULL);
		//notify(0, STORAGE_STATUS_UNMOUNTING, NULL);
	} else if (mMounted == false && isMount(MOUNT_POINT)) { //mount
		//notify(0, STORAGE_STATUS_PENDING, NULL);
		//notify(0, STORAGE_STATUS_MOUNTED, NULL);
	}
	usleep(500*1000);
}
#endif

void StorageManager::dblock(bool flag)
{
	if (flag) {
		mDBInitL.lock();
	} else {
		mDBInitL.unlock();
	}
}

void StorageManager::stopUpdate()
{
	mStopDBUpdate = true;
}
