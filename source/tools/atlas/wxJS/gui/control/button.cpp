#include "precompiled.h"

/*
 * wxJavaScript - button.cpp
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
 * $Id: button.cpp 810 2007-07-13 20:07:05Z fbraem $
 */

/***
 * <file>control/button</file>
 * <module>gui</module>
 * <class name="wxButton" prototype="@wxControl">
 *	A button is a control that contains a text string,
 *	and is one of the commonest elements of a GUI. It may 
 *	be placed on a dialog box or panel, or indeed almost any other window.
 *  An example:
 *  <pre><code class="whjs">// dlg is a wxDialog
 *  var button = new wxButton(dlg, -1, "Click me");
 *  button.onClicked = function(event)
 *  {
 *    wxMessageBox("You've clicked me");
 *  }</code></pre><br />
 * </class>
 */

#ifndef WX_PRECOMP
  #include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../ext/wxjs_ext.h"

#include "../event/jsevent.h"
#include "../event/command.h"

#include "../misc/size.h"
#include "../misc/validate.h"

#include "button.h"
#include "window.h"
#include "../errors.h"

using namespace wxjs;
using namespace wxjs::gui;

WXJS_INIT_CLASS(Button, "wxButton", 3)

void Button::InitClass(JSContext* WXUNUSED(cx), 
                       JSObject* WXUNUSED(obj), 
                       JSObject* WXUNUSED(proto))
{
    ButtonEventHandler::InitConnectEventMap();
}

/***
 * <properties>
 *  <property name="label" type="String">
 *   Get/Set the label of the button.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(Button)
  WXJS_PROPERTY(P_LABEL, "label")
WXJS_END_PROPERTY_MAP()

bool Button::GetProperty(wxButton *p, 
                         JSContext *cx, 
                         JSObject* WXUNUSED(obj), 
                         int id, 
                         jsval *vp)
{
  if ( p == NULL )
    return false;

  if ( id == P_LABEL )
    *vp = ToJS(cx, p->GetLabel());

  return true;
}

bool Button::SetProperty(wxButton *p, 
                         JSContext *cx, 
                         JSObject* WXUNUSED(obj), 
                         int id, 
                         jsval *vp)
{
  if ( p == NULL )
    return false;

  if ( id == P_LABEL )
  {
    wxString label;
    FromJS(cx, *vp, label);
    p->SetLabel(label);
  }
  return true;
}

bool Button::AddProperty(wxButton *p, 
                         JSContext* WXUNUSED(cx), 
                         JSObject* WXUNUSED(obj), 
                         const wxString &prop, 
                         jsval* WXUNUSED(vp))
{
  if ( WindowEventHandler::ConnectEvent(p, prop, true) )
    return true;
  
  ButtonEventHandler::ConnectEvent(p, prop, true);
  return true;
}

bool Button::DeleteProperty(wxButton *p, 
                            JSContext* WXUNUSED(cx), 
                            JSObject* WXUNUSED(obj), 
                            const wxString &prop)
{
  if ( WindowEventHandler::ConnectEvent(p, prop, false) )
    return true;
  
  ButtonEventHandler::ConnectEvent(p, prop, false);
  return true;
}

/***
 * <class_properties>
 *  <property name="defaultSize" type="Integer" readonly="Y">
 *    Gets the default size of a button.
 *  </property>
 * </class_properties>
 */
WXJS_BEGIN_STATIC_PROPERTY_MAP(Button)
  WXJS_READONLY_STATIC_PROPERTY(P_DEFAULT_SIZE, "defaultSize")
WXJS_END_PROPERTY_MAP()

bool Button::GetStaticProperty(JSContext *cx, int id, jsval *vp)
{
  if ( id == P_DEFAULT_SIZE )
  {
    *vp = Size::CreateObject(cx, new wxSize(wxButton::GetDefaultSize()));
  }
  return true;
}

/***
 * <constants>
 *  <type name="Style">
 *   <constant name="LEFT">
 *    Left-justifies the label. Windows and GTK+ only.
 *   </constant>
 *   <constant name="RIGHT">
 *    Right-justifies the bitmap label. Windows and GTK+ only.
 *   </constant>
 *   <constant name="TOP">
 *    Aligns the label to the top of the button. Windows and GTK+ only.
 *   </constant>
 *   <constant name="BOTTOM">
 *    Aligns the label to the bottom of the button. Windows and GTK+ only.
 *   </constant>
 *   <constant name="EXACTFIT">
 *    Creates the button as small as possible instead of making it of the 
 *    standard size (which is the default behaviour ).
 *   </constant>
 *   <constant name="NO_BORDER">
 *    Creates a flat button. Windows and GTK+ only.
 *   </constant>
 *   <constant name="AUTODRAW">
 *    If this is specified, the button will be drawn automatically using 
 *    the label bitmap only, providing a 3D-look border. If this style is not 
 *    specified, 
 *    the button will be drawn without borders and using all provided bitmaps. 
 *    WIN32 only.
 *   </constant>
 *  </type>
 * </constants>
 */
