#include "precompiled.h"

/*
 * wxJavaScript - iconize.cpp
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
 * $Id: iconize.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// iconize.cpp

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "jsevent.h"
#include "iconize.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>event/iconize</file>
 * <module>gui</module>
 * <class name="wxIconizeEvent" prototype="@wxEvent">
 *  An event being sent when the frame is iconized (minimized) or restored.
 *  Currently only Windows and GTK generate such events.
 *  See @wxFrame.
 * </class>
 */
WXJS_INIT_CLASS(IconizeEvent, "wxIconizeEvent", 0)

/***
 * <properties>
 *	<property name="iconized" type="Boolean" readonly="Y">
 *	 Returns true when the frame is iconized, false when it's not.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(IconizeEvent)
  WXJS_READONLY_PROPERTY(P_ICONIZED, "iconized")
WXJS_END_PROPERTY_MAP()

bool IconizeEvent::GetProperty(PrivIconizeEvent *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    wxIconizeEvent *event = p->GetEvent();
	if ( id == P_ICONIZED )
		*vp = ToJS(cx, event->Iconized());
	return true;
}
