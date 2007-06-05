#include "precompiled.h"

/*
 * wxJavaScript - uri.cpp
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
 * $Id: uri.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../common/main.h"
#include "uri.h"

using namespace wxjs;
using namespace wxjs::io;

/***
 * <file>uri</file>
 * <module>io</module>
 * <class name="wxURI" version="0.8.4">
 *  wxURI is used to extract information from a URI (Uniform Resource Identifier).
 * </class>
 */
WXJS_INIT_CLASS(URI, "wxURI", 1)

/***
 * <constants>
 *  <type name="HostType">
 *   <constant name="REGNAME">Server is a host name, or the Server component itself is undefined.</constant>
 *   <constant name="IPV4ADDRESS">Server is a IP version 4 address (XXX.XXX.XXX.XXX)</constant>
 *   <constant name="IPV6ADDRESS">Server is a IP version 6 address ((XXX:)XXX::(XXX)XXX:XXX</constant>
 *   <constant name="IPVFUTURE">Server is an IP address, but not versions 4 or 6</constant>
 *  </type>
 * </constants>
 */
WXJS_BEGIN_CONSTANT_MAP(URI)
	WXJS_CONSTANT(wxURI_, REGNAME)
	WXJS_CONSTANT(wxURI_, IPV4ADDRESS)
	WXJS_CONSTANT(wxURI_, IPV6ADDRESS)
	WXJS_CONSTANT(wxURI_, IPVFUTURE)
WXJS_END_CONSTANT_MAP()

/***
 * <ctor>
 *  <function>
 *   <arg name="uri" type="String" />
 *  </function>
 *  <desc>
 *   Creates a new wxURI object
 *  </desc>
 * </ctor>
 */
wxURI *URI::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
	wxString uri;
	FromJS(cx, argv[0], uri);
	return new wxURI(uri);
}

/***
 * <properties>
 *  <property name="fragment" type="String" readonly="Y">
 *   The fragment of a URI is the last value of the URI.
 *  </property>
 *  <property name="hostType" type="Integer" readonly="Y">
 *   Obtains the host type of this URI, which is of type @wxURI#HostType.
 *  </property>
 *  <property name="password" type="String" readonly="Y">
 *   Returns the password part of the userinfo component of this URI. 
 *   Note that this is explicitly depreciated by RFC 1396 and should generally be avoided if possible.
 *  </property>
 *  <property name="path" type="String" readonly="Y">
 *   Returns the (normalized) path of the URI.
 *  </property>
 *  <property name="port" type="String" readonly="Y">
 *   Returns a string representation of the URI's port.
 *  </property>
 *  <property name="query" type="String" readonly="Y">
 *   Returns the Query component of the URI.
 *  </property>
 *  <property name="scheme" type="String" readonly="Y">
 *   Returns the Scheme component of the URI.
 *  </property>
 *  <property name="server" type="String" readonly="Y">
 *   Returns the Server component of the URI.
 *  </property>
 *  <property name="user" type="String" readonly="Y">
 *   Returns the username component of the URI.
 *  </property>
 *  <property name="userInfo" type="String" readonly="Y">
 *   Returns the userinfo component of the URI.
 *  </property>
 *  <property name="hasPath" type="Boolean" readonly="Y">
 *   Returns true if the path component of the URI exists.
 *  </property>
 *  <property name="hasPort" type="Boolean" readonly="Y">
 *   Returns true if the port component of the URI exists.
 *  </property>
 *  <property name="hasQuery" type="Boolean" readonly="Y">
 *   Returns true if the query component of the URI exists.
 *  </property>
 *  <property name="hasScheme" type="Boolean" readonly="Y">
 *   Returns true if the scheme component of the URI exists.
 *  </property>
 *  <property name="hasServer" type="Boolean" readonly="Y">
 *   Returns true if the server component of the URI exists.
 *  </property>
 *  <property name="hasUser" type="Boolean" readonly="Y">
 *   Returns true if the user component of the URI exists.
 *  </property>
 *  <property name="isReference" type="Boolean" readonly="Y">
 *   Returns true if a valid [absolute] URI, otherwise this URI
 *   is a URI reference and not a full URI, and IsReference returns false.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(URI)
	WXJS_READONLY_PROPERTY(P_FRAGMENT, "fragment")
	WXJS_READONLY_PROPERTY(P_HOST_TYPE, "hostType")
	WXJS_READONLY_PROPERTY(P_PASSWORD, "password")
	WXJS_READONLY_PROPERTY(P_PATH, "path")
	WXJS_READONLY_PROPERTY(P_PORT, "port")
	WXJS_READONLY_PROPERTY(P_QUERY, "query")
	WXJS_READONLY_PROPERTY(P_SCHEME, "scheme")
	WXJS_READONLY_PROPERTY(P_SERVER, "server")
	WXJS_READONLY_PROPERTY(P_USER, "user")
	WXJS_READONLY_PROPERTY(P_USERINFO, "userInfo")
	WXJS_READONLY_PROPERTY(P_HAS_PATH, "hasPath")
	WXJS_READONLY_PROPERTY(P_HAS_PORT, "hasPort")
	WXJS_READONLY_PROPERTY(P_HAS_QUERY, "hasQuery")
	WXJS_READONLY_PROPERTY(P_HAS_SCHEME, "hasScheme")
	WXJS_READONLY_PROPERTY(P_HAS_SERVER, "hasServer")
	WXJS_READONLY_PROPERTY(P_HAS_USER, "hasUser")
	WXJS_READONLY_PROPERTY(P_IS_REFERENCE, "isReference")
WXJS_END_PROPERTY_MAP()

bool URI::GetProperty(wxURI *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	switch(id)
	{
	case P_FRAGMENT:
		*vp = ToJS(cx, p->GetFragment());
		break;
	case P_HOST_TYPE:
		*vp = ToJS<int>(cx, p->GetHostType());
		break;
	case P_PASSWORD:
		*vp = ToJS(cx, p->GetPassword());
		break;
	case P_PATH:
		*vp = ToJS(cx, p->GetPath());
		break;
	case P_PORT:
		*vp = ToJS(cx, p->GetPort());
		break;
	case P_QUERY:
		*vp = ToJS(cx, p->GetQuery());
		break;
	case P_SCHEME:
		*vp = ToJS(cx, p->GetScheme());
		break;
	case P_USER:
		*vp = ToJS(cx, p->GetUser());
		break;
	case P_USERINFO:
		*vp = ToJS(cx, p->GetUserInfo());
		break;
	case P_HAS_PATH:
		*vp = ToJS(cx, p->HasPath());
		break;
	case P_HAS_PORT:
		*vp = ToJS(cx, p->HasPort());
		break;
	case P_HAS_QUERY:
		*vp = ToJS(cx, p->HasQuery());
		break;
	case P_HAS_SCHEME:
		*vp = ToJS(cx, p->HasScheme());
		break;
	case P_HAS_USER:
		*vp = ToJS(cx, p->HasUserInfo());
		break;
	case P_IS_REFERENCE:
		*vp = ToJS(cx, p->IsReference());
		break;
	}
	return true;
}

WXJS_BEGIN_METHOD_MAP(URI)
	WXJS_METHOD("buildURI", buildURI, 0)
	WXJS_METHOD("buildUnescapedURI", buildUnescapedURI, 0)
	WXJS_METHOD("create", create, 1)
	WXJS_METHOD("resolve", resolve, 2)
	WXJS_METHOD("unescape", unescape, 1)
WXJS_END_METHOD_MAP()

/***
 * <method name="buildURI">
 *  <function returns="String" />
 *  <desc>
 *   Builds the URI from its individual components and adds proper separators.
 *  </desc>
 * </method>
 */
