#include "precompiled.h"

/*
 * wxJavaScript - aostream.cpp
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
 * $Id: aostream.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/archive.h>

#include "../common/main.h"
#include "stream.h"
#include "aostream.h"
#include "aistream.h"
#include "archentry.h"

using namespace wxjs;
using namespace wxjs::io;

/***
 * <file>aostream</file>
 * <module>io</module>
 * <class name="wxArchiveOutputStream" prototype="wxOutputStream" version="0.8.3">
 *  wxArchiveOutputStream is a prototype object for archive output streams such
 *  as @wxZipOutputStream.
 * </class>
 */

WXJS_INIT_CLASS(ArchiveOutputStream, "wxArchiveOutputStream", 0)

WXJS_BEGIN_METHOD_MAP(ArchiveOutputStream)
	WXJS_METHOD("closeEntry", closeentry, 0)
	WXJS_METHOD("copyEntry", copyEntry, 2)
	WXJS_METHOD("copyArchiveMetaData", copyArchiveMetaData, 1)
	WXJS_METHOD("putNextEntry", putNextEntry, 1)
	WXJS_METHOD("putNextDirEntry", putNextDirEntry, 1)
WXJS_END_METHOD_MAP()

/***
 * <method name="closeEntry">
 *  <function returns="Boolean" />
 *  <desc>
 *   Closes the current entry. On a non-seekable stream reads to the end of the current entry first.
 *  </desc>
 * </method>
 */
JSBool ArchiveOutputStream::closeentry(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    Stream *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

	*rval = ToJS(cx, ((wxArchiveOutputStream *)p->GetStream())->CloseEntry());
	return JS_TRUE;
}

/***
 * <method name="copyArchiveMetaData">
 *  <function returns="Boolean">
 *   <arg name="Zip" type="@wxArchiveInputStream" />
 *  </function>
 *  <desc>
 *   Transfers the zip comment from the @wxArchiveInputStream to this output stream.
 *  </desc>
 * </method>
 */
JSBool ArchiveOutputStream::copyArchiveMetaData(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	Stream *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

	wxArchiveOutputStream *aos = dynamic_cast<wxArchiveOutputStream *>(p->GetStream());

	Stream *in = ArchiveInputStream::GetPrivate(cx, obj);
	if ( in != NULL )
	{
		*rval = ToJS(cx, aos->CopyArchiveMetaData(*dynamic_cast<wxArchiveInputStream*>(in->GetStream())));
		return JS_TRUE;
	}

	return JS_FALSE;
}

/***
 * <method name="copyEntry">
 *  <function returns="Boolean">
 *   <arg name="Entry" type="@wxArchiveEntry" />
 *   <arg name="Input" type="@wxArchiveInputStream" />
 *  </function>
 *  <desc>
 *   Takes ownership of entry and uses it to create a new entry in the zip. 
 *   Entry is then opened in inputStream and its contents copied to this stream.
 *   copyEntry() is much more efficient than transferring the data using read() 
 *   and write() since it will copy them without decompressing and recompressing them.
 *   Creates a new entry in the archive.
 *  </desc>
 * </method>
 */
JSBool ArchiveOutputStream::copyEntry(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	Stream *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

	wxArchiveOutputStream *aos = dynamic_cast<wxArchiveOutputStream *>(p->GetStream());

	wxArchiveEntry *entry = ArchiveEntry::GetPrivate(cx, argv[0]);
	if ( entry != NULL )
	{
		Stream *str = ArchiveInputStream::GetPrivate(cx, argv[1]);
		if ( str != NULL )
		{
			*rval = ToJS(cx, aos->CopyEntry(entry, *dynamic_cast<wxArchiveInputStream*>(str->GetStream())));
			return JS_TRUE;
		}
	}

	return JS_FALSE;
}

/***
 * <method name="putNextDirEntry">
 *  <function returns="Boolean">
 *   <arg name="Name" type="String" />
 *   <arg name="Date" type="Date" default="Now()" />
 *  </function>
 *  <desc>
 *   Creates a new directory entry in the archive with the given name and timestamp.
 *  </desc>
 * </method>
 */
JSBool ArchiveOutputStream::putNextDirEntry(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	Stream *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

	wxArchiveOutputStream *zos = dynamic_cast<wxArchiveOutputStream *>(p->GetStream());

	wxDateTime dt = wxDateTime::Now();
	switch(argc)
	{
	case 2:
		if ( ! FromJS(cx, argv[1], dt) )
			break;
		// Fall through
	default:
		{
			wxString name;
			FromJS(cx, argv[0], name);
			*rval = ToJS(cx, zos->PutNextDirEntry(name, dt));
			return JS_TRUE;
		}
	}
	
	return JS_FALSE;
}

/***
 * <method name="putNextEntry">
 *  <function returns="Boolean">
 *   <arg name="Entry" type="@wxArchiveEntry" />
 *  </function>
 *  <function returns="Boolean">
 *   <arg name="Name" type="String" />
 *   <arg name="Date" type="Date" default="Now()" />
 *   <arg name="Offset" type="Integer" default="-1" />
 *  </function>
 *  <desc>
 *   Creates a new entry in the archive
 *  </desc>
 * </method>
 */
JSBool ArchiveOutputStream::putNextEntry(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	Stream *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

	wxArchiveOutputStream *aos = dynamic_cast<wxArchiveOutputStream *>(p->GetStream());

	if ( JSVAL_IS_OBJECT(argv[0]) )
	{
		wxArchiveEntry *entry = ArchiveEntry::GetPrivate(cx, argv[0]);
		if ( entry != NULL )
		{
			// Clone the entry, because wxArchiveOutputStream manages the deletion
			// and we don't want to delete our JavaScript object
			*rval = ToJS(cx, aos->PutNextEntry(entry->Clone()));
			return JS_TRUE;
		}
	}
	else
	{
		off_t size = wxInvalidOffset;
		wxDateTime dt = wxDateTime::Now();
		switch(argc)
		{
		case 3:
			if ( ! FromJS(cx, argv[2], size) )
				break;
			// Fall through
		case 2:
			if ( ! FromJS(cx, argv[1], dt) )
				break;
			// Fall through
		default:
			{
				wxString name;
				FromJS(cx, argv[0], name);
				*rval = ToJS(cx, aos->PutNextEntry(name, dt, size));
				return JS_TRUE;
			}
		}
	}
	
	return JS_FALSE;
}
