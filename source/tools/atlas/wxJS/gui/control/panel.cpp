#include "precompiled.h"

/*
 * wxJavaScript - panel.cpp
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
 * $Id: panel.cpp 708 2007-05-14 15:30:45Z fbraem $
 */
// panel.cpp

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "../../common/main.h"

#include "../event/jsevent.h"


#include "../misc/size.h"
#include "../misc/point.h"

#include "button.h"
#include "panel.h"
#include "window.h"
#include "../errors.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>control/panel</file>
 * <module>gui</module>
 * <class name="wxPanel" prototype="@wxWindow">
 *  A panel is a window on which controls are placed. It is usually placed within a frame. 
 *  Its main purpose is to be similar in appearance and functionality to a dialog,
 *  but with the flexibility of having any window as a parent.
 *  See @wxDialog and @wxFrame
 * </class>
 */
WXJS_INIT_CLASS(Panel, "wxPanel", 2)

void Panel::InitClass(JSContext* WXUNUSED(cx),
                      JSObject* WXUNUSED(obj), 
                      JSObject* WXUNUSED(proto))
{
  PanelEventHandler::InitConnectEventMap();
}

/***
 * <properties>
 *  <property name="defaultItem" type="@wxButton">
 *   Get/Set the default button.
 *  </property>
 * </properties> 
 */
WXJS_BEGIN_PROPERTY_MAP(Panel)
	WXJS_PROPERTY(P_DEFAULT_ITEM, "defaultItem")
WXJS_END_PROPERTY_MAP()

bool Panel::GetProperty(wxPanel *p,
                        JSContext* WXUNUSED(cx),
                        JSObject* WXUNUSED(obj),
                        int id,
                        jsval *vp)
{
  if ( id == P_DEFAULT_ITEM )
  {
    wxWindow *win = NULL;
   
    #if wxCHECK_VERSION(2,7,0)
      wxTopLevelWindow *tlw = wxDynamicCast(wxGetTopLevelParent(p), 
                                            wxTopLevelWindow);
      if ( tlw )
          win = tlw->GetDefaultItem();
    #else       
      win = p->GetDefaultItem();
    #endif

    if ( win )
    {
      JavaScriptClientData *data 
        = dynamic_cast<JavaScriptClientData*>(win->GetClientObject());
      *vp = data->GetObject() == NULL ? JSVAL_VOID 
                                      : OBJECT_TO_JSVAL(data->GetObject()); 
    }
  }
  return true;
}

bool Panel::SetProperty(wxPanel *p,
                        JSContext *cx,
                        JSObject* WXUNUSED(obj),
                        int id, 
                        jsval *vp)
{
  if ( id == P_DEFAULT_ITEM )
  {
    wxWindow *win = Window::GetPrivate(cx, *vp);
    if ( win != NULL )
    {
      #if wxCHECK_VERSION(2,7,0)
        wxTopLevelWindow *tlw = wxDynamicCast(wxGetTopLevelParent(p), 
                                              wxTopLevelWindow);
        if ( tlw )
          tlw->SetDefaultItem(win);
      #else
        p->SetDefaultItem(win);
      #endif
    } 
  }
  return true;
}

bool Panel::AddProperty(wxPanel *p, 
                        JSContext* WXUNUSED(cx), 
                        JSObject* WXUNUSED(obj), 
                        const wxString &prop, 
                        jsval* WXUNUSED(vp))
{
  if ( WindowEventHandler::ConnectEvent(p, prop, true) )
      return true;
    
  PanelEventHandler::ConnectEvent(p, prop, true);

  return true;
}


bool Panel::DeleteProperty(wxPanel *p, 
                           JSContext* WXUNUSED(cx), 
                           JSObject* WXUNUSED(obj), 
                           const wxString &prop)
{
  if ( WindowEventHandler::ConnectEvent(p, prop, false) )
    return true;
  
  PanelEventHandler::ConnectEvent(p, prop, false);
  return true;
}

