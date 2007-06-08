#include "precompiled.h"

/*
 * wxJavaScript - socksrv.cpp
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
 * $Id: socksrv.cpp 716 2007-05-20 17:57:22Z fbraem $
 */
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../common/main.h"
#include "../common/jsutil.h"
#include "sockbase.h"
#include "sockaddr.h"
#include "socksrv.h"
#include "sockevth.h"

using namespace wxjs;
using namespace wxjs::io;

SocketServer::SocketServer(JSContext *cx
						   , JSObject *obj
						   , wxSockAddress &address
						   , wxSocketFlags flags) : wxSocketServer(address, flags)
{
	m_evtHandler = new SocketEventHandler();
    m_evtHandler->SetClientObject(new JavaScriptClientData(cx, obj, false, true));
	SetEventHandler(*m_evtHandler, -1);
}

SocketServer::~SocketServer()
{
	delete m_evtHandler;
}

/*** 
 * <file>socksrv</file>
 * <module>io</module>
 * <class name="wxSocketServer" prototype="@wxSocketBase" version="0.8.4">
 *  This class implements server sockets. A server socket waits for requests 
 *  to come in over the network. 
 * </class>
 */
WXJS_INIT_CLASS(SocketServer, "wxSocketServer", 1)

/***
 * <ctor>
 *  <function>
 *   <arg name="Address" type="@wxSockAddress" />
 *   <arg name="Flag" type="@wxSocketBase#wxSocketFlags" default="wxSocketFlags.NONE" />
 *  </function>
 *  <desc>
 *   Constructs a new wxSocketServer.
 *  </desc>
 * </ctor>
 */
wxSocketServer *SocketServer::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
	int flags = wxSOCKET_NONE;
	if ( argc > 1 )
	{
		if ( ! FromJS(cx, argv[0], flags) )
		{
			return NULL;
		}
	}

	wxSockAddress *address = SockAddress::GetPrivate(cx, argv[0]);
	if ( address != NULL )
	{
		return new SocketServer(cx, obj, *address, flags);
	}
	return NULL;
}

void SocketServer::Destruct(JSContext *cx, wxSocketServer *p)
{
	p->Destroy();
}

WXJS_BEGIN_METHOD_MAP(SocketServer)
	WXJS_METHOD("accept", accept, 0) 
	WXJS_METHOD("acceptWith", acceptWith, 1) 
	WXJS_METHOD("waitForAccept", waitForAccept, 0)
WXJS_END_METHOD_MAP()

/***
 * <method name="accept">
 *  <function returns="@wxSocketBase">
 *   <arg name="Wait" type="Boolean" default="true">
 *    If wait is true and there are no pending connections to be accepted, 
 *    it will wait for the next incoming connection to arrive.</arg>
 *  </function>
 *  <desc>
 *   Accepts an incoming connection request, and creates a new @wxSocketBase object 
 *   which represents the server-side of the connection.
 *   Returns an opened socket connection, or NULL if an error occurred or if the 
 *   wait parameter was false and there were no pending connections.
 *  </desc>
 * </method>
 */
JSBool SocketServer::accept(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxSocketServer *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	bool wait = true;
	if ( argc > 1 )
	{
		if ( ! FromJS(cx, argv[1], wait) )
			return JS_FALSE;
	}

	wxSocketBase *client = p->Accept(wait);
	if ( client == NULL )
	{
		*rval = JSVAL_VOID;
	}
	else
	{
		*rval = SocketBase::CreateObject(cx, new SocketBasePrivate(client));
	}
	return JS_TRUE;
}

/***
 * <method name="acceptWith">
 *  <function returns="Boolean">
 *   <arg name="Socket" type="@wxSocketBase" />
 *   <arg name="Wait" type="Boolean" default="true">
 *    If wait is true and there are no pending connections to be accepted, 
 *    it will wait for the next incoming connection to arrive.</arg>
 *  </function>
 *  <desc>
 *   Accept an incoming connection using the specified socket object.
 *  </desc>
 * </method>
 */
JSBool SocketServer::acceptWith(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxSocketServer *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	SocketBasePrivate *base = SocketBase::GetPrivate(cx, argv[0]);
	if ( ! base )
	{
		return JS_FALSE;
	}

	bool wait = true;
	if ( argc > 1 )
	{
		if ( ! FromJS(cx, argv[1], wait) )
			return JS_FALSE;
	}

	*rval = ToJS(cx, p->AcceptWith(*base->GetBase(), wait));
	return JS_TRUE;
}

/***
 * <method name="waitForAccept">
 *  <function returns="Boolean">
 *   <arg name="Seconds" type="Integer" default="-1" /> 
 *   <arg name="MilliSeconds" type="Integer" default="0" /> 
 *  </function>
 *  <desc>
 *   This function waits for an incoming connection. Use it if you want to call 
 *   accept or acceptWith with wait set to false, to detect when an incoming connection 
 *   is waiting to be accepted.
 *   Returns true if an incoming connection arrived, false if the timeout elapsed.
 *  </desc>
 * </method>
 */
JSBool SocketServer::waitForAccept(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxSocketServer *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	long secs = -1;
	long ms = 0;

	if ( argc > 2 )
		argc = 2;

	switch(argc)
	{
	case 2:
		if ( ! FromJS(cx, argv[1], ms) )
			return JS_FALSE;
		// Fall through
	case 1:
		if ( ! FromJS(cx, argv[0], secs) )
			return JS_FALSE;
		// Fall through
	default:
		*rval = ToJS(cx, p->WaitForAccept(secs, ms));
	}
	return JS_TRUE;	
}
