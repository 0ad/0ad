#include "precompiled.h"

/*
 * wxJavaScript - event.cpp
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
 * $Id: event.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// event.cpp
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/type.h"

#include "jsevent.h"
#include "event.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>event/event</file>
 * <module>gui</module>
 * <class name="wxEvent">
 *	wxEvent is the prototype for the following event objects.
 * </class>
 */
WXJS_INIT_CLASS(Event, "wxEvent", 0)

/***
 * <properties>
 *	<property name="id" type="Integer" readonly="Y">
 *	 The identifier of the associated window
 *  </property>
 *	<property name="skip" type="Boolean">
 *	 Set this to true when you want to tell the system to skip the current event handler
 *	 and to use the next valid handler.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(Event)
  WXJS_READONLY_PROPERTY(P_ID, "id")
  WXJS_PROPERTY(P_SKIP, "skip")
WXJS_END_PROPERTY_MAP()

bool Event::GetProperty(PrivEvent *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	wxEvent *event = p->GetEvent();

	switch (id) 
	{
	case P_ID:
		*vp = ToJS(cx, event->GetId());
		break;
	case P_SKIP:
		*vp = ToJS(cx, event->GetSkipped());
		break;
	}
    return true;
}

bool Event::SetProperty(PrivEvent *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	wxEvent *event = p->GetEvent();
	if ( id == P_SKIP )
	{
		bool skip;
		if ( FromJS(cx, *vp, skip) )
			event->Skip(skip);
	}
	return true;
}
