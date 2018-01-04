#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include <binder/IMemory.h>
#include <include_media/media/HerbMediaMetadataRetriever.h>
#include <private/media/VideoFrame.h>
#include <CedarMediaServerCaller.h>

#include "windows.h"
#include "keyEvent.h"
#include "cdr_message.h"
#include "StorageManager.h"
#include "Dialog.h"
#undef LOG_TAG
#define LOG_TAG "PlayBackPreview.cpp"
#include "debug.h"

extern int releaseMemFromFatFile(char **buffer);
extern int getMemFromFatFile(const char *file, char **data, int *size);
extern const char* get_extension (const char* filename);
extern int _mkdirs(const char *dir);

#define EMBEDDED_PICTURE_TYPE_ANY		0xFFFF
#define ONT_SHOT_TIMER	100

using namespace android;
int PlayBackPreview::keyProc(int keyCode, int isLongPress)
{
	HWND hStatusBar;
	hStatusBar = mCdrMain->getWindowHandle(WINDOWID_STATUSBAR);
	switch(keyCode) {
	case CDR_KEY_OK:
		if(isLongPress == SHORT_PRESS) {			
			//getFileInfo(); todo
			return playBackVideo();
		}
		break;
	case CDR_KEY_MODE:
		if(isLongPress == SHORT_PRESS) {
			db_msg("go to RecordPreview\n");
			SendMessage(hStatusBar, STBM_CLEAR_LABEL1_TEXT, WINDOWID_PLAYBACKPREVIEW, 0);
			return WINDOWID_RECORDPREVIEW;
		}
		break;
	case CDR_KEY_MENU:
		if (mNeedCloseLayer) {	//to avoid overlaying images
			mCdrMain->stopPreviewOutside(CMD_CLOSE_LAYER);
			mNeedCloseLayer = false;
		}
		if(isLongPress == SHORT_PRESS) {
			lockedCurrentFile();
		}else if(isLongPress == LONG_PRESS){
			StorageManager* sm;
			sm = StorageManager::getInstance();
			if(sm->isMount() == false) {
				ShowTipLabel(mHwnd, LABEL_NO_TFCARD, false, 2000);    // show 2000 ms
				break;
			}
			getFileInfo();
			if(fileInfo.fileCounts == 0) {
				db_msg("no file to delete\n");	
				ShowTipLabel(mHwnd, LABEL_FILELIST_EMPTY, false);
			} else{
				if(!strncmp(fileInfo.curInfoString.string(), INFO_LOCK, 4)){
					db_msg(">>>>>>>>>>>>>>>>>>>>>>>>>the file locked!\n");
					ShowTipLabel(mHwnd, LABEL_FILE_LOCKED, false, 2000);	// show 2000 ms
				}else
					deleteFileDialog();
			}

		}
		break;
	case CDR_KEY_LEFT:
		if(isLongPress == SHORT_PRESS){
				prevPreviewFile();
		}
		else
			{
           lockedCurrentFile();
		}
		break;
	case CDR_KEY_RIGHT:
		if(isLongPress == SHORT_PRESS){
				nextPreviewFile();
		}
		break;
	default:
		break;
	}
	return WINDOWID_PLAYBACKPREVIEW;
}

int PlayBackPreview::msgCallback(int message, int data)
{
	int ret = 0;
	switch(message) {
	case MSG_RM_GET_FILELIST:
		ret = getFileList((char **)data);
		break;
	case MSG_RM_DEL_FILE:
		{
			const char *file = (const char *)data;
			StorageManager *sm = StorageManager::getInstance();
			if(!sm->dbIsFileExist(String8(file))) {
				return -1;
			}
			ret = sm->dbDelFile(file);
			if (ret != 0) {
				db_error("fail to delete file %s from db", file);
				return ret;
			}
			ret = sm->deleteFile(file);
			if (ret != 0) {
				db_error("fail to delete file %s", file);
			}
		}
		break;
	default:
		break;
	}
	return ret;
}

void PlayBackPreview::prepareCamera4Playback(int flag)
{
	if (!flag) {
		mCdrMain->prepareCamera4Playback(true);
	} else {
		mCdrMain->stopPreviewOutside(flag);	//the memory may be not enough
	}
}

int PlayBackPreviewProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
	PlayBackPreview* mPLBPreview = (PlayBackPreview*)GetWindowAdditionalData(hWnd);
	int ret = 0;
	db_msg("message code: [%d], %s, wParam=%d, lParam=%lu\n", message, Message2Str(message), wParam, lParam);
	switch(message) {
	case MSG_CREATE:
		mPLBPreview->createSubWidgets(hWnd);
		break;
	case MSG_SHOWWINDOW:
		if(wParam == SW_SHOWNORMAL) {
			mPLBPreview->showPlaybackPreview();
		} else {
			mPLBPreview->releaseResource();
		}
		break;
	case MSG_CDR_KEY:
		db_msg("key code is %d\n", wParam);
		return mPLBPreview->keyProc(wParam, lParam);
	case MWM_CHANGE_FROM_WINDOW:
		mPLBPreview->prepareCamera4Playback();
		break;
	case MSG_STOP_PREVIEW:
		mPLBPreview->prepareCamera4Playback(wParam);
		break;
	case MSG_UPDATE_LIST:
		{
			int ret = mPLBPreview->updatePreviewFilelist();
			if(ret == 0) {
				mPLBPreview->showThumbnail();
			} else {
				mPLBPreview->showBlank();
			}
		}
		break;
	case MSG_SET_CURRENT_FILEID:
		mPLBPreview->mCurID = (int)wParam;
		break;
	default:
		ret = mPLBPreview->msgCallback(message, (int)wParam);
		break;
	}
	if (ret != 0) {
		return ret;
	}
	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

	PlayBackPreview::PlayBackPreview(CdrMain* pCdrMain)
	: mCdrMain(pCdrMain)
	, bmpIcon()
	, bmpImage()
	  , fileInfo()
	 ,mNeedCloseLayer(false)
{
	HWND hParent;
	RECT rect;
	CDR_RECT STBRect;
	ResourceManager* mRM;
	hParent = mCdrMain->getHwnd();
	mRM = ResourceManager::getInstance();
	mRM->getResRect(ID_STATUSBAR, STBRect);
	GetWindowRect(hParent, &rect);
	rect.top += (STBRect.h + 0); //+1 will produce gap
	mHwnd = CreateWindowEx(WINDOW_PLAYBACKPREVIEW, "",
			WS_NONE,
			WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT,
			WINDOWID_PLAYBACKPREVIEW,
			rect.left, rect.top, RECTW(rect), RECTH(rect),
			hParent, (DWORD)this);
	if(mHwnd == HWND_INVALID) {
		db_error("create PlayBack Preview window failed\n");
		return;
	}
	mRM->setHwnd(WINDOWID_PLAYBACKPREVIEW, mHwnd);
	mCurID = 0;
	ShowWindow(mHwnd, SW_HIDE);
#ifdef FATFS
	mThumbPic.data = NULL;
	mThumbPic.data_len = 0;
#endif
}
PlayBackPreview::~PlayBackPreview()
{
}

int PlayBackPreview::createSubWidgets(HWND hParent)
{
	HWND hWnd;
	ResourceManager* mRM;
	CDR_RECT rect;
	int retval;

	mRM = ResourceManager::getInstance();
	mRM->getResRect(ID_PLAYBACKPREVIEW_IMAGE, rect);

	hWnd = CreateWindowEx(CTRL_STATIC, "",
			WS_VISIBLE | WS_CHILD | SS_BITMAP | SS_CENTERIMAGE,
			WS_EX_NONE,
			ID_PLAYBACKPREVIEW_IMAGE,
			rect.x, rect.y, rect.w, rect.h,
			hParent, 0);

	if(hWnd == HWND_INVALID) {
		db_error("create playback previe image label failed\n");
		return -1;
	}

	mRM->getResRect(ID_PLAYBACKPREVIEW_ICON, rect);

	retval = mRM->getResBmp(ID_PLAYBACKPREVIEW_ICON, BMPTYPE_BASE, bmpIcon);
	if(retval < 0) {
		db_error("get RES_BMP_CURRENT_PLBPREVIEW_ICON failed\n");
		return -1;
	}

	hWnd = CreateWindowEx(CTRL_STATIC, "",
			WS_VISIBLE | WS_CHILD | SS_BITMAP | SS_CENTERIMAGE,
			WS_EX_NONE | WS_EX_TRANSPARENT,
			ID_PLAYBACKPREVIEW_ICON,
			rect.x, rect.y, bmpIcon.bmWidth, bmpIcon.bmHeight,
			hParent, (LPARAM)&bmpIcon);
	if(hWnd == HWND_INVALID) {
		db_error("create playback preview image label failed\n");
		return -1;
	}
	//	ShowWindow(hWnd, SW_HIDE);

	return 0;
}

