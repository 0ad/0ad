#include "precompiled.h"

/*
 * wxJavaScript - boxsizer.cpp
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
 * $Id: boxsizer.cpp 733 2007-06-05 21:17:25Z fbraem $
 */
// boxsizer.cpp
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/apiwrap.h"

#include "sizer.h"
#include "boxsizer.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>misc/boxsizer</file>
 * <module>gui</module>
 * <class name="wxBoxSizer" prototype="@wxSizer">
 *	The basic idea behind a box sizer is that windows will most often be laid out in 
 *  rather simple basic geometry, typically in a row or a column or several hierarchies of either.
 * </class>
 */
WXJS_INIT_CLASS(BoxSizer, "wxBoxSizer", 1)
 
/***
 * <properties>
 *	<property name="orientation" type="Integer" readonly="Y">
 *	 Gets the orientation. See @wxOrientation
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(BoxSizer)
  WXJS_READONLY_PROPERTY(P_ORIENTATION, "orientation")
WXJS_END_PROPERTY_MAP()

bool BoxSizer::GetProperty(wxBoxSizer *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	if ( id == P_ORIENTATION )
	{
		*vp = ToJS(cx, p->GetOrientation());
	}
	return true;
}

/***
 * <ctor>
 *	<function>
 *	 <arg name="orientation" type="Integer">
 *	  Orientation VERTICAL or HORIZONTAL for creating either a column 
 *	  or row sizer. See @wxOrientation
 *   </arg>
 *  </function>
 *  <desc>
 *   Creates a new wxBoxSizer object
 *  </desc>
 * </ctor>
 */
wxBoxSizer *BoxSizer::Construct(JSContext *cx,
                              JSObject *obj,
                              uintN argc,
                              jsval *argv,
                              bool constructing)
{
	int orient = wxVERTICAL;

	if ( FromJS(cx, argv[0], orient) )
    {
      wxBoxSizer *p = new wxBoxSizer(orient);
      p->SetClientObject(new JavaScriptClientData(cx, obj, false, true));
	  return p;
    }
    return NULL;
}
