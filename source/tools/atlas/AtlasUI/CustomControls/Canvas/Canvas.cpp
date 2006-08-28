#include "stdafx.h"

#include "Canvas.h"

#include "GameInterface/Messages.h"
#include "ScenarioEditor/tools/Common/Tools.h"

Canvas::Canvas(wxWindow* parent, int* attribList, long style)
	: wxGLCanvas(parent, -1, wxDefaultPosition, wxDefaultSize, style, _T("GLCanvas"), attribList),
	m_SuppressResize(true),
	m_LastMousePos(-1, -1), m_MouseCaptured(false)
{
}

void Canvas::OnResize(wxSizeEvent&)
{
	// Be careful not to send 'resize' messages to the game before we've
	// told it that this canvas exists
	if (! m_SuppressResize)
		POST_MESSAGE(ResizeScreen, (GetClientSize().GetWidth(), GetClientSize().GetHeight()));
		// TODO: fix flashing
}

void Canvas::InitSize()
{
	m_SuppressResize = false;
	SetSize(320, 240);
}


void Canvas::OnMouseCapture(wxMouseCaptureChangedEvent& WXUNUSED(evt))
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

void Canvas::OnMouse(wxMouseEvent& evt)
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

	// Reject motion events if the mouse has not actually moved
	if (evt.Moving() || evt.Dragging())
	{
		if (m_LastMousePos == evt.GetPosition())
			return;
		m_LastMousePos = evt.GetPosition();
	}

	HandleMouseEvent(evt);
}

BEGIN_EVENT_TABLE(Canvas, wxGLCanvas)
	EVT_SIZE(Canvas::OnResize)
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
