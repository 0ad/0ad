#include "precompiled.h"

/*
 * wxJavaScript - stsizer.cpp
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
 * $Id: stsizer.cpp 733 2007-06-05 21:17:25Z fbraem $
 */
// stsizer.cpp

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "../control/staticbx.h"
#include "sizer.h"
#include "stsizer.h"
using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>misc/stsizer</file>
 * <module>gui</module>
 * <class name="wxStaticBoxSizer" prototype="@wxBoxSizer">
 *	wxStaticBoxSizer is a sizer derived from wxBoxSizer but adds
 *	a static box around the sizer. Note that this static box has to be created separately.
 *	See @wxBoxSizer, @wxSizer and @wxStaticBox
 * </class>
 */
WXJS_INIT_CLASS(StaticBoxSizer, "wxStaticBoxSizer", 2)

/***
 * <properties>
 *	<property name="staticBox" type="@wxStaticBox" readonly="Y">
 *	 The associated static box.
 *  </property>
 * </properties>
 */

WXJS_BEGIN_PROPERTY_MAP(StaticBoxSizer)
  WXJS_READONLY_PROPERTY(P_STATIC_BOX, "staticBox")
WXJS_END_PROPERTY_MAP()

bool StaticBoxSizer::GetProperty(wxStaticBoxSizer *p,
                                 JSContext *cx,
                                 JSObject *obj,
                                 int id,
                                 jsval *vp)
{
	if ( id == P_STATIC_BOX )
	{
		wxStaticBox *staticBox = p->GetStaticBox();
        if ( staticBox != NULL )
        {
          JavaScriptClientData *data
            = dynamic_cast<JavaScriptClientData*>(staticBox->GetClientObject());
		  *vp = data == NULL ? JSVAL_VOID 
					         : OBJECT_TO_JSVAL(data->GetObject());
        }
	}
	return true;
}

/***
 * <ctor>
 *	<function>
 *   <arg name="StaticBox" type="@wxStaticBox">
 *	  The staticbox associated with the sizer.
 *   </arg>
 *   <arg name="Orientation" type="Integer">
 *	  Orientation wxVERTICAL or wxHORIZONTAL for creating either a column 
 *	  or row sizer. See @wxOrientation
 *   </arg>
 *  </function>
 *  <desc>
 *	 Constructs a new wxStaticBoxSizer object.
 *  </desc>
 * </ctor>
 */
wxStaticBoxSizer* StaticBoxSizer::Construct(JSContext *cx,
                                          JSObject *obj,
                                          uintN argc,
                                          jsval *argv,
                                          bool constructing)
{
  wxStaticBoxSizer *p = NULL;

    if ( argc > 2 )
        argc = 2;

    wxStaticBox *box = StaticBox::GetPrivate(cx, argv[0]);
    if ( box == NULL )
        return NULL;

    int orient;

	if ( FromJS(cx, argv[1], orient) )
	{
   	  p = new wxStaticBoxSizer(box, orient);
	}

    if ( p != NULL )
    {
      p->SetClientObject(new JavaScriptClientData(cx, obj, false, true));
    }

	return p;
}
