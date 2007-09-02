#include "precompiled.h"

/*
 * wxJavaScript - jsevent.cpp
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
 * $Id: jsevent.cpp 744 2007-06-11 19:57:09Z fbraem $
 */
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "jsevent.h"
#include "event.h"
#include "command.h"
#include "iconize.h"
#include "close.h"
#include "key.h"
#include "activate.h"
#include "mouse.h"
#include "move.h"
#include "cal.h"
#include "findr.h"
#include "scroll.h"
#include "scrollwin.h"
#include "help.h"
#include "sizeevt.h"
#include "htmllink.h"
#include "split.h"
#include "spinevt.h"
#include "notebookevt.h"

#include "notify.h"
#include "listevt.h"
#include "treeevt.h"

using namespace wxjs;
using namespace wxjs::gui;

bool wxjs::gui::InitEventClasses(JSContext *cx, JSObject *global)
{
	JSObject *obj = Event::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxEvent creation prototype failed"));
	if ( !obj )
		return false;

	obj = CommandEvent::JSInit(cx, global, Event::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxCommandEvent prototype creation failed"));
	if (! obj )
		return false;

	obj = KeyEvent::JSInit(cx, global, Event::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxKeyEvent prototype creation failed"));
	if (! obj )
		return false;

	obj = ActivateEvent::JSInit(cx, global, Event::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxActivateEvent prototype creation failed"));
	if (! obj )
		return false;

	obj = CloseEvent::JSInit(cx, global, Event::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxCloseEvent prototype creation failed"));
	if (! obj )
		return false;

	obj = FocusEvent::JSInit(cx, global, Event::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxFocusEvent prototype creation failed"));
	if (! obj )
		return false;

	obj = InitDialogEvent::JSInit(cx, global, Event::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxInitDialogEvent prototype creation failed"));
	if (! obj )
		return false;

	obj = MouseEvent::JSInit(cx, global, Event::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxMouseEvent prototype creation failed"));
	if (! obj )
		return false;

	obj = MoveEvent::JSInit(cx, global, Event::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxMoveEvent prototype creation failed"));
	if (! obj )
		return false;

	obj = SizeEvent::JSInit(cx, global, Event::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxSizeEvent prototype creation failed"));
	if (! obj )
		return false;

    obj = CalendarEvent::JSInit(cx, global, Event::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxCalendarEvent prototype creation failed"));
	if (! obj )
		return false;

	obj = IconizeEvent::JSInit(cx, global, Event::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxIconizeEvent prototype creation failed"));
	if (! obj )
		return false;

	obj = MaximizeEvent::JSInit(cx, global, Event::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxMaximizeEvent prototype creation failed"));
	if (! obj )
		return false;

	obj = FindDialogEvent::JSInit(cx, global, Event::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxFindDialogEvent prototype creation failed"));
	if (! obj )
		return false;

	obj = ScrollEvent::JSInit(cx, global, CommandEvent::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxScrollEvent prototype creation failed"));
	if (! obj )
		return false;

	obj = ScrollWinEvent::JSInit(cx, global, Event::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxScrollWinEvent prototype creation failed"));
	if (! obj )
		return false;

	obj = SysColourChangedEvent::JSInit(cx, global, Event::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxSysColourChangedEvent prototype creation failed"));
	if (! obj )
		return false;

	obj = HelpEvent::JSInit(cx, global, CommandEvent::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxHelpEvent prototype creation failed"));
	if (! obj )
		return false;

	obj = NotifyEvent::JSInit(cx, global, CommandEvent::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxNotifyEvent prototype creation failed"));
	if (! obj )
		return false;

	obj = ListEvent::JSInit(cx, global, NotifyEvent::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxListEvent prototype creation failed"));
	if (! obj )
		return false;

    obj = TreeEvent::JSInit(cx, global, NotifyEvent::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxTreeEvent prototype creation failed"));
	if (! obj )
		return false;

    obj = HtmlLinkEvent::JSInit(cx, global, CommandEvent::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxHtmlLinkEvent prototype creation failed"));
	if (! obj )
		return false;

    obj = SplitterEvent::JSInit(cx, global, NotifyEvent::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxSplitterEvent prototype creation failed"));
	if (! obj )
		return false;

	obj = SpinEvent::JSInit(cx, global, NotifyEvent::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxSpinEvent prototype creation failed"));
	if (! obj )
		return false;

	obj = NotebookEvent::JSInit(cx, global, NotifyEvent::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxNotebookEvent prototype creation failed"));
	if (! obj )
		return false;

    return true;
}
