#include "precompiled.h"

/*
 * wxJavaScript - evthand.cpp
 *
 * Copyright (c) 2002-2007 Franky Braem and the wxJavaScript project
 *
 * Project Info: http://www.wxjavascript.net or http://wxjs.sourceforge.net
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * $Id: evthand.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// evthand.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "jsevent.h"
#include "evthand.h"
#include "key.h"
#include "activate.h"
#include "mouse.h"
#include "move.h"
#include "sizeevt.h"
#include "help.h"
#include "scrollwin.h"

using namespace wxjs;
using namespace wxjs::gui;

void EventHandler::OnActivate(wxActivateEvent &event)
{
	PrivActivateEvent::Fire<ActivateEvent>(m_obj, event, "onActivate");
}

void EventHandler::OnChar(wxKeyEvent &event)
{
	PrivKeyEvent::Fire<KeyEvent>(m_obj, event, "onChar");
}

void EventHandler::OnCharHook(wxKeyEvent &event)
{
	PrivKeyEvent::Fire<KeyEvent>(m_obj, event, "onCharHook");
}

void EventHandler::OnKeyDown(wxKeyEvent &event)
{
	PrivKeyEvent::Fire<KeyEvent>(m_obj, event, "onKeyDown");
}

void EventHandler::OnKeyUp(wxKeyEvent &event)
{
    PrivKeyEvent::Fire<KeyEvent>(m_obj, event, "onKeyUp");
}

void EventHandler::OnSetFocus(wxFocusEvent &event)
{
	PrivFocusEvent::Fire<FocusEvent>(m_obj, event, "onSetFocus");
}

void EventHandler::OnKillFocus(wxFocusEvent &event)
{
	PrivFocusEvent::Fire<FocusEvent>(m_obj, event, "onKillFocus");
}

void EventHandler::OnInitDialog(wxInitDialogEvent &event)
{
	PrivInitDialogEvent::Fire<InitDialogEvent>(m_obj, event, "onInitDialog");
}

void EventHandler::OnMouseEvents(wxMouseEvent &event)
{
	if ( ! PrivMouseEvent::Fire<MouseEvent>(m_obj, event, "onMouseEvents") )
	{
		// MouseEvents is not handled, so try the other ones
		char *property;
		WXTYPE eventType = event.GetEventType();
		if ( eventType == wxEVT_LEFT_DOWN )
			property = "onLeftDown";
		else if ( eventType == wxEVT_LEFT_UP )
			property = "onLeftUp";
		else if ( eventType == wxEVT_LEFT_DCLICK )
			property = "onLeftDClick";
		else if ( eventType == wxEVT_MIDDLE_DOWN )
			property = "onMiddleDown";
		else if ( eventType == wxEVT_MIDDLE_UP )
			property = "onMiddleUp";
		else if ( eventType == wxEVT_MIDDLE_DCLICK )
			property = "onMiddleDClick";
		else if ( eventType == wxEVT_RIGHT_DOWN )
			property = "onRightDown";
		else if ( eventType == wxEVT_RIGHT_UP )
			property = "onRightUp";
		else if ( eventType == wxEVT_RIGHT_DCLICK )
			property = "onRightDClick";
		else if ( eventType == wxEVT_MOTION )
			property = "onMotion";
		else if ( eventType == wxEVT_ENTER_WINDOW )
			property = "onEnterWindow";
		else if ( eventType == wxEVT_LEAVE_WINDOW )
			property = "onLeaveWindow";
		else if ( eventType == wxEVT_MOUSEWHEEL )
			property = "onMouseWheel";
		else
			return;
		PrivMouseEvent::Fire<MouseEvent>(m_obj, event, property);
	}
}

void EventHandler::OnMove(wxMoveEvent &event)
{
	PrivMoveEvent::Fire<MoveEvent>(m_obj, event, "onMove");
}

void EventHandler::OnSize(wxSizeEvent &event)
{
	PrivSizeEvent::Fire<SizeEvent>(m_obj, event, "onSize");
}

void EventHandler::OnScroll(wxScrollWinEvent& event)
{
	PrivScrollWinEvent::Fire<ScrollWinEvent>(m_obj, event, "onScroll");
}

void EventHandler::OnScrollTop(wxScrollWinEvent& event)
{
	PrivScrollWinEvent::Fire<ScrollWinEvent>(m_obj, event, "onScrollTop");
}

void EventHandler::OnScrollBottom(wxScrollWinEvent& event)
{
	PrivScrollWinEvent::Fire<ScrollWinEvent>(m_obj, event, "onScrollBottom");
}

void EventHandler::OnScrollLineUp(wxScrollWinEvent& event)
{
	PrivScrollWinEvent::Fire<ScrollWinEvent>(m_obj, event, "onScrollLineUp");
}

void EventHandler::OnScrollLineDown(wxScrollWinEvent& event)
{
	PrivScrollWinEvent::Fire<ScrollWinEvent>(m_obj, event, "onScrollLineDown");
}

void EventHandler::OnScrollPageUp(wxScrollWinEvent& event)
{
	PrivScrollWinEvent::Fire<ScrollWinEvent>(m_obj, event, "onScrollPageUp");
}

void EventHandler::OnScrollPageDown(wxScrollWinEvent& event)
{
	PrivScrollWinEvent::Fire<ScrollWinEvent>(m_obj, event, "onScrollPageDown");
}

void EventHandler::OnScrollThumbTrack(wxScrollWinEvent& event)
{
	PrivScrollWinEvent::Fire<ScrollWinEvent>(m_obj, event, "onScrollThumbTrack");
}

void EventHandler::OnScrollThumbRelease(wxScrollWinEvent& event)
{
	PrivScrollWinEvent::Fire<ScrollWinEvent>(m_obj, event, "onScrollThumbRelease");
}

void EventHandler::OnHelp(wxHelpEvent &event)
{
	PrivHelpEvent::Fire<HelpEvent>(m_obj, event, "onHelp");
}

BEGIN_EVENT_TABLE(EventHandler, wxEvtHandler)
	EVT_ACTIVATE(EventHandler::OnActivate)
	
	// Key Events
	EVT_CHAR(EventHandler::OnChar)
	EVT_CHAR_HOOK(EventHandler::OnCharHook)
	EVT_KEY_DOWN(EventHandler::OnKeyDown)
	EVT_KEY_UP(EventHandler::OnKeyUp)
	
	// Mouse events
	EVT_MOUSE_EVENTS(EventHandler::OnMouseEvents)

	// Focus events
	EVT_SET_FOCUS(EventHandler::OnSetFocus)
	EVT_KILL_FOCUS(EventHandler::OnKillFocus)

	// Init Dialog
	EVT_INIT_DIALOG(EventHandler::OnInitDialog)

	EVT_MOVE(EventHandler::OnMove)

    EVT_SIZE(EventHandler::OnSize)

	// Scroll Events
	EVT_SCROLLWIN(EventHandler::OnScroll)
	EVT_SCROLLWIN_TOP(EventHandler::OnScrollTop)
	EVT_SCROLLWIN_BOTTOM(EventHandler::OnScrollBottom)
	EVT_SCROLLWIN_LINEUP(EventHandler::OnScrollLineUp)
	EVT_SCROLLWIN_LINEDOWN(EventHandler::OnScrollLineDown)
	EVT_SCROLLWIN_PAGEUP(EventHandler::OnScrollPageUp)
	EVT_SCROLLWIN_PAGEDOWN(EventHandler::OnScrollPageDown)
	EVT_SCROLLWIN_THUMBTRACK(EventHandler::OnScrollThumbTrack)
	EVT_SCROLLWIN_THUMBRELEASE(EventHandler::OnScrollThumbRelease)

    EVT_HELP(-1, EventHandler::OnHelp)

END_EVENT_TABLE()
