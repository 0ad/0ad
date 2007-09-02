#include "precompiled.h"

/*
 * wxJavaScript - point.cpp
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
 * $Id$
 */
// point.cpp

#include <wx/wx.h>

#include "../common/main.h"

#include "point.h"
using namespace wxjs;
using namespace wxjs::ext;

/***
 * <file>point</file>
 * <module>ext</module>
 * <class name="wxPoint">
 *  A wxPoint is a useful data structure for graphics operations.
 *  It simply contains integer x and y members.
 * </class>
 */
WXJS_INIT_CLASS(Point, "wxPoint", 0)

/***
 * <properties>
 *  <property name="x" type="Integer">
 *   The x-coordinate.
 *  </property>
 *  <property name="y" type="Integer">
 *   The y-coordinate.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(Point)
  WXJS_PROPERTY(P_X, "x")
  WXJS_PROPERTY(P_Y, "y")
WXJS_END_PROPERTY_MAP()

bool Point::GetProperty(wxPoint *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch (id) 
	{
	case P_X:
		*vp = ToJS(cx, p->x);
		break;
	case P_Y:
		*vp = ToJS(cx, p->y);
		break;
    }
    return true;
}

bool Point::SetProperty(wxPoint *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch (id) 
	{
	case P_X:
		FromJS(cx, *vp, p->x);
		break;
	case P_Y:
		FromJS(cx, *vp, p->y);
		break;
    }
    return true;
}

/***
 * <ctor>
 *  <function>
 *   <arg name="X" type="Integer" default="wxDefaultPosition.x">
 *    The X-coordinate
 *   </arg>
 *   <arg name="Y" type="Integer" default="0">
 *    The Y-coordinate.
 *   </arg>
 *  </function>
 *  <desc>
 *   Creates a new wxPoint. When no arguments are given then wxPoint gets the same value
 *   as wxDefaultPosition.
 *  </desc>
 * </ctor>
 */
wxPoint* Point::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
	if ( argc == 0 )
	{
		return new wxPoint();
	}
	else
	{
		int x = 0;
		int y = 0;
		if ( argc > 0 )
		{
			FromJS(cx, argv[0], x);
		}
		
		if ( argc > 1 )
		{
			FromJS(cx, argv[1], y);
		}
		return new wxPoint(x, y);
	}
}
