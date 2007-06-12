#include "precompiled.h"

/*
 * wxJavaScript - command.cpp
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
 * $Id: command.cpp 744 2007-06-11 19:57:09Z fbraem $
 */
// command.cpp

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "jsevent.h"
#include "command.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>event/command</file>
 * <module>gui</module>
 * <class name="wxCommandEvent" prototype="@wxEvent">
 *	This event class contains information about command events, 
 *	which originate from a variety of simple controls.
 * </class>
 */
WXJS_INIT_CLASS(CommandEvent, "wxCommandEvent", 0)

/***
 * <properties>
 *  <property name="integer" type="Integer" readonly="Y">
 *   Returns the integer identifier corresponding to a listbox, choice or 
 *   radiobox selection (only if the event was a selection, not a deselection)
 *   , or a boolean value representing the value of a checkbox. 
 *  </property>
 *	<property name="checked" type="Boolean" readonly="Y">
 *	 This can be used for menus or checkboxes to check if they are checked or not
 *  </property>
 *	<property name="selection" type="Integer" readonly="Y">
 *	 This can be used to know which item is selected in a @wxListBox or @wxChoice control
 *  </property>
 *	<property name="string" type="String" readonly="Y">
 *	 This can be used to know the string of the item selected in a 
 *	 @wxListBox or @wxChoice control.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(CommandEvent)
	WXJS_READONLY_PROPERTY(P_CHECKED, "checked")
	WXJS_READONLY_PROPERTY(P_SELECTION, "selection")
	WXJS_READONLY_PROPERTY(P_STRING, "string")
	WXJS_READONLY_PROPERTY(P_INTEGER, "integer")
WXJS_END_PROPERTY_MAP()

bool CommandEvent::GetProperty(PrivCommandEvent *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    wxCommandEvent *event = p->GetEvent();
	switch(id)
	{
	case P_CHECKED:
		*vp = ToJS(cx, event->IsChecked());
		break;
	case P_SELECTION:
		*vp = ToJS(cx, event->GetSelection());
		break;
	case P_STRING:
		*vp = ToJS(cx, event->GetString());
		break;
    case P_INTEGER:
      *vp = ToJS(cx, event->GetInt());
      break;
	}
	return true;
}
