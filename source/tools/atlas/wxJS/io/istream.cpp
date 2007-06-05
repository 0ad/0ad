#include "precompiled.h"

/*
 * wxJavaScript - istream.cpp
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
 * $Id: istream.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../common/main.h"
#include "../ext/wxjs_ext.h"

#include "stream.h"
#include "ostream.h"
#include "istream.h"

using namespace wxjs;
using namespace wxjs::io;

/***
 * <file>istream</file>
 * <module>io</module>
 * <class name="wxInputStream" prototype="@wxStreamBase" version="0.8.2">
 *  wxInputStream is a prototype for input streams. You can't construct it directly.
 *  See @wxMemoryInputStream, @wxFileInputStream and @wxFFileInputStream.
 * </class>
 */
WXJS_INIT_CLASS(InputStream, "wxInputStream", 0)

/***
 * <properties>
 *  <property name="c" type="String">
 *   Returns the first character in the input queue and removes it.
 *   When set it puts back the character or full String in the input queue.
 *  </property>
 *  <property name="eof" type="Boolean" readonly="Y">
 *   Returns true when end-of-file occurred.
 *  </property>
 *  <property name="lastRead" type="Integer" readonly="Y">
 *   Returns the last number of bytes read.
 *  </property>
 *  <property name="peek" type="String" readonly="Y">
 *   Returns the first character in the input queue without removing it.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(InputStream)
    WXJS_PROPERTY(P_C, "c")
    WXJS_READONLY_PROPERTY(P_EOF, "eof")
    WXJS_READONLY_PROPERTY(P_LAST_READ, "lastRead")
    WXJS_READONLY_PROPERTY(P_PEEK, "peek")
    WXJS_READONLY_PROPERTY(P_TELL_I, "tellI")
WXJS_END_PROPERTY_MAP()

bool InputStream::GetProperty(Stream *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    wxInputStream *stream = (wxInputStream*) p->GetStream();
    switch (id)
    {
    case P_C:
        *vp = ToJS(cx, wxString::FromAscii(stream->GetC()));
        break;
    case P_EOF:
        *vp = ToJS(cx, stream->Eof());
        break;
    case P_LAST_READ:
        *vp = ToJS(cx, (int) stream->LastRead());
        break;
    case P_PEEK:
		*vp = ToJS(cx, wxString::FromAscii(stream->Peek()));
        break;
    }
    return true;
}

bool InputStream::SetProperty(Stream *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    if ( id == P_C )
    {
        wxInputStream *stream = (wxInputStream*) p->GetStream();
        wxString s;
        FromJS(cx, *vp, s);
        if ( s.length() > 0 )
        {
            stream->Ungetch(s, s.length());
        }
    }
    return true;
}

WXJS_BEGIN_METHOD_MAP(InputStream)
    WXJS_METHOD("getC", getC, 0)
    WXJS_METHOD("peek", peek, 0)
    WXJS_METHOD("read", read, 1)
    WXJS_METHOD("seekI", seekI, 1)
    WXJS_METHOD("ungetch", ungetch, 1)
WXJS_END_METHOD_MAP()

/***
 * <method name="getC">
 *  <function returns="String" />
 *  <desc>  
 *   Returns the next character from the input queue and removes it.
 *  </desc>
 * </method>
 */
JSBool InputStream::getC(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    Stream *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxInputStream *in = (wxInputStream *) p->GetStream();
	*rval = ToJS(cx, wxString::FromAscii(in->GetC()));
    return JS_TRUE;
}

/***
 * <method name="peek">
 *  <function returns="String" />
 *  <desc>
 *   Returns the next character from the input queue without removing it.
 *  </desc>
 * </method>
 */
JSBool InputStream::peek(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    Stream *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxInputStream *in = (wxInputStream *) p->GetStream();

	*rval = ToJS(cx, wxString::FromAscii(in->Peek()));
    return JS_TRUE;
}

