#include "stdafx.h"

#include "ScenarioEditor.h"

#include "wx/glcanvas.h"
#include "wx/evtloop.h"
#include "wx/tooltip.h"

#include "SnapSplitterWindow/SnapSplitterWindow.h"
#include "HighResTimer/HighResTimer.h"
#include "Buttons/ToolButton.h"

#include "GameInterface/MessagePasser.h"
#include "GameInterface/Messages.h"

#include "tools/Common/Tools.h"

//#define UI_ONLY

static HighResTimer g_Timer;

//////////////////////////////////////////////////////////////////////////

// TODO: move into another file
class Canvas : public wxGLCanvas
{
public:
	Canvas(wxWindow* parent, int* attribList)
		: wxGLCanvas(parent, -1, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS, _T("GLCanvas"), attribList),
		m_SuppressResize(true),
		m_MouseState(NONE), m_LastMouseState(NONE), m_MouseCaptured(false),
		m_LastMousePos(-1, -1)
	{
	}

	void OnResize(wxSizeEvent&)
	{
#ifndef UI_ONLY
		// Be careful not to send 'resize' messages to the game before we've
		// told it that this canvas exists
		if (! m_SuppressResize)
			POST_MESSAGE(ResizeScreen, (GetClientSize().GetWidth(), GetClientSize().GetHeight()));
			// TODO: fix flashing
#endif // UI_ONLY
	}

	void InitSize()
	{
		m_SuppressResize = false;
		SetSize(320, 240);
	}

	bool KeyScroll(wxKeyEvent& evt, bool enable)
	{
#ifndef UI_ONLY
		int dir;
		switch (evt.GetKeyCode())
		{
		case WXK_LEFT:  dir = AtlasMessage::eScrollConstantDir::LEFT; break;
		case WXK_RIGHT: dir = AtlasMessage::eScrollConstantDir::RIGHT; break;
		case WXK_UP:    dir = AtlasMessage::eScrollConstantDir::FORWARDS; break;
		case WXK_DOWN:  dir = AtlasMessage::eScrollConstantDir::BACKWARDS; break;
		case WXK_SHIFT: case WXK_CONTROL: dir = -1; break;
		default: return false;
		}

		float speed = 120.f;
		if (wxGetKeyState(WXK_SHIFT) && wxGetKeyState(WXK_CONTROL))
			speed /= 64.f;
		else if (wxGetKeyState(WXK_CONTROL))
			speed /= 4.f;
		else if (wxGetKeyState(WXK_SHIFT))
			speed *= 4.f;

		if (dir == -1) // changed modifier keys - update all currently-scrolling directions
		{
			if (wxGetKeyState(WXK_LEFT))  POST_MESSAGE(ScrollConstant, (AtlasMessage::eScrollConstantDir::LEFT, speed));
			if (wxGetKeyState(WXK_RIGHT)) POST_MESSAGE(ScrollConstant, (AtlasMessage::eScrollConstantDir::RIGHT, speed));
			if (wxGetKeyState(WXK_UP))    POST_MESSAGE(ScrollConstant, (AtlasMessage::eScrollConstantDir::FORWARDS, speed));
			if (wxGetKeyState(WXK_DOWN))  POST_MESSAGE(ScrollConstant, (AtlasMessage::eScrollConstantDir::BACKWARDS, speed));
		}
		else
		{
			POST_MESSAGE(ScrollConstant, (dir, enable ? speed : 0.0f));
		}
#endif // UI_ONLY
		return true;
	}

	void OnKeyDown(wxKeyEvent& evt)
	{
		if (GetCurrentTool().OnKey(evt, ITool::KEY_DOWN))
		{
			// Key event has been handled by the tool, so don't try
			// to use it for camera motion too
			return;
		}

		if (KeyScroll(evt, true))
			return;

		evt.Skip();
	}

