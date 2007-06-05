#include "precompiled.h"

/*
 * wxJavaScript - radiobox.cpp
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
 * $Id: radiobox.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// radiobox.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/index.h"

#include "../event/evthand.h"
#include "../event/jsevent.h"
#include "../event/command.h"

#include "../misc/size.h"
#include "../misc/point.h"
#include "../misc/validate.h"

#include "radiobox.h"
#include "radioboxit.h"
#include "window.h"

using namespace wxjs;
using namespace wxjs::gui;

RadioBox::RadioBox(JSContext *cx, JSObject *obj)
	: wxRadioBox() 
	, Object(obj, cx)
{
	PushEventHandler(new EventHandler(this));
}

RadioBox::~RadioBox()
{
	PopEventHandler(true);
}

/***
 * <file>control/radiobox</file>
 * <module>gui</module>
 * <class name="wxRadioBox" prototype="@wxControl">
 *  A radio box item is used to select one of number of mutually exclusive choices. 
 *  It is displayed as a vertical column or horizontal row of labelled buttons.
 * </class>
 */
WXJS_INIT_CLASS(RadioBox, "wxRadioBox", 3)

/***
 * <properties>
 *  <property name="count" type="Integer" readonly="Y">
 *   Get the number of items
 *  </property>
 *  <property name="item" type="Array" readonly="Y">
 *   Get an array of with @wxRadioBoxItem items.
 *  </property>
 *  <property name="selection" type="Integer">
 *   Get/Set the selected button. (zero-indexed)
 *  </property>
 *  <property name="stringSelection" type="String">
 *   Get/Set the selected string.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(RadioBox)
	WXJS_PROPERTY(P_SELECTION, "selection")
	WXJS_PROPERTY(P_STRING_SELECTION, "stringSelection")
	WXJS_READONLY_PROPERTY(P_COUNT, "count")
	WXJS_READONLY_PROPERTY(P_ITEM, "item")
WXJS_END_PROPERTY_MAP()

bool RadioBox::GetProperty(wxRadioBox *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch (id) 
	{
	case P_COUNT:
		*vp = ToJS(cx, p->GetCount());
		break;
	case P_SELECTION:
		*vp = ToJS(cx, p->GetSelection());
		break;
	case P_STRING_SELECTION:
		*vp = ToJS(cx, p->GetStringSelection());
		break;
	case P_ITEM:
        *vp = RadioBoxItem::CreateObject(cx, new Index(0));
		break;
    }
    return true;
}

bool RadioBox::SetProperty(wxRadioBox *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch (id) 
	{
	case P_SELECTION:
		{
			int value;
			if ( FromJS(cx, *vp, value) )
				p->SetSelection(value);
			break;
		}
	case P_STRING_SELECTION:
		{
			wxString value;
			FromJS(cx, *vp, value);
			p->SetStringSelection(value);
			break;
		}
	}
    return true;
}

/***
 * <constants>
 *  <type name="Styles">
 *   <constant name="SPECIFY_ROWS">The major dimension parameter refers to the maximum number of rows.</constant>
 *   <constant name="SPECIFY_COLS">The major dimension parameter refers to the maximum number of columns.</constant>
 *  </type>
 * </constants>
 */
WXJS_BEGIN_CONSTANT_MAP(RadioBox)
    WXJS_CONSTANT(wxRA_, SPECIFY_ROWS)
	WXJS_CONSTANT(wxRA_, SPECIFY_COLS)
WXJS_END_CONSTANT_MAP()

/***
 * <ctor>
 *  <function>
 *   <arg name="Parent" type="@wxWindow">The parent of wxRadioBox.</arg>
 *   <arg name="Id" type="Integer">A window identifier. Use -1 when you don't need it.</arg>
 *   <arg name="Title" type="String">The title of the radiobox.</arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *    The position of the RadioBox control on the given parent.
 *   </arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *    The size of the RadioBox control.
 *   </arg>
 *   <arg name="Items" type="Array" default="null">
 *    An array of Strings to initialize the control.
 *   </arg>
 *   <arg name="MaximumDimension" type="Integer" default="0">
 *    Specifies the maximum number of rows (if style contains SPECIFY_ROWS) or columns
 *    (if style contains SPECIFY_COLS) for a two-dimensional radiobox.
 *   </arg>
 *   <arg name="Style" type="Integer" default="SPECIFY_COLS">
 *    The wxRadioBox style.
 *   </arg>
 *   <arg name="Validator" type="@wxValidator" default="null" />
 *  </function>
 *  <desc>
 *   Constructs a new wxRadioBox object.
 *  </desc>
 * </ctor>
 */