/***
 * <method name="read">
 *  <function returns="wxInputStream">
 *   <arg name="Buffer" type="@wxMemoryBuffer" />
 *  </function>
 *  <function returns="wxInputStream">
 *   <arg name="Output" type="@wxOutputStream">
 *    The outputstream that gets the data from the input queue.
 *   </arg>
 *  </function>
 *  <desc>
 *   Reads the specified number of bytes (the size of the buffer) and stores them in the buffer.
 *   <br /><br />
 *   Reads data from the input queue and stores it in the specified output stream. 
 *   The data is read until an error is raised by one of the two streams.
 *  </desc>
 * </method>
 */
JSBool InputStream::read(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    Stream *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxInputStream *in = (wxInputStream *) p->GetStream();

    if ( OutputStream::HasPrototype(cx, argv[0]) )
    {
        Stream *p_out = OutputStream::GetPrivate(cx, argv[0], false);
        wxOutputStream *out = (wxOutputStream *) p_out->GetStream();
        in->Read(*out);
		*rval = OBJECT_TO_JSVAL(obj);
        return JS_TRUE;
    }
    else
    {
        wxMemoryBuffer *buffer = wxjs::ext::GetMemoryBuffer(cx, JSVAL_TO_OBJECT(argv[0]));
		
		if ( buffer != NULL )
		{
			in->Read(buffer->GetData(), buffer->GetBufSize());
            buffer->SetDataLen(in->LastRead());
			*rval = OBJECT_TO_JSVAL(obj);
			return JS_TRUE;
		}
	}
    return JS_FALSE;
}

/***
 * <method name="seekI">
 *  <function returns="Integer">
 *   <arg name="Offset" type="Integer">
 *    Offset to seek to
 *   </arg>
 *   <arg name="Mode" type="@wxFile#wxSeekMode" default="wxSeekMode.FromStart" />
 *  </function>
 *  <desc>
 *   Seeks the offset. Returns the actual position or -1 on error.
 *  </desc>
 * </method>
 */
JSBool InputStream::seekI(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    Stream *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxInputStream *in = (wxInputStream *) p->GetStream();

    int offset;
	if ( ! FromJS(cx, argv[0], offset) )
		return JS_FALSE;

	int pos;

	if ( argc > 1 )
	{
		int mode;
		if ( FromJS(cx, argv[1], mode) )
		{
			pos = in->SeekI(offset, (wxSeekMode) mode);
		}
		else
			return JS_FALSE;
	}
	else
		pos = in->SeekI(offset);

	*rval = ToJS(cx, pos);
	return JS_TRUE;
}

/***
 * <method name="ungetch">
 *  <function returns="Integer">
 *   <arg name="Buffer" type="String" />
 *  </function>
 *  <function returns="Integer">
 *   <arg name="Buffer" type="@wxMemoryBuffer" />
 *  </function>
 *  <desc>
 *   This function is only useful in read mode. It is the manager of the "Write-Back" buffer. 
 *   This buffer acts like a temporary buffer where datas which has to be read during the next 
 *   read IO call are put. This is useful when you get a big block of data which you didn't want 
 *   to read: you can replace them at the top of the input queue by this way.
 *   <br /><br />
 *   Be very careful about this call in connection with calling @wxInputStream#seekI on the 
 *   same stream. Any call to @wxInputStream#seekI will invalidate any previous call to this 
 *   method (otherwise you could @wxInputStream#seekI to one position, "unread" a few bytes
 *   there, @wxInputStream#seekI to another position and data would be either lost or corrupted).
 *  </desc>
 * </method>
 */
JSBool InputStream::ungetch(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    Stream *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxInputStream *in = (wxInputStream *) p->GetStream();

	if ( JSVAL_IS_OBJECT(argv[0]) )
	{
		wxMemoryBuffer* buffer = wxjs::ext::GetMemoryBuffer(cx, JSVAL_TO_OBJECT(argv[0]));
		if ( buffer != NULL )
		{
			*rval = ToJS(cx, (int) in->Ungetch(buffer->GetData(), buffer->GetDataLen()));
			return JS_TRUE;
		}
	}
    
	wxString s;
    FromJS(cx, argv[0], s);
    if ( s.length() > 0 )
    {
        *rval = ToJS(cx, (int) in->Ungetch(s, s.length()));
    }
    else
        *rval = ToJS(cx, 0);

    return JS_TRUE;
}
