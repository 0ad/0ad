#include "precompiled.h"

/*
 * wxJavaScript - ipv4addr.cpp
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
 * $Id: ipv4addr.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../common/main.h"
#include "ipv4addr.h"

using namespace wxjs;
using namespace wxjs::io;

/***
 * <file>ipv4addr</file>
 * <module>io</module>
 * <class name="wxIPV4address" prototype="@wxIPaddress" version="0.8.4">
 *  Implements the IPV4 of the internet protocal.
 * </class>
 */
WXJS_INIT_CLASS(IPV4address, "wxIPV4address", 0)

wxIPV4address* IPV4address::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
	return new wxIPV4address();
}
