#include "precompiled.h"

/*
 * wxJavaScript - bitmap.cpp
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
 * $Id: bitmap.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// bitmap.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "bitmap.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>misc/bitmap</file>
 * <module>gui</module>
 * <class name="wxBitmap">
 *  This class encapsulates the concept of a platform-dependent bitmap, either monochrome or colour.
 *  See @wxIcon and @wxBitmapType
 * </class>
 */
WXJS_INIT_CLASS(Bitmap, "wxBitmap", 0)

/***
 * <properties>
 *  <property name="depth" type="Integer">
 *   Get/Set the colour depth of the bitmap. A value of 1 indicates a monochrome bitmap.
 *  </property>
 *  <property name="height" type="Integer">
 *   Get/Set the height of the bitmap in pixels.
 *  </property>
 *  <property name="ok" type="Boolean" readonly="Y">
 *   Returns true when the bitmap data is available.
 *  </property>
 *  <property name="width" type="Integer">
 *   Get/Set the width of the bitmap in pixels.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(Bitmap)
  WXJS_PROPERTY(P_DEPTH, "depth")
  WXJS_PROPERTY(P_HEIGHT, "height")
  WXJS_READONLY_PROPERTY(P_OK, "ok")
  WXJS_PROPERTY(P_WIDTH, "width")
WXJS_END_PROPERTY_MAP()

WXJS_BEGIN_METHOD_MAP(Bitmap)
  WXJS_METHOD("create", create, 2)
  WXJS_METHOD("loadFile", loadFile, 2)
WXJS_END_METHOD_MAP()

bool Bitmap::GetProperty(wxBitmap *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch(id) 
	{
	case P_DEPTH:
		*vp = ToJS(cx, p->GetDepth());
		break;
	case P_HEIGHT:
		*vp = ToJS(cx, p->GetHeight());
		break;
	case P_WIDTH:
		*vp = ToJS(cx, p->GetWidth());
		break;
	case P_OK:
		*vp = ToJS(cx, p->Ok());
		break;
    }
    return true;
}

bool Bitmap::SetProperty(wxBitmap *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch (id) 
	{
	case P_DEPTH:
		{
			int value;
			if ( FromJS(cx, *vp, value) )
				p->SetDepth(value);
			break;
		}
	case P_HEIGHT:
		{
			int value;
			if ( FromJS(cx, *vp, value) )
				p->SetHeight(value);
			break;
		}
	case P_WIDTH:
		{
			int value;
			if ( FromJS(cx, *vp, value) )
				p->SetWidth(value);
			break;
		}
	}
    return true;
}

/***
 * <ctor>
 *  <function />
 *  <function>
 *   <arg name="Name" type="String">Filename</arg>
 *   <arg name="Type" type="Integer">The type of the bitmap.</arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxBitmap object.
 *   See @wxBitmapType
 *  </desc>
 * </ctor>
 */
wxBitmap* Bitmap::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    if ( argc > 2 )
        argc = 2;

	switch(argc)
	{
	case 0:
		return new wxBitmap();
	case 2:
		{
			wxString name;
			int type;
			FromJS(cx, argv[0], name);
			if ( FromJS(cx, argv[1], type) )
				return new wxBitmap(name, (wxBitmapType) type);
		}
	}	
	return NULL;
}

/***
 * <method name="create">
 *  <function returns="Boolean">
 *   <arg name="Width" type="Integer">
 *    The width of the bitmap in pixels.
 *   </arg>
 *   <arg name="Height" type="Integer">
 *    The height of the bitmap in pixels.
 *   </arg>
 *   <arg name="Depth" type="Integer" default="-1">
 *    The depth of the bitmap in pixels. When omitted (or a value -1) , the screen depth is used.
 *   </arg>
 *  </function>
 *  <desc>
 *   Creates a fresh bitmap.
 *  </desc>
 * </method>
 */
JSBool Bitmap::create(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxBitmap *p = GetPrivate(cx, obj);

	int width = 0;
	int height = 0;
	int depth = -1;

	if (    FromJS(cx, argv[0], width)
		 && FromJS(cx, argv[1], height) )
	{
		if ( argc > 2 )
		{
			if ( ! FromJS(cx, argv[2], depth) )
			{
				return JS_FALSE;
			}
		}

		*rval = ToJS(cx, p->Create(width, height, depth));
		return JS_TRUE;
	}

    return JS_FALSE;
}
	
/***
 * <method name="loadFile">
 *  <function returns="Boolean">
 *   <arg name="Name" type="String">
 *    The name of the file.
 *   </arg>
 *   <arg name="Type" type="Integer">
 *    The type of the bitmap.
 *   </arg>
 *  </function>
 *  <desc>
 *   Loads a bitmap from a file.
 *  </desc>
 * </method>
 */
JSBool Bitmap::loadFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxBitmap *p = GetPrivate(cx, obj);

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
