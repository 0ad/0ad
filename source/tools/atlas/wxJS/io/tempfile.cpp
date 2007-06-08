#include "precompiled.h"

/*
 * wxJavaScript - tempfile.cpp
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
 * $Id: tempfile.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../common/main.h"
#include "../ext/wxjs_ext.h"
#include "tempfile.h"

using namespace wxjs;
using namespace wxjs::io;

/***
 * <file>tempfile</file>
 * <module>io</module>
 * <class name="wxTempFile" version="0.8.3">
 *  wxTempFile provides a relatively safe way to replace the contents of the existing file. 
 *  The name is explained by the fact that it may be also used as just a temporary file if
 *  you don't replace the old file contents.
 *  <br /><br />
 *  Usually, when a program replaces the contents of some file it first opens it for writing,
 *  thus losing all of the old data and then starts recreating it. This approach is not very safe 
 *  because during the regeneration of the file bad things may happen: the program may find that 
 *  there is an internal error preventing it from completing file generation, the user may interrupt 
 *  it (especially if file generation takes long time) and, finally, any other external interrupts 
 *  (power supply failure or a disk error) will leave you without either the original file or the new one.
 *  <br /><br />
 *  wxTempFile addresses this problem by creating a temporary file which is meant to replace the original file
 *  - but only after it is fully written. So, if the user interrupts the program during the file generation, 
 *  the old file won't be lost. Also, if the program discovers itself that it doesn't want to replace the 
 *  old file there is no problem - in fact, wxTempFile will not replace the old file by default, you should 
 *  explicitly call @wxTempFile#commit to do it. Calling @wxTempFile#discard explicitly discards any modifications: 
 *  it closes and deletes the temporary file and leaves the original file unchanged. If you don't call 
 *  neither of @wxTempFile#commit and @wxTempFile#discard, the destructor will call @wxTempFile#discard
 *  automatically.
 *  <br /><br />
 *  To summarize: if you want to replace another file, create an instance of wxTempFile passing 
 *  the name of the file to be replaced to the constructor (you may also use default constructor 
 *  and pass the file name to @wxTempFile#open). Then you can write to wxTempFile using wxFile-like 
 *  functions and later call @wxTempFile#commit to replace the old file (and close this one)
 *  or call @wxTempFile#discard to cancel the modifications.
 * </class>
 */
WXJS_INIT_CLASS(TempFile, "wxTempFile", 0)

/***
 * <properties>
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
WXJS_BEGIN_PROPERTY_MAP(TempFile)
	WXJS_READONLY_PROPERTY(P_OPENED, "opened")
	WXJS_READONLY_PROPERTY(P_LENGTH, "length")
	WXJS_READONLY_PROPERTY(P_TELL, "tell")
WXJS_END_PROPERTY_MAP()

bool TempFile::GetProperty(wxTempFile *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	switch(id)
	{
	case P_OPENED:
		*vp = ToJS(cx, p->IsOpened());
		break;
	case P_LENGTH:
		*vp = p->IsOpened() ? ToJS(cx, (long) p->Length()) : JSVAL_VOID;
		break;
	case P_TELL:
		*vp = p->IsOpened() ? ToJS(cx, (long) p->Tell()) : JSVAL_VOID;
		break;
	}
	return true;
}

/***
 * <ctor>
 *  <function />
 *  <function>
 *   <arg name="FileName" type="String">The filename</arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxTempFile object. When a filename is passed,
 *   The file is opened, and you can check whether the operation
*    was successful or not by checking @wxTempFile#opened.
 *  </desc>
 * </ctor>
 */
wxTempFile *TempFile::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
	wxString fileName;

	switch(argc)
	{
	case 1:
		FromJS(cx, argv[0], fileName);
		return new wxTempFile(fileName);
		break;
	case 0:
		return new wxTempFile();
		break;
	}
	return NULL;
}

WXJS_BEGIN_METHOD_MAP(TempFile)
	WXJS_METHOD("open", open, 2)
	WXJS_METHOD("seek", seek, 2)
	WXJS_METHOD("write", write, 1)
	WXJS_METHOD("commit", commit, 0)
	WXJS_METHOD("discard", discard, 0)
WXJS_END_METHOD_MAP()

/***
 * <method name="commit">
 *  <function returns="Boolean" />
 *  <desc>
 *   Validate changes: deletes the old file and renames the new file to the old name. 
 *   Returns true if both actions succeeded. If false is returned it may unfortunately 
 *   mean two quite different things: either that either the old file couldn't be deleted or 
 *   that the new file couldn't be renamed to the old name.
 *  </desc>
 * </method>
 */
JSBool TempFile::commit(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxTempFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxTempFile"));

	*rval = ToJS(cx, p->Commit());
	return JS_TRUE;
}

/***
 * <method name="discard">
 *  <function />
 *  <desc> 
 *   Discard changes: the old file contents is not changed, temporary file is deleted.
 *  </desc>
 * </method>
 */
JSBool TempFile::discard(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxTempFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxTempFile"));

	p->Discard();

	return JS_TRUE;
}

/***
 * <method name="open">
 *  <function returns="Boolean">
 *   <arg name="FileName" type="String">The name of the file</arg>
 *  </function>
 *  <desc>
 *   Open the temporary file, returns true on success, false if an error occurred.
 *   <i>FileName</i> is the name of file to be replaced. The temporary file is always created 
 *   in the directory where <i>FileName</i> is. In particular, if <i>FileName</i> doesn't include
 *   the path, it is created in the current directory and the program should have write access
 *   to it for the function to succeed. 
 *  </desc>
 * </method>
 */
JSBool TempFile::open(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxTempFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxTempFile"));

	wxString fileName;
	FromJS(cx, argv[0], fileName);

	*rval = ToJS(cx, p->Open(fileName));
	
	return JS_TRUE;
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
JSBool TempFile::seek(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxTempFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxTempFile"));

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
 * <method name="write">
 *  <function returns="Integer">
 *   <arg name="Buffer" type="@wxMemoryBuffer" />
 *  </function>
 *  <function returns="Integer">
 *   <arg name="Str" type="String" />
 *  </function>
 *  <desc>
 *   Writes the string or buffer to the file. 
 *   Returns the actual number of bytes written to the file.
 *  </desc>
 * </method>
 */
JSBool TempFile::write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxTempFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxTempFile"));

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

	wxString content;
	FromJS(cx, argv[0], content);
	int count = content.length(); 
	*rval = ToJS(cx, (int) p->Write(content.c_str(), count));

	return JS_TRUE;
}