WXJS_BEGIN_CONSTANT_MAP(Button)
  WXJS_CONSTANT(wxBU_, LEFT)
  WXJS_CONSTANT(wxBU_, RIGHT)
  WXJS_CONSTANT(wxBU_, TOP)
  WXJS_CONSTANT(wxBU_, BOTTOM)
  WXJS_CONSTANT(wxBU_, EXACTFIT)
  WXJS_CONSTANT(wxBU_, AUTODRAW)
  WXJS_CONSTANT(wx, NO_BORDER)
WXJS_END_CONSTANT_MAP()

/***
 * <ctor>
 *  <function />
 *  <function>
 *   <arg name="Parent" type="@wxWindow">The parent of the button.</arg>
 *   <arg name="Id" type="Integer">
 *    An window identifier. Use -1 when you don't need it.
 *   </arg>
 *   <arg name="Text" type="String">The label of the button</arg>
 *   <arg name="Pos" type="@wxPoint" default="wxDefaultPosition">
 *    The position of the button on the given parent.
 *   </arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *    The size of the button.
 *   </arg>
 *   <arg name="Style" type="Integer" default="wxButton.AUTODRAW">
 *    The button style
 *   </arg>
 *   <arg name="Validator" type="@wxValidator" default="wxDefaultValidator" />
 *  </function>
 *  <desc>
 *   Constructs a new wxButton object.
 *  </desc>
 * </ctor>
 */
wxButton *Button::Construct(JSContext *cx, 
                            JSObject *obj, 
                            uintN argc, 
                            jsval *argv, 
                            bool WXUNUSED(constructing))
{
  wxButton *p = new wxButton();
  SetPrivate(cx, obj, p);

  if ( argc > 0 )
  {
    jsval rval;
    if ( ! create(cx, obj, argc, argv, &rval) )
    {
      return NULL;
    }
  }
  return p;
}

WXJS_BEGIN_METHOD_MAP(Button)
  WXJS_METHOD("create", create, 3)
  WXJS_METHOD("setDefault", setDefault, 0)
WXJS_END_METHOD_MAP()

/***
 * <method name="create">
 *  <function returns="Boolean">
 *   <arg name="Parent" type="@wxWindow">The parent of the button.</arg>
 *   <arg name="Id" type="Integer">
 *    An window identifier. Use -1 when you don't need it.
 *   </arg>
 *   <arg name="Text" type="String">The label of the button</arg>
 *   <arg name="Pos" type="@wxPoint" default="wxDefaultPosition">
 *    The position of the button on the given parent.
 *   </arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *    The size of the button.
 *   </arg>
 *   <arg name="Style" type="Integer" default="wxButton.AUTODRAW">
 *    The button style
 *   </arg>
 *   <arg name="Validator" type="@wxValidator" default="wxDefaultValidator" />
 *  </function>
 *  <desc>
 *   Creates a button.
 *  </desc>
 * </method>
 */
JSBool Button::create(JSContext *cx,
                      JSObject *obj,
                      uintN argc,
                      jsval *argv,
                      jsval *rval)
{
  wxButton *p = GetPrivate(cx, obj);
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
  case 5:
    size = Size::GetPrivate(cx, argv[4]);
    if ( size == NULL )
    {
      JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 5, "wxSize");
      return JS_FALSE;
    }
  case 4:
    pt = wxjs::ext::GetPoint(cx, argv[3]);
    if ( pt == NULL )
    {
      JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 4, "wxPoint");
      return JS_FALSE;
    }
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

    JavaScriptClientData *clntParent 
              = dynamic_cast<JavaScriptClientData *>(parent->GetClientObject());
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
 * <method name="setDefault">
 *  <function />
 *  <desc>
 *   This sets the button to be the default item for the panel or dialog box.
 *   see @wxPanel#defaultItem.
 *  </desc>
 * </method>
 */
JSBool Button::setDefault(JSContext *cx, 
                          JSObject *obj, 
                          uintN WXUNUSED(argc), 
                          jsval* WXUNUSED(argv), 
                          jsval* WXUNUSED(rval))
{
  wxButton *p = GetPrivate(cx, obj);
  if ( p == NULL )
    return JS_FALSE;

  p->SetDefault();

  return JS_TRUE;
}

/***
 * <events>
 *  <event name="onClicked">
 *   Called when the button is clicked. The type of the argument that your
*    handler receives is @wxCommandEvent.
 *  </event>
 * </events>
 */
WXJS_INIT_EVENT_MAP(wxButton)
const wxString WXJS_BUTTON_CLICKED_EVENT = wxT("onClicked");

void ButtonEventHandler::OnClicked(wxCommandEvent &event)
{
  PrivCommandEvent::Fire<CommandEvent>(event, WXJS_BUTTON_CLICKED_EVENT);
}

void ButtonEventHandler::ConnectClicked(wxButton *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ButtonEventHandler::OnClicked));
  }
  else
  {
    p->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, 
                  wxCommandEventHandler(ButtonEventHandler::OnClicked));
  }
}

void ButtonEventHandler::InitConnectEventMap()
{
    AddConnector(WXJS_BUTTON_CLICKED_EVENT, ConnectClicked);
}
