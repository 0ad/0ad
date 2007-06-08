#include "precompiled.h"

/*
 * wxJavaScript - colourdb.cpp
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
 * $Id: colourdb.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// colourdb.cpp

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "../../common/main.h"

#include "colourdb.h"
#include "colour.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>file/colourdb</file>
 * <module>gui</module>
 * <class name="wxColourDatabase">
 *  wxWindows maintains a database of standard RGB colours for a predefined set 
 *  of named colours (such as "BLACK", "LIGHT GREY"). wxColourDatabase is a singleton
 *  and is instantiated when the application starts: wxTheColourDatabase.
 *  An example:
 *  <code class="whjs">
 *   var colour = wxTheColourDatabase.findColour("RED");</code>
 * </class>
 */
WXJS_INIT_CLASS(ColourDatabase, "wxColourDatabase", 0)

WXJS_BEGIN_METHOD_MAP(ColourDatabase)
    WXJS_METHOD("find", find, 1)
    WXJS_METHOD("findName", findName, 1)
WXJS_END_METHOD_MAP()

/***
 * <method name="find">
 *  <function returns="wxColour">
 *   <arg name="Name" type="String">The name of a colour.</arg>
 *  </function>
 *  <desc>
 *   Returns the colour with the given name. <I>undefined</I> is returned 
 *   when the colour does not exist.
 *  </desc>
 * </method>
 */
JSBool ColourDatabase::find(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxColourDatabase *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxString name;
    FromJS(cx, argv[0], name);

    // Create a copy of the colour, because the colour pointer is owned by wxWindows.
    *rval = Colour::CreateObject(cx, new wxColour(p->Find(name)));
    
    return JS_TRUE;
}

/***
 * <method name="findName">
 *  <function returns="String">
 *   <arg name="Colour" type="@wxColour" />
 *  </function>
 *  <desc>
 *   Returns the name of the colour. <I>undefined</I> is returned when the colour has no name.
 *  </desc>
 * </method>
 */
JSBool ColourDatabase::findName(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxColourDatabase *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxColour *colour = Colour::GetPrivate(cx, argv[0]);
    if ( colour == NULL )
        return JS_FALSE;

    wxString name = p->FindName(*colour);
    *rval = ( name.Length() == 0 ) ? JSVAL_VOID : ToJS(cx, name);

    return JS_TRUE;
}
