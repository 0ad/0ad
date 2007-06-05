#include "precompiled.h"

/*
 * wxJavaScript - key.cpp
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
 * $Id: key.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// key.cpp

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "jsevent.h"
#include "key.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>event/key</file>
 * <module>gui</module>
 * <class name="wxKeyEvent" prototype="@wxEvent">
 *	This event class contains information about keypress (character) events. Look
 *	at @wxWindow#onChar for an example. See @wxWindow#onChar,
 *	@wxWindow#onCharHook, @wxWindow#onKeyDown, @wxWindow#onKeyUp
 * </class>
 */
WXJS_INIT_CLASS(KeyEvent, "wxKeyEvent", 0)

/***
 * <properties>
 *	<property name="altDown" type="Boolean" readonly="Y">
 *	 True when the ALT key was pressed at the time of the event.
 *  </property>
 *	<property name="controlDown" type="Boolean" readonly="Y">
 *	 True when the CTRL key was pressed at the time of the event.
 *  </property>
 *	<property name="hasModifiers" type="Boolean" readonly="Y">
 *	 True when META, CTRL or ALT key was down at the time of the event.
 *  </property>
 *	<property name="keyCode" type="Integer" readonly="Y">
 *	 The code of the key. See @wxKeyCode
 *  </property>
 *	<property name="metaDown" type="Boolean" readonly="Y">
 *	 True when the META key was down at the time of the event.
 *  </property>
 *	<property name="shiftDown" type="Boolean" readonly="Y">
 *	 True when the SHIFT key was down at the time of the event.
 *  </property>
 *	<property name="x" type="Integer" readonly="Y">
 *	 The x position of the event
 *  </property>
 *	<property name="y" type="Integer" readonly="Y">
 *	 The y position of the event
 *  </property>
 * </properties>
 */

 WXJS_BEGIN_PROPERTY_MAP(KeyEvent)
	WXJS_READONLY_PROPERTY(P_ALT_DOWN, "altDown")
	WXJS_READONLY_PROPERTY(P_CONTROL_DOWN, "controlDown")
	WXJS_READONLY_PROPERTY(P_KEY_CODE, "keyCode")
	WXJS_READONLY_PROPERTY(P_META_DOWN, "metaDown")
	WXJS_READONLY_PROPERTY(P_SHIFT_DOWN, "shiftDown")
	WXJS_READONLY_PROPERTY(P_HAS_MODIFIERS, "hasModifiers")
	WXJS_READONLY_PROPERTY(P_X, "x")
	WXJS_READONLY_PROPERTY(P_Y, "y")
WXJS_END_PROPERTY_MAP()

bool KeyEvent::GetProperty(PrivKeyEvent *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    wxKeyEvent *event = ((wxKeyEvent*) p->GetEvent());
	switch (id) 
	{
	case P_ALT_DOWN:
		*vp = ToJS(cx, event->AltDown());
		break;
	case P_CONTROL_DOWN:
		*vp = ToJS(cx, event->ControlDown());
		break;
	case P_KEY_CODE:
		*vp = ToJS(cx, event->GetKeyCode());
		break;
	case P_META_DOWN:
		*vp = ToJS(cx, event->MetaDown());
		break;
	case P_SHIFT_DOWN:
		*vp = ToJS(cx, event->ShiftDown());
		break;
	case P_X:
		*vp = ToJS(cx, event->GetX());
		break;
	case P_Y:
		*vp = ToJS(cx, event->GetY());
		break;
	case P_HAS_MODIFIERS:
		*vp = ToJS(cx, event->HasModifiers());
		break;
	}
	return true;
}
