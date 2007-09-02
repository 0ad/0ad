#include "precompiled.h"

/*
 * wxJavaScript - rect.cpp
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
 * $Id: rect.cpp 810 2007-07-13 20:07:05Z fbraem $
 */
// rect.cpp

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/apiwrap.h"
#include "../../ext/wxjs_ext.h"

#include "rect.h"
#include "size.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>misc/rect</file>
 * <module>gui</module>
 * <class name="wxRect">
 *	A class for manipulating rectangles
 * </class>
 */
WXJS_INIT_CLASS(Rect, "wxRect", 0)

/***
 * <properties>
 *	<property name="width" type="Integer">
 *	 The width of the rectangle
 *  </property>
 *	<property name="height" type="Integer">
 *	 The height of the rectangle
 *  </property>
 *	<property name="bottom" type="Integer">
 *	 The bottom
 *  </property>
 *  <property name="left" type="Integer" />
 *  <property name="position" type="@wxPoint" readonly="Y" />
 *  <property name="right" type="Integer" />
 *  <property name="size" type="@wxSize" readonly="Y" />
 *  <property name="top" type="Integer" />
 *  <property name="x" type="Integer" />
 *  <property name="y" type="Integer" />
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(Rect)
	WXJS_PROPERTY(P_WIDTH, "width")
	WXJS_PROPERTY(P_HEIGHT, "height")
	WXJS_PROPERTY(P_BOTTOM, "bottom")
	WXJS_READONLY_PROPERTY(P_LEFT, "left")
	WXJS_PROPERTY(P_POSITION, "position")
	WXJS_PROPERTY(P_RIGHT, "right")
	WXJS_READONLY_PROPERTY(P_SIZE, "size")
	WXJS_PROPERTY(P_TOP, "top")
	WXJS_PROPERTY(P_X, "x")
	WXJS_PROPERTY(P_Y, "y")
WXJS_END_PROPERTY_MAP()

bool Rect::GetProperty(wxRect *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	switch (id) 
	{
	case P_WIDTH:
		*vp = ToJS(cx, p->GetWidth());
		break;
	case P_HEIGHT:
		*vp = ToJS(cx, p->GetHeight());
		break;
	case P_BOTTOM:
		*vp = ToJS(cx, p->GetBottom());
		break;
	case P_LEFT:
		*vp = ToJS(cx, p->GetLeft());
		break;
	case P_POSITION:
      *vp = wxjs::ext::CreatePoint(cx, p->GetPosition());
		break;
	case P_RIGHT:
		*vp = ToJS(cx, p->GetRight());
		break;
	case P_SIZE:
		*vp = Size::CreateObject(cx, new wxSize(p->GetSize()));
		break;
	case P_TOP:
		*vp = ToJS(cx, p->GetTop());
		break;
	case P_X:
		*vp = ToJS(cx, p->GetX());
		break;
	case P_Y:
		*vp = ToJS(cx, p->GetY());
		break;
	}
	return true;
}

bool Rect::SetProperty(wxRect *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	switch (id) 
	{
	case P_WIDTH:
		{
			int width;
			if ( FromJS(cx, *vp, width) )
				p->SetWidth(width);
			break;
		}
	case P_HEIGHT:
		{
			int height;
			if ( FromJS(cx, *vp, height) )
				p->SetHeight(height);
			break;
		}
	case P_BOTTOM:
		{
			int bottom;
			if ( FromJS(cx, *vp, bottom) )
				p->SetBottom(bottom);
			break;
		}
	case P_LEFT:
		{
			int left;
			if ( FromJS(cx, *vp, left) )
				p->SetLeft(left);
			break;
		}
	case P_RIGHT:
		{
			int right;
			if ( FromJS(cx, *vp, right) )
				p->SetRight(right);
			break;
		}
	case P_TOP:
		{
			int top;
			if ( FromJS(cx, *vp, top) )
				p->SetTop(top);
			break;
		}
	case P_X:
		{
			int x;
			if ( FromJS(cx, *vp, x ) )
				p->SetX(x);
			break;
		}
	case P_Y:
		{
			int y;
			if ( FromJS(cx, *vp, y) )
				p->SetY(y);
			break;
		}
	}
	return true;
}

/***
 * <ctor>
 *  <function>
 *   <arg name="X" type="Integer">
 *	  X-coordinate of the top level corner
 *   </arg>
 *   <arg name="Y" type="Integer">
 *	  Y-coordinate of the top level corner
 *   </arg>
 *   <arg name="Width" type="Integer">
 *	  The width of the rectangle
 *   </arg>
 *   <arg name="Height" type="Integer">
 *	  The height of the rectangle
 *   </arg>
 *  </function>
 *  <function>
 *	 <arg name="TopLeft" type="@wxPoint">
 *	  The top-left corner
 *   </arg>
 *	 <arg name="BottomRight" type="@wxPoint">
 *	  The bottom-right corner
 *   </arg>
 *  </function>
 *  <function>
 *	 <arg name="Position" type="@wxPoint" />
 *   <arg name="Size" type="@wxSize" />
 *  </function>
 *  <desc>
 *	 Constructs a new wxRect object.
 *  </desc>
 * </ctor>
 */
