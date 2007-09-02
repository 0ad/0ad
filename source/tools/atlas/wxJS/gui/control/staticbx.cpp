#include "precompiled.h"

/*
 * wxJavaScript - staticbx.cpp
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
 * $Id: staticbx.cpp 810 2007-07-13 20:07:05Z fbraem $
 */

#include <wx/wx.h>

#include "../../common/main.h"
#include "../../ext/wxjs_ext.h"

#include "staticbx.h"
#include "window.h"

#include "../misc/size.h"
#include "../errors.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>control/staticbox</file>
 * <module>gui</module>
 * <class name="wxStaticBox" prototype="@wxControl">
 *  A static box is a rectangle drawn around other panel items to denote a 
 *  logical grouping of items.
 * </class>
 */
WXJS_INIT_CLASS(StaticBox, "wxStaticBox", 3)

bool StaticBox::AddProperty(wxStaticBox *p, 
                            JSContext* WXUNUSED(cx), 
                            JSObject* WXUNUSED(obj), 
                            const wxString &prop, 
                            jsval* WXUNUSED(vp))
{
  WindowEventHandler::ConnectEvent(p, prop, true);
  return true;
}

bool StaticBox::DeleteProperty(wxStaticBox *p, 
                               JSContext* WXUNUSED(cx), 
                               JSObject* WXUNUSED(obj), 
                               const wxString &prop)
{
  WindowEventHandler::ConnectEvent(p, prop, false);
  return true;
}

/***
 * <ctor>
 *  <function />
 *  <function>
 *   <arg name="Parent" type="@wxWindow">
 *    The parent of the staticbox
 *   </arg>
 *   <arg name="Id" type="Integer">
 *    The unique identifier.
 *   </arg>
 *   <arg name="Text" type="String">
 *    The text of the staticbox
 *   </arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *    The position of the staticbox.
 *   </arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *    The size of the staticBox.
 *   </arg>
 *   <arg name="Style" type="Integer" default="0">
 *    The style of the staticbox.
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxStaticBox object
 *  </desc>
 * </ctor>
 */
wxStaticBox* StaticBox::Construct(JSContext *cx,
                                  JSObject *obj,
                                  uintN argc,
                                  jsval *argv,
                                  bool WXUNUSED(constructing))
{
  wxStaticBox *p = new wxStaticBox();
  SetPrivate(cx, obj, p);

  if ( argc > 0 )
  {
    jsval rval;
    if ( ! create(cx, obj, argc, argv, &rval) )
      return NULL;
  }
  return p;
}

WXJS_BEGIN_METHOD_MAP(StaticBox)
  WXJS_METHOD("create", create, 3)
WXJS_END_METHOD_MAP()

/***
 * <method>
 *  <function returns="Boolean">
 *   <arg name="Parent" type="@wxWindow">
 *    The parent of the staticbox
 *   </arg>
 *   <arg name="Id" type="Integer">
 *    The unique identifier.
 *   </arg>
 *   <arg name="Text" type="String">
 *    The text of the staticbox
 *   </arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *    The position of the staticbox.
 *   </arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *    The size of the staticBox.
 *   </arg>
 *   <arg name="Style" type="Integer" default="0">
 *    The style of the staticbox.
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxStaticBox object
 *  </desc>
 * </method>
 */
JSBool StaticBox::create(JSContext *cx,
                         JSObject *obj,
                         uintN argc,
                         jsval *argv,
                         jsval *rval)
{
  wxStaticBox *p = GetPrivate(cx, obj);
  *rval = JSVAL_FALSE;

  if ( argc > 6 )
    argc = 6;

  const wxPoint *pt = &wxDefaultPosition;
  const wxSize *size = &wxDefaultSize;
  int style = 0;

  switch(argc)
  {
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

    if ( p->Create(parent, id, text, *pt, *size, style) )
    {
      *rval = JSVAL_TRUE;
      p->SetClientObject(new JavaScriptClientData(cx, obj, true, false));
    }
  }
  return JS_TRUE;
}
