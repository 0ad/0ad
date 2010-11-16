#include "precompiled.h"

/*
 * wxJavaScript - file.cpp
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
 * $Id: file.cpp 810 2007-07-13 20:07:05Z fbraem $
 */
// file.cpp
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../common/main.h"
#include "../ext/wxjs_ext.h"
#include "../ext/jsmembuf.h"
#include "file.h"

using namespace wxjs;
using namespace wxjs::io;

/***
 * <file>file</file>
 * <module>io</module>
 * <class name="wxFile">
 *	 A wxFile performs raw file I/O. 
 *   This is a very small class designed to minimize the overhead of using it - 
 *   in fact, there is hardly any overhead at all, but using it brings you 
 *   automatic error checking and hides differences between platforms.
 *   wxFile is a wrapper around a file descriptor, while @wxFFile is a wrapper
 *   around the FILE structure.
 * </class>
 */
WXJS_INIT_CLASS(File, "wxFile", 0)

/***
 * <constants>
 *  <type name="wxFileKind">
 *   <constant name="KIND_UNKNOWN" />
 *   <constant name="KIND_DISK" />
 *   <constant name="KIND_TERMINAL" />
 *   <constant name="KIND_PIPE" />
 *  </type>
 *  <type name="Mode">
 *   <constant name="read" />
 *   <constant name="write" />
 *   <constant name="read_write" />
 *   <constant name="write_append" />
 *   <constant name="write_excl" />
 *  </type>
 *  <type name="FileDescriptor">
 *   <constant name="fd_stdin" />
 *   <constant name="fd_stdout" />
 *   <constant name="fd_stderr" />
 *  </type>
 *  <type name="wxSeekMode">
 *   <constant name="FromCurrent" />
 *   <constant name="FromStart" />
 *   <constant name="FromEnd" />
 *   <desc>
 *    wxSeekMode is ported as a separate JavaScript object.
 *   </desc>
 *  </type>
 *  <type name="FileAccess">
 *   <constant name="IRUSR" />
 *   <constant name="IWUSR" />
 *   <constant name="IXUSR" />
 *   <constant name="IRGRP" />
 *   <constant name="IWGRP" />
 *   <constant name="IXGRP" />
 *   <constant name="IROTH" />
 *   <constant name="IWOTH" />
 *   <constant name="IXOTH" />
 *   <constant name="DEFAULT" />
 *  </type>
 * </constants>
 */
WXJS_BEGIN_CONSTANT_MAP(File)
    WXJS_CONSTANT(wxFILE_, KIND_UNKNOWN)
    WXJS_CONSTANT(wxFILE_, KIND_DISK)
    WXJS_CONSTANT(wxFILE_, KIND_TERMINAL)
    WXJS_CONSTANT(wxFILE_, KIND_PIPE)
    { wxFile::read, "read", WXJS_READONLY },
    { wxFile::write, "write", WXJS_READONLY },
    { wxFile::read_write, "read_write", WXJS_READONLY },
    { wxFile::write_append, "write_append", WXJS_READONLY },
    { wxFile::write_excl, "write_excl", WXJS_READONLY },
    { wxFile::fd_stdin, "fd_stdin", WXJS_READONLY },
    { wxFile::fd_stdout, "fd_stdout", WXJS_READONLY },
    { wxFile::fd_stderr, "fd_stderr", WXJS_READONLY },
	WXJS_CONSTANT(wxS_, IRUSR)
	WXJS_CONSTANT(wxS_, IWUSR)
	WXJS_CONSTANT(wxS_, IXUSR)
	WXJS_CONSTANT(wxS_, IRGRP)
	WXJS_CONSTANT(wxS_, IWGRP)
	WXJS_CONSTANT(wxS_, IXGRP)
	WXJS_CONSTANT(wxS_, IROTH)
	WXJS_CONSTANT(wxS_, IWOTH)
	WXJS_CONSTANT(wxS_, IXOTH)
	WXJS_CONSTANT(wxS_, DEFAULT)
WXJS_END_CONSTANT_MAP()