int PlayBackPreview::getFileList(char **file_list)
{
	int retval;
	StorageManager* mSTM;
	dbRecord_t dbRecord;
	int cnt = 0;
	int cur_file_list_len = 0;
	int file_len = 0;
	char *str;
	int buf_size = strlen(*file_list) + 1;
	mSTM = StorageManager::getInstance();
	if (mSTM->isMount() == false) {
		db_msg(" tf card is not mount");
		**file_list = 0;
		return 1;
	}
	retval = mSTM->dbCount((char*)TYPE_VIDEO);
	if(retval < 0) {
		db_error("get db count failed\n");
		return -1;
	}
	cnt += retval;

	retval = mSTM->dbCount((char*)TYPE_PIC);
	if(retval < 0) {
		db_error("get db count failed\n");
		return -1;
	}
	cnt += retval;
	**file_list = 0;
	if (cnt == 0) {
		return 0;
	}
	char sFileSize[32]={0};
	for(int i=0; i<cnt; i++) {
		if(mSTM->dbGetSortByTimeRecord(dbRecord, i) < 0) {
			db_error("get sort by time record failed\n");
			return -1;
		}
		str = (char*)(dbRecord.file.string());
		file_len = strlen(str);
		sprintf(sFileSize, "%lu", mSTM->fileSize(String8(str)));
		cur_file_list_len = cur_file_list_len + file_len + strlen(sFileSize) + strlen(":") + strlen(";");
		if (cur_file_list_len >= buf_size) {

			buf_size += 1024;
			db_error("realloc %d\n", buf_size);
			*file_list = (char*)realloc(*file_list, sizeof(char)*(buf_size));
			if(*file_list == NULL) {
				return -1;
			}
		}
		strncat(*file_list, str, file_len);
		strncat(*file_list, ":", 1);
		strncat(*file_list, sFileSize, strlen(sFileSize));
		strncat(*file_list, ";", 1);
	}
	*(*file_list + cur_file_list_len) = 0;
	return 0;
}
	
int PlayBackPreview::getFileInfo(void)
{
	int retval;
	StorageManager* mSTM;
	dbRecord_t dbRecord;
	mSTM = StorageManager::getInstance();
	retval = mSTM->dbCount((char*)TYPE_VIDEO);
	if(retval < 0) {
		db_error("get db count failed\n");
		return -1;
	}
	db_msg("video fileCounts is %d\n", retval);
	fileInfo.fileCounts = retval;

	retval = mSTM->dbCount((char*)TYPE_PIC);
	if(retval < 0) {
		db_error("get db count failed\n");
		return -1;
	}
	db_msg("photo fileCounts is %d\n", retval);
	fileInfo.fileCounts += retval;

	if(fileInfo.fileCounts == 0) {
		db_msg("empty database\n");	
		return 0;
	}
	int idx = fileInfo.curIdx;
	bool desc = true;
	if (desc) {
		idx = fileInfo.fileCounts - 1 - fileInfo.curIdx;
	}
	db_msg("curIdx is %d, actual idx %d\n", fileInfo.curIdx, idx);
	if(mSTM->dbGetSortByTimeRecord(dbRecord, (idx)) < 0) {
		db_error("get sort by time record failed\n");
		return -1;
	}
	fileInfo.curFileName = dbRecord.file;
	fileInfo.curTypeString = dbRecord.type;
	fileInfo.curInfoString = dbRecord.info;
	db_msg("cur file is %s\n", fileInfo.curFileName.string());
	db_msg("cur Type is %s\n", fileInfo.curTypeString.string());
	db_msg("cur info is %s\n", fileInfo.curInfoString.string());
	return 0;
}

static void simplifyFileName(char *name)
{
	if (!name) {
		return ;
	}
	char *ptr = NULL;
	ptr = strrchr(name, '.');
	*ptr = 0;
}

static void simplifyFileName2(char *name)
{
	if (!name) {
		return ;
	}
	char *ptr = NULL;
	ptr = strrchr(name, '_');
	*ptr = 0;
}

/*
 * 20150108_005607A ---> 15-01-08 00:56:07 A
 */
