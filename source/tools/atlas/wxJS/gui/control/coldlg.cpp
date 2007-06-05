#include "precompiled.h"

/*
 * wxJavaScript - coldlg.cpp
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
 * $Id: coldlg.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// coldlg.cpp

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/index.h"
#include "coldlg.h"
#include "coldata.h"
#include "window.h"

#include "../misc/point.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>control/coldlg</file>
 * <module>gui</module>
 * <class name="wxColourDialog" prototype="@wxDialog">
 *	The wxColourDialog presents a colour selector to the user.
 *  See also @wxColourData. The following sample shows this 
 *  dialog:
 *  <pre><code class="whjs">
 *    wxTheApp.onInit = function()
 *    {
 *      clrData = new wxColourData();
 *      // Set the selected colour
 *      clrData.colour = new wxColour(0, 0, 0);
 *
 *      // Set a custom colour
 *      clrData.customColour[0]  = wxRED;
 *    
 *      dlg = new wxColourDialog(null, clrData);
 *      dlg.title = "Select a colour";
 *      dlg.showModal();
 *
 *      // Return false to exit the mainloop
 *      return false;
 *    }
 *    wxTheApp.mainLoop();
 *  </code></pre>
 * </class>
 */
WXJS_INIT_CLASS(ColourDialog, "wxColourDialog", 1)

/***
 * <properties>
 *	<property name="colourData" type="@wxColourData" readonly="Y">
 *	 Gets the colour data.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(ColourDialog)
  WXJS_READONLY_PROPERTY(P_COLOUR_DATA, "colourData")
WXJS_END_PROPERTY_MAP()

bool ColourDialog::GetProperty(wxColourDialog *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    if ( id == P_COLOUR_DATA )
    {
        *vp = ColourData::CreateObject(cx, new wxColourData(p->GetColourData()));
	}
	return true;
}

/***
 * <ctor>
 *	<function>
 *	 <arg name="Parent" type="@wxWindow">
 *	  The parent of wxColourDialog.
 *   </arg>
 *   <arg name="ColourData" type="@wxColourData">
 *	  The colour data.
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxColourDialog object
 *  </desc>
 * </ctor>
 */
wxColourDialog* ColourDialog::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    if ( argc > 2 )
        argc = 2;

    wxColourData *data = NULL;
    if ( argc == 2 )
    {
        data = ColourData::GetPrivate(cx, argv[1]);
        if ( data == NULL )
            return NULL;
    }

    wxWindow *parent = Window::GetPrivate(cx, argv[0]);
	if ( parent != NULL )
    {
        Object *wxjsParent = dynamic_cast<Object *>(parent);
        JS_SetParent(cx, obj, wxjsParent->GetObject());
    }

	return new wxColourDialog(parent, data);
}

void ColourDialog::Destruct(JSContext *cx, wxColourDialog *p)
{
    p->Destroy();
}