void File::InitClass(JSContext *cx, JSObject *obj, JSObject *proto)
{
    JSConstDoubleSpec wxPathFormatMap[] = 
    {
		WXJS_CONSTANT(wx, FromCurrent)
		WXJS_CONSTANT(wx, FromStart)
		WXJS_CONSTANT(wx, FromEnd)
	    { 0 }
    };

    JSObject *constObj = JS_DefineObject(cx, obj, "wxSeekMode", 
									 	 NULL, NULL,
							             JSPROP_READONLY | JSPROP_PERMANENT);
    JS_DefineConstDoubles(cx, constObj, wxPathFormatMap);
}

/***
 * <properties>
 *	<property name="eof" type="Boolean" readonly="Y">
 *	 Returns true when end of file is reached. 
 *   When the file is not open, undefined is returned.
 *  </property>
 *	<property name="opened" type="Boolean" readonly="Y">
 *	 Returns true when the file is open.
 *  </property>
 *	<property name="length" type="Integer" readonly="Y">
 *	 Returns the length of the file.
 *   When the file is not open, undefined is returned.
 *  </property>
 *	<property name="tell" type="Integer" readonly="Y">
 *	 Returns the current position or -1.
 *   When the file is not open, undefined is returned.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(File)
	WXJS_READONLY_PROPERTY(P_EOF, "eof")
	WXJS_READONLY_PROPERTY(P_OPENED, "opened")
	WXJS_READONLY_PROPERTY(P_LENGTH, "length")
	WXJS_READONLY_PROPERTY(P_TELL, "tell")
	WXJS_READONLY_PROPERTY(P_KIND, "kind")
WXJS_END_PROPERTY_MAP()

bool File::GetProperty(wxFile *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	switch(id)
	{
	case P_EOF:
		*vp = p->IsOpened() ? ToJS(cx, p->Eof()) : JSVAL_VOID;
		break;
	case P_OPENED:
		*vp = ToJS(cx, p->IsOpened());
		break;
	case P_LENGTH:
		*vp = p->IsOpened() ? ToJS(cx, (long) p->Length()) : JSVAL_VOID;
		break;
	case P_TELL:
		*vp = p->IsOpened() ? ToJS(cx, (long) p->Tell()) : JSVAL_VOID;
		break;
	case P_KIND:
		*vp = p->IsOpened() ? ToJS(cx, (int) p->GetKind()) : JSVAL_VOID;
		break;
	}
	return true;
}

/***
 * <ctor>
 *  <function />
 *  <function>
 *   <arg name="FileName" type="String">The filename</arg>
 *   <arg name="Mode" type="OpenMode" default="wxFile.read">
 *    The mode in which to open the file. See @wxFile#Mode.
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxFile object. When a filename is passed,
 *   The file is opened, and you can check whether the operation
*    was successful or not by checking @wxFile#opened.
 *  </desc>
 * </ctor>
 */
wxFile *File::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
	if ( argc > 2 )
        argc = 2;

	wxString fileName;

	int mode = wxFile::read;

	switch(argc)
	{
	case 2:
		if ( ! FromJS(cx, argv[1], mode) )
			break;
		// Walk through
	case 1:
		FromJS(cx, argv[0], fileName);
		return new wxFile(fileName, (wxFile::OpenMode) mode);
		break;
	case 0:
		return new wxFile();
		break;
	}
	return NULL;
}

WXJS_BEGIN_METHOD_MAP(File)
	WXJS_METHOD("attach", attach, 1)
	WXJS_METHOD("detach", detach, 0)
	WXJS_METHOD("close", close, 0)
	WXJS_METHOD("create", create, 3)
	WXJS_METHOD("flush", flush, 0)
	WXJS_METHOD("open", open, 2)
	WXJS_METHOD("read", read, 1)
	WXJS_METHOD("seek", seek, 2)
	WXJS_METHOD("seekEnd", seekEnd, 1)
	WXJS_METHOD("write", write, 1)
WXJS_END_METHOD_MAP()

