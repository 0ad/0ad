#include "precompiled.h"

/*
 * wxJavaScript - sockbase.cpp
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
 * $Id: sockbase.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// file.cpp
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../common/main.h"
#include "../ext/wxjs_ext.h"

#include "sockbase.h"

using namespace wxjs;
using namespace wxjs::io;

/***
 * <file>sockbase</file>
 * <module>io</module>
 * <class name="wxSocketBase" version="0.8.4">
 *  wxSocketBase is the property class for all socket-related objects,
 *  and it defines all basic IO functionality.
 * </class>
 */
WXJS_INIT_CLASS(SocketBase, "wxSocketBase", 0)

/***
 * <constants>
 *  <type name="wxSocketError">
 *   <constant name="NOERROR">No error happened.</constant>
 *   <constant name="INVOP">Invalid operation.</constant>
 *   <constant name="IOERR">Input/Output error.</constant>
 *   <constant name="INVADDR">Invalid address.</constant>
 *   <constant name="INVSOCK">Invalid socket (uninitialized).</constant>
 *   <constant name="NOHOST">No corresponding host.</constant>
 *   <constant name="INVPORT">Invalid port.</constant>
 *   <constant name="WOULDBLOCK">The socket is non-blocking and the operation would block.</constant>
 *   <constant name="TIMEDOUT">The timeout for this operation expired.</constant>
 *   <constant name="MEMERR">Memory exhausted.</constant>
 *   <desc>
 *    wxSocketError is ported to JavaScript as a separate class. Note that this
 *    class doesn't exist in wxWidgets.
 *   </desc>
 *  </type>
 *  <type name="wxSocketEventType">
 *   <constant name="INPUT">There is data available for reading.</constant>
 *   <constant name="OUTPUT">The socket is ready to be written to.</constant>
 *   <constant name="CONNECTION">Incoming connection request (server), or successful connection establishment (client).</constant>
 *   <constant name="LOST">The connection has been closed.</constant>
 *   <desc>
 *    wxSocketEventType is ported to JavaScript as a separate class. Note that this
 *    class doesn't exist in wxWidgets.
 *   </desc>
 *  </type>
 *  <type name="wxSocketFlags">
 *   <constant name="NONE">Normal functionality.</constant>
 *   <constant name="NOWAIT">Read/write as much data as possible and return immediately.</constant>
 *   <constant name="WAITALL">Wait for all required data to be read/written unless an error occurs.</constant>
 *   <constant name="BLOCK">Block the GUI (do not yield) while reading/writing data.</constant>
 *   <constant name="REUSEADDR">Allows the use of an in-use port (wxServerSocket only)</constant>  
 *   <desc>
 *    wxSocketFlags is ported to JavaScript as a separate class. Note that this
 *    class doesn't exist in wxWidgets.
 *   </desc>
 *  </type>
 * </constants>
 */
