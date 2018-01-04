#ifndef _CDR_DISPLAY_H
#include <CedarDisplay.h>

#include "misc.h"

typedef struct view_info ViewInfo;

using namespace android;
class CdrDisplay
{
public:
	CdrDisplay(int disp_id, ViewInfo *sur);
	CdrDisplay(int disp_id);
	~CdrDisplay();
	int getHandle();
	void setBottom();
	void setRect(CDR_RECT &rect);
	void open(int hlay=-1);
	void close(int hlay=-1);
	void openAdasScreen();
	void closeAdasScreen();
	void exchange(int hlay, int flag); //flag:1-otherOnTop
	void otherScreen(int screen, int hlay);
	void clean(void);
private:
	CedarDisplay* mCD;
	int mHlay;
	bool mLayerOpened;
};

#define _CDR_DISPLAY_H
#endif
