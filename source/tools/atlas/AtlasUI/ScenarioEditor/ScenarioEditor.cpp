#include "stdafx.h"

#include "ScenarioEditor.h"

#include "wx/glcanvas.h"
#include "CustomControls/SnapSplitterWindow/SnapSplitterWindow.h"

#include "GameInterface/MessagePasser.h"
#include "GameInterface/Messages.h"

#include "Sections/Map/Map.h"

//#define UI_ONLY

//////////////////////////////////////////////////////////////////////////

class Canvas : public wxGLCanvas
{
public:
	Canvas(wxWindow* parent, int* attribList)
		: wxGLCanvas(parent, -1, wxDefaultPosition, wxDefaultSize, 0, _T("GLCanvas"), attribList),
		m_SuppressResize(true)
	{
	}

	void OnResize(wxSizeEvent&)
	{
		// Be careful not to send 'resize' messages to the game before we've
		// told it that this canvas exists
		if (! m_SuppressResize)
			AtlasMessage::g_MessagePasser->Add(new AtlasMessage::mResizeScreen(GetSize().GetWidth(), GetSize().GetHeight()));
	}

	void InitSize()
	{
		m_SuppressResize = false;
		SetSize(320, 240);
	}

private:
	bool m_SuppressResize;
	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(Canvas, wxGLCanvas)
	EVT_SIZE(Canvas::OnResize)
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(ScenarioEditor, wxFrame)
	EVT_CLOSE(ScenarioEditor::OnClose)
END_EVENT_TABLE()


ScenarioEditor::ScenarioEditor()
: wxFrame(NULL, wxID_ANY, _("Atlas - Scenario Editor"), wxDefaultPosition, wxSize(1024, 768))
{
	SnapSplitterWindow* splitter = new SnapSplitterWindow(this);

	// Set up GL canvas:

	int glAttribList[] = {
		WX_GL_RGBA,
		WX_GL_DOUBLEBUFFER,
		WX_GL_DEPTH_SIZE, 24, // TODO: wx documentation doesn't say this is valid
		0
	};
	Canvas* canvas = new Canvas(splitter, glAttribList);
	// The canvas' context gets made current on creation; but it can only be
	// current for one thread at a time, and it needs to be current for the
	// thread that is doing the draw calls, so disable it for this one.
	wglMakeCurrent(NULL, NULL);

	// Set up sidebars:

	Sidebar* sidebar = new MapSidebar(splitter);

	// Build layout:

	splitter->SplitVertically(sidebar, canvas, 200);

	// Send setup messages to game engine:

#ifndef UI_ONLY
	ADD_MESSAGE(SetContext(canvas->GetHDC(), canvas->GetContext()->GetGLRC()));

	ADD_MESSAGE(CommandString("init"));

	canvas->InitSize();

	ADD_MESSAGE(CommandString("render_enable"));
#endif
}

void ScenarioEditor::OnClose(wxCloseEvent&)
{
#ifndef UI_ONLY
	ADD_MESSAGE(CommandString("shutdown"));
#endif
	ADD_MESSAGE(CommandString("exit"));
	
	// TODO: What if it's still rendering while we're destroying the canvas?
	Destroy();
}
