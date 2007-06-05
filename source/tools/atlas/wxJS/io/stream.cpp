#include "precompiled.h"

/*
 * wxJavaScript - stream.cpp
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
 * $Id: stream.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// stream.cpp

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../common/main.h"

#include "stream.h"

using namespace wxjs;
using namespace wxjs::io;

/***
 * <file>streambase</file>
 * <module>io</module>
 * <class name="wxStreamBase" version="0.8.2" >
 *  wxStreamBase is a prototype class. It's the prototype of the Stream classes.
 *  See @wxInputStream, @wxOutputStream.
 * </class>
 */
WXJS_INIT_CLASS(StreamBase, "wxStreamBase", 0)

/***
 * <properties>
 *  <property name="lastError" type="Integer" readonly="Y">
 *   Returns the last error. 
 *  </property>
 *  <property name="ok" type="Boolean" readonly="Y">
 *   Returns true if no error occurred.
 *  </property>
 *  <property name="size" type="Integer" readonly="Y">
 *   Returns the size of the stream. For example, for a file it is the size of the file.
 *   There are streams which do not have size by definition, such as socket streams. 
 *   In these cases, size returns 0.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(StreamBase)
    WXJS_READONLY_PROPERTY(P_OK, "ok")
    WXJS_READONLY_PROPERTY(P_SIZE, "size")
    WXJS_READONLY_PROPERTY(P_LAST_ERROR, "lastError")
WXJS_END_PROPERTY_MAP()

bool StreamBase::GetProperty(Stream *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    wxStreamBase *stream = p->GetStream();
    switch (id)
    {
    case P_OK:
        *vp = ToJS(cx, stream->IsOk());
        break;
    case P_SIZE:
        *vp = ToJS(cx, (long) stream->GetSize());
        break;
    case P_LAST_ERROR:
        *vp = ToJS(cx, (int) stream->GetLastError());
        break;
    }
    return true;
}
