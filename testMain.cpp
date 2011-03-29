#include "MBGlobal.h"
#include "MApplication.h"
#include "MWidget.h"
#include "vld.h"

#include <iostream>
#include <windows.h>
#include <tchar.h>

#include "test/test_slot.cpp"
#include "test/test_widget.cpp"
#include "test/test_stylesheet.cpp"

#ifdef MSVC
int WINAPI _tWinMain(HINSTANCE,HINSTANCE,PTSTR,int)
#else
int main(int argc,char** argv)
#endif
{
//	testDisplay();
//	testWidgetclose();
//	test_resource();
	testCSSBackgroundRenderObject();
	return 0;
}
