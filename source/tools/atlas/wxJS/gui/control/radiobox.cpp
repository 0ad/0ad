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
 * $Id: radiobox.cpp 746 2007-06-11 20:58:21Z fbraem $
 */
// radiobox.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/index.h"


#include "../event/jsevent.h"
#include "../event/command.h"

#include "../misc/size.h"
#include "../misc/point.h"
#include "../misc/validate.h"

#include "radiobox.h"
#include "radioboxit.h"
#include "window.h"
#include "../errors.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>control/radiobox</file>
 * <module>gui</module>
 * <class name="wxRadioBox" prototype="@wxControl">
 *  A radio box item is used to select one of number of mutually exclusive choices. 
 *  It is displayed as a vertical column or horizontal row of labelled buttons.
 * </class>
 */
WXJS_INIT_CLASS(RadioBox, "wxRadioBox", 3)

void RadioBox::InitClass(JSContext* WXUNUSED(cx),
                         JSObject* WXUNUSED(obj), 
                         JSObject* WXUNUSED(proto))
{
  RadioBoxEventHandler::InitConnectEventMap();
}

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

bool RadioBox::GetProperty(wxRadioBox* p,
                           JSContext* cx,
                           JSObject* WXUNUSED(obj),
                           int id,
                           jsval* vp)
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

bool RadioBox::SetProperty(wxRadioBox* p,
                           JSContext* cx,
                           JSObject* WXUNUSED(obj),
                           int id,
                           jsval* vp)
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

bool RadioBox::AddProperty(wxRadioBox *p, 
                           JSContext* WXUNUSED(cx), 
                           JSObject* WXUNUSED(obj), 
                           const wxString &prop, 
                           jsval* WXUNUSED(vp))
{
    if ( WindowEventHandler::ConnectEvent(p, prop, true) )
        return true;
    
    RadioBoxEventHandler::ConnectEvent(p, prop, true);

    return true;
}

bool RadioBox::DeleteProperty(wxRadioBox *p, 
                              JSContext* WXUNUSED(cx), 
                              JSObject* WXUNUSED(obj), 
                              const wxString &prop)
{
  if ( WindowEventHandler::ConnectEvent(p, prop, false) )
    return true;
  
  RadioBoxEventHandler::ConnectEvent(p, prop, false);
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
 *  <function />
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
wxRadioBox* RadioBox::Construct(JSContext* cx,
                                JSObject* obj, 
                                uintN argc, 
                                jsval* argv, 
                                bool WXUNUSED(constructing))
{
  wxRadioBox *p = new wxRadioBox();
  SetPrivate(cx, obj, p);

  if ( argc > 0 )
  {
    jsval rval;
    if ( ! create(cx, obj, argc, argv, &rval) )
      return NULL;
  }
  return p;
}

WXJS_BEGIN_METHOD_MAP(RadioBox)
	WXJS_METHOD("create", create, 3)
	WXJS_METHOD("setString", setString, 2)
	WXJS_METHOD("findString", setString, 2)
	WXJS_METHOD("enable", enable, 2)
	WXJS_METHOD("show", show, 2)
WXJS_END_METHOD_MAP()

/***
 * <method name="create">
 *  <function returns="Boolean">
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
 * </method>
 */
JSBool RadioBox::create(JSContext *cx,
                        JSObject *obj,
                        uintN argc,
                        jsval *argv,
                        jsval *rval)
{
  wxRadioBox *p = GetPrivate(cx, obj);
  *rval = JSVAL_FALSE;

  if ( argc > 9 )
    argc = 9;

  int style = wxRA_SPECIFY_COLS;
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
    {
      JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 9, "wxValidator");
      return JS_FALSE;
    }
  case 8:
    if ( ! FromJS(cx, argv[7], style) )
    {
      JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 8, "Integer");
      return JS_FALSE;
    }
    // Fall through
  case 7:
    if ( ! FromJS(cx, argv[6], max) )
    {
      JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 7, "Integer");
      return JS_FALSE;
    }
    // Fall through
  case 6:
    if ( ! FromJS(cx, argv[5], items) )
    {
      JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 6, "String Array");
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
    wxString title;
    FromJS(cx, argv[2], title);

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
    if ( p->Create(parent, id, title, *pt, *size, 
                   items.GetCount(), items.GetStrings(), max, style, *val) )
    {
      *rval = JSVAL_TRUE;
      p->SetClientObject(new JavaScriptClientData(cx, obj, true, false));
    }
  }

  return JS_TRUE;
}

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
JSBool RadioBox::setString(JSContext* cx,
                           JSObject* obj,
                           uintN WXUNUSED(argc),
                           jsval* argv,
                           jsval* WXUNUSED(rval))
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
JSBool RadioBox::findString(JSContext *cx,
                            JSObject *obj,
                            uintN WXUNUSED(argc),
                            jsval *argv,
                            jsval *rval)
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
JSBool RadioBox::enable(JSContext* cx,
                        JSObject* obj,
                        uintN argc,
                        jsval* argv,
                        jsval* WXUNUSED(rval))
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
JSBool RadioBox::show(JSContext* cx,
                      JSObject* obj,
                      uintN argc,
                      jsval* argv,
                      jsval* WXUNUSED(rval))
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
 *	 Called when a radio button is clicked. The type of the argument that your
 *   handler receives is @wxCommandEvent.
 *  </event>
 * </events>
 */
WXJS_INIT_EVENT_MAP(wxRadioBox)
const wxString WXJS_RADIOBOX_EVENT = wxT("onRadioBox");

void RadioBoxEventHandler::OnRadioBox(wxCommandEvent &event)
{
	PrivCommandEvent::Fire<CommandEvent>(event, WXJS_RADIOBOX_EVENT);
}

void RadioBoxEventHandler::ConnectRadioBox(wxRadioBox *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED, 
               wxCommandEventHandler(RadioBoxEventHandler::OnRadioBox));
  }
  else
  {
    p->Disconnect(wxEVT_COMMAND_RADIOBOX_SELECTED, 
                  wxCommandEventHandler(RadioBoxEventHandler::OnRadioBox));
  }
}

void RadioBoxEventHandler::InitConnectEventMap()
{
    AddConnector(WXJS_RADIOBOX_EVENT, ConnectRadioBox);
}
