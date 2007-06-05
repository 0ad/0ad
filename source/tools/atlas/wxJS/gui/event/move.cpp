#include "precompiled.h"

/*
 * wxJavaScript - move.cpp
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
 * $Id: move.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// move.cpp

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/apiwrap.h"

#include "jsevent.h"
#include "../misc/point.h"
#include "move.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>event/move</file>
 * <module>gui</module>
 * <class name="wxMoveEvent" prototype="@wxEvent">
 *	 A move event holds information about move change events.
 *   Handle this event by setting a function to the @wxWindow#onMove 
 *   property on a @wxWindow object. 
 * </class>
 */
WXJS_INIT_CLASS(MoveEvent, "wxMoveEvent", 0)

/***
 * <properties>
 *	<property name="position" type="@wxPoint" readonly="Y">
 *	 Returns the position of the window generating this event.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(MoveEvent)
	WXJS_READONLY_PROPERTY(P_POSITION, "position")
WXJS_END_PROPERTY_MAP()

bool MoveEvent::GetProperty(PrivMoveEvent *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    wxMoveEvent *event = p->GetEvent();

	if ( id == P_POSITION )
	{
		*vp = Point::CreateObject(cx, new wxPoint(event->GetPosition()));
	}
	return true;
}
