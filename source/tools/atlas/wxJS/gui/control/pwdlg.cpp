#include "precompiled.h"

/*
 * wxJavaScript - pwdlg.cpp
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
 * $Id: pwdlg.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif
#include <wx/textdlg.h>

#include "../../common/main.h"

#include "../misc/point.h"

#include "../event/jsevent.h"
#include "../event/evthand.h"

#include "pwdlg.h"
#include "window.h"

using namespace wxjs;
using namespace wxjs::gui;

PasswordEntryDialog::PasswordEntryDialog(  JSContext *cx
												 , JSObject *obj
												 , wxWindow* parent
												 , const wxString& message
												 , const wxString& caption
												 , const wxString& defaultValue
												 , long style
												 , const wxPoint& pos)
   :   wxPasswordEntryDialog(parent, message, caption, defaultValue, style, pos)
	 , Object(obj, cx)
{
	PushEventHandler(new EventHandler(this));
}

PasswordEntryDialog::~PasswordEntryDialog()
{
	PopEventHandler(true);
}

/***
 * <file>control/pwdlg</file>
 * <module>gui</module>
 * <class name="wxPasswordEntryDialog" prototype="@wxTextEntryDialog">
 *	This class represents a dialog that requests a password from the user. 
 * </class>
 */
WXJS_INIT_CLASS(PasswordEntryDialog, "wxPasswordEntryDialog", 2)

/***
 * <ctor>
 *	<function>
 *	 <arg name="Parent" type="@wxWindow">The parent of the dialog. null is Allowed.</arg>
 *	 <arg name="Message" type="String">Message to show on the dialog.</arg>
 *   <arg name="Title" type="String">The title of the dialog.</arg>
 *   <arg name="DefaultValue" type="String">The default value of the text control.</arg>
 *	 <arg name="Position" type="@wxPoint" default="wxDefaultPosition">The position of the dialog.</arg>
 *	 <arg name="Style" type="Integer" default="wxId.OK + wxId.CANCEL">A dialog style, the buttons wxId.OK and wxId.CANCEL can be used.</arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxPasswordEntryDialog object.
 *  </desc>
 * </ctor>
 */
wxPasswordEntryDialog* PasswordEntryDialog::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
	if ( argc > 6 )
        argc = 6;

	int style = wxOK | wxCANCEL | wxCENTRE;
    const wxPoint *pt = &wxDefaultPosition;
	wxString defaultValue = wxEmptyString;
	wxString caption = wxEmptyString;

    switch(argc)
    {
    case 6:
		pt = Point::GetPrivate(cx, argv[5]);
		if ( pt == NULL )
			break;
		// Fall through
    case 5:
        if ( ! FromJS(cx, argv[4], style) )
            break;
        // Fall through
	case 4:
		if ( ! FromJS(cx, argv[3], defaultValue) )
			break;
		// Fall through
	case 3:
		if ( ! FromJS(cx, argv[2], caption) )
			break;
		// Fall through
    default:
        wxString message;
        if ( ! FromJS(cx, argv[1], message) )
			break;

        wxWindow *parent = Window::GetPrivate(cx, argv[0]);
		if ( parent != NULL )
        {
            Object *wxjsParent = dynamic_cast<Object *>(parent);
            JS_SetParent(cx, obj, wxjsParent->GetObject());
        }

	    PasswordEntryDialog *p = new PasswordEntryDialog(cx, obj, parent, message, 
																 caption, defaultValue, style, *pt);
        return p;
    }

    return NULL;
}

void PasswordEntryDialog::Destruct(JSContext *cx, wxPasswordEntryDialog *p)
{
}

BEGIN_EVENT_TABLE(PasswordEntryDialog, wxPasswordEntryDialog)
END_EVENT_TABLE()
