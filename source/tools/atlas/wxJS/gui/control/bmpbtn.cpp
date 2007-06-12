#include "precompiled.h"

/*
 * wxJavaScript - bmpbtn.cpp
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
 * $Id: bmpbtn.cpp 746 2007-06-11 20:58:21Z fbraem $
 */
// bmpbtn.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

/***
 * <file>control/bmpbtn</file>
 * <module>gui</module>
 * <class name="wxBitmapButton" prototype="@wxButton">
 *  A button that contains a bitmap.
 * </class>
 */

#include "../../common/main.h"

#include "../misc/size.h"
#include "../misc/point.h"
#include "bmpbtn.h"
#include "../misc/bitmap.h"
#include "button.h"
#include "window.h"
#include "../errors.h"

using namespace wxjs;
using namespace wxjs::gui;

WXJS_INIT_CLASS(BitmapButton, "wxBitmapButton", 3)

/***
 * <properties>
 *  <property name="bitmapDisabled" type="@wxBitmap">Bitmap to show when the button is disabled.</property>
 *  <property name="bitmapFocus" type="@wxBitmap">Bitmap to show when the button has the focus.</property>
 *  <property name="bitmapLabel" type="@wxBitmap">The default bitmap.</property>
 *  <property name="bitmapSelected" type="@wxBitmap">Bitmap to show when the button is selected.</property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(BitmapButton)
  WXJS_PROPERTY(P_BITMAP_DISABLED, "bitmapDisabled")
  WXJS_PROPERTY(P_BITMAP_FOCUS, "bitmapFocus")
  WXJS_PROPERTY(P_BITMAP_LABEL, "bitmapLabel")
  WXJS_PROPERTY(P_BITMAP_SELECTED, "bitmapSelected")
WXJS_END_PROPERTY_MAP()

bool BitmapButton::GetProperty(wxBitmapButton* p, 
                               JSContext* cx, 
                               JSObject* WXUNUSED(obj), 
                               int id, 
                               jsval* vp)
{
  switch (id) 
  {
  case P_BITMAP_DISABLED:
    *vp = Bitmap::CreateObject(cx, new wxBitmap(p->GetBitmapDisabled()));
    break;
  case P_BITMAP_FOCUS:
    *vp = Bitmap::CreateObject(cx, new wxBitmap(p->GetBitmapFocus()));
    break;
  case P_BITMAP_LABEL:
    *vp = Bitmap::CreateObject(cx, new wxBitmap(p->GetBitmapLabel()));
    break;
  case P_BITMAP_SELECTED:
    *vp = Bitmap::CreateObject(cx, new wxBitmap(p->GetBitmapSelected()));
    break;
  }
  return true;
}

bool BitmapButton::SetProperty(wxBitmapButton *p, 
                               JSContext *cx, 
                               JSObject* WXUNUSED(obj), 
                               int id, 
                               jsval *vp)
{
  switch (id) 
  {
  case P_BITMAP_DISABLED:
    {
      wxBitmap *bitmap = Bitmap::GetPrivate(cx, *vp);
      if ( bitmap != NULL )
          p->SetBitmapDisabled(*bitmap);
      break;
    }
  case P_BITMAP_FOCUS:
    {
      wxBitmap *bitmap = Bitmap::GetPrivate(cx, *vp);
      if ( bitmap != NULL )
          p->SetBitmapFocus(*bitmap);
      break;
    }
  case P_BITMAP_LABEL:
    {
      wxBitmap *bitmap = Bitmap::GetPrivate(cx, *vp);
      if ( bitmap != NULL )
          p->SetBitmapLabel(*bitmap);
      break;
    }
  case P_BITMAP_SELECTED:
    {
      wxBitmap *bitmap = Bitmap::GetPrivate(cx, *vp);
      if ( bitmap != NULL )
          p->SetBitmapSelected(*bitmap);
      break;
    }
  }
  return true;
}

bool BitmapButton::AddProperty(wxBitmapButton *p, 
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

bool BitmapButton::DeleteProperty(wxBitmapButton *p, 
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
 * <ctor>
 *  <function />
 *  <function>
 *   <arg name="Parent" type="@wxWindow">The parent window</arg>
 *   <arg name="Id" type="Integer">A windows identifier. Use -1 when you don't need it.</arg>
 *   <arg name="Bitmap" type="@wxBitmap">The bitmap to display</arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">The position of the control on the given parent</arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">The size of the control</arg>
 *   <arg name="Style" type="Integer" default="wxButton.AUTO_DRAW">The style of the control</arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxBitmapButton object.
 *  </desc>
 * </ctor>
 */
wxBitmapButton* BitmapButton::Construct(JSContext *cx, 
                                        JSObject *obj, 
                                        uintN argc, 
                                        jsval *argv, 
                                        bool WXUNUSED(constructing))
{
    wxBitmapButton *p = new wxBitmapButton();
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

WXJS_BEGIN_METHOD_MAP(BitmapButton)
    WXJS_METHOD("create", create, 3)
WXJS_END_METHOD_MAP()

/***
 * <method name="create">
 *  <function returns="Boolean">
 *   <arg name="Parent" type="@wxWindow">The parent window</arg>
 *   <arg name="Id" type="Integer">A windows identifier. 
 *    Use -1 when you don't need it.</arg>
 *   <arg name="Bitmap" type="@wxBitmap">The bitmap to display</arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *    The position of the control on the given parent</arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *    The size of the control</arg>
 *   <arg name="Style" type="Integer" default="wxButton.AUTO_DRAW">
 *    The style of the control</arg>
 *  </function>
 *  <desc>
 *   Creates a bitmap button.
 *  </desc>
 * </method>
 */
JSBool BitmapButton::create(JSContext *cx,
                            JSObject *obj,
                            uintN argc,
                            jsval *argv,
                            jsval *rval)
{
    wxBitmapButton *p = GetPrivate(cx, obj);
    *rval = JSVAL_FALSE;

    const wxPoint *pt = &wxDefaultPosition;
    const wxSize *size = &wxDefaultSize;
    int style = wxBU_AUTODRAW;

    if ( argc > 6 )
        argc = 6;

    switch(argc)
    {
    case 6:
        if ( ! FromJS(cx, argv[5], style) )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 6, "Integer");
          return JS_FALSE;
        }
        // Walk through
    case 5:
        size = Size::GetPrivate(cx, argv[4]);
        if ( size == NULL )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 5, "wxSize");
          return JS_FALSE;
        }
        // Walk through
    case 4:
        pt = Point::GetPrivate(cx, argv[3]);
        if ( pt == NULL )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 4, "wxPoint");
          return JS_FALSE;
        }
        // Walk through
    default:
        wxBitmap *bmp = Bitmap::GetPrivate(cx, argv[2]);
        if ( bmp == NULL )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 3, "wxBitmap");
          return JS_FALSE;
        }

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

        if ( p->Create(parent, id, *bmp, *pt, *size, style) )
        {
            *rval = JSVAL_TRUE;
            p->SetClientObject(new JavaScriptClientData(cx, obj, true, false));
        }
    }
    return JS_TRUE;
}
