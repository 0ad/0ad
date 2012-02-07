/* Copyright (C) 2012 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "Canvas.h"

#include "GameInterface/Messages.h"
#include "ScenarioEditor/Tools/Common/Tools.h"

Canvas::Canvas(wxWindow* parent, int* attribList, long style)
	: wxGLCanvas(parent, -1, attribList, wxDefaultPosition, wxDefaultSize, style, _T("GLCanvas")),
	m_GLContext(this),
	m_SuppressResize(true),
	m_LastMousePos(-1, -1), m_MouseCaptured(false)
{
}

void Canvas::SetCurrent()
{
	m_GLContext.SetCurrent(*this);
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


void Canvas::OnMouseCaptureLost(wxMouseCaptureLostEvent& WXUNUSED(evt))
{
	// Mouse capture lost due to "external" event, like a dialog box or alt-tabbing
	//	(this is currently a Windows only event and failure to handle it will lead
	//	to an assertion failure in debug builds)
	m_MouseCaptured = false;

	// Because of this, it is never guaranteed that e.g. two LeftDown events will be
	//	separated by a LeftUp
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
	EVT_SIZE          (Canvas::OnResize)
	EVT_LEFT_DCLICK   (Canvas::OnMouse)
	EVT_LEFT_DOWN     (Canvas::OnMouse)
	EVT_LEFT_UP       (Canvas::OnMouse)
	EVT_RIGHT_DCLICK  (Canvas::OnMouse)
	EVT_RIGHT_DOWN    (Canvas::OnMouse)
	EVT_RIGHT_UP      (Canvas::OnMouse)
	EVT_MIDDLE_DCLICK (Canvas::OnMouse)
	EVT_MIDDLE_DOWN   (Canvas::OnMouse)
	EVT_MIDDLE_UP     (Canvas::OnMouse)
	EVT_MOUSEWHEEL    (Canvas::OnMouse)
	EVT_MOTION        (Canvas::OnMouse)
	EVT_MOUSE_CAPTURE_LOST(Canvas::OnMouseCaptureLost)
END_EVENT_TABLE()
