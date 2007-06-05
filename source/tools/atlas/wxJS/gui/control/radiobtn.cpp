#include "precompiled.h"

/*
 * wxJavaScript - radiobtn.cpp
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
 * $Id: radiobtn.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// radiobtn.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "../event/evthand.h"
#include "../event/jsevent.h"
#include "../event/command.h"

#include "../misc/size.h"
#include "../misc/point.h"
#include "../misc/validate.h"

#include "radiobtn.h"
#include "window.h"

using namespace wxjs;
using namespace wxjs::gui;

RadioButton::RadioButton(JSContext *cx, JSObject *obj)
	: wxRadioButton() 
	, Object(obj, cx)
{
	PushEventHandler(new EventHandler(this));
}

RadioButton::~RadioButton()
{
	PopEventHandler(true);
}

/***
 * <file>control/radiobtn</file>
 * <module>gui</module>
 * <class name="wxRadioButton" prototype="@wxControl">
 *  A radio button item is a button which usually denotes one of several mutually exclusive 
 *  options. It has a text label next to a (usually) round button.
 *  <br /><br />You can create a group of mutually-exclusive radio buttons by specifying 
 *  wxRadioButton.GROUP for the first in the group. The group ends when another radio button
 *  group is created, or there are no more radio buttons. 
 *  See also @wxRadioBox.
 * </class>
 */
WXJS_INIT_CLASS(RadioButton, "wxRadioButton", 3)

/***
 * <properties>
 *  <property name="value" type="Boolean">
 *   Select/Deselect the button
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(RadioButton)
	WXJS_PROPERTY(P_VALUE, "value")
WXJS_END_PROPERTY_MAP()

bool RadioButton::GetProperty(wxRadioButton *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	if (id == P_VALUE )
	{
		*vp = ToJS(cx, p->GetValue());
    }
    return true;
}

bool RadioButton::SetProperty(wxRadioButton *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	if ( id == P_VALUE )
	{
		bool value; 
		if ( FromJS(cx, *vp, value) )
			p->SetValue(value);
	}
    return true;
}

/***
 * <constants>
 *  <type name="Styles">
 *   <constant name="GROUP">Marks the beginning of a new group of radio buttons.</constant>
 *  </type>
 * </constants>
 */
WXJS_BEGIN_CONSTANT_MAP(RadioButton)
    WXJS_CONSTANT(wxRB_, GROUP)
WXJS_END_CONSTANT_MAP()

/***
 * <ctor>
 *  <function>
 *   <arg name="Parent" type="@wxWindow">
 *    The parent of wxRadioButton.
 *   </arg>
 *   <arg name="Id" type="Integer">
 *    An window identifier. Use -1 when you don't need it.
 *   </arg>
 *   <arg name="Label" type="String">
 *    The title of the RadioButton.
 *   </arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *    The position of the RadioButton control on the given parent.
 *   </arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *    The size of the RadioButton control.
 *   </arg>
 *   <arg name="Style" type="Integer" default="0">
 *    The wxRadioButton style.
 *   </arg>
 *   <arg name="Validator" type="@wxValidator" default="null" />
 *  </function>
 *  <desc>
 *   Constructs a new wxRadioButton object.
 *  </desc>
 * </ctor>
 */
wxRadioButton* RadioButton::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    if ( argc > 7 )
        argc = 7;

	const wxPoint *pt = &wxDefaultPosition;
	const wxSize *size = &wxDefaultSize;
    int style = 0;
    const wxValidator *val = &wxDefaultValidator;

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

	    RadioButton *p = new RadioButton(cx, obj);
	    p->Create(parent, id, text, *pt, *size, style, *val);
        return p;
    }

    return NULL;
}

/***
 * <events>
 *  <event name="onRadioButton">
 *	 Called when a radio button is clicked. The type of the argument that your handler receives 
 *	 is @wxCommandEvent.
 *  </event>
 * </events>
 */
void RadioButton::OnRadioButton(wxCommandEvent &event)
{
	PrivCommandEvent::Fire<CommandEvent>(this, event, "onRadioButton");
}

BEGIN_EVENT_TABLE(RadioButton, wxRadioButton)
	EVT_RADIOBUTTON(-1, RadioButton::OnRadioButton)
END_EVENT_TABLE()
