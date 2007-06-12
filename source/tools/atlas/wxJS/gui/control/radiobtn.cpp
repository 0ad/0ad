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
 * $Id: radiobtn.cpp 746 2007-06-11 20:58:21Z fbraem $
 */

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"


#include "../event/jsevent.h"
#include "../event/command.h"

#include "../misc/size.h"
#include "../misc/point.h"
#include "../misc/validate.h"

#include "radiobtn.h"
#include "window.h"
#include "../errors.h"

using namespace wxjs;
using namespace wxjs::gui;

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
void RadioButton::InitClass(JSContext* WXUNUSED(cx),
                            JSObject* WXUNUSED(obj), 
                            JSObject* WXUNUSED(proto))
{
  RadioButtonEventHandler::InitConnectEventMap();
}

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

bool RadioButton::GetProperty(wxRadioButton *p,
                              JSContext *cx,
                              JSObject* WXUNUSED(obj),
                              int id,
                              jsval *vp)
{
	if (id == P_VALUE )
	{
		*vp = ToJS(cx, p->GetValue());
    }
    return true;
}

bool RadioButton::SetProperty(wxRadioButton *p,
                              JSContext *cx,
                              JSObject* WXUNUSED(obj),
                              int id,
                              jsval *vp)
{
	if ( id == P_VALUE )
	{
		bool value; 
		if ( FromJS(cx, *vp, value) )
			p->SetValue(value);
	}
    return true;
}

bool RadioButton::AddProperty(wxRadioButton *p, 
                              JSContext* WXUNUSED(cx), 
                              JSObject* WXUNUSED(obj), 
                              const wxString &prop, 
                              jsval* WXUNUSED(vp))
{
    if ( WindowEventHandler::ConnectEvent(p, prop, true) )
        return true;
    
    RadioButtonEventHandler::ConnectEvent(p, prop, true);

    return true;
}

bool RadioButton::DeleteProperty(wxRadioButton *p, 
                                 JSContext* WXUNUSED(cx), 
                                 JSObject* WXUNUSED(obj), 
                                 const wxString &prop)
{
  if ( WindowEventHandler::ConnectEvent(p, prop, false) )
    return true;
  
  RadioButtonEventHandler::ConnectEvent(p, prop, false);
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
 *  <function returns="Boolean" />
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
wxRadioButton* RadioButton::Construct(JSContext *cx,
                                      JSObject *obj,
                                      uintN argc,
                                      jsval *argv,
                                      bool WXUNUSED(constructing))
{
  wxRadioButton *p = new wxRadioButton();
  SetPrivate(cx, obj, p);

  if ( argc > 0 )
  {
    jsval rval;
    if ( ! create(cx, obj, argc, argv, &rval) )
      return NULL;
  }
  return p;
}

WXJS_BEGIN_METHOD_MAP(RadioButton)
  WXJS_METHOD("create", create, 3)
WXJS_END_METHOD_MAP()

JSBool RadioButton::create(JSContext *cx,
                           JSObject *obj,
                           uintN argc,
                           jsval *argv,
                           jsval *rval)
{
  wxRadioButton *p = GetPrivate(cx, obj);
  *rval = JSVAL_FALSE;

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
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 7, "wxValidator");
          return JS_FALSE;
        }
    case 6:
        if ( ! FromJS(cx, argv[5], style) )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 6, "Integer");
          return JS_FALSE;
        }
        // Fall through
    case 5:
		size = Size::GetPrivate(cx, argv[4]);
		if ( size == NULL )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 5, "wxSize");
          return JS_FALSE;
        }
		// Fall through
	case 4:
		pt = Point::GetPrivate(cx, argv[3]);
		if ( pt == NULL )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 4, "wxPoint");
          return JS_FALSE;
        }
		// Fall through
    default:
        wxString text;
        FromJS(cx, argv[2], text);

        int id;
        if ( ! FromJS(cx, argv[1], id) )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 2, "Integer");
          return JS_FALSE;
        }

        wxWindow *parent = Window::GetPrivate(cx, argv[0]);
        if ( parent == NULL )
        {
            JS_ReportError(cx, WXJS_NO_PARENT_ERROR, GetClass()->name);
            return JS_FALSE;
        }
        JavaScriptClientData *clntParent =
              dynamic_cast<JavaScriptClientData *>(parent->GetClientObject());
        if ( clntParent == NULL )
        {
            JS_ReportError(cx, WXJS_NO_PARENT_ERROR, GetClass()->name);
            return JS_FALSE;
        }
        JS_SetParent(cx, obj, clntParent->GetObject());

	    if ( p->Create(parent, id, text, *pt, *size, style, *val) )
        {
          *rval = JSVAL_TRUE;
          p->SetClientObject(new JavaScriptClientData(cx, obj, true, false));
        }
    }

    return JS_TRUE;
}

/***
 * <events>
 *  <event name="onRadioButton">
 *	 Called when a radio button is clicked. The type of the argument that
 *   your handler receives is @wxCommandEvent.
 *  </event>
 * </events>
 */
WXJS_INIT_EVENT_MAP(wxRadioButton)
const wxString WXJS_RADIOBUTTON_EVENT = wxT("onRadioButton");

void RadioButtonEventHandler::OnRadioButton(wxCommandEvent &event)
{
	PrivCommandEvent::Fire<CommandEvent>(event, WXJS_RADIOBUTTON_EVENT);
}

void RadioButtonEventHandler::ConnectRadioButton(wxRadioButton *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_COMMAND_RADIOBUTTON_SELECTED,
               wxCommandEventHandler(OnRadioButton));
  }
  else
  {
    p->Disconnect(wxEVT_COMMAND_RADIOBUTTON_SELECTED, 
                  wxCommandEventHandler(OnRadioButton));
  }
}

void RadioButtonEventHandler::InitConnectEventMap()
{
    AddConnector(WXJS_RADIOBUTTON_EVENT, ConnectRadioButton);
}