	void OnKeyUp(wxKeyEvent& evt)
	{
		if (GetCurrentTool().OnKey(evt, ITool::KEY_UP))
			return;

		if (KeyScroll(evt, false))
			return;

		evt.Skip();
	}

	void OnChar(wxKeyEvent& evt)
	{
		if (GetCurrentTool().OnKey(evt, ITool::KEY_CHAR))
			return;

		int dir = 0;
		if (evt.GetKeyCode() == '-' || evt.GetKeyCode() == '_')
			dir = -1;
		else if (evt.GetKeyCode() == '+' || evt.GetKeyCode() == '=')
			dir = +1;
		// TODO: internationalisation (-/_ and +/= don't always share a key)

		if (dir)
		{
			float speed = 16.f;
			if (wxGetKeyState(WXK_SHIFT))
				speed *= 4.f;
			POST_MESSAGE(SmoothZoom, (speed*dir));
		}
		else
			evt.Skip();

	}

	void OnMouseCapture(wxMouseCaptureChangedEvent& WXUNUSED(evt))
	{
		if (m_MouseCaptured)
		{
			// unexpected loss of capture (i.e. not through ReleaseMouse)
			m_MouseCaptured = false;
			
			// (Note that this can be made to happen easily, by alt-tabbing away,
			// and mouse events will be missed; so it is never guaranteed that e.g.
			// two LeftDown events will be separated by a LeftUp.)
		}
	}

	void OnMouse(wxMouseEvent& evt)
	{
		// Capture on button-down, so we can respond even when the mouse
		// moves off the window
		if (!m_MouseCaptured && evt.ButtonDown())
		{
			m_MouseCaptured = true;
			CaptureMouse();
		}
		// Un-capture when all buttons are up
		else if (m_MouseCaptured && evt.ButtonUp() &&
			! (evt.ButtonIsDown(wxMOUSE_BTN_LEFT) || evt.ButtonIsDown(wxMOUSE_BTN_MIDDLE) || evt.ButtonIsDown(wxMOUSE_BTN_RIGHT))
			)
		{
			m_MouseCaptured = false;
			ReleaseMouse();
		}

		// Set focus when clicking
		if (evt.ButtonDown())
			SetFocus();

		// TODO or at least to think about: When using other controls in the
		// editor, it's annoying that keyboard/scrollwheel no longer navigate
		// around the world until you click on it.
		// Setting focus back whenever the mouse moves over the GL window
		// feels like a fairly natural solution to me, since I can use
		// e.g. brush-editing controls normally, and then move the mouse to
		// see the brush outline and magically get given back full control
		// of the camera.
		if (evt.Moving())
			SetFocus();

		// Reject motion events if the mouse has not actually moved
		if (evt.Moving() || evt.Dragging())
		{
			if (m_LastMousePos == evt.GetPosition())
				return;
			m_LastMousePos = evt.GetPosition();
		}

#ifndef UI_ONLY

		if (GetCurrentTool().OnMouse(evt))
		{
			// Mouse event has been handled by the tool, so don't try
			// to use it for camera motion too
			return;
		}

		if (evt.GetWheelRotation())
		{
			float speed = 16.f;
			if (wxGetKeyState(WXK_SHIFT) && wxGetKeyState(WXK_CONTROL))
				speed /= 64.f;
			else if (wxGetKeyState(WXK_CONTROL))
				speed /= 4.f;
			else if (wxGetKeyState(WXK_SHIFT))
				speed *= 4.f;

			POST_MESSAGE(SmoothZoom, (evt.GetWheelRotation() * speed / evt.GetWheelDelta()));
		}
		else
		{
			if (evt.MiddleIsDown())
			{
				if (wxGetKeyState(WXK_CONTROL))
					m_MouseState = ROTATEAROUND;
				else
					m_MouseState = SCROLL;
			}
			else
				m_MouseState = NONE;

			if (m_MouseState != m_LastMouseState)
			{
				switch (m_MouseState)
				{
				case NONE: break;
				case SCROLL: POST_MESSAGE(Scroll, (AtlasMessage::eScrollType::FROM, evt.GetPosition())); break;
				case ROTATEAROUND: POST_MESSAGE(RotateAround, (AtlasMessage::eRotateAroundType::FROM, evt.GetPosition())); break;
				default: wxFAIL;
				}
				m_LastMouseState = m_MouseState;
			}
			else if (evt.Dragging())
			{
				switch (m_MouseState)
				{
				case NONE: break;
				case SCROLL: POST_MESSAGE(Scroll, (AtlasMessage::eScrollType::TO, evt.GetPosition())); break;
				case ROTATEAROUND: POST_MESSAGE(RotateAround, (AtlasMessage::eRotateAroundType::TO, evt.GetPosition())); break;
				default: wxFAIL;
				}
			}
		}
#endif // UI_ONLY
	}

private:
	bool m_SuppressResize;