void SocketBase::InitClass(JSContext *cx, JSObject *obj, JSObject *proto)
{
    JSConstDoubleSpec wxSocketErrorMap[] = 
    {
		WXJS_CONSTANT(wxSOCKET_, NOERROR)
		WXJS_CONSTANT(wxSOCKET_, INVOP)
		WXJS_CONSTANT(wxSOCKET_, IOERR)
		WXJS_CONSTANT(wxSOCKET_, INVADDR)
		WXJS_CONSTANT(wxSOCKET_, INVSOCK)
		WXJS_CONSTANT(wxSOCKET_, NOHOST)
		WXJS_CONSTANT(wxSOCKET_, INVPORT)
		WXJS_CONSTANT(wxSOCKET_, WOULDBLOCK)
		WXJS_CONSTANT(wxSOCKET_, TIMEDOUT)
		WXJS_CONSTANT(wxSOCKET_, MEMERR)
	    { 0 }
    };

    JSObject *constObj = JS_DefineObject(cx, obj, "wxSocketError", 
									 	 NULL, NULL,
							             JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineConstDoubles(cx, constObj, wxSocketErrorMap);

    JSConstDoubleSpec wxSocketEventTypeMap[] = 
    {
		WXJS_CONSTANT(wxSOCKET_, INPUT)
		WXJS_CONSTANT(wxSOCKET_, OUTPUT)
		WXJS_CONSTANT(wxSOCKET_, CONNECTION)
		WXJS_CONSTANT(wxSOCKET_, LOST)
	    { 0 }
    };
    constObj = JS_DefineObject(cx, obj, "wxSocketEventType", 
						 	   NULL, NULL,
							   JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineConstDoubles(cx, constObj, wxSocketEventTypeMap);

    JSConstDoubleSpec wxSocketFlagMap[] = 
    {
		WXJS_CONSTANT(wxSOCKET_, NONE)
		WXJS_CONSTANT(wxSOCKET_, NOWAIT)
		WXJS_CONSTANT(wxSOCKET_, WAITALL)
		WXJS_CONSTANT(wxSOCKET_, BLOCK)
		WXJS_CONSTANT(wxSOCKET_, REUSEADDR)
	    { 0 }
    };
    constObj = JS_DefineObject(cx, obj, "wxSocketFlags", 
						 	   NULL, NULL,
							   JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineConstDoubles(cx, constObj, wxSocketFlagMap);
}

/***
 * <properties>
 *  <property name="connected" type="Boolean" readonly="Y">
 *   Returns true if the socket is connected.
 *  </property>
 *  <property name="data" type="Boolean" readonly="Y">
 *   This property waits until the socket is readable. This might mean that queued data
 *   is available for reading or, for streamed sockets, that the connection has been closed, 
 *   so that a read operation will complete immediately without blocking 
 *   (unless the wxSocketFlags.WAITALL flag is set, in which case the operation might still block).
 *  </property>
 *  <property name="disconnected" type="Boolean" readonly="Y">
 *   Returns true if the socket is disconnected.
 *  </property>
 *  <property name="error" type="Boolean" readonly="Y">
 *   Returns true if an error occurred in the last IO operation.
 *  </property>
 *  <property name="lastCount" type="Integer" readonly="Y">
 *   Returns the number of bytes read or written by the last IO call
 *  </property>
 *  <property name="lastError" type="@wxSocketBase#wxSocketError" readonly="Y">
 *   Returns the last wxSocket error.
 *   Please note that this property merely returns the last error code, but it should not 
 *   be used to determine if an error has occurred (this is because successful operations 
 *   do not change the lastError value). Use @wxSocketBase#error first, in order to determine
 *   if the last IO call failed. If this returns true, use lastError to discover the cause of the error.
 *  </property>
 *  <property name="ok" type="Boolean" readonly="Y">
 *   Returns true if the socket is initialized and ready and false in other cases.
 *   <br /><b>Remark/Warning: </b><br />
 *   For @wxSocketClient, ok won't return true unless the client is connected to a server.
 *   For @wxSocketServer, ok will return true if the server could bind to the specified 
 *   address and is already listening for new connections.
 *   ok does not check for IO errors; use @wxSocketBase#error instead for that purpose.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(SocketBase)
	WXJS_READONLY_PROPERTY(P_CONNECTED, "connected")
	WXJS_READONLY_PROPERTY(P_DATA, "data")
	WXJS_READONLY_PROPERTY(P_DISCONNECTED, "disconnected")
	WXJS_READONLY_PROPERTY(P_ERROR, "error")
	WXJS_READONLY_PROPERTY(P_LASTCOUNT, "lastCount")
	WXJS_READONLY_PROPERTY(P_LASTCOUNT, "lastError")
	WXJS_READONLY_PROPERTY(P_OK, "ok")
WXJS_END_PROPERTY_MAP()

bool SocketBase::GetProperty(SocketBasePrivate *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	switch(id)
	{
	case P_CONNECTED:
		*vp = ToJS(cx, p->GetBase()->IsConnected());
		break;
	case P_DATA:
		*vp = ToJS(cx, p->GetBase()->IsData());
		break;
	case P_DISCONNECTED:
		*vp = ToJS(cx, p->GetBase()->IsDisconnected());
		break;
	case P_ERROR:
		*vp = ToJS(cx, p->GetBase()->Error());
		break;
	case P_LASTCOUNT:
		{
			long count = p->GetBase()->LastCount();
			*vp = ToJS(cx, count);
			break;
		}
	case P_LASTERROR:
		{
			int error = p->GetBase()->LastError();
			*vp = ToJS(cx, error);
			break;
		}
	case P_OK:
		*vp = ToJS(cx, p->GetBase()->Ok());
		break;
	}
	return true;
}

WXJS_BEGIN_METHOD_MAP(SocketBase)
	WXJS_METHOD("close", close, 0) 
	WXJS_METHOD("discard", discard, 0) 
	WXJS_METHOD("interruptWait", interruptWait, 0) 
	WXJS_METHOD("notify", notify, 1) 
	WXJS_METHOD("peek", peek, 1)
	WXJS_METHOD("read", read, 1)
	WXJS_METHOD("readMsg", readMsg, 1)
	WXJS_METHOD("restoreState", restoreState, 0) 
	WXJS_METHOD("saveState", saveState, 0) 
	WXJS_METHOD("setTimeout", setTimeout, 1) 
	WXJS_METHOD("unread", unread, 1)
	WXJS_METHOD("wait", wait, 0)
	WXJS_METHOD("waitForLost", waitForLost, 0)
	WXJS_METHOD("waitForRead", waitForRead, 0)
	WXJS_METHOD("waitForWrite", waitForWrite, 0)
	WXJS_METHOD("write", write, 1)
	WXJS_METHOD("writeMsg", writeMsg, 1)
WXJS_END_METHOD_MAP()

/***
 * <method name="close">
 *  <function />
 *  <desc>
 *   This function shuts down the socket, disabling further transmission and reception of data; 
 *   it also disables events for the socket and frees the associated system resources.
 *   Upon socket destruction, close is automatically called, so in most cases you won't need to
 *   do it yourself, unless you explicitly want to shut down the socket, typically to notify 
 *   the peer that you are closing the connection.
 *   <br /><br />
 *   <b>Remark/Warning:</b> Although close immediately disables events for the socket, 
 *   it is possible that event messages may be waiting in the application's event queue. 
 *   The application must therefore be prepared to handle socket event messages even after calling close.
 *  </desc>
 * </method>
 */
JSBool SocketBase::close(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	SocketBasePrivate *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	p->GetBase()->Close();

	return JS_TRUE;
}

/***
 * <method name="discard">
 *  <function />
 *  <desc>
 *   This function simply deletes all bytes in the incoming queue. 
 *   This function always returns immediately and its operation is not affected by IO flags.
 *   Use @wxSocketBase#lastCount to verify the number of bytes actually discarded.
 *   If you use @wxSocketBase#error, it will always return false.
 *  </desc>
 * </method>
 */
JSBool SocketBase::discard(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	SocketBasePrivate *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	p->GetBase()->Discard();

	return JS_TRUE;
}

/***
 * <method name="interruptWait">
 *  <function />
 *  <desc>
 *   Use this method to interrupt any wait operation currently in progress. Note that this is not intended
 *   as a regular way to interrupt a @wxSocketBase#wait call, but only as an escape mechanism for 
 *   exceptional situations where it is absolutely necessary to use it, for example to abort 
 *   an operation due to some exception or abnormal problem. InterruptWait is automatically called when 
 *   you @wxSocketBase#close a socket (and thus also upon socket destruction), so you don't need 
 *   to use it in these cases.
 *  </desc>
 * </method>
 */
JSBool SocketBase::interruptWait(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	SocketBasePrivate *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	p->GetBase()->InterruptWait();

	return JS_TRUE;
}

/***
 * <method name="notify">
 *  <function>
 *   <arg name="Flag" type="Boolean" />
 *  </function>
 *  <desc>
 *   According to the <i>Flag</i> value, this function enables or disables socket events. 
 *   If <i>Flag</i> is true, the events configured with @wxSocketBase#setNotify will be sent 
 *   to the application. If <i>Flag</i> is false; no events will be sent.
 *  </desc>
 * </method>
 */
JSBool SocketBase::notify(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	SocketBasePrivate *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	bool notify;
	if ( FromJS(cx, argv[0], notify) )
	{
		p->GetBase()->Notify(notify);
		return JS_TRUE;
	}
	return JS_FALSE;
}

/***
 * <method name="peek">
 *  <function returns="wxSocketBase">
 *   <arg name="Buffer" type="@wxMemoryBuffer" />
 *  </function>
 *  <desc>
 *   This function peeks a buffer (with the given size of the buffer) from the socket. 
 *   Peeking a buffer doesn't delete it from the socket input queue.
 *   Use @wxSocketBase#lastCount to verify the number of bytes actually peeked.
 *   Use @wxSocketBase#error to determine if the operation succeeded.
 *   Returns a reference to the current object.
 *   <br /><br />
 *   <b>Remark/Warning: </b>The exact behaviour of peek depends on the combination 
 *   of flags being used.
 *  </desc>
 * </method>
 */
JSBool SocketBase::peek(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	SocketBasePrivate *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	if ( JSVAL_IS_OBJECT(argv[0]) )
	{
		wxMemoryBuffer *buffer = wxjs::ext::GetMemoryBuffer(cx, JSVAL_TO_OBJECT(argv[0]));
		
		if ( buffer != NULL )
		{
			p->GetBase()->Peek(buffer->GetData(), buffer->GetBufSize());

			*rval = OBJECT_TO_JSVAL(obj);
			return JS_TRUE;
		}
	}
	return JS_FALSE;
}

/***
 * <method name="read">
 *  <function returns="wxSocketBase">
 *   <arg name="Buffer" type="@wxMemoryBuffer" />
 *  </function>
 *  <desc>
 *   This function reads a buffer (with the given size of the buffer) from the socket.
 *   Use @wxSocketBase#lastCount to verify the number of bytes actually read.
 *   Use @wxSocketBase#error to determine if the operation succeeded.
 *   Returns a reference to the current object.
 *   <br /><br />
 *   <b>Remark/Warning: </b>The exact behaviour of read depends on the combination 
 *   of flags being used.
 *  </desc>
 * </method>
 */
JSBool SocketBase::read(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	SocketBasePrivate *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	if ( JSVAL_IS_OBJECT(argv[0]) )
	{
		wxMemoryBuffer *buffer = wxjs::ext::GetMemoryBuffer(cx, JSVAL_TO_OBJECT(argv[0]));
		
		if ( buffer != NULL )
		{
			p->GetBase()->Read(buffer->GetData(), buffer->GetBufSize());

			*rval = OBJECT_TO_JSVAL(obj);
			return JS_TRUE;
		}
	}
	return JS_FALSE;
}

/***
 * <method name="readMsg">
 *  <function returns="wxSocketBase">
 *   <arg name="Buffer" type="@wxMemoryBuffer" />
 *  </function>
 *  <desc>
 *   This method reads a buffer sent by @wxSocketBase#writeMsg on a socket. 
 *   If the buffer passed to the function isn't big enough, the remaining bytes 
 *   will be discarded. This function always waits for the buffer to be entirely filled, 
 *   unless an error occurs.
 *   Use @wxSocketBase#lastCount to verify the number of bytes actually read.
 *   Use @wxSocketBase#error to determine if the operation succeeded.
 *   Returns a reference to the current object.
 *   <br /><br />
 *   <b>Remark/Warning: </b>readMsg will behave as if the wxSocketFlag.WAITALL flag was always
 *   set and it will always ignore the wxSocketFlag.NOWAIT flag.
 *  </desc>
 * </method>
 */
JSBool SocketBase::readMsg(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	SocketBasePrivate *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	if ( JSVAL_IS_OBJECT(argv[0]) )
	{
		wxMemoryBuffer *buffer = wxjs::ext::GetMemoryBuffer(cx, JSVAL_TO_OBJECT(argv[0]));
		
		if ( buffer != NULL )
		{
			p->GetBase()->ReadMsg(buffer->GetData(), buffer->GetBufSize());

			*rval = OBJECT_TO_JSVAL(obj);
			return JS_TRUE;
		}
	}
	return JS_FALSE;
}

/***
 * <method name="unread">
 *  <function returns="wxSocketBase">
 *   <arg name="Buffer" type="@wxMemoryBuffer" />
 *  </function>
 *  <desc>
 *   This function unreads a buffer. That is, the data in the buffer is put back in the incoming queue.
 *   This function is not affected by wxSocketFlag.
 *   Use @wxSocketBase#lastCount to verify the number of bytes actually read.
 *   Use @wxSocketBase#error to determine if the operation succeeded.
 *   Returns a reference to the current object.
 *  </desc>
 * </method>
 */
JSBool SocketBase::unread(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	SocketBasePrivate *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	if ( JSVAL_IS_OBJECT(argv[0]) )
	{
		wxMemoryBuffer *buffer = wxjs::ext::GetMemoryBuffer(cx, JSVAL_TO_OBJECT(argv[0]));
		if ( buffer != NULL )
		{
			p->GetBase()->Unread(buffer->GetData(), buffer->GetDataLen());

			*rval = OBJECT_TO_JSVAL(obj);
			return JS_TRUE;
		}
	}
	return JS_FALSE;
}

/***
 * <method name="restoreState">
 *  <function />
 *  <desc>
 *   This function restores the previous state of the socket, as saved with @wxSocketBase#saveState.
 *   Calls to saveState and restoreState can be nested. 
 *  </desc>
 * </method>
 */
JSBool SocketBase::restoreState(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	SocketBasePrivate *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	p->GetBase()->RestoreState();

	return JS_TRUE;
}

/***
 * <method name="saveState">
 *  <function />
 *  <desc>
 *   This method saves the current state of the socket in a stack. 
 *   Socket state includes flags, as set with @wxSocketBase#flags, 
 *   event mask, as set with @wxSocketBase#setNotify and @wxSocketBase#notify,
 *   user data, as set with @wxSocketBase#clientData.
 *   Calls to saveState and restoreState can be nested.
 *  </desc>
 * </method>
 */
JSBool SocketBase::saveState(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	SocketBasePrivate *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	p->GetBase()->SaveState();

	return JS_TRUE;
}

/***
 * <method name="setTimeout">
 *  <function>
 *   <arg name="Seconds" type="Integer" /> 
 *  </function>
 *  <desc>
 *   This function sets the default socket timeout in seconds. 
 *   This timeout applies to all IO calls, and also to the Wait 
 *   family of functions if you don't specify a wait interval. Initially, 
 *   the default timeout is 10 minutes.
 *  </desc>
 * </method>
 */
JSBool SocketBase::setTimeout(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	SocketBasePrivate *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	long secs;
	if ( FromJS(cx, argv[0], secs) )
	{
		p->GetBase()->SetTimeout(secs);
		return JS_TRUE;
	}

	return JS_FALSE;
}

/***
 * <method name="wait">
 *  <function returns="Boolean">
 *   <arg name="Seconds" type="Integer" default="-1" /> 
 *   <arg name="MilliSeconds" type="Integer" default="0" /> 
 *  </function>
 *  <desc>
 *   This function waits until any of the following conditions is true:
 *   <ul>
 *    <li>The socket becomes readable.</li>
 *    <li>The socket becomes writable.</li>
 *    <li>An ongoing connection request has completed (@wxSocketClient only)</li>
 *    <li>An incoming connection request has arrived (@wxSocketServer only)</li>
 *    <li>The connection has been closed.</li>
 *   </ul>
 *   Note that it is recommended to use the individual wait functions to wait for 
 *   the required condition, instead of this one.
 *   <br />Returns true when any of the above conditions is satisfied, false if the timeout was reached.
 *  </desc>
 * </method>
 */
JSBool SocketBase::wait(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
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
		*rval = ToJS(cx, p->GetBase()->Wait(secs, ms));
	}
	return JS_TRUE;	
}

/***
 * <method name="waitForLost">
 *  <function returns="Boolean">
 *   <arg name="Seconds" type="Integer" default="-1" /> 
 *   <arg name="MilliSeconds" type="Integer" default="0" /> 
 *  </function>
 *  <desc>
 *   This function waits until the connection is lost. 
 *   This may happen if the peer gracefully closes the connection or if the connection breaks.
 *   Returns true if the connection was lost, false if the timeout was reached.
 *  </desc>
 * </method>
 */
JSBool SocketBase::waitForLost(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
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
		*rval = ToJS(cx, p->GetBase()->WaitForLost(secs, ms));
	}
	return JS_TRUE;	
}

/***
 * <method name="waitForRead">
 *  <function returns="Boolean">
 *   <arg name="Seconds" type="Integer" default="-1" /> 
 *   <arg name="MilliSeconds" type="Integer" default="0" /> 
 *  </function>
 *  <desc>
 *   This function waits until the socket is readable. This might mean that queued data 
 *   is available for reading or, for streamed sockets, that the connection has been closed, 
 *   so that a read operation will complete immediately without blocking (unless the
 *   wxSocketFlag.WAITALL flag is set, in which case the operation might still block).
 *   Returns true if the socket becomes readable, false on timeout.
 *  </desc>
 * </method>
 */
JSBool SocketBase::waitForRead(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
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
		*rval = ToJS(cx, p->GetBase()->WaitForRead(secs, ms));
	}
	return JS_TRUE;	
}