wxRect* Rect::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
	if ( argc == 0 )
		return new wxRect();

    if ( argc >= 4 )
	{
		int x;
		int y;
		int width;
		int height;

        if (    FromJS(cx, argv[0], x)
             && FromJS(cx, argv[1], y)
             && FromJS(cx, argv[2], width)
             && FromJS(cx, argv[3], height) )
		{
			return new wxRect(x, y, width, height);
		}
	}
	else if ( argc == 2 )
	{
      wxPoint *pt1 = wxjs::ext::GetPoint(cx, argv[0]);
		if ( pt1 != NULL )
		{
          wxPoint *pt2 = wxjs::ext::GetPoint(cx, argv[1]);
			if ( pt2 != NULL )
			{
				return new wxRect(*pt1, *pt2);
			}
			else
			{
				wxSize *size = Size::GetPrivate(cx, argv[1]);
				if ( size != NULL )
					return new wxRect(*pt1, *size);
			}
		}
	}
    return NULL;
}

WXJS_BEGIN_METHOD_MAP(Rect)
  WXJS_METHOD("inflate", inflate, 1)
  WXJS_METHOD("deflate", deflate, 1)
  WXJS_METHOD("offset", offset, 1)
  WXJS_METHOD("intersect", intersect, 1)
  WXJS_METHOD("inside", inside, 1)
  WXJS_METHOD("intersects", intersects, 1)
WXJS_END_METHOD_MAP()

/***
 * <method name="inflate">
 *	<function>
 *	 <arg name="X" type="Integer" />
 *	 <arg name="Y" type="Integer" />
 *  </function>
 *  <desc>
 *	 Increases the rectangle size by X in x direction and by Y in y direction.
 *	 When Y is not specified then the value of X is used. You can use negative
 *	 values to decrease the size.
 *  </desc>
 * </method>
 */
JSBool Rect::inflate(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  if ( argc > 2 )
      argc = 2;

  wxCoord x = 0;
  wxCoord y = 0;

  if ( FromJS(cx, argv[0], x) )
  {
	if ( argc > 1 )
	{
		if ( ! FromJS(cx, argv[1], y) )
		{
			return JS_FALSE;
		}
	}
	else
	{
		y = x;
	}
  }
  else
  {
	return JS_FALSE;
  }

  wxRect *p = Rect::GetPrivate(cx, obj);
  p->Inflate(x, y);
  return JS_TRUE;
}

/***
 * <method name="deflate">
 *	<function>
 *	 <arg name="X" type="Integer" />
 *	 <arg name="Y" type="Integer" default="X"/>
 *  </function>
 *  <desc>
 *	 Decreases the rectangle size by X in x direction and by Y in y direction.
 *	 When Y is not specified then the value of X is used. You can use negative
 *	 values to decrease the size.
 *  </desc>
 * </method>
 */
