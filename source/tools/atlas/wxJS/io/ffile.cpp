#include "precompiled.h"

/*
 * wxJavaScript - ffile.cpp
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
 * $Id: ffile.cpp 810 2007-07-13 20:07:05Z fbraem $
 */
// file.cpp
#include <wx/strconv.h>

#include "../common/main.h"
#include "../common/apiwrap.h"
#include "../common/type.h"
#include "../ext/wxjs_ext.h"
#include "../ext/jsmembuf.h"
#include "ffile.h"

using namespace wxjs;
using namespace wxjs::io;

/***
 * <file>ffile</file>
 * <module>io</module>
 * <class name="wxFFile">
 *	 A wxFFile performs raw file I/O. 
 *   This is a very small class designed to minimize the overhead of using it - 
 *   in fact, there is hardly any overhead at all, but using it brings you 
 *   automatic error checking and hides differences between platforms.
 *   It wraps inside it a FILE * handle used by standard C IO library (also known as stdio).
 *   <br /><br /><b>Remark :</b> 
 *   A FILE* structure can't be used directly in JavaScript. That's why some methods are not
 *   ported.
 * </class>
 */
WXJS_INIT_CLASS(FFile, "wxFFile", 0)

/***
 * <constants>
 *  <type name="wxFileKind">
 *   <constant name="KIND_UNKNOWN" />
 *   <constant name="KIND_DISK" />
 *   <constant name="KIND_TERMINAL" />
 *   <constant name="KIND_PIPE" />
 *  </type>
 *  <type name="wxSeekMode">
 *   <constant name="FromCurrent" />
 *   <constant name="FromStart" />
 *   <constant name="FromEnd" />
 *  </type>
 * </constants>
 */
WXJS_BEGIN_CONSTANT_MAP(FFile)
    WXJS_CONSTANT(wxFILE_, KIND_UNKNOWN)
    WXJS_CONSTANT(wxFILE_, KIND_DISK)
    WXJS_CONSTANT(wxFILE_, KIND_TERMINAL)
    WXJS_CONSTANT(wxFILE_, KIND_PIPE)
	WXJS_CONSTANT(wx, FromCurrent)
	WXJS_CONSTANT(wx, FromStart)
	WXJS_CONSTANT(wx, FromEnd)
WXJS_END_CONSTANT_MAP()

/***
 * <properties>
 *	<property name="eof" type="Boolean" readonly="Y">
 *	 True when end of file is reached. When the file is not open, undefined is returned.
 *  </property>
 *  <property name="kind" type="Integer" readonly="Y">
 *   Returns the type of the file
 *  </property>
 *	<property name="opened" type="Boolean" readonly="Y">
 *	 True when the file is open.
 *  </property>
 *	<property name="length" type="Integer" readonly="Y">
 *	 Returns the length of the file. When the file is not open, undefined is returned.
 *  </property>
 *	<property name="tell" type="Integer" readonly="Y">
 *	 Returns the current position or -1 (invalid offset).
 *   When the file is not open, undefined is returned.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(FFile)
	WXJS_READONLY_PROPERTY(P_EOF, "eof")
	WXJS_READONLY_PROPERTY(P_KIND, "kind")
	WXJS_READONLY_PROPERTY(P_OPENED, "opened")
	WXJS_READONLY_PROPERTY(P_LENGTH, "length")
	WXJS_READONLY_PROPERTY(P_TELL, "tell")
WXJS_END_PROPERTY_MAP()

bool FFile::GetProperty(wxFFile *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	switch(id)
	{
	case P_EOF:
		if ( p->IsOpened() )
			*vp = ToJS(cx, p->Eof());
		else
			*vp = JSVAL_VOID;
		break;
	case P_KIND:
		*vp = ToJS(cx, (int) p->GetKind());
		break;
	case P_OPENED:
		*vp = ToJS(cx, p->IsOpened());
		break;
	case P_LENGTH:
		if ( p->IsOpened() )
			*vp = ToJS(cx, (int) p->Length());
		else
			*vp = JSVAL_VOID;
		break;
	case P_TELL:
		if ( p->IsOpened() )
			*vp = ToJS(cx, (int) p->Tell());
		else
			*vp = JSVAL_VOID;
		break;
	}
	return true;
}

/***
 * <ctor>
 *  <function>
 *   <arg name="name" type="String">Filename</arg>
 *   <arg name="mode" type="String" default="r">
 *    The mode in which to open the file
 *   </arg>
 *  </function>
 *  <desc>
 *   Creates a new wxFFile object
 *  </desc>
 * </ctor>
 */
