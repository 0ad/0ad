#include "precompiled.h"

/*
 * wxJavaScript - sockclient.cpp
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
 * $Id: sockclient.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../common/main.h"
#include "../common/jsutil.h"
#include "sockaddr.h"
#include "sockclient.h"
#include "sockevth.h"

using namespace wxjs;
using namespace wxjs::io;

SocketClient::SocketClient(  JSContext *cx
								   , JSObject *obj
								   , wxSocketFlags flags) : wxSocketClient(flags)
{
	m_evtHandler = new SocketEventHandler(cx, obj);
	SetEventHandler(*m_evtHandler, -1);
}

SocketClient::~SocketClient()
{
	delete m_evtHandler;
}

/*** 
 * <file>sockclient</file>
 * <module>io</module>
 * <class name="wxSocketClient" prototype="@wxSocketBase" version="0.8.4">
 *  This class implements client sockets.
 * </class>
 */
WXJS_INIT_CLASS(SocketClient, "wxSocketClient", 0)

/***
 * <ctor>
 *  <function>
 *   <arg name="Flag" type="@wxSocketBase#wxSocketFlags" default="wxSocketFlags.NONE" />
 *  </function>
 *  <desc>
 *   Constructs a new wxSocketClient.
 *  </desc>
 * </ctor>
 */
SocketBasePrivate *SocketClient::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
	if ( argc == 0 )
	{
		return new SocketBasePrivate(new SocketClient(cx, obj));
	}
	else
	{
		int flags;
		if ( FromJS(cx, argv[0], flags) )
		{
			return new SocketBasePrivate(new SocketClient(cx, obj, flags));
		}
	}
	return NULL;
}

void SocketClient::Destruct(JSContext *cx, SocketBasePrivate *p)
{
	p->GetBase()->Destroy();
	delete p;
}

WXJS_BEGIN_METHOD_MAP(SocketClient)
	WXJS_METHOD("connect", connect, 1) 
	WXJS_METHOD("waitForConnect", waitOnConnect, 0)
WXJS_END_METHOD_MAP()

/***
 * <method name="connect">
 *  <function returns="Boolean">
 *   <arg name="Address" type="@wxSockAddress">Address of the server</arg>
 *   <arg name="Wait" type="Boolean" default="true">Wait for the connection to complete?</arg>
 *  </function>
 *  <desc>
 *   Connects to a server using the specified address.
 *   Returns true if the connection is established and no error occurs.
 *   If <i>Wait</i> is false, connect will try to establish the connection and return 
 *   immediately, without blocking the GUI. When used this way, even if connect
 *   returns false, the connection request can be completed later. To detect this, 
 *   use @wxSocketClient#waitOnConnect, or catch socket events.
 *  </desc>
 * </method>
 */
JSBool SocketClient::connect(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	SocketBasePrivate *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	wxSockAddress *addr = SockAddress::GetPrivate(cx, argv[0]);
	if ( addr != NULL )
	{
		bool wait = true;
		if ( argc > 1 )
		{
			if ( ! FromJS(cx, argv[1], wait) )
				return JS_FALSE;
		}
		wxSocketClient *sockClient = dynamic_cast<wxSocketClient *>(p->GetBase());
		*rval = ToJS(cx, sockClient->Connect(*addr, wait));
		return JS_TRUE;
	}
	return JS_FALSE;
}

/***
 * <method name="waitOnConnect">
 *  <function returns="Boolean">
 *   <arg name="Seconds" type="Integer" default="-1" /> 
 *   <arg name="MilliSeconds" type="Integer" default="0" /> 
 *  </function>
 *  <desc>
 *   Wait until a connection request completes, or until the specified timeout 
 *   elapses. Use this function after issuing a call to Connect with wait set to false.
 *   waitOnConnect returns true if the connection request completes. This does not necessarily
 *   mean that the connection was successfully established; it might also happen that the 
 *   connection was refused by the peer. Use @wxSocketBase#connected to distinguish between
 *   these two situations.
 *  </desc>
 * </method>
 */
JSBool SocketClient::waitOnConnect(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	SocketBasePrivate *p = GetPrivate(cx, obj);
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
		{
			wxSocketClient *sockClient = dynamic_cast<wxSocketClient *>(p->GetBase());
			*rval = ToJS(cx, sockClient->WaitOnConnect(secs, ms));
		}
	}
	return JS_TRUE;	
}
