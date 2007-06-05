#include "precompiled.h"

/*
 * wxJavaScript - aistream.cpp
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
 * $Id: aistream.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/archive.h>

#include "../common/main.h"
#include "stream.h"
#include "aistream.h"
#include "archentry.h"

using namespace wxjs;
using namespace wxjs::io;

/***
 * <file>aistream</file>
 * <module>io</module>
 * <class name="wxArchiveInputStream" prototype="wxInputStream" version="0.8.3">
 *  wxArchiveInputStream is a prototype object for archive input streams such
 *  as @wxZipInputStream.
 * </class>
 */

WXJS_INIT_CLASS(ArchiveInputStream, "wxArchiveInputStream", 0)

WXJS_BEGIN_METHOD_MAP(ArchiveInputStream)
	WXJS_METHOD("closeEntry", closeentry, 0)
	WXJS_METHOD("openEntry", openEntry, 1)
WXJS_END_METHOD_MAP()

/***
 * <method name="closeEntry">
 *  <function returns="Boolean" />
 *  <desc>
 *   Closes the current entry. On a non-seekable stream reads to the end of the current entry first.
 *  </desc>
 * </method>
 */
JSBool ArchiveInputStream::closeentry(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    Stream *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

	*rval = ToJS(cx, ((wxArchiveInputStream *)p->GetStream())->CloseEntry());
	return JS_TRUE;
}

/***
 * <method name="openEntry">
 *  <function returns="Boolean">
 *   <arg name="Entry" type="@wxZipEntry" />
 *  </function>
 *  <desc>
 *   Closes the current entry if one is open, then opens the entry specified by the entry object.
 *   entry should be from the same zip file, and the zip should be on a seekable stream.
 *  </desc>
 * </method>
 */
JSBool ArchiveInputStream::openEntry(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	Stream *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

	wxArchiveInputStream *ais = dynamic_cast<wxArchiveInputStream *>(p->GetStream());

	wxArchiveEntry *entry = ArchiveEntry::GetPrivate(cx, argv[0]);
	if ( entry != NULL )
	{
		*rval = ToJS(cx, ais->OpenEntry(*entry));
		return JS_TRUE;
	}

	return JS_FALSE;
}
