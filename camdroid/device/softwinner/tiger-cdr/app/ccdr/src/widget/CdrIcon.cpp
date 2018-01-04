
#include "CdrIcon.h"

#undef LOG_TAG
#define LOG_TAG "CdrIcon"
#include "debug.h"

using android::String8;

static WNDPROC oldProc = NULL;

static int cdrIconProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
	CdrIcon* cdrIcon;

	cdrIcon = (CdrIcon*)GetWindowAdditionalData(hWnd);
	//db_msg("message code: [%d], %s, wParam=%d, lParam=0x%lx\n", message, Message2Str(message), wParam, lParam);

	if(cdrIcon != NULL)
		return cdrIcon->windowProc(hWnd, message, wParam, lParam);

	return 0;
}

int CdrIcon::windowProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
	//db_msg("id [%d], name: %s, message code: [%d], %s\n", mId, mName.string(), message, Message2Str(message));
	switch(message) {
	case MSG_PAINT:
		{
			HDC hdc;
			RECT rect;
			gal_pixel oldColor, bkColor;

			if(mBkgroudBmp.bmBits != NULL && mBmp.bmBits != NULL) {
				GetClientRect(hWnd, &rect);
				hdc = BeginPaint(hWnd);

				SetBkMode(hdc, BM_TRANSPARENT);

				FillBoxWithBitmap(hdc, 0, 0, RECTW(rect), RECTH(rect), &mBkgroudBmp);

				int x, y, w, h;
				if( (mBmp.bmWidth < RECTW(rect)) || (mBmp.bmHeight < RECTH(rect)) ) {
					w = mBmp.bmWidth;
					h = mBmp.bmHeight;
					x = (rect.right - w) >> 1;
					y = (rect.bottom - h) >> 1;
				} else {
					x = y = 0;
					w = RECTW(rect);
					h = RECTH(rect);
				}
				FillBoxWithBitmap(hdc, x, y, w, h, &mBmp);

				EndPaint(hWnd, hdc);
				return 0;
			}
		}
		break;
	default:
		break;
	}

	if(oldProc != NULL)
		return (*oldProc)(hWnd, message, wParam, lParam);

	return 0;
}

CdrIcon::CdrIcon(CdrIcon_t* iconData, XCreateParams* createParam) :
	mHwnd(HWND_INVALID),
	mHparent(createParam->hParent),
	mId(iconData->id),
	mName(String8(iconData->name)),
	mIconIndex(0)
{
	int retval;
	ResourceManager* rm;
	CDR_RECT rect;
	DWORD style = WS_CHILD | SS_BITMAP;
	DWORD exStyle = WS_EX_NONE;

	memset(&mBmp, 0, sizeof(BITMAP));
	memset(&mBkgroudBmp, 0, sizeof(BITMAP));
	db_msg("CdrIcon Constructor, name: %s, id: %d\n", mName.string(), mId);

	rm = ResourceManager::getInstance();
	retval = rm->getResRect(mId, rect);
	if(retval < 0) {
		db_error("get %s rect failed\n", mName.string());
		return;
	}
	db_msg("%d %d %d %d", rect.x, rect.y, rect.w, rect.h);

	setIconIndex(rm->getCurrentIconValue(mId));

	retval = rm->getResBmp(mId, BMPTYPE_BASE, mBmp);
	if(retval < 0) {
		db_error("loadb bitmap of %s failed\n", mName.string());
	}

	style |= createParam->style;
	exStyle |= createParam->exStyle;

	mHwnd = CreateWindowEx(CTRL_STATIC, "",
			style,
			exStyle,
			mId,
			rect.x, rect.y, rect.w, rect.h,
			mHparent, (DWORD)&mBmp);
	if(mHwnd == HWND_INVALID) {
		db_error("create icon Widget %s failed\n", mName.string());
		return;
	}

	SetWindowAdditionalData(mHwnd, (DWORD)this);
	oldProc = SetWindowCallbackProc(mHwnd, cdrIconProc);
}

CdrIcon::~CdrIcon()
{
	if(mBmp.bmBits != NULL) {
		UnloadBitmap(&mBmp);
	}

	if(mBkgroudBmp.bmBits != NULL) {
		UnloadBitmap(&mBkgroudBmp);
	}
}

HWND CdrIcon::getHwnd(void)
{
	return mHwnd;
}

String8 CdrIcon::getName(void)
{
	return mName;
}

PBITMAP CdrIcon::getImage(void)
{
	return &mBmp;
}

enum ResourceID CdrIcon::getId(void)
{
	return mId;
}

unsigned int CdrIcon::getIconIndex(void)
{
	return mIconIndex;
}