wxRadioBox* RadioBox::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    if ( argc > 9 )
        argc = 9;

    int style = 0;
    int max = 0;
    StringsPtr items;
    const wxSize *size = &wxDefaultSize;
    const wxPoint *pt = &wxDefaultPosition;
    const wxValidator *val = &wxDefaultValidator;

    switch(argc)
    {
    case 9:
        val = Validator::GetPrivate(cx, argv[8]);
        if ( val == NULL )
            break;
    case 8:
        if ( ! FromJS(cx, argv[7], style) )
            break;
        // Fall through
    case 7:
        if ( ! FromJS(cx, argv[6], max) )
            break;
        // Fall through
    case 6:
        if ( ! FromJS(cx, argv[5], items) )
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
        wxString title;
        FromJS(cx, argv[2], title);

        int id;
        if ( ! FromJS(cx, argv[1], id) )
            break;

        wxWindow *parent = Window::GetPrivate(cx, argv[0]);
		if ( parent == NULL )
            break;

        Object *wxjsParent = dynamic_cast<Object *>(parent);
        JS_SetParent(cx, obj, wxjsParent->GetObject());

        wxRadioBox *p = new RadioBox(cx, obj);
	    p->Create(parent, id, title, *pt, *size, 
                  items.GetCount(), items.GetStrings(), max, style, *val);
        return p;
    }

	return NULL;
}

WXJS_BEGIN_METHOD_MAP(RadioBox)
	WXJS_METHOD("setString", setString, 2)
	WXJS_METHOD("findString", setString, 2)
	WXJS_METHOD("enable", enable, 2)
	WXJS_METHOD("show", show, 2)
WXJS_END_METHOD_MAP()

/***
 * <method name="setString">
 *  <function>
 *   <arg name="Index" type="Integer">
 *    The zero-based index of a button
 *   </arg> 
 *   <arg name="Label" type="String">
 *    Sets the label of the button.
 *   </arg>
 *  </function>
 *  <desc />
 * </method>
 */
JSBool RadioBox::setString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxRadioBox *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	int idx;
	wxString label;

	if ( FromJS(cx, argv[0], idx) )
	{
		FromJS(cx, argv[1], label);
		p->SetString(idx, label);
	}
	else
	{
		return JS_FALSE;
	}
	return JS_TRUE;
}

/***
 * <method name="findString">
 *  <function returns="Integer">
 *   <arg name="Str" type="String" />
 *  </function>
 *  <desc>
 *   Finds a button matching the given string, returning the position if found, 
 *   or -1 if not found.
 *  </desc>
 * </method>
 */
JSBool RadioBox::findString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxRadioBox *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	wxString label;
	FromJS(cx, argv[0], label);
	*rval = ToJS(cx, p->FindString(label));

	return JS_TRUE;
}

/***
 * <method name="enable">
 *  <function>
 *   <arg name="Switch" type="Boolean" />
 *  </function>
 *  <function>
 *   <arg name="Index" type="Integer">
 *    The zero-based index of a button
 *   </arg> 
 *   <arg name="Switch" type="Boolean" />
 *  </function>
 *  <desc>
 *   Enables/Disables the button at the given index.
 *   See @wxRadioBoxItem @wxRadioBoxItem#enable.
 *  </desc>
 * </method>
 */
JSBool RadioBox::enable(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxRadioBox *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;
	
	if ( argc == 1 ) // The prototype method enable
	{
		bool sw;
		if ( FromJS(cx, argv[0], sw) )
		{
			p->Enable(sw);
			return JS_TRUE;
		}
	}
	else if ( argc == 2 )
	{
		int idx;
		bool sw;
		if (    FromJS(cx, argv[0], idx) 
			 && FromJS(cx, argv[1], sw) )
		{
			p->Enable(idx, sw);
			return JS_TRUE;
		}
	}

	return JS_FALSE;
}

/***
 * <method name="show">
 *  <function>
 *   <arg name="Switch" type="Boolean" />
 *  </function>
 *  <function>
 *   <arg name="Index" type="Integer">
 *    The zero-based index of a button
 *   </arg> 
 *   <arg name="Switch" type="Boolean" />
 *  </function>
 *  <desc>
 *   Shows/Hides the button at the given index.
 *   See @wxRadioBoxItem @wxRadioBoxItem#enable.
 *  </desc>
 * </method>
 */
JSBool RadioBox::show(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxRadioBox *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;
	
	if ( argc == 1 ) // The prototype method enable
	{
		bool sw;
		if ( FromJS(cx, argv[0], sw) )
		{
			p->Show(sw);
			return JS_TRUE;
		}
	}
	else if ( argc == 2 )
	{
		int idx;
		bool sw;
		if (    FromJS(cx, argv[0], idx) 
			 && FromJS(cx, argv[1], sw) )
		{
			p->Show(idx, sw);
			return JS_TRUE;
		}
	}
	
	return JS_FALSE;
}

/***
 * <events>
 *  <event name="onRadioBox">
 *	 Called when a radio button is clicked. The type of the argument that your handler receives 
 *	 is @wxCommandEvent.
 *  </event>
 * </events>
 */
void RadioBox::OnRadioBox(wxCommandEvent &event)
{
	PrivCommandEvent::Fire<CommandEvent>(this, event, "onRadioBox");
}

BEGIN_EVENT_TABLE(RadioBox, wxRadioBox)
	EVT_RADIOBOX(-1, RadioBox::OnRadioBox)
END_EVENT_TABLE()