/***
 * <ctor>
 *  <function />
 *  <function>
 *   <arg name="Parent" type="@wxWindow">
 *    The parent of wxPanel.
 *   </arg>
 *   <arg name="Id" type="Integer">
 *    A window identifier. Use -1 when you don't need it.
 *   </arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *    The position of the Panel control on the given parent.
 *   </arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *    The size of the Panel control.
 *   </arg>
 *   <arg name="Style" type="Integer" default="0">
 *    The wxPanel style.
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxPanel object.
 *  </desc>
 * </ctor>
 */
wxPanel* Panel::Construct(JSContext *cx,
                          JSObject *obj, 
                          uintN argc,
                          jsval *argv,
                          bool WXUNUSED(constructing))
{
  wxPanel *p = new wxPanel();
  SetPrivate(cx, obj, p);
  if ( argc > 0 )
  {
    jsval rval;
    create(cx, obj, argc, argv, &rval);
  }

  return p;
}

WXJS_BEGIN_METHOD_MAP(Panel)
  WXJS_METHOD("create", create, 2)
WXJS_END_METHOD_MAP()

/***
 * <method name="create">
 *  <function returns="Boolean">
 *   <arg name="Parent" type="@wxWindow">
 *    The parent of wxPanel.
 *   </arg>
 *   <arg name="Id" type="Integer">
 *    A window identifier. Use -1 when you don't need it.
 *   </arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *    The position of the Panel control on the given parent.
 *   </arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *    The size of the Panel control.
 *   </arg>
 *   <arg name="Style" type="Integer" default="0">
 *    The wxPanel style.
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxPanel object.
 *  </desc>
 * </method>
 */
JSBool Panel::create(JSContext *cx,
                     JSObject *obj,
                     uintN argc,
                     jsval *argv,
                     jsval *rval)
{
  wxPanel *p = GetPrivate(cx, obj);
  *rval = JSVAL_FALSE;

  if ( argc > 5 )
      argc = 5;

    int style = wxTAB_TRAVERSAL;
    const wxPoint *pt = &wxDefaultPosition;
    const wxSize *size = &wxDefaultSize;

    switch(argc)
    {
    case 5:
        if ( ! FromJS(cx, argv[4], style) )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 5, "Integer");
          return JS_FALSE;
        }
        // Fall through
    case 4:
		size = Size::GetPrivate(cx, argv[3]);
		if ( size == NULL )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 4, "wxSize");
          return JS_FALSE;
        }
        // Fall through
    case 3:
		pt = Point::GetPrivate(cx, argv[2]);
		if ( pt == NULL )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 3, "wxPoint");
          return JS_FALSE;
        }
        // Fall through
    default:
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

		if ( p->Create(parent, id, *pt, *size, style) )
        {
          *rval = JSVAL_TRUE;
          p->SetClientObject(new JavaScriptClientData(cx, obj, true, false));
        }
    }

    return JS_TRUE;
}

/***
 * <events>
 *  <event name="onSysColourChanged">
 *   To process a system colour changed event, use this property to set
 *   an event handler function. The function takes a @wxSysColourChangedEvent argument.
 *  </event>
 * </events>
 */
WXJS_INIT_EVENT_MAP(wxPanel)
const wxString WXJS_SYS_COLOUR_CHANGED_EVENT = wxT("onSysColourChanged");

void PanelEventHandler::OnSysColourChanged(wxSysColourChangedEvent &event)
{
  PrivSysColourChangedEvent::Fire<SysColourChangedEvent>(event, WXJS_SYS_COLOUR_CHANGED_EVENT);
}

void PanelEventHandler::ConnectSysColourChanged(wxPanel *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_SYS_COLOUR_CHANGED,
               wxSysColourChangedEventHandler(OnSysColourChanged));
  }
  else
  {
    p->Disconnect(wxEVT_SYS_COLOUR_CHANGED, 
                  wxSysColourChangedEventHandler(OnSysColourChanged));
  }
}

void PanelEventHandler::InitConnectEventMap()
{
    AddConnector(WXJS_SYS_COLOUR_CHANGED_EVENT, ConnectSysColourChanged);
}