JSBool URI::buildURI(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxURI *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;
	
	*rval = ToJS(cx, p->BuildURI());
	return JS_TRUE;
}

/***
 * <method name="buildUnescapedURI">
 *  <function returns="String" />
 *  <desc>
 *   Builds the URI from its individual components, 
 *   adds proper separators, and returns escape sequences to normal characters.
 *  </desc>
 * </method>
 */
JSBool URI::buildUnescapedURI(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxURI *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;
	
	*rval = ToJS(cx, p->BuildUnescapedURI());
	return JS_TRUE;
}

/***
 * <method name="buildUnescapedURI">
 *  <function>
 *   <arg name="URI" type="String" />
 *  </function>
 *  <desc>
 *   Creates this URI from the string uri
 *  </desc>
 * </method>
 */
JSBool URI::create(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxURI *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	wxString uri;
	FromJS(cx, argv[0], uri);
	p->Create(uri);
	return JS_TRUE;
}

/***
 * <method name="resolve">
 *  <function>
 *   <arg name="URI" type="@wxURI" />
 *   <arg name="Strict" type="Boolean" default="True" />
 *  </function>
 *  <desc>
 *   Inherits this URI from a base URI - components that do not exist in this URI are 
 *   copied from the base, and if this URI's path is not an absolute path (prefixed by a '/'), 
 *   then this URI's path is merged with the base's path.
 *   <br /><br />
 *   For instance, resolving "../mydir" from "http://mysite.com/john/doe" results in the scheme
 *   (http) and server (mysite.com) being copied into this URI, since they do not exist. 
 *   In addition, since the path of this URI is not absolute (does not begin with '/'), 
 *   the path of the base's is merged with this URI's path, resulting in the 
 *   URI "http://mysite.com/john/mydir".
 *  </desc>
 * </method>
 */
JSBool URI::resolve(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxURI *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	wxURI *uri = GetPrivate(cx, argv[0]);
	if ( uri == NULL )
		return JS_FALSE;

	bool strict = true;
	if (    argc > 1 
		 && !FromJS(cx, argv[1], strict) )
	{
		return JS_FALSE;
	}

	p->Resolve(*uri, strict ? wxURI_STRICT : 0);
	return JS_TRUE;
}

/***
 * <method name="Unescape">
 *  <function returns="Boolean">
 *   <arg name="URI" type="String" />
 *  </function>
 *  <desc>
 *   Translates all escape sequences (normal characters and returns the result
 *  </desc>
 * </method>
 */
JSBool URI::unescape(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxURI *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	wxString uri;
	FromJS(cx, argv[0], uri);
	*rval = ToJS(cx, p->Unescape(uri));
	return JS_TRUE;
}

