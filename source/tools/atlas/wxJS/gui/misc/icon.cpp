#include "precompiled.h"

/*
 * wxJavaScript - icon.cpp
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
 * $Id: icon.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// icon.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "icon.h"
using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>misc/icon</file>
 * <module>gui</module>
 * <class name="wxIcon" prototype="@wxBitmap">
 *  An icon is a small rectangular bitmap usually used for denoting a minimized application. 
 *  It differs from a wxBitmap in always having a mask associated with it for transparent drawing. 
 *  On some platforms, icons and bitmaps are implemented identically,
 *  since there is no real distinction between a wxBitmap with a mask and an icon; 
 *  and there is no specific icon format on some platforms 
 *  (X-based applications usually standardize on XPMs for small bitmaps and icons). 
 *  However, some platforms (such as Windows) make the distinction, so a separate class is provided.
 *  See @wxBitmap, wxFrame @wxFrame#icon property and @wxBitmapType
 * </class>
 */
WXJS_INIT_CLASS(Icon, "wxIcon", 0)

WXJS_BEGIN_METHOD_MAP(Icon)
  WXJS_METHOD("loadFile", loadFile, 2)
WXJS_END_METHOD_MAP()

/***
 * <ctor>
 *  <function />
 *  <function>
 *   <arg name="Name" type="String">
 *    Filename
 *   </arg>
 *   <arg name="Type" type="Integer">
 *    The type of the Icon. Use the bitmap types.
 *   </arg>
 *   <arg name="DesiredWidth" type="Integer" default="-1" />
 *   <arg name="DesiredHeight" type="Integer" default="-1" />
 *  </function>
 *  <desc>
 *   Constructs a new wxIcon object. The first constructor creates
 *   an icon without data. Use @wxIcon#loadFile to load an icon.
 *  </desc>
 * </ctor>
 */
wxIcon* Icon::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    if ( argc == 0 )
        return new wxIcon();

    if ( JSVAL_IS_STRING(argv[0]) )
    {
        if ( argc > 4 )
            argc = 4;

        int desiredWidth = -1;
        int desiredHeight = -1;

        switch(argc)
        {
        case 4:
            if ( ! FromJS(cx, argv[3], desiredHeight) )
                return NULL;
            // Fall through
        case 3:
            if ( ! FromJS(cx, argv[2], desiredWidth) )
                return NULL;
            // Fall through
        default:
            wxString name;
            int type;

            if ( FromJS(cx, argv[1], type) )
            { 
                FromJS(cx, argv[0], name);
                return new wxIcon(name, (wxBitmapType) type, desiredWidth, desiredHeight);
            }
        }
    }

    return NULL;
}

/***
 * <method name="loadFile">
 *  <function returns="Boolean">
 *   <arg name="Name" type="String">
 *    The name of the file.
 *   </arg>
 *   <arg name="Type" type="Integer">
 *    The type of the Icon.
 *   </arg>
 *  </function>
 *  <desc>
 *   Loads an icon from a file.
 *  </desc>
 * </method>
 */
JSBool Icon::loadFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxIcon *p = GetPrivate(cx, obj);

	wxString name;
	int type;
	FromJS(cx, argv[0], name);
	if ( FromJS(cx, argv[1], type) )
	{
		*rval = ToJS(cx, p->LoadFile(name, (wxBitmapType) type));
		return JS_TRUE;
	}
	return JS_FALSE;
}
