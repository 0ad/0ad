#include "precompiled.h"

/*
 * wxJavaScript - jsmembuf.cpp
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
 * $Id: jsmembuf.cpp 737 2007-06-08 18:12:16Z fbraem $
 */
// jsmembuf.cpp
#include "../common/main.h"
#include "jsmembuf.h"
#include <wx/string.h>

using namespace wxjs;
using namespace ext;

/***
 * <file>memorybuffer</file>
 * <module>ext</module>
 * <class name="wxMemoryBuffer">
 *  A wxMemoryBuffer is a useful data structure for storing arbitrary sized blocks of memory.
 *  <br />
 *  <br />
 *  You can access the data of the buffer as a JavaScript array.
 *  For example:<br />
 *  <pre><code class="whjs">
 *   var buffer = new wxMemoryBuffer(10);
 *   buffer[0] = 10;
 *   buffer[1] = 'a';
 *  </code></pre>
 * </class>
 */
WXJS_INIT_CLASS(MemoryBuffer, "wxMemoryBuffer", 0)

/***
 * <properties>
 *  <property name="isNull" type="Boolean" readonly="Y">
 *   Is the buffer null? (dataLen and bufSize are 0).
 *  </property>
 *  <property name="dataLen" type="Integer">
 *   Get/Set the length of the data in the buffer. The length of the data
 *   can be less then the length of the buffer.
 *  </property>
 *  <property name="bufSize" type="Integer">
 *   Get/Set the size of the buffer.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(MemoryBuffer)
	WXJS_PROPERTY(P_DATA_LENGTH, "dataLen")
	WXJS_PROPERTY(P_LENGTH, "bufSize")
	WXJS_READONLY_PROPERTY(P_IS_NULL, "isNull")
WXJS_END_PROPERTY_MAP()

bool MemoryBuffer::GetProperty(wxMemoryBuffer *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	if ( id > -1 )
	{
		if ( (unsigned int) id < p->GetDataLen() ) 
		{
			unsigned int *data = (unsigned int*) p->GetData();
			*vp = ToJS(cx, (int) data[id]);
		}
	}
	else
	{
		switch(id)
		{
		case P_DATA_LENGTH:
			*vp = ToJS(cx, p->GetDataLen());
			break;
		case P_LENGTH:
			*vp = ToJS(cx, p->GetBufSize());
			break;
		case P_IS_NULL:
			*vp = ToJS(cx, p->GetDataLen() == 0 && p->GetBufSize() == 0);
			break;
		}
	}
	return true;
}

bool MemoryBuffer::SetProperty(wxMemoryBuffer *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	if ( id > -1 )
	{
		if ( (unsigned int) id < p->GetDataLen() )
		{
			if ( JSVAL_IS_STRING(*vp) )
			{
				wxString str;
				FromJS(cx, *vp, str);
				if ( str.Length() > 0 )
				{
					char *bufdata = (char *) p->GetData();
					bufdata[id] = str[0];
				}
			}
			else
			{
				int data;
				if ( FromJS(cx, *vp, data) )
				{
					char *bufdata = (char *) p->GetData();
					bufdata[id] = data;
				}
			}
		}
	}
	else
	{
		switch(id)
		{
		case P_DATA_LENGTH:
			{
				int length = 0;
				if ( FromJS(cx, *vp, length) )
					p->SetDataLen(length);
				break;
			}
		case P_LENGTH:
			{
				int dlength = 0;
				if ( FromJS(cx, *vp, dlength) )
					p->SetBufSize(dlength);
				break;
			}
		}
	}
	return true;
}

/***
 * <ctor>
 *  <function>
 *   <arg name="Size" type="Integer" default="0">The size of the buffer</arg>
 *  </function>
 *  <function>
 *   <arg name="Str" type="String">A string to fill the buffer</arg>
 *   <arg name="Encoding" type="String" default="UTF-16">The encoding to use to put this string in the buffer</arg>
 *  </function>
 *  <desc>
 *   Creates a new wxMemoryBuffer object with the given size or with
 *   string as content.
 *  </desc>
 * </ctor>
 */