static void changeTimeShowStyle(char *dst, char *name)
{
	int i = 0;
	char date_flag  = '-';
	char space_flag = ' ';
	char time_flag  = ':';
	//year
	dst[i++] = name[0];  dst[i++] = name[1];
	dst[i++] = name[2];  dst[i++] = name[3];
	dst[i++] = date_flag;
	//mon
	dst[i++] = name[4];  dst[i++] = name[5];
	dst[i++] = date_flag;
	//day
	dst[i++] = name[6];  dst[i++] = name[7];
	dst[i++] = space_flag;
	dst[i++] = space_flag;
	//hour
	dst[i++] = name[9];  dst[i++] = name[10];
	dst[i++] = time_flag;
	//min
	dst[i++] = name[11]; dst[i++] = name[12];
	dst[i++] = time_flag;
	//sec
	dst[i++] = name[13]; dst[i++] = name[14];
	dst[i++] = space_flag;
	dst[i++] = space_flag;
	//flag A/B
	dst[i++] = name[15];
	dst[i] = 0;
}

int PlayBackPreview::updatePreviewFilelist(void)
{
	HWND hStatusBar;
	char buf[50] = {0};
	char buf2[50] = {0};
	int ret = 0;
	if(getFileInfo() < 0)
		return -1;
	StorageManager *sm = StorageManager::getInstance();
	if(!sm->isMount() || (fileInfo.fileCounts == 0)) {
		sprintf(buf, "(00/00)");
		ret = 1;
	} else {
		if(getBaseName(fileInfo.curFileName.string(), buf2) != NULL){
			if(!strncmp(fileInfo.curInfoString.string(), INFO_LOCK, 4))
				simplifyFileName2(buf2);
			else
				simplifyFileName(buf2);
		}
		changeTimeShowStyle(buf, buf2);
		sprintf(buf+strlen(buf), "(%02d/%02d)", fileInfo.curIdx + 1, fileInfo.fileCounts);
	}
	hStatusBar = mCdrMain->getWindowHandle(WINDOWID_STATUSBAR);
	SendMessage(hStatusBar, STBM_SET_FILELIST, 0, (LPARAM)buf);
	return ret;
}

static void createDirs()
{
#ifdef FATFS
		_mkdirs(CON_3STRING(MOUNT_PATH, PIC_DIR, TYPE_THUMB));
#else
		if(access(CON_3STRING(MOUNT_PATH, VIDEO_DIR, THUMBNAIL_DIR), F_OK) != 0) {
			if(mkdir(CON_3STRING(MOUNT_PATH, VIDEO_DIR, THUMBNAIL_DIR), 0777) != 0) {
				db_error("create dir %s failed\n", CON_3STRING(MOUNT_PATH, VIDEO_DIR, THUMBNAIL_DIR));
				return ;
			}
		}
#endif
}

static int getThumbnailName(const char *list_file, int type_pic, char *picFile)
{
	const char *type = type_pic ? TYPE_PIC : TYPE_VIDEO;
	const char *dir  = type_pic
		? CON_3STRING(MOUNT_PATH, PIC_DIR, THUMBNAIL_DIR)
		: CON_3STRING(MOUNT_PATH, VIDEO_DIR, THUMBNAIL_DIR);

	char buf2[128] = {0};
	char buf[128] = {0};
	if(getBaseName(list_file, buf2) == NULL) {
		db_error("get base name failed, file is %s\n", list_file);
		return -1;
	}
	if(removeSuffix(buf2, buf) == NULL) {
		db_error("remove the file suffix failed, file is %s\n", list_file);
		return -1;
	}
	sprintf(picFile, "%s%s.bmp", dir,buf);
	return 0;
}

static bool inline file_exist(const char *picFile)
{
#ifdef FATFS
	return FILE_EXSIST(picFile);
#else
	return !access(picFile, F_OK);
#endif
}


int decodeJpeg(String8 file, const char *save_thumbnail)
{
	int ret = 0;
	CedarMediaServerCaller *caller = new CedarMediaServerCaller();
	if (caller == NULL) {
	    ALOGE("Failed to alloc CedarMediaServerCaller");
		return -1;
	}
	sp<IMemory> mem = caller->jpegDecode(file);
	if (mem == NULL) {
	    ALOGE("jpegDecode error!");
	    delete caller;
	    return -1;
	}
	VideoFrame *videoFrame = NULL;
	videoFrame = static_cast<VideoFrame *>(mem->pointer());
	if (videoFrame == NULL) {
		db_error("getFrameAtTime: videoFrame is a NULL pointer");
		delete caller;
		ret = -1;
		return ret;
	}
	StorageManager *sm = StorageManager::getInstance();
	ret = sm->saveVideoFrameToBmpFile(videoFrame, save_thumbnail);
	if (mem != NULL)
		mem.clear();
	delete caller;
	return ret;
}

