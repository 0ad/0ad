#include "precompiled.h"

/*
 * wxJavaScript - sockevth.cpp
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
 * $Id: sockevth.cpp 716 2007-05-20 17:57:22Z fbraem $
 */
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/socket.h>

#include "../common/main.h"
#include "../common/jsutil.h"

#include "sockevth.h"

using namespace wxjs;
using namespace wxjs::io;

BEGIN_EVENT_TABLE (SocketEventHandler, wxEvtHandler)
	EVT_SOCKET(-1, SocketEventHandler::OnSocketEvent)
END_EVENT_TABLE()

void SocketEventHandler::OnSocketEvent(wxSocketEvent &event)
{
  JavaScriptClientData *data 
    = dynamic_cast<JavaScriptClientData*>(GetClientObject());
  if ( data == NULL )
    return;

  JSContext *cx = data->GetContext();
  JSObject *obj = data->GetObject();
  jsval rval;
  switch(event.GetSocketEvent())
  {
    case wxSOCKET_INPUT      : 
		CallFunctionProperty(cx, obj, "onInput", 0, NULL, &rval);
		break;
    case wxSOCKET_LOST       : 
		CallFunctionProperty(cx, obj, "onLost", 0, NULL, &rval);
		break;
    case wxSOCKET_CONNECTION : 
		CallFunctionProperty(cx, obj, "onConnection", 0, NULL, &rval);
		break;
  }
} 
