
#include "CdrLabel.h"
#include "cdr_widgets.h"
#undef LOG_TAG
#define LOG_TAG "CdrLabel"
#include "debug.h"

using android::String8;
static WNDPROC oldProc = NULL;
static int cdrLabelProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
	CdrLabel* cdrLabel;

	cdrLabel = (CdrLabel*)GetWindowAdditionalData(hWnd);
	//db_msg("message code: [%d], %s, wParam=%d, lParam=0x%lx\n", message, Message2Str(message), wParam, lParam);

	if(cdrLabel != NULL)
		return cdrLabel->windowProc(hWnd, message, wParam, lParam);

	return 0;
}

int CdrLabel::windowProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
	//db_msg("id [%d], name: %s, message code: [%d], %s\n", mId, mName.string(), message, Message2Str(message));
	switch(message) {
	case MSG_PAINT:
		{
			HDC hdc;
			RECT rect;
			ResourceManager* rm;
			rm = ResourceManager::getInstance();
			GetClientRect(hWnd, &rect);
			hdc = BeginPaint(hWnd);
			if(mBkgroudBmp.bmBits != NULL)
				FillBoxWithBitmap (hdc, 0, 0, RECTW(rect), RECTH(rect), &mBkgroudBmp );

			SetBkMode(hdc, BM_TRANSPARENT);
			SelectFont(hdc, rm->getLogFont());//new add
			SetTextColor(hdc, mTextColor );
			DrawText( hdc, mString.string(), -1, &rect, mAlignFormat);

			EndPaint(hWnd, hdc);

			return 0;
		}
		break;
	default:
		break;
	}

	if(oldProc != NULL)
		return (*oldProc)(hWnd, message, wParam, lParam);

	return 0;
}

CdrLabel::CdrLabel(CdrLabel_t* labelData , XCreateParams* createParam ) :
	mHwnd(HWND_INVALID),
	mHparent(createParam->hParent),
	mId(labelData->id),
	mName(String8(labelData->name)),
	mAlign(alCenter),
	mAlignFormat(DT_CENTER |DT_VCENTER |DT_SINGLELINE),
	mTextColor(PIXEL_lightwhite)
{
	DWORD dwStyle, dwStyleEx;
	ResourceManager* rm;
	int retval;
	CDR_RECT rect;

	db_msg("CdrLabel Constructor: %s\n", mName.string());
	dwStyle = WS_CHILD;		// not visible
	dwStyleEx = WS_EX_NONE;

	dwStyle |= createParam->style;
	dwStyleEx |= createParam->exStyle;

	memset(&mBkgroudBmp, 0, sizeof(BITMAP));

	rm = ResourceManager::getInstance();
	retval = rm->getResRect(mId, rect);
	if(retval < 0) {
		db_error("get %s rect failed\n", mName.string());
		return;
	}
	db_msg("%d %d %d %d", rect.x, rect.y, rect.w, rect.h);

	mHwnd = CreateWindowEx(CTRL_STATIC, "",
			dwStyle,
			dwStyleEx,
			mId,
			rect.x, rect.y, rect.w, rect.h,
			mHparent, (DWORD)this);
	if(mHwnd == HWND_INVALID) {
		db_error("create status bar window name failed\n");
		return;
	}

	oldProc = SetWindowCallbackProc(mHwnd, cdrLabelProc);
}

CdrLabel::~CdrLabel()
{
	db_msg("CdrLabel Destructor, mId %d\n", mId);
}


HWND CdrLabel::getHwnd(void)
{
	return mHwnd;
}

String8 CdrLabel::getName(void)
{
	return mName;
}

void CdrLabel::setBkgroudBmp(PBITMAP pBmp)
{
	mBkgroudBmp = *pBmp;
	InvalidateRect(mHwnd, NULL, TRUE);
}

void CdrLabel::setTextColor(gal_pixel color)
{
	mTextColor = color;
}

void CdrLabel::setText(const char* string)
{
	RECT rect;
	if(string != NULL) {
		mString = String8(string);

		if(mHwnd != 0 && mHwnd != HWND_INVALID) {
			GetClientRect(mHwnd, &rect);
			InvalidateRect(mHwnd, &rect, TRUE);
		}
	}
}

void CdrLabel::setText(int value)
{
	char buf[25];

	snprintf(buf, sizeof(buf), "%d", value);

	setText( buf );
}

void CdrLabel::setText(double value)
{
	char buf[25];

	snprintf(buf, sizeof(buf), "%lf", value);

	setText( buf );
}

void CdrLabel::setAlign(ALIGN aValue)
{

	if (mAlign != aValue) {
		switch(aValue) {
		case alLeft:
			mAlignFormat = DT_LEFT | DT_VCENTER | DT_SINGLELINE;
			break;
		case alRight:
			mAlignFormat = DT_RIGHT | DT_VCENTER | DT_SINGLELINE;
			break;
		case alCenter:
			mAlignFormat = DT_CENTER |DT_VCENTER |DT_SINGLELINE;
			break;
		default:
			return;
		}
		mAlign = aValue;

		RECT rect;
		if(mHwnd != 0 && mHwnd != HWND_INVALID) {
			GetClientRect(mHwnd, &rect);
			InvalidateRect(mHwnd, &rect, TRUE);
		}
	}
}

void CdrLabel::setBkgroud(int id)
{
	ResourceManager* rm;

	rm = ResourceManager::getInstance();

	rm->getResBkgroundBmp(id, mBkgroudBmp);
	InvalidateRect(mHwnd, NULL, TRUE);
}

void CdrLabel::unsetBkgroud(void)
{
	if(mBkgroudBmp.bmBits != NULL) {
		UnloadBitmap(&mBkgroudBmp);
		InvalidateRect(mHwnd, NULL, TRUE);
	}
}

// ---------------------------------------------------------------------------

CdrLabelOps::CdrLabelOps()
{
	mLabel.clear();
}

CdrLabelOps::~CdrLabelOps()
{
	for(unsigned int i = 0; i< mLabel.size(); i++) {
		if( mLabel.valueAt(i) != NULL)
			delete mLabel.valueAt(i);
	}
	mLabel.clear();
}

int CdrLabelOps::createCdrLabel(CdrLabel::CdrLabel_t* labelData, XCreateParams* createParam)
{
	CdrLabel* label;

	label = new CdrLabel(labelData, createParam );
	if(label->getHwnd() == HWND_INVALID) {
		db_error("createLabel %s failed\n", labelData->name );
		return -1;
	}	

	mLabel.add(labelData->id, label);

	return 0;
}

void CdrLabelOps::hideLabel(enum ResourceID id)
{
	CdrLabel* label = getLabel(id);

	if(label != NULL) {
		if(IsWindowVisible(label->getHwnd()) == TRUE)
			ShowWindow(label->getHwnd(), SW_HIDE);
	}
}

void CdrLabelOps::showLabel(enum ResourceID id)
{
	CdrLabel* label = getLabel(id);

	if(label != NULL) {
		if(IsWindowVisible(label->getHwnd()) != TRUE)
			ShowWindow(label->getHwnd(), SW_SHOWNORMAL);
	}
}
	
CdrLabel* CdrLabelOps::getLabel(enum ResourceID id)
{
	ssize_t index = mLabel.indexOfKey(id);
	if(index < 0) {
		db_error("key %d not find\n", id);
		return NULL;
	} else {
		return mLabel.valueAt(index);
	}
}
	
size_t CdrLabelOps::size(void)
{
	return mLabel.size();
}


