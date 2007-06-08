#include "precompiled.h"

/*
 * wxJavaScript - flexgrid.cpp
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
 * $Id: flexgrid.cpp 733 2007-06-05 21:17:25Z fbraem $
 */
// flexgrid.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/apiwrap.h"

#include "sizer.h"
#include "flexgrid.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>misc/flexgrid</file>
 * <module>gui</module>
 * <class name="wxFlexGridSizer" prototype="@wxGridSizer">
 *  A flex grid sizer is a sizer which lays out its children in a two-dimensional
 *  table with all table fields in one row having the same height and all fields 
 *  in one column having the same width.
 * </class>
 */
WXJS_INIT_CLASS(FlexGridSizer, "wxFlexGridSizer", 4)

/***
 * <ctor>
 *  <function>
 *   <arg name="Rows">
 *    The number of rows.
 *   </arg>
 *   <arg name="Cols">
 *    The number of columns.
 *   </arg>
 *   <arg name="Vgap">
 *    The space between the columns
 *   </arg>
 *   <arg name="Hgap">
 *    The space between the rows
 *   </arg>
 *  </function>
 *  <function>
 *   <arg name="Cols">
 *    The number of columns.
 *   </arg>
 *   <arg name="Vgap" type="Integer" default="0">
 *    The space between the columns
 *   </arg>
 *   <arg name="Hgap" type="Integer" default="0">  
 *    The space between the rows
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxFlexGridSizer object.
 *  </desc>
 * </ctor>
 */
wxFlexGridSizer* FlexGridSizer::Construct(JSContext *cx,
                                          JSObject *obj,
                                          uintN argc, 
                                          jsval *argv,
                                          bool constructing)
{
  wxFlexGridSizer *p = NULL;

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
			p = new wxFlexGridSizer(rows, cols, vgap, hgap);
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
			p = new wxFlexGridSizer(cols, vgap, hgap);
		}
	}

    if ( p != NULL )
    {
      p->SetClientObject(new JavaScriptClientData(cx, obj, false, true));
    }

	return p;
}
