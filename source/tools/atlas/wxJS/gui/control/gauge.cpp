#include "precompiled.h"

/*
 * wxJavaScript - gauge.cpp
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
 * $Id: gauge.cpp 810 2007-07-13 20:07:05Z fbraem $
 */
// gauge.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../ext/wxjs_ext.h"

#include "../misc/size.h"
#include "../misc/validate.h"

#include "gauge.h"
#include "window.h"
#include "../errors.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>control/gauge</file>
 * <module>gui</module>
 * <class name="wxGauge" prototype="@wxControl">
 *  A gauge is a horizontal or vertical bar which shows a quantity (often time).
 * </class>
 */
WXJS_INIT_CLASS(Gauge, "wxGauge", 3)

/***
 * <properties>
 *  <property name="bezelFace" type="Integer">
 *   Get/Set the width of the 3D bezel face. <I>Windows only</I>
 *  </property>
 *  <property name="range" type="Integer">
 *   Get/Set the maximum position of the gauge.
 *  </property>
 *  <property name="shadowWidth" type="Integer">
 *   Get/Set the 3D shadow margin width. <I>Windows only</I>
 *  </property>
 *  <property name="value" type="Integer">
 *   Get/Set the current value of the gauge.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(Gauge)
  WXJS_PROPERTY(P_BEZEL_FACE, "bezelFace")
  WXJS_PROPERTY(P_RANGE, "range")
  WXJS_PROPERTY(P_SHADOW_WIDTH, "shadowWidth")
  WXJS_PROPERTY(P_VALUE, "value")
WXJS_END_PROPERTY_MAP()

bool Gauge::GetProperty(wxGauge *p,
                        JSContext *cx,
                        JSObject* WXUNUSED(obj),
                        int id,
                        jsval *vp)
{
    switch (id) 
	{
	case P_BEZEL_FACE:
		*vp = ToJS(cx, p->GetBezelFace());
		break;
	case P_RANGE:
		*vp = ToJS(cx, p->GetRange());
		break;
	case P_SHADOW_WIDTH:
		*vp = ToJS(cx, p->GetShadowWidth());
		break;
	case P_VALUE:
		*vp = ToJS(cx, p->GetValue());
		break;
    }
    return true;
}

bool Gauge::SetProperty(wxGauge *p,
                        JSContext *cx,
                        JSObject* WXUNUSED(obj),
                        int id,
                        jsval *vp)
{
    switch (id) 
	{
	case P_BEZEL_FACE:
		{
			int value;
			if ( FromJS(cx, *vp, value) )
				p->SetBezelFace(value);
			break;
		}
	case P_RANGE:
		{
			int value;
			if ( FromJS(cx, *vp, value) )
				p->SetRange(value);
			break;
		}
	case P_SHADOW_WIDTH:
		{
			int value;
			if ( FromJS(cx, *vp, value) )
				p->SetShadowWidth(value);
			break;
		}
	case P_VALUE:
		{
			int value;
			if ( FromJS(cx, *vp, value) )
				p->SetValue(value);
			break;
		}
	}
    return true;
}

bool Gauge::AddProperty(wxGauge *p, 
                        JSContext* WXUNUSED(cx), 
                        JSObject* WXUNUSED(obj), 
                        const wxString &prop, 
                        jsval* WXUNUSED(vp))
{
  return WindowEventHandler::ConnectEvent(p, prop, true);
}


bool Gauge::DeleteProperty(wxGauge *p, 
                           JSContext* WXUNUSED(cx), 
                           JSObject* WXUNUSED(obj), 
                           const wxString &prop)
{
  return WindowEventHandler::ConnectEvent(p, prop, false);
}

/***
 * <constants>
 *  <type name="Style">
 *   <constant name="HORIZONTAL" />
 *   <constant name="VERTICAL" />
 *   <constant name="PROGRESSBAR" />
 *   <constant name="SMOOTH" />
 *  </type>
 * </constants>
 */
WXJS_BEGIN_CONSTANT_MAP(Gauge)
  WXJS_CONSTANT(wxGA_, HORIZONTAL)
  WXJS_CONSTANT(wxGA_, VERTICAL)
  WXJS_CONSTANT(wxGA_, PROGRESSBAR)
  WXJS_CONSTANT(wxGA_, SMOOTH)
WXJS_END_CONSTANT_MAP()

/***
 * <ctor>
 *  <function />
 *  <function>
 *   <arg name="Parent" type="@wxWindow">
 *    The parent of wxGauge.
 *   </arg>
 *   <arg name="Id" type="Integer">
 *    An window identifier. Use -1 when you don't need it.
 *   </arg>
 *   <arg name="Range" type="Integer">
 *    The maximum value of the gauge
 *   </arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *    The position of the Gauge control on the given parent.
 *   </arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *    The size of the Gauge control.
 *   </arg>
 *   <arg name="Style" type="Integer" default="wxGauge.HORIZONTAL">
 *    The wxGauge style.
 *   </arg>
 *   <arg name="Validator" type="@wxValidator" default="null" />
 *  </function>
 *  <desc>
 *   Constructs a new wxGauge object.
 *  </desc>
 * </ctor>
 */
wxGauge *Gauge::Construct(JSContext *cx,
                          JSObject *obj,
                          uintN argc,
                          jsval *argv,
                          bool WXUNUSED(constructing))
{
  wxGauge *p = new wxGauge();
  SetPrivate(cx, obj, p);

  if ( argc > 0 )
  {
    jsval rval;
    if ( ! create(cx, obj, argc, argv, &rval) )
      return NULL;
  }
  return p;
}

WXJS_BEGIN_METHOD_MAP(Gauge)
  WXJS_METHOD("create", create, 3)
WXJS_END_METHOD_MAP()

/***
 * <method name="create">
 *	<function returns="Boolean">
 *   <arg name="Parent" type="@wxWindow">
 *    The parent of wxGauge.
 *   </arg>
 *   <arg name="Id" type="Integer">
 *    An window identifier. Use -1 when you don't need it.
 *   </arg>
 *   <arg name="Range" type="Integer">
 *    The maximum value of the gauge
 *   </arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *    The position of the Gauge control on the given parent.
 *   </arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *    The size of the Gauge control.
 *   </arg>
 *   <arg name="Style" type="Integer" default="wxGauge.HORIZONTAL">
 *    The wxGauge style.
 *   </arg>
 *   <arg name="Validator" type="@wxValidator" default="null" />
 *  </function>
 *  <desc>
 *	 Creates a wxGauge
 *  </desc>
 * </method>
 */
JSBool Gauge::create(JSContext *cx,
                     JSObject *obj,
                     uintN argc,
                     jsval *argv,
                     jsval *rval)
{
  wxGauge *p = GetPrivate(cx, obj);
  *rval = JSVAL_FALSE;

  int style = 0;
  const wxPoint *pt = &wxDefaultPosition;
  const wxSize *size = &wxDefaultSize;
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
  case 4:
    pt = wxjs::ext::GetPoint(cx, argv[3]);
	if ( pt == NULL )
    {
      JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 4, "wxPoint");
      return JS_FALSE;
    }
  default:
      int range;
      if ( ! FromJS(cx, argv[2], range) )
      {
        JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 3, "Integer");
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

      if ( p->Create(parent, id, range, *pt, *size, style, *val) )
      {
        *rval = JSVAL_TRUE;
        p->SetClientObject(new JavaScriptClientData(cx, obj, true, false));
      }
  }
	
  return JS_TRUE;
}
