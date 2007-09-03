#include "precompiled.h"

/*
 * wxJavaScript - size.cpp
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
 * $Id: size.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// size.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "size.h"
using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>misc/size</file>
 * <module>gui</module>
 * <class name="wxSize">
 *  A wxSize is a useful data structure for graphics operations. 
 *  It simply contains integer width and height members.
 * </class>
 */
WXJS_INIT_CLASS(Size, "wxSize", 0)

/***
 * <properties>
 *  <property name="height" type="Integer">
 *   The height property.
 *  </property>
 *  <property name="width" type="Integer">
 *   The width property.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(Size)
  WXJS_PROPERTY(P_WIDTH, "width")
  WXJS_PROPERTY(P_HEIGHT, "height")
WXJS_END_PROPERTY_MAP()

bool Size::GetProperty(wxSize *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch (id) 
	{
	case P_WIDTH:
		*vp = ToJS(cx, p->GetWidth());
		break;
	case P_HEIGHT:
		*vp = ToJS(cx, p->GetHeight());
		break;
    }
	return true;
}

bool Size::SetProperty(wxSize *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch (id) 
	{
	case P_WIDTH:
		{
			int value;
			if ( FromJS(cx, *vp, value) )
				p->SetWidth(value);
			break;
		}
	case P_HEIGHT:
		{
			int value;
			if ( FromJS(cx, *vp, value) )
				p->SetHeight(value);
			break;
		}
	}
    return true;
}

/***
 * <ctor>
 *  <function>
 *   <arg name="Width" type="Integer" default="-1">
 *    The width
 *   </arg>
 *   <arg name="Height" type="Integer" default="-1">
 *     The height. When not specified and Width is specified the value will be 0.
 *   </arg>
 *  </function>
 *  <desc>
 *   Creates a new wxSize. When no arguments are given then wxSize gets the same value
 *   as wxDefaultSize.
 *  </desc>
 * </ctor>
 */
wxSize* Size::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
	int x = -1;
	int y = -1;

	if (      argc > 0
		 && ! FromJS(cx, argv[0], x) )
		return NULL;

	if (      argc > 1
		 && ! FromJS(cx, argv[1], y) )
		return NULL;

	return new wxSize(x, y);
}

