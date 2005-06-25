#include "stdafx.h"

#include "ScenarioEditor.h"

#include "wx/glcanvas.h"

#include "GameInterface/MessageHandler.h"
#include "GameInterface/Messages.h"

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
		if (! m_SuppressResize)
			AtlasMessage::g_MessageHandler->Add(new AtlasMessage::mResizeScreen(GetSize().GetWidth(), GetSize().GetHeight()));
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
: wxFrame(NULL, wxID_ANY, _("Atlas - Scenario Editor"))
{
	wxPanel* panel = new wxPanel(this);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	int glAttribList[] = {
		WX_GL_RGBA,
		WX_GL_DOUBLEBUFFER,
		WX_GL_DEPTH_SIZE, 24,
		0
	};
	Canvas* canvas = new Canvas(panel, glAttribList);
	// The canvas' context gets made current on creation; but it can only be
	// current for one thread at a time, and it needs to be current for the
	// thread that is doing the draw calls, so disable it for this one.
	wglMakeCurrent(NULL, NULL);

	sizer->Add(canvas, wxSizerFlags().Proportion(1).Expand());

	AtlasMessage::g_MessageHandler->Add(new AtlasMessage::mSetContext(canvas->GetHDC(), canvas->GetContext()->GetGLRC()));

	AtlasMessage::g_MessageHandler->Add(new AtlasMessage::mCommandString("init"));

	canvas->InitSize();

	AtlasMessage::g_MessageHandler->Add(new AtlasMessage::mCommandString("render_enable"));
}

void ScenarioEditor::OnClose(wxCloseEvent&)
{
	AtlasMessage::g_MessageHandler->Add(new AtlasMessage::mCommandString("shutdown"));
	AtlasMessage::g_MessageHandler->Add(new AtlasMessage::mCommandString("exit"));
	
	// TODO: What if it's still rendering while we're destroying the canvas?
	Destroy();
}
