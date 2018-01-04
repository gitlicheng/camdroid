#include "CdrDisplay.h"
#undef LOG_TAG
#define LOG_TAG "CdrDisplay.cpp"
#include "debug.h"
#include "windows.h"
CdrDisplay::CdrDisplay(int disp_id,    ViewInfo *sur)
{
	mCD = new CedarDisplay(disp_id);
	mHlay = mCD->requestSurface(sur);
	mLayerOpened = true;
}

int CdrDisplay::getHandle()
{
	return mHlay;	
}

CdrDisplay::CdrDisplay(int disp_id)
{
	ViewInfo sur;
	memset(&sur, 0, sizeof(ViewInfo));
	CdrDisplay(disp_id, &sur);
}

CdrDisplay::~CdrDisplay()
{
	db_msg("CdrDisplay Destructor\n");
	mCD->releaseSurface(mHlay);
	delete mCD;
	mCD = NULL;
}

void CdrDisplay::setBottom()
{
	db_msg("there is no need setBottom in V3");
#ifndef DE2
	db_msg("setBottom");
	mCD->setPreviewBottom(mHlay);
#endif
}

void CdrDisplay::setRect(CDR_RECT &rect)
{
	ViewInfo vi;
	if(mCD) {
		vi.x = rect.x;
		vi.y = rect.y;
		vi.w = rect.w;
		vi.h = rect.h;
		mCD->setPreviewRect(&vi);
	} else {
		db_error("mCD is invalid\n");	
	}
}

void CdrDisplay::open(int hlay)
{
	if (hlay < 0) {
		hlay = mHlay;
	}
	if(mLayerOpened == true && hlay == mHlay)
		return;
	if (mCD) {
		int ret;
		ret = mCD->open(hlay, 1);
		db_msg("ret is %d\n", ret);
		if(ret == 0 && hlay== mHlay)
			mLayerOpened = true;
	}
}

void CdrDisplay::openAdasScreen()
{
	db_msg("<**CdrDisplay::openAdasScreen**>");
	if (mCD) {
		int ret;
		ret = mCD->open(9, 1);
		db_msg("ret is %d\n", ret);
		if(ret != 0)
			db_msg("<****openAdasScreen failed****>");
	}
}

void CdrDisplay::close(int hlay)
{
	if (hlay < 0) {
		hlay = mHlay;
	}
//	if(mLayerOpened == false)
//		return;
	if (mCD) {
		int ret;
		ret = mCD->open(hlay, 0);
		db_msg("ret is %d\n", ret);
		if(ret == 0 && hlay== mHlay)
			mLayerOpened = false;
	}
}

void CdrDisplay::closeAdasScreen()
{
	db_msg("<**CdrDisplay::closeAdasScreen**>");
	if (mCD) {
		int ret;
		ret = mCD->open(9, 0);
		db_msg("ret is %d\n", ret);
		if(ret != 0)
			db_msg("<****closeAdasScreen failed****>");
	}
}

void CdrDisplay::exchange(int hlay, int flag)
{
	mCD->exchangeSurface(mHlay, hlay, flag);
}

void CdrDisplay::otherScreen(int screen, int hlay)
{
	mCD->otherScreen(screen, mHlay, hlay);
}

void CdrDisplay::clean(void)
{
	mCD->clearSurface(mHlay);
}