#include "precompiled.h"

/*
 * wxJavaScript - colour.cpp
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
 * $Id: colour.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// colour.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "colour.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>misc/colour</file>
 * <module>gui</module>
 * <class name="wxColour">
 *  A colour is an object representing a combination of Red, Green, and Blue (RGB) intensity values.
 *  This is a list of predefined colour objects:
 *  wxBLACK, wxWHITE, wxRED, wxBLUE, wxGREEN, wxCYAN and wxLIGHT_GREY
 * </class>
 * <properties>
 *  <property name="blue" type="Integer" readonly="Y">
 *   Gets the Blue value
 *  </property>
 *  <property name="green" type="Integer" readonly="Y">
 *   Gets the Green value
 *  </property>
 *  <property name="ok" type="Boolean" readonly="Y">
 *   Returns true when the colour is valid
 *  </property>
 *  <property name="red" type="Integer" readonly="Y">
 *   Gets the Red value
 *  </property>
 * </properties>
 */
WXJS_INIT_CLASS(Colour, "wxColour", 1)

WXJS_BEGIN_PROPERTY_MAP(Colour)
	WXJS_READONLY_PROPERTY(P_BLUE, "blue")
	WXJS_READONLY_PROPERTY(P_GREEN, "green")
	WXJS_READONLY_PROPERTY(P_RED, "red")
	WXJS_READONLY_PROPERTY(P_OK, "ok")
WXJS_END_PROPERTY_MAP()

WXJS_BEGIN_METHOD_MAP(Colour)
    WXJS_METHOD("set", set, 3)
WXJS_END_METHOD_MAP()

bool Colour::GetProperty(wxColour *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch(id) 
	{
	case P_BLUE:
		*vp = ToJS<int>(cx, p->Blue());
		break;
	case P_GREEN:
		*vp = ToJS<int>(cx, p->Green());
		break;
	case P_RED:
		*vp = ToJS<int>(cx, p->Red());
		break;
	case P_OK:
		*vp = ToJS<bool>(cx, p->Ok());
		break;
    }
	return true;
}

/***
 * <ctor>
 *  <function>
 *   <arg name="Red" type="Integer">
 *    The red value
 *   </arg>
 *   <arg name="Green" type="Integer">
 *    The green value
 *   </arg>
 *   <arg name="Blue" type="Integer">
 *    The blue value
 *   </arg>
 *  </function>
 *  <function>
 *   <arg name="ColourName" type="String">
 *    The name used to retrieve the colour from wxTheColourDatabase
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxColour object.
 *  </desc>
 * </ctor>
 */
wxColour *Colour::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    if ( argc == 3 ) // RGB
    {
		int R;
		int G;
		int B;
		if (    FromJS<int>(cx, argv[0], R)
			 && FromJS<int>(cx, argv[1], G)
			 && FromJS<int>(cx, argv[2], B) )
		{
			return new wxColour(R, G, B);
		}
    }
    else if ( argc == 1 )
    {
		wxString name;
		FromJS(cx, argv[0], name);
        return new wxColour(name);
    }
	return NULL;
}

/***
 * <method name="set">
 *  <function>
 *   <arg name="Red" type="Integer">
 *    The red value
 *   </arg>
 *   <arg name="Green" type="Integer">
 *    The green value
 *   </arg>
 *   <arg name="Blue" type="Integer">
 *    The blue value
 *   </arg>
 *  </function>
 *  <desc>
 *   Sets the RGB values.
 *  </desc>
 * </method>
 */
JSBool Colour::set(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxColour *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxColour"));

	int R;
	int G;
	int B;
	if (    FromJS<int>(cx, argv[0], R)
		 && FromJS<int>(cx, argv[1], G)
		 && FromJS<int>(cx, argv[2], B) )
	{
		p->Set((unsigned char) R,
			   (unsigned char) G,
			   (unsigned char) B);
	}
	else
	{
		return JS_FALSE;
	}

	return JS_TRUE;
}

void wxjs::gui::DefineGlobalColours(JSContext *cx, JSObject *obj)
{
    DefineGlobalColour(cx, obj, "wxRED", wxRED);
	DefineGlobalColour(cx, obj, "wxBLACK", wxBLACK);
	DefineGlobalColour(cx, obj, "wxWHITE", wxWHITE);
	DefineGlobalColour(cx, obj, "wxRED", wxRED);
	DefineGlobalColour(cx, obj, "wxBLUE", wxBLUE);
	DefineGlobalColour(cx, obj, "wxGREEN", wxGREEN);
	DefineGlobalColour(cx, obj, "wxCYAN", wxCYAN);
	DefineGlobalColour(cx, obj, "wxLIGHT_GREY", wxLIGHT_GREY);
	DefineGlobalColour(cx, obj, "wxNullColour", &wxNullColour);
}

void wxjs::gui::DefineGlobalColour(JSContext *cx, JSObject *obj,
						const char*name, const wxColour *colour)
{
    wxASSERT_MSG(colour != NULL, wxT("wxColour can't be NULL"));
    // Create a new colour object, because wxWindows destroys the global
    // colour objects and wxJSColour does the same. 
	Colour::DefineObject(cx, obj, name, new wxColour(*colour)); 
}
