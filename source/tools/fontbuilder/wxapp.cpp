#include "stdafx.h"

#include "wx/app.h"

#include "wxframe.h"
#include "wxconfig.h"

#include "version.h"

class FontBuilderApp : public wxApp
{
public:
	virtual bool OnInit();
	virtual int OnExit();
};


IMPLEMENT_APP(FontBuilderApp)

bool FontBuilderApp::OnInit()
{
	ConfigInit();

	MainFrame *frame = new MainFrame(wxString::Format(wxT("Font Builder %s"), (const wchar_t*)version), wxDefaultPosition, wxSize(640,480));
	frame->SetIcon(wxIcon(wxT("IDI_ICON1")));
	frame->Show(true);
	SetTopWindow(frame);

	return true;
}


extern void FreeDLLs();

int FontBuilderApp::OnExit()
{
	ConfigDestroy();
	FreeDLLs();
	return 0;
}