/***
 * <method name="waitForWrite">
 *  <function returns="Boolean">
 *   <arg name="Seconds" type="Integer" default="-1" /> 
 *   <arg name="MilliSeconds" type="Integer" default="0" /> 
 *  </function>
 *  <desc>
 *   This function waits until the socket becomes writable. 
 *   This might mean that the socket is ready to send new data, 
 *   or for streamed sockets, that the connection has been closed, 
 *   so that a write operation is guaranteed to complete immediately 
 *   (unless the wxSocketFlag.WAITALL flag is set, in which case the operation might still block)
 *   Returns true if the socket becomes writable, false on timeout.
 *  </desc>
 * </method>
 */
JSBool SocketBase::waitForWrite(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
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
		*rval = ToJS(cx, p->GetBase()->WaitForWrite(secs, ms));
	}
	return JS_TRUE;	
}

/***
 * <method name="write">
 *  <function returns="wxSocketBase">
 *   <arg name="Buffer" type="@wxMemoryBuffer" />
 *  </function>
 *  <desc>
 *   This function writes a buffer (with the given size of the buffer) to the socket.
 *   Use @wxSocketBase#lastCount to verify the number of bytes actually written.
 *   Use @wxSocketBase#error to determine if the operation succeeded.
 *   Returns a reference to the current object.
 *   <br /><br />
 *   <b>Remark/Warning: </b>The exact behaviour of write depends on the combination 
 *   of flags being used.
 *  </desc>
 * </method>
 */