int PlayBackPreview::getPicFileName(String8 &bmpFile)
{
	char picFile[128] = {0};
	int ret = 0;
	bool type_pic = false;
	if(strncmp(fileInfo.curTypeString.string(), TYPE_PIC, 5) == 0) {
		type_pic = true;
	}
	getThumbnailName(fileInfo.curFileName.string(), type_pic, picFile);
	if (!file_exist(picFile)) {
		db_msg("file not exist");
		SendNotifyMessage(mHwnd, MSG_STOP_PREVIEW, CMD_STOP_PREVIEW|CMD_CLEAN_LAYER, 0);
		if (type_pic) {
			ret = decodeJpeg(fileInfo.curFileName, picFile);
			if (ret < 0) {
				return ret;
			}
		} else {
			if(getBmpPicFromVideo(fileInfo.curFileName.string(), picFile) != 0) {
				db_error("get picture from video failed\n");	
				return -1;
			}
		}
	}
	bmpFile = picFile;
 	return 0;
}

void PlayBackPreview::showBlank()
{
 	HWND retWnd;
	retWnd = GetDlgItem(mHwnd, ID_PLAYBACKPREVIEW_IMAGE);

	db_msg("erase PLBPREVIEW_IMAGE background!");
	SendMessage(mCdrMain->getWindowHandle(WINDOWID_STATUSBAR), STBM_FILE_STATUS, 0, 0);
	ResourceManager * rs = ResourceManager::getInstance();
	SendMessage(retWnd, STM_SETIMAGE, (WPARAM)(rs->getNoVideoImage()), 0);

	retWnd = GetDlgItem(mHwnd, ID_PLAYBACKPREVIEW_ICON);
	ShowWindow(retWnd, SW_HIDE);
}

void PlayBackPreview::showThumbnail_ths(void)
{
	int ret;
	String8 bmpFile;
	HWND retWnd;
	db_msg("fileInfo.fileCounts :%d", fileInfo.fileCounts);
	if(fileInfo.fileCounts == 0) {
		db_msg("no file to preview\n");
		goto out;
	}
	db_msg("current file type is %s\n", fileInfo.curTypeString.string());
	db_msg("current file name is %s\n", fileInfo.curFileName.string());
	if(getPicFileName(bmpFile) < 0)	{
		db_error("get pic file name failed\n");
		showBlank();
		goto out;
	}
#ifdef FATFS
	releaseMemFromFatFile(&mThumbPic.data);
	getMemFromFatFile(bmpFile.string(), &mThumbPic.data, &mThumbPic.data_len);
    const char* ext;
    /* format, png, jpg etc. */
    if ((ext = get_extension (bmpFile.string())) == NULL) {
		db_msg("ERR_BMP_UNKNOWN_TYPE");
        goto out;
    }
	db_msg(" ext:%s", ext);
	if(bmpImage.bmBits) {
		UnloadBitmap(&bmpImage);
	}
	if ((ret = LoadBitmapFromMem(HDC_SCREEN, &bmpImage, mThumbPic.data, mThumbPic.data_len, ext)) != ERR_BMP_OK) {
		db_error("load %s failed, ret is %d, load once again\n", bmpFile.string(), ret);
		StorageManager* sm = StorageManager::getInstance();
		sm->deleteFile(bmpFile.string());
		if(getPicFileName(bmpFile) < 0)	{
			db_error("get pic file name failed\n");
			showBlank();
			goto out;
		}
		if((ret = LoadBitmapFromMem(HDC_SCREEN, &bmpImage, mThumbPic.data, mThumbPic.data_len, ext)) != ERR_BMP_OK) {
			db_error("load %s failed finally, ret is %d\n", bmpFile.string(), ret);
			goto out;
		}
	}
#else
	if(bmpImage.bmBits) {
		UnloadBitmap(&bmpImage);
	}
	if((ret = LoadBitmapFromFile(HDC_SCREEN, &bmpImage, bmpFile.string())) != ERR_BMP_OK) {
		db_error("load %s failed, ret is %d, load once again\n", bmpFile.string(), ret);
		StorageManager* sm = StorageManager::getInstance();
		sm->deleteFile(bmpFile.string());
		if(getPicFileName(bmpFile) < 0)	{
			db_error("get pic file name failed\n");
			showBlank();
			goto out;
		}
		if((ret = LoadBitmapFromFile(HDC_SCREEN, &bmpImage, bmpFile.string())) != ERR_BMP_OK) {
			db_error("load %s failed finally, ret is %d\n", bmpFile.string(), ret);
			goto out;
		}
	}
#endif
	retWnd = GetDlgItem(mHwnd, ID_PLAYBACKPREVIEW_IMAGE);
	SendMessage(retWnd, STM_SETIMAGE, (WPARAM)&bmpImage, 0);
	retWnd = GetDlgItem(mHwnd, ID_PLAYBACKPREVIEW_ICON);
	if(strncmp(fileInfo.curTypeString.string(), TYPE_PIC, 5) == 0) {
		if(IsWindowVisible(retWnd)) {
			db_msg("xxxxxxxxxx\n");	
			ShowWindow(retWnd, SW_HIDE);
		}
	} else {
		if(!IsWindowVisible(retWnd)) {
			ShowWindow(retWnd, SW_SHOWNORMAL);
		}
	}

	///////////////////////show locked icon//////////////////////////////////
	if(!strncmp(fileInfo.curInfoString.string(), INFO_LOCK, 4) )
		SendMessage(mCdrMain->getWindowHandle(WINDOWID_STATUSBAR), STBM_FILE_STATUS, 1, 0);
	else
		SendMessage(mCdrMain->getWindowHandle(WINDOWID_STATUSBAR), STBM_FILE_STATUS, 0, 0);
out:
	mFinishedGen = true;
	///////////////////////////////////////////////////////////////////////
	return;
}