wxFFile *FFile::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
	if ( argc > 2 )
	    argc = 2;

    if ( argc == 0 )
        return new wxFFile();

	wxString fileName;
    wxString mode = wxT("r"); 
    if ( argc == 2 )
        FromJS(cx, argv[1], mode);

	FromJS(cx, argv[0], fileName);

    return new wxFFile(fileName, mode);
}

WXJS_BEGIN_METHOD_MAP(FFile)
	WXJS_METHOD("close", close, 0)
	WXJS_METHOD("flush", flush, 0)
	WXJS_METHOD("open", open, 2)
	WXJS_METHOD("read", read, 1)
	WXJS_METHOD("readAll", readAll, 0)
	WXJS_METHOD("seek", seek, 2)
	WXJS_METHOD("seekEnd", seekEnd, 1)
	WXJS_METHOD("write", write, 1)
WXJS_END_METHOD_MAP()

/***
 * <method name="close">
 *  <function returns="Boolean" />
 *  <desc>
 *   Closes the file.
 *  </desc>
 * </method>
 */
JSBool FFile::close(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxFFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxFFile"));

	if ( p->IsOpened() )
		*rval = ToJS(cx, p->Close());
	else
		*rval = JSVAL_FALSE;

	return JS_TRUE;
}

/***
 * <method name="flush">
 *  <function returns="Boolean" />
 *  <desc>
 *   Flushes the file.
 *  </desc>
 * </method>
 */
JSBool FFile::flush(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxFFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxFFile"));

	if ( p->IsOpened() )
		*rval = ToJS(cx, p->Flush());
	else
		*rval = JSVAL_FALSE;

	return JS_TRUE;
}

/***
 * <method name="open">
 *  <function returns="Boolean">
 *   <arg name="Filename" type="String">Name of the file</arg>
 *   <arg name="Mode" type="String" default="r">The mode in which to open the file</arg>
 *  </function>
 *  <desc>
 *   Opens a file.
 *  </desc>
 * </method>
 */
JSBool FFile::open(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxFFile *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

	wxString fileName;
    wxString mode = wxT("r"); 
    if ( argc == 2 )
        FromJS(cx, argv[1], mode);

    FromJS(cx, argv[0], fileName);

    *rval = ToJS(cx, p->Open(fileName, mode));
	return JS_TRUE;
}

/***
 * <method name="read">
 *  <function returns="@wxMemoryBuffer">
 *   <arg name="Count" type="Integer">The number of bytes to read.</arg>
 *  </function>
 *  <desc>
 *   Reads the specified number of bytes.
 *  </desc>
 * </method>
 */
JSBool FFile::read(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxFFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxFFile"));

	if ( ! p->IsOpened() )
	{
		return JS_FALSE;
	}
	int count;
	if (    FromJS(cx, argv[0], count) 
		 && count > 0 )
	{
		unsigned char *buffer = new unsigned char[count];
		int readCount = p->Read(buffer, count);
		if ( readCount == wxInvalidOffset )
		{
			*rval = JSVAL_VOID;
		}
		else
		{
            *rval = wxjs::ext::CreateMemoryBuffer(cx, buffer, count);
		}
		delete[] buffer;


		return JS_TRUE;
	}

    return JS_FALSE;
}

/***
 * <method name="readAll">
 *  <function returns="String">
 *   <arg name="encoding" type="String" default="UTF-8">
 *    The encoding of the file.
 *   </arg>
 *  </function>
 *  <desc>
 *   Reads all the data and return it in a String. Internally JavaScript
 *   stores the Strings as UTF-16. But because most files are stored
 *   in UTF-8 or ASCII, the default of the encoding is UTF-8. 
 *  </desc>
 * </method>
 */