/***
 * <method name="attach">
 *  <function>
 *   <arg name="Fd" type="Integer">
 *    A file descriptor
 *   </arg>
 *  </function>
 *  <desc>
 *   Attaches an existing file descriptor to the wxFile object. 
 *   Example of predefined file descriptors are 0, 1 and 2 which correspond to 
 *   stdin, stdout and stderr (and have symbolic names of wxFile.fd_stdin, wxFile.fd_stdout
 *   and wxFile.fd_stderr).
 *  </desc>
 * </method>
 */
JSBool File::attach(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxFile"));

	if ( ! p->IsOpened() )
	{
		return JS_FALSE;
	}

	int fd;
	if ( FromJS(cx, argv[0], fd) )
	{
		p->Attach(fd);
		return JS_TRUE;
	}

	return JS_FALSE;
}

/***
 * <method name="close">
 *  <function />
 *  <desc> 
 *   Closes the file.
 *  </desc>
 * </method>
 */
JSBool File::close(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxFile"));

	if ( p->IsOpened() )
		p->Close();

	return JS_TRUE;
}

/***
 * <method name="detach">
 *  <function />
 *  <desc>  
 *   Get back a file descriptor from wxFile object - the caller is responsible for closing
 *   the file if this descriptor is opened. @wxFile#opened will return false
 *   after call to detach.
 *  </desc>
 * </method>
 */
JSBool File::detach(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxFile"));

	if ( ! p->IsOpened() )
	{
		return JS_FALSE;
	}

    p->Detach();

	return JS_TRUE;
}

/***
 * <method name="create">
 *  <function returns="Boolean">
 *   <arg name="FileName" type="String">The name of the file to create.</arg>
 *   <arg name="Overwrite" type="Boolean" default="false">
 *    Overwrite the file when it already exist? The default is false.
 *   </arg>
 *   <arg name="Access" type="Integer" default="wxFile.DEFAULT" />
 *  </function>
 *  <desc>
 *   Creates a file for writing.
 *  </desc>
 * </method>
 */
JSBool File::create(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxFile"));

	wxString fileName;
	bool overwrite = false;
	int access = wxS_DEFAULT;

	FromJS(cx, argv[0], fileName);
	if (   argc > 1 
		&& ! FromJS(cx, argv[1], overwrite) )
	{
		return JS_FALSE;
	}

	if (    argc > 2 
		 && ! FromJS(cx, argv[2], access) ) 
	{
		return JS_FALSE;
	}

	*rval = ToJS(cx, p->Create(fileName, overwrite, access));

	return JS_TRUE;
}

/***
 * <method name="flush">
 *  <function />
 *  <desc>
 *   Flushes the file.
 *  </desc>
 * </method>
 */
JSBool File::flush(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxFile"));

	if ( ! p->IsOpened() )
	{
		return JS_FALSE;
	}

    p->Flush();

	return JS_TRUE;
}

/***
 * <method name="open">
 *  <function returns="Boolean">
 *   <arg name="FileName" type="String">The name of the file to open</arg>
 *   <arg name="Mode" type="Integer" default="wxFile.read"/>
 *  </function>
 *  <desc>
 *   Opens a file.
 *  </desc>
 * </method>
 */
JSBool File::open(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxFile"));

	wxString fileName;
	bool result = false;
	FromJS(cx, argv[0], fileName);

	if ( argc > 1 )
	{
		int mode;
		if ( FromJS(cx, argv[1], mode) )
		{
			result = p->Open(fileName, (wxFile::OpenMode) mode);
		}
		else
		{
			return JS_FALSE;
		}
	}
	else
		result = p->Open(fileName);

	*rval = result ? JSVAL_TRUE : JSVAL_FALSE;
	
	return JS_TRUE;
}

/***
 * <method name="read">
 *  <function returns="@wxMemoryBuffer">
 *   <arg name="Count" type="Integer">The number of bytes to read.</arg>
 *  </function>
 *  <desc>
 *   Reads the specified number of bytes and returns a buffer.
 *  </desc>
 * </method>
 */