void PlayBackPreview::showThumbnail()
{
	if (firstPic) {
		showThumbnail_ths();
		firstPic = false;
		return ;
	}
	if (!mFinishedGen) {
		return;
	}
	mFinishedGen = false;
	sp<STAThread> sta_ths = new STAThread(this);
	sta_ths->start();
	sta_ths.clear();
	sta_ths = NULL;
}

int PlayBackPreview::getBmpPicFromVideo(const char* videoFile, const char* picFile)
{
	status_t ret;
	int retval = 0;
	sp<IMemory> frameMemory = NULL;
	HerbMediaMetadataRetriever *retriever;
	StorageManager* mSTM;

	mSTM = StorageManager::getInstance();
	retriever = new HerbMediaMetadataRetriever();
	if (retriever == NULL) {
		db_error("Failed to alloc HerbMediaMetadataRetriever");
		return -1;
	}

	ret = retriever->setDataSource(videoFile);
	if (ret != NO_ERROR) {
		db_error("setDataSource failed(%d)", ret);
		retval = -1;
		goto out;
	}

	frameMemory = retriever->getEmbeddedPicture(EMBEDDED_PICTURE_TYPE_ANY);
	if (frameMemory != NULL) {
		MediaAlbumArt* mediaAlbumArt = NULL;
		db_msg("Have EmbeddedPicture");
		mediaAlbumArt = static_cast<MediaAlbumArt *>(frameMemory->pointer());
		if (mediaAlbumArt == NULL) {
			db_error("getEmbeddedPicture: Call to getEmbeddedPicture failed.");
			retval = -1;
			goto out;
		}
		//		unsigned int len = mediaAlbumArt->mSize;
		//		char *data = (char*)mediaAlbumArt + sizeof(MediaAlbumArt);
		/*
		 * TODO: how to get the width and height
		 * */
	} else {
		VideoFrame *videoFrame = NULL;
		db_msg("getFrameAtTime");

		frameMemory = retriever->getFrameAtTime();
		if (frameMemory == NULL) {
			db_error("Failed to get picture");
			retval = -1;
			goto out;
		}

		videoFrame = static_cast<VideoFrame *>(frameMemory->pointer());
		if (videoFrame == NULL) {
			db_error("getFrameAtTime: videoFrame is a NULL pointer");
			retval = -1;
			goto out;
		}

		db_msg("videoFrame: width=%d, height=%d, disp_width=%d,disp_height=%d, size=%d, RotationAngle=%d",
				videoFrame->mWidth, videoFrame->mHeight, videoFrame->mDisplayWidth, videoFrame->mDisplayHeight, videoFrame->mSize, videoFrame->mRotationAngle);

		ret = mSTM->saveVideoFrameToBmpFile(videoFrame, picFile);
		if (ret != ERR_BMP_OK) {
			db_msg("Failed to save picture, ret is %d", ret);
			retval = -1;
			goto out;
		}
	}

out:
	if(retriever)
		delete retriever;
	if(frameMemory != NULL)
		frameMemory.clear();
	return retval;
}

