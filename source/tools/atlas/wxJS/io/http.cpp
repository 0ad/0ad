#include "precompiled.h"

/*
 * wxJavaScript - http.cpp
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
 * $Id: http.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../common/main.h"
#include "constant.h"
#include "jsstream.h"
#include "sockbase.h"
#include "http.h"
#include "httphdr.h"
#include "sockaddr.h"

using namespace wxjs;
using namespace wxjs::io;

/***
 * <file>http</file>
 * <module>io</module>
 * <class name="wxHTTP" prototype="@wxProtocol" version="0.8.4">
 *  Implements the HTTP protocol.
 * </class>
 */
WXJS_INIT_CLASS(HTTP, "wxHTTP", 0)

/***
 * <ctor>
 *  <function />
 *  <desc>
 *   Creates a new wxHTTP object
 *  </desc>
 * </ctor>
 */
SocketBasePrivate *HTTP::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
	return new SocketBasePrivate(new wxHTTP());
}

void HTTP::Destruct(JSContext *cx, SocketBasePrivate *p)
{
	p->DestroyStreams(cx);
	delete p;
}

/***
 * <properties>
 *  <property name="headers" type="Array" readonly="Y">
 *   Contains the headers. Access the elements of the array with String keys.
 *  </property>
 *  <property name="response" type="Integer" readonly="Y">
 *   Returns the HTTP response code returned by the server.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(HTTP)
	WXJS_READONLY_PROPERTY(P_RESPONSE, "response")
	WXJS_READONLY_PROPERTY(P_HEADERS, "headers")
WXJS_END_PROPERTY_MAP()

bool HTTP::GetProperty(SocketBasePrivate *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	wxHTTP *http = dynamic_cast<wxHTTP *>(p->GetBase());
	switch(id)
	{
	case P_RESPONSE:
		*vp = ToJS(cx, http->GetResponse());
		break;
	case P_HEADERS:
		*vp = HTTPHeader::CreateObject(cx, new HTTPHeader(), obj);
		break;
	}
	return true;
}
