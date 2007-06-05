#include "precompiled.h"

/*
 * wxJavaScript - staticbx.cpp
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
 * $Id: staticbx.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// staticbx.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "../event/evthand.h"

#include "staticbx.h"
#include "window.h"

#include "../misc/point.h"
#include "../misc/size.h"

using namespace wxjs;
using namespace wxjs::gui;

StaticBox::StaticBox(JSContext *cx, JSObject *obj)
	:    wxStaticBox()
	   , Object(obj, cx)
{
	PushEventHandler(new EventHandler(this));
}
	
StaticBox::~StaticBox()
{
	PopEventHandler(true);
}

/***
 * <file>control/staticbox</file>
 * <module>gui</module>
 * <class name="wxStaticBox" prototype="@wxControl">
 *  A static box is a rectangle drawn around other panel items to denote a logical grouping of items.
 * </class>
 */
WXJS_INIT_CLASS(StaticBox, "wxStaticBox", 6)

/***
 * <ctor>
 *  <function>
 *   <arg name="Parent" type="@wxWindow">
 *    The parent of the staticbox
 *   </arg>
 *   <arg name="Id" type="Integer">
 *    The unique identifier.
 *   </arg>
 *   <arg name="Text" type="String">
 *    The text of the staticbox
 *   </arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *    The position of the staticbox.
 *   </arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *    The size of the staticBox.
 *   </arg>
 *   <arg name="Style" type="Integer" default="0">
 *    The style of the staticbox.
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxStaticBox object
 *  </desc>
 * </ctor>
 */
wxStaticBox* StaticBox::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    if ( argc > 6 )
        argc = 6;

	const wxPoint *pt = &wxDefaultPosition;
	const wxSize *size = &wxDefaultSize;
    int style = 0;

    switch(argc)
    {
    case 6:
        if ( ! FromJS(cx, argv[5], style) )
            break;
        // Fall through
    case 5:
		size = Size::GetPrivate(cx, argv[4]);
		if ( size == NULL )
			break;
		// Fall through
	case 4:
		pt = Point::GetPrivate(cx, argv[3]);
		if ( pt == NULL )
			break;
		// Fall through
    default:
        wxString text;
        FromJS(cx, argv[2], text);

        int id;
        if ( ! FromJS(cx, argv[1], id) )
            break;

        wxWindow *parent = Window::GetPrivate(cx, argv[0]);
		if ( parent == NULL )
			break;

        Object *wxjsParent = dynamic_cast<Object *>(parent);
        JS_SetParent(cx, obj, wxjsParent->GetObject());

	    wxStaticBox *p = new StaticBox(cx, obj);
	    p->Create(parent, id, text, *pt, *size, style);
        return p;
    }

    return NULL;
}
