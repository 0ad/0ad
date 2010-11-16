#include "precompiled.h"

/*
 * wxJavaScript - textline.cpp
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
 * $Id: textline.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../common/main.h"
#include "../common/index.h"
#include "textline.h"
#include "textfile.h"

using namespace wxjs;
using namespace wxjs::io;

/***
 * <file>textline</file>
 * <module>io</module>
 * <class name="wxTextLine" version="0.8.3">
 *  wxTextLine is a helper class. It doesn't exist in wxWidgets,
 *  but wxJS needs it, so you can access lines in @wxTextFile as
 *  an array:
 *  <pre><code class="whjs">
 *   var i;
 *   for(i = 0; i &lt; file.lineCount; i++)
 *   {
 *     file.lines[i].content = ...
 *   }
 *   </code></pre>
 *  or as:
 *  <pre><code class="whjs">
 *   for each(line in textfile.lines)
 *   {
 *      line.content = ...
 *   }</code></pre>
 * </class>
 */
WXJS_INIT_CLASS(TextLine, "wxTextLine", 0)

bool TextLine::Resolve(JSContext *cx, JSObject *obj, jsval id)
{
    if (    JSVAL_IS_INT(id) 
		 && JSVAL_TO_INT(id) >= 0 )
	{
        return JS_DefineElement(cx, obj, JSVAL_TO_INT(id), INT_TO_JSVAL(0), NULL, NULL, 0) == JS_TRUE;
	}
    return true;
}

/***
 * <properties>
 *  <property name="content" type="String">
 *   Get/Set the content of the line.<br /><br />
 *   <b>Remark: </b>When you access this object as an array of @wxTextFile
 *   you don't need this property. You can write:
 *   <code class="whjs">
 *    textfile.lines[1] = 'Text';</code>
 *  </property>
 *  <property name="lineType" type="@wxTextFile#wxTextFileType" readonly="Y">
 *   Get the type of the line
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(TextLine)
	WXJS_READONLY_PROPERTY(P_LINE_TYPE, "lineType")
	WXJS_PROPERTY(P_CONTENT, "content")
WXJS_END_PROPERTY_MAP()

bool TextLine::GetProperty(Index *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    JSObject *parent = JS_GetParent(cx, obj);
    wxASSERT_MSG(parent != NULL, wxT("No parent found for TextLine"));

    wxTextFile *file = TextFile::GetPrivate(cx, parent);
    if ( file == NULL )
        return false;

    if ( id >= 0 && id < file->GetLineCount() )
    {
        p->SetIndex(id);
		*vp = OBJECT_TO_JSVAL(obj); //ToJS(cx, file->GetLine(id));
    }
	else
	{
		switch(id)
		{
		case P_LINE_TYPE:
			*vp = ToJS(cx, (int) file->GetLineType(p->GetIndex()));
			break;
		case P_CONTENT:
			*vp = ToJS(cx, file->GetLine(p->GetIndex()));
			break;
		default:
			*vp = JSVAL_VOID;
		}
	}

    return true;
}

bool TextLine::SetProperty(Index *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    JSObject *parent = JS_GetParent(cx, obj);
    wxASSERT_MSG(parent != NULL, wxT("No parent found for TextLine"));

    wxTextFile *file = TextFile::GetPrivate(cx, parent);
    if ( file == NULL )
        return false;

    if ( id >= 0 && id < file->GetLineCount() )
    {
        p->SetIndex(id);
		wxString content;
		FromJS(cx, *vp, content);
		file->GetLine(id) = content;
    }
	else
	{
		if ( id == P_CONTENT )
		{
			wxString content;
			FromJS(cx, *vp, content);
			file->GetLine(p->GetIndex()) = content;
		}
	}
    return true;
}

bool TextLine::Enumerate(Index *p, JSContext *cx, JSObject *obj, JSIterateOp enum_op, jsval *statep, jsid *idp)
{
	JSObject *parent = JS_GetParent(cx, obj);
	if ( parent == NULL )
	{
		*statep = JSVAL_NULL;
        if (idp)
            *idp = INT_TO_JSID(0);
		return true;
	}

	wxTextFile *file = TextFile::GetPrivate(cx, parent);
	if ( file == NULL )
	{
		*statep = JSVAL_NULL;
        if (idp)
            *idp = INT_TO_JSID(0);
		return true;
	}

	bool ok = true;

	switch(enum_op)
	{
	case JSENUMERATE_INIT:
		*statep = ToJS(cx, 0);
		if ( idp )
			*idp = INT_TO_JSID(file->GetLineCount());
		break;
	case JSENUMERATE_NEXT:
		{
			int pos;
			FromJS(cx, *statep, pos);
			if ( pos < file->GetLineCount() )
			{
				JS_ValueToId(cx, ToJS(cx, pos), idp);
				*statep = ToJS(cx, ++pos);
				break;
			}
			// Fall through
		}
	case JSENUMERATE_DESTROY:
		*statep = JSVAL_NULL;
		break;
	default:
		ok = false;
	}
	return ok;
}

WXJS_BEGIN_METHOD_MAP(TextLine)
	WXJS_METHOD("insert", insertLine, 1)
	WXJS_METHOD("remove", removeLine, 0)
WXJS_END_METHOD_MAP()

/***
 * <method name="insert">
 *  <function>
 *   <arg name="Line" type="String">The line to insert (without the end-of-line character(s)).</arg>
 *   <arg name="Type" type="@wxTextFile#wxTextFileType" default="typeDefault" />
 *  </function>
 *  <desc>
 *   Insert a line before this line.
 *  </desc>
 * </method>
 */
JSBool TextLine::insertLine(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	Index *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxTextLine"));

	JSObject *objParent = JS_GetParent(cx, obj);
	if ( objParent == NULL )
		return JS_FALSE;

	wxTextFile *file = TextFile::GetPrivate(cx, objParent);

	wxString line;
	FromJS(cx, argv[0], line);

	if ( argc > 1 )
	{
		int type;
		if ( ! FromJS(cx, argv[1], type) )
			return JS_FALSE;
		file->InsertLine(line, p->GetIndex(), (wxTextFileType) type);
	}
	else
	{
		file->InsertLine(line, p->GetIndex());
	}
	return JS_TRUE;
}

/***
 * <method name="remove">
 *  <function />
 *  <desc>
 *   Removes this line from the file
 *  </desc>
 * </method>
 */
JSBool TextLine::removeLine(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	Index *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxTextLine"));

	JSObject *objParent = JS_GetParent(cx, obj);
	if ( objParent == NULL )
		return JS_FALSE;

	wxTextFile *file = TextFile::GetPrivate(cx, objParent);

	file->RemoveLine(p->GetIndex());
	return JS_TRUE;
}
