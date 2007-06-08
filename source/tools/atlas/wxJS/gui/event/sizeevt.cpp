#include "precompiled.h"

/*
 * wxJavaScript - sizeevt.cpp
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
 * $Id: sizeevt.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// sizeevt.cpp

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/apiwrap.h"

#include "jsevent.h"
#include "../misc/size.h"
#include "sizeevt.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>event/sizeevt</file>
 * <module>gui</module>
 * <class name="wxSizeEvent" prototype="@wxEvent">
 *	 A size event holds information about size change events.
 *   Handle this event by setting a function to the @wxWindow#onSize 
 *   property on a @wxWindow object. 
 *   <br /><br />
 *   The size retrieved with size property is the size of the whole window.
 *   Use @wxWindow#clientSize for the area which may be used by the application.
 * </class>
 */
WXJS_INIT_CLASS(SizeEvent, "wxSizeEvent", 0)

/***
 * <properties>
 *	<property name="size" type="@wxSize" readonly="Y">
 * 	 Returns the size of the window generating this event.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(SizeEvent)
	WXJS_READONLY_PROPERTY(P_SIZE, "size")
WXJS_END_PROPERTY_MAP()

bool SizeEvent::GetProperty(PrivSizeEvent *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    wxSizeEvent *event = (wxSizeEvent*) p->GetEvent();

	if ( id == P_SIZE )
	{
		*vp = Size::CreateObject(cx, new wxSize(event->GetSize()));
	}
	return true;
}