	enum { NONE, SCROLL, ROTATEAROUND };
	int m_MouseState, m_LastMouseState;
	bool m_MouseCaptured;

	wxPoint m_LastMousePos;

	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(Canvas, wxGLCanvas)
	EVT_SIZE      (Canvas::OnResize)
	EVT_KEY_DOWN  (Canvas::OnKeyDown)
	EVT_KEY_UP    (Canvas::OnKeyUp)
	EVT_CHAR      (Canvas::OnChar)
	
	EVT_LEFT_DOWN  (Canvas::OnMouse)
	EVT_LEFT_UP    (Canvas::OnMouse)
	EVT_RIGHT_DOWN (Canvas::OnMouse)
	EVT_RIGHT_UP   (Canvas::OnMouse)
	EVT_MIDDLE_DOWN(Canvas::OnMouse)
	EVT_MIDDLE_UP  (Canvas::OnMouse)
	EVT_MOUSEWHEEL (Canvas::OnMouse)
	EVT_MOTION     (Canvas::OnMouse)
	EVT_MOUSE_CAPTURE_CHANGED(Canvas::OnMouseCapture)
END_EVENT_TABLE()

// GL functions exported from DLL, and called by game (in a separate
// thread to the standard wx one)
ATLASDLLIMPEXP void Atlas_GLSetCurrent(void* context)
{
	((wxGLContext*)context)->SetCurrent();
}

ATLASDLLIMPEXP void Atlas_GLSwapBuffers(void* context)
{
	((wxGLContext*)context)->SwapBuffers();
}


//////////////////////////////////////////////////////////////////////////


volatile bool g_FrameHasEnded;
// Called from game thread
ATLASDLLIMPEXP void Atlas_NotifyEndOfFrame()
{
	g_FrameHasEnded = true;
}

enum
{
	ID_Quit = 1,
//	ID_New,
//	//	ID_Import,
//	//	ID_Export,
//	ID_Open,
//	ID_Save,
//	ID_SaveAs,

	ID_Wireframe,
	ID_MessageTrace,
	ID_Screenshot,

	ID_Toolbar // must be last in the list
};

BEGIN_EVENT_TABLE(ScenarioEditor, wxFrame)
	EVT_CLOSE(ScenarioEditor::OnClose)
	EVT_TIMER(wxID_ANY, ScenarioEditor::OnTimer)

	EVT_MENU(ID_Quit, ScenarioEditor::OnQuit)
	EVT_MENU(wxID_UNDO, ScenarioEditor::OnUndo)
	EVT_MENU(wxID_REDO, ScenarioEditor::OnRedo)

	EVT_MENU(ID_Wireframe, ScenarioEditor::OnWireframe)
	EVT_MENU(ID_MessageTrace, ScenarioEditor::OnMessageTrace)
	EVT_MENU(ID_Screenshot, ScenarioEditor::OnScreenshot)

