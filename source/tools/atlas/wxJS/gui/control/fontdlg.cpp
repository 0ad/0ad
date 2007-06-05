#include "precompiled.h"

/*
 * wxJavaScript - fontdlg.cpp
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
 * $Id: fontdlg.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// fontdlg.cpp

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "fontdlg.h"
#include "fontdata.h"
#include "window.h"

#include "../misc/point.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>control/fontdlg</file>
 * <module>gui</module>
 * <class name="wxFontDialog" prototype="@wxDialog">
 *	The wxFontDialog presents a Font selector to the user.
 * </class>
 */
WXJS_INIT_CLASS(FontDialog, "wxFontDialog", 1)

/***
 * <properties>
 *	<property name="FontData" type="@wxFontData" readonly="Y">
 *	 Gets the Font data.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(FontDialog)
  WXJS_READONLY_PROPERTY(P_FONT_DATA, "FontData")
WXJS_END_PROPERTY_MAP()

bool FontDialog::GetProperty(wxFontDialog *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	if ( id == P_FONT_DATA )
        *vp = FontData::CreateObject(cx, new wxFontData(p->GetFontData()));
    return true;
}

/***
 * <ctor>
 *	<function>
 *	 <arg name="Parent" type="@wxWindow">
 *	  The parent of wxFontDialog.
 *   </arg>
 *	 <arg name="FontData" type="@wxFontData">
 *	  The Font data.
 *   </arg>
 *  </function>
 *	<desc>
 *   Constructs a new wxFontDialog object
 *  </desc>
 * </ctor>
 */
wxFontDialog* FontDialog::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    if ( argc > 2 )
        return NULL;

    wxFontData *data = NULL;
    if ( argc == 2 )
    {
        if ( (data = FontData::GetPrivate(cx, argv[1])) == NULL )
            return NULL;
    }

    wxWindow *parent = Window::GetPrivate(cx, argv[0]);
	if ( parent != NULL )
    {
        Object *wxjsParent = dynamic_cast<Object *>(parent);
        JS_SetParent(cx, obj, wxjsParent->GetObject());
    }

#if wxCHECK_VERSION(2,7,0)
    wxFontDialog *p = new wxFontDialog(parent, *data);
#else
    wxFontDialog *p = new wxFontDialog(parent, data);
#endif
    return p;
}

void FontDialog::Destruct(JSContext *cx, wxFontDialog *p)
{
	p->Destroy();
}