void CdrIcon::setIconIndex(unsigned int index)
{
	if(index != mIconIndex) {
		mIconIndex = index;
		updateIcon();
	}
}

// auto update the mIconIndex from ResourceManager
void CdrIcon::updateIconIndex(void)
{
	unsigned int index;
	ResourceManager* rm;	

	rm = ResourceManager::getInstance();

	index = rm->getCurrentIconValue(mId);
	setIconIndex(index);
}

void CdrIcon::setBkgroud(int id)
{
	ResourceManager* rm;

	rm = ResourceManager::getInstance();

	rm->getResBkgroundBmp(id, mBkgroudBmp);
	InvalidateRect(mHwnd, NULL, TRUE);
}

void CdrIcon::unsetBkgroud(void)
{
	if(mBkgroudBmp.bmBits != NULL) {
		UnloadBitmap(&mBkgroudBmp);
		InvalidateRect(mHwnd, NULL, TRUE);
	}
}

void CdrIcon::updateIcon(void)
{
	ResourceManager *rm;
	int retval;

//	db_msg("update %s -----val:%d mId%d\n", mName.string(), mIconIndex, mId);
	if(mHwnd != HWND_INVALID) {
		rm = ResourceManager::getInstance();
		
		rm->setCurrentIconValue(mId, mIconIndex);

		rm->unloadBitMap(mBmp);
		retval = rm->getResBmp(mId, BMPTYPE_BASE, mBmp);
		if (retval) {
			db_msg("get icon %s bmp failed\n", mName.string());
		}

		SendMessage(mHwnd, STM_SETIMAGE, (DWORD)&mBmp, 0);
		InvalidateRect(mHwnd, NULL, TRUE);
	}
}

// ------------------------------------------------------------------------------------------
CdrIconOps::CdrIconOps()
{
	mPic.clear();
}

CdrIconOps::~CdrIconOps()
{
	for(unsigned int i = 0; i< mPic.size(); i++) {
		if( mPic.valueAt(i) != NULL)
			delete mPic.valueAt(i);
	}
	mPic.clear();
}

int CdrIconOps::createCdrIcon(CdrIcon::CdrIcon_t* iconData, XCreateParams* createParam)
{
	CdrIcon* icon;

	icon = new CdrIcon(iconData, createParam);
	if(icon->getHwnd() == HWND_INVALID) {
		db_error("createIcon %s failed\n", iconData->name);
		return -1;
	}

	// check if the id has been created
	if(mPic.indexOfKey(iconData->id) >= 0) {
		db_error("id %d has beed created\n, ", iconData->id);
		return -1;
	}

	mPic.add(iconData->id, icon);

	return 0;
}

void CdrIconOps::hideIcon(enum ResourceID id)
{
	CdrIcon* icon = getIcon(id);

	if(icon != NULL) {
		if(IsWindowVisible(icon->getHwnd()) == TRUE)
			ShowWindow(icon->getHwnd(), SW_HIDE);
	}
}

void CdrIconOps::showIcon(enum ResourceID id)
{
	CdrIcon* icon = getIcon(id);

	if(icon != NULL) {
		if(IsWindowVisible(icon->getHwnd()) != TRUE)
			ShowWindow(icon->getHwnd(), SW_SHOWNORMAL);
	}
}

CdrIcon* CdrIconOps::getIcon(enum ResourceID id)
{
	ssize_t index = mPic.indexOfKey(id);
	if(index < 0) {
		db_error("key %d not find\n", id);
		return NULL;
	} else {
		return mPic.valueAt(index);
	}
}

void CdrIconOps::setIconIndex(enum ResourceID id, int value)
{
	CdrIcon* icon = getIcon(id);

	if(icon != NULL) {
		icon->setIconIndex(value);
	}
}

void CdrIconOps::updateIconIndex(enum ResourceID id)
{
	CdrIcon* icon = getIcon(id);

	if(icon != NULL) {
		icon->updateIconIndex();
	}
}

size_t CdrIconOps::size(void)
{
	return mPic.size();
}

unsigned int CdrIconOps::getIconIndex(enum ResourceID id)
{
	CdrIcon* icon = getIcon(id);

	if(icon != NULL) {
		return icon->getIconIndex();
	}

	return 0;
}

void CdrIconOps::setBkgroud(enum ResourceID resId, int bkgroudId)
{
	CdrIcon* icon = getIcon(resId);

	if(icon != NULL) {
		return icon->setBkgroud(bkgroudId);;
	}
}

void CdrIconOps::unsetBkgroud(ResourceID resId)
{
	CdrIcon* icon = getIcon(resId);

	if(icon != NULL) {
		return icon->unsetBkgroud();;
	}
}
