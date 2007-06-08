#include "precompiled.h"

/*
 * wxJavaScript - protocol.cpp
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
 * $Id: protocol.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../common/main.h"
#include "jsstream.h"
#include "sistream.h"
#include "sockbase.h"
#include "protocol.h"

using namespace wxjs;
using namespace wxjs::io;

/***
 * <file>protocol</file>
 * <module>io</module>
 * <class name="wxProtocol" prototype="@wxSocketClient" version="0.8.4">
 *  Prototype for all protocol classes.
 * </class>
 */
WXJS_INIT_CLASS(Protocol, "wxProtocol", 0)

/***
 * <constants>
 *  <type name="wxProtocolError">
 *   <constant name="NOERR">No error</constant>
 *   <constant name="NETERR">A generic network error occurred.</constant>
 *   <constant name="PROTERR">An error occurred during negotiation.</constant>
 *   <constant name="CONNERR">The client failed to connect the server.</constant>
 *   <constant name="INVVAL">Invalid value.</constant>
 *   <constant name="NOHNDLR" />
 *   <constant name="NOFILE">The remote file doesn't exist.</constant>
 *   <constant name="ABRT">Last action aborted.</constant>
 *   <constant name="RCNCT">An error occurred during reconnection.</constant>
 *   <constant name="STREAMING">Someone tried to send a command during a transfer.</constant>
 *   <desc>
 *    wxProtocolError is ported to JavaScript as a separate class. Note that this
 *    class doesn't exist in wxWidgets.
 *   </desc>
 *  </type>
 * </constants>
 */
void Protocol::InitClass(JSContext *cx, JSObject *obj, JSObject *proto)
{
    JSConstDoubleSpec wxProtocolErrorMap[] = 
    {
		WXJS_CONSTANT(wxPROTO_, NOERR)
		WXJS_CONSTANT(wxPROTO_, NETERR)
		WXJS_CONSTANT(wxPROTO_, PROTERR)
		WXJS_CONSTANT(wxPROTO_, CONNERR)
		WXJS_CONSTANT(wxPROTO_, INVVAL)
		WXJS_CONSTANT(wxPROTO_, NOHNDLR)
		WXJS_CONSTANT(wxPROTO_, NOFILE)
		WXJS_CONSTANT(wxPROTO_, ABRT)
		WXJS_CONSTANT(wxPROTO_, RCNCT)
		WXJS_CONSTANT(wxPROTO_, STREAMING)
	    { 0 }
    };

    JSObject *constObj = JS_DefineObject(cx, obj, "wxProtocolError", 
									 	 NULL, NULL,
							             JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineConstDoubles(cx, constObj, wxProtocolErrorMap);
}

/***
 * <properties>
 *  <property name="contentType" type="String" readonly="Y">
 *   Returns the type of the content of the last opened stream. It is a mime-type
 *  </property>
 *  <property name="error" type="Integer" readonly="Y">
 *   Returns the last occurred error. See @wxProtocol#wxProtocolError.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(Protocol)
	WXJS_READONLY_PROPERTY(P_CONTENT_TYPE, "contentType")
	WXJS_READONLY_PROPERTY(P_ERROR, "error")
WXJS_END_PROPERTY_MAP()

bool Protocol::GetProperty(SocketBasePrivate *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	wxProtocol *protocol = dynamic_cast<wxProtocol *>(p->GetBase());

	switch(id)
	{
	case P_CONTENT_TYPE:
		*vp = ToJS(cx, protocol->GetContentType());
		break;
	case P_ERROR:
		*vp = ToJS<int>(cx, protocol->GetError());
		break;
	}
	return true;
}

WXJS_BEGIN_METHOD_MAP(Protocol)
	WXJS_METHOD("abort", abort, 0)
	WXJS_METHOD("getInputStream", getInputStream, 1)
	WXJS_METHOD("reconnect", reconnect, 0)
	WXJS_METHOD("setPassword", setPassword, 1)
	WXJS_METHOD("setUser", setUser, 1)
WXJS_END_METHOD_MAP()

/***
 * <method name="abort">
 *  <function returns="Boolean" />
 *  <desc>
 *   Abort the current stream.
 *  </desc>
 * </method>
 */
JSBool Protocol::abort(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	SocketBasePrivate *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	wxProtocol *protocol = dynamic_cast<wxProtocol *>(p->GetBase());
	*rval = ToJS(cx, protocol->Abort());
	return JS_TRUE;
}

/***
 * <method name="getInputStream">
 *  <function returns="@wxInputStream">
 *   <arg name="Path" type="String" />
 *  </function>
 *  <desc>
 *   Creates a new input stream on the specified path. 
 *   You can use all but seek functionality of wxStream. 
 *   @wxInputStream#seekI isn't available on all stream. For example, http or 
 *   ftp streams doesn't deal with it. Other functions like 
 *   @wxStreamBase#size aren't available for the moment for 
 *   this sort of stream. You will be notified when the EOF 
 *   is reached by an error.
 *  </desc>
 * </method>
 */
JSBool Protocol::getInputStream(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	SocketBasePrivate *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	wxProtocol *protocol = dynamic_cast<wxProtocol *>(p->GetBase());

	wxString path;
	FromJS(cx, argv[0], path);

	wxInputStream *stream = protocol->GetInputStream(path);
	if ( stream != NULL )
	{
		Stream *js_stream = new Stream(stream, false);
		p->AddStream(js_stream);
		*rval = SocketInputStream::CreateObject(cx, js_stream, NULL);
		js_stream->SetObject(JSVAL_TO_OBJECT(*rval));	
	}
	return JS_TRUE;
}

/***
 * <method name="reconnect">
 *  <function returns="Boolean" />
 *  <desc>
 *   Tries to reestablish a previous opened connection (close and renegotiate connection).
 *  </desc>
 * </method>
 */
JSBool Protocol::reconnect(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	SocketBasePrivate *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	wxProtocol *protocol = dynamic_cast<wxProtocol *>(p->GetBase());

	*rval = ToJS(cx, protocol->Reconnect());
	return JS_TRUE;
}

/***
 * <method name="setPassword">
 *  <function>
 *   <arg name="pwd" type="String" />
 *  </function>
 *  <desc>
 *   Sets the authentication password. It is mainly useful when FTP is used.
 *  </desc>
 * </method>
 */
JSBool Protocol::setPassword(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	SocketBasePrivate *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	wxProtocol *protocol = dynamic_cast<wxProtocol *>(p->GetBase());

	wxString pwd;
	FromJS(cx, argv[0], pwd);
	protocol->SetPassword(pwd);
	return JS_TRUE;
}

/***
 * <method name="setUser">
 *  <function>
 *   <arg name="user" type="String" />
 *  </function>
 *  <desc>
 *   Sets the authentication user. It is mainly useful when FTP is used.
 *  </desc>
 * </method>
 */
JSBool Protocol::setUser(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	SocketBasePrivate *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	wxProtocol *protocol = dynamic_cast<wxProtocol *>(p->GetBase());

	wxString user;
	FromJS(cx, argv[0], user);
	protocol->SetUser(user);
	return JS_TRUE;
}
