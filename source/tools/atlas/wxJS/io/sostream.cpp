#include "precompiled.h"

/*
 * wxJavaScript - sostream.cpp
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
 * $Id: sostream.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../common/main.h"
#include "stream.h"
#include "sockbase.h"
#include "sostream.h"

using namespace wxjs;
using namespace wxjs::io;

/***
 * <file>sostream</file>
 * <module>io</module>
 * <class name="wxSocketOutputStream" prototype="@wxOutputStream" version="0.8.4">
 *  This class implements an output stream which writes data from a connected socket.
 *  Note that this stream is purely sequential and it does not support seeking.
 * </class>
 */
WXJS_INIT_CLASS(SocketOutputStream, "wxSocketOutputStream", 1)

/***
 * <ctor>
 *  <function>
 *   <arg name="Socket" type="@wxSocketBase">A socket</arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxSocketOutputStream object.
 *  </desc>
 * </ctor>
 */
Stream* SocketOutputStream::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    if ( SocketBase::HasPrototype(cx, argv[0]) )
    {
        SocketBasePrivate *base = SocketBase::GetPrivate(cx, argv[0], false);
		// This is needed, because otherwise the socket can be garbage collected.
		// Another method could be to root socket, but how are we going to unroot it?
		JS_DefineProperty(cx, obj, "__socket__", argv[0], NULL, NULL, JSPROP_READONLY);

		return new Stream(new wxSocketOutputStream(*base->GetBase()));
    }
    return NULL;
}