JSBool FFile::readAll(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxFFile *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    if ( ! p->IsOpened() )
	{
		return JS_FALSE;
	}

    wxString encoding(wxJS_EXTERNAL_ENCODING);
    if ( argc > 0 )
    {
        FromJS(cx, argv[0], encoding);
    }
    wxCSConv conv(encoding);

    wxString data;
    if ( p->ReadAll(&data, conv) )
    {
        *rval = ToJS(cx, data);
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="seek">
 *  <function returns="Boolean">
 *   <arg name="Offset" type="Integer">Offset to seek to</arg>
 *   <arg name="Mode" type="@wxFFile#wxSeekMode" default="wxFFile.FromStart" />
 *  </function>
 *  <desc>
 *   Seeks the offset. Returns the actual position or -1 on error.
 *  </desc>
 * </method>
 */
JSBool FFile::seek(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxFFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxFFile"));

	if ( ! p->IsOpened() )
	{
		return JS_FALSE;
	}
    
    int offset;
	if ( ! FromJS(cx, argv[0], offset) )
	{
		return JS_FALSE;
	}

	int pos;

	if ( argc > 1 )
	{
		int mode;
		if ( FromJS(cx, argv[1], mode) )
		{
			pos = p->Seek((wxFileOffset) offset, (wxSeekMode) mode);
		}
		else
		{
			return JS_FALSE;
		}
	}
	else
	{
		pos = p->Seek((wxFileOffset) offset);
	}
	*rval = ToJS(cx, pos);
	return JS_TRUE;
}

/***
 * <method name="seekEnd">
 *  <function returns="Boolean">
 *   <arg name="Offset" type="Integer">Offset to seek to.</arg>
 *  </function>
 *  <desc>
 *   Moves the file pointer to the specified number of bytes before the end of the file.
 *   Returns true on success.
 *  </desc>
 * </method>
 */
JSBool FFile::seekEnd(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxFFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxFFile"));

	if ( ! p->IsOpened() )
	{
		return JS_FALSE;
	}

    int offset;
	if ( FromJS(cx, argv[0], offset) )
	{
		*rval = ToJS(cx, p->SeekEnd((wxFileOffset) offset));
		return JS_TRUE;
	}

	return JS_FALSE;
}

/***
 * <method name="write">
 *  <function returns="Integer">
 *   <arg name="Buffer" type="@wxMemoryBuffer" />
 *  </function>
 *  <function returns="Boolean">
 *   <arg name="Str" type="String" />
 *   <arg name="Encoding" type="String" default="UTF-8" />
 *  </function>
 *  <desc>
 *   Writes the string or buffer to the file. When you write
 *   a String you can specify an encoding. Because most files
 *   are still written as UTF-8 or ASCII, UTF-8 is the default
 *   (wxJS uses UTF-16 internally).
 *   Returns the actual number of bytes written to the file when
 *   a buffer is used. Otherwise a boolean indicating
 *   success or failure is returned.
 *  </desc>
 * </method>
 */
JSBool FFile::write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxFFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxFFile"));

	if ( ! p->IsOpened() )
	{
		return JS_FALSE;
	}

	if ( JSVAL_IS_OBJECT(argv[0]) )
	{
        wxMemoryBuffer* buffer = wxjs::ext::GetMemoryBuffer(cx, JSVAL_TO_OBJECT(argv[0]));
		if ( buffer != NULL )
		{
			*rval = ToJS(cx, p->Write(buffer->GetData(), buffer->GetDataLen()));
		}
	}
	else
	{
        wxString encoding(wxJS_EXTERNAL_ENCODING);
        if ( argc > 1 )
        {
            FromJS(cx, argv[1], encoding);
        }
        wxCSConv conv(encoding);
		wxString content;
		FromJS(cx, argv[0], content);
		*rval = ToJS(cx, (int) p->Write(content, conv));
	}
	return JS_TRUE;
}
