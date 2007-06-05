#include "precompiled.h"

/*
 * wxJavaScript - notify.cpp
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
 * $Id: notify.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// notify.cpp

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "jsevent.h"
#include "notify.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>event/notify</file>
 * <module>gui</module>
 * <class name="wxNotifyEvent" prototype="@wxEvent">
 *	This class is a prototype for several events.
 *  See @wxListEvent.
 * </class>
 */
WXJS_INIT_CLASS(NotifyEvent, "wxNotifyEvent", 0)

/***
 * <properties>
 *	<property name="allowed" type="Boolean"> 
 *   Allow/Disallow the change. Setting allowed to false, is the same as setting veto to true.
 *  </property>
 *	<property name="veto" type="Boolean">
 *	 When set to true, prevents the change announced by this event from happening.
 *   Setting veto to false, is the same as setting allow to true.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(NotifyEvent)
  WXJS_READONLY_PROPERTY(P_ALLOWED, "allowed")
  WXJS_PROPERTY(P_VETO, "veto")
WXJS_END_PROPERTY_MAP()

bool NotifyEvent::GetProperty(PrivNotifyEvent *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    wxNotifyEvent *event = p->GetEvent();
	switch (id) 
	{
	case P_ALLOWED:
		*vp = ToJS(cx, event->IsAllowed());
		break;
	case P_VETO:
		*vp = ToJS(cx, ! event->IsAllowed());
		break;
	}
	return true;
}

bool NotifyEvent::SetProperty(PrivNotifyEvent *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    wxNotifyEvent *event = p->GetEvent();
	switch (id) 
	{
	case P_ALLOWED:
        {
		    bool allow;
		    if ( FromJS(cx, *vp, allow) )
		    {
                if ( allow )
                    event->Allow();
                else
                    event->Veto();
		    }
		    break;
        }
	case P_VETO:
        {
		    bool veto;
		    if ( FromJS(cx, *vp, veto) )
		    {
                if ( veto )
                    event->Veto();
                else
                    event->Allow();
		    }
		    break;
        }
    }
	return true;
}