bool PlayBackPreview::isCurrentVideoFile(void)
{
#ifdef FATFS
	if(!FILE_EXSIST(fileInfo.curFileName.string())) {
#else
	if(access(fileInfo.curFileName.string(), F_OK) != 0) {
#endif
		db_error("the current file %s not exsist\n", fileInfo.curFileName.string());
		return false;
	}
	if(strncmp(fileInfo.curTypeString.string(), TYPE_VIDEO, 5) == 0)
		return true;
	else 
		return false;
}

void PlayBackPreview::nextPreviewFile(void)
{
	if(fileInfo.fileCounts == 0)
		return;

	fileInfo.curIdx++;
	if(fileInfo.curIdx >= fileInfo.fileCounts)
		fileInfo.curIdx = 0;
	int ret = updatePreviewFilelist();
	if(ret == 0) {
		showThumbnail();
	} else {
		showBlank();
	}
}

void PlayBackPreview::prevPreviewFile(void)
{
	if(fileInfo.fileCounts == 0) {
		return;
	}

	if(fileInfo.curIdx == 0)
		fileInfo.curIdx = fileInfo.fileCounts - 1;
	else
		fileInfo.curIdx--;
	int ret = updatePreviewFilelist();
	if(ret == 0) {
		showThumbnail();
	} else {
		showBlank();
	}
}

void PlayBackPreview::getCurrentFileName(String8 &file)
{
	file = fileInfo.curFileName;
}

void PlayBackPreview::getCurrentFileInfo(PLBPreviewFileInfo_t &info)
{
	info = fileInfo;
}

void PlayBackPreview::showPlaybackPreview()
{
	fileInfo.curIdx = mCurID;
	mCurID=0;
	int ret = updatePreviewFilelist();
	showBlank();
	createDirs();
	if(ret == 0) {
		showThumbnail();
	}
	mNeedCloseLayer = true;
}

void PlayBackPreview::fixIdx()
{
	if(fileInfo.fileCounts == 0)
		return;
	if((fileInfo.curIdx + 1) == fileInfo.fileCounts)
		fileInfo.curIdx--;
}

static void deleteDialogCallback(HWND hDlg, void* data)
{
	PlayBackPreview* playbackPreview;
	StorageManager* sm;
	PLBPreviewFileInfo_t fileInfo;
	int ret = 0;
	playbackPreview = (PlayBackPreview*)data;
	sm = StorageManager::getInstance();

	/* delete file */
	playbackPreview->getCurrentFileInfo(fileInfo);
	db_msg("delete the %drd file %s\n", fileInfo.curIdx, fileInfo.curFileName.string());
	ret = sm->deleteFile(fileInfo.curFileName.string());
	ret = sm->dbDelFile(fileInfo.curFileName.string());
	playbackPreview->fixIdx();
	ret = playbackPreview->updatePreviewFilelist();
	if(ret == 0) {
		playbackPreview->showThumbnail();
	} else {
		playbackPreview->showBlank();
	}
	EndDialog(hDlg, IDC_BUTTON_OK);
}

void PlayBackPreview::deleteFileDialog(void)
{
	int retval;
	const char* ptr;
	MessageBox_t messageBoxData;
	ResourceManager* rm;
	String8 textInfo;

	rm = ResourceManager::getInstance();
	memset(&messageBoxData, 0, sizeof(messageBoxData));
	messageBoxData.dwStyle = MB_OKCANCEL | MB_HAVE_TITLE | MB_HAVE_TEXT;
	messageBoxData.flag_end_key = 0; /* end the dialog when endkey keydown */

	ptr = rm->getLabel(LANG_LABEL_DELETEFILE_TITLE);
	if(ptr == NULL) {
		db_error("get deletefile title failed\n");
		return;
	}
	messageBoxData.title = ptr;

	ptr = rm->getLabel(LANG_LABEL_DELETEFILE_TEXT);
	if(ptr == NULL) {
		db_error("get deletefile text failed\n");
		return;
	}
	messageBoxData.text = ptr;

	ptr = rm->getLabel(LANG_LABEL_SUBMENU_OK);
	if(ptr == NULL) {
		db_error("get ok string failed\n");
		return;
	}
	messageBoxData.buttonStr[0] = ptr;

	ptr = rm->getLabel(LANG_LABEL_SUBMENU_CANCEL);
	if(ptr == NULL) {
		db_error("get ok string failed\n");
		return;
	}
	messageBoxData.buttonStr[1] = ptr;

	messageBoxData.pLogFont = rm->getLogFont();

	rm->getResRect(ID_MENU_LIST_MB, messageBoxData.rect);
	messageBoxData.fgcWidget = rm->getResColor(ID_MENU_LIST_MB, COLOR_FGC);
	messageBoxData.bgcWidget = rm->getResColor(ID_MENU_LIST_MB, COLOR_BGC);
	messageBoxData.linecTitle = rm->getResColor(ID_MENU_LIST_MB, COLOR_LINEC_TITLE);
	messageBoxData.linecItem = rm->getResColor(ID_MENU_LIST_MB, COLOR_LINEC_ITEM);
	messageBoxData.confirmCallback = deleteDialogCallback;
	messageBoxData.confirmData = (void*)this;

	db_msg("this is %p\n", this);
	retval = showMessageBox(mHwnd, &messageBoxData);
	db_msg("retval is %d\n", retval);
}


void PlayBackPreview::lockedCurrentFile(void)
{
	StorageManager* sm = StorageManager::getInstance();
	db_msg(">>>>>>>>>>>>>>>>>>  eeeeeeeee\n");
	if (!sm->isInsert()){
		ShowTipLabel(mHwnd, LABEL_NO_TFCARD,  false, 3000);
		return ;
	}	
	getFileInfo();
	if (fileInfo.fileCounts == 0){
		BroadcastMessage(MSG_CLOSE_TIP_LABEL, 0, 0);
		ShowTipLabel(mHwnd, LABEL_FILELIST_EMPTY,  false, 3000);
		return ;	
	}

	String8 newFile;
	if( !strncmp(fileInfo.curTypeString.string(), TYPE_VIDEO, 5)
		&& !strncmp(fileInfo.curInfoString.string(), INFO_NORMAL, 6) )
	{
		sm->setFileInfo(fileInfo.curFileName.string(), TRUE ,newFile);
		fileInfo.curFileName = newFile;
		SendMessage(mCdrMain->getWindowHandle(WINDOWID_STATUSBAR), STBM_FILE_STATUS, TRUE , 0);
		ShowTipLabel(mHwnd, LABEL_LOCKED_FILE, false,3000);

	}
	else if(  !strncmp(fileInfo.curTypeString.string(), TYPE_VIDEO, 5)
		&&  !strncmp(fileInfo.curInfoString.string(), INFO_LOCK, 4) )
	{
		sm->setFileInfo(fileInfo.curFileName.string(), FALSE,newFile);
		fileInfo.curFileName = newFile;
		SendMessage(mCdrMain->getWindowHandle(WINDOWID_STATUSBAR), STBM_FILE_STATUS, FALSE , 0);
		ShowTipLabel(mHwnd, LABEL_UNLOCK_FILE, false,3000);
	}
}


int PlayBackPreview::playBackVideo(void)
{
	int retval;
	HWND hPlayBack, hStatusBar;
	StorageManager* sm;

	sm = StorageManager::getInstance();
	if(sm->isMount() == false) {
		ShowTipLabel(mHwnd, LABEL_NO_TFCARD);
		goto out;
	}
	if(fileInfo.fileCounts == 0) {
		ShowTipLabel(mHwnd, LABEL_FILELIST_EMPTY);
		goto out;
	}

	if(isCurrentVideoFile() == true) {
		db_msg("start playback\n");
		hPlayBack = mCdrMain->getWindowHandle(WINDOWID_PLAYBACK);
		retval = SendMessage(hPlayBack, MSG_PREPARE_PLAYBACK, (WPARAM)&(fileInfo.curFileName), fileInfo.curIdx);
		if(retval < 0) {
			db_msg("prepare playback failed\n");	
			ShowTipLabel(mHwnd, LABEL_PLAYBACK_FAIL);
			goto out;
		}

		hStatusBar = mCdrMain->getWindowHandle(WINDOWID_STATUSBAR);
		SendMessage(hStatusBar, STBM_CLEAR_LABEL1_TEXT, WINDOWID_PLAYBACKPREVIEW, 0);
		mCdrMain->stopPreviewOutside(CMD_STOP_PREVIEW|CMD_CLEAN_LAYER);
		return WINDOWID_PLAYBACK;
	}
out:
	return WINDOWID_PLAYBACKPREVIEW;
}

int PlayBackPreview::releaseResource()
{
	firstPic = true;
#ifdef FATFS
	releaseMemFromFatFile(&mThumbPic.data);
	if(bmpImage.bmBits) {
		UnloadBitmap(&bmpImage);
	}
#endif
	return 0;
}