	EVT_IDLE(ScenarioEditor::OnIdle)
END_EVENT_TABLE()


static AtlasWindowCommandProc g_CommandProc;
AtlasWindowCommandProc& ScenarioEditor::GetCommandProc() { return g_CommandProc; }

ScenarioEditor::ScenarioEditor(wxWindow* parent)
: wxFrame(parent, wxID_ANY, _("Atlas - Scenario Editor"), wxDefaultPosition, wxSize(1024, 768))
{
	// Global application initialisation:
	//	wxLog::SetTraceMask(wxTraceMessages);
	SetIcon(wxIcon(_T("ICON_ScenarioEditor")));
	wxToolTip::Enable(true);
	wxImage::AddHandler(new wxPNGHandler);

	//////////////////////////////////////////////////////////////////////////
	// Menu

	wxMenuBar* menuBar = new wxMenuBar;
	SetMenuBar(menuBar);

	wxMenu *menuFile = new wxMenu;
	menuBar->Append(menuFile, _("&File"));
	{
//		menuFile->Append(ID_New, _("&New"));
//		//		menuFile->Append(ID_Import, _("&Import..."));
//		//		menuFile->Append(ID_Export, _("&Export..."));
//		menuFile->Append(ID_Open, _("&Open..."));
//		menuFile->Append(ID_Save, _("&Save"));
//		menuFile->Append(ID_SaveAs, _("Save &As..."));
//		menuFile->AppendSeparator();//-----------
		menuFile->Append(ID_Quit,   _("E&xit"));
//		m_FileHistory.UseMenu(menuFile);//-------
//		m_FileHistory.AddFilesToMenu();
	}

//	m_menuItem_Save = menuFile->FindItem(ID_Save); // remember this item, to let it be greyed out
//	wxASSERT(m_menuItem_Save);

	wxMenu *menuEdit = new wxMenu;
	menuBar->Append(menuEdit, _("&Edit"));
	{
		menuEdit->Append(wxID_UNDO, _("&Undo"));
		menuEdit->Append(wxID_REDO, _("&Redo"));
	}

	GetCommandProc().SetEditMenu(menuEdit);
	GetCommandProc().Initialize();


	wxMenu *menuMisc = new wxMenu;
	menuBar->Append(menuMisc, _("&Misc hacks"));
	{
		menuMisc->AppendCheckItem(ID_Wireframe, _("&Wireframe"));
		menuMisc->AppendCheckItem(ID_MessageTrace, _("Message debug trace"));
		menuMisc->Append(ID_Screenshot, _("&Screenshot"));
	}

	// Toolbar:

	ToolButtonBar* toolbar = new ToolButtonBar(this, ID_Toolbar);
	// TODO: configurable small vs large icon images
	// (button label; tooltip text; image; internal tool name)
	toolbar->AddToolButton(_("Default"), _("Default"), _T("default.png"), _T(""));
	toolbar->AddToolButton(_("Move"), _("Move/rotate object"), _T("moveobject.png"), _T("TransformObject"));
	toolbar->AddToolButton(_("Elevation"), _("Alter terrain elevation"), _T("alterelevation.png"), _T("AlterElevation"));
	toolbar->AddToolButton(_("Flatten"), _("Flatten terrain elevation"), _T("flattenelevation.png"), _T("FlattenElevation"));
	toolbar->AddToolButton(_("Paint Terrain"), _("Paint terrain texture"), _T("paintterrain.png"), _T("PaintTerrain"));
	toolbar->Realize();
	SetToolBar(toolbar);
	// Set the default tool to be selected
	SetCurrentTool(_T(""));

	//////////////////////////////////////////////////////////////////////////
	// Main window

	m_SectionLayout.SetWindow(this);

	// Set up GL canvas:

	int glAttribList[] = {
		WX_GL_RGBA,
		WX_GL_DOUBLEBUFFER,
		WX_GL_DEPTH_SIZE, 24, // TODO: wx documentation doesn't say 24 is valid
		WX_GL_BUFFER_SIZE, 24, // colour bits
		WX_GL_MIN_ALPHA, 8, // alpha bits
		0
	};
	Canvas* canvas = new Canvas(m_SectionLayout.GetCanvasParent(), glAttribList);
	m_SectionLayout.SetCanvas(canvas);
	// The canvas' context gets made current on creation; but it can only be
	// current for one thread at a time, and it needs to be current for the
	// thread that is doing the draw calls, so disable it for this one.
	wglMakeCurrent(NULL, NULL);

	// Set up sidebars:

	m_SectionLayout.Build();

	// Send setup messages to game engine:

#ifndef UI_ONLY
	POST_MESSAGE(SetContext, (canvas->GetContext()));

	POST_MESSAGE(CommandString, ("init"));

	canvas->InitSize();

	// Start with a blank map (so that the editor can assume there's always
	// a valid map loaded)
	POST_MESSAGE(GenerateMap, (9));

	POST_MESSAGE(CommandString, ("render_enable"));
#endif

	// Set up a timer to make sure tool-updates happen frequently (in addition
	// to the idle handler (which makes them happen more frequently if there's nothing
	// else to do))
	m_Timer.SetOwner(this);
	m_Timer.Start(20);
}


void ScenarioEditor::OnClose(wxCloseEvent&)
{
	SetCurrentTool(_T(""));

#ifndef UI_ONLY
	POST_MESSAGE(CommandString, ("shutdown"));
#endif

	AtlasMessage::qExit().Post();
		// blocks until engine has noticed the message, so we won't be
		// destroying the GLCanvas while it's still rendering

	Destroy();
}


static void UpdateTool()
{
	// Don't keep posting events if the game can't keep up
	if (g_FrameHasEnded)
	{
		g_FrameHasEnded = false; // (thread safety doesn't matter here)
		// TODO: Smoother timing stuff?
		static double last = g_Timer.GetTime();
		double time = g_Timer.GetTime();
		GetCurrentTool().OnTick(time-last);
		last = time;
	}
}
void ScenarioEditor::OnTimer(wxTimerEvent&)
{
	UpdateTool();
}
void ScenarioEditor::OnIdle(wxIdleEvent&)
{
	UpdateTool();
}

void ScenarioEditor::OnQuit(wxCommandEvent&)
{
	Close();
}

void ScenarioEditor::OnUndo(wxCommandEvent&)
{
	GetCommandProc().Undo();
}

void ScenarioEditor::OnRedo(wxCommandEvent&)
{
	GetCommandProc().Redo();
}

//////////////////////////////////////////////////////////////////////////

void ScenarioEditor::OnWireframe(wxCommandEvent& event)
{
	POST_MESSAGE(RenderStyle, (event.IsChecked()));
}

void ScenarioEditor::OnMessageTrace(wxCommandEvent& event)
{
	POST_MESSAGE(MessageTrace, (event.IsChecked()));
}

void ScenarioEditor::OnScreenshot(wxCommandEvent& WXUNUSED(event))
{
	POST_MESSAGE(Screenshot, (10));
}

//////////////////////////////////////////////////////////////////////////

AtlasMessage::Position::Position(const wxPoint& pt)
: type(1)
{
	type1.x = pt.x;
	type1.y = pt.y;
}

static void QueryCallback()
{
	// If this thread completely blocked on the semaphore inside Query, it would
	// never respond to window messages, and the system deadlocks if the
	// game tries to display an assertion failure dialog. (See
	// WaitForSingleObject on MSDN.)
	// So, this callback is called occasionally, and gives wx a change to
	// handle messages.

	// This is kind of like wxYield, but without the ProcessPendingEvents -
	// it's enough to make Windows happy and stop deadlocking, without actually
	// calling the event handlers (which could lead to nasty recursion)
	while (wxEventLoop::GetActive()->Pending())
		wxEventLoop::GetActive()->Dispatch();
}
void AtlasMessage::QueryMessage::Post()
{
	g_MessagePasser->Query(this, &QueryCallback);
}
