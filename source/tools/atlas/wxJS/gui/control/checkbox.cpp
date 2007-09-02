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
 * $Id: checkbox.cpp 810 2007-07-13 20:07:05Z fbraem $
 */

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../ext/wxjs_ext.h"

#include "../event/jsevent.h"
#include "../event/command.h"

#include "checkbox.h"
#include "window.h"

#include "../misc/size.h"
#include "../misc/validate.h"
#include "../errors.h"

using namespace wxjs;
using namespace wxjs::gui;

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
void CheckBox::InitClass(JSContext* WXUNUSED(cx),
                         JSObject* WXUNUSED(obj), 
                         JSObject* WXUNUSED(proto))
{
    CheckBoxEventHandler::InitConnectEventMap();
}

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

bool CheckBox::GetProperty(wxCheckBox* p, 
                           JSContext* cx,
                           JSObject* WXUNUSED(obj),
                           int id,
                           jsval* vp)
{
    if ( id == P_VALUE )
	{
		*vp = ToJS(cx, p->GetValue());
    }
    return true;
}

bool CheckBox::SetProperty(wxCheckBox* p,
                           JSContext* cx,
                           JSObject* WXUNUSED(obj),
                           int id,
                           jsval* vp)
{
    if (id == P_VALUE )
	{
		bool value;
		if ( FromJS(cx, *vp, value) )
			p->SetValue(value);
    }
    return true;
}

bool CheckBox::AddProperty(wxCheckBox *p, 
                           JSContext* WXUNUSED(cx), 
                           JSObject* WXUNUSED(obj), 
                           const wxString &prop, 
                           jsval* WXUNUSED(vp))
{
    if ( WindowEventHandler::ConnectEvent(p, prop, true) )
        return true;
    
    CheckBoxEventHandler::ConnectEvent(p, prop, true);
    return true;
}

bool CheckBox::DeleteProperty(wxCheckBox *p, 
                              JSContext* WXUNUSED(cx), 
                              JSObject* WXUNUSED(obj), 
                              const wxString &prop)
{
  if ( WindowEventHandler::ConnectEvent(p, prop, false) )
    return true;
  
  CheckBoxEventHandler::ConnectEvent(p, prop, false);
  return true;
}

/***
 * <ctor>
 *  <function />
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
wxCheckBox *CheckBox::Construct(JSContext *cx,
                                JSObject *obj,
                                uintN argc,
                                jsval *argv,
                                bool WXUNUSED(constructing))
{
  wxCheckBox *p = new wxCheckBox();
  SetPrivate(cx, obj, p);

  if ( argc > 0 )
  {
    jsval rval;
    if ( ! create(cx, obj, argc, argv, &rval) )
      return NULL;
  }
  return p;
}

WXJS_BEGIN_METHOD_MAP(CheckBox)
  WXJS_METHOD("create", create, 3)
WXJS_END_METHOD_MAP()

/***
 * <method name="create">
 *  <function returns="Boolean">
 *   <arg name="Parent" type="@wxWindow">
 *    The parent of the checkbox</arg>
 *   <arg name="Id" type="Integer">
 *    A window identifier. Use -1 when you don't need it
 *   </arg>
 *   <arg name="Text" type="String">The label of the checkbox
 *   </arg>
 *   <arg name="Pos" type="@wxPoint" default="wxDefaultPosition">
 *    The position of the checkbox on the given parent.
 *   </arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *    The size of the checkbox
 *   </arg>
 *   <arg name="Style" type="Integer" default="0">
 *    The style of the checkbox
 *   </arg>
 *   <arg name="Validator" type="@wxValidator" default="null">A validator</arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxCheckBox object.
 *  </desc>
 * </method>
 */
JSBool CheckBox::create(JSContext *cx,
                        JSObject *obj,
                        uintN argc,
                        jsval *argv,
                        jsval *rval)
{
    wxCheckBox *p = GetPrivate(cx, obj);
    *rval = JSVAL_FALSE;

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
    case 5:
		size = Size::GetPrivate(cx, argv[4]);
		if ( size == NULL )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 5, "wxSize");
          return JS_FALSE;
        }
		// Fall through
	case 4:
      pt = wxjs::ext::GetPoint(cx, argv[3]);
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
 *  <event name="onCheckBox">
 *   Called when the checkbox is clicked. The type of the argument that your 
 *   handler receives is @wxCommandEvent.
 *  </event>
 * </events>
 */

WXJS_INIT_EVENT_MAP(wxCheckBox)
const wxString WXJS_CHECKBOX_EVENT = wxT("onCheckBox");

void CheckBoxEventHandler::OnCheckBox(wxCommandEvent &event)
{
	PrivCommandEvent::Fire<CommandEvent>(event, WXJS_CHECKBOX_EVENT);
}


void CheckBoxEventHandler::ConnectCheckBox(wxCheckBox *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, 
               wxCommandEventHandler(CheckBoxEventHandler::OnCheckBox));
  }
  else
  {
    p->Disconnect(wxEVT_COMMAND_CHECKBOX_CLICKED, 
                  wxCommandEventHandler(CheckBoxEventHandler::OnCheckBox));
  }
}

void CheckBoxEventHandler::InitConnectEventMap()
{
    AddConnector(WXJS_CHECKBOX_EVENT, ConnectCheckBox);
}
