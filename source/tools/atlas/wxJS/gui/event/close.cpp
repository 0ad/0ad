#include "precompiled.h"

/*
 * wxJavaScript - close.cpp
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
 * $Id: close.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// close.cpp

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "jsevent.h"
#include "close.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>event/close</file>
 * <module>gui</module>
 * <class name="wxCloseEvent" prototype="@wxEvent">
 *	This object is passed to a function that is set to an onClose property of a
 *	@wxFrame or @wxDialog. The following example vetoes the close event
 *  because the text control was changed.
 *  <pre><code class="whjs">
 *	frame.onclose = close;
 *
 *	function close(closeEvent)
 *	{
 *	  if ( closeEvent.canVeto )
 *	  {
 *		if ( textCtrl.modified )
 *		{
 *		  wxMessageBox("Can't close because you didn't save the contents");
 *		  closeEvent.veto = true;
 *		}
 *	  }
 *	}
 *  </code></pre>
 * </class>
 */
WXJS_INIT_CLASS(CloseEvent, "wxCloseEvent", 0)

/***
 * <properties>
 *	<property name="canVeto" type="Boolean" readonly="Y">
 *	 Returns true when the close event can be vetoed.
 *  </property>
 *	<property name="loggingOff" type="Boolean" readonly="Y">
 *	 Returns true when the user is logging off.
 *  </property>
 *	<property name="veto" type="Boolean"> 
 *	 Set this to true when you don't want to close the window.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(CloseEvent)
  WXJS_READONLY_PROPERTY(P_CAN_VETO, "canVeto")
  WXJS_READONLY_PROPERTY(P_LOGGING_OFF, "loggingOff")
  WXJS_PROPERTY(P_VETO, "veto")
WXJS_END_PROPERTY_MAP()

bool CloseEvent::GetProperty(PrivCloseEvent *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    wxCloseEvent *event = p->GetEvent();
	switch (id) 
	{
	case P_CAN_VETO:
		*vp = ToJS(cx, event->CanVeto());
		break;
	case P_LOGGING_OFF:
		*vp = ToJS(cx, event->GetLoggingOff());
		break;
	case P_VETO:
		*vp = ToJS(cx, event->GetVeto());
		break;
	}
	return true;
}

bool CloseEvent::SetProperty(PrivCloseEvent *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    wxCloseEvent *event = p->GetEvent();
	if (    id == P_VETO 
		 && event->CanVeto() )
	{
		bool veto;
		if ( FromJS(cx, *vp, veto) )
		{
			event->Veto(veto);
		}
	}
	return true;
}