JSBool SocketBase::write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	SocketBasePrivate *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	if ( JSVAL_IS_OBJECT(argv[0]) )
	{
		wxMemoryBuffer *buffer = wxjs::ext::GetMemoryBuffer(cx, JSVAL_TO_OBJECT(argv[0]));
		
		if ( buffer != NULL )
		{
			p->GetBase()->Write(buffer->GetData(), buffer->GetDataLen());
			*rval = OBJECT_TO_JSVAL(obj);
			return JS_TRUE;
		}
	}
	return JS_FALSE;
}

/***
 * <method name="writeMsg">
 *  <function returns="wxSocketBase">
 *   <arg name="Buffer" type="@wxMemoryBuffer" />
 *  </function>
 *  <desc>
 *   This function writes a buffer to the socket, but it writes a short header 
 *   before so that @wxSocketBase#readMsg knows how much data should it actually read.
 *   So, a buffer sent with writeMsg must be read with @wxSocketBase#readMsg. 
 *   This function always waits for the entire buffer to be sent, unless an error occurs.
 *   Use @wxSocketBase#lastCount to verify the number of bytes actually read.
 *   Use @wxSocketBase#error to determine if the operation succeeded.
 *   Returns a reference to the current object.
 *   <br /><br />
 *   <b>Remark/Warning: </b>writeMsg will behave as if the wxSocketFlag.WAITALL flag was always
 *   set and it will always ignore the wxSocketFlag.NOWAIT flag.
 *  </desc>
 * </method>
 */
JSBool SocketBase::writeMsg(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	SocketBasePrivate *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	if ( JSVAL_IS_OBJECT(argv[0]) )
	{
		wxMemoryBuffer *buffer = wxjs::ext::GetMemoryBuffer(cx, JSVAL_TO_OBJECT(argv[0]));
		
		if ( buffer != NULL )
		{
			p->GetBase()->WriteMsg(buffer->GetData(), buffer->GetDataLen());

			*rval = OBJECT_TO_JSVAL(obj);
			return JS_TRUE;
		}
	}
	return JS_FALSE;
}
