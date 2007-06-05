#include "precompiled.h"

/*
 * wxJavaScript - sockaddr.cpp
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
 * $Id: sockaddr.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../common/main.h"
#include "sockaddr.h"

using namespace wxjs;
using namespace wxjs::io;

/***
 * <file>sockaddr</file>
 * <module>io</module>
 * <class name="wxSockAddress" version="0.8.4">
 *  Prototype class for socket addresses.
 * </class>
 */
WXJS_INIT_CLASS(SockAddress, "wxSockAddress", 0)

WXJS_BEGIN_METHOD_MAP(SockAddress)
	WXJS_METHOD("clear", clear, 0) 
WXJS_END_METHOD_MAP()

/***
 * <method name="clear">
 *  <function />
 *  <desc>
 *   Delete all information of the address.
 *  </desc>
 * </method>
 */
JSBool SockAddress::clear(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxSockAddress *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	p->Clear();
	return JS_TRUE;
}
