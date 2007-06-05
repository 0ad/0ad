#include "precompiled.h"

/*
 * wxJavaScript - ostream.cpp
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
 * $Id: ostream.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../common/main.h"
#include "../ext/wxjs_ext.h"
#include "../ext/membuf.h"

#include "stream.h"
#include "istream.h"
#include "ostream.h"

using namespace wxjs;
using namespace wxjs::io;

/***
 * <file>ostream</file>
 * <module>io</module>
 * <class name="wxOutputStream" prototpye="@wxStreamBase" version="0.8.2">
 *  wxOutputStream is a prototype for output streams. You can't construct it directly.
 *  See @wxMemoryOutputStream, @wxFileOutputStream, @wxFFileOutputStream.
 * </class>
 */
WXJS_INIT_CLASS(OutputStream, "wxOutputStream", 0)

/***
 * <properties>
 *  <property name="lastWrite" type="Integer" readonly="Y">
 *   Gets the last number of bytes written.
 *  </property> 
 *  <property name="tellO" type="Integer" readonly="Y">
 *   Returns the current position.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(OutputStream)
    WXJS_READONLY_PROPERTY(P_LAST_WRITE, "lastWrite")
    WXJS_READONLY_PROPERTY(P_TELL_O, "tellO")
WXJS_END_PROPERTY_MAP()

bool OutputStream::GetProperty(Stream *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    wxOutputStream *stream = (wxOutputStream *) p->GetStream();
    switch (id)
    {
    case P_LAST_WRITE:
        *vp = ToJS(cx, (long) stream->LastWrite());
        break;
    case P_TELL_O:
        *vp = ToJS(cx, (long) stream->TellO());
        break;
    }
    return true;
}

WXJS_BEGIN_METHOD_MAP(OutputStream)
    WXJS_METHOD("close", close, 0)
    WXJS_METHOD("putC", putC, 1)
    WXJS_METHOD("write", write, 1)
    WXJS_METHOD("seekO", seekO, 1)
    WXJS_METHOD("sync", sync, 0)
WXJS_END_METHOD_MAP()

/***
 * <method name="close">
 *  <function returns="Boolean" />
 *  <desc>
 *   Closes the stream
 *  </desc>
 * </method>
 */
JSBool OutputStream::close(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    Stream *p = GetPrivate(cx, obj);
    if ( p != NULL )
    {
        wxOutputStream *out = (wxOutputStream *) p->GetStream();
        *rval = ToJS(cx, out->Close());
    }
    return JS_TRUE;
}

/***
 * <method name="putC">
 *  <function>
 *   <arg name="Char" type="String" />
 *  </function>
 *  <desc>
 *   The first character of the String is written to the output stream.
 *  </desc>
 * </method>
 */
JSBool OutputStream::putC(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    Stream *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxOutputStream *out = (wxOutputStream *) p->GetStream();

    wxString Buffer;
    FromJS(cx, argv[0], Buffer);
    if ( Buffer.length() > 0 )
    {
        out->PutC(Buffer.at(0));
    }
    return JS_TRUE;
}

/***
 * <method name="write">
 *  <function returns="Integer">
 *   <arg name="Buffer" type="@wxMemoryBuffer" />
 *   <arg name="Count" type="Integer">The number of characters to write. Default is the length of buffer.</arg>
 *  </function>
 *  <function returns="Integer">
 *   <arg name="Str" type="String" />
 *  </function>
 *  <function returns="Integer">
 *   <arg name="Input" type="@wxInputStream" />
 *  </function>
 *  <desc>
 *  1. Writes the buffer to the outputstream. Unlike wxWindows, the size of the buffer must not
 *  be specified. If ommitted then the full buffer is written.
 *  <br /><br />
 *  2. Reads data from the specified input stream and stores them in the current stream. 
 *  The data is read until an error is raised by one of the two streams.
 *  <br /><br />
 *  Unlike wxWidgets, this method returns the number of bytes written.
 *  </desc>
 * </method>
 */
JSBool OutputStream::write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    Stream *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxOutputStream *out = (wxOutputStream *) p->GetStream();

    if ( InputStream::HasPrototype(cx, argv[0]) )
    {
        Stream *p_in = InputStream::GetPrivate(cx, argv[0], false);
        wxInputStream *in = (wxInputStream *) p_in->GetStream();
        out->Write(*in);
    }
    else
    {
		if ( JSVAL_IS_OBJECT(argv[0]) )
		{
			wxMemoryBuffer* buffer = wxjs::ext::GetMemoryBuffer(cx, JSVAL_TO_OBJECT(argv[0]));
			if ( buffer != NULL )
			{
				*rval = ToJS(cx, out->Write(buffer->GetData(), buffer->GetDataLen()).LastWrite());
				return JS_TRUE;
			}
		}

        wxString buffer;
        FromJS(cx, argv[0], buffer);

        int count = buffer.length();
        if ( argc > 1 )
        {
          if ( FromJS(cx, argv[1], count) )
          {
            if ( count > (int) buffer.length() )
                count = buffer.length();
          }
          else
              return JS_FALSE;
        }

		*rval = ToJS(cx, out->Write(buffer, count).LastWrite());
    }
    return JS_TRUE;
}

/***
 * <method name="seekO">
 *  <function returns="Integer">
 *   <arg name="Offset" type="Integer">Offset to seek to</arg>
 *   <arg name="Mode" type="@wxFile#wxSeekMode" default="wxSeekMode.FromStart" />
 *  </function>
 *  <desc>
 *   Seeks the offset. Returns the actual position or -1 on error.
 *  </desc>
 * </method>
 */
JSBool OutputStream::seekO(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    Stream *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxOutputStream *out = (wxOutputStream *) p->GetStream();

    int offset;
	if ( ! FromJS(cx, argv[0], offset) )
		return JS_FALSE;

	int pos;

	if ( argc > 1 )
	{
		int mode;
		if ( FromJS(cx, argv[1], mode) )
		{
			pos = out->SeekO(offset, (wxSeekMode) mode);
		}
		else
			return JS_FALSE;
	}
	else
		pos = out->SeekO(offset);

	*rval = ToJS(cx, pos);
	return JS_TRUE;
}

/***
 * <method name="sync">
 *  <function />
 *  <desc>
 *   Flushes the buffer.
 *  </desc>
 * </method>
 */
JSBool OutputStream::sync(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    Stream *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxOutputStream *out = (wxOutputStream *) p->GetStream();

    out->Sync();
    return JS_TRUE;
}

