#include "precompiled.h"

/*
 * wxJavaScript - checkbox.cpp
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
 * $Id: checkbox.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// checkbox.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "../event/evthand.h"
#include "../event/jsevent.h"
#include "../event/command.h"

#include "checkbox.h"
#include "window.h"

#include "../misc/point.h"
#include "../misc/size.h"
#include "../misc/validate.h"

using namespace wxjs;
using namespace wxjs::gui;

CheckBox::CheckBox(JSContext *cx, JSObject *obj)
		:   wxCheckBox()
		  , Object(obj, cx)
{
  PushEventHandler(new EventHandler(this));
}

CheckBox::~CheckBox()
{
  PopEventHandler(true);
}

/***
 * <file>control/checkbox</file>
 * <module>gui</module>
 * <class name="wxCheckBox" prototype="@wxControl">
 *  A checkbox is a labeled box which is either on 
 *  (checkmark is visible) or off (no checkmark).
 *  <br />An example:
 *  <pre><code class="whjs">
 *   // dlg is a wxDialog
 *   var chkbox = new wxCheckBox(dlg, -1, "Check me");
 *   chkbox.onCheckBox = function(event)
 *   {
 *     if ( event.checked )
 *       wxMessageBox("Checked");
 *     else
 *       wxMessageBox("Unchecked");
 *   }
 *  </code></pre>
 * </class>
 */
WXJS_INIT_CLASS(CheckBox, "wxCheckBox", 3)

/***
 * <properties>
 *  <property name="value" type="Boolean">
 *   Checks/Unchecks the checkbox.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(CheckBox)
  WXJS_PROPERTY(P_VALUE, "value")
WXJS_END_PROPERTY_MAP()

bool CheckBox::GetProperty(wxCheckBox *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    if ( id == P_VALUE )
	{
		*vp = ToJS(cx, p->GetValue());
    }
    return true;
}

bool CheckBox::SetProperty(wxCheckBox *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    if (id == P_VALUE )
	{
		bool value;
		if ( FromJS(cx, *vp, value) )
			p->SetValue(value);
    }
    return true;
}
/***
 * <ctor>
 *  <function>
 *   <arg name="Parent" type="@wxWindow">The parent of the checkbox</arg>
 *   <arg name="Id" type="Integer">A window identifier. Use -1 when you don't need it</arg>
 *   <arg name="Text" type="String">The label of the checkbox</arg>
 *   <arg name="Pos" type="@wxPoint" default="wxDefaultPosition">The position of the checkbox on the given parent</arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">The size of the checkbox</arg>
 *   <arg name="Style" type="Integer" default="0">The style of the checkbox</arg>
 *   <arg name="Validator" type="@wxValidator" default="null">A validator</arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxCheckBox object.
 *  </desc>
 * </ctor>
 */
wxCheckBox *CheckBox::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
	const wxPoint *pt = &wxDefaultPosition;
	const wxSize *size = &wxDefaultSize;
    int style = 0;
    const wxValidator *val = &wxDefaultValidator;

    if ( argc > 7 )
        argc = 7;

    switch(argc)
    {
    case 7:
        val = Validator::GetPrivate(cx, argv[6]);
        if ( val == NULL )
            break;
    case 6:
        if ( ! FromJS(cx, argv[5], style) )
            break;
        // Fall through
    case 5:
		size = Size::GetPrivate(cx, argv[4]);
		if ( size == NULL )
			break;
		// Fall through
	case 4:
		pt = Point::GetPrivate(cx, argv[3]);
		if ( pt == NULL )
			break;
		// Fall through
    default:
        wxString text;
        FromJS(cx, argv[2], text);

        int id;
        if ( ! FromJS(cx, argv[1], id) )
            break;

        wxWindow *parent = Window::GetPrivate(cx, argv[0]);
		if ( parent == NULL )
            break;

        Object *wxjsParent = dynamic_cast<Object *>(parent);
        JS_SetParent(cx, obj, wxjsParent->GetObject());

	    CheckBox *p = new CheckBox(cx, obj);
	    p->Create(parent, id, text, *pt, *size, style, *val);
        return p;
    }

    return NULL;
}

/***
 * <events>
 *  <event name="onCheckBox">
 *   Called when the checkbox is clicked. The type of the argument that your handler receives 
 *   is @wxCommandEvent.
 *  </event>
 * </events>
 */
void CheckBox::OnCheckBox(wxCommandEvent &event)
{
	PrivCommandEvent::Fire<CommandEvent>(this, event, "onCheckBox");
}

BEGIN_EVENT_TABLE(CheckBox, wxCheckBox)
  EVT_CHECKBOX(-1, CheckBox::OnCheckBox)
END_EVENT_TABLE()
