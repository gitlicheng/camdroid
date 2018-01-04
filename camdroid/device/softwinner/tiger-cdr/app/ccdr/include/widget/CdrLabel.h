
#ifndef __CDRLABEL_H__
#define __CDRLABEL_H__

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include <utils/String8.h>

#include "misc.h"
#include "cdr_widgets.h"
#include "ResourceManager.h"

class CdrLabel 
{
public:
	enum ALIGN
	{
		alLeft,
		alRight,
		alCenter
	};

	typedef struct __CdrLabel_t
	{
		enum ResourceID id;
		const char *name;
	}CdrLabel_t;

	CdrLabel(CdrLabel_t* labelData, XCreateParams* createParam);
	virtual ~CdrLabel();

	HWND getHwnd(void);
	android::String8 getName(void);
	void setBkgroudBmp(PBITMAP pBmp);
	void setTextColor(gal_pixel color);
	void setText(const char* string);
	void setText(int value);
	void setText(double value);
	void setAlign(ALIGN aValue);
	void setBkgroud(int id);
	void unsetBkgroud();

	int windowProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam);
private:
	HWND mHwnd;
	HWND mHparent;
	ResourceID mId;
	android::String8 mName;
	ALIGN mAlign;
	unsigned int mAlignFormat;
	BITMAP mBkgroudBmp;
	gal_pixel mTextColor;
	android::String8 mString;
};

class CdrLabelOps
{
public:
	CdrLabelOps();
	~CdrLabelOps();

	int createCdrLabel(CdrLabel::CdrLabel_t* labelData, XCreateParams* createParam);
	void hideLabel(enum ResourceID);
	void showLabel(enum ResourceID);
	CdrLabel* getLabel(enum ResourceID);
	size_t size(void);

protected:
	android::KeyedVector<ResourceID, CdrLabel*> mLabel;
};

#endif
