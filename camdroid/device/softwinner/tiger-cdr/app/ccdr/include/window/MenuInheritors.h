/*************************************************************************
  > File Name: MenuInheritors.h
  > Description: 
  >		classes Inherited from Menu::MenuObj
  > Author: CaoDan
  > Mail: caodan2519@gmail.com 
  > Created Time: 2014年12月15日 星期一 19时29分19秒
 ************************************************************************/

#include "windows.h"

namespace android {

class MenuSystem : public Menu::MenuObj, public CommonListener
{
public:
	MenuSystem(class Menu* parent, AvaliableMenu menuObjId);
	~MenuSystem();
	// ------------------ start of pure virtual functions
	virtual int menuObjWindowProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam);
	virtual int keyProc(int keyCode, int isLongPress);
	virtual int HandleSubMenuChange(unsigned int menuIndex, int newSel);
	virtual int HandleSubMenuChange(unsigned int menuIndex, bool newSel);
	// ------------------ end of pure virtual functions

	virtual void finish(int what, int extra);	// pure vitual functions from CommonListener

	// ------------------ start of normal virtual functions
	// ------------------ end of normal virtual functions

public:
	int ShowDateDialog(void);		// normal functions
	int formatTfCard(MessageBox_t* data, bool alphaEffect);		// format tfcard;
	void factoryReset();
	void card_ota_upgrade();
	
	void doFormatTFCard(void);
	void doFactoryReset(void);
	int card_speed_test(TestResult *test_result);
	int ShowFormattingTip(bool alphaEffect);

private:
	void initMenuObjParams(); // init params such as mMenuResourceID, must be called in the constructor

	int getMessageBoxData(unsigned int menuIndex, MessageBox_t &messageBoxData);
	int get_Card_Speed_Data_Box(unsigned int menuIndex, MessageBox_t &messageBoxData,TestResult *test_result);
	bool isFormat;
};

class MenuRecordPreview : public Menu::MenuObj
{
public:
	MenuRecordPreview(class Menu* parent, AvaliableMenu menuObjId);
	~MenuRecordPreview();
	// ------------------ start of pure virtual functions
	virtual int menuObjWindowProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam);
	virtual int keyProc(int keyCode, int isLongPress);
	virtual int HandleSubMenuChange(unsigned int menuIndex, int newSel);
	virtual int HandleSubMenuChange(unsigned int menuIndex, bool newSel);
	// ------------------ end of pure virtual functions

	// ------------------ start of normal virtual functions
	//virtual int getMessageBoxData(unsigned int menuIndex, MessageBox_t &messageBoxData);
	// ------------------ end of normal virtual functions

	int showLicensePlateWM(void);
private:
	void initMenuObjParams(); // init params such as mMenuResourceID, must be called in the constructor
};


class MenuPhoto : public Menu::MenuObj
{
public:
	MenuPhoto(class Menu* parent, AvaliableMenu menuObjId);
	~MenuPhoto();
	// ------------------ start of pure virtual functions
	virtual int menuObjWindowProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam);
	virtual int keyProc(int keyCode, int isLongPress);
	virtual int HandleSubMenuChange(unsigned int menuIndex, int newSel);
	virtual int HandleSubMenuChange(unsigned int menuIndex, bool newSel);
	// ------------------ end of pure virtual functions

private:

	void initMenuObjParams(); // init params such as mMenuResourceID, must be called in the constructor
};


} // end of namespace android