wxMemoryBuffer *MemoryBuffer::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    if ( argc == 0 )
        return new wxMemoryBuffer();

	if (    argc == 1 
         && JSVAL_IS_INT(argv[0]) )
	{
		int size = 0;
		if (    FromJS(cx, argv[0], size)
		     && size > 0 )
		{
			return new wxMemoryBuffer(size);
		}
	}

    wxString encoding(wxJS_INTERNAL_ENCODING);
    if ( argc > 1 )
    {
        FromJS(cx, argv[1], encoding);
    }
    wxString data;
	FromJS(cx, argv[0], data);

    wxCharBuffer content;
    if ( encoding.CmpNoCase(wxJS_INTERNAL_ENCODING) == 0 )
    {
        content = data.mb_str();
    }
    else
    {
        wxCSConv conv(encoding);
        content = data.mb_str(conv);
    }

    wxMemoryBuffer *buffer = new wxMemoryBuffer();
    buffer->AppendData(content, strlen(content));

    return buffer;
}

WXJS_BEGIN_METHOD_MAP(MemoryBuffer)
	WXJS_METHOD("append", append, 1)
	WXJS_METHOD("toString", toString, 0)
WXJS_END_METHOD_MAP()

/***
 * <method name="append">
 *  <function>
 *   <arg name="Byte" type="integer">The byte to add</arg>
 *  </function>
 *  <function>
 *   <arg name="Buffer" type="wxMemoryBuffer">The buffer to add</arg>
 *   <arg name="Size" type="Integer" default="Buffer.size">The size of the buffer to add. When not set, the full buffer is added.</arg>
 *  </function>
 *  <desc>
 *   Concatenate a byte or buffer to this buffer.
 *  </desc>
 * </method>
 */
JSBool MemoryBuffer::append(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxMemoryBuffer *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	if ( JSVAL_IS_INT(argv[0]) )
	{
		int byte;
		FromJS(cx, argv[0], byte);
		p->AppendByte((char) byte);
		return JS_TRUE;
	}

	if ( JSVAL_IS_OBJECT(argv[0]) )
	{
		wxMemoryBuffer *buffer = GetPrivate(cx, argv[0], false);
		if ( buffer != NULL )
		{
			if ( argc > 1 )
			{
				int size;
				if ( FromJS(cx, argv[1], size) )
				{
					if ( size > (int) buffer->GetDataLen() )
						size = buffer->GetDataLen();
					p->AppendData(buffer->GetData(), size);
				}
				else
				{
					return JS_FALSE;
				}
			}
			else
			{
				p->AppendData(buffer->GetData(), buffer->GetDataLen());
				return JS_TRUE;
			}
		}
	}

    wxString encoding(wxJS_INTERNAL_ENCODING);
    if ( argc > 1 )
    {
        FromJS(cx, argv[1], encoding);
    }

    wxString data;
	FromJS(cx, argv[0], data);

    wxCharBuffer content;
    if ( encoding.CmpNoCase(wxJS_INTERNAL_ENCODING) == 0 )
    {
        content = data.mb_str();
    }
    else
    {
        wxCSConv conv(encoding);
        content = data.mb_str(conv);
    }

    p->AppendData(content, strlen(content));
	
    return JS_TRUE;
}

/***
 * <method name="toString">
 *  <function returns="String">
 *   <arg name="Encoding" type="String" default="UTF-16">
 *    The encoding of the string in this buffer.
 *   </arg>
 *  </function>
 *  <desc>
 *   Converts the content in the buffer to a String. 
 *   The default encoding is UTF-16 because in JavaScript all strings
 *   are stored in UTF-16. A conversion is done to UTF-16,
 *   when another encoding is specified.
 *  </desc>
 * </method>
 */
JSBool MemoryBuffer::toString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxMemoryBuffer *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

    wxString encoding(wxJS_INTERNAL_ENCODING);
    if ( argc > 0 )
        FromJS(cx, argv[0], encoding);
    
    wxCSConv conv(encoding);
    wxString content((const char*) p->GetData(), conv, p->GetDataLen());
    *rval = ToJS(cx, content);

	return JS_TRUE;
}