JSBool Rect::deflate(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  if ( argc > 2 )
      argc = 2;

  wxCoord x = 0;
  wxCoord y = 0;

  if ( FromJS(cx, argv[0], x) )
  {
	if ( argc > 1 )
	{
		if ( ! FromJS(cx, argv[1], y) )
		{
			return JS_FALSE;
		}
	}
	else
	{
		y = x;
	}
  }
  else
  {
	return JS_FALSE;
  }

  wxRect *p = Rect::GetPrivate(cx, obj);
  p->Deflate(x, y);

  return JS_TRUE;
}

/***
 * <method name="offset">
 *	<function>
 *	 <arg name="X" type="Integer" />
 *	 <arg name="Y" type="Integer" />
 *  </function>
 *  <function>
 *   <arg name="Pt" type="@wxPoint" />
 *  </function>
 *  <desc>
 *	 Moves the rectangle.
 *  </desc>
 * </method>
 */
JSBool Rect::offset(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxRect *p = GetPrivate(cx, obj);

	switch (argc)
    {
    case 2:
	    {
		    int x;
		    int y;
		    if (    FromJS(cx, argv[0], x)
			     && FromJS(cx, argv[1], y) )
		    {
			    p->Offset(x, y);
		    }
            else
            {
                return JS_FALSE;
            }
            break;
	    }
    case 1:
	    {
          wxPoint *pt = wxjs::ext::GetPoint(cx, argv[0]);
		    if ( pt != NULL )
		    {
			    p->Offset(*pt);
		    }
            else
            {
                return JS_FALSE;
            }
            break;
	    }
    default:
        return JS_FALSE;
    }

	return JS_TRUE;
}

/***
 * <method name="intersect">
 *  <function returns="wxRect">
 *	 <arg name="Rect" type="wxRect" />
 *  </function>
 * </method>
 */
JSBool Rect::intersect(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxRect *p = GetPrivate(cx, obj);

    wxRect *argRect = GetPrivate(cx, argv[0]);
	if ( argRect != NULL )
	{
        *rval = CreateObject(cx, new wxRect(p->Intersect(*argRect)));
		return JS_TRUE;
	}

    return JS_FALSE;
}

/***
 * <method name="inside">
 *	<function returns="Boolean">
 *	 <arg name="X" type="Integer" />
 *	 <arg name="Y" type="Integer" />
 *  </function>
 *  <function>
 *   <arg name="Pt" type="@wxPoint" />
 *  </function>
 *  <desc>
 *	 Returns true when the given coordinates are in the rectangle area.
 *  </desc>
 * </method>
 */
JSBool Rect::inside(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxRect *p = GetPrivate(cx, obj);

    switch(argc)
    {
    case 2:
	    {
		    int x;
		    int y;
		    if (    FromJS(cx, argv[0], x)
			     && FromJS(cx, argv[1], y) )
		    {
			    *rval = ToJS(cx, p->Inside(x, y));
		    }
            else
            {
                return JS_FALSE;
            }
            break;
	    }
    case 1:
	    {
          wxPoint *pt = wxjs::ext::GetPoint(cx, argv[0]);
		    if ( pt != NULL )
		    {
			    *rval = ToJS(cx, p->Inside(*pt));
		    }
            else
            {
                return JS_FALSE;
            }
            break;
	    }
    default:
        return JS_FALSE;
    }

    return JS_TRUE;
}

/***
 * <method name="intersects">
 *	<function returns="Boolean">
 *	 <arg name="Rect" type="wxRect" />
 *  </function>
 *  <desc>
 *	 Returns true when the rectangles have a non empty intersection
 *  </desc>
 * </method>
 */
JSBool Rect::intersects(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxRect *p = GetPrivate(cx, obj);

    wxRect *argRect = GetPrivate(cx, argv[0]);
	if ( argRect != NULL )
	{
		*rval = ToJS(cx, p->Intersects(*argRect));
	    return JS_TRUE;
	}

    return JS_FALSE;
}
