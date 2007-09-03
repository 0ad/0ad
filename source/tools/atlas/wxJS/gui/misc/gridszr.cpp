#include "precompiled.h"

/*
 * wxJavaScript - gridszr.cpp
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
 * $Id: gridszr.cpp 733 2007-06-05 21:17:25Z fbraem $
 */
// gridszr.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/apiwrap.h"

#include "sizer.h"
#include "gridszr.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>misc/gridszr</file>
 * <module>gui</module>
 * <class name="wxGridSizer" prototype="@wxSizer">
 *  A grid sizer is a sizer which lays out its children in a two-dimensional table with 
 *  all table fields having the same size, i.e. the width of each field is the width of 
 *  the widest child, the height of each field is the height of the tallest child.
 * </class>
 */
WXJS_INIT_CLASS(GridSizer, "wxGridSizer", 4)

WXJS_BEGIN_PROPERTY_MAP(GridSizer)
  WXJS_PROPERTY(P_COLS, "cols")
  WXJS_PROPERTY(P_ROWS, "rows")
  WXJS_PROPERTY(P_HGAP, "hgap")
  WXJS_PROPERTY(P_VGAP, "vgap")
WXJS_END_PROPERTY_MAP()

bool GridSizer::GetProperty(wxGridSizer *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch(id)
    {
    case P_COLS:
        *vp = ToJS(cx, p->GetCols());
        break;
    case P_ROWS:
        *vp = ToJS(cx, p->GetRows());
        break;
    case P_HGAP:
        *vp = ToJS(cx, p->GetHGap());
        break;
    case P_VGAP:
        *vp = ToJS(cx, p->GetVGap());
        break;
    }
    return true;
}

bool GridSizer::SetProperty(wxGridSizer *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch (id)
    {
    case P_COLS:
        {
            int value;
            if ( FromJS(cx, *vp, value) )
                p->SetCols(value);
            break;
        }
    case P_ROWS:
        {
            int value;
            if ( FromJS(cx, *vp, value) )
                p->SetRows(value);
            break;
        }
    case P_HGAP:
        {
            int value;
            if ( FromJS(cx, *vp, value) )
                p->SetHGap(value);
            break;
        }
    case P_VGAP:
        {
            int value;
            if ( FromJS(cx, *vp, value) )
                p->SetVGap(value);
            break;
        }
    }
    return true;
}

/***
 * <ctor>
 *  <function>
 *   <arg name="Rows" type="Integer">
 *    The number of rows.
 *   </arg>
 *   <arg name="Cols" type="Integer">
 *    The number of columns.
 *   </arg>
 *   <arg name="Vgap" type="Integer">
 *    The space between the columns
 *   </arg>
 *   <arg name="Hgap" type="Integer">
 *    The space between the rows
 *   </arg>
 *  </function>
 *  <function>
 *   <arg name="Cols" type="Integer">
 *    The number of columns.
 *   </arg>
 *   <arg name="Vgap" type="Integer">
 *    The space between the columns
 *   </arg>
 *   <arg name="Hgap" type="Integer">
 *    The space between the rows
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxGridSizer object.
 *  </desc>
 * </ctor>
 */
wxGridSizer *GridSizer::Construct(JSContext *cx, 
                                  JSObject *obj,
                                  uintN argc,
                                  jsval *argv,
                                  bool constructing)
{
  wxGridSizer *p = NULL;

    int cols = 0;
	int rows = 0;
	int vgap = 0;
	int hgap = 0;

	if ( argc > 4 )
        argc = 4;

	if ( argc == 4 )
	{
		if (    FromJS(cx, argv[0], rows)
			 && FromJS(cx, argv[1], cols)
			 && FromJS(cx, argv[2], vgap)
			 && FromJS(cx, argv[3], hgap) )
		{
			p = new wxGridSizer(rows, cols, vgap, hgap);
		}
	}
	else if ( argc < 4 )
	{
		switch(argc)
		{
		case 3:
			if ( ! FromJS(cx, argv[2], hgap) )
				break;
		case 2:
			if ( ! FromJS(cx, argv[1], vgap) )
				break;
		case 1:
			if ( ! FromJS(cx, argv[0], cols) )
				break;
			p = new wxGridSizer(cols, vgap, hgap);
		}
	}

    if ( p != NULL )
    {
      p->SetClientObject(new JavaScriptClientData(cx, obj, false, true));
    }

	return p;
}