JSBool File::read(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxFile"));

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
 * <method name="seek">
 *  <function returns="Integer">
 *   <arg name="Offset" type="Integer">Offset to seek to.</arg>
 *   <arg name="Mode" type="@wxFile#wxSeekMode" default="wxFile.FromStart" />
 *  </function>
 *  <desc>
 *   Seeks the offset. Returns the actual position or -1.
 *   See also @wxFile#seekEnd.
 *  </desc>
 * </method>
 */
JSBool File::seek(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxFile"));

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
			pos = p->Seek(offset, (wxSeekMode) mode);
		}
		else
		{
			return JS_FALSE;
		}
	}
	else
		pos = p->Seek(offset);

	*rval = ToJS(cx, pos);
	return JS_TRUE;
}

/***
 * <method name="seekEnd">
 *  <function>
 *   <arg name="Offset" type="Integer">Offset to seek to.</arg>
 *  </function>
 *  <desc>
 *   Moves the file pointer to the specified number of bytes before the end of the file.
 *   Returns the actual position or -1 on error.
 *   <br /><br /><b>Remark:</b> Is the same as calling seek(Offset, wxFile.FromEnd)
 *  </desc>
 * </method>
 */
JSBool File::seekEnd(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxFile"));

	if ( ! p->IsOpened() )
	{
		return JS_FALSE;
	}

    int offset;
	if ( FromJS(cx, argv[0], offset) )
	{
		*rval = ToJS(cx, (int) p->SeekEnd(offset));
		return JS_TRUE;
	}

	return JS_FALSE;
}

/***
 * <method name="write">
 *  <function returns="Integer">
 *   <arg name="Buffer" type="@wxMemoryBuffer" />
 *  </function>
 *  <function returns="Integer">
 *   <arg name="Str" type="String" />
 *   <arg name="Encoding" type="String" default="UTF-8" />
 *  </function>
 *  <desc>
 *   Writes the string or buffer to the file. A String
 *   is written in the specified encoding. The default is
 *   UTF-8 because most files are still written as UTF-8
 *   or ASCII. Returns the actual number of bytes written to the file.
 *  </desc>
 * </method>
 */
JSBool File::write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxFile *p = GetPrivate(cx, obj);
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
			return JS_TRUE;
		}
	}

    wxString encoding(wxJS_EXTERNAL_ENCODING);
    if ( argc > 1 )
    {
        FromJS(cx, argv[1], encoding);
    }
    wxCSConv conv(encoding);

    wxString content;
	FromJS(cx, argv[0], content);
	*rval = ToJS(cx, (int) p->Write(content, conv));

	return JS_TRUE;
}

WXJS_BEGIN_STATIC_METHOD_MAP(File)
	WXJS_METHOD("access", access, 2)
	WXJS_METHOD("exists", exists, 1)
WXJS_END_METHOD_MAP()

/***
 * <class_method name="access">
 *  <function returns="Boolean">
 *   <arg name="Filename" type="String">Filename to check</arg>
 *   <arg name="Mode" type="@wxFile#Mode">File mode</arg> 
 *  </function> 
 *  <desc>
 *   This function verifies if we may access the given file in specified mode. 
 *   Only values of @wxFile#read or @wxFile#write really make sense here.
 *  </desc>
 * </class_method>
 */
JSBool File::access(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxString filename;
	FromJS(cx, argv[0], filename);

	int mode;
	
	if ( FromJS(cx, argv[1], mode) )
	{
		*rval = ToJS(cx, wxFile::Access(filename, (wxFile::OpenMode) mode));
		return JS_TRUE;
	}
	return JS_FALSE;
}

/***
 * <class_method name="exists">
 *  <function returns="Boolean">
 *   <arg name="Filename" type="String" />
 *  </function>
 *  <desc>
 *   Returns true when the file exists.
 *  </desc>
 * </class_method>
 */
JSBool File::exists(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxString filename;
	FromJS(cx, argv[0], filename);
	*rval = ToJS(cx, wxFile::Exists(filename));
	return JS_TRUE;
}

