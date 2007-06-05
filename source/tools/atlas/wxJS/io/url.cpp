#include "precompiled.h"

/*
 * wxJavaScript - url.cpp
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
 * $Id: url.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../common/main.h"
#include "jsstream.h"
#include "istream.h"
#include "url.h"

using namespace wxjs;
using namespace wxjs::io;

/***
 * <file>url</file>
 * <module>io</module>
 * <class name="wxURL" prototype="@wxURI" version="0.8.4">
 *  Parses URLs.
 * </class>
 */
WXJS_INIT_CLASS(URL, "wxURL", 1)

/***
 * <constants>
 *  <type name="wxURLError">
 *   <constant name="NOERR">No error.</constant>
 *   <constant name="SNTXERR">Syntax error in the URL string.</constant>
 *   <constant name="NOPROTO">Found no protocol which can get this URL.</constant>
 *   <constant name="NOHOST">An host name is required for this protocol.</constant>
 *   <constant name="NOPATH">A path is required for this protocol.</constant>
 *   <constant name="CONNERR">Connection error.</constant>
 *   <constant name="PROTOERR">An error occurred during negotiation.</constant>
 *   <desc>
 *    wxURLError is ported to JavaScript as a separate class. Note that this
 *    class doesn't exist in wxWidgets.
 *   </desc>
 *  </type>
 * </constants>
 */
void URL::InitClass(JSContext *cx, JSObject *obj, JSObject *proto)
{
    JSConstDoubleSpec wxURLErrorMap[] = 
    {
		WXJS_CONSTANT(wxURL_, NOERR)
		WXJS_CONSTANT(wxURL_, SNTXERR)
		WXJS_CONSTANT(wxURL_, NOPROTO)
		WXJS_CONSTANT(wxURL_, NOHOST)
		WXJS_CONSTANT(wxURL_, NOPATH)
		WXJS_CONSTANT(wxURL_, CONNERR)
		WXJS_CONSTANT(wxURL_, PROTOERR)
		{ 0 }
    };

    JSObject *constObj = JS_DefineObject(cx, obj, "wxURLError", 
									 	 NULL, NULL,
							             JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineConstDoubles(cx, constObj, wxURLErrorMap);
}

/***
 * <ctor>
 *  <function>
 *   <arg name="Url" type="String" />
 *  </function>
 *  <desc>
 *   Constructs a URL object from the string. The URL must be valid according to RFC 1738.
 *   In particular, file URLs must be of the format 'file://hostname/path/to/file'. It is valid 
 *   to leave out the hostname but slashes must remain in place-- i.e. a file URL without a
 *   hostname must contain three consecutive slashes.
 *  </desc>
 * </ctor>
 */
wxURL *URL::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
	wxString url;
	FromJS(cx, argv[0], url);
	return new wxURL(url);
}

/***
 * <properties>
 *  <property name="error" type="Integer" readonly="Y">
 *   Returns the last occurred error. See @wxURL#wxURLError.
 *  </property>
 *  <property name="inputStream" type="@wxInputStream" readonly="Y">
 *   Creates a new input stream on the specified URL. You can use all but 
 *   seek functionality of a stream. Seek isn't available on all streams.
 *   For example, http or ftp streams doesn't deal with it.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(URL)
	WXJS_READONLY_PROPERTY(P_ERROR, "error")
	WXJS_READONLY_PROPERTY(P_INPUT_STREAM, "inputStream")
WXJS_END_PROPERTY_MAP()

bool URL::GetProperty(wxURL *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	switch(id)
	{
	case P_ERROR:
		*vp = ToJS<int>(cx, p->GetError());
		break;
	case P_INPUT_STREAM:
		{
			wxInputStream *stream = p->GetInputStream();
			*vp = InputStream::CreateObject(cx, new Stream(stream), NULL);
			break;
		}
	}
	return true;
}

WXJS_BEGIN_METHOD_MAP(URL)
	WXJS_METHOD("setProxy", setProxy, 1)
WXJS_END_METHOD_MAP()

/***
 * <method name="setProxy">
 *  <function>
 *   <arg name="proxy" type="String" />
 *  </function>
 *  <desc>
 *   Sets the proxy to use for this URL.
 *  </desc>
 * </method>
 */
JSBool URL::setProxy(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxURL *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	wxString proxy;
	FromJS(cx, argv[0], proxy);
	p->SetProxy(proxy);
	return JS_TRUE;
}

WXJS_BEGIN_STATIC_METHOD_MAP(URL)
	WXJS_METHOD("setDefaultProxy", setDefaultProxy, 1)
WXJS_END_METHOD_MAP()

/***
 * <class_method name="setDefaultProxy">
 *  <function>
 *   <arg name="proxy" type="String" />
 *  </function>
 *  <desc>
 *   Sets the default proxy server to use to get the URL. 
 *   The string specifies the proxy like this: &lt;hostname&gt;:&lt;port number&gt;.
 *  </desc>
 * </class_method>
 */
JSBool URL::setDefaultProxy(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxString proxy;
	FromJS(cx, argv[0], proxy);
	wxURL::SetDefaultProxy(proxy);
	return JS_TRUE;
}
