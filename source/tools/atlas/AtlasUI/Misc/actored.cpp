// Just a boring app that creates an ActorEditor window

#include "stdafx.h"

#include "ActorEditor/ActorEditor.h"

#include "wx/app.h"

class MyApp: public wxApp
{
	bool OnInit();
};

IMPLEMENT_APP(MyApp)

#include <crtdbg.h>

bool MyApp::OnInit()
{
//	_CrtSetBreakAlloc(1358);
//	MyFrame *frame = new MyFrame("Hello World");
	wxFrame *frame = new ActorEditor(NULL);
	frame->Show();
	SetTopWindow(frame);
	return true;
}
